// Source for CHelpAttach class

#include "GenericUtils.h"

#include "CHelpAttach.h"
#include "CBalloonApp.h"

#include <UEnvironment.h>

LPane*	CHelpAttach::sHelpLinePane = nil;

void
CHelpAttach::SetHelpLinePane(LPane* inPane) {
	sHelpLinePane = inPane;
}


// strID is STR# res for active, enabled balloon text
// strID+1 is STR# for active, disabled balloon text
// strID+2 is STR# for inactive balloon text

// Default constructor
CHelpAttach::CHelpAttach(short strId, short index)
		: LAttachment(msg_AnyMessage, true) {
	mStrId = strId;		// Cache the resource info
	mIndex = index;
}

// Show help balloon
void 
CHelpAttach::ExecuteSelf(MessageT inMessage, void *ioParam) { //MessageT inMessage
	LPane* thePane = (LPane*)ioParam;	// get the pane and determine it's state

	// balloon help message	
  #if BALLOON_SUPPORT
	if (inMessage == msg_ShowHelp) {
		if (thePane->IsActive()) {
			if (thePane->IsEnabled())
				mPaneState = pane_Enabled;	// active && enabled
			else
				mPaneState = pane_Disabled;	// active && disabled
		} else
			mPaneState = pane_Inactive;	// inactive, whether enabled or disabled

		if (::HMIsBalloon()) {	// Remove any old balloon
			::HMRemoveBalloon();
		}
	
		::HMMessageRecord	aHelpMsg;	// Fill in Help manager record
		FillHMRecord(aHelpMsg);

		// Get hot rect so that balloon help automatically removes the balloon if it
		// moves out of the pane
		Rect hotRect;
		CalcPaneHotRect(thePane, hotRect);
		thePane->PortToGlobalPoint(topLeft(hotRect));
		thePane->PortToGlobalPoint(botRight(hotRect));
	
		// Set tip of balloon to centre of pane
		Point tip = {(hotRect.top + hotRect.bottom)/2,
						(hotRect.left + hotRect.right)/2};
	
		OSErr err = noErr;
		// Show the balloon
		err =::HMShowBalloon(&aHelpMsg,tip,&hotRect,nil,0,0,kHMSaveBitsWindow);
	
		// If there was an error set the cached pane to nil to force the balloon
		// to be redrawn the next time AdjustCursor is called. This is neccessary because
		// sometimes the mouse is moving too quick for help manager and balloon fails to draw.	
		if (err) {
			CBalloonApp::sHelpPane = nil;
		}
			
	// Help Line support
	} 
	else 
  #endif //BALLOON_SUPPORT
	if (inMessage == msg_ShowHelpLine) {
		if (CBalloonApp::sHelpLineDisplay) {
			LStr255 s(mStrId+3, mIndex);	// mStrID+3 is STR# for help line text
			CBalloonApp::sHelpLineDisplay->SetDescriptor(s);
			CBalloonApp::sHelpLineDisplay->UpdatePort();
		}
	}
}

void
CHelpAttach::CalcPaneHotRect(LPane* inPane, Rect &outRect) { // where to show the balloon
	inPane->CalcPortFrameRect(outRect);
}


// mStrId is STR# res for active, enabled balloon text
// mStrId+1 is STR# for active, disabled balloon text
// mStrId+2 is STR# for inactive balloon text
// mStrId+3 is STR# for help line text

#if BALLOON_SUPPORT
// Fill in the HMMessageRecord
void CHelpAttach::FillHMRecord(HMMessageRecord	&theHelpMsg)
{
	// Fill in Help manager record
	theHelpMsg.hmmHelpType = khmmStringRes;
	theHelpMsg.u.hmmStringRes.hmmResID = mStrId + (2-(int)mPaneState);
	theHelpMsg.u.hmmStringRes.hmmIndex = mIndex;	
}
#endif //BALLOON_SUPPORT



