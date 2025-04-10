add_subdirectory(pmd)
add_subdirectory(oss)
add_subdirectory(client)
add_subdirectory(pd)
add_subdirectory(msg)
add_subdirectory(dms)
add_subdirectory(rtn)
add_subdirectory(ixm)

add_library(edblib STATIC ${ALL_OBJECT_FILES})
find_package(Threads REQUIRED)

# Set link libs
set(EDB_LIBS 
    edb_pmd
    edb_dms
    edb_oss
    edb_client
    edb_pd
    edb_msg
    edb_rtn
    edb_ixm
    fmt
    argparse)



target_link_libraries(edblib ${EDB_LIBS})
