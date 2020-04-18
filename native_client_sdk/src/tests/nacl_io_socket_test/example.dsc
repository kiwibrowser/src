{
  'TOOLS': ['clang-newlib', 'glibc', 'pnacl', 'linux', 'mac'],
  'TARGETS': [
    {
      'NAME' : 'nacl_io_socket_test',
      'TYPE' : 'main',
      'SOURCES' : [
        'main.cc',
        'socket_test.cc',
        'echo_server.cc',
        'echo_server.h',
      ],
      'DEPS': ['ppapi_simple_cpp', 'nacl_io'],
      'LIBS': ['ppapi_simple_cpp', 'ppapi_cpp', 'ppapi', 'nacl_io', 'pthread'],
      'EXTRA_SOURCES' : ['../../src/gtest/src/gtest-all.cc'],
      'INCLUDES': [
        '../../src/gtest/include',
        '../../src/gtest'
      ],
      'CXXFLAGS': ['-Wno-sign-compare']
    }
  ],
  'DATA': [
    'example.js'
  ],
  'DEST': 'tests',
  'NAME': 'nacl_io_socket_test',
  'TITLE': 'NaCl IO Socket test',
  'SOCKET_PERMISSIONS': [
    "tcp-listen:*:*",
    "tcp-connect",
    "resolve-host",
    "udp-bind:*:*",
    "udp-send-to:*:*"
  ]
}
