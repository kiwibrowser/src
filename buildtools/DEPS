use_relative_paths = True

vars = {
  "chromium_url": "https://chromium.googlesource.com",

  # When changing these, also update the svn revisions in deps_revisions.gni
  "clang_format_revision": "0653eee0c81ea04715c635dd0885e8096ff6ba6d",
  "libcxx_revision":       "85a7702b4cc5d69402791fe685f151cf3076be71",
  "libcxxabi_revision":    "05a73941f3fb177c4a891d4ff2a4ed5785e3b80c",
  "libunwind_revision":    "1e1c6b739595098ba5c466bfe9d58b993e646b48",
}

deps = {
  "clang_format/script":
    Var("chromium_url") + "/chromium/llvm-project/cfe/tools/clang-format.git@" +
    Var("clang_format_revision"),
  "third_party/libc++/trunk":
    Var("chromium_url") + "/chromium/llvm-project/libcxx.git" + "@" +
    Var("libcxx_revision"),
  "third_party/libc++abi/trunk":
    Var("chromium_url") + "/chromium/llvm-project/libcxxabi.git" + "@" +
    Var("libcxxabi_revision"),
  "third_party/libunwind/trunk":
    Var("chromium_url") + "/external/llvm.org/libunwind.git" + "@" +
    Var("libunwind_revision"),
}
