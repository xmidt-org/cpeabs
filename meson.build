# Meson build file
#
# SPDX-FileCopyrightText: 2021 Comcast Cable Communications Management, LLC
# SPDX-License-Identifier: Apache-2.0

project('cpeabs', 'c',
        version: '0.0.0',
        meson_version: '>=0.53',
        license: ['Apache-2.0'],
        default_options: ['c_std=c99',
                          'b_coverage=true'])

if not meson.is_subproject()
    # We want strict flags
    warning_level = 3   # -Wall
    werror = true
endif

script = join_paths(meson.source_root(), 'workaround.sh')

r = run_command('/bin/sh', script)
if r.returncode() != 0
  message('script failed')
endif

output = r.stdout().strip()
errortxt = r.stderr().strip()

prefix = get_option('prefix')

################################################################################
# Dependencies
################################################################################

if meson.version().version_compare('>=0.54.0')
libcjson_dep     = dependency('libcjson',      version: '>=1.7.14', fallback: ['cjson'])
librtMessage_dep = dependency('librtMessage')
librbuscore_dep  = dependency('librbuscore')
librbus_dep      = dependency('librbus')
else
warning('You are running an older version of meson.')
warning('In order for dependency management to be supported you need a newer version of meson (0.54+)')
# For older versions of meson, you need to have the upstream dependencies installed
# in the correct locations.
libcjson_dep     = dependency('libcjson')
librtMessage_dep = dependency('librtMessage')
librbuscore_dep  = dependency('librbuscore')
librbus_dep      = dependency('librbus')
endif

all_dep = [libcjson_dep, librtMessage_dep, librbuscore_dep, librbus_dep]

#wdmp-c dependencies

#wdmpcmake = import('cmake')

#wdmp_opts = wdmpcmake.subproject_options()

#wdmp_opts.add_cmake_defines({'CMAKE_POSITION_INDEPENDENT_CODE': true})

#wdmp_proj = wdmpcmake.subproject('wdmp', options: wdmp_opts)

#libwdmp_dep = wdmp_proj.dependency('wdmp-c')

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

executable(meson.project_name(),
                    sources,
                    include_directories: inc,
                    dependencies: all_dep,
                    install: true,
                    link_args: src_args)

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
