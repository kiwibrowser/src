{
  'TOOLS': ['clang-newlib', 'glibc'],
  'TARGETS': [
    {
      'NAME': 'multi_platform',
      'TYPE': 'main',
      'SOURCES': ['multi_platform.cc'],
      'LIBS': ['ppapi_cpp', 'ppapi', 'pthread'],
    }
  ],
  'DATA': [
    'example.js',
    'README',
  ],
  'DEST': 'examples/tutorial',
  'NAME': 'multi_platform',
  'TITLE': 'Multi-platform App',
  'GROUP': 'Tutorial',
  'MULTI_PLATFORM': True,
}
