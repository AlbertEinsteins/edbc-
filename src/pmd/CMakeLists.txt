add_library(
    edb_pmd
    pmdMain.cpp   
)

set(ALL_OBJECT_FILES
    ${ALL_OBJECTS_FILES} $<TARGET_OBJECTS:edb_pmd>
    PARENT_SCOPE)