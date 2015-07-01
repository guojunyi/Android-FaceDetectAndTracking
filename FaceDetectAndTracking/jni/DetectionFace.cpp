#include "DetectionFace.h"

#include <opencv2/opencv.hpp>
#include <vector>
#include <CompressiveTracker.h>
#include <android/log.h>

#define TAG    "**guojunyi**"

#undef LOG // 取消默认的LOG#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__)#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN,TAG,__VA_ARGS__)#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,TAG,__VA_ARGS__)using namespace cv;using namespace std;static CascadeClassifier mCascade1;static CascadeClassifier mCascade2;static CascadeClassifier mCascade3;static CascadeClassifier mCascade4;static CompressiveTracker ct;int vmin = 10, vmax = 256, smin = 30;bool isTracking;Mat frame, hsv, hue, mask, hist, histimg = Mat::zeros(200, 320, CV_8UC3), backproj;bool isInitTracker;Rect trackFaceRect;
float hranges[] = { 0, 180 };
const float* phranges = hranges;
int hsize = 16;

bool enableSingleTracking;
jint Java_com_smartcamera_core_CameraManager_loadCascade(JNIEnv *env,
		jobject obj, jstring cascade1, jstring cascade2, jstring cascade3,
		jstring cascade4) {
	frame = Scalar::all(0);
	hsv = Scalar::all(0);
	hue = Scalar::all(0);
	mask = Scalar::all(0);
	hist = Scalar::all(0);
	histimg = Mat::zeros(200, 320, CV_8UC3);
	backproj = Scalar::all(0);
	isTracking = false;
	isInitTracker = false;
	enableSingleTracking = false;
	hsize = 16;

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
		cvtColor(image, hsv, CV_BGR2HSV);
		int _vmin = vmin, _vmax = vmax;
		inRange(hsv, Scalar(0, smin, MIN(_vmin, _vmax)),
					Scalar(180, 256, MAX(_vmin, _vmax)), mask);
		int ch[] = { 0, 0 };
		hue.create(hsv.size(), hsv.depth());
		mixChannels(&hsv, 1, &hue, 1, ch, 1);

		const int stateNum = 4;
		const int measureNum = 2;

		kalman = cvCreateKalman(stateNum, measureNum, 0); //state(x,y,detaX,detaY)
		CvMat* process_noise = cvCreateMat(stateNum, 1, CV_32FC1);
		measurement = cvCreateMat(measureNum, 1, CV_32FC1); //measurement(x,y)
		CvRNG rng = cvRNG(-1);
		float A[stateNum][stateNum] = { //transition matrix
				1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1 };
		memcpy(kalman->transition_matrix->data.fl, A, sizeof(A));
		cvSetIdentity(kalman->measurement_matrix, cvRealScalar(1));
		cvSetIdentity(kalman->process_noise_cov, cvRealScalar(1e-5));
		cvSetIdentity(kalman->measurement_noise_cov, cvRealScalar(1e-1));
		cvSetIdentity(kalman->error_cov_post, cvRealScalar(1));
		//initialize post state of kalman filter at random
		cvRandArr(&rng, kalman->state_post, CV_RAND_UNI, cvRealScalar(0),
				cvRealScalar(winHeight > winWidth ? winWidth : winHeight));


		Mat roi(hue, trackFaceRect), maskroi(mask, trackFaceRect);
		calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);

		normalize(hist, hist, 0, 255, CV_MINMAX);
		histimg = Scalar::all(0);
		int binW = histimg.cols / hsize;
		Mat buf(1, hsize, CV_8UC3);
		for (int i = 0; i < hsize; i++)
			buf.at < Vec3b > (i) = Vec3b(
					saturate_cast < uchar > (i * 180. / hsize), 255, 255);
		cvtColor(buf, buf, CV_HSV2BGR);
		for (int i = 0; i < hsize; i++) {
			int val = saturate_cast<int>(
					hist.at<float>(i) * histimg.rows / 255);
			rectangle(histimg, Point(i * binW, histimg.rows),
					Point((i + 1) * binW, histimg.rows - val),
					Scalar(buf.at < Vec3b > (i)), -1, 8);
		}
		calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
		backproj &= mask;

		RotatedRect track_box = CamShift(backproj, trackFaceRect,
					TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1));

		trackFaceRect = track_box.boundingRect();
		minWidth = track_box.size.width;
		minHeight = track_box.size.height;
	}
}

