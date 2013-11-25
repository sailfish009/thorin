#ifndef THORIN_ANALYSES_SCHEDULE_H
#define THORIN_ANALYSES_SCHEDULE_H

#include <vector>

#include "thorin/util/array.h"

namespace thorin {

class Scope;
class PrimOp;

typedef LambdaMap<std::vector<const PrimOp*>> Schedule;

Schedule schedule_early(const Scope&);
Schedule schedule_late(const Scope&, DefSet&);
inline Schedule schedule_late(const Scope& scope) { DefSet set; return schedule_late(scope, set); }
Schedule schedule_smart(const Scope&);

} // namespace thorin

#endif
