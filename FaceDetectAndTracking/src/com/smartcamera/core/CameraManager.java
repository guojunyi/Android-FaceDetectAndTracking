package com.smartcamera.core;

import java.util.List;

import android.graphics.ImageFormat;
import android.graphics.RectF;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.media.MediaPlayer;
import android.util.Log;

public class CameraManager {
	
	
	private static final String TAG = "CameraManager";
	private static CameraManager mCameraManager = null;
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
	
	public int trackFace(int[] datas){   
		RectF rect = new RectF(datas[0],datas[1],datas[0]+datas[2],datas[1]+datas[3]);
		if(null!=mTrackingCallback){
			mTrackingCallback.onCallback(rect, datas[4]);
		}
        return 0;    
    } 
	
	private TrackingCallback mTrackingCallback;
	
	public void setTrackingCallback(TrackingCallback trackingCallback){
		mTrackingCallback = trackingCallback;
	}
	
	public interface TrackingCallback{
		
		public void onCallback(RectF rect,int clearFlag);
	}
}
