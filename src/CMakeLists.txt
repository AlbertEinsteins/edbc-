add_subdirectory(pmd)


add_library(edblib STATIC ${ALL_OBJECT_FILES})

# Set link livs
set(EDB_LIBS edb_pmd)



find_package(Threads REQUIRED)

target_link_libraries(edblib ${EDB_LIBS})

target_include_directories(edblib PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)