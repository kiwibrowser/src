{
  'TOOLS': ['clang-newlib', 'glibc', 'pnacl', 'linux', 'mac'],
  'TARGETS': [
    {
      'NAME' : 'pi_generator',
      'TYPE' : 'main',
      'SOURCES' : ['pi_generator.cc'],
      'DEPS': ['ppapi_simple', 'nacl_io'],
      'LIBS': ['ppapi_simple', 'nacl_io', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples/demo',
  'NAME': 'pi_generator',
  'TITLE': 'Monte Carlo Estimate for Pi',
  'GROUP': 'Demo'
}
