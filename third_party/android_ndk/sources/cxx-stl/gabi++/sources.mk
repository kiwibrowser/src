libgabi++_path := $(call my-dir)

libgabi++_src_files := \
        src/array_type_info.cc \
        src/auxilary.cc \
        src/class_type_info.cc \
        src/cxxabi.cc \
        src/cxxabi_vec.cc \
        src/delete.cc \
        src/dwarf_helper.cc \
        src/dynamic_cast.cc \
        src/enum_type_info.cc \
        src/exception.cc \
        src/fatal_error.cc \
        src/function_type_info.cc \
        src/fundamental_type_info.cc \
        src/helper_func_internal.cc \
        src/new.cc \
        src/one_time_construction.cc \
        src/pbase_type_info.cc \
        src/personality.cc \
        src/pointer_type_info.cc \
        src/pointer_to_member_type_info.cc \
        src/call_unexpected.cc \
        src/si_class_type_info.cc \
        src/stdexcept.cc \
        src/terminate.cc \
        src/type_info.cc \
        src/vmi_class_type_info.cc

libgabi++_c_includes := $(libgabi++_path)/include
