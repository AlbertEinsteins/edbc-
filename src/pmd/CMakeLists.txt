add_library(
    edb_pmd
    pmdMain.cpp  
    pmdListener.cpp 
    pmdOptions.cpp
    pmd.cpp
    pmdEdu.cpp
    pmdEduMgr.cpp
    pmdAgent.cpp
)

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:edb_pmd>
    PARENT_SCOPE)