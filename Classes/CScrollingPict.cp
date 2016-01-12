//	CScrollingPict.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
//	Displays and automatically scrolls a 'PICT' resource
//  modified 6/11/97, ERZ, resets scroll pos to above start point

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include "GenericUtils.h"

#include "CScrollingPict.h"
#include <LStream.h>

// ---------------------------------------------------------------------------
//		е CScrollingPict
// ---------------------------------------------------------------------------
//	Default Constructor

CScrollingPict::CScrollingPict() {
	InitScrollingPictDefaults();
	InitScrollingPict();
}


// ---------------------------------------------------------------------------
//		е CScrollingPict(SPaneInfo&, SViewInfo&, ResIDT, Boolean, Boolean)
// ---------------------------------------------------------------------------
//	Construct Picture from input parameters
//	save background can only be used if useOffscreen is also used

CScrollingPict::CScrollingPict(const SPaneInfo &inPaneInfo, const SViewInfo &inViewInfo,
								ResIDT inPICTid, Boolean inUseOffscreen, Boolean inSaveBackground)
: LPicture(inPaneInfo, inViewInfo, inPICTid) {
	InitScrollingPictDefaults();
	mUseOffscreen = inUseOffscreen;
	mSaveBackground = inSaveBackground && inUseOffscreen;
	InitScrollingPict();
}


// ---------------------------------------------------------------------------
//		е CScrollingPict(LStream*)
// ---------------------------------------------------------------------------
//	Construct Picture from the data in a Stream

CScrollingPict::CScrollingPict(LStream *inStream)
: LPicture(inStream) {
	InitScrollingPictDefaults();
	*inStream >> mStartTicks;
	*inStream >> mTicksPerMove;
	*inStream >> mHorzOffset;
	*inStream >> mVertOffset;
	*inStream >> mNeedsColor;
	*inStream >> mUseOffscreen;
	*inStream >> mSaveBackground;
	mStartTicks += (UInt32)::TickCount();
	InitScrollingPict();
}


// ---------------------------------------------------------------------------
//		е CScrollingPict(ResIDT, Boolean, Boolean)
// ---------------------------------------------------------------------------
//	Construct a ScrollingPict from a PICT Resource ID

CScrollingPict::CScrollingPict(ResIDT inPictureID, Boolean inUseOffscreen, Boolean inSaveBackground)
: LPicture(inPictureID) {
	InitScrollingPictDefaults();
	mUseOffscreen = inUseOffscreen;
	mSaveBackground = inSaveBackground && inUseOffscreen;
	InitScrollingPict();
}


CScrollingPict::~CScrollingPict() {
	if (mWorkGWorld)
		delete mWorkGWorld;
	if (mPictGWorld)
		delete mPictGWorld;
	if (mScreenCopyGWorld)
		delete mScreenCopyGWorld;
}

// ---------------------------------------------------------------------------
//		е InitScrollingPictDefaults
// ---------------------------------------------------------------------------
//	Private Initializer. Inits scrolling info to defaults

void
CScrollingPict::InitScrollingPictDefaults() {
	mStartTicks = -1;
	mTicksPerMove = 4;
	mHorzOffset = 0;
	mVertOffset = 0;
	mNeedsColor = false;
	mUseOffscreen = false;
	mSaveBackground = false;
	mGotBackground = false;
	mAmRepeating = false;
	mPictGWorld = nil;
	mScreenCopyGWorld = nil;
	mWorkGWorld = nil;
	::MacSetRect(&mImageVisible, 0, 0, mImageSize.width, mImageSize.height);
	if (mImageVisible.right > mFrameSize.width)
		mImageVisible.right = mFrameSize.width;
	if (mImageVisible.bottom > mFrameSize.height)
		mImageVisible.bottom = mFrameSize.height;
}

// ---------------------------------------------------------------------------
//		е InitScrollingPict
// ---------------------------------------------------------------------------
//	protected Initializer. Gets it ready to go.

void
CScrollingPict::InitScrollingPict() {
	mLastScrolledTicks = mStartTicks;
	if (mUseOffscreen) {
		try {
			if (mNeedsColor) {
				mPictGWorld = new CNewGWorld(mPICTid);	// put pict into GWorld, match screen depth
			} else {
				mPictGWorld = new CNewGWorld(mPICTid, 1);	// put pict into 1 bit deep GWorld
			}
			if (mSaveBackground) {
				Rect aframe;
				CalcPortFrameRect(aframe);
				::MacOffsetRect(&aframe, - aframe.left, - aframe.top);
				mScreenCopyGWorld = new CNewGWorld(aframe);	// make GWorld to match pane's frame
			}
		}
		catch(...) {	// couldn't allocate GWorlds
			mUseOffscreen = false;	// may be ugly, but it will work
			mSaveBackground = false;
			if (mPictGWorld) {		// don't leak memory
				delete mPictGWorld;
				mPictGWorld = nil;
			}
			if (mScreenCopyGWorld) {// don't leak memory
				delete mScreenCopyGWorld;
				mScreenCopyGWorld = nil;
			}
		}
		if (mSaveBackground) {
			try {
				Rect frame;
				CalcPortFrameRect(frame);
				::MacOffsetRect(&frame, - frame.left, - frame.top);
				mWorkGWorld = new CNewGWorld(frame);	// make buffer GWorld, too
			}
			catch(...) { // if it fails, don't sweat it, we can just live with the flicker
				if (mWorkGWorld)
					delete mWorkGWorld;
				mWorkGWorld = nil;
			}
		}
	}
}

