#include <jni.h>
#ifndef _DetectionFace
#define _DetectionFace
#ifdef __cplusplus
extern "C" {
#endif

jint Java_com_smartcamera_core_CameraManager_loadCascade(JNIEnv *env,
		jobject obj, jstring,jstring,jstring,jstring);

jintArray Java_com_smartcamera_core_CameraManager_decodeYUV420(JNIEnv *env,
		jobject obj, jbyteArray buf, jint width, jint height);

jintArray Java_com_smartcamera_core_CameraManager_grayImage(JNIEnv *,jobject,jintArray,jint,jint);


jintArray Java_com_smartcamera_core_CameraManager_detectFaceX(JNIEnv *env,
		jobject obj,jbyteArray,jint,jint,jint);

int Java_com_smartcamera_core_CameraManager_enableSingleTracking(JNIEnv *env,
		jobject obj,jint);

#ifdef __cplusplus
}
#endif
#endif
