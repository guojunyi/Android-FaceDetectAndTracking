# Android-FaceDetectAndTracking

## Sample Application
<a href="https://raw.githubusercontent.com/guojunyi/Android-FaceDetectAndTracking
/master/apk/FaceDetectAndTracking.apk" target="_blank" title="Download From Google Play">Click to Download the simple apk</a>

##Other

关于人脸追踪有什么好的建议希望能发送1413111736@qq.com :blush:

##2015-07-03
因为扫描人脸需要正脸扫描 所以在jni对图片进行了90度旋转再扫描人脸，而camshit追踪不依赖于物体的形状变化所以
在追踪时取消图片旋转，最终对Rect坐标旋转，速度按我的测试机从130ms左右提高到60ms左右，差不多一倍的速度，丢失的情况也好一点了.


##2015-07-08
为了更远距离精准扫描人脸 摄像头预览大小设置成最大，以我的测试机为例是1280x960,而camshift 不依赖物体大小变化所以在追踪时对图片进行压缩640x480(太小也不行，容易变形)，速度从60ms每次提高在35ms左右，
另外增加丢失判断追踪区域大小大于初始化大小2.5倍或者小于初始化大小0.25视为严重变形判断为丢失,快速移动下丢失情况又好一点了，但复杂背景色差变形依然存在.


##2015-07-11
增加多人追踪，异步检测，camshift追踪区域从整个脸部缩小范围锁定额头，复杂背景下更容易锁定不易变形。

