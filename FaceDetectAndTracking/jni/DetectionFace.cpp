#include "DetectionFace.h"

#include <opencv2/opencv.hpp>
#include <vector>
#include "TrackObj.h"
#include <android/log.h>
#include "CompressiveTracker.h"
#define TAG    "**guojunyi**"

#undef LOG // 取消默认的LOG#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__)#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN,TAG,__VA_ARGS__)#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,TAG,__VA_ARGS__)using namespace cv;using namespace std;static CascadeClassifier mCascade1;static CascadeClassifier mCascade2;static CascadeClassifier mCascade3;static CascadeClassifier mCascade4;int vmin = 65, vmax = 256, smin = 55;bool enableTracking;
bool enableAsyncDetect;
int trackingCount;

#define MAX_FACES 30

#define TRACKING_IMAGE_WIDTH 640
#define TRACKING_IMAGE_HEIGHT 480
#define MAX_TRACKING_OBJ_SIZE 6

TrackObj trackObjs[MAX_TRACKING_OBJ_SIZE];

jint Java_com_smartcamera_core_CameraManager_loadCascade(JNIEnv *env,
		jobject obj, jstring cascade1, jstring cascade2, jstring cascade3,
		jstring cascade4) {

	enableTracking = false;
	trackingCount = 1;
	enableAsyncDetect = false;

	for (int i = 0; i < MAX_TRACKING_OBJ_SIZE; i++) {
		trackObjs[i].flag = false;
	}
	const char *cascadePath1 = env->GetStringUTFChars(cascade1, 0);
	const char *cascadePath2 = env->GetStringUTFChars(cascade2, 0);
	const char *cascadePath3 = env->GetStringUTFChars(cascade3, 0);
	const char *cascadePath4 = env->GetStringUTFChars(cascade4, 0);
	jint result = 1;
	if (!mCascade1.load(cascadePath1)) {
		LOGE("load cascade file fault.");
		result = 0;
	}

	if (!mCascade2.load(cascadePath2)) {
		LOGE("load cascade file fault.");
		result = 0;
	}

	if (!mCascade3.load(cascadePath3)) {
		LOGE("load cascade file fault.");
		result = 0;
	}

	if (!mCascade4.load(cascadePath4)) {
		LOGE("load cascade file fault.");
		result = 0;
	}

	return result;
}

void decodeYUV420(char *yuvDatas, int width, int height, int *rgb) {
	int frameSize = width * height;
	int i = 0, j = 0, yp = 0;
	int uvp = 0, u = 0, v = 0;
	for (j = 0, yp = 0; j < height; j++) {
		uvp = frameSize + (j >> 1) * width;
		u = 0;
		v = 0;
		for (i = 0; i < width; i++, yp++) {
			int y = (0xff & ((int) yuvDatas[yp])) - 16;
			if (y < 0)
				y = 0;
			if ((i & 1) == 0) {
				v = (0xff & yuvDatas[uvp++]) - 128;
				u = (0xff & yuvDatas[uvp++]) - 128;
			}

			int y1192 = 1192 * y;
			int r = (y1192 + 1634 * v);
			int g = (y1192 - 833 * v - 400 * u);
			int b = (y1192 + 2066 * u);

			if (r < 0)
				r = 0;
			else if (r > 262143)
				r = 262143;
			if (g < 0)
				g = 0;
			else if (g > 262143)
				g = 262143;
			if (b < 0)
				b = 0;
			else if (b > 262143)
				b = 262143;

			rgb[yp] = 0xff000000 | ((r << 6) & 0xff0000) | ((g >> 2) & 0xff00)
					| ((b >> 10) & 0xff);
		}
	}
}

jintArray Java_com_smartcamera_core_CameraManager_decodeYUV420(JNIEnv *env,
		jobject obj, jbyteArray buf, jint width, jint height) {
	jbyte * yuv420sp = env->GetByteArrayElements(buf, 0);

	int *rgb = (int*) malloc(width * height * sizeof(int));
	decodeYUV420((char*) yuv420sp, width, height, rgb);

	jintArray result = env->NewIntArray(width * height);
	env->SetIntArrayRegion(result, 0, width * height, rgb);
	free(rgb);
	env->ReleaseByteArrayElements(buf, yuv420sp, 0);
	return result;
}

