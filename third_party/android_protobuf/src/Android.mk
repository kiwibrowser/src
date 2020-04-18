# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#

LOCAL_PATH := $(call my-dir)

JAVA_LITE_SRC_FILES := \
    java/src/main/java/com/google/protobuf/UninitializedMessageException.java \
    java/src/main/java/com/google/protobuf/MessageLite.java \
    java/src/main/java/com/google/protobuf/InvalidProtocolBufferException.java \
    java/src/main/java/com/google/protobuf/CodedOutputStream.java \
    java/src/main/java/com/google/protobuf/ByteString.java \
    java/src/main/java/com/google/protobuf/CodedInputStream.java \
    java/src/main/java/com/google/protobuf/ExtensionRegistryLite.java \
    java/src/main/java/com/google/protobuf/AbstractMessageLite.java \
    java/src/main/java/com/google/protobuf/AbstractParser.java \
    java/src/main/java/com/google/protobuf/FieldSet.java \
    java/src/main/java/com/google/protobuf/Internal.java \
    java/src/main/java/com/google/protobuf/WireFormat.java \
    java/src/main/java/com/google/protobuf/GeneratedMessageLite.java \
    java/src/main/java/com/google/protobuf/BoundedByteString.java \
    java/src/main/java/com/google/protobuf/LazyField.java \
    java/src/main/java/com/google/protobuf/LazyFieldLite.java \
    java/src/main/java/com/google/protobuf/LazyStringList.java \
    java/src/main/java/com/google/protobuf/LazyStringArrayList.java \
    java/src/main/java/com/google/protobuf/UnmodifiableLazyStringList.java \
    java/src/main/java/com/google/protobuf/LiteralByteString.java \
    java/src/main/java/com/google/protobuf/MessageLiteOrBuilder.java \
    java/src/main/java/com/google/protobuf/Parser.java \
    java/src/main/java/com/google/protobuf/ProtocolStringList.java \
    java/src/main/java/com/google/protobuf/RopeByteString.java \
    java/src/main/java/com/google/protobuf/SmallSortedMap.java \
    java/src/main/java/com/google/protobuf/Utf8.java

# This contains more source files than needed for the full version, but the
# additional files should not create any conflict.
JAVA_FULL_SRC_FILES := \
    $(call all-java-files-under, java/src/main/java) \
    src/google/protobuf/descriptor.proto

# Java nano library (for device-side users)
# =======================================================
include $(CLEAR_VARS)

LOCAL_MODULE := libprotobuf-java-nano
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := 8

LOCAL_SRC_FILES := $(call all-java-files-under, java/src/main/java/com/google/protobuf/nano)
LOCAL_SRC_FILES += $(call all-java-files-under, java/src/device/main/java/com/google/protobuf/nano)

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_STATIC_JAVA_LIBRARY)

# Java nano library (for host-side users)
# =======================================================
include $(CLEAR_VARS)

LOCAL_MODULE := host-libprotobuf-java-nano
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, java/src/main/java/com/google/protobuf/nano)

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_HOST_JAVA_LIBRARY)

# Java micro library (for device-side users)
# =======================================================
include $(CLEAR_VARS)

LOCAL_MODULE := libprotobuf-java-micro
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := 8

LOCAL_SRC_FILES := $(call all-java-files-under, java/src/main/java/com/google/protobuf/micro)

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_STATIC_JAVA_LIBRARY)

# Java micro library (for host-side users)
# =======================================================
include $(CLEAR_VARS)

LOCAL_MODULE := host-libprotobuf-java-micro
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, java/src/main/java/com/google/protobuf/micro)

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_HOST_JAVA_LIBRARY)

# Java lite library (for device-side users)
# =======================================================
include $(CLEAR_VARS)

LOCAL_MODULE := libprotobuf-java-lite
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := 9

LOCAL_SRC_FILES := $(JAVA_LITE_SRC_FILES)

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_STATIC_JAVA_LIBRARY)

# Java lite library (for host-side users)
# =======================================================
include $(CLEAR_VARS)

LOCAL_MODULE := host-libprotobuf-java-lite
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(JAVA_LITE_SRC_FILES)

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_HOST_JAVA_LIBRARY)

# Java full library (for host-side users)
# =======================================================
include $(CLEAR_VARS)

LOCAL_MODULE := host-libprotobuf-java-full
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(JAVA_FULL_SRC_FILES)

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_HOST_JAVA_LIBRARY)

# To test java proto params build rules.
# =======================================================
include $(CLEAR_VARS)

LOCAL_MODULE := aprotoc-test-nano-params
LOCAL_MODULE_TAGS := tests
LOCAL_SDK_VERSION := current

LOCAL_PROTOC_OPTIMIZE_TYPE := nano

LOCAL_SRC_FILES := \
        src/google/protobuf/unittest_import_nano.proto \
        src/google/protobuf/unittest_simple_nano.proto \
        src/google/protobuf/unittest_stringutf8_nano.proto \
        src/google/protobuf/unittest_recursive_nano.proto


LOCAL_PROTOC_FLAGS := --proto_path=$(LOCAL_PATH)/src

LOCAL_PROTO_JAVA_OUTPUT_PARAMS := \
        java_package = $(LOCAL_PATH)/src/google/protobuf/unittest_import_nano.proto|com.google.protobuf.nano, \
        java_outer_classname = $(LOCAL_PATH)/src/google/protobuf/unittest_import_nano.proto|UnittestImportNano

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_STATIC_JAVA_LIBRARY)

# To test Android-specific nanoproto features.
# =======================================================
include $(CLEAR_VARS)

# Parcelable messages
LOCAL_MODULE := android-nano-test-parcelable
LOCAL_MODULE_TAGS := tests
LOCAL_SDK_VERSION := current
# Only needed at compile-time.
LOCAL_JAVA_LIBRARIES := android-support-annotations

LOCAL_PROTOC_OPTIMIZE_TYPE := nano

LOCAL_SRC_FILES := src/google/protobuf/unittest_simple_nano.proto

LOCAL_PROTOC_FLAGS := --proto_path=$(LOCAL_PATH)/src

LOCAL_PROTO_JAVA_OUTPUT_PARAMS := \
        parcelable_messages = true, \
        generate_intdefs = true

include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)

# Parcelable and extendable messages
LOCAL_MODULE := android-nano-test-parcelable-extendable
LOCAL_MODULE_TAGS := tests
LOCAL_SDK_VERSION := current
# Only needed at compile-time.
LOCAL_JAVA_LIBRARIES := android-support-annotations

LOCAL_PROTOC_OPTIMIZE_TYPE := nano

LOCAL_SRC_FILES := src/google/protobuf/unittest_extension_nano.proto

LOCAL_PROTOC_FLAGS := --proto_path=$(LOCAL_PATH)/src

LOCAL_PROTO_JAVA_OUTPUT_PARAMS := \
        parcelable_messages = true, \
        generate_intdefs = true, \
        store_unknown_fields = true

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)

# Test APK
LOCAL_PACKAGE_NAME := NanoAndroidTest

LOCAL_SDK_VERSION := 8

LOCAL_MODULE_TAGS := tests

LOCAL_SRC_FILES := $(call all-java-files-under, java/src/device/test/java/com/google/protobuf/nano)

LOCAL_MANIFEST_FILE := java/src/device/test/AndroidManifest.xml

LOCAL_STATIC_JAVA_LIBRARIES := libprotobuf-java-nano \
        android-nano-test-parcelable \
        android-nano-test-parcelable-extendable

LOCAL_DEX_PREOPT := false

include $(BUILD_PACKAGE)
