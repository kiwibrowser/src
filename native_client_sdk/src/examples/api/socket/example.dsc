{
  'TARGETS': [
    {
      'NAME' : 'socket',
      'TYPE' : 'main',
      'SOURCES' : ['socket.cc', 'echo_server.cc', 'echo_server.h'],
      'LIBS': ['ppapi_cpp', 'ppapi']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples/api',
  'NAME': 'socket',
  'TITLE': 'socket',
  'GROUP': 'API',
  'SOCKET_PERMISSIONS': ["tcp-listen:*:*", "tcp-connect", "resolve-host", "udp-bind:*:*", "udp-send-to:*:*"]
}