void rotate(Mat& src, double angle, Mat& dst) {
	// get rotation matrix for rotating the image around its center
	Point2f center(src.cols / 2.0, src.rows / 2.0);
	Mat rot = getRotationMatrix2D(center, angle, 1.0);

	// determine bounding rectangle
	Rect bbox = RotatedRect(center, src.size(), angle).boundingRect();

	// adjust transformation matrix
	rot.at<double>(0, 2) += bbox.width / 2.0 - center.x;
	rot.at<double>(1, 2) += bbox.height / 2.0 - center.y;

	warpAffine(src, dst, rot, bbox.size());
}

bool isOverlap(Rect r1, Rect r2) {
	int nMaxLeft = 0;
	int nMaxTop = 0;
	int nMinRight = 0;
	int nMinBottom = 0;

	nMaxLeft = r1.x >= r2.x ? r1.x : r2.x;
	nMaxTop = r1.y >= r2.y ? r1.y : r2.y;
	nMinRight =
			(r1.x + r1.width) <= (r2.x + r2.width) ?
					(r1.x + r1.width) : (r2.x + r2.width);
	nMinBottom =
			(r1.y + r1.height) <= (r2.y + r2.height) ?
					(r1.y + r1.height) : (r2.y + r2.height);

	if (nMaxLeft > nMinRight || nMaxTop > nMinBottom) {
		return false;
	} else {
		return true;
	}
}

void initTracker(Mat image, int winWidth, int winHeight, Rect rect) {
	int i;
	for (i = 0; i < trackingCount; i++) {
		if (trackObjs[i].flag) {
			Rect r1 = trackObjs[i].trackFaceRect;
			bool result = isOverlap(r1, rect);
			if (result) {
				return;
			}
		}
	}

	int index = -1;
	for (i = 0; i < trackingCount; i++) {
		if (!trackObjs[i].flag) {
			index = i;
			break;
		}
	}
	if (index < 0) {
		return;
	}

	TrackObj &obj = trackObjs[index];
	obj.index = index;
	obj.initSize = rect.width * rect.height;
	obj.trackFaceRect = rect;
	obj.frame = Scalar::all(0);
	obj.hsv = Scalar::all(0);
	obj.hue = Scalar::all(0);
	obj.mask = Scalar::all(0);
	obj.hist = Scalar::all(0);
	obj.histimg = Mat::zeros(200, 320, CV_8UC3);
	obj.backproj = Scalar::all(0);
	obj.hsize = 16;

	cvtColor(image, obj.hsv, CV_BGR2HSV);
	int _vmin = vmin, _vmax = vmax;
	inRange(obj.hsv, Scalar(0, smin, MIN(_vmin, _vmax)),
			Scalar(180, 256, MAX(_vmin, _vmax)), obj.mask);
	int ch[] = { 0, 0 };
	obj.hue.create(obj.hsv.size(), obj.hsv.depth());
	mixChannels(&obj.hsv, 1, &obj.hue, 1, ch, 1);

	Mat roi(obj.hue, obj.trackFaceRect), maskroi(obj.mask, obj.trackFaceRect);
	calcHist(&roi, 1, 0, maskroi, obj.hist, 1, &obj.hsize, &obj.phranges);

	normalize(obj.hist, obj.hist, 0, 255, CV_MINMAX);

	obj.flag = true;
}

void trackFace(TrackObj& trackObj, Mat image, float *result, int imageWidth,
		int imageHeight, JNIEnv *env, jobject obj) {
	cvtColor(image, trackObj.hsv, CV_BGR2HSV);
	int _vmin = vmin, _vmax = vmax;
	inRange(trackObj.hsv, Scalar(0, smin, MIN(_vmin, _vmax)),
			Scalar(180, 256, MAX(_vmin, _vmax)), trackObj.mask);
	int ch[] = { 0, 0 };
	trackObj.hue.create(trackObj.hsv.size(), trackObj.hsv.depth());
	mixChannels(&trackObj.hsv, 1, &trackObj.hue, 1, ch, 1);

	calcBackProject(&trackObj.hue, 1, 0, trackObj.hist, trackObj.backproj,
			&trackObj.phranges);
	trackObj.backproj &= trackObj.mask;

	RotatedRect track_box = CamShift(trackObj.backproj, trackObj.trackFaceRect,
			TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1));

//	trackObj.trackFaceRect = Rect(trackObj.trackFaceRect.x,
//			trackObj.trackFaceRect.y, trackObj.trackFaceRect.height,
//			trackObj.trackFaceRect.height);

	Rect finalRect = Rect(trackObj.trackFaceRect.x, trackObj.trackFaceRect.y,
			trackObj.trackFaceRect.height, trackObj.trackFaceRect.height);
	int scaleSize = finalRect.width - (int) ((float) finalRect.width / 5.0 * 4);
	finalRect = Rect(finalRect.x, finalRect.y + scaleSize / 2,
			finalRect.width / 2, finalRect.height - scaleSize);

