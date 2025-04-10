add_library(
    edb_msg
    msg.cpp
)

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:edb_msg>
    PARENT_SCOPE)