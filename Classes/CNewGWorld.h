// CNewGWorld.h
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

#ifndef CNEWGWORLD_H_INCLUDED
#define CNEWGWORLD_H_INCLUDED

#include <UGWorld.h>

class CNewGWorld : public LGWorld {
public:
		CNewGWorld(ResIDT inPICTid, SInt16 inPixelDepth = 0, GWorldFlags inFlags = 0,
						CTabHandle inCTableH = nil, GDHandle inGDeviceH = nil);
		CNewGWorld(const Rect &inBounds, SInt16 inPixelDepth = 0, GWorldFlags inFlags = 0,
						CTabHandle inCTableH = nil, GDHandle inGDeviceH = nil);
		virtual ~CNewGWorld() {}

	void	GetBounds(Rect &outBounds) const {outBounds = mBounds;}
	void	SetNumSubImages(int inHorzSubs, int inVertSubs = 0);
	void	SetSubImageSize(int inHorzSize, int inVertSize = 0);
	ResIDT	GetPICTid() {return mPICTid;}	// if this returns 0, PICT failed to load

	virtual Boolean	PrepareDrawing();	// use this instead of BeginDrawing()

	Rect	GetSubImageSizeRect();
	
	void	CopyFrom(GrafPtr inSrcPort, const Rect &inSrcRect, SInt16 inXferMode = srcCopy,
							RgnHandle inMaskRgn = nil);

	void	CopySubImage(GrafPtr inDestPort, const Rect &inDestRect, int inHorzSub,
							int inVertSub = 1, SInt16 inXferMode = srcCopy,
							RgnHandle inMaskRgn = nil);

	void	CopyImageRect(GrafPtr inDestPort, const Rect &inDestRect, const Rect &inSrcRect,
							SInt16 inXferMode = srcCopy, RgnHandle inMaskRgn = nil);

	void	CopySubImageFrom(GrafPtr inSrcPort, const Rect &inSrcRect, int inHorzSub,
							int inVertSub = 1, SInt16 inXferMode = srcCopy,
							RgnHandle inMaskRgn = nil);
protected:
	ResIDT	mPICTid;
	Point	mSubSize;
	SInt8	mHorzSubs;
	SInt8	mVertSubs;
	SInt16		mDepth;
	GWorldFlags	mFlags;
	CTabHandle	mCTabH;
	GDHandle	mGDH;
};


#endif // CNEWGWORLD_H_INCLUDED

