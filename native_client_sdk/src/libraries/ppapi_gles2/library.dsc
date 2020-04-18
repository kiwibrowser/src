{
  'SEARCH' : [
    '../../../../ppapi/lib/gl/gles2',
    '../../../../ppapi/lib/gl/include/EGL',
    '../../../../ppapi/lib/gl/include/GLES2',
    '../../../../ppapi/lib/gl/include/KHR'
  ],
  'TARGETS': [
    {
      'NAME' : 'ppapi_gles2',
      'TYPE' : 'lib',
      'SOURCES' : [
        'gl2ext_ppapi.c',
        'gles2.c'
      ],
    }
  ],
  'HEADERS': [
    # ppapi/lib/gl/include/KHR
    {
      'FILES': [
        'khrplatform.h'
      ],
      'DEST': 'include/KHR',
    },

    # ppapi/lib/gl/include/GLES2
    {
      'FILES': [
        'gl2.h',
        'gl2ext.h',
        'gl2platform.h',
      ],
      'DEST': 'include/GLES2',
    },

    # ppapi/lib/gl/gles2
    {
      'FILES': [ 'gl2ext_ppapi.h' ],
      'DEST': 'include/ppapi/gles2',
    },

    # Create a duplicate copy of this header
    # TODO(sbc), remove this copy once we find a way to build gl2ext_ppapi.c.
    {
      'FILES': [ 'gl2ext_ppapi.h' ],
      'DEST': 'include/ppapi/lib/gl/gles2',
    },
  ],
  'DEST': 'src',
  'NAME': 'ppapi_gles2',
}

