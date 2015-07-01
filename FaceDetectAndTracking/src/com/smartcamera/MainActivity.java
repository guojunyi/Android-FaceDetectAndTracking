package com.smartcamera;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame;
import org.opencv.core.Mat;
import org.opencv.core.MatOfRect;
import org.opencv.core.Rect;
import org.opencv.core.Scalar;
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;
import org.opencv.objdetect.CascadeClassifier;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.WindowManager;

import com.smartcamera.widget.XCameraView;
import com.smartcamera.widget.XCameraView.CvCameraViewListener2;

public class MainActivity extends Activity implements CvCameraViewListener2{
 
	static {
		System.loadLibrary("opencv_java3");
	}

	XCameraView cameraView;
	Handler mHandler = new Handler(Looper.getMainLooper());

	private File                   mCascadeFile;
    private CascadeClassifier      mJavaDetector;
	
    private Mat                    mRgba;
    private Mat                    mGray;
    
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		
		
		super.onCreate(savedInstanceState);
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		setContentView(R.layout.activity_main_x);
		cameraView = (XCameraView) findViewById(R.id.cameraView);
		cameraView.setCvCameraViewListener(this);
		initOpencv();
	}

	@Override
	public void onResume() {
		super.onResume();
	}

	@Override
	public void onStop() {
		super.onStop();
		cameraView.disconnectCamera();
		mGray.release();
        mRgba.release();
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		// CameraManager.getInstance().closeCamera();
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
	}

	public void initOpencv() {
		try{
			InputStream is = getResources().openRawResource(R.raw.lbpcascade_frontalface);
	        File cascadeDir = getDir("cascade", Context.MODE_PRIVATE);
	        mCascadeFile = new File(cascadeDir, "lbpcascade_frontalface.xml");
	        FileOutputStream os = new FileOutputStream(mCascadeFile);

	        byte[] buffer = new byte[4096];
	        int bytesRead;
	        while ((bytesRead = is.read(buffer)) != -1) {
	            os.write(buffer, 0, bytesRead);
	        }
	        is.close();
	        os.close();

	        mJavaDetector = new CascadeClassifier(mCascadeFile.getAbsolutePath());
	        

	        cascadeDir.delete();
	        
	        cameraView.connectCamera();
	        
	        mGray = new Mat();
	        mRgba = new Mat();
		}catch(Exception e){
			e.printStackTrace();
		}
	}

	
	long time;
	private float                  mRelativeFaceSize   = 0.2f;
    private int                    mAbsoluteFaceSize   = 0;
    private static final Scalar    FACE_RECT_COLOR     = new Scalar(0, 255, 0, 255);
	@Override
	public Mat onCameraFrame(CvCameraViewFrame inputFrame) {
		// TODO Auto-generated method stub
		mRgba = inputFrame.rgba();
        mGray = inputFrame.gray();

        MatOfRect faces = new MatOfRect();
        if (mAbsoluteFaceSize == 0) {
            int height = mGray.rows();
            if (Math.round(height * mRelativeFaceSize) > 0) {
                mAbsoluteFaceSize = Math.round(height * mRelativeFaceSize);
            }
        }
        
        if (mJavaDetector != null)
        	mJavaDetector.detectMultiScale(mGray, faces, 1.1, 2, 2, // TODO: objdetect.CV_HAAR_SCALE_IMAGE
                    new Size(mAbsoluteFaceSize, mAbsoluteFaceSize), new Size());
        Rect[] facesArray = faces.toArray();
        for (int i = 0; i < facesArray.length; i++)
            Imgproc.rectangle(mRgba, facesArray[i].tl(), facesArray[i].br(), FACE_RECT_COLOR, 3);

        return mRgba;
	}
	

}
