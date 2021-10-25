# Meson build file
#
# SPDX-FileCopyrightText: 2021 Comcast Cable Communications Management, LLC
# SPDX-License-Identifier: Apache-2.0

project('cpeabs', 'c',
        version: '0.0.0',
        license: ['Apache-2.0'],
        default_options: ['c_std=c99',
                          'b_coverage=true'])

if not meson.is_subproject()
    # We want strict flags
    warning_level = 3   # -Wall
    werror = true
endif

prefix = get_option('prefix')

################################################################################
# Dependencies
################################################################################
#cjson
libcjson_dep         = dependency('libcjson',      version: '>=1.7.14', fallback: ['cjson'])

#rtMessage
rtMessagecmake  = import('cmake')

rtMessage_opts = rtMessagecmake.subproject_options()
rtMessage_opts.add_cmake_defines({ #'BUILD_FOR_DESKTOP': true,
                                   'CMAKE_C_FLAGS': '-std=c99 -D_GNU_SOURCE=1',
                                   'CMAKE_CXX_FLAGS': '-std=c99 -D_GNU_SOURCE=1',
                                   'BUILD_RTMESSAGE_SAMPLE_APP': true,
                                   'BUILD_RTMESSAGE_LIB': true,
                                   'BUILD_DATAPROVIDER_LIB': false,
                                   })

rtMessage_proj = rtMessagecmake.subproject('rtMessage', options: rtMessage_opts)
librtMessage_dep = rtMessage_proj.dependency('rtMessage')

#rbus-core
rbuscorecmake  = import('cmake')

rbuscore_opts = rbuscorecmake.subproject_options()

rbuscore_opts.add_cmake_defines({ #'BUILD_FOR_DESKTOP': true,
                                  #'CMAKE_INSTALL_PREFIX': prefix+'/include',
                                  'RTMESSAGE_INCLUDE_DIRS': 'subprojects/rtMessage',
                                  'RTMESSAGE_LIBRARIES': 'subprojects/rtMessage',
                                  })

rbuscore_proj = rbuscorecmake.subproject('rbus-core', options: rbuscore_opts)

librbuscore_dep = rbuscore_proj.dependency('rbus-core')

message(prefix+'/include')

#rbus


rbuscmake  = import('cmake')

rbus_opts = rbuscmake.subproject_options()

rbus_opts.add_cmake_defines({'BUILD_FOR_DESKTOP': true,
                             'RTMESSAGE_INCLUDE_DIRS': 'subprojects/rtMessage',
                             'RTMESSAGE_LIBRARIES': 'subprojects/rtMessage',
                             'RBUSCORE_INCLUDE_DIRS': 'subprojects/rbus-core',
                             'RBUSCORE_LIBRARIES':'subprojects/rbus-core'
                             })

rbus_proj = rbuscmake.subproject('rbus', options: rbus_opts)

librbus_dep = rbus_proj.dependency('rbus')

all_dep = [libcjson_dep, librtMessage_dep, librbuscore_dep, librbus_dep]

#wdmp-c dependencies

#wdmpcmake = import('cmake')

#wdmp_opts = wdmpcmake.subproject_options()

#wdmp_opts.add_cmake_defines({'CMAKE_POSITION_INDEPENDENT_CODE': true})

#wdmp_proj = wdmpcmake.subproject('wdmp', options: wdmp_opts)

#libwdmp_dep = wdmp_proj.dependency('wdmp-c')

#cjson dependencies
#libcjson_dep = dependency('libcjson', version: '>=1.7.14', fallback: ['cjson'])

#all_dep = [libcjson_dep, libwdmp_dep]

################################################################################
# Common variable definitions
################################################################################

inc_base = ['include/'+meson.project_name()]#, 'subprojects/rtMessage', 'subprojects/rbus-core', 'subprojects/rbus']

################################################################################
# Generate the version header file
################################################################################

subdir('include/'+meson.project_name())

install_headers(['include/'+meson.project_name()+'/cpeabs.h', ver_h], subdir: meson.project_name())

################################################################################
# Define the libraries
################################################################################

inc = include_directories([inc_base])

src_args = ['-lcjson', '-lpthread']
sources = ['src/common.c']

libcpeabs = library(meson.project_name(),
                    sources,
                    include_directories: inc,
                    dependencies: all_dep,
                    install: true,
                    link_args: src_args)

#executable(meson.project_name(),
 #                   sources,
  #                  include_directories: inc,
                   # dependencies: all_dep,
   #                 install: true,
    #                link_args: src_args)

################################################################################
# Define the tests
################################################################################

if not meson.is_subproject()
    test_args = ['-fprofile-arcs', '-ftest-coverage', '-O0']

    cunit_dep = dependency('cunit')

    tests = [
              ['test common', 'test_common'],
            ]

    foreach test : tests
      test(test[0],
           executable(test[1], ['tests/'+test[1]+'.c'],
                      include_directories: inc,
                      dependencies: cunit_dep,
                      install: false,
                      link_args: test_args,
                      link_with: libcpeabs))
    endforeach

    add_test_setup('valgrind',
                   is_default: true,
                   exe_wrapper: [ 'valgrind',
                                  '--leak-check=full',
                                  '--error-exitcode=1' ],
                   timeout_multiplier: 2)
endif

################################################################################
# Add the license check target
################################################################################

reuse_bin = find_program('reuse', required: false)
if reuse_bin.found()
    custom_target('license',
                  command: [reuse_bin, 'lint'],
                  build_always_stale: true,
                  output: ['license'])
endif

libcpeabs_dep = declare_dependency(include_directories: ['include'],
                                          link_with: libcpeabs)

if meson.version().version_compare('>=0.54.0')
    meson.override_dependency(meson.project_name(), libcpeabs_dep)
endif
