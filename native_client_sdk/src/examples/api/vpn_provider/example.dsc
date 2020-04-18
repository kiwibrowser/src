{
  'TARGETS': [
    {
      'NAME' : 'vpn_provider',
      'TYPE' : 'main',
      'SOURCES' : [
        'vpn_provider.cc',
        'vpn_provider_helper.cc',
        'vpn_provider_helper.h'
      ],
      'LIBS': ['ppapi_cpp', 'ppapi']
    }
  ],
  'DATA': [
    'example.js',
    'README',
  ],
  'DEST': 'examples/api',
  'NAME': 'vpn_provider',
  'TITLE': 'VPN Provider',
  'GROUP': 'API',
  'PERMISSIONS': [
    'vpnProvider'
  ]
}
