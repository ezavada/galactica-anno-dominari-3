// GalacticaUtilsUI.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 5/6/96 ERZ

#ifndef GALACTICA_UTILS_UI_H_INCLUDED
#define GALACTICA_UTILS_UI_H_INCLUDED

#include "GalacticaConsts.h"
#include "GalacticaUtils.h"
#include "GenericUtilsUI.h" // UI based utilites
#include <Balloons.h>
#include <ostream>
#include <vector>

//=======================================================================

// uncomment the following line to use include the debugging cheats
// the current cheats are:
// 1) using a Galactic Density of 1 will make only 1 star for each player and one free star
//		and will give everyone tech level 200
// 2) All of Player 1's planets (except the home planet) will be given tech level 50

//#define DO_DEBUGGING_CHEATS

// ••• other debugging defines that are not specific to Galactica are found in GenericUtils.h

#include <LStream.h>
#include <LPeriodical.h>	// for use by the CTimer class
#include <LComparator.h>

// GUI only
#include <LPane.h>		// for the SMouseDownEvent type
#include <UDrawingState.h>  // for QDGlobals
#include <CHelpAttach.h>	// for use by the CThingyHelpAttach class

class AGalacticThingy;
class AGalacticThingyUI;
class CDialogAttachment;
class LWindow;
class LCommander;

AGalacticThingyUI*  ValidateThingyUI(const AGalacticThingy* inThingy);

// this function creates an PowerPlant LArray class that has the same
// contents as a std::vector<Waypoint>, as returned from CShip::GetWaypoints()
// It is used to populate PowerPlant LTable classes. The LTable should own
// the LArray, since otherwise it will be leaked
LArray* MakeArrayFromWaypointVector(const std::vector<Waypoint>& inWaypointVector);

// GUI ONLY

typedef struct MapClickT {
	SMouseDownEvent		*mouseEvent;
	Waypoint			clickedWP;
} MapClickT, *MapClickPtr;

extern short gGalacticaFontNum;


// dialog stuff

void MovableParamText(SInt16 number, LStr255 &inStr);	    // number must be 0..4
void MovableParamText(SInt16 number, const char* inCStr);	// number must be 0..4

long MovableAlert(ResIDT inWindowID, LCommander* inSuper = nil, 
					int inAutoCloseHalfSecs = 0, bool inBeep = true,
					CDialogAttachment* inAttachment = nil);

bool AskForOneValue(LCommander* inSuper, ResIDT inDialogID, PaneIDT inEntryPaneID,
					SInt32& ioNumber);

// Drawing stuff

void DrawHealthBar(Point inCenterPt, SInt16 inBarLength, SInt16 inPercentHealth);
float CalcBrightnessFromRange(float inCurrValue, float inMinValue, 
								float inMaxValue, float inMinBrightness = 0.0f);


// Utility Classes ===============================================================

// attachment class for use with MovableAlert()
class CDialogAttachment : public LAttachment {
public:
//				CDialogAttachment();
				CDialogAttachment(MessageT inMessage = msg_AnyMessage, bool inExecuteHost = true,
									bool inDoBeep = true);
	void			SetDialog(LWindow* inDialog) {mDialog = inDialog;}
	virtual bool	PrepareDialog();				// return false to abort display of dialog
	virtual bool	CloseDialog(MessageT inMsg);	// false to prohibit closing
	virtual bool	AutoDelete() {return false;}	// true to have the MovableModal delete attachment
	virtual void 	SetParamText(UInt8 inTextNum, LStr255 &ioNewString);	// changing ^0, etc…
	bool			DoSysBeep() {return mDoSysBeep;}
protected:
	LWindow			*mDialog;
	bool			mDoSysBeep;
};


class CGet1StringDlgHndlr : public CDialogAttachment {
public:
	CGet1StringDlgHndlr(LStr255 *inResultStr) : CDialogAttachment() {mResultStr = inResultStr;}
	virtual bool	PrepareDialog();
	virtual bool	CloseDialog(MessageT inMsg);	// false to prohibit closing
	virtual bool	AutoDelete() {return true;}		// MovableAlert() will delete this for us
protected:
	LPane* 		mResult;
	LStr255*	mResultStr;
};

class CThingyHelpAttach : public CHelpAttach {
public:
		CThingyHelpAttach(AGalacticThingy *inThingy);
protected:
	virtual void ExecuteSelf(MessageT inMessage,void *ioParam);
	virtual void CalcPaneHotRect(LPane* inPane, Rect &outRect);
#if BALLOON_SUPPORT
	virtual	void FillHMRecord(HMMessageRecord &theHelpMsg);
#endif
};



class UPlaybackDelay : public UDelay {
public:
	UPlaybackDelay(AGalacticThingyUI* inSelectedThingy) {mSelectedThingy = inSelectedThingy;}
	virtual ~UPlaybackDelay() {};
	virtual bool DoIdle(UInt32 currTick);
protected:
	AGalacticThingyUI* mSelectedThingy;
};

#if defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX )
   StScrpHandle  GetStScrpHandleForStyle(Style inStyle);
#endif


#endif // GALACTICA_UTILS_UI_H_INCLUDED

