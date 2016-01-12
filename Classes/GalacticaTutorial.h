// Galactica Tutorial.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 11/29/97 ERZ
// completely rewritten, 9/27/99, ERZ

#include <LWindow.h>
#include <LTextEditView.h>
#include <LControl.h>
#include <LListener.h>
#include <LPicture.h>
#include <LPeriodical.h>

#ifndef NO_GALACTICA_DEPENDENCIES
#include <CStyledText.h>
#include <GalacticaConsts.h>
#include <DebugMacros.h>

const SInt32 kTutorialUserCon = 70007000;

// ===== Galactica specific page numbers, corresponding to 'page' resources =====

#define tutorialPage_Welcome		1
#define tutorialPage_ChooseNewGame	2
#define tutorialPage_GameSetup		3
#define tutorialPage_Login			4
#define tutorialPage_GameWindIntro	5
#define tutorialPage_StarmapIntro	6
#define tutorialPage_StarmapMove	7
#define tutorialPage_YourStarSys	8
#define tutorialPage_SystemProduct	9
#define tutorialPage_ReduceShipbld	10
#define tutorialPage_SliderColor	11
#define tutorialPage_EndTurnOne		12
#define tutorialPage_NewColonyShip	13
#define tutorialPage_EnemyMove		14
#define tutorialPage_MovingShips1	15
#define tutorialPage_MovingShips2	16
#define tutorialPage_MovingShips3	17
#define tutorialPage_MovingShipsOpps	18
#define tutorialPage_BuildSatellite	19

#define tutorialPage_SetSliderEqual	20
#define tutorialPage_EndTurnTwo		21
#define tutorialPage_EndTurnThree	22
#define tutorialPage_NewSatellite	23
#define tutorialPage_NewColony		24
#define tutorialPage_RenameColony	25
#define tutorialPage_BuildBattleshp	26
#define tutorialPage_HitTab			27
#define tutorialPage_ShipsOrders	28
#define tutorialPage_HitTabAgain	29
#define tutorialPage_SelectHome		30
#define tutorialPage_BuildCouriers	31
#define tutorialPage_NoticeRing		32
#define tutorialPage_NewCourier		33
#define tutorialPage_CourierOrders	34
#define tutorialPage_HitTab_2		35
#define tutorialPage_SelectHome2	36
#define tutorialPage_BuildBattlshp2	37
#define tutorialPage_DoNewRendez	38
#define tutorialPage_SetRendezPt	39
#define tutorialPage_NameAlpha		40
#define tutorialPage_ClickSendTo	41
#define tutorialPage_NTerraAutoDst	42
#define tutorialPage_EndTurnFive	43
#define tutorialPage_CourierArrive	44
#define tutorialPage_EndTurnSix		45
#define tutorialPage_EnemyMove2		46
#define tutorialPage_AnotherColony	47
#define tutorialPage_AwaitingOrders	48
#define tutorialPage_PatrolHitTab	49
#define tutorialPage_DoCompPlayers	50
#define tutorialPage_CompPlayers	51
#define tutorialPage_EndTurnSeven	52
#define tutorialPage_IgnoreCourier	53
#define tutorialPage_EndTurnEight	54
#define tutorialPage_NewTraveler	55
#define tutorialPage_FleetBuilt		56
#define tutorialPage_ClickShipsButn	57
#define tutorialPage_ShipRemoval	58
#define tutorialPage_HealthBar		59
#define tutorialPage_EndOfTutorial	60

#define tutorialPage_LastPage		tutorialPage_EndOfTutorial

#define tutorialPage_HidingGame 	100
#define tutorialPage_CloseModal 	101
#define tutorialPage_StickToProgram	102
#define tutorialPage_NoMultiplayerTutorial 103

#define tutorial_HomeStarID			2L
#define tutorial_NewTerraStarID		9L	// Pane ID of star 20 in our tutorial saved game
#define tutorial_S7StarID			7L	// Pane ID of star 7 in our tutorial saved game
#define tutorial_ColonyShipID		43L	// pane ID of player's first Colony ship
#define tutorial_CourierShipID		58L	// pane ID of player's first Courier ship
#define tutorial_RendezvousAlphaID	61L	// Pane ID of Rendezvous Alpha
#define tutorial_CompPlayerAnchorID	2L	// Pane ID of an invisible anchor pane in the Player Comparison


#endif NO_GALACTICA_DEPENDENCIES

#ifndef TUTORIAL_SUPPORT
    #if TARGET_OS_MAC
        #define TUTORIAL_SUPPORT 1
    #else
        #define TUTORIAL_SUPPORT 0
    #endif
#endif

#if TUTORIAL_SUPPORT
// ===== internal definition for structure of a 'page' resource =====

// these must be packed structures
#ifndef PACKED_STRUCT
#define PACKED_STRUCT
#if defined( __MWERKS__ )
    #pragma options align=mac68k
#elif defined( _MSC_VER )
    #pragma pack(push, 2)
#elif defined( __GNUC__ )
	#undef PACKED_STRUCT
	#define PACKED_STRUCT __attribute__((__packed__))
#else
	#pragma pack()
#endif
#endif

typedef struct PageExceptionT {
	union {
		PaneIDT	frontWindID;
		PaneIDT	visiblePaneID;
		PaneIDT enabledPaneID;
		PaneIDT targetPaneID;
	};
	ResIDT	gotoPage;
} PACKED_STRUCT PageExceptionT;