void trackFace(Mat image, int *result,int imageWidth, int imageHeight) {
	cvtColor(image, hsv, CV_BGR2HSV);
	int _vmin = vmin, _vmax = vmax;
	inRange(hsv, Scalar(0, smin, MIN(_vmin, _vmax)),
			Scalar(180, 256, MAX(_vmin, _vmax)), mask);
	int ch[] = { 0, 0 };
	hue.create(hsv.size(), hsv.depth());
	mixChannels(&hsv, 1, &hue, 1, ch, 1);
	calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
	backproj &= mask;

	RotatedRect track_box = CamShift(backproj, trackFaceRect,
			TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1));

	trackFaceRect = track_box.boundingRect();

//	mousePosition.x = track_box.center.x;
//	mousePosition.y = track_box.center.y;
//
//	const CvMat* prediction = cvKalmanPredict(kalman, 0);
//	CvPoint predict_pt = cvPoint((int) prediction->data.fl[0],
//			(int) prediction->data.fl[1]);
//
//	measurement->data.fl[0] = (float) mousePosition.x;
//	measurement->data.fl[1] = (float) mousePosition.y;
//
//	cvKalmanCorrect(kalman, measurement);
//
//	int iBetween = 0;
//	iBetween = sqrt(
//			powf((kalman->PosterState[0] - track_box.center.x), 2)
//					+ powf((kalman->PosterState[1] - track_box.center.y), 2));
//
//	CvPoint prePoint;
//
//	if (iBetween > 5) {
//		//当实际点 在 预测点 右边
//		if (track_box.center.x > kalman->PosterState[0]) {
//			//且，实际点在 预测点 下面
//			if (track_box.center.y > kalman->PosterState[1]) {
//				prePoint.x = track_box.center.x
//						+ iAbsolute(track_box.center.x, kalman->PosterState[0]);
//				prePoint.y = track_box.center.y
//						+ iAbsolute(track_box.center.y, kalman->PosterState[1]);
//			}
//			//且，实际点在 预测点 上面
//			else {
//				prePoint.x = track_box.center.x
//						+ iAbsolute(track_box.center.x, kalman->PosterState[0]);
//				prePoint.y = track_box.center.y
//						- iAbsolute(track_box.center.y, kalman->PosterState[1]);
//			}
//			//宽高
//			if (trackFaceRect.width != 0) {
//				trackFaceRect.width += iBetween
//						+ iAbsolute(track_box.center.x, kalman->PosterState[0]);
//			}
//			if (trackFaceRect.height != 0) {
//				trackFaceRect.height += iBetween
//						+ iAbsolute(track_box.center.x, kalman->PosterState[0]);
//			}
//		}
//		//当实际点 在 预测点 左边
//		else {
//			//且，实际点在 预测点 下面
//			if (track_box.center.y > kalman->PosterState[1]) {
//				prePoint.x = track_box.center.x
//						- iAbsolute(track_box.center.x, kalman->PosterState[0]);
//				prePoint.y = track_box.center.y
//						+ iAbsolute(track_box.center.y, kalman->PosterState[1]);
//			}
//			//且，实际点在 预测点 上面
//			else {
//				prePoint.x = track_box.center.x
//						- iAbsolute(track_box.center.x, kalman->PosterState[0]);
//				prePoint.y = track_box.center.y
//						- iAbsolute(track_box.center.y, kalman->PosterState[1]);
//			}
//			//宽高
//			if (trackFaceRect.width != 0) {
//				trackFaceRect.width += iBetween
//						+ iAbsolute(track_box.center.x, kalman->PosterState[0]);
//			}
//			if (trackFaceRect.height != 0) {
//				trackFaceRect.height += iBetween
//						+ iAbsolute(track_box.center.x, kalman->PosterState[0]);
//			}
//		}
//		trackFaceRect.x = prePoint.x - iBetween;
//		trackFaceRect.y = prePoint.y - iBetween;
//	} else {
//		trackFaceRect.x -= iBetween;
//		trackFaceRect.y -= iBetween;
//		//宽高
//		if (trackFaceRect.width != 0) {
//			trackFaceRect.width += iBetween;
//		}
//		if (trackFaceRect.height != 0) {
//			trackFaceRect.height += iBetween;
//		}
//	}
//	//跟踪的矩形框不能小于初始化检测到的大小，当这个情况的时候，X 和 Y可以适当的在缩小
//	if (trackFaceRect.width < minWidth) {
//		trackFaceRect.width = minWidth;
//		trackFaceRect.x -= iBetween;
//	}
//	if (trackFaceRect.height < minHeight) {
//		trackFaceRect.height = minHeight;
//		trackFaceRect.y -= iBetween;
//	}
//	//确保调整后的矩形大小在640 * 480之内
//	if (trackFaceRect.x <= 0) {
//		trackFaceRect.x = 0;
//	}
//	if (trackFaceRect.y <= 0) {
//		trackFaceRect.y = 0;
//	}
//	if (trackFaceRect.x >= imageWidth-40) {
//		trackFaceRect.x = imageWidth-40;
//	}
//	if (trackFaceRect.y >= imageHeight-40) {
//		trackFaceRect.y = imageHeight-40;
//	}
//	if (trackFaceRect.width + trackFaceRect.x >= imageWidth) {
//		trackFaceRect.width = imageWidth - trackFaceRect.x;
//	}
//	if (trackFaceRect.height + trackFaceRect.y >= imageWidth) {
//		trackFaceRect.height = imageWidth - trackFaceRect.y;
//	}

	Rect brect = track_box.boundingRect();
	int centerX = brect.x + brect.width / 2;
	int centerY = brect.y + brect.height / 2;
	int width = (int) ((float) brect.width / 5.0 * 4);
	int height = (int) ((float) brect.height / 5.0 * 4);
	result[0] = brect.x;
	result[1] = brect.y;
	result[2] = brect.width;
	result[3] = brect.height;
	result[4] = 1;
	result[5] = 1;
}

