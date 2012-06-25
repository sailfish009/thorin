#ifndef ANYDSL_DUMP_H
#define ANYDSL_DUMP_H

#include <iostream>

namespace anydsl {

class AIRNode;

void dump(const AIRNode* n, std::ostream& o = std::cout);

} // namespace anydsl

#endif
