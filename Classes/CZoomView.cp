// CZoomView.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// a view that supports various levels of magnification
//
// written:  3/7/96, Ed Zavada
// modified: 10/12/97, ERZ Added CMouseTracker support

#include "CZoomView.h"
#include <LStream.h>
#include <LArrayIterator.h>

#if ZOOM_VIEW_TRACKING
  #include "CMouseTracker.h"
#endif

#define AUTOSCROLL_BOUNDRY_PIXELS	5
#define MAX_AUTOSCROLL_SPEED		20



// ---------------------------------------------------------------------------
//		¥ CZoomView()
// ---------------------------------------------------------------------------
//	Default Constructor

CZoomView::CZoomView()
: LView() {
	InitZoomView();
}


// ---------------------------------------------------------------------------
//		¥ CZoomView(SPaneInfo&, SViewInfo&)
// ---------------------------------------------------------------------------
//	Construct ZoomView from input parameters

CZoomView::CZoomView(const SPaneInfo &inPaneInfo, const SViewInfo &inViewInfo)
: LView(inPaneInfo, inViewInfo) {
	InitZoomView();
}


// ---------------------------------------------------------------------------
//		¥ CZoomView(LStream*)
// ---------------------------------------------------------------------------
//	Construct View from data in a Stream

CZoomView::CZoomView(LStream *inStream)
:LView(inStream) {
	InitZoomView();
}


// ---------------------------------------------------------------------------
//		¥ ~CZoomView
// ---------------------------------------------------------------------------
//	Destructor
//

CZoomView::~CZoomView() {
	if (mCurrTracker) {
		delete mCurrTracker;	// can't possible keep using it
	}
}


// ---------------------------------------------------------------------------
//		¥ InitZoomView
// ---------------------------------------------------------------------------
//	Private Initializer

void
CZoomView::InitZoomView() {
	mZoomLevel = 0x0100;	// 1:1 magnification
	mImageSizeNorm = mImageSize;			// SDimension32	
	mImageLocationNorm = mImageLocation;	// SPoint32
	mCurrTracker = 0;		// no CMouseTrackers are operating on us yet
}



// ---------------------------------------------------------------------------
//		¥ SavePlace
// ---------------------------------------------------------------------------
//	Write size and location information to a Stream for later retrieval
//	by the RestorePlace() function

void
CZoomView::SavePlace(LStream *outPlace) {
	outPlace->WriteData(&mZoomLevel, sizeof(short));			// save the zoom info
	outPlace->WriteData(&mImageSizeNorm, sizeof(SDimension32));
	outPlace->WriteData(&mImageLocationNorm, sizeof(SPoint32));
	LView::SavePlace(outPlace);			// Save info for this View
}



// ---------------------------------------------------------------------------
//		¥ RestorePlace
// ---------------------------------------------------------------------------
//	Read size and location information stored in a Stream by the
//	SavePlace() function

void
CZoomView::RestorePlace(LStream *inPlace) {
	*inPlace >> mZoomLevel;
	*inPlace >> mImageSizeNorm.width;
	*inPlace >> mImageSizeNorm.height;
	*inPlace >> mImageLocationNorm.h;
	*inPlace >> mImageLocationNorm.v;
	LView::RestorePlace(inPlace);		// Restore info for this View
}



// ---------------------------------------------------------------------------
//		¥ ScrollImageBy
// ---------------------------------------------------------------------------
//  Does whatever LView::ScrollImageBy() does, but does additional maintanence
//  to update the unmagnified position

void
CZoomView::ScrollImageBy(SInt32 inLeftDelta, SInt32 inTopDelta, Boolean inRefresh) {
	LView::ScrollImageBy(inLeftDelta, inTopDelta, inRefresh);	// do the work
	CalcImageLocationNorm();
}


void
CZoomView::ScrollToSubPane(LPane *inPane) {
	Rect r;
	Point pt;
	SPoint32 p;
	inPane->CalcPortFrameRect(r);
	long height = r.bottom - r.top;
	pt.h = r.left;
	pt.v = r.top;
	PortToLocalPoint(pt);
	LocalToImagePoint(pt, p);
	if ( ! ImagePointIsInFrame(p.h, p.v) ) {
		ScrollImageTo(p.h, p.v, true);
	}
}


