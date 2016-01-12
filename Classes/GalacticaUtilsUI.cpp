// GalacticaUtils.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 5/6/96 ERZ

// еее explanations of Galactic coordinates in GalacticaUtils.h еее
 
#include "GalacticaUtilsUI.h"
#include "CFleet.h"

#if TUTORIAL_SUPPORT
#include "GalacticaTutorial.h"
#endif // TUTORIAL_SUPPORT

#include "CKeyCmdAttach.h"
#include "UNewModalDialogs.h"

#include "GalacticaPanes.h"
#include "GalacticaPanels.h"
#include "Galactica.h"
#include "GalacticaDoc.h"
#include "USound.h"

#if TARGET_OS_MAC && defined(DEBUG) && defined(__MWERKS__)	// ERZ, v1.2b8, DebugNew only supported on CW MacOS
  #include <DebugNew.h>
#endif

// if THINGYREF_REMEMBER_LOOKUPS is true, once a ThingyRef looks up the
// the thingy it references in the Everything list, it stores a pointer to
// it. Otherwise, it has to do a lookup by ID number every time. 
#define THINGYREF_REMEMBER_LOOKUPS	true


// ERZ Win32 Mod -- stuff needed for string conversions
#if TARGET_OS_WIN32
#define      FIXEDDECIMAL   ((char)(1))
#endif


// defined in GalacticaUtils.cpp
extern bool ThingyIsValid(const AGalacticThingy* inThingy);

AGalacticThingyUI*
ValidateThingyUI(const AGalacticThingy* inThingy) {
    inThingy = ValidateThingy(inThingy); 
    if (inThingy == NULL) {
        return NULL;
    }
    AGalacticThingyUI* ui = const_cast<AGalacticThingy*>(inThingy)->AsThingyUI();
    if (ui == NULL) {
		DEBUG_OUT("Detected Non-UI Thingy "<<inThingy<<" {"<<(void*)inThingy<<"}", DEBUG_ERROR);
		return NULL;
	} else {
	    return ui;
	}
}

// this function creates an PowerPlant LArray class that has the same
// contents as a std::vector<Waypoint>, as returned from CShip::GetWaypoints()
// It is used to populate PowerPlant LTable classes. The LTable should own
// the LArray, since otherwise it will be leaked
LArray* 
MakeArrayFromWaypointVector(const std::vector<Waypoint>& inWaypointVector) {
    DEBUG_OUT("Creating new array from ship waypoints vector:", DEBUG_TRIVIA | DEBUG_USER);
    LArray* theArray = new LArray(sizeof(Waypoint));
    for (int i = 0; i < (int)inWaypointVector.size(); i++) {
        Waypoint wp = inWaypointVector[i];
        DEBUG_OUT(" " << i << " = " << wp, DEBUG_TRIVIA | DEBUG_USER);
		theArray->InsertItemsAt(1, kEndOfWaypointList, &wp);
    }
    return theArray;
}

#pragma mark -

//CDialogAttachment::CDialogAttachment()	// just init everything to defaults
//:LAttachment() {
//	mDialog = nil;
//	mDoSysBeep = true;
//}

CDialogAttachment::CDialogAttachment(MessageT inMessage, bool inExecuteHost, bool inDoBeep)
:LAttachment(inMessage, inExecuteHost) {
	mDialog = nil;
	mDoSysBeep = inDoBeep;
}

bool
CDialogAttachment::PrepareDialog() {
	// MovableDialog() is about to display a dialog. Return false to abort display
	// and return msg_Nothing to the caller.
	// mDialog will contain the LWindow that is about to be shown
	// if you set mDoSysBeep to true here, the sysbeep will be played
	// setting it to false will skip the beep. true is the default value
	return true;
}

bool
CDialogAttachment::CloseDialog(MessageT ) { //inMsg) {
	// MovableDialog() is about to close the dialog in response to a message. 
	// inMsg is the message. Return false if you want to prohibit closing
	// If you want to have some controls in the dialog do something, this is a good
	// place to handle it. Setup the control to broadcast a message, then override
	// this method to check for that message and perform the action, returning false
	// so that MovableAlert() won't close the dialog.
	return true;
}

void
CDialogAttachment::SetParamText(UInt8 , LStr255 &) { // inTextNum, ioNewString
	// MovableDialog() found a "^0" style ParamText arguement. inTextNum is the number that
	// followed the ^ character, and ioString is the proposed replacement. Set ioString to
	// something different if you want.
}

#pragma mark -

LStr255 movableModalParamStrs[4];

