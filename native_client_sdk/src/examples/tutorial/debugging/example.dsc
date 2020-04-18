{
  'DISABLE_PACKAGE': True,  # Doesn't work in packaged apps yet.
  'TOOLS': ['clang-newlib'],
  'TARGETS': [
    {
      'NAME' : 'debugging',
      'TYPE' : 'main',
      'SOURCES' : [
        'debugging.c',
      ],
      'CFLAGS': ['-fno-omit-frame-pointer'],
      'DEPS' : ['error_handling'],
      'LIBS' : ['error_handling', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples/tutorial',
  'NAME': 'debugging',
  'TITLE': 'Debugging',
  'GROUP': 'Tutorial'
}

