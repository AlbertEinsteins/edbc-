add_library(
    edb_dms
    dms.cpp
)

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:edb_dms>
    PARENT_SCOPE)