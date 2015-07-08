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
import android.widget.ImageView;
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
	ImageView imageView;
	Handler mHandler = new Handler(Looper.getMainLooper());

	public TextView textView;
	CheckBox checkBox;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		setContentView(R.layout.activity_main);

		cameraView = (CameraView) findViewById(R.id.cameraView);
		faceFrameView = (CameraFaceFrameView) findViewById(R.id.faceFrameView);
		imageView = (ImageView) findViewById(R.id.imageView);
		cameraView.setCameraFaceFrameView(faceFrameView);

		cameraView.setOnDetectEndListener(new OnDetectEndListener() {

			@Override
			public void onDetectEnd(final Bitmap bitmap) {
				// TODO Auto-generated method stub
				mHandler.post(new Runnable() {

					@Override
					public void run() {
						// TODO Auto-generated method stub
						imageView.setImageBitmap(bitmap);
					}

				});

			}

		});
		initOpencv();

		
		checkBox = (CheckBox) findViewById(R.id.checkBox);
		checkBox.setOnClickListener(new OnClickListener(){

			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				if(checkBox.isChecked()){
					checkBox.setChecked(true);
					CameraManager.getInstance().enableSingleTracking(1);
				}else{
					checkBox.setChecked(false);
					CameraManager.getInstance().enableSingleTracking(0);
				}
			}
			
		});
		
		textView = (TextView) findViewById(R.id.textView);
		
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