//	rectangle(image,finalRect,Scalar(255,0,0));
//	imwrite("/storage/emulated/0/123.jpg",image);

//	if(!beOnce){
//		beOnce = true;
//		rectangle(image,finalRect,Scalar(255,0,0));
//		imwrite("/storage/emulated/0/123.jpg",image);
//	}

	trackObj.trackFaceRect = finalRect;

	Rect brect = trackObj.trackFaceRect;
	int x = TRACKING_IMAGE_HEIGHT - brect.y - brect.height;
	int y = brect.x;
	int width = brect.height;
	int height = brect.height;

	//Mat rotateMat;
	//rotate(image, -90, rotateMat);

	//rectangle(rotateMat,Rect(x,y,width,height),Scalar(255,0,0));
	//imwrite("/storage/emulated/0/123.jpg",rotateMat);

	float left = (float) x / (float) TRACKING_IMAGE_HEIGHT;
	float top = (float) y / (float) TRACKING_IMAGE_WIDTH;
	float right = (float) (x + width) / (float) TRACKING_IMAGE_HEIGHT;
	float bottom = (float) (y + height) / (float) TRACKING_IMAGE_WIDTH;

	result[0] = left;
	result[1] = top;
	result[2] = right;
	result[3] = bottom;

}

int detectFace(Mat image, float *result, int imageWidth, int imageHeight,
		int angle) {
	vector < Rect > faces;
	Mat frame_gray;
	cvtColor(image, frame_gray, CV_BGR2GRAY);
	equalizeHist(frame_gray, frame_gray);

	mCascade1.detectMultiScale(frame_gray, faces, 1.1, 2, 0, Size(60, 60),
			Size(2147483647, 2147483647));
	int effectiveFaceCount = 0;
	for (int i = 0; i < faces.size(); i++) {
		if (effectiveFaceCount >= MAX_FACES) {
			break;
		}

		vector < Rect > leftEyes;
		vector < Rect > rightEyes;

		Mat faceROI = image(faces[i]);
		mCascade2.detectMultiScale(faceROI, leftEyes, 1.1, 2,
				0 | CV_HAAR_SCALE_IMAGE, Size(10, 10));

		mCascade3.detectMultiScale(faceROI, rightEyes, 1.1, 2,
				0 | CV_HAAR_SCALE_IMAGE, Size(10, 10));

		if (leftEyes.size() > 0 && rightEyes.size() > 0) {
			int centerX = faces[i].x + faces[i].width / 2;
			int centerY = faces[i].y + faces[i].height / 2;
			int width = (int) ((float) faces[i].width / 5.0 * 4);
			int height = (int) ((float) faces[i].height / 5.0 * 4);

			result[4 * effectiveFaceCount + 0] = (float) (centerX - width / 2)
					/ (float) imageWidth;
			result[4 * effectiveFaceCount + 1] = (float) (centerY - height / 2)
					/ (float) imageHeight;
			result[4 * effectiveFaceCount + 2] = (float) (centerX - width / 2
					+ width) / (float) imageWidth;
			result[4 * effectiveFaceCount + 3] = (float) (centerY - height / 2
					+ height) / (float) imageHeight;

			if (enableTracking) {
				int width = (int) ((float) faces[i].width / 5.0 * 5);
				int height = (int) ((float) faces[i].height / 5.0 * 5);
				Mat rotateMat;
				rotate(image, -angle, rotateMat);
				int x = centerY - height / 2;
				int y = imageWidth - (centerX + width / 2);
				Rect rect = Rect(x, y, width, width);

				float left = (float) rect.x / (float) imageHeight;
				float top = (float) rect.y / (float) imageWidth;
				float right = (float) (rect.x + rect.width)
						/ (float) imageHeight;
				float bottom = (float) (rect.y + rect.height)
						/ (float) imageWidth;
				resize(rotateMat, rotateMat,
						Size(TRACKING_IMAGE_WIDTH, TRACKING_IMAGE_HEIGHT));
				Rect finalRect = Rect(TRACKING_IMAGE_WIDTH * left,
				TRACKING_IMAGE_HEIGHT * top,
				TRACKING_IMAGE_WIDTH * right - TRACKING_IMAGE_WIDTH * left,
				TRACKING_IMAGE_HEIGHT * bottom - TRACKING_IMAGE_HEIGHT * top);

				int scaleSize = finalRect.width
						- (int) ((float) finalRect.width / 5.0 * 4);
				finalRect = Rect(finalRect.x, finalRect.y + scaleSize / 2,
						finalRect.width / 2, finalRect.height - scaleSize);
//				rectangle(rotateMat,finalRect,Scalar(255,0,0));
//				imwrite("/storage/emulated/0/456.jpg",rotateMat);
				initTracker(rotateMat, imageWidth, imageHeight, finalRect);

			}
			effectiveFaceCount++;
		}
	}

	return effectiveFaceCount;
}

