# Copyright 2013 The Android Open Source Project
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

LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	../ndk_yuv_encode.cpp \


LOCAL_LDLIBS := -llog -lmediandk
# LOCAL_SHARED_LIBRARIES := libbinder libmediandk liblog libmedia libutils

# LOCAL_STATIC_LIBRARIES := \


LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../ \


LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS += -Wno-multichar -lrt
# LOCAL_CFLAGS += -DANDROID_7
LOCAL_CFLAGS += -DANDROID_10

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= yuv_encode

include $(BUILD_EXECUTABLE)
# include $(BUILD_SHARED_LIBRARY)
