// ======================================================
// CActiveScrollerControl.h
//
// a fix for a bug in LActiveScroller
//
// copyright © 1996, Sacred Tree Software, inc.
//
// written:  8/18/96, Ed Zavada
// modified: 
// ======================================================

#pragma once

#include <LStdControl.h>

typedef class CActiveScrollerControl : public LStdControl {
public:
		CActiveScrollerControl(const LStdControl &inOriginal)
					: LStdControl(inOriginal) {}
		CActiveScrollerControl(const SPaneInfo &inPaneInfo,
			MessageT inValueMessage, SInt32 inValue,
			SInt32 inMinValue, SInt32 inMaxValue,
			SInt16 inControlKind, ResIDT inTextTraitsID,
			Str255 inTitle, SInt32 inMacRefCon) 
					: LStdControl(inPaneInfo, inValueMessage, inValue, inMinValue, inMaxValue,
							inControlKind, inTextTraitsID, inTitle, inMacRefCon) {}
	virtual Boolean	TrackHotSpotTrackHotSpot(SInt16 inHotSpot, Point inPoint, SInt16 inModifiers);
} CActiveScrollerControl;

