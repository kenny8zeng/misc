LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPPFLAGS  += -DANDROID_BUILD_REMOTER -std=c++0x
TARGET_PLATFORM := android-19
LOCAL_MODULE    := serialport
LOCAL_SRC_FILES := serialport/serialport.cpp
LOCAL_LDLIBS    := -lm -llog

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_CPPFLAGS  += -DANDROID_BUILD_REMOTER -std=c++0x
TARGET_PLATFORM := android-19
LOCAL_MODULE    := remoter_service
LOCAL_SRC_FILES := remoter_service.c keymap.cpp
APP_STL      := gnustl_shared
LOCAL_LDLIBS    := -lm -llog
LOCAL_STATIC_LIBRARIES := serialport

include $(BUILD_SHARED_LIBRARY)