enum {
	pageVarient_Standard 		= 0,
	pageVarient_BindToPaneID	= 1,
#ifndef NO_GALACTICA_DEPENDENCIES
	pageVarient_BindToPaneInStarmap	= 2
#endif // NO_GALACTICA_DEPENDENCIES

//	pageVarient_ExceptVisiblePaneID	= 2,	// we are checking for a pane being visible in the front window 
//	pageVarient_ExceptEnabledPaneID	= 3,	// checking for a pane being enabled in the front window 
//	pageVarient_ExceptTargetPaneID	= 4		// checking for a pane being the current target
};

typedef struct pageResT {
	SInt16	variant;
	SInt16	pageHeight;
	SInt16	pageWidth;
	ResIDT	titleSTRxID;
	SInt16	titleSTRxIndex;
	ResIDT	mainTextID;
	ResIDT	buttonSTRxID;
	SInt16	buttonSTRxIndex;
	ResIDT	pictID;
	Rect	pictLocation;
	ResIDT	sndID;
	PaneIDT	bindToWindowID;
	Boolean	positionAtTop;
	char	_unused1;
	Boolean	positionAtLeft;
	char	_unused2;
	Boolean	positionAtBottom;
	char	_unused3;
	Boolean	positionAtRight;
	char	_unused4;
	ResIDT	nextPage;
	SInt16	exceptionCount;
	PageExceptionT	exceptions[];
} PACKED_STRUCT pageResT, *pageResPtr, **pageResHnd;

#if defined( __MWERKS__ )
    #pragma options align=reset
#elif defined( _MSC_VER )
    #pragma pack(pop)
#endif

class Tutorial : public LListener {
public:
	static Tutorial*	GetTutorial();
	static bool			TutorialIsActive() {return sActive;}
	void	StartTutorial(LCommander* inSuperCommander, SInt16 inAtPageNum = 1);
	void	GotoPage(SInt16 inPageNum) {LoadPage(inPageNum);}
	void	NextPage();
	void	ForceNextPageToBe(SInt16 inNewNextPage) {
				mNextPageNum = inNewNextPage;
				#ifndef NO_GALACTICA_DEPENDENCIES
				 DEBUG_OUT("Force next page to "<<mNextPageNum, DEBUG_DETAIL | DEBUG_TUTORIAL);
				#endif
				}
	SInt16	GetPageNum() {return mPageNum;}
	void	StopTutorial();
	void	ReassignControlTo(LCommander* inNewSuperCommander) {mSuper = inNewSuperCommander;}
	LWindow* GetTutorialWindow() {return mWindow;}
	static FSSpec&	GetTutorialFileSpec() {return sFSSpec;}
	virtual void	ListenToMessage(MessageT inMessage, void* ioParam);
protected:
	Tutorial();
	void	LoadPage(SInt16 inPageNum);
// member variables
	static	Tutorial*	sTutorial;
	static	bool		sActive;
	static	FSSpec		sFSSpec;
	short			mRefNum;
	LCommander* 	mSuper;
	LWindow* 		mWindow;
	LControl*		mButton;
	LTextEditView*	mText;
	LPicture*		mPicture;
	LPane*			mTitle;
	LPane*			mTitleBackground;
	SInt16 			mPageNum;
	SInt16 			mNextPageNum;
	Handle			mCurrPageData;
	SInt16			mWindStructExtraHeight;
	SInt16			mWindStructExtraWidth;
};



// Doing:
//   new TutorialWaiter(secondsToWait, pageToGotoWhenTimeIsUp);
// will automatically go to a particular page in the tutorial when the time expires
// and delete itself. Passing in -1 for the page number will just go to the next
// page in the sequence
// If something changes and you want to cancel the timer, do:
//   TutorialWaiter::StopTutorialWaiter();
// Or perhaps you want to give the user more time:
//   TutorialWaiter::ResetTutorialWaiter();
// It is possible to have multiple tutorialWaiters running simultaneously, but then
// you will need to keep track of them yourself rather than using the convenient
// static singleton calls.
class TutorialWaiter : public LPeriodical {
public:
	static TutorialWaiter*	GetTutorialWaiter() {return sTutorialWaiter;}
	static void				StopTutorialWaiter() {if (sTutorialWaiter) delete sTutorialWaiter;}
	static void				ResetTutorialWaiter() {if (sTutorialWaiter) sTutorialWaiter->Reset();}
	TutorialWaiter(SInt16 inSecondsToWait, SInt16 timeUpPageNum = -1, bool autoDelete = true) {
						mTimeUpDelay = inSecondsToWait*60; mPageNum = timeUpPageNum;
						mAutoDelete = autoDelete; sTutorialWaiter = this; Reset(); StartIdling();}
	~TutorialWaiter() {if (sTutorialWaiter == this) sTutorialWaiter = NULL;}
	void Reset() { mTimeUpTicks = ::TickCount() + mTimeUpDelay;}
	virtual void		SpendTime( const EventRecord& inMacEvent);
protected:
	static	TutorialWaiter*	sTutorialWaiter;
	UInt32	mTimeUpTicks;
	UInt32	mTimeUpDelay;
	bool	mAutoDelete;
	SInt16	mPageNum;
};

#endif // TUTORIAL_SUPPORT

