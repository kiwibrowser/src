{
  # Don't copy the packaged app files: manifest.json, etc.
  'NO_PACKAGE_FILES': True,
  'TARGETS': [
    {
      'NAME': 'part2',
      'TYPE': 'main',
      'SOURCES': ['hello_tutorial.cc'],
      'LIBS': ['ppapi_cpp', 'ppapi', 'pthread'],
    }
  ],
  'DATA': [
    'example.js',
    'README',
  ],
  'DEST': 'getting_started',
  'NAME': 'part2',
  'TITLE': 'Getting Started: Part 2',
  'GROUP': 'Getting Started',
}
