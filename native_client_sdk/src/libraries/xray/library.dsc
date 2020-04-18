{
  'TOOLS': ['clang-newlib', 'glibc', 'pnacl'],
  'TARGETS': [
    {
      'NAME' : 'xray',
      'TYPE' : 'lib',
      'SOURCES' : [
        'demangle.c',
        'hashtable.c',
        'parsesymbols.c',
        'report.c',
        'browser.c',
        'stringpool.c',
        'symtable.c',
        'xray.c'
      ],
      'CFLAGS': ['-DXRAY -DXRAY_ANNOTATE -O2']
    }
  ],
  'HEADERS': [
    {
      'FILES': [
        'xray.h',
        'xray_priv.h'
      ],
      'DEST': 'include/xray',
    }
  ],
  'DEST': 'src',
  'NAME': 'xray',
  'EXPERIMENTAL': True,
}
