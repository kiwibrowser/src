{
  'TOOLS': ['clang-newlib'],
  'TARGETS': [
    {
      'NAME' : 'error_handling',
      'TYPE' : 'lib',
      'SOURCES' : [
        "error_handling.c",
        "string_stream.c"
      ],
    }
  ],
  'HEADERS': [
    {
      'FILES': [
        "error_handling.h",
        "string_stream.h",
      ],
      'DEST': 'include/error_handling',
    },
  ],
  'DEST': 'src',
  'NAME': 'error_handling',
}
