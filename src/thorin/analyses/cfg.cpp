#include "thorin/analyses/cfg.h"
#include "thorin/analyses/scope.h"

#include <iostream>

namespace thorin {

CFG::CFG(const Scope& scope) 
    : scope_(scope)
    , lambdas_(scope.rpo())
{
    cfa();
}

struct FlowVal {
    LambdaSet lambdas;
    bool top = false;
    bool join(const FlowVal& other) {
        bool result = false;
        for (auto l : other.lambdas)
            result |= this->lambdas.insert(l).second;
        return result;
    }
};

void CFG::cfa() {
    DefMap<FlowVal> param2fv;

    // init
    for (auto lambda : scope()) {
        if (!lambda->empty()) {
            if (auto to = lambda->to()->isa_lambda()) {
                for (size_t i = 0, e = lambda->num_args(); i != e; ++i) {
                    if (auto arg = lambda->arg(i)->isa_lambda())
                        param2fv[to->param(i)].lambdas.insert(arg);
                }
            }
        }
    }

    // keep iterating to collect param flow infos until things are stable
    bool todo;
    do {
        todo = false;
        for (auto lambda : scope()) {
            if (!lambda->empty()) {
                if (auto to = lambda->to()->isa_lambda()) {
                    for (size_t i = 0, e = lambda->num_args(); i != e; ++i) {
                        if (auto arg = lambda->arg(i)->isa<Param>())
                            todo |= param2fv[to->param(i)].join(param2fv[arg]);
                    }
                }
            }
        }
    } while (todo);
}

}
