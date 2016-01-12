// CNewGWorld.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// An improved LGWorld class that can initialize
// itself with a PICT Resource and divide itself
// into subimages which can be drawn separately.
// Can copy itself from a port (ie. the screen)
// rather than just copying itself to a port.
// 
// created	6/6/96, ERZ	
// modified 7/27/97, ERZ, added PrepareDrawing()
// modified 5/11/98, ERZ, removed unnecessary include of "GenericUtils.h"
// modified 4/6/99, ERZ, added some input parameter checking

#include "CNewGWorld.h"
#include "DebugMacros.h"
#include <MacWindows.h>
#include <UDrawingState.h>

#if !ACCESSOR_CALLS_ARE_FUNCTIONS
const BitMap* GetPortBitMapForCopyBits(void* port);

const BitMap*
GetPortBitMapForCopyBits(void* port) {
    return &((GrafPtr)port)->portBits;
}
#endif

Rect GetPictRect(ResIDT inPICTid);

Rect GetPictRect(ResIDT inPICTid) {
	Rect rp;
	PicHandle macPictH = ::GetPicture(inPICTid);
	if (macPictH) {						 // Use PICT if found
 		rp = (**macPictH).picFrame;
        rp.top = EndianS16_BtoN(rp.top); // Mac Picture is Big Endian
        rp.left = EndianS16_BtoN(rp.left);
        rp.bottom = EndianS16_BtoN(rp.bottom); // Mac Picture is Big Endian
        rp.right = EndianS16_BtoN(rp.right);
 		::MacOffsetRect(&rp, -rp.left, -rp.top);
 		::ReleaseResource((Handle)macPictH);
 	} else {	// PICT didn't exist, use size of desktop instead
		RgnHandle	grayRgnH = ::GetGrayRgn();
		::GetRegionBounds(grayRgnH, &rp);
 	}
 	return rp;
}

CNewGWorld::CNewGWorld(ResIDT inPICTid, SInt16 inPixelDepth, GWorldFlags inFlags,
						CTabHandle inCTableH, GDHandle inGDeviceH) 
:LGWorld( GetPictRect(inPICTid), inPixelDepth, inFlags, inCTableH, inGDeviceH) {
	mHorzSubs = 1;
	mVertSubs = 1;
	mSubSize.h = mBounds.right - mBounds.left;
	mSubSize.v = mBounds.bottom - mBounds.top;
	if (inPICTid) {
		BeginDrawing();	// do this before loading PICT so Picture allocation can't purge pixels of gworld
		PicHandle macPictH = ::GetPicture(inPICTid);
		if (macPictH) {
			::ForeColor(blackColor);
			::BackColor(whiteColor);
			::DrawPicture(macPictH, &mBounds);
			::ReleaseResource((Handle)macPictH);
			mPICTid = inPICTid;
		} else {
			mPICTid = 0;	// signals failed to load picture
		}
		EndDrawing();
	}
	mDepth = inPixelDepth;
	mFlags = inFlags;
	mCTabH = inCTableH;
	mGDH = inGDeviceH;
}

CNewGWorld::CNewGWorld(const Rect &inBounds, SInt16 inPixelDepth, GWorldFlags inFlags,
						CTabHandle inCTableH, GDHandle inGDeviceH) 
:LGWorld(inBounds, inPixelDepth, inFlags, inCTableH, inGDeviceH) {
	mPICTid = 0;
	mHorzSubs = 1;
	mVertSubs = 1;
	mSubSize.h = mBounds.right - mBounds.left;
	mSubSize.v = mBounds.bottom - mBounds.top;
	mDepth = inPixelDepth;
	mFlags = inFlags;
	mCTabH = inCTableH;
	mGDH = inGDeviceH;
}

void
CNewGWorld::SetNumSubImages(int inHorzSubs, int inVertSubs) {
	if (inHorzSubs > 0) {
		mHorzSubs = inHorzSubs;
		mSubSize.h = (mBounds.right-mBounds.left)/inHorzSubs;	// now recalc sub-image horizontal size to match
	}
	if (inVertSubs > 0) {
		mVertSubs = inVertSubs;
		mSubSize.v = (mBounds.bottom-mBounds.top)/inVertSubs;	// now recalc sub-image vertical size to match
	}
}

void
CNewGWorld::SetSubImageSize(int inHorzSize, int inVertSize) {
	if (inHorzSize > 0) {
		mSubSize.h = inHorzSize;
		mHorzSubs = (mBounds.right-mBounds.left+mSubSize.h-1)/mSubSize.h;	// calc new num h subs
	}
	if (inVertSize > 0) {
		mSubSize.v = inVertSize;
		mVertSubs = (mBounds.bottom-mBounds.top+mSubSize.v-1)/mSubSize.v;	// calc new num v subs
	}
}

