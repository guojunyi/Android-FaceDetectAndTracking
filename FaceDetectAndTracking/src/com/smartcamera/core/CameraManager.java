package com.smartcamera.core;

import java.util.List;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.media.MediaPlayer;
import android.util.Log;

public class CameraManager {
	
	
	private static final String TAG = "CameraManager";
	private static CameraManager mCameraManager = null;
	private MediaPlayer player = null;
	public static CameraManager getInstance() {
		
		if (null == mCameraManager) {
			synchronized (CameraManager.class) {
				if (null == mCameraManager) {
					mCameraManager = new CameraManager();
				}
			}
		}
		return mCameraManager;
	}
	
	
	private Camera mCamera;
	private int currentCameraId;
	//private PreviewCallback mPreviewCallback;
	public Camera open(int cameraId){
		if (null != mCamera) {
			return mCamera;
		}

		try {
			mCamera = Camera.open(cameraId);
			currentCameraId = cameraId;
			//mCamera.setDisplayOrientation(90);
			//setParams();
		} catch (Exception e) {
			Log.e(TAG, "open camera error");
			e.printStackTrace();
			if(null!=mCamera){
				mCamera.release();
				mCamera = null;
			}
		}
		return mCamera;
	}
	
	private void setParams(){
		if(mCamera!=null){
			Camera.Parameters p = mCamera.getParameters();
			List<Size> supportedPreviewSizes = p.getSupportedPreviewSizes();
			int maxSize = 0;
			Size maxPreviewSize = null;
			for (Size size : supportedPreviewSizes) {
				if (maxSize < (size.width * size.height)) {
					maxSize = size.width * size.height;
					maxPreviewSize = size;
				}
			}
			p.setPreviewSize(maxPreviewSize.width, maxPreviewSize.height);
			
			List<Size> supportedPictureSizes = p.getSupportedPictureSizes();
			maxSize = 0;
			Size maxPictureSize = null;
			for (Size size : supportedPictureSizes) {
				if (maxSize < (size.width * size.height)) {
					maxSize = size.width * size.height;
					maxPictureSize = size;
				}
			}
			p.setPreviewFrameRate(3);
			p.setPictureSize(maxPictureSize.width, maxPictureSize.height);
			p.setPreviewFormat(ImageFormat.NV21);
			mCamera.setParameters(p);
		}
	}
	
	public Camera getCamera(){
		return mCamera;
	}
	
	
	public void closeCamera(){
		try{
			synchronized(CameraManager.this){
				if (mCamera != null) {
					mCamera.stopPreview();
					mCamera.setPreviewCallback(null);
					mCamera.release(); // release the camera for other applications
					mCamera = null;
				}
			}
		}catch(Exception e){
			e.printStackTrace();
		}
		
	}
	
	
//	public interface PreviewCallback{
//		public void onPreviewFrame(byte[] data, Camera camera);
//	}
//	
//	public void setPreviewCallback(PreviewCallback callback){
//		this.mPreviewCallback = callback;
//	}
	
	public int getCurrentCameraId(){
		return this.currentCameraId;
	}
	
	public native int[] decodeYUV420(byte[] datas,int w,int h);
	
	public native int[] grayImage(int[] buf,int w,int h); 
	
	public native int loadCascade(String cascade1,String cascade2,String cascade3,String cascade4);
	
	public native int[] detectFaceX(byte[] datas,int width,int height,int angle);
	
	public native int enableSingleTracking(int isEnable);
}
