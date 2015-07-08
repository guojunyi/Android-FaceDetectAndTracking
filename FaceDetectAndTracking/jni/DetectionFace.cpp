#include "DetectionFace.h"

#include <opencv2/opencv.hpp>
#include <vector>
#include "TrackObj.h"
#include <android/log.h>
#define TAG    "**guojunyi**"

#undef LOG // 取消默认的LOG#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__)#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN,TAG,__VA_ARGS__)#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,TAG,__VA_ARGS__)using namespace cv;using namespace std;static CascadeClassifier mCascade1;
static CascadeClassifier mCascade2;
static CascadeClassifier mCascade3;
static CascadeClassifier mCascade4;
int vmin = 65, vmax = 256, smin = 55;
bool isTracking;
bool isInitTracker;
bool enableSingleTracking;
TrackObj trackObj;
int trackingCount;
#define MAX_FACES 30

#define TRACKING_IMAGE_WIDTH 640
#define TRACKING_IMAGE_HEIGHT 480

jint Java_com_smartcamera_core_CameraManager_loadCascade(JNIEnv *env,
		jobject obj, jstring cascade1, jstring cascade2, jstring cascade3,
		jstring cascade4) {

	isTracking = false;
	isInitTracker = false;
	enableSingleTracking = false;
	trackingCount = 0;
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
	Rect bbox = cv::RotatedRect(center, src.size(), angle).boundingRect();

	// adjust transformation matrix
	rot.at<double>(0, 2) += bbox.width / 2.0 - center.x;
	rot.at<double>(1, 2) += bbox.height / 2.0 - center.y;

	warpAffine(src, dst, rot, bbox.size());
}

void initTracker(Mat image, int winWidth, int winHeight) {
	if (!isInitTracker) {
		isInitTracker = true;
		TrackObj &obj = trackObj;

		obj.flag = true;

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

		Mat roi(obj.hue, obj.trackFaceRect), maskroi(obj.mask,
				obj.trackFaceRect);
		calcHist(&roi, 1, 0, maskroi, obj.hist, 1, &obj.hsize, &obj.phranges);

		normalize(obj.hist, obj.hist, 0, 255, CV_MINMAX);
	}
}

void missTracking(JNIEnv *env, jobject obj, double value) {
	jclass native_clazz = env->GetObjectClass(obj);
	jmethodID methodID = env->GetMethodID(native_clazz, "showHanming", "(D)I");
	env->CallIntMethod(obj, methodID, value);
}

void trackFace(Mat image, float *result, int imageWidth, int imageHeight,
		JNIEnv *env, jobject obj) {
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

	trackObj.trackFaceRect = Rect(trackObj.trackFaceRect.x,
			trackObj.trackFaceRect.y, trackObj.trackFaceRect.height,
			trackObj.trackFaceRect.height);

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

	mCascade1.detectMultiScale(frame_gray, faces, 1.1, 2, 0, Size(20, 20),
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
				0 | CV_HAAR_SCALE_IMAGE, Size(5, 5));

		mCascade3.detectMultiScale(faceROI, rightEyes, 1.1, 2,
				0 | CV_HAAR_SCALE_IMAGE, Size(5, 5));

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
			if (enableSingleTracking) {
				if (!isTracking) {
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
					trackObj.trackFaceRect = Rect(TRACKING_IMAGE_WIDTH * left,
					TRACKING_IMAGE_HEIGHT * top,
					TRACKING_IMAGE_WIDTH * right - TRACKING_IMAGE_WIDTH * left,
							TRACKING_IMAGE_HEIGHT * bottom
									- TRACKING_IMAGE_HEIGHT * top);

					trackObj.initSize = trackObj.trackFaceRect.width
							* trackObj.trackFaceRect.height;
					initTracker(rotateMat, imageWidth, imageHeight);
				}

			}
			effectiveFaceCount++;
		}

		if (!isTracking) {
			if (effectiveFaceCount > 0 && enableSingleTracking) {
				isTracking = true;
			} else {
				isTracking = false;
			}
		}
	}

	return effectiveFaceCount;
}