void
CScrollingPict::DrawSelf() {
	Rect r;
	if (!mAmRepeating) {
		StartRepeating();	// now that we've been told to draw, we can start repeating
		mAmRepeating = true;
	}
	if (mSaveBackground && !mGotBackground) {
		GrafPtr srcPort;
		::GetPort(&srcPort);
		mScreenCopyGWorld->GetBounds(r);			// copy the area to be overwritten
		mScreenCopyGWorld->CopyFrom(srcPort, r);	// from out of the current port
		mGotBackground = true;	
	}
	if (mUseOffscreen) {
		Rect srcRect = mImageVisible;
		Rect frame = mImageVisible;
		::MacOffsetRect(&frame, - frame.left, - frame.top);
//		OffsetRect(&frame, mFrameLocation.h - frame.left, mFrameLocation.v - frame.top);
		GrafPtr dstPort;
		mPictGWorld->GetBounds(r);			// clip to offscreen bitmap
		short diff = srcRect.bottom - r.bottom;	// these are for scrolling up or left
		if (diff > 0) {
			srcRect.bottom -= diff;
			frame.bottom -= diff;
		}
		diff = srcRect.right - r.right;
		if (diff > 0) {
			srcRect.right -= diff;
			frame.right -= diff;
		}
		diff = r.top - srcRect.top;	// these are for scrolling down or right
		if (diff > 0) {
			srcRect.top += diff;
			frame.top += diff;
		}
		diff = r.left - srcRect.left;
		if (diff > 0) {
			srcRect.left += diff;
			frame.left += diff;
		}
		if (mSaveBackground) {
			Rect r;
			GrafPtr dstPort;
			if (mWorkGWorld)
				dstPort = (GrafPtr) mWorkGWorld->GetMacGWorld(); // have buffer, draw into it
			else
				::GetPort(&dstPort);		// opps, no buffer, draw directly to screen
			mScreenCopyGWorld->GetBounds(r);		  // redraw the screen area that was
			mScreenCopyGWorld->CopyImage(dstPort, r); // overwritten from mScreenCopyGWorld
		}
		if (mWorkGWorld && mGotBackground)
			dstPort = (GrafPtr) mWorkGWorld->GetMacGWorld();	// have buffer, draw into it
		else
			::GetPort(&dstPort);		// opps, no buffer, draw directly to screen
		if (mNeedsColor)
			mPictGWorld->CopyImageRect(dstPort, frame, srcRect, srcCopy);	// еее notSrcBic ???
		else
			mPictGWorld->CopyImageRect(dstPort, frame, srcRect, srcBic);	// еее notSrcBic ???
		if (mWorkGWorld && mGotBackground) {
			::GetPort(&dstPort);
			mWorkGWorld->GetBounds(r);			// now dump the buffer 
			mWorkGWorld->CopyImage(dstPort, r);	// into the screen area to update
		}
	} else 
		LPicture::DrawSelf();
}

void
CScrollingPict::SpendTime(const EventRecord &) { //inMacEvent) {
	FocusDraw();
	UInt32 tick = ::TickCount();
	if (tick > mStartTicks) {
		UInt32 waited = tick - mLastScrolledTicks;
		if (waited > mTicksPerMove) {	// have we waited long enough between scrolls ?
			waited /= mTicksPerMove;
			long horz = mHorzOffset;// * waited;
			long vert = mVertOffset;// * waited;
			::MacOffsetRect(&mImageVisible, horz, vert);		// keep track of what we can see
			if ( (mImageVisible.left > mImageSize.width) ||	// image totally outside frame?
				 (mImageVisible.top > mImageSize.height) ) {
				::MacOffsetRect(&mImageVisible, -mImageVisible.left, -mImageVisible.top);
				if (mHorzOffset) {
					::MacOffsetRect(&mImageVisible, -mFrameSize.width, 0);
				}
				if (mVertOffset) {
					::MacOffsetRect(&mImageVisible, 0, -mFrameSize.height);
				}
			}
			else if ( (mImageVisible.right < 0) ||	// image totally outside frame?
						(mImageVisible.bottom < 0) ) {
				::MacOffsetRect(&mImageVisible, -mImageVisible.left - mFrameSize.width, 
								-mImageVisible.top - mFrameSize.height);
			}
			if (!mUseOffscreen)
				ScrollImageTo(mImageVisible.left, mImageVisible.top, true);	// redraw immediately
			else {
				DrawSelf();
			}
			mLastScrolledTicks = tick;
		}
	}
}
