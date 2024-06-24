cmake_minimum_required(VERSION 3.20)
project(chaotic_core)

set(CMAKE_CXX_STANDARD 20)

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

function(add_files VAR)
    set(headers ${${VAR}})
    set(sources ${${VAR}})
    foreach (file ${ARGN})
        set(headers ${headers} ${file}.h)
        set(sources ${sources} ${file}.cpp)
    endforeach ()
    set(${VAR} ${headers} ${sources} PARENT_SCOPE)
endfunction()


function(add_files_c VAR)
    set(headers ${${VAR}})
    set(sources ${${VAR}})
    foreach (file ${ARGN})
        set(headers ${headers} ${file}.h)
        set(sources ${sources} ${file}.c)
    endforeach ()
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
        process_units
        group
        internal_common
)

add_library(
        chaotic_core SHARED ${CORE_FILES}
        common.h
        chaotic_api_types.h
        operations.cpp
        playerop.cpp
        processor.cpp
        protocol_buffers/messages.pb.cc
        protocol_buffers/common.pb.cc
        protocol_buffers/query.pb.cc
)


add_executable(chaotic_core_test test/test.cpp)

add_library(pyChaotic SHARED python_modules/py_ChaoticCore.cpp)

find_package(
        SQLite3
        REQUIRED
        PATH
)
if (SQLITE3_FOUND)
    include_directories(${SQLite3_INCLUDE_DIR})
    target_link_libraries(chaotic_core_test ${SQLite3_LIBRARY} chaotic_core)
endif (SQLITE3_FOUND)

include(CMakePrintHelpers)


find_package(Protobuf REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development)

if (Python3_FOUND)
    include_directories(${Python3_INCLUDE_DIRS})
    target_link_libraries(pyChaotic ${Python3_LIBRARIES} chaotic_core)
endif (Python3_FOUND)

if (Protobuf_FOUND)
    include_directories(${Protobuf_INCLUDE_DIRS})
    target_link_libraries(chaotic_core ${Protobuf_LIBRARIES})
endif (Protobuf_FOUND)

