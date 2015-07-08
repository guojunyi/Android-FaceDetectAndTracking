package com.smartcamera.core;

import android.graphics.RectF;
import android.hardware.Camera;
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
	
	public Face[] findFaces(byte[] datas,int width,int height,int angle){
		float[] faceDatas = nativeDetectFace(datas,width,height,angle);
		if(null!=faceDatas&&faceDatas.length>0){
			int count = faceDatas.length/4;
			Face[] faces = new Face[count];
			for(int i=0;i<count;i++){
				Face face = new Face();
				face.left = faceDatas[i*4+0];
				face.top = faceDatas[i*4+1];
				face.right = faceDatas[i*4+2];
				face.bottom = faceDatas[i*4+3];
				faces[i] = face;
			}
			return faces;
		}
		
		return null;
	}
	
	public Face[] trackingFace(byte[] datas,int width,int height,int angle){
		float[] faceDatas = nativeTrackingFace(datas,width,height,angle);
		if(null!=faceDatas&&faceDatas.length>0){
			int count = faceDatas.length/4;
			Face[] faces = new Face[count];
			for(int i=0;i<count;i++){
				Face face = new Face();
				face.left = faceDatas[i*4+0];
				face.top = faceDatas[i*4+1];
				face.right = faceDatas[i*4+2];
				face.bottom = faceDatas[i*4+3];
				faces[i] = face;
			}
			return faces;
		}
		
		return null;
	}
	
	public native int[] decodeYUV420(byte[] datas,int w,int h);
	
	
	public native int loadCascade(String cascade1,String cascade2,String cascade3,String cascade4);
	
	private native float[] nativeDetectFace(byte[] datas,int width,int height,int angle);
	
	private native float[] nativeTrackingFace(byte[] datas,int width,int height,int angle);
	
	public native int enableSingleTracking(int isEnable);
	
	public int trackFace(int[] datas){   
		RectF rect = new RectF(datas[0],datas[1],datas[0]+datas[2],datas[1]+datas[3]);
		if(null!=mTrackingCallback){
			mTrackingCallback.onCallback(rect, datas[4]);
		}
        return 0;    
    } 
	
	public int showHanming(double value){
		mTrackingCallback.onCallback(null, value);
		return 0;
	}
	
	private TrackingCallback mTrackingCallback;
	
	public void setTrackingCallback(TrackingCallback trackingCallback){
		mTrackingCallback = trackingCallback;
	}
	
	public interface TrackingCallback{
		
		public void onCallback(RectF rect,double clearFlag);
	}
}
