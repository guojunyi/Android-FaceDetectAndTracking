package com.smartcamera.widget;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.RectF;
import android.os.Handler;
import android.os.Looper;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

import com.smartcamera.core.Face;
import com.smartcamera.utils.Utils;

public class CameraFaceFrameView extends View {
	Paint mPaint;

	Face[] faces;
	float animateMargin = 30;
	Handler mHandler = new Handler(Looper.getMainLooper());
	boolean clearFlag;
	public CameraFaceFrameView(Context context, AttributeSet attrs) {
		super(context, attrs);
		// TODO Auto-generated constructor stub
		mPaint = new Paint();
		mPaint.setStyle(Style.STROKE);
		mPaint.setAntiAlias(true);
		mPaint.setStrokeWidth(Utils.dp2px(context, 2));
		mPaint.setColor(0xff5cb7f0);
		
		animateMargin = Utils.dp2px(context, (int)animateMargin);
	}

	@Override
	public void onDraw(Canvas canvas) {
		if(null!=faces&&faces.length>0){
			for (Face face : faces) {
				int left = (int) (face.left*getWidth());
				int top = (int)  (face.top*getHeight());
				int right = (int)  (face.right*getWidth());
				int bottom = (int)  (face.bottom*getHeight());
				canvas.drawRoundRect(new RectF(left,top,right,bottom), Utils.dp2px(getContext(), 5), Utils.dp2px(getContext(), 5), mPaint);
			}
		}
	}

	public void drawFaceFrames(Face[] faces) {
		this.faces = faces;
		invalidate();
	}

}
