{
  'SEARCH': [
      '../../../../ppapi/c/private',
      '../../../../ppapi/cpp/private',
  ],
  'TARGETS': [
    {
      'NAME' : 'ppapi_cpp_private',
      'TYPE' : 'lib',
      'SOURCES' : [
          'ext_crx_file_system_private.cc',
          'file_io_private.cc',
          'host_resolver_private.cc',
          'isolated_file_system_private.cc',
          'net_address_private.cc',
          'pass_file_handle.cc',
          'tcp_socket_private.cc',
          'tcp_server_socket_private.cc',
          'udp_socket_private.cc',
          'uma_private.cc',
          'x509_certificate_private.cc',
      ],
    }
  ],
  'HEADERS': [
    # ppapi/c/private
    {
      'FILES': [
        'ppb_ext_crx_file_system_private.h',
        'ppb_file_io_private.h',
        'ppb_file_ref_private.h',
        'ppb_host_resolver_private.h',
        'ppb_isolated_file_system_private.h',
        'ppb_net_address_private.h',
        'ppb_tcp_server_socket_private.h',
        'ppb_tcp_socket_private.h',
        'ppb_udp_socket_private.h',
        'ppb_uma_private.h',
        'ppb_x509_certificate_private.h',
        'pp_file_handle.h',
      ],
      'DEST': 'include/ppapi/c/private',
    },

    # ppapi/cpp/private
    {
      'FILES': [
        'ext_crx_file_system_private.h',
        'file_io_private.h',
        'host_resolver_private.h',
        'isolated_file_system_private.h',
        'net_address_private.h',
        'pass_file_handle.h',
        'tcp_server_socket_private.h',
        'tcp_socket_private.h',
        'udp_socket_private.h',
        'uma_private.h',
        'x509_certificate_private.h',
      ],
      'DEST': 'include/ppapi/cpp/private',
    },
  ],
  'DEST': 'src',
  'NAME': 'ppapi_cpp_private',
}