void
CZoomView::AutoScrollDragging(SPoint32 inMouseImagePt, Point &ioTrackingPt) {
	SPoint32 framePos;
	SInt32 vDelta = 0, hDelta = 0;
	framePos.h = inMouseImagePt.h + mImageLocation.h - mFrameLocation.h; // get frame relative
	framePos.v = inMouseImagePt.v + mImageLocation.v - mFrameLocation.v; // coordinates
	if (framePos.h < AUTOSCROLL_BOUNDRY_PIXELS) {
		hDelta = framePos.h - AUTOSCROLL_BOUNDRY_PIXELS;
	} else if (framePos.h > (mFrameSize.width - AUTOSCROLL_BOUNDRY_PIXELS) ) {
		hDelta = framePos.h - (mFrameSize.width - AUTOSCROLL_BOUNDRY_PIXELS);
	}
	if (framePos.v < AUTOSCROLL_BOUNDRY_PIXELS) {
		vDelta = framePos.v - AUTOSCROLL_BOUNDRY_PIXELS;
	} else if (framePos.v > (mFrameSize.height - AUTOSCROLL_BOUNDRY_PIXELS) ) {
		vDelta = framePos.v - (mFrameSize.height - AUTOSCROLL_BOUNDRY_PIXELS);
	}
	if (vDelta || hDelta) {
		if (hDelta > MAX_AUTOSCROLL_SPEED)	// limit speed of autoscroll, 1.2d8
			hDelta = MAX_AUTOSCROLL_SPEED;
		if (hDelta < -MAX_AUTOSCROLL_SPEED)
			hDelta = -MAX_AUTOSCROLL_SPEED;
		if (vDelta > MAX_AUTOSCROLL_SPEED)
			vDelta = MAX_AUTOSCROLL_SPEED;
		if (vDelta < -MAX_AUTOSCROLL_SPEED)
			vDelta = -MAX_AUTOSCROLL_SPEED;
		hDelta *= 2;
		vDelta *= 2;
		ScrollImageBy(hDelta, vDelta, true);
		ioTrackingPt.h += hDelta;
		ioTrackingPt.v += vDelta;
	}
}

// ---------------------------------------------------------------------------
//		¥ ResizeImageBy
// ---------------------------------------------------------------------------
//	Change the Image size by the specified pixel increments

void
CZoomView::ResizeImageBy(SInt32 inWidthDelta, SInt32 inHeightDelta, Boolean inRefresh) {
	LView::ResizeImageBy(inWidthDelta, inHeightDelta, inRefresh);
	CalcImageSizeNorm();
}



// ---------------------------------------------------------------------------
//		¥ CalcImageLocationNorm
// ---------------------------------------------------------------------------
//	Change the normal image location in response to a change in the zoomed image location

void
CZoomView::CalcImageLocationNorm() {
	float a;
	float zoom = GetZoom();
	a = (float) mImageLocation.h / zoom;	// calc the 1:1 location based on the zoom level
	mImageLocationNorm.h = a;
	a = (float) mImageLocation.v / zoom;
	mImageLocationNorm.v = a;
}



// ---------------------------------------------------------------------------
//		¥ CalcImageSizeNorm
// ---------------------------------------------------------------------------
//	Change the normal image size in response to a change in the zoomed image size

void
CZoomView::CalcImageSizeNorm() {
	float a;
	float zoom = GetZoom();
	a = (float) mImageSize.width / zoom;	// calc the 1:1 size based on the zoom level
	mImageSizeNorm.width = a;
	a = (float) mImageSize.height / zoom;
	mImageSizeNorm.height = a;
}
	


// ---------------------------------------------------------------------------
//		¥ ZoomIn
// ---------------------------------------------------------------------------

void
CZoomView::ZoomIn() {
	float zoom = GetZoom() * 1.25;
	if (zoom > 256) {
		zoom = 256;	// max zoom
	}
	ZoomTo( zoom );
}



// ---------------------------------------------------------------------------
//		¥ ZoomOut
// ---------------------------------------------------------------------------

void
CZoomView::ZoomOut() {
	const float minZoom = 0.015625;	// 1/64th
	float zoom = GetZoom() * 0.8;
	if (zoom < minZoom ) {
		zoom = minZoom;	// min zoom
	}
	ZoomTo( zoom );
}

// ---------------------------------------------------------------------------
//		¥ ZoomFill
// ---------------------------------------------------------------------------
//	Zooms to optimally fit the entire image within its frame

