// ===========================================================================
//	CResCaption.cp				   ©1993-1995 Metrowerks Inc. All rights reserved.
// ===========================================================================
//
//	Pane with a single line of text

#include "GenericUtils.h"

#include "CResCaption.h"

#include <LStream.h>
#include <UTextTraits.h>
#include <UDrawingUtils.h>


// ---------------------------------------------------------------------------
//		¥ CResCaption
// ---------------------------------------------------------------------------
//	Default Constructor

CResCaption::CResCaption() {
	mSTRxID = 0;
	mStrIdx = 0;
	mDrawDisabled = false;
}


// ---------------------------------------------------------------------------
//		¥ CResCaption(SPaneInfo&, ResIDT, short, ResIDT)
// ---------------------------------------------------------------------------
//	Construct from parameters. Use this constructor to create a Caption
//	from runtime data.

CResCaption::CResCaption(const SPaneInfo &inPaneInfo,
	ResIDT inTextTraitsID, ResIDT inSTRxID, short inStrIdx)
:LCaption(inPaneInfo, "\p", inTextTraitsID) {
	SetStrID(inSTRxID, inStrIdx);
}	


// ---------------------------------------------------------------------------
//		¥ CResCaption(LStream*)
// ---------------------------------------------------------------------------
//	Construct from data in a Stream

CResCaption::CResCaption(LStream *inStream)
: LCaption(inStream) {
	*inStream >> mSTRxID;
	*inStream >> mStrIdx;
	*inStream >> mDrawDisabled;
	if (mSTRxID)
		mText.Assign(mSTRxID, mStrIdx);
}


// ---------------------------------------------------------------------------
//		¥ SetStrID
// ---------------------------------------------------------------------------
//	Set a Caption to the text contained in a STR# resource at a particular index

void
CResCaption::SetStrID(ResIDT inSTRxID, short inStrIdx) {
	mSTRxID = inSTRxID;
	mStrIdx = inStrIdx;
	ReloadStrRes();
}

void
CResCaption::ReloadStrRes() {
	if (mSTRxID) {
		mText.Assign(mSTRxID, mStrIdx);
	} else {
		mText.Assign("\p");
	}
	Refresh();
}

void
CResCaption::DrawSelf() {
	Rect	frame;
	CalcLocalFrameRect(frame);
	SInt16	just = UTextTraits::SetPortTextTraits(mTxtrID);
	RGBColor	textColor;
	::GetForeColor(&textColor);
	ApplyForeAndBackColors();
	::RGBForeColor(&textColor);
	if (mDrawDisabled && (mEnabled != triState_On)) {
		::TextMode(grayishTextOr);
	}
	UTextDrawing::DrawWithJustification((Ptr)&mText[1], mText[0], frame, just);
}

void
CResCaption::EnableSelf() {
	if (mDrawDisabled && FocusExposed()) {
		DrawSelf();
	}
}

void
CResCaption::DisableSelf() {
	if (mDrawDisabled && FocusExposed()) {
		DrawSelf();
	}
}
