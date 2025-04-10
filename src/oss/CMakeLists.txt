add_library(
    edb_oss
    ossSocket.cpp
    ossFileOp.cpp
    ossMmapFile.cpp
)


set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:edb_oss>
    PARENT_SCOPE)