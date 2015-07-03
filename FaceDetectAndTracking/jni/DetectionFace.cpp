#include "DetectionFace.h"

#include <opencv2/opencv.hpp>
#include <vector>
#include "TrackObj.h"
#include <android/log.h>

#define TAG    "**guojunyi**"

#undef LOG // 取消默认的LOG#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__)#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN,TAG,__VA_ARGS__)#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,TAG,__VA_ARGS__)using namespace cv;using namespace std;static CascadeClassifier mCascade1;static CascadeClassifier mCascade2;static CascadeClassifier mCascade3;static CascadeClassifier mCascade4;int vmin = 65, vmax = 256, smin = 55;bool isTracking;bool isInitTracker;bool enableSingleTracking;TrackObj trackObj;
jint Java_com_smartcamera_core_CameraManager_loadCascade(JNIEnv *env,
		jobject obj, jstring cascade1, jstring cascade2, jstring cascade3,
		jstring cascade4) {

	isTracking = false;
	isInitTracker = false;
	enableSingleTracking = false;

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

jintArray Java_com_smartcamera_core_CameraManager_decodeYUV420(JNIEnv *env,
		jobject obj, jbyteArray buf, jint width, jint height) {
	jbyte * yuv420sp = env->GetByteArrayElements(buf, 0);
	int frameSize = width * height;
	int *rgb = (int*) malloc(width * height * sizeof(int));
	memset(rgb, 0, width * height * sizeof(int));
	int i = 0, j = 0, yp = 0;
	int uvp = 0, u = 0, v = 0;
	for (j = 0, yp = 0; j < height; j++) {
		uvp = frameSize + (j >> 1) * width;
		u = 0;
		v = 0;
		for (i = 0; i < width; i++, yp++) {
			int y = (0xff & ((int) yuv420sp[yp])) - 16;
			if (y < 0)
				y = 0;
			if ((i & 1) == 0) {
				v = (0xff & yuv420sp[uvp++]) - 128;
				u = (0xff & yuv420sp[uvp++]) - 128;
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

	jintArray result = env->NewIntArray(frameSize);
	env->SetIntArrayRegion(result, 0, frameSize, rgb);
	free(rgb);
	env->ReleaseByteArrayElements(buf, yuv420sp, 0);
	return result;
}

jintArray Java_com_smartcamera_core_CameraManager_grayImage(JNIEnv *env,
		jobject obj, jintArray buf, jint w, jint h) {
	jint *cbuf = env->GetIntArrayElements(buf, 0);
	Mat mat(h, w, CV_8UC4, cbuf);

	for (int j = 0; j < mat.rows / 2; j++) {
		mat.row(j).setTo(Scalar(125, 125, 125, 125));
	}
	int size = w * h;
	jintArray result = env->NewIntArray(size);
	env->SetIntArrayRegion(result, 0, size, cbuf);
	env->ReleaseIntArrayElements(buf, cbuf, 0);
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

int iAbsolute(int a, int b) {
	int c = 0;
	if (a > b) {
		c = a - b;
	} else {
		c = b - a;
	}
	return c;
}

CvPoint mousePosition;
CvKalman *kalman;
CvMat* measurement;
int minWidth;
int minHeight;

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

//		histimg = Scalar::all(0);
//		int binW = histimg.cols / hsize;
//		Mat buf(1, hsize, CV_8UC3);
//		for (int i = 0; i < hsize; i++)
//			buf.at < Vec3b > (i) = Vec3b(
//					saturate_cast < uchar > (i * 180. / hsize), 255, 255);
//		cvtColor(buf, buf, CV_HSV2BGR);
//		for (int i = 0; i < hsize; i++) {
//			int val = saturate_cast<int>(
//					hist.at<float>(i) * histimg.rows / 255);
//			rectangle(histimg, Point(i * binW, histimg.rows),
//					Point((i + 1) * binW, histimg.rows - val),
//					Scalar(buf.at < Vec3b > (i)), -1, 8);
//		}
//		calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
//		backproj &= mask;
//
//		RotatedRect track_box = CamShift(backproj, trackFaceRect,
//					TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1));
//
//		trackFaceRect = track_box.boundingRect();
//		minWidth = track_box.size.width;
//		minHeight = track_box.size.height;
//		minWidth = trackFaceRect.width;
//		minHeight = trackFaceRect.height;

	}
}

void trackFace(Mat image, int *result, int imageWidth, int imageHeight) {

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

	Rect brect = track_box.boundingRect();
	int centerX = brect.x + brect.width / 2;
	int centerY = brect.y + brect.height / 2;
	int width = (int) ((float) brect.width / 5.0 * 4);
	int height = (int) ((float) brect.height / 5.0 * 4);
	result[0] = imageHeight - brect.y - brect.height;
	result[1] = brect.x;
	result[2] = brect.height;
	result[3] = brect.width;

//	result[0] = brect.x;
//	result[1] = brect.y;
//	result[2] = brect.width;
//	result[3] = brect.height;

	result[4] = 1;
	result[5] = 1;
}

#define MAX_FACES 30
#define FIND_FACE_COUNT 1

int detectFace(Mat image, int *result, int imageWidth, int imageHeight,
		int angle) {
	vector < Rect > faces;
	Mat frame_gray;
	cvtColor(image, frame_gray, CV_BGR2GRAY);
	equalizeHist(frame_gray, frame_gray);

//	face_cascade.detectMultiScale(frame_gray, faces, 1.1, 2,
//				CV_HAAR_FIND_BIGGEST_OBJECT | CV_HAAR_DO_CANNY_PRUNING,
//				Size(30, 30));

	mCascade1.detectMultiScale(frame_gray, faces, 1.1, 2, 0, Size(20, 20),
			Size(2147483647, 2147483647));
	int bestFaceCount = 0;
	for (int i = 0; i < faces.size(); i++) {
		if (i >= MAX_FACES) {
			break;
		}

		if (enableSingleTracking && bestFaceCount >= FIND_FACE_COUNT) {
			break;
		}

		int centerX = faces[i].x + faces[i].width / 2;
		int centerY = faces[i].y + faces[i].height / 2;
		int width = (int) ((float) faces[i].width / 5.0 * 4);
		int height = (int) ((float) faces[i].height / 5.0 * 4);
		result[5 * i + 0] = centerX - width / 2;
		result[5 * i + 1] = centerY - height / 2;
		result[5 * i + 2] = width;
		result[5 * i + 3] = height;

		vector < Rect > noses;
		vector < Rect > eyes;
		//vector < Rect > mouthes;
		Mat faceROI = image(faces[i]);
		mCascade2.detectMultiScale(faceROI, noses, 1.1, 2,
				0 | CV_HAAR_SCALE_IMAGE, Size(5, 5));

		mCascade3.detectMultiScale(faceROI, eyes, 1.1, 2,
				0 | CV_HAAR_SCALE_IMAGE, Size(5, 5));

//		mouth_cascade.detectMultiScale(faceROI, mouthes, 1.1, 2,
//				0 | CV_HAAR_SCALE_IMAGE, Size(30, 30));
		bool flag = false;

		if (noses.size() > 0 && eyes.size() > 0) {
			bestFaceCount++;
//			result[5 * i + 0] = noses[0].x+faces[i].x;
//			result[5 * i + 1] = noses[0].y+faces[i].y;
//			result[5 * i + 2] = noses[0].width;
//			result[5 * i + 3] = noses[0].height;
//			int centerX = result[5 * i + 0]+result[5 * i + 2]/2;
//			int centerY = result[5 * i + 1]+result[5 * i + 3]/2;
//			int width = result[5 * i + 2]/2;
//			int height = result[5 * i + 3]/2;
//
//
//			result[5 * i + 0] = centerX-width/2;
//			result[5 * i + 1] = centerY-height/2;
//			result[5 * i + 2] = width;
//			result[5 * i + 3] = height;

			if (enableSingleTracking) {
				Mat rotateMat;
				rotate(image, -angle, rotateMat);
//				trackFaceRect = Rect(result[5 * i + 0], result[5 * i + 1],
//						result[5 * i + 2], result[5 * i + 3]);

				int x = centerY - height / 2;
				int y = imageHeight - (centerX + width / 2);
				Rect rect = Rect(x, y, result[5 * i + 3], result[5 * i + 2]);

				trackObj.trackFaceRect = rect;
//				rectangle(rotateMat,trackFaceRect,Scalar(255,0,0));
//				imwrite("/storage/emulated/0/123.jpg",rotateMat);

				initTracker(rotateMat, imageWidth, imageHeight);
			}
			LOGE("start tracking");
			flag = true;

		}

		if (flag) {
			if (enableSingleTracking) {
				isTracking = true;
			}
			result[5 * i + 4] = 1;
		} else {
			isTracking = false;
			result[5 * i + 4] = 0;
		}
	}
	result[faces.size() * 5] = faces.size();

	return faces.size();
}

jintArray Java_com_smartcamera_core_CameraManager_detectFaceX(JNIEnv *env,
		jobject obj, jbyteArray yuvDatas, jint width, jint height, jint angle) {
	clock_t time_a, time_b;
	time_a = clock();
	jbyte *pBuf = (jbyte*) env->GetByteArrayElements(yuvDatas, 0);
	Mat yuv420Mat(height + height / 2, width, CV_8UC1, (unsigned char *) pBuf);
	Mat bgrMat;
	cvtColor(yuv420Mat, bgrMat, CV_YUV2BGR_NV21);

	if (isTracking && enableSingleTracking) {
		//imwrite("/storage/emulated/0/123.jpg",bgrMat);
		int *facesData = (int*) malloc(6 * sizeof(int));
		trackFace(bgrMat, facesData, width, height);
		if (facesData[0] <= 0 || facesData[1] <= 0
				|| (facesData[0] + facesData[2] >= height)
				|| (facesData[1] + facesData[3] >= width)) {
			facesData[4] = 0;
			facesData[5] = 0;
			isTracking = false;
			isInitTracker = false;
			LOGE("miss tracking");
		}
		jintArray result = env->NewIntArray(6);
		env->SetIntArrayRegion(result, 0, 6, facesData);
		free(facesData);
		env->ReleaseByteArrayElements(yuvDatas, pBuf, 0);
		time_b = clock();
		double duration = (double) (time_b - time_a) / CLOCKS_PER_SEC;
		LOGE("%f", duration);
		return result;
	}

	Mat rotateMat;
	rotate(bgrMat, angle, rotateMat);

	int *facesData = (int*) malloc(MAX_FACES * 5 * sizeof(int) + sizeof(int));
	int facesSize = detectFace(rotateMat, facesData, width, height, angle);
	int size = facesSize * 5 + 1;
	jintArray result = env->NewIntArray(size);
	env->SetIntArrayRegion(result, 0, size, facesData);
	free(facesData);

	env->ReleaseByteArrayElements(yuvDatas, pBuf, 0);

	time_b = clock();
	double duration = (double) (time_b - time_a) / CLOCKS_PER_SEC;
	LOGE("%f", duration);
	return result;
}

jint Java_com_smartcamera_core_CameraManager_enableSingleTracking(JNIEnv *env,
		jobject obj, jint isEnable) {
	enableSingleTracking = isEnable == 1 ? true : false;

	return isEnable;
}