void
MovableParamText(SInt16 number, LStr255 &inStr) {
	Assert_((number >= 0) && (number < 4));
	movableModalParamStrs[number] = inStr;
}

void
MovableParamText(SInt16 number, const char* inCStr) {
	Assert_((number >= 0) && (number < 4));
    movableModalParamStrs[number] = inCStr;
}


// will use the ::MovableParamText() strings same as an Alert, but will look for a caption ids starting at
// 100 to change the Paramtext strings, IDs must be consecutive
// if inAttachment is not nil, the CDialogAttachment is added to the EventDispatcher while the Alert
// is displayed, and removed when closed.

long
MovableAlert(ResIDT inWindowID, LCommander* inSuper, int inAutoCloseHalfSecs, 
				bool inBeep, CDialogAttachment* inAttachment) {
	if (inSuper == nil) {
		inSuper = LCommander::GetTopCommander();	// assign the application if necessary
	}
	StNewDialogHandler aDialog(inWindowID, inSuper);
	LWindow* wind = aDialog.GetDialog();
	LEventDispatcher* theEventer = LEventDispatcher::GetCurrentEventDispatcher();
	bool show = true;
	MessageT response = msg_Nothing;
	if (inAttachment && theEventer) {
		theEventer->AddAttachment(inAttachment);
		inAttachment->SetDialog(wind);
	}
	if (inAttachment) {
		show = inAttachment->PrepareDialog();
		inBeep = inAttachment->DoSysBeep();
	} else if (inAutoCloseHalfSecs != 0) {	// don't beep on autoclose
		inBeep = false;
	}
	if (show) {
		LStr255 s[5];
		for (int i = 0; i < 4; i++) {	// get LPane.h
			s[i] = movableModalParamStrs[i];
		}
		unsigned long closeTicks;
		if (inAutoCloseHalfSecs) {
			closeTicks = inAutoCloseHalfSecs * 30 + ::TickCount();
		} else { 
			closeTicks = 0xffffffff;	// take a really long time to auto-close
		}
		LStr255 ds;
		PaneIDT id = 100;	// only check panes with ID of 100 and up
		LPane* thePane;
		char code[2] = {'^','0'};
		int pos;
		while (true) {	// check for ParamTexts in the fields of the Dialog
			thePane = wind->FindPaneByID(id);
			if (thePane == nil) {
				break;
			}
			thePane->GetDescriptor(ds);
			if ( ds.Find("^", 1, 1) != 0) {	// do we have any ParamTexts?
				for (int i = 0; i < 9; i++) {	// check for specific: ^0...^9
					code[1] = i + '0';
					do {
						pos = ds.Find(code, 2, 1);
						if (pos) {
							LStr255 ns;
							if (i < 4) {
								ns = s[i];
							}
							if (inAttachment) {
								inAttachment->SetParamText(i, ns);	// let attachment filter it
							}
							ds.Replace(pos, 2, ns);
						}
					} while (pos);	// keep repeating till we don't find it, may be multipule
				}
				thePane->SetDescriptor(ds);
			}
			id++;
		}
		// now the ParamTexts are assigned, show the window
		CKeyCmdAttach::DisableCmdKeys(); // no keyboard shortcuts while alert is showing
		wind->Show();
		wind->UpdatePort();
		if (inBeep) {	// don't beep until we've had a chance to handle update event
			SysBeep(1);
		}
		bool canClose = true;
		do {
			do {
				response = aDialog.DoDialog();	// handle events for the dialog
			} while ( (response == msg_Nothing) && (::TickCount() < closeTicks) );
			if (inAttachment) {
				canClose = inAttachment->CloseDialog(response);
			}
		} while (!canClose);	// repeat until we are allowed to close
		CKeyCmdAttach::EnableCmdKeys(); // disabled while alert was showing
	}
	aDialog.SetUpdateCommandStatus(true);
	if (inAttachment && theEventer) {
		inAttachment->SetDialog(nil);
		theEventer->RemoveAttachment(inAttachment);
		if (inAttachment->AutoDelete()) {
			delete inAttachment;
		}
	}
	return response;	// then return the resulting message
}


// ---------------------------------------------------------------------------
//	е AskForOneValue										 [static] [public]
// ---------------------------------------------------------------------------
//	Present a Moveable Modal dialog for retrieving a particular value
//
//	Returns TRUE if entry is OK'd.
//	On entry, ioNumber is the current value to display for the number
//	On exit, ioNumber is the new value if OK'd, unchanged if Canceled

