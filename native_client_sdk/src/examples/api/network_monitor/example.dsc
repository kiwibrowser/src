{
  'TARGETS': [
    {
      'NAME' : 'network_monitor',
      'TYPE' : 'main',
      'SOURCES' : ['network_monitor.cc'],
      'LIBS': ['ppapi_cpp', 'ppapi']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples/api',
  'NAME': 'network_monitor',
  'TITLE': 'Network Monitor',
  'GROUP': 'API',
  'SOCKET_PERMISSIONS': ["network-state"]
}
