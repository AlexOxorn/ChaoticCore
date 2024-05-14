cmake_minimum_required(VERSION 3.20)
project(chaotic_core)

set(CMAKE_CXX_STANDARD 17)

function(add_files VAR)
    set(headers ${${VAR}})
    set(sources ${${VAR}})
    foreach (file ${ARGN})
        set(headers ${headers} ${file}.h)
        set(sources ${sources} ${file}.cpp)
    endforeach()
    set(${VAR} ${headers} ${sources} PARENT_SCOPE)
endfunction()


function(add_files_c VAR)
    set(headers ${${VAR}})
    set(sources ${${VAR}})
    foreach (file ${ARGN})
        set(headers ${headers} ${file}.h)
        set(sources ${sources} ${file}.c)
    endforeach()
    set(${VAR} ${headers} ${sources} PARENT_SCOPE)
endfunction()

add_files(
        CORE_FILES
        card
        lua_obj
        effect
        field
        match
        chaotic_api
)

add_library(chaotic_core SHARED ${CORE_FILES} library.cpp common.h internal_common.h chaotic_api_types.h)


add_executable(chaotic_core_test test/test.cpp)
find_package(
        SQLite3
        REQUIRED
        PATH
)
if (SQLITE3_FOUND)
    include_directories(${SQLite3_INCLUDE_DIR})
    target_link_libraries (chaotic_core_test ${SQLite3_LIBRARY} chaotic_core)
endif (SQLITE3_FOUND)

