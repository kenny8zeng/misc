LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPPFLAGS  := -DANDROID_BUILD_REMOTER
TARGET_PLATFORM := android-19
LOCAL_MODULE    := serialport
LOCAL_SRC_FILES := serialport.cpp
LOCAL_LDLIBS    := -lm -llog

include $(BUILD_STATIC_LIBRARY)
