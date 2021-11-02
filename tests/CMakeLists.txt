#   Copyright 2021 Comcast Cable Communications Management, LLC
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

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTEST ")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W  -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -O0")

if(NOT DISABLE_VALGRIND)
set (MEMORY_CHECK valgrind --leak-check=full --show-reachable=yes --track-origins=yes -v)
endif ()

link_directories ( ${LIBRARY_DIR} )

if (BUILD_YOCTO)
add_definitions(-DBUILD_YOCTO)
endif (BUILD_YOCTO)

if (WEBCONFIG_BIN_SUPPORT)
add_definitions(-DWEBCONFIG_BIN_SUPPORT)
endif (WEBCONFIG_BIN_SUPPORT)

if(WEBCONFIG_BIN_SUPPORT)
#-------------------------------------------------------------------------------
#   test_rdkb
#-------------------------------------------------------------------------------
add_test(NAME test_rdkb COMMAND ${MEMORY_CHECK} ./test_rdkb)

set(SOURCES test_rdkb.c ../src/rdkb/impl.c )

add_executable(test_rdkb ${SOURCES})

target_link_libraries (test_rdkb -lcjson -lcunit -lmsgpackc -lwdmp-c -lcimplog -lpthread -lrbus -lrbus-core)

target_link_libraries (test_rdkb gcov -Wl,--no-as-needed )

else()

#-------------------------------------------------------------------------------
#   test_pods
#-------------------------------------------------------------------------------
add_test(NAME test_pods COMMAND ${MEMORY_CHECK} ./test_pods)

set(SOURCES test_pods.c ../src/pods/impl.c )

add_executable(test_pods ${SOURCES})

target_link_libraries (test_pods -lcunit)

target_link_libraries (test_pods gcov -Wl,--no-as-needed )

endif(WEBCONFIG_BIN_SUPPORT)

# Code coverage

add_custom_target(coverage
COMMAND lcov -q --capture --directory 
${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_common.dir/__/src --output-file test_common.info

COMMAND lcov
-a test_common.info
--output-file coverage.info

COMMAND genhtml coverage.info
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
