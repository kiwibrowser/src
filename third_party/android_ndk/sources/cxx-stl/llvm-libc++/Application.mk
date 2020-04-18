NDK_TOOLCHAIN_VERSION := clang
# Even the system STL is too much because it will link libsupc++ for rtti and
# exceptions.
APP_STL := none