bool
AskForOneValue(
	LCommander*		inSuper,
	ResIDT			inDialogID,
	PaneIDT			inEntryPaneID,
	SInt32&			ioNumber)
{
	StDialogHandler	theHandler(inDialogID, inSuper);
	LWindow			*theDialog = theHandler.GetDialog();
	
	LPane *theField = theDialog->FindPaneByID(inEntryPaneID);
	
	if (theField == nil) {
		SignalStringLiteral_("No Entry Pane with specified ID");
		return false;
	}
	
	theField->SetValue(ioNumber);
//	theField->SelectAll();
//	theDialog->SetLatentSub(theField);
	theDialog->Show();
	
	bool		entryOK = false;
	
	while (true) {
		MessageT	hitMessage = theHandler.DoDialog();
		
		if (hitMessage == msg_Cancel) {
			break;
			
		} else if (hitMessage == msg_OK) {
			ioNumber = theField->GetValue();
			entryOK = true;
			break;
		}
	}
	
	return entryOK;
}


#pragma mark -

// a predone dialog attachment, useful for getting a value from the user. Its main advantage
// over UModalDialogs::AskForOneString() is that you can put ParamText() codes into the dialog
// and it will deal with them properly. You may also find it useful as a base class for a
// dialog attachment that does something slightly fancier, or just as a simple example of how
// the CDialogAttachment class works with the MovableAlert() call.

bool
CGet1StringDlgHndlr::PrepareDialog() {
	mResult = mDialog->FindPaneByID(10);
	ThrowIfNil_(mResult);
	mResult->SetDescriptor(*mResultStr);
	mDoSysBeep = false;	// don't beep
	// begin tutorial stuff
  #if TUTORIAL_SUPPORT
	if (Tutorial::TutorialIsActive()) {
		Tutorial* t = Tutorial::GetTutorial();
		SInt16 pageNum = t->GetPageNum();
		if (pageNum == tutorialPage_SetRendezPt) {
			// we were waiting for them to click to locate the new rendezvous point
			t->NextPage();
		}
	}
  #endif // TUTORIAL_SUPPORT
	// end tutorial stuff
	return true;	// dialog prepared sucessfully
}

bool
CGet1StringDlgHndlr::CloseDialog(MessageT inMsg) {
	if (inMsg == 1) {	// okay button pushed, fill in the result string
		mResult->GetDescriptor(*mResultStr);
	}
	return true;	// it's okay to close the dialog
}

#pragma mark -


CThingyHelpAttach::CThingyHelpAttach(AGalacticThingy *inThingy) 
:CHelpAttach(0, 0) {
	switch (inThingy->GetThingySubClassType()) {
		case thingyType_Star:
			mStrId = STRx_StarHelp;
			break;
		case thingyType_Ship:
			mStrId = STRx_ShipHelp;
			break;
		case thingyType_Fleet:
			mStrId = STRx_FleetHelp;
			break;
		case thingyType_Wormhole:
			mStrId = STRx_WormholeHelp;
			break;
		case thingyType_StarLane:
			mStrId = STRx_StarlaneHelp;
			break;
		case thingyType_Rendezvous:
			mStrId = STRx_RendezvousHelp;
			break;
		default:
			mStrId = STRx_StarHelp;	// just so we don't cause crashes 
	}
}

void 
CThingyHelpAttach::ExecuteSelf(MessageT inMessage, void *ioParam) {
	// don't do flyby help balloons for items in the starmap
	if ((inMessage == msg_ShowHelp) && CBalloonApp::sMouseLingerActive) {
		return;
	} else {
		CHelpAttach::ExecuteSelf(inMessage, ioParam);
	}
}

void
CThingyHelpAttach::CalcPaneHotRect(LPane* inPane, Rect &outRect) { // where to show the balloon
	((AGalacticThingyUI*)inPane)->CalcPortHotSpotRect(outRect);
}

#if BALLOON_SUPPORT
void
CThingyHelpAttach::FillHMRecord(HMMessageRecord &theHelpMsg) {
	theHelpMsg.hmmHelpType = khmmString;
	LAttachable* host = GetOwnerHost();
	AGalacticThingyUI* aThingy = dynamic_cast<AGalacticThingyUI*>(host);
	if (!aThingy) {
		return;
	}
	if (aThingy->IsDead()) {
		return;
	}
	short index;
	if (!aThingy->IsOwned()) {
		index = str_unowned;
	}
	else if (aThingy->IsMyPlayers()) {
		index = str_playerOwned;
	} else {
		index = str_otherOwned;
	}
	LStr255 helpStr(mStrId, index);
	// now modify the string according to the state of the thingy.
	if (aThingy->IsHilited()) {
		index = str_hilited;
	} else if (aThingy->IsSelected()) {
		index = str_selected;
	} else {
		index = str_normal;
	}
	LStr255 s(mStrId, index);
	helpStr += s;
	LString::CopyPStr(helpStr, theHelpMsg.u.hmmString);
}
#endif // BALLOON_SUPPORT