#define MAX_FACES 30
#define FIND_FACE_COUNT 1

int globalSize = 100;
int detectFace(Mat image, int *result,int imageWidth,int imageHeight) {
	vector < Rect > faces;
	Mat frame_gray;
	cvtColor(image, frame_gray, CV_BGR2GRAY);
	equalizeHist(frame_gray, frame_gray);

//	face_cascade.detectMultiScale(frame_gray, faces, 1.1, 2,
//				CV_HAAR_FIND_BIGGEST_OBJECT | CV_HAAR_DO_CANNY_PRUNING,
//				Size(30, 30));

	mCascade1.detectMultiScale(frame_gray, faces, 1.1, 2,
				0, Size(20,20),Size(2147483647,2147483647));
	int bestFaceCount = 0;
	for (int i = 0; i < faces.size(); i++) {
		if (i >= MAX_FACES) {
			break;
		}

		if(enableSingleTracking&&bestFaceCount>=FIND_FACE_COUNT){
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

			if(enableSingleTracking){
				trackFaceRect = Rect(result[5 * i + 0],result[5 * i + 1],result[5 * i + 2],result[5 * i + 3]);
				initTracker(image,imageWidth,imageHeight);
			}
			LOGE("start tracking");
			flag = true;

		}

		if (flag) {
			if(enableSingleTracking){
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
	jbyte *pBuf = (jbyte*) env->GetByteArrayElements(yuvDatas, 0);
	Mat yuv420Mat(height + height / 2, width, CV_8UC1, (unsigned char *) pBuf);
	Mat bgrMat;
	cvtColor(yuv420Mat, bgrMat, CV_YUV2BGR_NV21);

	Mat rotateMat;
	rotate(bgrMat, angle, rotateMat);

	if (isTracking&&enableSingleTracking) {
		int *facesData = (int*) malloc(6 * sizeof(int));
		trackFace(rotateMat, facesData,height,width);
		if (facesData[0] < 0 || facesData[1] < 0
				|| (facesData[0] + facesData[2] > height)
				|| (facesData[1] + facesData[3] > width)) {
			facesData[4] = 0;
			facesData[5] = 0;
			isTracking = false;
			isInitTracker = false;
			LOGE("miss tracking");
			LOGE("Tracking %i %i %i %i", facesData[0], facesData[1],
					facesData[2], facesData[3]);
		}

		jintArray result = env->NewIntArray(6);
		env->SetIntArrayRegion(result, 0, 6, facesData);
		free(facesData);
		env->ReleaseByteArrayElements(yuvDatas, pBuf, 0);
		return result;
	}

//imwrite("/storage/emulated/0/123.jpg",rotateMat);
	int *facesData = (int*) malloc(MAX_FACES * 5 * sizeof(int) + sizeof(int));
	int facesSize = detectFace(rotateMat, facesData,height,width);
	int size = facesSize * 5 + 1;
	jintArray result = env->NewIntArray(size);
	env->SetIntArrayRegion(result, 0, size, facesData);
	free(facesData);

	env->ReleaseByteArrayElements(yuvDatas, pBuf, 0);
	return result;
}

jint Java_com_smartcamera_core_CameraManager_enableSingleTracking(JNIEnv *env,
		jobject obj, jint isEnable) {
	enableSingleTracking = isEnable==1?true:false;

	return isEnable;
}
