cmake_minimum_required(VERSION 3.21)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(
    EDatabase
    VERSION 1.0
    DESCRIPTION "The DBMS"
    LANGUAGES C CXX
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        message(STATUS "Setting build type to `Debug` as none was specified.")
        set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}  -Wno-unused-parameter-Wno-attributes -Wunused-parameter") # TODO: remove
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")


# Set Include
set(SRC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/include)
# set(TEST_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/test/include)
set(THIRD_PARTY_DIR
                    ${PROJECT_SOURCE_DIR}/third_party/json/include
                    ${PROJECT_SOURCE_DIR}/third_party/fmt/include
                    ${PROJECT_SOURCE_DIR}/third_party/argparse/include)

include_directories(${SRC_INCLUDE_DIR} ${THIRD_PARTY_DIR})


add_subdirectory(third_party/json)
add_subdirectory(third_party/fmt)
add_subdirectory(third_party/argparse)

add_subdirectory(src)
# add_subdirectory(test)


########## Make Target ###########                    



add_executable(edb src/pmd/pmdMain.cpp)
target_link_libraries(edb edblib)

add_executable(edb-client src/client/edb.cpp)
target_link_libraries(edb-client edblib)
