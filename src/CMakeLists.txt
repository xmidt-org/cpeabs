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

set(PROJ_CPEABS cpeabs)

if (BUILD_YOCTO)
add_definitions(-DBUILD_YOCTO)
endif (BUILD_YOCTO)

if (PLATFORM STREQUAL "DEVICE_GATEWAY")
add_definitions(-DDEVICE_GATEWAY)
if(FEATURE_SUPPORT_MQTTCM)
add_definitions(-DFEATURE_SUPPORT_MQTTCM)
endif()
elseif (PLATFORM STREQUAL "DEVICE_VIDEO")
add_definitions(-DDEVICE_VIDEO)
elseif (PLATFORM STREQUAL "DEVICE_EXTENDER")
add_definitions(-DDEVICE_EXTENDER)
elseif (PLATFORM STREQUAL "DEVICE_CAMERA")
add_definitions(-DDEVICE_CAMERA)
endif ()

if (PLATFORM STREQUAL "DEVICE_GATEWAY")
set(SOURCES rdkb/impl.c)
set(HEADERS ${PROJECT_SOURCE_DIR}/include/cpeabs.h)
elseif (PLATFORM STREQUAL "DEVICE_VIDEO")
set(SOURCES rdkv/impl.c)
set(HEADERS ${PROJECT_SOURCE_DIR}/include/cpeabs.h)
elseif (PLATFORM STREQUAL "DEVICE_EXTENDER")
set(SOURCES pods/impl.c pods/cpeabs_ovsdb_utils.c)
set(HEADERS ${PROJECT_SOURCE_DIR}/include/cpeabs_ovsdb_utils.h ${PROJECT_SOURCE_DIR}/include/cpeabs.h)
add_definitions(-DPS_FILE_PATH=${PS_FILE_PATH})
elseif (PLATFORM STREQUAL "DEVICE_CAMERA")
set(SOURCES rdkc/impl.c)
set(HEADERS ${PROJECT_SOURCE_DIR}/include/cpeabs.h)
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

