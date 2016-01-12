// CMouseTracker.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Class for tracking mouse movements within a view as a periodical task, and drawing something
// in response to mouse position changes. Works with a CZoomView and responds to changes in 
// magnification. Will autoscroll the CZoomView if necessary.
// 10/12/97  ERZ

#include "CMouseTracker.h"


#pragma mark -


CMouseTracker::CMouseTracker(CZoomView* inWithinView) {
	mWithinView = inWithinView;
	mDrawn = false;
	*((long*)&mLastMousePt) = -1;
}

CMouseTracker::~CMouseTracker() {
	Hide(true);
}

void
CMouseTracker::AdjustZoomed() {	// compensate for zooming
	*((long*)&mLastMousePt) = -1;
}

void
CMouseTracker::Hide(Boolean inStop) {
	if (mDrawn) {		// don't leave line drawn
		if (mWithinView->GetSuperView()) {	// еее why??
			mWithinView->OutOfFocus(nil);
			mWithinView->FocusDraw();
			PrepareToDraw();
			EraseSelf(mLastMousePt);
		}
		mDrawn = false;
	}
	*((long*)&mLastMousePt) = -1;
	if (inStop) {
		LPeriodical::StopRepeating();
		if (mWithinView->GetMouseTracker() == this)	{	// not current anymore
			mWithinView->SetMouseTracker(nil);
		}
	}
}


void
CMouseTracker::Show(Boolean inStart) {
	if (inStart) {
		LPeriodical::StartRepeating();
		mWithinView->SetMouseTracker(this);
	}
}

void
CMouseTracker::StopRepeating() {		// funnel Stops and Starts through Hide() and Show()
	Hide(true);
}

void
CMouseTracker::StartRepeating() {
	Show(true);
}

void
CMouseTracker::SpendTime(const EventRecord &) {
	if (!mWithinView->GetSuperView())	// еее again, why???
		return;
	if (mWithinView->GetMouseTracker() != this) {	// don't allow a tracker to be Spending Time 
		delete this;								// if it is not the current one.
		return;
	}
	Point p;
	SPoint32 lp;
	mWithinView->FocusDraw();
	PrepareToDraw();
	::GetMouse(&p);
	if (*((long*)&p) == *((long*)&mLastMousePt)) 
		return;			// nothing to do if mouse hasn't moved
	if (mDrawn) {
		EraseSelf(mLastMousePt);
		mDrawn = false;
	}
	mLastMousePt = p;
	mWithinView->LocalToImagePoint(p, lp);
	if (mWithinView->ImagePointIsInFrame(lp.h, lp.v)) {
		Point deltaPt = {0,0};
		mWithinView->AutoScrollDragging(lp, deltaPt);
		if (*((long*)&deltaPt) != 0) {		// if it was scrolled, then the Within view is
			mWithinView->FocusDraw();		// no longer in focus and the drawing environment
			PrepareToDraw();				// is hosed, so we need to re-establish them
		}
		DrawSelf(p);
		mLastMousePt = p;
		mDrawn = true;
	}
}

void
CMouseTracker::PrepareToDraw() {
}

void
CMouseTracker::DrawSelf(Point) {	// inWherePt
}

void	
CMouseTracker::EraseSelf(Point) {	// inWherePt
}