bool isMissTracking(TrackObj& trackObj, float left, float top, float right,
		float bottom) {
	if (left <= 0 || top <= 0 || right >= 1.0 || (bottom >= 1.0)) {
		return true;
	}

	int width = TRACKING_IMAGE_HEIGHT * (right - left);
	int height = TRACKING_IMAGE_WIDTH * (bottom - top) / 2;
	if (width * height > (trackObj.initSize * 1.5)) {

		LOGE("too large missing tracking. %d %d", width * height,
				trackObj.initSize);
		return true;
	}

	if (width * height < (trackObj.initSize * 0.25)) {
		LOGE("too small missing tracking.");
		return true;
	}

	for (int i = 0; i < MAX_TRACKING_OBJ_SIZE; i++) {
		TrackObj& obj = trackObjs[i];
		if (obj.index != trackObj.index) {
			if (obj.flag) {
				Rect r1 = trackObj.trackFaceRect;
				Rect r2 = obj.trackFaceRect;
				bool result = isOverlap(r1, r2);
				if (result) {
					if (r1.height > r2.height) {
						trackObj.flag = false;
					} else {
						obj.flag = false;
					}
					LOGE("overlap missing tracking.");
					return true;
				}

			}
		}
	}
	return false;
}

bool isTracking() {
	for (int i = 0; i < MAX_TRACKING_OBJ_SIZE; i++) {
		if (trackObjs[i].flag) {
			return true;
		}
	}
	return false;
}

jfloatArray Java_com_smartcamera_core_CameraManager_nativeTrackingFace(
		JNIEnv *env, jobject obj, jbyteArray yuvDatas, jint width, jint height,
		jint angle) {

	if (isTracking() && enableTracking && enableAsyncDetect) {
		clock_t time_a, time_b;
		time_a = clock();
		jbyte *pBuf = (jbyte*) env->GetByteArrayElements(yuvDatas, 0);
		Mat yuv420Mat(height + height / 2, width, CV_8UC1,
				(unsigned char *) pBuf);
		Mat bgrMat;
		cvtColor(yuv420Mat, bgrMat, CV_YUV2BGR_NV21);
		resize(bgrMat, bgrMat,
				Size(TRACKING_IMAGE_WIDTH, TRACKING_IMAGE_HEIGHT));

		int i;
		int effectiveFaceCount = 0;
		float *facesData = (float*) malloc(
		MAX_TRACKING_OBJ_SIZE * 4 * sizeof(float));
		for (i = 0; i < trackingCount; i++) {
			TrackObj &trackObj = trackObjs[i];
			if (trackObj.flag) {
				float *result = (float*) malloc(4 * sizeof(float));
				trackFace(trackObj, bgrMat, result, TRACKING_IMAGE_WIDTH,
				TRACKING_IMAGE_HEIGHT, env, obj);
				if (isMissTracking(trackObj, result[0], result[1], result[2],
						result[3])) {

					trackObj.flag = false;
					LOGE("miss tracking");
				} else {
					facesData[4 * effectiveFaceCount + 0] = result[0];
					facesData[4 * effectiveFaceCount + 1] = result[1];
					facesData[4 * effectiveFaceCount + 2] = result[2];
					facesData[4 * effectiveFaceCount + 3] = result[3];
					effectiveFaceCount++;
				}
				free(result);
			}
		}

		int size = effectiveFaceCount * 4;
		jfloatArray result = env->NewFloatArray(size);
		env->SetFloatArrayRegion(result, 0, size, facesData);
		free(facesData);
		env->ReleaseByteArrayElements(yuvDatas, pBuf, 0);
		time_b = clock();
		double duration = (double) (time_b - time_a) / CLOCKS_PER_SEC;
		//LOGE("%f", duration);
		return result;
	}

	return NULL;
}

