IF ( LLVM_FOUND )
    FUNCTION ( get_thorin_llvm_dependency_libs OUT_VAR )
    llvm_map_components_to_libnames ( THORIN_LLVM_TEMP_LIBS core mc native ipo irreader bitreader bitwriter scalaropts support )
    SET ( ${OUT_VAR} ${THORIN_LLVM_TEMP_LIBS} PARENT_SCOPE )
    ENDFUNCTION ()
ELSE ()
    FUNCTION ( GET_THORIN_LLVM_DEPENDENCY_LIBS OUT_VAR )
        SET ( ${OUT_VAR} PARENT_SCOPE )
    ENDFUNCTION ()
ENDIF ()

IF ( WFV2_FOUND )
    FUNCTION ( GET_THORIN_WFV2_DEPENDENCY_LIBS OUT_VAR )
        GET_WFV2_LLVM_DEPENDENCY_LIBS ( THORIN_WFV2_TEMP_LIBS )
        SET ( ${OUT_VAR} ${WFV2_LIBRARIES} ${THORIN_WFV2_TEMP_LIBS} PARENT_SCOPE )
    ENDFUNCTION ()
ELSE ()
    FUNCTION ( GET_THORIN_WFV2_DEPENDENCY_LIBS OUT_VAR )
        SET ( ${OUT_VAR} PARENT_SCOPE )
    ENDFUNCTION ()
ENDIF ()

FUNCTION ( GET_THORIN_DEPENDENCY_LIBS OUT_VAR )
    GET_THORIN_LLVM_DEPENDENCY_LIBS ( THORIN_LLVM_TEMP_LIBS )
    GET_THORIN_WFV2_DEPENDENCY_LIBS ( THORIN_WFV2_TEMP_LIBS )
    SET ( ${OUT_VAR} ${THORIN_LLVM_TEMP_LIBS} ${THORIN_WFV2_TEMP_LIBS} PARENT_SCOPE )
ENDFUNCTION ()
