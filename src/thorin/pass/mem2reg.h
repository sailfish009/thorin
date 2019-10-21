#ifndef THORIN_PASS_MEM2REG_H
#define THORIN_PASS_MEM2REG_H

#include <set>

#include "thorin/pass/pass.h"
#include "thorin/util/bitset.h"

namespace thorin {

/**
 * SSA construction algorithm that promotes @p Slot%s, @p Load%s, and @p Store%s to SSA values.
 * This is loosely based upon:
 * "Simple and Efficient Construction of Static Single Assignment Form"
 * by Braun, Buchwald, Hack, Leißa, Mallon, Zwinkau
 */
class Mem2Reg : public Pass {
public:
    Mem2Reg(PassMan& man, size_t index)
        : Pass(man, index)
    {}

    bool enter_scope(Def*) override;
    bool enter_nominal(Def*) override;
    void inspect(Def*) override;
    const Def* rewrite(const Def*) override;
    bool analyze(const Def*) override;

    struct Info {
        enum Lattice { Preds0, Preds1, PredsN, Keep };

        Info()
            : lattice(Preds0)
        {}

        GIDMap<const Analyze*, const Def*> proxy2val;
        GIDSet<const Analyze*> writable;
        Lam* pred = nullptr;
        Lam* new_lam = nullptr;
        unsigned num_slots = 0;
        unsigned lattice :  2;
    };

private:
    const Analyze* isa_proxy(const Def*);
    const Analyze* isa_virtual_phi(const Def*);
    const Def* get_val(Lam*, const Analyze*);
    const Def* get_val(const Analyze* proxy) { return get_val(man().new_entry<Lam>(), proxy); }
    const Def* set_val(Lam*, const Analyze*, const Def*);
    const Def* set_val(const Analyze* proxy, const Def* val) { return set_val(man().new_entry<Lam>(), proxy, val); }

    Lam* original(Lam* new_lam) {
        if (auto old_lam = new2old_.lookup(new_lam)) return *old_lam;
        return new_lam;
    }

    LamMap<Info> lam2info_;
    LamMap<Lam*> new2old_;
    LamMap<std::set<const Analyze*, GIDLt<const Analyze*>>> lam2phis_;
    DefSet keep_;
    LamSet preds_n_;
};

}

#endif