jfloatArray Java_com_smartcamera_core_CameraManager_nativeDetectFace(
		JNIEnv *env, jobject obj, jbyteArray yuvDatas, jint width, jint height,
		jint angle) {

	if (isTracking() && !enableAsyncDetect && enableTracking) {
		jbyte *pBuf = (jbyte*) env->GetByteArrayElements(yuvDatas, 0);
		Mat yuv420Mat(height + height / 2, width, CV_8UC1,
				(unsigned char *) pBuf);
		Mat bgrMat;
		cvtColor(yuv420Mat, bgrMat, CV_YUV2BGR_NV21);
		resize(bgrMat, bgrMat,
				Size(TRACKING_IMAGE_WIDTH, TRACKING_IMAGE_HEIGHT));

		int i;
		int effectiveFaceCount = 0;
		float *facesData = (float*) malloc(
		MAX_TRACKING_OBJ_SIZE * 4 * sizeof(float));
		for (i = 0; i < trackingCount; i++) {
			TrackObj &trackObj = trackObjs[i];
			if (trackObj.flag) {
				float *result = (float*) malloc(4 * sizeof(float));
				trackFace(trackObj, bgrMat, result, TRACKING_IMAGE_WIDTH,
				TRACKING_IMAGE_HEIGHT, env, obj);
				if (isMissTracking(trackObj, result[0], result[1], result[2],
						result[3])) {

					trackObj.flag = false;
					LOGE("miss tracking");
				} else {
					facesData[4 * effectiveFaceCount + 0] = result[0];
					facesData[4 * effectiveFaceCount + 1] = result[1];
					facesData[4 * effectiveFaceCount + 2] = result[2];
					facesData[4 * effectiveFaceCount + 3] = result[3];
					effectiveFaceCount++;
				}
				free(result);
			}
		}

		int size = effectiveFaceCount * 4;
		jfloatArray result = env->NewFloatArray(size);
		env->SetFloatArrayRegion(result, 0, size, facesData);
		free(facesData);
		env->ReleaseByteArrayElements(yuvDatas, pBuf, 0);
		return result;
	}

	jbyte *pBuf = (jbyte*) env->GetByteArrayElements(yuvDatas, 0);
	Mat yuv420Mat(height + height / 2, width, CV_8UC1, (unsigned char *) pBuf);
	Mat bgrMat;
	cvtColor(yuv420Mat, bgrMat, CV_YUV2BGR_NV21);

	Mat rotateMat;
	rotate(bgrMat, angle, rotateMat);

	float *facesData = (float*) malloc(MAX_FACES * 4 * sizeof(float));
	int facesSize = detectFace(rotateMat, facesData, height, width, angle);
	int size = facesSize * 4;
	jfloatArray result = env->NewFloatArray(size);
	env->SetFloatArrayRegion(result, 0, size, facesData);
	free(facesData);

	env->ReleaseByteArrayElements(yuvDatas, pBuf, 0);

	return result;
}

jint Java_com_smartcamera_core_CameraManager_nativeEnableTracking(JNIEnv *env,
		jobject obj, jint isEnable) {
	enableTracking = isEnable == 1 ? true : false;
	if (!enableTracking) {
		for (int i = 0; i < MAX_TRACKING_OBJ_SIZE; i++) {
			trackObjs[i].flag = false;
		}
	}
	return isEnable;
}

int Java_com_smartcamera_core_CameraManager_nativeIsEnableTracking(JNIEnv *env,
		jobject obj) {
	return enableTracking;
}

int Java_com_smartcamera_core_CameraManager_nativeSetTrackingMode(JNIEnv *env,
		jobject obj, jint mode) {
	for (int i = 0; i < MAX_TRACKING_OBJ_SIZE; i++) {
		trackObjs[i].flag = false;
	}
	if (mode == 0) {
		trackingCount = 1;
	} else if (mode == 1) {
		trackingCount = 2;
	} else if (mode == 2) {
		trackingCount = MAX_TRACKING_OBJ_SIZE;
	}
}

int Java_com_smartcamera_core_CameraManager_nativeEnableAsyncDetect(JNIEnv *env,
		jobject obj, jint isEnable) {
	enableAsyncDetect = isEnable == 1 ? true : false;
	return isEnable;
}

int Java_com_smartcamera_core_CameraManager_nativeIsEnableAsyncDetect(
		JNIEnv *env, jobject obj) {
	return enableAsyncDetect;
}
