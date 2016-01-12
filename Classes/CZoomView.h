// CZoomView.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// a view that supports various levels of magnification
//
// written:  3/7/96, Ed Zavada
// modified: 10/12/97, ERZ Added CMouseTracker support

#ifndef CZOOMVIEW_H_INCLUDED
#define CZOOMVIEW_H_INCLUDED

#include "LView.h"

#define ZOOM_VIEW_TRACKING 1		// set to zero to disable CMouseTracker support

#if ZOOM_VIEW_TRACKING
  class CMouseTracker;
#endif

class CZoomView : public LView {
public:
	enum { class_ID = 'Zoom' };

					CZoomView();
					CZoomView(const SPaneInfo &inPaneInfo,
							  const SViewInfo &inViewInfo);
					CZoomView(LStream *inStream);
	virtual			~CZoomView();
	
	virtual void	SavePlace(LStream *outPlace);
	virtual void	RestorePlace(LStream *inPlace);

	virtual void	ScrollImageBy(SInt32 inLeftDelta, SInt32 inTopDelta,
									Boolean inRefresh);
	virtual void	ResizeImageBy(SInt32 inWidthDelta, SInt32 inHeightDelta,
									Boolean inRefresh);
	
	void			ScrollToSubPane(LPane *inPane);
	void			AutoScrollDragging(SPoint32 inMouseImagePt, Point &ioTrackingPt);
	void			CalcImageLocationNorm();
	void			CalcImageSizeNorm();
	
	float			GetZoom() {return (float)mZoomLevel/(float)256;}
	void			ZoomIn();
	void			ZoomOut();
	void			ZoomFill();
	virtual void	ZoomTo(float inZoom, Boolean inRefresh = true);
	
	void			ZoomAllSubPanes(float deltaZoom);
	virtual void	ZoomSubPane(LPane *inSub, float deltaZoom);
  #if ZOOM_VIEW_TRACKING
	CMouseTracker*	GetMouseTracker() {return mCurrTracker;}
	void			SetMouseTracker(CMouseTracker* inTracker) {mCurrTracker = inTracker;}
  #endif

protected:
	UInt16			mZoomLevel;		// magnification = mZoomLevel/256
	SDimension32	mImageSizeNorm;
	SPoint32		mImageLocationNorm;
  #if ZOOM_VIEW_TRACKING
	CMouseTracker*	mCurrTracker;
  #endif
private:
	void			InitZoomView();
};

#endif // CZOOMVIEW_H_INCLUDED



