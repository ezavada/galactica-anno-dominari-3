//	CScrollingPict.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#pragma once

#include <LPicture.h>
#include <LPeriodical.h>
#include "CNewGWorld.h"

class	CScrollingPict : public LPicture, public LPeriodical {

public:
	enum { class_ID = 'SPic' };
	
						CScrollingPict();
						CScrollingPict(const SPaneInfo &inPaneInfo,
							const SViewInfo &inViewInfo, ResIDT inPICTid,
							Boolean inUseOffscreen, Boolean inSaveBackground);
						CScrollingPict(LStream *inStream);
						CScrollingPict(ResIDT inPictureID, 
							Boolean inUseOffscreen, Boolean inSaveBackground);
						~CScrollingPict();
						
	virtual void	DrawSelf();
					
	void			SetTicksTillStart(UInt32 ticks)	{
									mStartTicks = ::TickCount()+ticks;
									mLastScrolledTicks = mStartTicks; 
								}
	void			SetTicksPerMove(UInt32 ticks) {mTicksPerMove = ticks;}
	void			SetOffsets(Point p) {mHorzOffset = p.h; mVertOffset = p.v;}
	void			GetOffsets(Point &outPt) const {outPt.h = mHorzOffset; outPt.v = mVertOffset;}
	void			SetNeedsColor(Boolean inNeedsIt) {mNeedsColor = inNeedsIt;}
	CNewGWorld*		GetPictGWorld() const {return mPictGWorld;}
	CNewGWorld*		GetScreenCopyGWorld() const {return mScreenCopyGWorld;}	
protected:
	CNewGWorld*	mPictGWorld;
	CNewGWorld*	mScreenCopyGWorld;
	CNewGWorld*	mWorkGWorld;
	UInt32	mStartTicks;
	UInt32	mTicksPerMove;
	UInt16	mHorzOffset;
	UInt16	mVertOffset;
	Boolean mNeedsColor;
	Boolean mUseOffscreen;
	Boolean mSaveBackground;
	Boolean mGotBackground;
	Boolean mAmRepeating;
	UInt32	mLastScrolledTicks;
	Rect	mImageVisible;	// the rectangle within the image that is visible (in Image coord)
	virtual	void	SpendTime(const EventRecord &inMacEvent);
	void			InitScrollingPict();
private:
	void			InitScrollingPictDefaults();
};