bool
UPlaybackDelay::DoIdle(UInt32 currTick) {
	if (mSelectedThingy) {
		mSelectedThingy->UpdateSelectionMarker(currTick);
	}
	return false;	// returns true to abort delay
}


#if defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX )
StScrpHandle 
GetStScrpHandleForStyle(Style inStyle) {
   StScrpHandle h = (StScrpHandle) MacAPI::NewHandle(sizeof(short) + sizeof(ScrpSTElement));  // optimize to one entry
   if (h) {
      MacAPI::HLock((Handle)h);
      StScrpPtr p = *h;
      p->scrpNStyles = 1;
      p->scrpStyleTab[0].scrpStartChar = 0;     // need to find way to use existing values, we don't want to
      p->scrpStyleTab[0].scrpHeight = 12;        // change these
      p->scrpStyleTab[0].scrpAscent = 2;
      p->scrpStyleTab[0].scrpFont = gGalacticaFontNum;
      p->scrpStyleTab[0].scrpSize = 10;
      p->scrpStyleTab[0].scrpColor.red = 0;     // black is probably okay
      p->scrpStyleTab[0].scrpColor.green = 0;
      p->scrpStyleTab[0].scrpColor.blue = 0;
      p->scrpStyleTab[0].scrpFace = inStyle;    // this is the only one we want to change
      MacAPI::HUnlock((Handle)h);
   }
   return h;
}
#endif


// ========== DRAWING UTILITIES ==========


/* DrawHealthBar

	Draw a bar representing the "health" of an item. The bar has a green segment and 
	a red segment, with the relative portion of green to red representing the % health
	of the item.
 PARAMETERS:
	inCenterPt		- the location in the current grafPort where the bar is to be drawn
	inBarLength		- the horizontal lenght of the bar (it is always 2 pixels thick vertically)
	inPercentHealth	- a number from 0 to 100 that represents the health of the item as
						a percentage of the maximum health
*/
void 
DrawHealthBar(Point inCenterPt, SInt16 inBarLength, SInt16 inPercentHealth) {
	Point greenStart, greenEnd, redEnd;
	SInt16 greenLength = ((long)inPercentHealth * (long)inBarLength) / 100L;
	greenStart.h = inCenterPt.h - (inBarLength/2);
	greenStart.v = inCenterPt.v;
	greenEnd.h = greenStart.h + greenLength;
	greenEnd.v = inCenterPt.v;
	redEnd.h = greenStart.h + inBarLength;
	redEnd.v = inCenterPt.v;
	SetRGBForeColor(0x0000,0xFFFF,0x0000);	// green
	::MoveTo(greenStart.h, greenStart.v);
	::PenSize(1,2);
	::MacLineTo(greenEnd.h, greenEnd.v);
	if (greenEnd.h < redEnd.h) {
		SetRGBForeColor(0xFFFF,0x0000,0x0000);	// red
		::MacLineTo(redEnd.h, redEnd.v);
	}
	::PenSize(1,1);
}


// returns a float from 0 to 1.0 + inMinBrightness, based on the position of inCurrValue
// between inMinValue and inMaxValue. If inCurrValue < inMinValue, the result is
// 0. If inCurrValue > inMaxValue, the result is 1.0 + inMinBrightness
// The value is calculated and then clipped to inMinBrightness. ie: if
// the result would be 0.1, and inMinBrightness is 0.5, 0.5 is returned.
float 
CalcBrightnessFromRange(float inCurrValue, float inMinValue, float inMaxValue, float inMinBrightness) {
	ASSERT(inMinBrightness < 1.0);
	float result;
//	if ( (inMaxValue - inMinValue) > 1.0) {
		inCurrValue -= inMinValue;		// make lowest brightness relative to lowest value
		inMaxValue -= inMinValue;	// rather than relative to zero
//	}
	result = inMinBrightness + inCurrValue/inMaxValue;
	if (result < inMinBrightness) {
		result = inMinBrightness;	// enforce a minimum brightness
	}
	return result;
}
