cmake_minimum_required(VERSION 3.2)
project(match_tree)
#enable_language(ASM)

#from github.com/zsummer

# plat : WIN32 APPLE UNIX    (UNIX contain UNIX like)

# CMAKE_CXX_COMPILER_ID:  GNU Intel Clang AppleClang MSVC 
# example IF (CMAKE_CXX_COMPILER_ID MATCHES "Clang") ENDIF()
# SET(CMAKE_C_COMPILER /usr/bin/gcc-8)
# SET(CMAKE_CXX_COMPILER /usr/bin/g++-8)
# jump compiler works check
# if(WIN32)
#    set(CMAKE_C_COMPILER_WORKS TRUE)
#    set(CMAKE_CXX_COMPILER_WORKS TRUE)
# endif(WIN32)

# jump this project build when msvc 
# set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

# set this project setup build when msvc 
# set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# show msvc folder
#  SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 

# use C++ 14
# set(CMAKE_CXX_FLAGS -std=c++14) 

# 
# CMAKE_SOURCE_DIR   cmake root dir 
# CMAKE_CURRENT_SOURCE_DIR current cmakelist.txt dir  
# EXECUTABLE_OUTPUT_PATH can set it change bin out dir
# CMAKE_MODULE_PATH can set it change module dir 
# PROJECT_NAME cur project name 

# include 
# include_directories  
# link_directories 
# link_libraries 

# 
# execute_process
set(CMAKE_BUILD_TYPE Debug CACHE STRING "cache debug release "  )

#分组添加到IDE
macro(GROUP_SRC_BY_DIR FILES)
    foreach(PATH_NAME ${${FILES}})
        string(REGEX REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/\(.*\) \\1 RELATIVE_PATH ${PATH_NAME})
        string(REGEX MATCH "(.+)/.*" HAS_GROUP ${RELATIVE_PATH})
        if(HAS_GROUP)
                string(REGEX REPLACE "(.+)/.*" \\1 GROUP_NAME ${RELATIVE_PATH})
                string(REPLACE "/" "\\" GROUP_NAME ${GROUP_NAME})
                message(${PATH_NAME} " into " ${GROUP_NAME})
                source_group(${GROUP_NAME} FILES ${PATH_NAME})
        else()
                source_group("" FILES ${PATH_NAME})
        endif()
    endforeach(PATH_NAME)
endmacro(GROUP_SRC_BY_DIR)



include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#通用设置部分 包括启用分组 设置 启动项目  
if(WIN32)
    ADD_DEFINITIONS(-DWIN32 -W3)
    SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
else()
    #ADD_DEFINITIONS(-Wall -O4 )
    ADD_DEFINITIONS(-Wall -O2 )
#    ADD_DEFINITIONS(-Wall -g -ggdb -O0 )
    set(CMAKE_CXX_FLAGS -std=c++14)
#    link_directories(${CMAKE_SOURCE_DIR}/res3.5/libs)
#    link_libraries(protobuf pthread m c)
     link_libraries(pthread m c)
endif()

if(Loop)
    ADD_DEFINITIONS(-DLOOP_IMPL_VERSION=${Loop} )
    MESSAGE(${Loop})
endif()
#输出
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")


MESSAGE(${CMAKE_CXX_COMPILER})
#自定义部分 


macro(SUB_DIR_LIST result curdir)
    file(GLOB children RELATIVE ${curdir} ${curdir}/*)
    set(dirlist "")
    foreach(child ${children})
        if(IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        endif()
    endforeach()
    set(${result} ${dirlist})
endmacro()

SUB_DIR_LIST(SUB_DIRS ${CMAKE_SOURCE_DIR}/tests/)
FOREACH(SUB_DIR ${SUB_DIRS}) 
    set(EXAMPLE_SOURCE "")
    MESSAGE(${SUB_DIR})
    FILE(GLOB_RECURSE EXAMPLE_SOURCE  
    ${CMAKE_SOURCE_DIR}/tests/${SUB_DIR}/*.h 
    ${CMAKE_SOURCE_DIR}/tests/${SUB_DIR}/*.c 
    ${CMAKE_SOURCE_DIR}/tests/${SUB_DIR}/*.cc 
    ${CMAKE_SOURCE_DIR}/tests/${SUB_DIR}/*.cpp)
    LIST(APPEND EXAMPLE_SOURCE ${CMAKE_SOURCE_DIR}/match_tree.h ${CMAKE_SOURCE_DIR}/ac_match_tree.h ${CMAKE_SOURCE_DIR}/tests/fn_log.h)
    GROUP_SRC_BY_DIR(EXAMPLE_SOURCE)
    message(${EXAMPLE_SOURCE})
    ADD_EXECUTABLE(${SUB_DIR}  ${EXAMPLE_SOURCE})
    target_include_directories(${SUB_DIR} PUBLIC ${CMAKE_SOURCE_DIR}/tests/)
    if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set_target_properties(${SUB_DIR} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    endif()
ENDFOREACH()

IF (ENABLE_GCOV AND NOT WIN32 AND NOT APPLE)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} 
        PROPERTIES 
        COMPILE_FLAGS "-ftest-coverage -fprofile-arcs" 
        LINK_FLAGS "-lgcov --coverage" 
    )
ENDIF()


