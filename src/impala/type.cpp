#include "impala/type.h"

#include <iterator>
#include <boost/functional/hash.hpp>

#include "anydsl/util/cast.h"
#include "anydsl/util/for_all.h"

using anydsl::dcast;

namespace impala {

bool PrimType::equal(const Type* t) const {
    if (const PrimType* p = t->isa<PrimType>())
        return kind() == p->kind();

    return false;
}

size_t PrimType::hash() const {
    return boost::hash_value(kind());
}

bool TypeError::equal(const Type* t) const {
    return t->isa<TypeError>();
}

size_t TypeError::hash() const {
    return boost::hash_value(Token::END_OF_FILE);
}

bool Void::equal(const Type* t) const {
    return t->isa<Void>();
}

size_t Void::hash() const {
    return boost::hash_value(Token::TYPE_void);
}

bool NoRet::equal(const Type* t) const {
    return t->isa<NoRet>();
}

size_t NoRet::hash() const {
    return boost::hash_value(Token::TYPE_noret);
}

Pi::Pi(anydsl::ArrayRef<const Type*> elems, const Type* ret) 
    : elems_(elems)
    , ret_(ret)
{}

size_t Pi::hash() const {
    size_t seed = 0;
    boost::hash_combine(seed, Token::PI);
    boost::hash_combine(seed, elems_);
    boost::hash_combine(seed, ret_);

    return seed;
}

bool Pi::equal(const Type* other) const {
    if (const Pi* pi = other->isa<Pi>())
        return elems_ == pi->elems_ && ret_ == pi->ret();

    return false;
}

Sigma::Sigma(anydsl::ArrayRef<const Type*> elems)
    : elems_(elems)
{}

size_t Sigma::hash() const {
    size_t seed = 0;
    boost::hash_combine(seed, Token::SIGMA);
    boost::hash_combine(seed, elems_);

    return seed;
}

bool Sigma::equal(const Type* other) const {
    if (const Sigma* sigma = other->isa<Sigma>())
        return elems_ == sigma->elems_;

    return false;
}

//------------------------------------------------------------------------------

TypeTable::TypeTable() 
    : type_error_((*types_.insert(new TypeError()).first)->as<TypeError>())
    , type_void_ ((*types_.insert(new Void()).first)->as<Void>())
    , noret_((*types_.insert(new NoRet()).first)->as<NoRet>())
#define IMPALA_TYPE(itype, atype) \
    , itype##_((*types_.insert(new PrimType(PrimType::TYPE_##itype)).first)->as<PrimType>())
#include "impala/tokenlist.h"
{}

TypeTable::~TypeTable() {
    for_all (type, types_)
        delete type;
}

const PrimType* TypeTable::type(PrimType::Kind kind) {
    switch (kind) {
#define IMPALA_TYPE(itype, atype) \
        case PrimType::TYPE_##itype: return itype##_;
#include "impala/tokenlist.h"
        default: ANYDSL_UNREACHABLE;
    }
}

const Pi* TypeTable::pi(anydsl::ArrayRef<const Type*> elems, const Type* ret) {
    const Pi* pi = new Pi(elems, ret);
    TypeSet::iterator i = types_.find(pi);

    if (i == types_.end()) {
        types_.insert(pi);
        return pi;
    }

    delete pi;
    return (*i)->as<Pi>();
}

const Sigma* TypeTable::sigma(anydsl::ArrayRef<const Type*> elems) {
    const Sigma* sigma = new Sigma(elems);
    TypeSet::iterator i = types_.find(sigma);

    if (i == types_.end()) {
        types_.insert(sigma);
        return sigma;
    }

    delete sigma;
    return (*i)->as<Sigma>();
}

} // namespace impala
