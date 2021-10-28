#   Copyright 2020
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

cmake_minimum_required(VERSION 2.8.7)

project(cpeabs)

include(ExternalProject)
include(CTest)

add_definitions(-std=c99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g -Werror -Wall -D_GNU_SOURCE=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c99 -g -Werror -Wall -D_GNU_SOURCE=1")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
endif()

set(INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/_install)
set(PREFIX_DIR ${CMAKE_CURRENT_BINARY_DIR}/_prefix)
set(INCLUDE_DIR ${INSTALL_DIR}/include)
set(LIBRARY_DIR ${INSTALL_DIR}/lib)
set(LIBRARY_DIR64 ${INSTALL_DIR}/lib64)
set(COMMON_LIBRARY_DIR ${INSTALL_DIR}/lib/${CMAKE_LIBRARY_ARCHITECTURE})
set(TEST_RESULTS_DIR ${CMAKE_CURRENT_BINARY_DIR}/test_results)
file(MAKE_DIRECTORY ${TEST_RESULTS_DIR})

include_directories(${INCLUDE_DIR}
                    ${INCLUDE_DIR}/cjson
                    ${INCLUDE_DIR}/msgpack
                    ${INCLUDE_DIR}/rbus
		    ${INCLUDE_DIR}/rbus-core
		    ${INCLUDE_DIR}/rtmessage
		    #${INCLUDE_DIR}/trower-base64
		    #${INCLUDE_DIR}/wdmp-c
		    #${INCLUDE_DIR}/cimplog
			#${INCLUDE_DIR}/wrp-c
#                    ${INCLUDE_DIR}/curl
                    )
#if (FEATURE_SUPPORT_AKER)
#include_directories(${INCLUDE_DIR}/nanomsg
#		    ${INCLUDE_DIR}/libparodus
#)
#endif (FEATURE_SUPPORT_AKER)

#if (WEBCONFIG_BIN_SUPPORT)
#include_directories(${INCLUDE_DIR}/rbus
#		    ${INCLUDE_DIR}/rbus-core
#		    ${INCLUDE_DIR}/rtmessage
#)
#endif (WEBCONFIG_BIN_SUPPORT)

if (NOT BUILD_YOCTO)

# msgpack-c external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(msgpack
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/msgpack
    GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
    GIT_TAG "7a98138f27f27290e680bf8fbf1f8d1b089bf138"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
                  -DMSGPACK_ENABLE_CXX=OFF
                  -DMSGPACK_BUILD_EXAMPLES=OFF
                  -DBUILD_TESTING=OFF
)
add_library(libmsgpack STATIC SHARED IMPORTED)
add_dependencies(libmsgpack msgpack)

# cJSON external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(cJSON
    PREFIX ${PREFIX_DIR}/cJSON
    GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
    GIT_TAG "aafb64a1c549b7b927e339df6d35b1d5059dc235"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
)
add_library(libcJSON STATIC IMPORTED)
add_dependencies(libcJSON cJSON)

# rtMessage external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(rtMessage
     PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/rtMessage
     GIT_REPOSITORY https://github.com/rdkcmf/rdk-rtmessage.git
     GIT_TAG rdk-next
     CMAKE_ARGS += -DBUILD_RTMESSAGE_LIB=ON
                   -DBUILD_RTMESSAGE_SAMPLE_APP=ON
                   -DBUILD_FOR_DESKTOP=ON
                   -DBUILD_DATAPROVIDER_LIB=ON
                   -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} -DBUILD_TESTING=OFF
)
add_library(librtMessage STATIC SHARED IMPORTED)
add_dependencies(librtMessage rtMessage)

# rbus-core external dependency	
#-------------------------------------------------------------------------------
ExternalProject_Add(rbus-core
     DEPENDS rtMessage
     PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/rbus-core
     GIT_REPOSITORY https://github.com/rdkcmf/rbuscore.git
     GIT_TAG rdk-next
     CMAKE_ARGS += DBUILD_FOR_DESKTOP=ON -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
                   -DBUILD_TESTING=OFF
)
add_library(librbus-core STATIC SHARED IMPORTED)
add_dependencies(librbus-core rbus-core)

# rbus external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(rbus
     DEPENDS rtMessage rbus-core
     PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/rbus
     GIT_REPOSITORY https://github.com/rdkcmf/rbus.git
     GIT_TAG rdk-next
     CMAKE_ARGS += -DBUILD_FOR_DESKTOP=ON -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} -DBUILD_TESTING=OFF
)

add_library(librbus STATIC SHARED IMPORTED)
add_dependencies(librbus rbus)

endif ()
link_directories ( ${LIBRARY_DIR} ${COMMON_LIBRARY_DIR} ${LIBRARY_DIR64} )

add_subdirectory(src)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif (BUILD_TESTING)
