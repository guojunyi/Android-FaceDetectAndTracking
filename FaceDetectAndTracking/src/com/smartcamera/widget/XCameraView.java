package com.smartcamera.widget;
import java.util.Collections;
import java.util.List;

import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame;
import org.opencv.android.Utils;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.imgproc.Imgproc;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.os.Build;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.smartcamera.core.CameraManager;
import com.smartcamera.widget.CameraView.ResolutionComparator;


public class XCameraView extends SurfaceView implements PreviewCallback,SurfaceHolder.Callback{
	public static final String TAG = "XCameraView";
	
	private static final int MAGIC_TEXTURE_ID = 10;
	
	private Bitmap mCacheBitmap;
	private byte mBuffer[];
	private int mFrameWidth,mFrameHeight;
	private Mat[] mFrameChain;
	protected JavaCameraFrame[] mCameraFrame;
	private SurfaceTexture mSurfaceTexture;
	private int mChainIdx = 0;
	private boolean mStopThread;
	private boolean mCameraFrameReady;
	private Thread mThread;
	private Object syncObject = new Object();
	
	public XCameraView(Context context, AttributeSet attrs) {
		super(context, attrs);
		// TODO Auto-generated constructor stub
	}

	private void initCamera(){
		getHolder().addCallback(this);
		Camera camera = CameraManager.getInstance().open(0);
		
		try {
			Camera.Parameters params = camera.getParameters();
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH && !android.os.Build.MODEL.equals("GT-I9100"))
                params.setRecordingHint(true);
			
			//set camera preview size
			List<Camera.Size> resolutionList = params
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
					params.setPreviewSize(previewSize.width,previewSize.height);
				}

			}
			
			Log.e(TAG,params.getPreviewSize().width+":"+params.getPreviewSize().height);
			
			//set camera preview format
			params.setPreviewFormat(ImageFormat.NV21);

			//set camera focus mode
			if (Build.VERSION.SDK_INT > Build.VERSION_CODES.FROYO) {
				List<String> focusModes = params.getSupportedFocusModes();
				if (focusModes != null) {
					Log.e("video", Build.MODEL);
					if (((Build.MODEL.startsWith("GT-I950"))
							|| (Build.MODEL.endsWith("SCH-I959")) || (Build.MODEL
								.endsWith("MEIZU MX3")))
							&& focusModes
									.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE)) {

						params
								.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE);
					} else if (focusModes
							.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
						params
								.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
					} else
						params.setFocusMode(Camera.Parameters.FOCUS_MODE_FIXED);
				}
			}
			
			camera.setParameters(params);

			mFrameWidth = params.getPreviewSize().width;
            mFrameHeight = params.getPreviewSize().height;

            int size = mFrameWidth * mFrameHeight;
            size  = size * ImageFormat.getBitsPerPixel(params.getPreviewFormat()) / 8;
            mBuffer = new byte[size];
            camera.addCallbackBuffer(mBuffer);
            camera.setPreviewCallbackWithBuffer(this);

            mFrameChain = new Mat[2];
            mFrameChain[0] = new Mat(mFrameHeight + (mFrameHeight/2), mFrameWidth, CvType.CV_8UC1);
            mFrameChain[1] = new Mat(mFrameHeight + (mFrameHeight/2), mFrameWidth, CvType.CV_8UC1);
            mCacheBitmap = Bitmap.createBitmap(mFrameWidth, mFrameHeight, Bitmap.Config.ARGB_8888);

            mCameraFrame = new JavaCameraFrame[2];
            mCameraFrame[0] = new JavaCameraFrame(mFrameChain[0], mFrameWidth, mFrameHeight);
            mCameraFrame[1] = new JavaCameraFrame(mFrameChain[1], mFrameWidth, mFrameHeight);

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            	Log.e(TAG, "startPreview");
            	mSurfaceTexture = new SurfaceTexture(MAGIC_TEXTURE_ID);
            	camera.setPreviewTexture(mSurfaceTexture);
            } else
            	camera.setPreviewDisplay(null);
                
            Log.e(TAG, "startPreview");
            camera.startPreview();
        } catch (Exception e) {
            e.printStackTrace();
        }
	}
	
	
	public boolean connectCamera() {
        Log.e(TAG, "Connecting to camera");
        initCamera();

        mCameraFrameReady = false;

        Log.e(TAG, "Starting processing thread");
        mStopThread = false;
        mThread = new Thread(new CameraWorker());
        mThread.start();

        return true;
    }
	
	public void disconnectCamera() {
        Log.e(TAG, "Disconnecting from camera");
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

        /* Now release camera */
        CameraManager.getInstance().closeCamera();
        
        synchronized (syncObject) {
            if (mFrameChain != null) {
                mFrameChain[0].release();
                mFrameChain[1].release();
            }
            if (mCameraFrame != null) {
                mCameraFrame[0].release();
                mCameraFrame[1].release();
            }
        }
        
        mCameraFrameReady = false;
        
        if (mCacheBitmap != null) {
            mCacheBitmap.recycle();
        }
        
    }
	
	@Override
	public void onPreviewFrame(byte[] data, Camera camera) {
		// TODO Auto-generated method stub
		//Log.e(TAG, "Preview Frame received. Frame size: " + data.length);
		synchronized (syncObject) {
            mFrameChain[mChainIdx].put(0, 0, data);
            mCameraFrameReady = true;
            syncObject.notify();
        }
		
        if (camera != null)
        	camera.addCallbackBuffer(mBuffer);
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width,
			int height) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		// TODO Auto-generated method stub
		
	}
	
	private class JavaCameraFrame implements CvCameraViewFrame {
        @Override
        public Mat gray() {
            return mYuvFrameData.submat(0, mHeight, 0, mWidth);
        }

        @Override
        public Mat rgba() {
            Imgproc.cvtColor(mYuvFrameData, mRgba, Imgproc.COLOR_YUV2RGBA_NV21, 4);
            return mRgba;
        }

        public JavaCameraFrame(Mat Yuv420sp, int width, int height) {
            super();
            mWidth = width;
            mHeight = height;
            mYuvFrameData = Yuv420sp;
            mRgba = new Mat();
        }

        public void release() {
            mRgba.release();
        }

        private Mat mYuvFrameData;
        private Mat mRgba;
        private int mWidth;
        private int mHeight;
    };

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
                    if (mCameraFrameReady)
                        mChainIdx = 1 - mChainIdx;
                }

                if (!mStopThread && mCameraFrameReady) {
                    mCameraFrameReady = false;
                    if (!mFrameChain[1 - mChainIdx].empty()){
                    	deliverAndDrawFrame(mCameraFrame[1 - mChainIdx]);
                    }
                }
            } while (!mStopThread);
            Log.e(TAG, "Finish processing thread");
        }
    }
    
    long time;
    private void deliverAndDrawFrame(CvCameraViewFrame frame) {
        Mat modified;

        if (mListener != null) {
            modified = mListener.onCameraFrame(frame);
        } else {
            modified = frame.rgba();
        }

        boolean bmpValid = true;
        if (modified != null) {
            try {
                Utils.matToBitmap(modified, mCacheBitmap);
            } catch(Exception e) {
                Log.e(TAG, "Mat type: " + modified);
                Log.e(TAG, "Bitmap type: " + mCacheBitmap.getWidth() + "*" + mCacheBitmap.getHeight());
                Log.e(TAG, "Utils.matToBitmap() throws an exception: " + e.getMessage());
                bmpValid = false;
            }
        }

        if (bmpValid && mCacheBitmap != null) {
            Canvas canvas = getHolder().lockCanvas();
            if (canvas != null) {
                canvas.drawColor(0, android.graphics.PorterDuff.Mode.CLEAR);
//                if (mScale != 0) {
//                    canvas.drawBitmap(mCacheBitmap, new Rect(0,0,mCacheBitmap.getWidth(), mCacheBitmap.getHeight()),
//                         new Rect((int)((canvas.getWidth() - mScale*mCacheBitmap.getWidth()) / 2),
//                         (int)((canvas.getHeight() - mScale*mCacheBitmap.getHeight()) / 2),
//                         (int)((canvas.getWidth() - mScale*mCacheBitmap.getWidth()) / 2 + mScale*mCacheBitmap.getWidth()),
//                         (int)((canvas.getHeight() - mScale*mCacheBitmap.getHeight()) / 2 + mScale*mCacheBitmap.getHeight())), null);
//                } else {
//                     
//                }
                
                canvas.drawBitmap(mCacheBitmap, new Rect(0,0,mCacheBitmap.getWidth(), mCacheBitmap.getHeight()),
                        new Rect(0,
                        0,
                        canvas.getWidth(),
                        canvas.getHeight()), null);
                
//                if (mFpsMeter != null) {
//                    mFpsMeter.measure();
//                    mFpsMeter.draw(canvas, 20, 30);
//                }
                getHolder().unlockCanvasAndPost(canvas);
            }
        }
        
        Log.e(TAG,(System.currentTimeMillis()-time)+"");
        time = System.currentTimeMillis();
    }
    
    private CvCameraViewListener2 mListener;
    
    public void setCvCameraViewListener(CvCameraViewListener2 listener) {
        mListener = listener;
    }
    
    public interface CvCameraViewListener2 {
        public Mat onCameraFrame(CvCameraViewFrame inputFrame);
    };
}
