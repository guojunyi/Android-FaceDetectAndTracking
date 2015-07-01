package com.smartcamera.widget;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.RectF;
import android.os.Handler;
import android.os.Looper;
import android.util.AttributeSet;
import android.view.View;

import com.smartcamera.utils.Utils;

public class CameraFaceFrameView extends View {
	Paint mPaint;

	List<Item> faces = new ArrayList<Item>();
	float animateMargin = 30;
	Handler mHandler = new Handler(Looper.getMainLooper());
	public float scaleFactorW;
	public float scaleFactorH;
	public CameraFaceFrameView(Context context, AttributeSet attrs) {
		super(context, attrs);
		// TODO Auto-generated constructor stub
		mPaint = new Paint();
		mPaint.setStyle(Style.STROKE);
		mPaint.setAntiAlias(true);
		mPaint.setStrokeWidth(Utils.dp2px(context, 1));
		mPaint.setColor(0xff5cb7f0);
		
		animateMargin = Utils.dp2px(context, (int)animateMargin);
		scaleFactorW = 1.0f;
		scaleFactorH = 1.0f;
	}

	@Override
	public void onDraw(Canvas canvas) {
		for (Item item : faces) {
			
			int left = (int) (item.rect.left/scaleFactorW);
			int top = (int)  (item.rect.top/scaleFactorH);
			int right = (int)  (item.rect.right/scaleFactorW);
			int bottom = (int)  (item.rect.bottom/scaleFactorH);
		
			canvas.drawRoundRect(new RectF(left,top,right,bottom), Utils.dp2px(getContext(), 5), Utils.dp2px(getContext(), 5), mPaint);
			
		}
	}

	public void drawFaceFrame(RectF rect) {
		faces.add(new Item(rect));
		invalidate();
	}

	public void clearFaceFrame() {
		faces.clear();
		invalidate();
	}

	private class Item {
		public Item(RectF rect) {
			this.rect = rect;
			progress = 1.0f;

		}
		RectF rect;
		float progress;
	}
}