void
CZoomView::ZoomFill() {
	const float minZoom = (float)1/(float)256;
	float zoomW = (float)mFrameSize.width / (float)mImageSizeNorm.width; // exact fit for width
	float zoomH = (float)mFrameSize.height / (float)mImageSizeNorm.height;	// & for height
	float zoom;
	if (zoomW < zoomH)	// find the smaller of the width & height zooms
		zoom = zoomW;
	else
		zoom = zoomH;
	if (zoom < minZoom )
		zoom = minZoom;	// min zoom
	ZoomTo( zoom );
	ScrollImageTo(0, 0, true);
}


// ---------------------------------------------------------------------------
//		¥ ZoomTo
// ---------------------------------------------------------------------------
//	Adjust the image size and location to a new magnification
//
//  Expects a magnification as an input parameter.
//  ie: inZoom = 2.0 is 2 times magnification (2:1 scale), 
//      inZoom = 0.25 is 1/4 magnification (1:4 scale)
//  256:1 is max zoom
//  1:256 is min zoom

void
CZoomView::ZoomTo(float inZoom, Boolean inRefresh) {
	UInt16 sZoom = inZoom * 256;
	if (sZoom != mZoomLevel) {	// only do this if the actual magnification is changing
	  #if ZOOM_VIEW_TRACKING
		if (mCurrTracker) {				// don't run risk of CMouseTracker being caught
			mCurrTracker->Hide();		// unawares and leaving old info drawn in the view
	  	}
	  #endif
		float deltaZoom = GetZoom();	// get old zoom as float
		mZoomLevel = sZoom;
		inZoom = GetZoom();
		deltaZoom = inZoom/deltaZoom;
		SDimension32 oldImageSize = mImageSize;
		float a;
		a = (float) mImageSizeNorm.width * inZoom;	// calc the zoomed size based on
		mImageSize.width = a;						// the normal, 1:1 size
		a = (float) mImageSizeNorm.height * inZoom;
		mImageSize.height = a;
		// calc image point that is in center of frame, then maintain that point
		// in the center after zooming
		SPoint32 p;
		GetScrollPosition(p);
		if (mImageSize.height <= mFrameSize.height) {
			p.v = oldImageSize.height/2 - mFrameSize.height/2;	// center point of image
		}
		if (mImageSize.width <= mFrameSize.width) {
			p.h = oldImageSize.width/2 - mFrameSize.width/2;
		}
		a = (float) (p.h + mFrameSize.width/2) * deltaZoom;	// current center point
		p.h = a;
		a = (float) (p.v + mFrameSize.height/2) * deltaZoom;// in image coord
		p.v = a;
		CalcRevealedRect();
		ScrollImageTo(p.h - mFrameSize.width/2, p.v - mFrameSize.height/2, false);
		ZoomAllSubPanes(deltaZoom);					// adjust the subpanes' positions
		if (inRefresh)
			Refresh();
	  #if ZOOM_VIEW_TRACKING
		if (mCurrTracker) {
			mCurrTracker->AdjustZoomed();
			mCurrTracker->Show();
		}
	  #endif
	}
}


void
CZoomView::ZoomAllSubPanes(float deltaZoom) {
	LArrayIterator iterator(mSubPanes, LArrayIterator::from_Start);
	LPane	*theSub;
	while (iterator.Next(&theSub)) {
		ZoomSubPane(theSub, deltaZoom);
	}
}

// This method adjusts the position and size of an item that is within the view
// when we zoom. As implemented, it has the serious drawback that the zoomed in position
// can never be more precise than the lowest resolution that the user zooms to.
// ie: if I position something while at 1:1 zoom, I have 1 pixel accuracy in my placement,
// but as soon as I zoom out to 1:4, all my positions are shifted PERMANENTLY to the nearest
// 4th pixel, so when I zoom back in to 1:1, all my positions have changed.
// There is no reasonable way around this other than having the individual items track their
// position with some additional, more precise coordinate system (inches, mm, lightyears, 
// etc...) and then overriding each this ZoomSubPane() method to calculate the pixel position
// based of each item based on that other coordinate system.
void
CZoomView::ZoomSubPane(LPane *inSub, float deltaZoom) {
	float a;
	SPoint32 p;
	SPoint32 superImageLoc = {0, 0};
	GetImageLocation(superImageLoc);
	
	inSub->GetFrameLocation(p);
	p.h -= superImageLoc.h;
	p.v -= superImageLoc.v;	// this gives offset from top left corner of image
	
	a = (float)p.h * deltaZoom;
	p.h = a;
	a = (float)p.v * deltaZoom;
	p.v = a;
	inSub->PlaceInSuperImageAt(p.h, p.v, false);	// don't refresh
}

