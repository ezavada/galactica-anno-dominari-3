// Galactica Panes.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ
// 9/21/96  1.0.8 fixes incorporated

#ifndef GALACTICA_PANES_H_INCLUDED
#define GALACTICA_PANES_H_INCLUDED

#include "GalacticaUtils.h"

#include <LPane.h>
#include <LView.h>
#include <LWindow.h>
#include <LPeriodical.h>
#include <LListener.h>
#include <LToggleButton.h>
#include <LString.h>
#include <LButton.h>
#include <LArrayIterator.h>
#include <LCaption.h>
#include <UDrawingState.h>
#include <PP_Messages.h>

#include "CZoomView.h"
#include "AGalacticThingyUI.h"
#include "CMouseTracker.h"

class GalacticaSharedUI;

typedef class CNewToggleButton : public LToggleButton {
public:
	enum { class_ID = 'Ntbt' };
	
			CNewToggleButton(LStream *inStream);
			
	virtual void 	SetValue(SInt32 inValue);
	virtual void	DrawGraphic(ResIDT inGraphicID);
	virtual void	HotSpotResult(SInt16 inHotSpot);
	virtual void	SimulateHotSpotClick(SInt16 inHotSpot);
} CNewToggleButton, *CNewToggleButtonPtr, **CNewToggleButtonHnd, **CNewToggleButtonHdl;


typedef class CNewButton : public LButton {
public:
	enum { class_ID = 'Nwbt' };
	
			CNewButton(LStream *inStream);
			
	virtual void	DrawGraphic(ResIDT inGraphicID);
} CNewButton, *CNewButtonPtr, **CNewButtonHnd, **CNewButtonHdl;


class CCoursePlotter;

enum EClickTypes {
	clickType_None,
	clickType_Waypoint,
	clickType_Rendezvous
};

class CStarMapView : public CZoomView, public LListener {
public:
	enum { class_ID = 'SMvw' };
	static void			ClickPending(EClickTypes inType) {sAwaitingClick = true; sClickType = inType;}
	static bool  		IsAwaitingClick() {return sAwaitingClick;}
	static void			ClickReceived();
	static EClickTypes	GetClickType() {return sClickType;}

	
			CStarMapView();		
			CStarMapView(const SPaneInfo &inPaneInfo, const SViewInfo &inViewInfo);
			CStarMapView(LStream *inStream);

	void			InitStarMap();
	void			SetSectorSize(SInt32 inSize);
	SInt32			GetSectorSize() const {return mSectorSize;}
//	void			CalcSectorBounds(Point3D &topLeftFront, Point3D &botRightRear); // galactic coords
//	void			CreateStars(SInt32 inNumStars);
	bool			ReadStream(LStream *inStream, UInt32 checkVersion);
	void			FinishStreaming();	// use after all ReadStream() are complete
	void			WriteStream(LStream *inStream);
	void			ScrollToThingy(AGalacticThingy *inThingy);
	virtual void	ListenToMessage(MessageT inMessage, void *ioParam);
	virtual void	Click(SMouseDownEvent &inMouseDown);
	int				GetNumStars() const {return mNumStars;}
	virtual void	AdjustMouse(Point inPortPt, const EventRecord &inMacEvent, RgnHandle outMouseRgn);
	virtual void	AdjustMouseSelf(Point inPortPt, const EventRecord& inMacEvent, RgnHandle outMouseRgn);
	virtual void 	Draw(RgnHandle inSuperDrawRgnH);
	virtual void 	DrawSelf();
	virtual void	ScrollBits(SInt32 inLeftDelta, SInt32 inTopDelta);
	void 			EndOfTurn(long inTurnNum);
	long			GetDisplayMode() const {return mDisplayMode;}
	void			SetDisplayMode(long inMode) {mDisplayMode = inMode;}
	GalacticaSharedUI*	GetGame() const {return mGame;}
	void			SetGame(GalacticaSharedUI* inGame) {mGame = inGame;}	// called by GalacticaDoc::MakeWindow()
	void			GalacticToImageCoord(Point3D &ioPt);
	void			GalacticToImagePt(Point &ioPt);
	void			PortToGalacticaCoord(Point inPortPt, Point3D &outPt);
	void			ImageToGalacticaCoord(SPoint32 inImagePt, Point3D &outPt);
	virtual void	ZoomSubPane(LPane *inSub, float deltaZoom);
	virtual void	ZoomTo(float inZoom, Boolean inRefresh = true);
	void			ResolveOverlappingPanes(LPane** ioClickedPaneP, StdThingyUIList* inPopupList);
	void			ResolveOverlappingPanesFromPoint(Point &clickPt, StdThingyUIList* inPopupList, LPane** ioClickedPaneP);
//	void			SetPlotter(CCoursePlotter* inPlotter) {mPlotter = inPlotter;}
//	static void		ClickMissedBy(SInt16 inMissDist) {if (inMissDist < sClosestDist) sClosestDist = inMissDist;}
	static Cursor	sArrowCursor;
protected:
	SInt32			mNumStars;
	SInt32			mSectorSize;
	long			mDisplayMode;
	GalacticaSharedUI*	mGame;
//	CCoursePlotter* mPlotter;
	static bool 	sCursorLoaded;
	static bool 	sAwaitingClick;
	static EClickTypes		sClickType;		// what type of click are we waiting for?
	static Cursor	sCrossHairCursor;
	static Cursor	sHandCursor;
	static Cursor	sXCursor;
	static Cursor	sMenuHandCursor;	// for a properties menu
	static bool		sBackgroundLoaded;
	static PicHandle sBackgroundImage; 
};


class UStarPopupItemCountAction : public UArrayAction {
protected:
	virtual bool DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData);
public:
	UStarPopupItemCountAction(PaneIDT inIgnoreID = PaneIDT_Undefined) {
		mIgnoreID = inIgnoreID;
		mItems = nil;
	}
	virtual ~UStarPopupItemCountAction() {if (mItems) { delete mItems; mItems = nil;} }
	StdThingyUIList* GetStarPopupItems() {return mItems;}
protected:
	PaneIDT		            mIgnoreID;
	StdThingyUIList*		mItems;
};



typedef class CCoursePlotter : public CMouseTracker {
public:
			CCoursePlotter(CStarMapView* inStarMap, Waypoint inFromWP, long inSpeed);
			~CCoursePlotter();
	virtual void	AdjustZoomed();
	virtual void	PrepareToDraw();			// override to set up pen mode, etc...
	virtual void	DrawSelf(Point inWherePt);
	virtual void	EraseSelf(Point inWherePt);
protected:
	Waypoint	mFromWP;
	SPoint32 	mFromPt;
	long		mSpeed;
	long		mAdjustedSpeed;
	long		mLastTurns;
	short		mLastStrWidth;
	LStr255		mLastTurnsStr;
} CCoursePlotter, *CCoursePlotterPtr, **CCoursePlotterHnd, **CCoursePlotterHdl;

#endif // GALACTICA_PANES_H_INCLUDED