bool isMissTracking(float left, float top, float right, float bottom) {
	if (left <= 0 || top <= 0 || right >= 1.0 || (bottom >= 1.0)) {
		return true;
	}

	int width = TRACKING_IMAGE_WIDTH * (right - left);
	int height = TRACKING_IMAGE_HEIGHT * (bottom - top);
	if (width * height > (trackObj.initSize * 2.5)) {
		LOGE("too large missing tracking.");
		return true;
	}

	if (width * height < (trackObj.initSize * 0.25)) {
		LOGE("too small missing tracking.");
		return true;
	}
	return false;
}

jfloatArray Java_com_smartcamera_core_CameraManager_nativeTrackingFace(
		JNIEnv *env, jobject obj, jbyteArray yuvDatas, jint width, jint height,
		jint angle) {

	if (isTracking) {
		clock_t time_a, time_b;
		time_a = clock();
		jbyte *pBuf = (jbyte*) env->GetByteArrayElements(yuvDatas, 0);
		Mat yuv420Mat(height + height / 2, width, CV_8UC1,
				(unsigned char *) pBuf);
		Mat bgrMat;
		cvtColor(yuv420Mat, bgrMat, CV_YUV2BGR_NV21);
		resize(bgrMat, bgrMat,
				Size(TRACKING_IMAGE_WIDTH, TRACKING_IMAGE_HEIGHT));
		float *facesData = (float*) malloc(4 * sizeof(float));
		trackFace(bgrMat, facesData, TRACKING_IMAGE_WIDTH,
				TRACKING_IMAGE_HEIGHT, env, obj);
		if (isMissTracking(facesData[0], facesData[1], facesData[2],
				facesData[3])) {
			isTracking = false;
			isInitTracker = false;
			LOGE("miss tracking");

			free(facesData);
			env->ReleaseByteArrayElements(yuvDatas, pBuf, 0);
			return NULL;
		}

		jfloatArray result = env->NewFloatArray(5);
		env->SetFloatArrayRegion(result, 0, 4, facesData);
		free(facesData);
		env->ReleaseByteArrayElements(yuvDatas, pBuf, 0);
		time_b = clock();
		double duration = (double) (time_b - time_a) / CLOCKS_PER_SEC;
		LOGE("%f", duration);
		return result;
	}

	return NULL;
}

jfloatArray Java_com_smartcamera_core_CameraManager_nativeDetectFace(
		JNIEnv *env, jobject obj, jbyteArray yuvDatas, jint width, jint height,
		jint angle) {

	if (isTracking) {
		clock_t time_a, time_b;
		time_a = clock();
		jbyte *pBuf = (jbyte*) env->GetByteArrayElements(yuvDatas, 0);
		Mat yuv420Mat(height + height / 2, width, CV_8UC1,
				(unsigned char *) pBuf);
		Mat bgrMat;
		cvtColor(yuv420Mat, bgrMat, CV_YUV2BGR_NV21);
		resize(bgrMat, bgrMat,
				Size(TRACKING_IMAGE_WIDTH, TRACKING_IMAGE_HEIGHT));
		float *facesData = (float*) malloc(4 * sizeof(float));
		trackFace(bgrMat, facesData, TRACKING_IMAGE_WIDTH,
				TRACKING_IMAGE_HEIGHT, env, obj);
		if (isMissTracking(facesData[0], facesData[1], facesData[2],
				facesData[3])) {
			isTracking = false;
			isInitTracker = false;
			LOGE("miss tracking");

			free(facesData);
			env->ReleaseByteArrayElements(yuvDatas, pBuf, 0);
			return NULL;
		}

		jfloatArray result = env->NewFloatArray(5);
		env->SetFloatArrayRegion(result, 0, 4, facesData);
		free(facesData);
		env->ReleaseByteArrayElements(yuvDatas, pBuf, 0);
		time_b = clock();
		double duration = (double) (time_b - time_a) / CLOCKS_PER_SEC;
		LOGE("%f", duration);
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

jint Java_com_smartcamera_core_CameraManager_enableSingleTracking(JNIEnv *env,
		jobject obj, jint isEnable) {
	enableSingleTracking = isEnable == 1 ? true : false;
	isTracking = false;
	isInitTracker = false;
	return isEnable;
}
