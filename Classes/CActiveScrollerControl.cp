// ======================================================
// CActiveScrollerControl.cp
//
// a fix for a bug in LActiveScroller
//
// copyright й 1996, Sacred Tree Software, inc.
//
// written:  8/18/96, Ed Zavada
// modified: 9/4/98, ERZ, updated to match code in CWPro3
// ======================================================

#include "GenericUtils.h"

#include "CActiveScrollerControl.h"

// ---------------------------------------------------------------------------
//		е TrackHotSpot
// ---------------------------------------------------------------------------
//	Track the mouse while it is down after clicking in a Control HotSpot
//  This is modified to fix a bug in LActiveScroller, caused by some very
//  strange behavior on the part of the Control Manager.
//
//	Returns whether the mouse is released within the HotSpot

Boolean
CActiveScrollerControl::TrackHotSpotTrackHotSpot(SInt16 inHotSpot, Point inPoint, SInt16 ) { //inModifiers
	Boolean fixThumbScrollBug = false; // ееее ERZ change to standard TrackHotSpot()

		// For some bizarre reason, the actionProc for a thumb (indicator)
		// has different parameters than that for other parts. This
		// class uses the actionProc in the ControlRecord for non-thumb
		// parts, and the member variable mThumbFunc for the thumb.
		// (ProcPtr)(-1) is a special flag that tells the ControlManager
		// to use the actionProc in the ControlRecord.
	
	ControlActionUPP	actionProc = (ControlActionUPP) (-1);
	if (inHotSpot >= kControlIndicatorPart) {
		actionProc = (ControlActionUPP) mThumbFunc;
		LStdControl::sTrackingControl = this;
		fixThumbScrollBug = true; // ееее ERZ change to standard TrackHotSpot()
	}
	
		// TrackControl handles tracking and returns zero
		// if the mouse is released outside the HotSpot
	
	SInt16	origValue = ::GetControlValue(mMacControlH);
	Boolean	releasedInHotSpot =
		::TrackControl(mMacControlH, inPoint, actionProc) != kControlNoPart;
		
	LStdControl::sTrackingControl = nil;

	if (fixThumbScrollBug) { // ееее ERZ changes to standard TrackHotSpot()
	
		// the ActiveScroller bug occured when scrolling with the thumb box.
		// For some unfathomable reason, the Control Manager was changing the
		// value of the control to its maximum value after the call to TrackControl()
		// Fortunately, the mValue field contains the correct value of the control
		// so we can use that to restore the correct value in the MacControl.
		SInt16	ctlValue = mValue;
		if (mUsingBigValues) {		// Scale from 32 to 16 bit value
			ctlValue = CalcSmallValue(mValue);
		}	
		FocusDraw();
		::SetControlValue(mMacControlH, ctlValue);
		
	} // ееее end ERZ changes to standard TrackHotSpot()
	
		// Control Manager can change the value while tracking.
		// If it did, we need to call SetValue() to update the
		// class's copy of the value.
		
	SInt32	currValue = ::GetControlValue(mMacControlH);
	if (currValue != origValue) {
		SInt32	newValue = currValue;
		if (mUsingBigValues) {		// Scale from 16 to 32 bit value
			newValue = CalcBigValue((SInt16) currValue);
		}
		
		SetValue(newValue);
	}
	
	return releasedInHotSpot;
}


