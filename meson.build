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

################################################################################
# Common variable definitions
################################################################################

inc_base = 'include/'+meson.project_name()

################################################################################
# Generate the version header file
################################################################################

subdir('include/'+meson.project_name())

install_headers([headers, ver_h], subdir: meson.project_name())

################################################################################
# Define the libraries
################################################################################

inc = include_directories(inc_base)

sources = ['src/common.c']

libcpeabs = library(meson.project_name(),
                    sources,
                    include_directories: inc,
                    install: true)

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

################################################################################
# Add the coverity check target
################################################################################

alias_target('coverity', libcpeabs )

################################################################################
# External/consumer definitions
################################################################################

libcpeabs_dep = declare_dependency(include_directories: ['include'],
                                          link_with: libcpeabs)

if meson.version().version_compare('>=0.54.0')
    meson.override_dependency(meson.project_name(), libcpeabs_dep)
endif
