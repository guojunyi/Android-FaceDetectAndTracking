# Android-FaceDetectAndTracking

## Sample Application
<a href="https://raw.githubusercontent.com/guojunyi/Android-FaceDetectAndTracking
/master/apk/FaceDetectAndTracking.apk" target="_blank" title="Download From Google Play">Click to Download the simple apk</a>

##Other
我用opencv detectMultiScale方法先扫描人脸 因为需要远距离锁定人脸 所以minSize设置的非常小 这样导致扫描非常耗时，所以用了camshift跟踪算法，但是碰到一个问题，单人简单背景下（背景和人脸色差区别大等..），效果勉强可以用，但是多人或者复杂背景出现一些奇怪的现象。</br></br>
真心希望知道的大神能给我指点一些方向 1413111736@qq.com 不胜感激 :blush:

##2015-07-03
因为扫描人脸需要正脸扫描 所以在jni对图片进行了90度旋转再扫描人脸，而camshit追踪不依赖于物体的形状变化所以
在追踪时取消图片旋转，最终对Rect坐标旋转，速度按我的测试机从130ms左右提高到60ms左右，差不多一倍的速度，丢失的情况也好一点了.


##2015-07-08
为了更远距离精准扫描人脸 摄像头预览大小设置成最大，以我的测试机为例是1280x960,而camshift 不依赖物体大小变化所以在追踪时对图片进行压缩640x480(太小也不行，容易变形)，速度从60ms每次提高在35ms左右，
另外增加丢失判断追踪区域大小大于初始化大小2.5倍或者小于初始化大小0.25视为严重变形判断为丢失.
``` java
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
``` 

