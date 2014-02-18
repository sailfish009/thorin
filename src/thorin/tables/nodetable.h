#ifndef THORIN_AIR_NODE
#error "define THORIN_AIR_NODE before including this file"
#endif

// Def
    THORIN_AIR_NODE(Lambda, lambda)
    // PrimOp
        // Literal
            THORIN_AIR_NODE(Any, any)
            THORIN_AIR_NODE(Bottom, bottom)
        // MemOp
            THORIN_AIR_NODE(Map, map)
            THORIN_AIR_NODE(Load, load)
            THORIN_AIR_NODE(Store, store)
            THORIN_AIR_NODE(Enter, enter)
            THORIN_AIR_NODE(Leave, leave)
        THORIN_AIR_NODE(Select, select)
        THORIN_AIR_NODE(Global, global)
        THORIN_AIR_NODE(Slot, slot)
        //THORIN_AIR_NODE(ArithOp)
        //THORIN_AIR_NODE(Cmp)
        //THORIN_AIR_NODE(Conv, conv)
            THORIN_AIR_NODE(Cast, cast)
            THORIN_AIR_NODE(Bitcast, bitcast)
        THORIN_AIR_NODE(ArrayAgg, array)
        THORIN_AIR_NODE(Tuple, tuple)
        THORIN_AIR_NODE(Vector, vector)
        THORIN_AIR_NODE(Extract, extract)
        THORIN_AIR_NODE(Insert, insert)
        THORIN_AIR_NODE(LEA, lea)
        THORIN_AIR_NODE(Run, run)
        THORIN_AIR_NODE(Hlt, hlt)
    THORIN_AIR_NODE(Param, param)
    // Type
        // PrimType
        THORIN_AIR_NODE(Frame, frame)
        THORIN_AIR_NODE(Generic, generic)
        THORIN_AIR_NODE(GenericRef, generic_ref)
        THORIN_AIR_NODE(Mem, mem)
        THORIN_AIR_NODE(Pi, pi)
        THORIN_AIR_NODE(Ptr, ptr)
        THORIN_AIR_NODE(Sigma, sigma)
        THORIN_AIR_NODE(DefArray, def_array)
        THORIN_AIR_NODE(IndefArray, indef_array)

#undef THORIN_AIR_NODE
