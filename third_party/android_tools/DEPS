use_relative_paths = True

vars = {
  'ndk_revision': 'e951c37287c7d8cd915bf8d4149fd4a06d808b55',
}
deps = {
  'ndk': {
    # This is necessary for the buildspecs used on release branches.
    # See crbug.com/783607 for more context.
    'condition': 'checkout_android',
    'url': 'https://chromium.googlesource.com/android_ndk.git' + '@' + Var('ndk_revision'),
  }
}
