{
  'TOOLS': ['glibc', 'pnacl', 'linux', 'mac', 'clang-newlib'],
  'SEARCH': [
    '.',
    '../ppapi_simple'
  ],
  'TARGETS': [
    {
      'NAME' : 'ppapi_simple_cpp',
      'TYPE' : 'linker-script',
      'SOURCES' : [
        "ppapi_simple_cpp.a.linkerscript",
        "ppapi_simple_cpp.so.linkerscript",
      ],
    },
    {
      'NAME' : 'ppapi_simple_cpp_real',
      'TYPE' : 'lib',
      'SOURCES' : [
        "ps.c",
        "ps_context_2d.c",
        "ps_event.c",
        "ps_instance.c",
        "ps_interface.c",
        "ps_main.c",
        "ps_main_default.c",
        "ps_entrypoints_cpp.cc"
      ],
    },
  ],
  'DEST': 'src',
  'NAME': 'ppapi_simple_cpp',
}
