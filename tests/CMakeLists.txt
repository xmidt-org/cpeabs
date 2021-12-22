# SPDX-FileCopyrightText: 2021 Comcast Cable Communications Management, LLC
# SPDX-License-Identifier: Apache-2.0

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

if (PLATFORM STREQUAL "DEVICE_GATEWAY")
add_definitions(-DDEVICE_GATEWAY)
elseif (PLATFORM STREQUAL "DEVICE_EXTENDER")
add_definitions(-DDEVICE_EXTENDER)
endif ()

if (PLATFORM STREQUAL "DEVICE_GATEWAY")

#-------------------------------------------------------------------------------
#   test_rdkb
#-------------------------------------------------------------------------------
add_test(NAME test_rdkb COMMAND ${MEMORY_CHECK} ./test_rdkb)

set(SOURCES test_rdkb.c ${PROJECT_SOURCE_DIR}/src/rdkb/impl.c )

add_executable(test_rdkb ${SOURCES})

target_link_libraries (test_rdkb -lcjson -lcunit -lmsgpackc -lwdmp-c -lcimplog -lpthread -lrbus )

target_link_libraries (test_rdkb gcov -Wl,--no-as-needed )


elseif (PLATFORM STREQUAL "DEVICE_EXTENDER")

#-------------------------------------------------------------------------------
#   test_pods
#-------------------------------------------------------------------------------
add_test(NAME test_pods COMMAND ${MEMORY_CHECK} ./test_pods)

set(SOURCES test_pods.c ${PROJECT_SOURCE_DIR}/src/pods/impl.c ${PROJECT_SOURCE_DIR}/src/pods/cpeabs_ovsdb_utils.c )

add_executable(test_pods ${SOURCES})

target_link_libraries (test_pods -lcunit -ljansson)

target_link_libraries (test_pods gcov -Wl,--no-as-needed )

else()

#-------------------------------------------------------------------------------
#   test_pc
#-------------------------------------------------------------------------------
add_test(NAME test_pc COMMAND ${MEMORY_CHECK} ./test_pc)

set(SOURCES test_pc.c ${PROJECT_SOURCE_DIR}/src/pc/impl.c )

add_executable(test_pc ${SOURCES})

target_link_libraries (test_pc -lcunit)

target_link_libraries (test_pc gcov -Wl,--no-as-needed )

endif()

# Code coverage

add_custom_target(coverage
COMMAND lcov -q --capture --directory 
${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_common.dir/__/src --output-file test_common.info

COMMAND lcov
-a test_common.info
--output-file coverage.info

COMMAND genhtml coverage.info
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
