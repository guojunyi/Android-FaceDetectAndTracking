package com.smartcamera.widget;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.ImageFormat;
import android.graphics.RectF;
import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.Size;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.AttributeSet;
import android.util.Log;
import android.view.OrientationEventListener;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.smartcamera.core.CameraManager;
import com.smartcamera.core.CameraManager.TrackingCallback;

public class CameraView extends SurfaceView implements SurfaceHolder.Callback,
		Camera.PreviewCallback,TrackingCallback {
	private static final String TAG = "CameraView";
	private Context mContext;
	private SurfaceHolder mHolder;
	
	private CameraFaceFrameView mCameraFaceFrameView;
	private Handler mHandler = new Handler(Looper.getMainLooper());
	
	private OrientationEventListener mOrientationEventListener;
	private int rotateAngle;
	
	private byte[] mDatas1;
	private byte[] mDatas2;
	private int index = 0;
	
	private Thread mThread;
	private Object syncObject = new Object();
	
	private boolean mStopThread;
	private boolean mCameraFrameReady;
	
	public CameraView(Context context) {
		// TODO Auto-generated constructor stub
		this(context, null);
	}

	public CameraView(Context context, AttributeSet attrs) {
		super(context, attrs);
		// TODO Auto-generated constructor stub
		mContext = context;
		init();
	}

	private void init() {
		mHolder = getHolder();
		mHolder.addCallback(this);
		mHolder.setKeepScreenOn(true);
		mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		CameraManager.getInstance().open(0);
		CameraManager.getInstance().setTrackingCallback(this);
		mOrientationEventListener = new OrientationEventListener(mContext) {

			@Override
			public void onOrientationChanged(int orientation) {
				// TODO Auto-generated method stub
				rotateAngle = orientation;
			}

		};
		
		mStopThread = false;
        mThread = new Thread(new CameraWorker());
        mThread.start();
	}

	public void setCameraFaceFrameView(CameraFaceFrameView cameraFaceFrameView) {
		mCameraFaceFrameView = cameraFaceFrameView;
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		// TODO Auto-generated method stub
		Log.e(TAG, "surfaceCreated");
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width,
			int height) {
		// TODO Auto-generated method stub
		Log.e(TAG, "surfaceChanged");
		mOrientationEventListener.enable();
		Camera camera = CameraManager.getInstance().getCamera();
		if (null == mHolder.getSurface() || null == camera) {
			return;
		}
		handleSurfaceChanged();
		try {
			camera.stopPreview();
			camera.setPreviewCallback(this);
			camera.setPreviewDisplay(holder);
			camera.startPreview();
		} catch (Exception e) {
			Log.e(TAG, "Error starting camera preview: " + e.getMessage());
		}
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		// TODO Auto-generated method stub
		try {
            mStopThread = true;
            Log.e(TAG, "Notify thread");
            synchronized (syncObject) {
            	syncObject.notify();
            }
            Log.e(TAG, "Wating for thread");
            if (mThread != null)
                mThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            mThread =  null;
        }
		mCameraFrameReady = false;
		mOrientationEventListener.disable();
		CameraManager.getInstance().closeCamera();

	}

	private AutoFocusCallback mAutoFocusCallback = new AutoFocusCallback() {

		@Override
		public void onAutoFocus(boolean success, Camera camera) {
			// TODO Auto-generated method stub
			if (success) {
				Log.e(TAG, "focus success");

			} else {
				Log.e(TAG, "focus failure");
			}
		}

	};

	public static class ResolutionComparator implements Comparator<Camera.Size> {
		@Override
		public int compare(Camera.Size size1, Camera.Size size2) {
			if (size1.height != size2.height)
				return size1.height - size2.height;
			else
				return size1.width - size2.width;
		}
	}

	private void handleSurfaceChanged() {
		try{
			Camera camera = CameraManager.getInstance().getCamera();
			Camera.Parameters parameters = camera.getParameters();

			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH && !android.os.Build.MODEL.equals("GT-I9100"))
				parameters.setRecordingHint(true);
			
			//set camera preview format
			parameters.setPreviewFormat(ImageFormat.NV21);

			
			//set camera preview size
			List<Camera.Size> resolutionList = parameters
					.getSupportedPreviewSizes();
			if (resolutionList != null && resolutionList.size() > 0) {
				Collections.sort(resolutionList, new ResolutionComparator());
				Camera.Size previewSize = null;
				int maxSize = 0;
				for (int i = 0; i < resolutionList.size(); i++) {
					Size size = resolutionList.get(i);
					
					if(size.width%160==0&&size.height%160==0){
						if(size.width*size.height>maxSize){
							maxSize = size.width*size.height;
							previewSize = size;
						}
					}
				}
				
				if(null!=previewSize){
					parameters.setPreviewSize(previewSize.width,previewSize.height);
				}

			}
			
			Log.e(TAG,parameters.getPreviewSize().width+":"+parameters.getPreviewSize().height);

			// set camera framerate
			parameters.setPreviewFrameRate(30);

			// set camera focus mode
			if (Build.VERSION.SDK_INT > Build.VERSION_CODES.FROYO) {
				List<String> focusModes = parameters.getSupportedFocusModes();
				if (focusModes != null) {
					if (((Build.MODEL.startsWith("GT-I950"))
							|| (Build.MODEL.endsWith("SCH-I959")) || (Build.MODEL
								.endsWith("MEIZU MX3")))
							&& focusModes
									.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE)) {

						parameters
								.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE);
					} else if (focusModes
							.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
						parameters
								.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
					} else
						parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_FIXED);
				}
			}
			
			camera.setDisplayOrientation(90);
			camera.setParameters(parameters);
		}catch(Exception e){
			e.printStackTrace();
		}
		
	}


	
	@Override
	public void onPreviewFrame(byte[] data, Camera camera) {
		// TODO Auto-generated method stub
		// the bitmap we want to fill with the image
		Camera.Parameters p = camera.getParameters();
		mCameraFaceFrameView.scaleFactorW = (float) p.getPreviewSize().height
				/ (float) getMeasuredWidth();
		mCameraFaceFrameView.scaleFactorH = (float) p.getPreviewSize().width / (float) getMeasuredHeight();
		
		synchronized (syncObject) {
			if(index==0){
				mDatas1 = data;
			}else if(index==1){
				mDatas2 = data;
			}
			
            mCameraFrameReady = true;
            syncObject.notify();
        }

	}

