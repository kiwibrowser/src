{
  'TARGETS': [
    {
      'NAME' : 'video_decode',
      'TYPE' : 'main',
      'SOURCES' : [
        'testdata.h',
        'video_decode.cc',
      ],
      'LIBS': ['ppapi_gles2', 'ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
  ],
  'DEST': 'examples/api',
  'NAME': 'video_decode',
  'TITLE': 'Video Decode',
  'GROUP': 'API',
  'MIN_CHROME_VERSION': '37.0.0.0'
}



