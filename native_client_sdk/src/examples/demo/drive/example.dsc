{
  'TOOLS': ['clang-newlib', 'glibc', 'pnacl'],
  'TARGETS': [
    {
      'NAME' : 'drive',
      'TYPE' : 'main',
      'SOURCES' : ['drive.cc'],
      'LIBS': ['jsoncpp', 'ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples/demo',
  'NAME': 'drive',
  'TITLE': 'Google Drive',
  'GROUP': 'Demo',
  'PERMISSIONS': [
    'identity',
    'https://www.googleapis.com/*/drive/*',
    'https://*.googleusercontent.com/*'
  ]
}
