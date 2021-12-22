# SPDX-FileCopyrightText: 2021 Comcast Cable Communications Management, LLC
# SPDX-License-Identifier: Apache-2.0

set(PROJ_CPEABS cpeabs)

if (BUILD_YOCTO)
add_definitions(-DBUILD_YOCTO)
endif (BUILD_YOCTO)

if (PLATFORM STREQUAL "DEVICE_GATEWAY")
add_definitions(-DDEVICE_GATEWAY)
elseif (PLATFORM STREQUAL "DEVICE_EXTENDER")
add_definitions(-DDEVICE_EXTENDER)
endif ()

if (PLATFORM STREQUAL "DEVICE_GATEWAY")
set(SOURCES rdkb/impl.c)
set(HEADERS ${PROJECT_SOURCE_DIR}/include/cpeabs/cpeabs.h)
elseif (PLATFORM STREQUAL "DEVICE_EXTENDER")
set(SOURCES pods/impl.c pods/cpeabs_ovsdb_utils.c)
set(HEADERS ${PROJECT_SOURCE_DIR}/include/cpeabs/cpeabs.h ${PROJECT_SOURCE_DIR}/src/pods/include/cpeabs_ovsdb_utils.h)
add_definitions(-DPS_FILE_PATH=${PS_FILE_PATH})
else ()
set(SOURCES pc/impl.c)
endif ()

#Support cpeabs lib for backward compatibility
add_library(${PROJ_CPEABS} STATIC ${HEADERS} ${SOURCES})
add_library(${PROJ_CPEABS}.shared SHARED ${HEADERS} ${SOURCES})
set_target_properties(${PROJ_CPEABS}.shared PROPERTIES OUTPUT_NAME ${PROJ_CPEABS})
set_property(TARGET ${PROJ_CPEABS} PROPERTY C_STANDARD 99)
set_property(TARGET ${PROJ_CPEABS}.shared PROPERTY C_STANDARD 99)

install (TARGETS ${PROJ_CPEABS} DESTINATION lib${LIB_SUFFIX})
install (TARGETS ${PROJ_CPEABS}.shared DESTINATION lib${LIB_SUFFIX})
install (FILES ${HEADERS} DESTINATION include/${PROJ_CPEABS})

