// CMouseTracker.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 10/12/97  ERZ
// modified, 11/2/98, ERZ, added IsDrawn() method

#ifndef CMOUSE_TRACKER_H_INCLUDED
#define CMOUSE_TRACKER_H_INCLUDED

#include <LView.h>
#include <LPeriodical.h>
#include <PP_Messages.h>

#include "CZoomView.h"

class CMouseTracker : public LPeriodical {
public:
			CMouseTracker(CZoomView* inWithinView);
			~CMouseTracker();
	virtual	void	StopRepeating();
	virtual	void	StartRepeating();
	virtual	void	SpendTime(const EventRecord &inMacEvent);
	virtual void	AdjustZoomed();
	void			Hide(Boolean inStop = false);
	void			Show(Boolean inStart = false);
	Boolean			IsDrawn() {return mDrawn;}
	virtual void	PrepareToDraw();			// override to set up pen mode, etc...
	virtual void	DrawSelf(Point inWherePt);	// override to draw
	virtual void	EraseSelf(Point inWherePt);	// override to erase
protected:
	CZoomView*	mWithinView;
	Point		mLastMousePt;
	Boolean		mDrawn;
};

#endif // CMOUSE_TRACKER_H_INCLUDED

