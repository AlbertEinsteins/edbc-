cmake_minimum_required(VERSION 3.21)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 14)
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
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unused-parameter -Wno-attributes") # TODO: remove
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -fsanitize=${REDBASE_SANITIZER} -fno-limit-debug-info -fno-omit-frame-pointer -fno-optimize-sibling-calls")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")


# Set Include
set(SRC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/include)
# set(TEST_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/test/include)
set(THIRD_PARTY_DIR
                    ${PROJECT_SOURCE_DIR}/boost_1_82_0
                    ${PROJECT_SOURCE_DIR}/json/include)

include_directories(${SRC_INCLUDE_DIR} ${THIRD_PARTY_DIR})


add_subdirectory(third_party/json)
add_subdirectory(src)
# add_subdirectory(test)


########## Make Target ###########

                    



add_executable(edb src/pmd/pmdMain.cpp)
target_link_libraries(edb edblib)