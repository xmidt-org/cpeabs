project('rtMessage', 'c',
        version: '0.0.1',
        meson_version: '>=0.54.0',
        license: ['Apache-2.0'],
        default_options: ['c_std=c18'])


libcjson_dep = dependency('libcjson', version: '>=1.7.14', fallback: ['cjson'])
thread_dep   = dependency('threads')

headers = files('rtMessage.h',
                'rtMessageHeader.h',
                'rtError.h',
                'rtConnection.h',
                'rtVector.h',
                'rtRetainable.h',
                'rtLog.h',
                'rtList.h',
                'rtTime.h',
                'rtAtomic.h',
                'rtm_discovery_api.h',
                'rtAdvisory.h')

subdir('include-workaround-meson/rtmessage')

librtMessage = library('rtMessage',
                   ['rtConnection.c',
                    'rtCipher.c',
                    'rtLog.c',
                    'rtError.c',
                    'rtMessageHeader.c',
                    'rtEncoder.c',
                    'rtMessage.c',
                    'rtSocket.c',
                    'rtVector.c',
                    'rtList.c',
                    'rtTime.c',
                    'rtSemaphore.c',
                    'rtRetainable.c',
                    'rtBase64.c',
                    'local_benchmarking.c'],
                   c_args:   ['-D_GNU_SOURCE', '-D_POSIX_C_SOURCE=199309L'],
                   cpp_args: ['-D_GNU_SOURCE', '-D_POSIX_C_SOURCE=199309L'],
                   dependencies: [ libcjson_dep ],
                   install: true)

librtMessage_dep = declare_dependency(include_directories: ['include-workaround-meson'],
                                  link_with: librtMessage)

if meson.version().version_compare('>=0.54.0')
    meson.override_dependency('librtMessage', librtMessage_dep)
endif
