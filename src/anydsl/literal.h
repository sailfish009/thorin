#ifndef ANYDSL_LITERAL_H
#define ANYDSL_LITERAL_H

#include <vector>

#include "anydsl/primop.h"
#include "anydsl/type.h"
#include "anydsl/util/box.h"

namespace anydsl {

class Type;
class World;

//------------------------------------------------------------------------------

class Literal : public PrimOp {
protected:

    Literal(int kind, const Type* type)
        : PrimOp(kind, type, 0)
    {}
};

//------------------------------------------------------------------------------

class Undef : public Literal {
private:

    Undef(const Type* type)
        : Literal(Node_Undef, type)
    {}

    virtual void vdump(Printer& printer) const ;

    friend class World;

};

//------------------------------------------------------------------------------

class Error : public Literal {
private:

    Error(const Type* type)
        : Literal(Node_Error, type)
    {}

    virtual void vdump(Printer& printer) const ;

    friend class World;
};

//------------------------------------------------------------------------------

class PrimLit : public Literal {
private:

    PrimLit(const Type* type, Box box)
        : Literal(Node_PrimLit, type)
        , box_(box)
    {}

public:

    Box box() const { return box_; }
    const PrimType* primtype() const { return type()->as<PrimType>(); }
    PrimTypeKind primtype_kind() const { return primtype()->primtype_kind(); }

    virtual bool equal(const Def* other) const;
    virtual size_t hash() const;

private:

    virtual void vdump(Printer& printer) const ;

    Box box_;

    friend class World;
};

//------------------------------------------------------------------------------

} // namespace anydsl

#endif
