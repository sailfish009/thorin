#include "thorin/pass/mem2reg.h"

#include "thorin/util.h"

namespace thorin {

/*
 * helpers
 */

static const Def* proxy_type(const Analyze* proxy) { return as<Tag::Ptr>(proxy->type())->arg(0); }
static std::tuple<Lam*, const Analyze*> split_virtual_phi(const Analyze* proxy) { return {proxy->op(0)->as_nominal<Lam>(), proxy->op(1)->as<Analyze>()}; }

const Analyze* Mem2Reg::isa_proxy(const Def* def) {
    if (auto analyze = isa<Analyze>(index(), def); analyze && !analyze->op(1)->isa<Analyze>()) return analyze;
    return nullptr;
}

const Analyze* Mem2Reg::isa_virtual_phi(const Def* def) {
    if (auto analyze = isa<Analyze>(index(), def); analyze && analyze->op(1)->isa<Analyze>()) return analyze;
    return nullptr;
}

/*
 * get_val/set_val
 */

const Def* Mem2Reg::get_val(Lam* lam, const Analyze* proxy) {
    const auto& info = lam2info_[lam];
    if (auto val = info.proxy2val.lookup(proxy)) {
        world().DLOG("get_val {} for {}: {}", lam, proxy, *val);
        return *val;
    }

    switch (info.lattice) {
        case Info::Preds0: return world().bot(proxy_type(proxy));
        case Info::Preds1:
                           world().DLOG("get_val pred: {}: {} -> {}", proxy, lam, info.pred);
                           return get_val(info.pred, proxy);
        default: {
            auto old_lam = original(lam);
            world().DLOG("virtual phi: {}/{} for {}", old_lam, lam, proxy);
            return set_val(lam, proxy, world().analyze(proxy_type(proxy), {old_lam, proxy}, index(), {"phi"}));
        }
    }
}

const Def* Mem2Reg::set_val(Lam* lam, const Analyze* proxy, const Def* val) {
    world().DLOG("set_val {} for {}: {}", lam, proxy, val);
    return lam2info_[lam].proxy2val[proxy] = val;
}

/*
 * PassMan hooks
 */

bool Mem2Reg::enter(Def* nom) {
    return nom->isa<Lam>();
#if 0
    if (auto lam = nom->isa<Lam>()) {
        auto orig = original(lam);
        if (orig != lam) {
            lam->set(orig->ops());
            // TODO move phi stuff here
        }
        return true;
    }

    return false;
#endif
}

Def* Mem2Reg::inspect(Def* def) {
    if (auto old_lam = def->isa<Lam>()) {
        auto& info = lam2info_[old_lam];
        if (preds_n_.contains(old_lam)) info.lattice = Info::PredsN;
        if (keep_   .contains(old_lam)) info.lattice = Info::Keep;

        if (old_lam->is_external() || old_lam->intrinsic() != Lam::Intrinsic::None) {
            info.lattice = Info::Keep;
        } else if (info.lattice != Info::Keep) {
            auto& phis = lam2phis_[old_lam];

            if (info.lattice == Info::PredsN && !phis.empty()) {
                std::vector<const Def*> types;
                for (auto i = phis.begin(); i != phis.end();) {
                    auto proxy = *i;
                    if (keep_.contains(proxy)) {
                        i = phis.erase(i);
                    } else {
                        types.emplace_back(proxy_type(proxy));
                        ++i;
                    }
                }
                //Array<const Def*> types(phis.size(), [&](auto) { return proxy_type(*phi++); });
                auto new_domain = merge_sigma(old_lam->domain(), types);
                auto new_lam = world().lam(world().pi(new_domain, old_lam->codomain()), old_lam->debug());
                world().DLOG("new_lam: {} -> {}", old_lam, new_lam);
                new2old_[new_lam] = old_lam;

                size_t n = new_lam->num_params() - phis.size();
                auto new_param = world().tuple(Array<const Def*>(n, [&](auto i) { return new_lam->param(i); }));
                man().local_map(old_lam->param(), new_param);

                info.new_lam = new_lam;
                lam2info_[new_lam].lattice = Info::PredsN;
                new_lam->set(old_lam->ops());

                size_t i = 0;
                for (auto phi : phis)
                    set_val(new_lam, phi, new_lam->param(n + i++));

                return new_lam;
            }
        }
    }

    return def;
}

const Def* Mem2Reg::rewrite(const Def* def) {
    if (auto slot = isa<Tag::Slot>(def)) {
        auto [out_mem, out_ptr] = slot->split<2>();
        auto orig = original(man().cur_nom<Lam>());
        auto slot_id = lam2info_[orig].num_slots++;
        auto proxy = world().analyze(out_ptr->type(), {orig, world().lit_nat(slot_id)}, index(), slot->debug());
        if (!keep_.contains(proxy)) {
            set_val(proxy, world().bot(proxy_type(proxy)));
            lam2info_[man().cur_nom<Lam>()].writable.emplace(proxy);
            return world().tuple({slot->arg(), proxy});
        }
    } else if (auto load = isa<Tag::Load>(def)) {
        auto [mem, ptr] = load->args<2>();
        if (auto proxy = isa_proxy(ptr))
            return world().tuple({mem, get_val(proxy)});
    } else if (auto store = isa<Tag::Store>(def)) {
        auto [mem, ptr, val] = store->args<3>();
        if (auto proxy = isa_proxy(ptr)) {
            if (lam2info_[man().cur_nom<Lam>()].writable.contains(proxy)) {
                set_val(proxy, val);
                return mem;
            }
        }
    } else if (auto app = def->isa<App>()) {
        if (auto lam = app->callee()->isa_nominal<Lam>(); lam && !man().outside(lam)) {
            const auto& info = lam2info_[lam];
            if (auto new_lam = info.new_lam) {
                auto& phis = lam2phis_[lam];
                auto phi = phis.begin();
                Array<const Def*> args(phis.size(), [&](auto) { return get_val(*phi++); });
                return world().app(new_lam, merge_tuple(app->arg(), args));
            }
        }
    }

    return def;
}

bool Mem2Reg::analyze(const Def* def) {
    if (def->isa<Param>()) return true;

    // we need to install a phi in lam next time around
    if (auto phi = isa_virtual_phi(def)) {
        auto [phi_lam, proxy] = split_virtual_phi(phi);

        auto& phi_info = lam2info_[phi_lam];
        auto& phis = lam2phis_[phi_lam];

        if (phi_info.lattice == Info::Keep) {
            if (keep_.emplace(proxy).second) {
                world().DLOG("keep 1: {}", proxy);
                if (auto i = phis.find(proxy); i != phis.end())
                    phis.erase(i);
            }
        } else {
            assert(phi_info.lattice == Info::PredsN);
            assertf(phis.find(proxy) == phis.end(), "already added proxy {} to {}", proxy, phi_lam);
            phis.emplace(proxy);
            auto lam = proxy->op(0)->as_nominal<Lam>();
            world().DLOG("Preds1 -> PredsN {}; phi needed: {} ", lam, phi);
            lam2info_[lam].lattice = Info::PredsN;
            preds_n_.emplace(lam);
        }
        return false;
    }

    if (isa_proxy(def)) return true;

    for (size_t i = 0, e = def->num_ops(); i != e; ++i) {
        auto op = def->op(i);

        if (auto proxy = isa_proxy(op)) {
            if (keep_.emplace(proxy).second) {
                world().DLOG("keep 2: {}", proxy);
                return false;
            }
        } else if (auto lam = op->isa_nominal<Lam>(); lam && !man().outside(lam)) {
            // TODO optimize
            if (lam->is_basicblock() && lam != man().cur_nom<Lam>())
                lam2info_[lam].writable.insert_range(range(lam2info_[man().cur_nom<Lam>()].writable));
            auto orig = original(lam);
            auto& info = lam2info_[orig];
            auto& phis = lam2phis_[orig];
            auto pred = man().cur_nom<Lam>();

            switch (info.lattice) {
                case Info::Preds0:
                    info.lattice = Info::Preds1;
                    info.pred = pred;
                    assert(phis.empty());
                    break;
                case Info::Preds1:
                    info.lattice = Info::PredsN;
                    preds_n_.emplace(orig);
                    // TODO maybe only do a retry if no phis present or sth like this
                    world().DLOG("Preds1 -> PredsN: {}", orig);
                    return false;
                default:
                    break;
            }

            // if lam does not occur as callee and has more than one pred
            if ((!def->isa<App>() || i != 0) && (info.lattice == Info::PredsN )) {
                info.lattice = Info::Keep;
                world().DLOG("keep 3: {}", lam);
                keep_.emplace(lam);
                for (auto phi : phis) keep_.emplace(phi);
                phis.clear();
                return false;
            }
        }
    }

    return true;
}

void Mem2Reg::retry() {
    lam2info_.clear();
}

void Mem2Reg::clear() {
    retry();
    new2old_.clear();
    lam2phis_.clear();
    keep_.clear();
    preds_n_.clear();
}

}
