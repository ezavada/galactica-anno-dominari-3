//	GalacticaDoc.h		
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#ifndef GALACTICA_DOC_H_INCLUDED
#define GALACTICA_DOC_H_INCLUDED

#include "GalacticaConsts.h"
#include "GalacticaSharedUI.h"
#include "GalacticaUtils.h"
#include "GenericUtilsUI.h"

#include <LWindow.h>
#include <LButton.h>
#include <LCaption.h>
#include "LPPobView.h"
#include "LSingleDoc.h"

enum {
	kDontPlayMessages = false,
	kPlayMessages = true
};

class CEventTable;
class CNewGWorld;
class CLayeredOffscreenView;
class CBoxView;
class LPicture;
class CResCaption;
class CLayeredOffscreenView;
class CMessage;

typedef class GalacticaWindow : public LWindow {
public:
		enum { class_ID = 'Gwin' };
		GalacticaWindow(LStream *inStream) : LWindow(inStream) {mStarMap = nil; mHelpLinePane = nil;}
	virtual void	DrawSelf();
	virtual void	ActivateSelf();
	virtual void	HandleClick(const EventRecord &inMacEvent, SInt16 inPart);
	void			SetStarMap(LView* inStarMap) {mStarMap = inStarMap;}
	void			SetHelpLinePane(LPane* inPane) {mHelpLinePane = inPane;}
	void 			UseFullScreen(Boolean inFull);
	virtual void	UpdatePort();

protected:
	LView*	mStarMap;
	LPane*	mHelpLinePane;
	static LAttachment *sFullScreenResizer;
} GalacticaWindow;

class CResCaption;

class GalacticaDoc : public LSingleDoc, public LPeriodical, public CTimed, virtual public GalacticaSharedUI {
public:
						GalacticaDoc(LCommander *inSuper);		// constructor
						~GalacticaDoc();	// stub destructor

// Informational Methods
	virtual bool        IsClient() const {return true;}
	Boolean				IsShowAllCoursesOn() const {return mShowAllCourses;}
	const char* 		GetMyPlayerName() {return mPlayerName.c_str();}
	PaneIDT				GetMyPlayerHomeID() {return mPlayerInfo.homeID;}
	virtual int		    GetMyPlayerNum() const = 0;

// UI methods
	CBoxView*			GetCurrButtonBar() const {return mButtonBar ? (CBoxView*)mButtonBar->GetActiveView() : nil;}
	virtual void		SetSelectedThingy(AGalacticThingyUI* inThingy);
	void		        ThingyAttendedTo(AGalacticThingy* inSatisfiedThingy);// del from needy list
	bool                FogOfWarOverride() const { return mFOWOverride;}    // true to override fog of war and draw everything
	bool				DrawNames() const {return mDrawNames;}
	bool				DrawStarLanes() const {return mDrawStarLanes;}
	bool				DrawShips() const {return mDrawShips;}
	bool				DrawGrid() const {return mDrawGrid;}
	bool				DrawRangeCircles() const {return mDrawRangeCircles;}
	bool				DrawCourierCourses() const {return mDrawCourierCourses;}
	bool				DrawStarmapNebulaBackground() {return mDrawStarmapNebulaBackground;}
	bool				WantsEnemyMoves() const {return mGotoEnemyMoves;}
	LWindow*            GetWindow() const {return mWindow;}
	CBoxView*			GetActivePanel() const {return mPanel ? (CBoxView*)mPanel->GetActiveView() : nil;}
	void                SetPanelCaption(LStr255 &s);
	RGBColor*			GetColor(int inOwner, Boolean inSelected);
	SInt32				GetColorIndex(int inOwner, Boolean inSelected);  // Added 970703 JRW
	long                GetHighest(int inWhich) const {return mHighest[inWhich];}
	long				GetLowest(int inWhich) const {return mLowest[inWhich];}
	LPicture*			GetDimmerPict() const {return mDimmerPict;}
	CResCaption*		GetEventText() const {return mEventText;}
	CLayeredOffscreenView*	GetViewer() const {return mViewer;}

	void				SetDrawCourierCourses(bool inDraw) { mDrawCourierCourses = inDraw; }
	void				SetDrawAllCourses(bool inDraw) { mShowAllCourses = inDraw; }
	void				SetDrawRangeCircles(bool inDraw) { mDrawRangeCircles = inDraw; }
	void				SetDrawNames(bool inDraw) { mDrawNames = inDraw; }
	void				SetDrawGrid(bool inDraw) { mDrawGrid = inDraw; }
	void				SetDrawStarLanes(bool inDraw) { mDrawStarLanes = inDraw; }
	void				SetDrawShips(bool inDraw) { mDrawShips = inDraw; }
	void				SetDrawStarmapNebulaBackground(bool inDraw) { mDrawStarmapNebulaBackground = inDraw; }
	virtual void		UserChangedSettings();

