LOCAL_PATH := $(call my-dir)

do_test = \
$(eval include $$(CLEAR_VARS))\
$(eval LOCAL_MODULE := test_gabixx_static_$1)\
$(eval LOCAL_SRC_FILES := $2)\
$(eval LOCAL_CFLAGS := $3)\
$(eval LOCAL_STATIC_LIBRARIES := gabi++_static)\
$(eval include $$(BUILD_EXECUTABLE))\
\
$(eval include $$(CLEAR_VARS))\
$(eval LOCAL_MODULE := test_gabixx_shared_$1)\
$(eval LOCAL_SRC_FILES := $2)\
$(eval LOCAL_CFLAGS := $3)\
$(eval LOCAL_SHARED_LIBRARIES := gabi++_shared)\
$(eval include $$(BUILD_EXECUTABLE))\

do_test_simple = $(call do_test,$1,$1.cpp,$2)

$(call do_test,rtti,test_gabixx_rtti.cpp)
$(call do_test,exceptions,test_gabixx_exceptions.cpp)
$(call do_test,aux_runtime,test_aux_runtime.cpp)
$(call do_test_simple,test_guard)
$(call do_test_simple,catch_array_01)
$(call do_test_simple,catch_array_02)
$(call do_test_simple,catch_class_01)
$(call do_test_simple,catch_class_02)
$(call do_test_simple,catch_class_03)
$(call do_test_simple,catch_class_04)
$(call do_test_simple,catch_const_pointer_nullptr,-std=c++11)
$(call do_test_simple,catch_function_01)
$(call do_test_simple,catch_function_02)
$(call do_test_simple,catch_member_data_pointer_01)
$(call do_test_simple,catch_member_function_pointer_01)
$(call do_test_simple,catch_member_pointer_nullptr,-std=c++11)
$(call do_test_simple,catch_pointer_nullptr,-std=c++11)
$(call do_test_simple,catch_ptr)
$(call do_test_simple,catch_ptr_02)
$(call do_test_simple,dynamic_cast3)
$(call do_test_simple,dynamic_cast5)
$(call do_test_simple,test_vector1)
$(call do_test_simple,test_vector2)
$(call do_test_simple,test_vector3)
$(call do_test_simple,unexpected_01,-std=c++11)
$(call do_test_simple,unexpected_02,-std=c++11)
$(call do_test_simple,unexpected_03)
$(call do_test_simple,unwind_01)
$(call do_test_simple,unwind_02)
$(call do_test_simple,unwind_03)
$(call do_test_simple,unwind_04)
$(call do_test_simple,unwind_05)

include $(CLEAR_VARS)
LOCAL_MODULE := libtest_malloc_lockup
LOCAL_SRC_FILES := libtest_malloc_lockup.cpp
LOCAL_STATIC_LIBRARIES := gabi++_static
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := malloc_lockup
LOCAL_SRC_FILES := malloc_lockup.cpp
include $(BUILD_EXECUTABLE)

$(call import-module,cxx-stl/gabi++)
