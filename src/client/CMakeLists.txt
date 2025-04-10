add_library(
    edb_client
    command.cpp
    commandFactory.cpp
    edb.cpp
)


set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:edb_client>
    PARENT_SCOPE)