	static GalacticaDoc*	GetStreamingDoc() {return sStreamingDoc;}
	

// File Management Methods
	virtual void		DoAESave(FSSpec &inFileSpec, OSType inFileType);
	void				Unspecify() {mIsSpecified = false;}	// removes the association between a file and the document

// User Interface Methods
	void 		        HideSelectionMarker();
	virtual Boolean	    ObeyCommand(CommandT inCommand, void* ioParam);	
	virtual void		FindCommandStatus(CommandT inCommand,
							Boolean &outEnabled, Boolean &outUsesMark,
							UInt16 &outMark, Str255 outName);
	virtual	Boolean	    AllowSubRemoval(LCommander* inSub);
	virtual Boolean		HandleKeyPress( const EventRecord& inKeyEvent );
//	virtual void		AttemptClose(Boolean inRecordIt);
//	virtual Boolean	    AttemptQuitSelf(SInt32 inSaveOption);
	virtual void		DoPrint();
	virtual void		MakeWindow();
	virtual void		Close();
	void				WindowActivated();
	void				ShowEventsWindow();
	void				MarkItemHandledInNeedyList(ArrayIndexT inWhichItem);
	ArrayIndexT			FindThingyInNeedyList(AGalacticThingy* inThingy, ThingMemInfoT *outInfo);
	void				SelectThingyInEventsWindow(AGalacticThingy* inThingy);
	void				ShowMessageEventText(CMessage* inMessage);
	void				DoComparePlayers();
	void                SortCompareWindowByColumn(int columnNum);
	void                CloseSystemsWindow();
	void                ShowSystemsWindow();
	void                SortSystemsWindowByColumn(int columnNum);
	void                CloseFleetsWindow();
	void                ShowFleetsWindow();
	void                SortFleetsWindowByColumn(int columnNum);
	void				AdjustWindowSize();
	void				ShowWindow(bool inPlayMessages);
	virtual void		SetStatusMessage(int inMessageNum, const LStr255 &inExtra);
	void				ShowMessages();
	bool				SetWantsMessages(bool inWantThem) { bool result = mWantGameMessages; 
															mWantGameMessages = inWantThem; 
															return result;
														  }
	void				   ShowNextNeedyThingy(bool inSelectHomeIfNothing = false);
	void				   SwitchToButtonBar(long inThingyCode);
	void				   UpdateTurnNumberDisplay();
	void				   RebuildEventMessages();

// Turn Processing Methods
	virtual	void		StartIdling();
	virtual	void		StopIdling();
	virtual	void		SpendTime(const EventRecord &inMacEvent);
	virtual void		DoEndTurn() = 0;
	virtual void		TimeIsUp();	// from CTimed, called by CTimer
	void				StopAllMouseTracking();
	long				GetSecondsRemainingInTurn();
	void				AddMessageToNeedyList(CMessage* inMsg);
	void				YieldTime();

protected:
   PrivatePlayerInfoT mPlayerInfo;     // for this player only
	UInt32			mNextSpendTime;
	CTimer			mAutoEndTurnTimer;
	std::string		mPlayerName;
	SInt32          mLastTurnPosted;    // only used in multiplayer, see if we can move it
	Bool8		    mDisallowCommands;
	LButton*		mEndTurnButton;
	bool            mFOWOverride;       // draw everything, regardless of fog of war
	bool  			    mDrawNames;
	bool			    mDrawStarLanes;
	bool			    mDrawShips;
	bool			    mDrawGrid;
	bool				mDrawStarmapNebulaBackground;
	bool			    mDrawRangeCircles;
	bool			    mDrawCourierCourses;
	bool			    mWantGameMessages;
	bool			    mWantPlayerMessages;
	bool			    mGotoEnemyMoves;
	bool			    mShowAllCourses;
	bool			    mIsIdling;
	bool			    mEventsWereShown;
	bool			    mShowMessagesPending;	// need to call ShowMessages() when window activated
	SInt16			    mNumUnprocessedGotos;
	SInt16			    mNumUnprocessedAutoShows;
	RGBColor		    mColorArray[kNumUniquePlayerColors];
	RGBColor		    mSelectedColorArray[kNumUniquePlayerColors];
	AGalacticThingyUI*	mOldSelectedThingy;	// used to tell ShowMessages() what to reselect
	LCaption*		    mPanelCaption;
	LPPobView*		    mButtonBar;
	LPPobView*		    mPanel;
	LCaption*		    mViewNameCaption;
	CLayeredOffscreenView*	mViewer;	// shows detailed graphic views of ships, stars, etc..
	LPicture*		    mDimmerPict;
	CResCaption*	    mEventText;
	SInt32			    mLastStarIndex;	// index of last star in Everything list
	LArray*			    mNewOrChangedClientThingys;	// list of new things created by client
	LArray*			    mNeedy;			// holds the list of items that need attention
	LArrayIterator*	    mNeedyIterator;	// keeps track of our current position in the needy list
	CEventTable*	    mEventsTable;
	LWindow*		    mEventsWindow;	// this particular document's events window
	static LWindow*	    sEventsWindow;	// current active events window
	LWindow*		    mCompareWindow;  // Store a reference to the Compare Players
	LWindow*            mSystemsWindow;   // Stores a reference to the Star Systems Window
	LWindow*            mFleetsWindow;   // Stores a reference to the Fleets Window
	LArray*			    mPlayerNames;		// holds the list of all the player names
	static GalacticaDoc*	sStreamingDoc;
	long			    mHighest[displayMode_NumOfModes];
	long			    mLowest[displayMode_NumOfModes];
	long			    mHighestSpending[spending_Last + 1];
	long			    mLowestSpending[spending_Last + 1];
	short			    mTurnsBetweenCallins;
	UControlCommandListener     mControlListener; 
	std::vector<CShip*> mFleetsList;        // only valid while a fleet compare window is open
	std::vector<CStar*> mSystemsList;       // only valid while systems compare window is open
	int                 mFleetsSortColumn;   // holds column fleets window is sorted by
	int                 mSystemsSortColumn; // holds column systems window is sorted by
	int                 mFleetsScrollPos;
	int                 mSystemsScrollPos;
	void	RecalcHighestValues();
private:
	UInt32			mNextCursorFlash;
};

#endif // GALACTICA_DOC_H_INCLUDED

