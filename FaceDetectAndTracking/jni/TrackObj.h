#include <opencv2/opencv.hpp>

using namespace cv;

#ifdef __cplusplus
extern "C" {
#endif

class TrackObj
{

public:

	TrackObj(void)
	{
		hranges[0] = 0;
		hranges[1] = 180;
		phranges = hranges;
		hsize = 16;
		flag = false;
	}

	Rect trackFaceRect;
	Mat frame;
	Mat hsv;
	Mat hue;
	Mat mask;
	Mat hist;
	Mat histimg;
	Mat backproj;

	int initSize;
	int hsize;
	float hranges[2];
	const float* phranges;

	bool flag;
};

#ifdef __cplusplus
}
#endif
