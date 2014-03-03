#include "impala/type.h"

#include <sstream>

#include "thorin/world.h"

#include "impala/ast.h"
#include "impala/dump.h"

using namespace thorin;

namespace impala {

//------------------------------------------------------------------------------

TypeTable::TypeTable()
    : types_()
#define IMPALA_TYPE(itype, atype) ,itype##_(unify(new PrimType(*this, Token::TYPE_##itype)))
#include "impala/tokenlist.h"
    , type_error_(unify(new TypeError(*this)))
    , noret_(unify(new NoRet(*this)))
    , void_(unify(new Void(*this)))
{}

TypeTable::~TypeTable() {
    for (auto type : types_)
        delete type;
}

const Type* TypeTable::unify_base(const Type* type) {
    auto i = types_.find(type);
    if (i != types_.end()) {
        delete type;
        return *i;
    }

    auto p = types_.insert(type);
    assert(p.second && "hash/equal broken");
    return type;
}

const PrimType* TypeTable::primtype(TokenKind kind) {
    switch (kind) {
#define IMPALA_TYPE(itype, atype) case Token::TYPE_##itype: return itype##_;
#include "impala/tokenlist.h"
        default: THORIN_UNREACHABLE;
    }
}

const DefiniteArray* TypeTable::definite_array(const Type* elem_type, u64 dim) { 
    return unify(new DefiniteArray(*this, elem_type, dim)); 
}
const IndefiniteArray* TypeTable::indefinite_array(const Type* elem_type) { 
    return unify(new IndefiniteArray(*this, elem_type)); 
}
const FnType* TypeTable::fntype(thorin::ArrayRef<const Type*> elems) { return unify(new FnType(*this, elems)); }
const TupleType* TypeTable::tupletype(thorin::ArrayRef<const Type*> elems) { return unify(new TupleType(*this, elems)); }
const Generic* TypeTable::generic(size_t index) { return unify(new Generic(*this, index)); }
const GenericRef* TypeTable::genericref(const Fun* fun, const Generic* generic) { 
    return unify(new GenericRef(*this, fun, generic)); 
}
const IdType* TypeTable::idtype(Symbol symbol) { return unify(new IdType(*this, symbol)); }

//------------------------------------------------------------------------------

size_t GenericBuilder::new_def() {
    size_t handle = index2generic_.size();
    index2generic_.push_back(nullptr);
    return handle;
}

const Generic* GenericBuilder::use(size_t handle) {
    assert(handle < index2generic_.size());
    const Generic*& ref = index2generic_[handle];
    if (auto generic = ref)
        return generic;

    return ref = typetable().generic(index_++);
}

//------------------------------------------------------------------------------

const Type*& GenericMap::operator [] (const Generic* generic) const {
    size_t i = generic->index();
    if (i >= types_.size())
        types_.resize(i+1, nullptr);
    return types_[i];
}

bool GenericMap::is_empty() const {
    for (size_t i = 0, e = types_.size(); i != e; ++i)
        if (!types_[i])
            return false;

    return true;
}

std::string GenericMap::to_string() const {
    std::ostringstream o;
    bool first = true;
    for (size_t i = 0, e = types_.size(); i != e; ++i) {
        if (auto type = types_[i]) {
            if (first)
                first = false;
            else
                o << ", ";
            o << Generic::to_string(i) << " = " << type;
        }
    }

    return o.str();
}

const GenericRef* Generic::genericref(const Fun* fun) const { return typetable_.genericref(fun, this); }

//------------------------------------------------------------------------------

bool Type::is_bool() const { 
    if (auto pt = isa<PrimType>()) 
        return pt->kind() == Token::TYPE_bool; 
    return false; 
}

bool Type::is_int() const {
    if (auto pt = isa<PrimType>()) {
        switch (pt->kind()) {
            case Token::TYPE_int8:
            case Token::TYPE_int16:
            case Token::TYPE_int32:
            case Token::TYPE_int64:
            case Token::TYPE_int:   return true;
            default:                return false;
        }
    }
    return false;
}

bool Type::is_float() const {
    if (auto pt = isa<PrimType>()) {
        switch (pt->kind()) {
            case Token::TYPE_float:
            case Token::TYPE_double: return true;
            default:                 return false;
        }
    }
    return false;
}

const Type* FnType::return_type() const {
    if (!empty()) {
        if (auto fn = elems().back()->isa<FnType>()) {
            switch (fn->size()) {
                case 0: return typetable_.type_void();
                case 1: return fn->elem(0);
            }
        }
    }

    return typetable_.noret();
}

bool Type::check_with(const Type* other) const {
    if (this == other || this->isa<Generic>() || this->isa<GenericRef>())
        return true;

    if (this->kind() != other->kind() || this->size() != other->size())
        return false;

    for (size_t i = 0, e = size(); i != e; ++i)
        if (!this->elem(i)->check_with(other->elem(i)))
            return false;

    return true;
}

bool Type::infer_with(GenericMap& map, const Type* other) const {
    if (auto genericref = this->isa<GenericRef>())
        return genericref->generic()->infer_with(map, other);
    if (auto genericref = other->isa<GenericRef>())
        other = genericref->generic();

    size_t num_elems = this->size();
    assert(this->isa<Generic>() || num_elems == other->size());
    assert(this->isa<Generic>() || this->kind() == other->kind());

    if (this == other)
        return true;

    if (auto generic = this->isa<Generic>()) {
        const Type*& mapped = map[generic];
        if (!mapped) {
            mapped = other;
            return true;
        } else
            return mapped == other;
    }

    for (size_t i = 0; i < num_elems; ++i) {
        if (!this->elem(i)->infer_with(map, other->elem(i)))
            return false;
    }

    return true;
}

/*static*/ std::string Generic::to_string(size_t index) {
    std::ostringstream oss;
    if (index < 26)
        oss << char('a' + index) << '\'';
    else
        oss << 'T' << index << '\'';
    return oss.str();
}

#define THORIN_REFINE_SPECIALIZE(T, constr) \
    const Type* T::refine(const Sema& sema) const { \
        thorin::Array<const Type*> nelems(size()); \
        for (size_t i = 0, e = size(); i != e; ++i) \
            nelems[i] = elem(i)->refine(sema); \
        return typetable_.constr(nelems); \
    } \
    const Type* T::specialize(const GenericMap& map) const { \
        thorin::Array<const Type*> nelems(size()); \
        for (size_t i = 0, e = size(); i != e; ++i) \
            nelems[i] = elem(i)->specialize(map); \
        return typetable_.constr(nelems); \
    }

THORIN_REFINE_SPECIALIZE(TupleType, tupletype)
THORIN_REFINE_SPECIALIZE(FnType, fntype)

const Type* DefiniteArray::refine(const Sema& sema) const {
    return typetable_.definite_array(elem_type()->refine(sema), dim());
}

const Type* IndefiniteArray::refine(const Sema& sema) const {
    return typetable_.indefinite_array(elem_type()->refine(sema));
}

const Type* DefiniteArray::specialize(const GenericMap& map) const {
    return typetable_.definite_array(elem_type()->specialize(map), dim());
}

const Type* IndefiniteArray::specialize(const GenericMap& map) const {
    return typetable_.indefinite_array(elem_type()->specialize(map));
}

//------------------------------------------------------------------------------

const thorin::Type* PrimType::convert(World& world) const {
    switch (kind()) {
#define IMPALA_TYPE(itype, atype) case Token::TYPE_##itype: return world.type_##atype();
#include "impala/tokenlist.h"
        default: THORIN_UNREACHABLE;
    }
}

const thorin::Type* DefiniteArray::convert(World& world) const {
    return world.def_array(elem_type()->convert(world), dim());
}

const thorin::Type* IndefiniteArray::convert(World& world) const {
    return world.ptr(world.indef_array(elem_type()->convert(world)));
}

const thorin::Type* FnType::convert(World& world) const {
    Array<const thorin::Type*> elems(size() + 1);
    elems[0] = world.mem();
    for (size_t i = 1, e = elems.size(); i != e; ++i)
        elems[i] = elem(i-1)->convert(world);

    return world.pi(elems);
}

const thorin::Type* TupleType::convert(World& world) const {
    Array<const thorin::Type*> elems(size());
    for (size_t i = 0, e = elems.size(); i != e; ++i)
        elems[i] = elem(i)->convert(world);

    return world.sigma(elems);
}

const thorin::Type* Generic::convert(thorin::World& world) const {
    return world.generic(index());
}

const thorin::Type* GenericRef::convert(thorin::World& world) const {
    return world.generic_ref(generic()->convert(world)->as<thorin::Generic>(), fun()->lambda());
}

//------------------------------------------------------------------------------

} // namespace impala
