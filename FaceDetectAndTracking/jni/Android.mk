LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OPENCV_INSTALL_MODULES:=on
OPENCV_CAMERA_MODULES:=off

OPENCV_LIB_TYPE:=SHARED
include F:\develop\YEclipse\OpenCV-android-sdk\sdk\native\jni/OpenCV.mk

LOCAL_SRC_FILES  := DetectionFace.cpp CompressiveTracker.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_LDLIBS     +=  -llog -ldl

LOCAL_MODULE     := face

include $(BUILD_SHARED_LIBRARY)