//	public int[] detectFace(byte[] datas) {
//		Camera camera = CameraManager.getInstance().getCamera();
//		Camera.Parameters p = camera.getParameters();
//		int[] rgb = CameraManager.getInstance().decodeYUV420(datas,
//				p.getPreviewSize().width, p.getPreviewSize().height);
//		Bitmap localBitmap1 = Bitmap.createBitmap(rgb,
//				p.getPreviewSize().width, p.getPreviewSize().height,
//				Config.RGB_565);
//
//		Matrix localMatrix = new Matrix();
//		localMatrix.setRotate(90.0F);
//		localMatrix.preScale(0.75f, 0.75f);
//		Bitmap localBitmap2 = Bitmap.createBitmap(localBitmap1, 0, 0,
//				localBitmap1.getWidth(), localBitmap1.getHeight(), localMatrix,
//				true);
//		int w = localBitmap2.getWidth();
//		int h = localBitmap2.getHeight();
//		// Log.e(TAG,w+":"+h);
//
//		// mOnDetectEndListener.onDetectEnd(localBitmap2);
//
//		int[] pix = new int[w * h];
//		localBitmap2.getPixels(pix, 0, w, 0, 0, w, h);
//		mCameraFaceFrameView.scaleFactorW = (float) localBitmap2.getWidth()
//				/ (float) getMeasuredWidth();
//		mCameraFaceFrameView.scaleFactorH = (float) localBitmap2.getHeight()
//				/ (float) getMeasuredHeight();
//		int[] faces = CameraManager.getInstance().detectFace(40, 40, pix, w, h);
//
//		return faces;
//	}
//
//	private void saveJPG(byte[] data) {
//		Camera camera = CameraManager.getInstance().getCamera();
//		Camera.Parameters p = camera.getParameters();
//
//		File file = new File(Environment.getExternalStorageDirectory()
//				.getAbsolutePath() + "/xxxx.jpg");
//
//		YuvImage localYuvImage = new YuvImage(data, 17,
//				p.getPreviewSize().width, p.getPreviewSize().height, null);
//		ByteArrayOutputStream bos = new ByteArrayOutputStream();
//		FileOutputStream outStream = null;
//
//		try {
//			if (!file.exists())
//				file.createNewFile();
//			localYuvImage.compressToJpeg(new Rect(0, 0,
//					p.getPreviewSize().width, p.getPreviewSize().height), 100,
//					bos);
//			Bitmap localBitmap1 = BitmapFactory.decodeByteArray(
//					bos.toByteArray(), 0, bos.toByteArray().length);
//			bos.close();
//			Matrix localMatrix = new Matrix();
//			localMatrix.setRotate(90.0F + rotateAngle);
//			Bitmap localBitmap2 = Bitmap.createBitmap(localBitmap1, 0, 0,
//					localBitmap1.getWidth(), localBitmap1.getHeight(),
//					localMatrix, true);
//			int w = localBitmap2.getWidth();
//			int h = localBitmap2.getHeight();
//			Log.e(TAG, w + ":" + h);
//
//			ByteArrayOutputStream bos2 = new ByteArrayOutputStream();
//			localBitmap2.compress(Bitmap.CompressFormat.JPEG, 100, bos2);
//			mCameraFaceFrameView.scaleFactorW = (float) localBitmap2.getWidth()
//					/ (float) getMeasuredWidth();
//			mCameraFaceFrameView.scaleFactorH = (float) localBitmap2
//					.getHeight() / (float) getMeasuredHeight();
//			outStream = new FileOutputStream(file);
//			outStream.write(bos2.toByteArray());
//			outStream.close();
//			localBitmap1.recycle();
//			localBitmap2.recycle();
//
//		} catch (FileNotFoundException e) {
//			e.printStackTrace();
//		} catch (IOException e) {
//			e.printStackTrace();
//		}
//	}

	private OnDetectEndListener mOnDetectEndListener;

	public interface OnDetectEndListener {
		public void onDetectEnd(Bitmap bitmap);
	}

	public void setOnDetectEndListener(OnDetectEndListener listener) {
		mOnDetectEndListener = listener;
	}
	
	long time;
	private class CameraWorker implements Runnable {

        @Override
        public void run() {
            do {
                synchronized (syncObject) {
                    try {
                        while (!mCameraFrameReady && !mStopThread) {
                        	syncObject.wait();
                        }
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    
                    index++;
                    if(index>1){
                    	index = 0;
                    }
                }
                
                if (!mStopThread && mCameraFrameReady) {
                    mCameraFrameReady = false;
                    
                    Camera camera = CameraManager.getInstance().getCamera();
        			Camera.Parameters p = camera.getParameters();
        			
        			byte[] imageBytes = index==0?mDatas2:mDatas1;
        			
        			
        			int[] faces = CameraManager.getInstance().detectFaceX(imageBytes, p.getPreviewSize().width, p.getPreviewSize().height,-90);
        			

        			if (faces != null && faces.length != 0) {
        				mHandler.post(new Runnable() {

            				@Override
            				public void run() {
            					// TODO Auto-generated method stub
            					mCameraFaceFrameView.clearFaceFrame();
            				}

            			});
        				int count = faces[faces.length - 1];
        				for (int i = 0; i < count; i++) {
        					int flag = faces[5 * i + 4];
        					if (flag == 1) {
        						final RectF rect = new RectF(faces[5 * i + 0],
        								faces[5 * i + 1], faces[5 * i + 0]
        										+ faces[5 * i + 2], faces[5 * i + 1]
        										+ faces[5 * i + 3]);
        						
        						mHandler.post(new Runnable() {

        							@Override
        							public void run() {
        								// TODO Auto-generated method stub
        								mCameraFaceFrameView.drawFaceFrame(rect);
        							}

        						});
        					}
        				}

        			}
        			
        			Log.e(TAG,"Detect Time Interval:"+(System.currentTimeMillis()-time));
        			time = System.currentTimeMillis();
                }
            } while (!mStopThread);
            Log.e(TAG, "Finish processing thread");
        }
    }

	@Override
	public void onCallback(final RectF rect, int clearFlag) {
		// TODO Auto-generated method stub
		if(clearFlag==1){
			mHandler.post(new Runnable() {

				@Override
				public void run() {
					// TODO Auto-generated method stub
					mCameraFaceFrameView.clearFaceFrame();
				}

			});
		}
		
		mHandler.post(new Runnable() {

			@Override
			public void run() {
				// TODO Auto-generated method stub
				mCameraFaceFrameView.drawFaceFrame(rect);
			}

		});
	}
}
