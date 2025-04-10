add_library(
    edb_ixm
    ixm.cpp
)

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:edb_ixm>
    PARENT_SCOPE)
    