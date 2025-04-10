add_library(
    edb_rtn
    rtn.cpp
)

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:edb_rtn>
    PARENT_SCOPE)