// call this
Boolean
CNewGWorld::PrepareDrawing() {
	Boolean ready = BeginDrawing();
	GWorldFlags flags = mFlags;
	if (!ready) {	// only reason it would fail is if the pixels have been purged, so reallocate them
		GWorldFlags flags = ::UpdateGWorld(&mMacGWorld, mDepth, &mBounds, mCTabH, mGDH, mFlags);
		ready = ( (flags & gwFlagErr) == 0);	// check for no failure flag
		if (!ready) {	// Update did not succeed.
			flags = mFlags | (useTempMem + pixPurge);	// try again with temp memory
			flags = ::UpdateGWorld(&mMacGWorld, mDepth, &mBounds, mCTabH, mGDH, flags);
			ready = ( (flags & gwFlagErr) == 0);
		}
		if (ready && mPICTid) {
			BeginDrawing();
			PicHandle macPictH = ::GetPicture(mPICTid);
			if (macPictH) {
				::ForeColor(blackColor);
				::BackColor(whiteColor);
				::DrawPicture(macPictH, &mBounds);
				::ReleaseResource((Handle)macPictH);
			}
		}
	}
	return ready;
}


Rect 
CNewGWorld::GetSubImageSizeRect() {
	Rect r;	// calc source rect
	r.top = 0;
	r.left = 0;
	r.bottom = mSubSize.v;
	r.right = mSubSize.h;
	return r;
}

void
CNewGWorld::CopyFrom(GrafPtr inSrcPort, const Rect &inSrcRect, 
					SInt16 inXferMode, RgnHandle inMaskRgn) {
	ASSERT(inSrcPort != nil);
	// unlike CopyImage, this routine will be used infrequently, so we can set the fore and
	// background colours to black and white so CopyBits will handle the transfer correctly
	StColorState colours;
	colours.Normalize();
	::HideCursor();	// just in case this was from the screen
    const BitMap*  srcBits = ::GetPortBitMapForCopyBits(inSrcPort);
    const BitMap*  dstBits = ::GetPortBitMapForCopyBits(mMacGWorld);
	::CopyBits(srcBits, dstBits, &inSrcRect, &mBounds, inXferMode, inMaskRgn);
	::MacShowCursor();
}

// inHorzSub and inVertSub specify sub-image starting with (1,1) for top left corner sub-image
void
CNewGWorld::CopySubImage(GrafPtr inDestPort, const Rect &inDestRect, int inHorzSub,
						int inVertSub, SInt16 inXferMode, RgnHandle inMaskRgn) {
	ASSERT(inDestPort != nil);
	Rect r;	// calc source rect
	r.top = 0;
	r.left = 0;
	r.bottom = mSubSize.v;
	r.right = mSubSize.h;
	if (inHorzSub > mHorzSubs) inHorzSub = mHorzSubs;
	if (inVertSub > mVertSubs) inVertSub = mVertSubs;
	inHorzSub = (inHorzSub > 0) ? inHorzSub-1 : 1;
	inVertSub = (inVertSub > 0) ? inVertSub-1 : 1;
	::MacOffsetRect(&r, mBounds.left + inHorzSub*mSubSize.h, mBounds.top + inVertSub*mSubSize.v);
    const BitMap* srcBits = ::GetPortBitMapForCopyBits(mMacGWorld);
    const BitMap* dstBits = ::GetPortBitMapForCopyBits(inDestPort);   
	::CopyBits(srcBits, dstBits, &r, &inDestRect, inXferMode, inMaskRgn);
}

// inSrcRect specifys portion of image to copy out
void
CNewGWorld::CopyImageRect(GrafPtr inDestPort, const Rect &inDestRect, const Rect &inSrcRect,
						SInt16 inXferMode, RgnHandle inMaskRgn) {
	ASSERT(inDestPort != nil);
    const BitMap* srcBits = ::GetPortBitMapForCopyBits(mMacGWorld);
    const BitMap* dstBits = ::GetPortBitMapForCopyBits(inDestPort);
	::CopyBits(srcBits, dstBits, &inSrcRect, &inDestRect, inXferMode, inMaskRgn);
}

// inHorzSub and inVertSub specify sub-image starting with (1,1) for top left corner sub-image
void
CNewGWorld::CopySubImageFrom(GrafPtr inSrcPort, const Rect &inSrcRect, int inHorzSub,
						int inVertSub, SInt16 inXferMode, RgnHandle inMaskRgn) {
	ASSERT(inSrcPort != nil);
	Rect r;		// calc dest rect
	r.top = 0;
	r.left = 0;
	r.bottom = mSubSize.v;
	r.right = mSubSize.h;
	if (inHorzSub > mHorzSubs) inHorzSub = mHorzSubs;
	if (inVertSub > mVertSubs) inVertSub = mVertSubs;
	inHorzSub = (inHorzSub > 0) ? inHorzSub-1 : 1;
	inVertSub = (inVertSub > 0) ? inVertSub-1 : 1;
	::MacOffsetRect(&r, mBounds.left + inHorzSub*mSubSize.h, mBounds.top + inVertSub*mSubSize.v);
	// unlike CopySubImage, this routine will be used infrequently, so we can set the fore and
	// background colours to black and white so CopyBits will handle the transfer correctly
	StColorState colours;
	colours.Normalize();
	::HideCursor();	// just in case this was from the screen
    const BitMap* srcBits = ::GetPortBitMapForCopyBits(inSrcPort);
    const BitMap* dstBits = ::GetPortBitMapForCopyBits(mMacGWorld);
	::CopyBits(srcBits, dstBits, &inSrcRect, &r, inXferMode, inMaskRgn);
	::MacShowCursor();
}
