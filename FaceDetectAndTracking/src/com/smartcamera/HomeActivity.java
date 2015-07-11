package com.smartcamera;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.app.Activity;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.TextView;

import com.smartcamera.core.CameraManager;
import com.smartcamera.widget.CameraFaceFrameView;
import com.smartcamera.widget.CameraView;
import com.smartcamera.widget.CameraView.OnDetectEndListener;

public class HomeActivity extends Activity{

	static {
		System.loadLibrary("opencv_java3");
		System.loadLibrary("face");
	}

	CameraView cameraView;
	CameraFaceFrameView faceFrameView;
	Handler mHandler = new Handler(Looper.getMainLooper());

	public TextView textView;
	CheckBox checkBox1,checkBox2;
	RadioButton radio1,radio2,radio3;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		setContentView(R.layout.activity_main);

		cameraView = (CameraView) findViewById(R.id.cameraView);
		faceFrameView = (CameraFaceFrameView) findViewById(R.id.faceFrameView);
		cameraView.setCameraFaceFrameView(faceFrameView);

		initOpencv();
		
		checkBox1 = (CheckBox) findViewById(R.id.checkBox1);
		checkBox1.setOnClickListener(new OnClickListener(){

			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				if(CameraManager.getInstance().nativeIsEnableTracking()==1){
					checkBox1.setChecked(false);
					CameraManager.getInstance().nativeEnableTracking(0);
				}else{
					checkBox1.setChecked(true);
					CameraManager.getInstance().nativeEnableTracking(1);
				}
			}
			
		});
		
		checkBox2 = (CheckBox) findViewById(R.id.checkBox2);
		checkBox2.setOnClickListener(new OnClickListener(){

			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				if(CameraManager.getInstance().nativeIsEnableAsyncDetect()==1){
					checkBox2.setChecked(false);
					CameraManager.getInstance().nativeEnableAsyncDetect(0);
				}else{
					checkBox2.setChecked(true);
					CameraManager.getInstance().nativeEnableAsyncDetect(1);
				}
			}
			
		});
		
		
		textView = (TextView) findViewById(R.id.textView);
		
		
		radio1 = (RadioButton) findViewById(R.id.radio1);
		radio2 = (RadioButton) findViewById(R.id.radio2);
		radio3 = (RadioButton) findViewById(R.id.radio3);
		radio1.setOnCheckedChangeListener(new OnCheckedChangeListener(){

			@Override
			public void onCheckedChanged(CompoundButton buttonView,
					boolean isChecked) {
				// TODO Auto-generated method stub
				if(isChecked){
					CameraManager.getInstance().nativeSetTrackingMode(0);
				}
				
			}
			
		});
		
		radio2.setOnCheckedChangeListener(new OnCheckedChangeListener(){

			@Override
			public void onCheckedChanged(CompoundButton buttonView,
					boolean isChecked) {
				// TODO Auto-generated method stub
				if(isChecked){
					CameraManager.getInstance().nativeSetTrackingMode(1);
				}
			}
			
		});
		
		radio3.setOnCheckedChangeListener(new OnCheckedChangeListener(){

			@Override
			public void onCheckedChanged(CompoundButton buttonView,
					boolean isChecked) {
				// TODO Auto-generated method stub
				if(isChecked){
					CameraManager.getInstance().nativeSetTrackingMode(2);
				}
			}
			
		});
		
	}

	@Override
	public void onResume() {
		super.onResume();

	}

	@Override
	public void onStop() {
		super.onStop();
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
	}

	public void initOpencv() {
		try {
			// load cascade file from application resources
			InputStream is = getResources().openRawResource(
					R.raw.lbpcascade_frontalface);
			File mCascadeFile = new File(Environment
					.getExternalStorageDirectory().getAbsolutePath()
					+ "/cascade1.xml");
			if (!mCascadeFile.exists()) {
				mCascadeFile.createNewFile();
			} else {
				mCascadeFile.delete();
				mCascadeFile.createNewFile();
			}
			FileOutputStream os = new FileOutputStream(mCascadeFile);
			byte[] buffer = new byte[4096];
			int bytesRead;
			while ((bytesRead = is.read(buffer)) != -1) {
				os.write(buffer, 0, bytesRead);
			}
			is.close();
			os.close();
		} catch (IOException e) {
			e.printStackTrace();
		}

		try {
			// load cascade file from application resources
			InputStream is = getResources().openRawResource(
					R.raw.haarcascade_mcs_lefteye);
			File mCascadeFile = new File(Environment
					.getExternalStorageDirectory().getAbsolutePath()
					+ "/cascade2.xml");
			if (!mCascadeFile.exists()) {
				mCascadeFile.createNewFile();
			} else {
				mCascadeFile.delete();
				mCascadeFile.createNewFile();
			}
			FileOutputStream os = new FileOutputStream(mCascadeFile);
			byte[] buffer = new byte[4096];
			int bytesRead;
			while ((bytesRead = is.read(buffer)) != -1) {
				os.write(buffer, 0, bytesRead);
			}
			is.close();
			os.close();
		} catch (IOException e) {
			e.printStackTrace();
		}

		try {
			// load cascade file from application resources
			InputStream is = getResources().openRawResource(
					R.raw.haarcascade_mcs_righteye);
			File mCascadeFile = new File(Environment
					.getExternalStorageDirectory().getAbsolutePath()
					+ "/cascade3.xml");
			if (!mCascadeFile.exists()) {
				mCascadeFile.createNewFile();
			} else {
				mCascadeFile.delete();
				mCascadeFile.createNewFile();
			}
			FileOutputStream os = new FileOutputStream(mCascadeFile);
			byte[] buffer = new byte[4096];
			int bytesRead;
			while ((bytesRead = is.read(buffer)) != -1) {
				os.write(buffer, 0, bytesRead);
			}
			is.close();
			os.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		try {
			// load cascade file from application resources
			InputStream is = getResources().openRawResource(
					R.raw.haarcascade_mcs_mouth);
			File mCascadeFile = new File(Environment
					.getExternalStorageDirectory().getAbsolutePath()
					+ "/cascade4.xml");
			if (!mCascadeFile.exists()) {
				mCascadeFile.createNewFile();
			} else {
				mCascadeFile.delete();
				mCascadeFile.createNewFile();
			}
			FileOutputStream os = new FileOutputStream(mCascadeFile);
			byte[] buffer = new byte[4096];
			int bytesRead;
			while ((bytesRead = is.read(buffer)) != -1) {
				os.write(buffer, 0, bytesRead);
			}
			is.close();
			os.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		CameraManager.getInstance().loadCascade(
				Environment.getExternalStorageDirectory().getAbsolutePath()
						+ "/cascade1.xml",
				Environment.getExternalStorageDirectory().getAbsolutePath()
						+ "/cascade2.xml",
				Environment.getExternalStorageDirectory().getAbsolutePath()
						+ "/cascade3.xml",
				Environment.getExternalStorageDirectory().getAbsolutePath()
						+ "/cascade4.xml");
	}

	


}
