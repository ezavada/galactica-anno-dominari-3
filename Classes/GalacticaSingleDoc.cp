//	GalacticaSingleDoc.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================


/* 

FUNCTIONAL OVERVIEW:

The GalacticaSingleDoc is the central coordinator of the entire single player 
game. It handles all functions that affect the game as a whole, such as 
preferences, saving, and host client communication. The GalacticaSingleDoc
is a combination of a GalacticaDoc (which handles game UI stuff), and a 
GalacticaProcessor (which has functions for End Of Turn processing and 
other host specific functions).

There are several major catagories of methods and data in a GameDoc, reflecting 
the basic functional areas:

1. Informational: These report/change game and player information, for use in
		both game processing functions and/or for display to the user. They
		generally do little or no actual processing of information.

2. Communication: There are none of these in a GalacticaSingleDoc, since the
        host and client are the same object

3. File Management: These handle saving and restoring Saved Turn files.

4. User Interface: These handle display of game specific information, and
		coordinate the display of thingy (ship, star, etc...) specific info.
		They are generally called during the user's turn in response to specific
		events (user actions). They are the second most complicated set of
		methods in a GameDoc.

5. Turn Processing: does all the creation and deletion of objects as a result
        of movements set during the turn. 


Event Messages Overview
1. Turn Processing: The GalacticaSingleDoc acts as both the host and the 
        client. The basic process is:
		a. User makes moves
		b. User ends turn. 						GalacticaSingleDoc::DoEndTurn()
		c. Host processes EOT, creates event	GalacticaSingleDoc::DoEndTurn()
			messages as needed.									
		
2. User Interface aspects: The event report window has a list of all events,
	and the list handles most of the details of displaying them.

*/

#include "GalacticaGlobals.h"
#include "GalacticaConsts.h"
#include "GalacticaSingleDoc.h"
#include "GalacticaKeys.h"
#include "GalacticaPanels.h"
#include "CGalacticSlider.h"
#include "GalacticaTables.h"
#include "CStar.h"
#include "GalacticaUtils.h"
#include "CShip.h"
#include "CRendezvous.h"
#include "GalacticaTutorial.h"
#include <PP_Messages.h>
#include <PP_Resources.h>
//#include <PPobClasses.h>
#include <UDrawingState.h>
#include <LArrayIterator.h>
#include <LEditField.h>
#include <LWindow.h>
#include <UModalDialogs.h>
#include <LApplication.h>

#include <LowMem.h>

#include "CLayeredOffscreenView.h"
#include "CMasterIndexFile.h"
#include "CMessage.h"
#include "CNewGWorld.h"
#include "CResCaption.h"
#include "CStyledText.h"
#include "CTextTable.h"
#include "CVarDataFile.h"
#include "CWindowMenu.h"
#include "LAnimateCursor.h"
#include "LPPobView.h"
#include "Slider.h"
#include "USound.h"

#include "pdg/sys/config.h"

#if TARGET_OS_MAC && !TARGET_API_MAC_CARBON
	#include <Profiler.h>	// for profiler information
#endif

#if TARGET_OS_MAC && !TARGET_API_MAC_CARBON
	#include "CDisplay.h"
#endif

#if TARGET_OS_MAC && defined(DEBUG) && __MWERKS__
	#include "DebugNew.h"
#endif

//#define PROFILE_EOT


#define snd_Attention	1001


GalacticaSingleDoc::GalacticaSingleDoc(LCommander *inSuper)
: GalacticaDoc(inSuper) {
	mLastThingyID = 100;    // start at 100 on a new map
	DEBUG_OUT("New GalacticaSingleDoc", DEBUG_IMPORTANT | DEBUG_USER);
}

GalacticaSingleDoc::~GalacticaSingleDoc() {
	DEBUG_OUT("GalacticaSingleDoc::~GalacticaSingleDoc", DEBUG_IMPORTANT);
}


void
GalacticaSingleDoc::FindCommandStatus(CommandT inCommand, Boolean &outEnabled, Boolean &outUsesMark, 
								UInt16 &outMark, Str255 outName) {
	switch (inCommand) {
		case cmd_SetPopulation:
		case cmd_SetTechLevel:
		case cmd_SetOwner:
		case cmd_NewShipAtStar:
			outEnabled = false;
			outUsesMark = false;
			outMark = ' ';
			if (mSelectedThingy != nil) {
				if (mSelectedThingy->GetThingySubClassType() == thingyType_Star) {
					outEnabled = (GetTurnNum() == 1);
				}
			}
			break;
		default:
			GalacticaDoc::FindCommandStatus(inCommand, outEnabled, outUsesMark, outMark, outName);
			break;
	}
}


Boolean
GalacticaSingleDoc::ObeyCommand(CommandT inCommand, void *ioParam) {
	switch (inCommand) {
		case cmd_SetPopulation:
        {
			CStar* theStar = (CStar*)mSelectedThingy;
			SInt32 pop = theStar->GetPopulation();
			if (UModalDialogs::AskForOneNumber(this, window_SetPopulation, 100, pop)) {
				theStar->SetPopulation(pop);
				theStar->Refresh();
				CBoxView* activePanel = GetActivePanel();
				if (activePanel) {
					activePanel->UpdatePanel(mSelectedThingy);
				}
			}
			break;
        }
		case cmd_SetTechLevel:
        {
			CStar* theStar = dynamic_cast<CStar*>(mSelectedThingy);
			SInt32 level = theStar->GetTechLevel();
			if (UModalDialogs::AskForOneNumber(this, window_SetTechLevel, 100, level)) {
				theStar->SetTechLevel(level);
    			theStar->SetScannerRange(UThingyClassInfo::GetScanRangeForTechLevel(level, IsFastGame(), HasFogOfWar()));
                theStar->ScannerChanged();
                theStar->UpdateWhatIsVisibleToMe();
				theStar->CalcPosition(kRefresh);
				CBoxView* activePanel = GetActivePanel();
				if (activePanel) {
					activePanel->UpdatePanel(mSelectedThingy);
				}
			}
			break;
		}
        case cmd_SetOwner:
		{
			CStar* theStar = dynamic_cast<CStar*>(mSelectedThingy);
			SInt32 owner = theStar->GetOwner();
			if (UModalDialogs::AskForOneNumber(this, window_SetOwner, 100, owner)) {
				if (owner > mGameInfo.totalNumPlayers) {
					SysBeep(1);
				} else {
					theStar->SetOwner(owner);
					theStar->Refresh();
 					CBoxView* activePanel = GetActivePanel();
					if (activePanel) {
						activePanel->UpdatePanel(mSelectedThingy);
					}
				}
			}
			break;
		}
        case cmd_NewShipAtStar:
		{
			CStar* theStar = dynamic_cast<CStar*>(mSelectedThingy);
			SInt32 hull = theStar->GetBuildHullType() + 1;
			if (AskForOneValue(this, window_ChooseNewShipType, 100, hull)) {
				theStar->BuildNewShip((EShipClass)(hull - 1));		
				theStar->Refresh();
				CBoxView* activePanel = GetActivePanel();
				if (activePanel) {
					activePanel->UpdatePanel(mSelectedThingy);
				}
			}
			break;
        }
		default:
		    return GalacticaDoc::ObeyCommand(inCommand, ioParam);
		    break;
	}
	return true;
}


void
GalacticaSingleDoc::UserChangedSettings() {
	GalacticaDoc::UserChangedSettings();
	// when the user changes settings, do whatever is necessary to adapt to the new settings
	GetPrivatePlayerInfo(GetMyPlayerNum(), mPlayerInfo);
	// for this client, assign from config
	pdg::ConfigManager* config = Galactica::Globals::GetInstance().getConfigManager();
	long tempGrowth, tempTech, tempShips, tempLong;
	if (config->getConfigLong(GALACTICA_PREF_DEFAULT_BUILD, tempLong)) {
		mPlayerInfo.defaultBuildType = tempLong;
	} else {
		mPlayerInfo.defaultBuildType = 1;
	}
	bool assigned = config->getConfigLong(GALACTICA_PREF_DEFAULT_GROWTH, tempGrowth);
	assigned &= config->getConfigLong(GALACTICA_PREF_DEFAULT_TECH, tempTech);
	assigned &= config->getConfigLong(GALACTICA_PREF_DEFAULT_SHIPS, tempShips);
    if (!assigned) {
    	// not all of them were assigned, so use the defaults
    	tempGrowth = 334;
    	tempTech = 333;
    	tempShips = 333;
   	}
	// they all have to be assigned or we don't want any of them
	mPlayerInfo.defaultGrowth = tempGrowth;
	mPlayerInfo.defaultTech = tempTech;
	mPlayerInfo.defaultShips = tempShips;
	// for other players (or if no config values available), use defaults
	SetPrivatePlayerInfo(GetMyPlayerNum(), mPlayerInfo);
}

OSErr
GalacticaSingleDoc::Open(FSSpec &inFileSpec) {
    OSErr result = noErr;
	DEBUG_OUT("SingleDoc::Open", DEBUG_IMPORTANT);
    try {
    	SpecifyFile(inFileSpec);
    	MakeWindow();
    	ReadSavedTurnFile();
		ShowWindow(kDontPlayMessages);
    }
    catch(LException &e) {
        DEBUG_OUT("PP Exception "<<e.what()<<" trying to open saved game file", DEBUG_ERROR);
        delete this;
        result = e.GetErrorCode();
    }
    catch(std::exception &e) {
        DEBUG_OUT("std::exception "<<e.what()<<" trying to open saved game file", DEBUG_ERROR);
        delete this;
        result = -1;
    }
	return result;
}

OSErr
GalacticaSingleDoc::DoNewGame(NewGameInfoT &inGameInfo, std::string& inPlayerName) {
    DEBUG_OUT("GalacticaSingleDoc::DoNewGame", DEBUG_IMPORTANT);
    OSErr result = noErr;
    try {
    	CreateNew(inGameInfo);
    	MakeWindow();
      #if TUTORIAL_SUPPORT
    	if (Tutorial::TutorialIsActive()) {
    		// v1.2fc7, read tutorial game from tutorial file's data fork
    		FSSpec theTutorialFile = Tutorial::GetTutorialFileSpec();
    		SpecifyFile(theTutorialFile);
    		ReadSavedTurnFile();
     		Unspecify();	// remove file association
    		// seed the random number generator with a particular value
    		// so we get the same sequence of psuedo-random numbers each time the
    		// tutorial is run
    		UQDGlobals::SetRandomSeed(101);
    	} 
    	else
      #endif // TUTORIAL_SUPPORT
    	{
    		// not a tutorial game, just create new data
    		InitNewGameData(inGameInfo);
    	}
        // make sure the player's name is correct
        PublicPlayerInfoT thePlayer;
        GetPlayerInfo(1, thePlayer);
        thePlayer.isAlive = true;
        thePlayer.isLoggedIn = true;
        std::strcpy(thePlayer.name, inPlayerName.c_str());
        SetPlayerInfo(1, thePlayer);
    	ShowWindow(kPlayMessages);
	}
    catch(LException &e) {
        DEBUG_OUT("PP Exception "<<e.what()<<" trying to create new game", DEBUG_ERROR);
        result = e.GetErrorCode();
    }
    catch(std::exception &e) {
        DEBUG_OUT("std::exception "<<e.what()<<" trying to create new game", DEBUG_ERROR);
        result = -1;
    }
    return result;
}


void
GalacticaSingleDoc::StartIdling() {
	LPeriodical::StartIdling();
	mIsIdling = true;
}

void
GalacticaSingleDoc::StopIdling() {
	LPeriodical::StopIdling();
	mIsIdling = false;
}

void
GalacticaSingleDoc::SpendTime(const EventRecord &) {
	// ====================== Repeated Constantly ============================

	UInt32 ticks = ::TickCount();
	if (mSelectedThingy) {		// if something is selected
		mSelectedThingy->UpdateSelectionMarker(ticks);
	}
}


void
GalacticaSingleDoc::SpecifyFile(FSSpec &inTurnSpec) {
	mFile = new LFileStream(inTurnSpec);	// create the file object
	mIsSpecified = true;
}

void
GalacticaSingleDoc::DoSave() {
	DEBUG_OUT("GalacticaSingleDoc::DoSave", DEBUG_IMPORTANT | DEBUG_USER | DEBUG_DATABASE);
	mGameInfo.sequenceNumber = mLastThingyID;		// make sure we don't duplicate any id numbers
	FSSpec fromSpec;
	mFile->GetSpecifier(fromSpec);	// do relative search, too
	mFile->OpenDataFork(fsWrPerm);
	LFileStream *theStream = (LFileStream*)mFile;
	theStream->SetMarker(0, streamFrom_Start);
	*theStream << (UInt32) type_SavedGameFile;	// this is a different type of file
	*theStream << (UInt32) version_SavedTurnFile;
    LStr255 password;
    SInt8 playerNum = GetMyPlayerNum();
    std::string tmpstr;
    GetPlayerName(playerNum, tmpstr);
    LStr255 playerName = tmpstr.c_str();
	*theStream << playerNum << (StringPtr)playerName << (StringPtr)password;
	mGameInfo.currTurn -= 3;						// make it harder to hack current turn number
	GameInfoT_v12 info;
	info = static_cast<GameInfoT_v12>(mGameInfo);
	info.currTurn = EndianS32_NtoB(mGameInfo.currTurn);
	info.maxTimePerTurn = EndianS32_NtoB(mGameInfo.maxTimePerTurn);
	info.secsRemainingInTurn = EndianS32_NtoB(mGameInfo.secsRemainingInTurn);
	info.totalNumPlayers = EndianS16_NtoB(mGameInfo.totalNumPlayers);
	info.numPlayersRemaining = EndianS16_NtoB(mGameInfo.numPlayersRemaining);
    ASSERT(sizeof(EGameState) == sizeof(SInt32));
    info.gameState = (EGameState)EndianS32_NtoB(mGameInfo.gameState);
	info.sequenceNumber = EndianS32_NtoB(mGameInfo.sequenceNumber);
	info.compSkill = EndianS16_NtoB(mGameInfo.compSkill);
	info.compAdvantage = EndianS16_NtoB(mGameInfo.compAdvantage);
	info.sectorDensity = EndianS16_NtoB(mGameInfo.sectorDensity);
	info.sectorSize = EndianS16_NtoB(mGameInfo.sectorSize);
	theStream->WriteBlock(&info, sizeof(GameInfoT_v12));
	mGameInfo.currTurn += 3;						// put it back the way it is

	SInt32 currTurnCheck = mGameInfo.currTurn;
	currTurnCheck *= 3;
	currTurnCheck += 13;
	*theStream << currTurnCheck;

  #define EXTRA_SIZE sizeof(SInt32)*15
	char blank[EXTRA_SIZE];
	for (int i = 0; i < EXTRA_SIZE; blank[i++] = 0) {}	// clear the zero space
	theStream->WriteBlock(&blank[0], EXTRA_SIZE);	// add in a little space for future expansion
													// must remember to subtract from size any amount used
    													// here, in ReadSavedTurnFile(), and in SpecifyHostFromSavedTurn()
	DEBUG_OUT("DoSave wrote header", DEBUG_DETAIL);
    WritePlayerInfoToFile(*theStream);

	float zoom = mStarMap->GetZoom();
	short displayMode = mStarMap->GetDisplayMode();
	PaneIDT selectedThingyID = 0;
	Rect r, er;
	Boolean hasEventsWin = false;
	mWindow->CalcPortFrameRect(r);
	mWindow->PortToGlobalPoint(topLeft(r));
	mWindow->PortToGlobalPoint(botRight(r));
	if (mEventsWindow) {
		hasEventsWin = true;
		mEventsWindow->CalcPortFrameRect(er);
		mEventsWindow->PortToGlobalPoint(topLeft(er));
		mEventsWindow->PortToGlobalPoint(botRight(er));
	}
	SPoint32 sp;
	mStarMap->GetScrollPosition(sp);
	if (mSelectedThingy) {
		selectedThingyID = mSelectedThingy->GetPaneID();
	}
	*theStream << mGameInfo.currTurn;
	*theStream << (Boolean)mDrawNames << (Boolean)mDrawGrid <<  (Boolean)mDrawStarLanes << (Boolean)mDrawShips
				<< (Boolean)mWantGameMessages << (Boolean)mWantPlayerMessages << selectedThingyID
				<< mNumUnprocessedAutoShows << mNumUnprocessedGotos << (Boolean)mGotoEnemyMoves
				<< r.top << r.left << r.bottom << r.right << sp.h << sp.v
				<< zoom << (Boolean)CSoundResourcePlayer::sPlaySound << (Boolean)Galactica::Globals::GetInstance().getFullScreenMode()
				<< displayMode << (Boolean)mShowAllCourses << (Boolean)hasEventsWin;
	if (hasEventsWin) {
		*theStream << er.top << er.left << er.bottom << er.right << (Boolean)mEventsWindow->IsVisible();
	}
	*theStream << (short)kNumUniquePlayerColors;    // 2.1b9r13, write out the number of colors
	for (int i = 0; i < kNumUniquePlayerColors; i++) {
		*theStream
			<< mColorArray[i].red
			<< mColorArray[i].green
			<< mColorArray[i].blue
			<< mSelectedColorArray[i].red
			<< mSelectedColorArray[i].green
			<< mSelectedColorArray[i].blue;
	}
	DEBUG_OUT("DoSave wrote game settings", DEBUG_DETAIL);

	Handle h = NULL;
	if (mNeedy) {	// write out the Event Report list
		h = mNeedy->GetItemsHandle();
	}
	theStream->WriteHandle(h);
	h = NULL;
	if (mNewOrChangedClientThingys)	{ // write out the list of new client thingys
		h = mNewOrChangedClientThingys->GetItemsHandle();
	}
	theStream->WriteHandle(h);
	DEBUG_OUT("DoSave wrote event report and new/changed thingys list", DEBUG_DETAIL);

	mStarMap->WriteStream(static_cast<LFileStream*>(mFile));
	DEBUG_OUT("DoSave wrote starmap", DEBUG_DETAIL);

	// 4/11/99, v1.2b10, write the needy messages to the stream
	UInt32 numMessages = 0;
	if (mNeedy) {
		numMessages = mNeedy->GetCount();
	}
	*theStream << numMessages;
	if (mNeedy) {
		ThingMemInfoT info;
		CMessage* msg;
		for (UInt32 i = 1; i <= numMessages; i++) {
			Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
			mNeedy->FetchItemAt(i, &info);
			msg = ValidateMessage(info.thing);
			ASSERT(msg != nil);
			ASSERT(IS_ANY_MESSAGE_TYPE(msg->GetThingySubClassType()));
			*theStream << msg->GetThingySubClassType();
			msg->WriteStream(theStream);
		}
	}
	DEBUG_OUT("DoSave wrote needy messages", DEBUG_DETAIL);

	// all done
	mFile->CloseDataFork();
	SetModified(false);
	DEBUG_OUT("Exit DoSave()", DEBUG_IMPORTANT);
}

void
GalacticaSingleDoc::ReadSavedTurnFile() {	// inReverting
	DEBUG_OUT("GalacticaSingleDoc::ReadSavedTurnFile TURN#" << mGameInfo.currTurn, DEBUG_IMPORTANT | DEBUG_DATABASE | DEBUG_USER);
	Handle h = nil;
	ASSERT(mNeedy == nil);
	AGalacticThingy *it;
	UInt32 checkVersion;
	Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
	mFile->OpenDataFork(fsRdPerm);
	LFileStream *theStream = (LFileStream*)mFile;
	theStream->SetMarker(sizeof(OSType), streamFrom_Start);
	*theStream >> checkVersion;
	Boolean wasTimed = mAutoEndTurnTimer.IsCounting();
    LStr255 playerName;
    LStr255 password;
    SInt8 playerNum;
	SetModified(false);	// presume for now that this will make mem version same as disk version
	if (wasTimed) {	// don't let the EOT be called on us while we are displaying a dialog
		mAutoEndTurnTimer.Pause();
		mAutoEndTurnTimer.SetTimeRemainingWhilePaused(mGameInfo.secsRemainingInTurn);
	}
	if (checkVersion == version_v12b10_SavedTurnFile) {
    	DEBUG_OUT("Game file version v1.2b10 legacy saved game acceptable. Reading (" << theStream->GetMarker() << ").", DEBUG_DETAIL);
		theStream->SetMarker(sizeof(DBPlayerInfoT), streamFrom_Marker);	// skip over the login info
    } else {
        if (checkVersion == version_v21d1_SavedTurnFile) {
    	    DEBUG_OUT("Game file version 2.1d1 legacy saved game acceptable. Reading (" << theStream->GetMarker() << ").", DEBUG_DETAIL);
        } else if (checkVersion == version_v21b9_SavedTurnFile) {
    	    DEBUG_OUT("Game file version v2.1b9r13 legacy saved game acceptable. Reading (" << theStream->GetMarker() << ").", DEBUG_DETAIL);
        } else if (checkVersion == version_SavedTurnFile) {
        	DEBUG_OUT("Game file version correct. Reading (" << theStream->GetMarker() << ").", DEBUG_DETAIL);
    	} else {	// wasn't a valid save game file
    		mFile->CloseDataFork();
    		Throw_(envBadVers);
	    }
    	*theStream >> playerNum >> playerName >> password;
	}
	theStream->ReadBlock(&mGameInfo, sizeof(GameInfoT_v12)); // endian swapping below
	// do endian conversion
	mGameInfo.currTurn = EndianS32_BtoN(mGameInfo.currTurn);
	mGameInfo.maxTimePerTurn = EndianS32_BtoN(mGameInfo.maxTimePerTurn);
	mGameInfo.secsRemainingInTurn = EndianS32_BtoN(mGameInfo.secsRemainingInTurn);
	mGameInfo.totalNumPlayers = EndianS16_BtoN(mGameInfo.totalNumPlayers);
	mGameInfo.numPlayersRemaining = EndianS16_BtoN(mGameInfo.numPlayersRemaining);
	mGameInfo.gameState = (EGameState)EndianS32_BtoN(mGameInfo.gameState);
	mGameInfo.sequenceNumber = EndianS32_BtoN(mGameInfo.sequenceNumber);
	mGameInfo.compSkill = EndianS16_BtoN(mGameInfo.compSkill);
	mGameInfo.compAdvantage = EndianS16_BtoN(mGameInfo.compAdvantage);
	mGameInfo.sectorDensity = EndianS16_BtoN(mGameInfo.sectorDensity);
	mGameInfo.sectorSize = EndianS16_BtoN(mGameInfo.sectorSize);
	
	mGameInfo.currTurn += 3;						// make it harder to hack current turn number
	mLastThingyID = mGameInfo.sequenceNumber;		// make sure we don't duplicate any id numbers
	mStarMap->SetSectorSize(mGameInfo.sectorSize);	// get correct dimensions for StarMapView

	SInt32 currTurnCheck;		// originally part of the space for future expansion
	*theStream >> currTurnCheck;
	currTurnCheck -= 13;
	currTurnCheck /= 3;
	if ((Galactica::Globals::GetInstance().sRegistration[0] == 0) && (currTurnCheck != mGameInfo.currTurn)) {
		DEBUG_OUT("Game file was hacked, #"<<currTurnCheck<<" --> #"<<mGameInfo.currTurn, DEBUG_ERROR);
		mFile->CloseDataFork();
		Throw_(envBadVers);
	}
	DEBUG_OUT("Base game info read (" << (void*)theStream->GetMarker() << ").", DEBUG_DETAIL);

	long extrasize = sizeof(SInt32)*15;
	theStream->SetMarker(extrasize, streamFrom_Marker);	// skip over space for future expansion

	DEBUG_OUT("Skipped expansion space (" << (void*)theStream->GetMarker() << ").", DEBUG_DETAIL);

	if (checkVersion == version_v12b10_SavedTurnFile) {
    	theStream->ReadHandle(h);	// read the aliases to the database files
    	if (h) {					// and dispose of them because they aren't needed
    		::DisposeHandle(h);		
    	}
    	theStream->ReadHandle(h);
    	if (h) {
    		::DisposeHandle(h);
    	}
    	DEBUG_OUT("Legacy Database aliases read (" << (void*)theStream->GetMarker() << ").", DEBUG_DETAIL);
	}
	
	short count;			// restore the list of players
    PublicPlayerInfoT publicInfo;
    PrivatePlayerInfoT privateInfo;
	*theStream >> count;
	if (checkVersion == version_v12b10_SavedTurnFile) {
     	DBPlayerInfoT theInfo;
       	for (int i = 1; i <= count; i++) {
        	DEBUG_OUT("Reading Legacy Player Info for player " << i << " from (" << (void*)theStream->GetMarker() 
    	            << "), size = " << sizeof(DBPlayerInfoT) << ".", DEBUG_TRIVIA);
    		theStream->ReadBlock(&theInfo, sizeof(DBPlayerInfoT)); // endian swapping below
    		
    		// byte swapping
			theInfo.lastTurnPosted = EndianS32_BtoN(theInfo.lastTurnPosted);
			theInfo.numPiecesRemaining = EndianS32_BtoN(theInfo.numPiecesRemaining);	// when this is 0, player is dead
			theInfo.score = EndianS32_BtoN(theInfo.score);
			theInfo.homeID = EndianS32_BtoN(theInfo.homeID);
			theInfo.expert.techStarID = EndianS32_BtoN(theInfo.expert.techStarID);		// id of main tech star (may be others)
			theInfo.hiStarTech = EndianS16_BtoN(theInfo.hiStarTech);		// used to decide when to send a "new discovery" message
			theInfo.hiShipTech = EndianS16_BtoN(theInfo.hiShipTech);		// used to decide when to send a "new class of ship" message

    		theInfo.GetPublicInfo(publicInfo, mGameInfo, i);
    		theInfo.GetPrivateInfo(privateInfo, mGameInfo);
    		SetPlayerInfo(i, publicInfo);
    		SetPrivatePlayerInfo(i, privateInfo);
    	}
    } else {
        // v2.1, stores public info and private info in separate data structures
        for (int i = 1; i <= count; i++) {
            theStream->ReadBlock(&publicInfo, sizeof(PublicPlayerInfoT)); // no endian swapping needed
            // read smaller old version for backward compatibility
            std::memset(&privateInfo, 0, sizeof(PrivatePlayerInfoT));
            theStream->ReadBlock(&privateInfo, sizeof(PrivatePlayerInfoT_v22)); // endian swapping below; read private info using old Data struct
    		// byte swapping
			privateInfo.lastTurnPosted = EndianS32_BtoN(privateInfo.lastTurnPosted);
			privateInfo.numPiecesRemaining = EndianS32_BtoN(privateInfo.numPiecesRemaining);	// when this is 0, player is dead
			privateInfo.homeID = EndianS32_BtoN(privateInfo.homeID);
			privateInfo.techStarID = EndianS32_BtoN(privateInfo.techStarID);		// id of main tech star (may be others)
			privateInfo.hiStarTech = EndianS16_BtoN(privateInfo.hiStarTech);		// used to decide when to send a "new discovery" message
			privateInfo.hiShipTech = EndianS16_BtoN(privateInfo.hiShipTech);		// used to decide when to send a "new class of ship" message
            // set values that are expected to be read from config
            bool assigned = false;
            if (GetMyPlayerNum() == i) {
            	// for this client, assign from config
            	pdg::ConfigManager* config = Galactica::Globals::GetInstance().getConfigManager();
            	long tempLong;
            	if (config->getConfigLong(GALACTICA_PREF_DEFAULT_BUILD, tempLong)) {
            		privateInfo.defaultBuildType = tempLong;
            	} else {
					privateInfo.defaultBuildType = 1;
            	}
            	assigned = config->getConfigLong(GALACTICA_PREF_DEFAULT_GROWTH, tempLong);
            	privateInfo.defaultGrowth = tempLong;
            	assigned &= config->getConfigLong(GALACTICA_PREF_DEFAULT_TECH, tempLong);
            	privateInfo.defaultTech = tempLong;
            	assigned &= config->getConfigLong(GALACTICA_PREF_DEFAULT_SHIPS, tempLong);
            	privateInfo.defaultShips = tempLong;
            }
            if (!assigned) {
          		// for other players (or if no config values available), use defaults
				privateInfo.defaultBuildType = 1;
				privateInfo.defaultGrowth = 334;
				privateInfo.defaultTech = 333;
				privateInfo.defaultShips = 333;
			}
			// now save the player info
            SetPlayerInfo(i, publicInfo);
            SetPrivatePlayerInfo(i, privateInfo);
        }
    }
   // v2.1b12, populate the private player info for ourselves from the list of player infos
	GetPrivatePlayerInfo(GetMyPlayerNum(), mPlayerInfo);
	DEBUG_OUT("Player Info for all players read (" << (void*)theStream->GetMarker() << ").", DEBUG_DETAIL);

	mStarMap->Disable();
	long turnNum;
	Rect r, er;
	PaneIDT selectedThingyID;
	SPoint32 sp;
	Boolean hasEventsWindow;
	float zoom;
	short displayMode;
	bool fullScreenMode;
	*theStream  >> turnNum;
	*theStream  >> mDrawNames >> mDrawGrid >> mDrawStarLanes >> mDrawShips
				>> mWantGameMessages >> mWantPlayerMessages >> selectedThingyID
				>> mNumUnprocessedAutoShows >> mNumUnprocessedGotos >> mGotoEnemyMoves
				>> r.top >> r.left >> r.bottom >> r.right >> sp.h >> sp.v
				>> zoom >> CSoundResourcePlayer::sPlaySound >> fullScreenMode
				>> displayMode >> mShowAllCourses >> hasEventsWindow;
	Galactica::Globals::GetInstance().setFullScreenMode(fullScreenMode);
	DEBUG_OUT("Base UI info read (" << (void*)theStream->GetMarker() << ").", DEBUG_DETAIL);
	if (hasEventsWindow) {
		Boolean isVisible;
		*theStream >> er.top >> er.left >> er.bottom >> er.right >> isVisible;
		ShowEventsWindow();
		mEventsWereShown = isVisible;
		mEventsWindow->DoSetBounds(er);
		if (!isVisible) {
			mEventsWindow->Hide();
			sEventsWindow = NULL;
		}
	    DEBUG_OUT("Events Window info read (" << (void*)theStream->GetMarker() << ").", DEBUG_DETAIL);
	}
	mWindow->DoSetBounds(r);
	mStarMap->SetDisplayMode(displayMode);
	LToggleButton* theButton = (LToggleButton*) mWindow->FindPaneByID(displayMode + cmd_DisplayNone);
	if (theButton) {
		theButton->SetValue(1);
	}
	mStarMap->ZoomTo(zoom);
	mStarMap->ScrollImageTo(sp.h, sp.h, false);	// restore the window size and scroll pos
	short numPlayerColors;
    if (checkVersion > version_v21d1_SavedTurnFile) {
        // v2.1b9r13, read in the number of colors that were saved into the file
        *theStream >> numPlayerColors;
    } else if (checkVersion == version_v12b10_SavedTurnFile) {
	    numPlayerColors = 8;    // v2.1b9r7, fix problem reading legacy saved games which
	                            // had a different number of player colors
	} else {
	    numPlayerColors = 15;   // versions between v1.2b10 and v2.1b9r13 (non-inclusive) 
	}                           // had 15 colors written
	for (int i = 0; i < numPlayerColors; i++) {
	    if (i < kNumUniquePlayerColors) {  // don't go past the end of the array
		    *theStream
			>> mColorArray[i].red
			>> mColorArray[i].green
			>> mColorArray[i].blue
			>> mSelectedColorArray[i].red
			>> mSelectedColorArray[i].green
			>> mSelectedColorArray[i].blue;
		} else {
		    // read off the extra colors that won't fit in our array
		    unsigned short junk;
		    *theStream
			>> junk >> junk >> junk 
			>> junk >> junk >> junk; 
		}
	}
	DEBUG_OUT("Color array read: "<< numPlayerColors<<" items (" << (void*)theStream->GetMarker() 
	        << ").", DEBUG_DETAIL);
	mNeedy = NULL;
	h = NULL;
	theStream->ReadHandle(h);
	if (h) {	// read in the Event Report list
		mNeedy = new LArray(sizeof(ThingMemInfoT), h);
	    DEBUG_OUT("Read non-Null Needy Array: " << mNeedy->GetCount() << " items (" 
	            << (void*)theStream->GetMarker() << ").", DEBUG_DETAIL);
	}
    if (mNeedy) {
	    // 2.1b9r7, set all the pointer values to NULL so we don't crash trying to look at them
	    for (int i = 1; i <= mNeedy->GetCount(); i++) {
	        ThingMemInfoT* infoP = static_cast<ThingMemInfoT*>(mNeedy->GetItemPtr(i));
	        infoP->thing = NULL;
	    }
    }
	h = NULL;
	theStream->ReadHandle(h);
	if (h) {	// read in the New Client thingys list
		mNewOrChangedClientThingys = new LArray(sizeof(NewThingInfoT), h);
		mNewOrChangedClientThingys->SetComparator(LLongComparator::GetComparator(), false);
	    DEBUG_OUT("Read non-Null Changed Array: " << mNewOrChangedClientThingys->GetCount() 
	            << " items (" << (void*)theStream->GetMarker() << ").", DEBUG_DETAIL);
	}
	long endOfHeaderPos = theStream->GetMarker();	// save position of end of header in case
	DEBUG_OUT("New or Changed list read (" << (void*)theStream->GetMarker() << ").", DEBUG_DETAIL);
	
	// v2.1, we read are about to start doing things that require use 
	// of the static method GalacticaShared::GetStreamingGame()
    StartStreaming();
    
	mStarMap->UpdatePort();				// get some screen redraw
	// v2.1b9r7, check return result from starmap->ReadStream for partial read
	if ( mStarMap->ReadStream(theStream, checkVersion) ) {;
    	// 4/11/99, v1.2b10, read the needy messages from the stream
    	DEBUG_OUT("Starmap read (" << (void*)theStream->GetMarker() << ").", DEBUG_DETAIL);
    	UInt32 numMessages;
    	*theStream >> numMessages;
    	if (numMessages > 0) {
    		CMessage* msg;
    		long thingyType;
    		for (UInt32 i = 1; i <= numMessages; i++) {
    			Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
    			*theStream >> thingyType;
    			ASSERT(IS_ANY_MESSAGE_TYPE(thingyType));
	            // v2.1b9r7, don't do this if the type is bad
    			if (IS_ANY_MESSAGE_TYPE(thingyType)) {
        			msg = (CMessage*)MakeThingy(thingyType);
        			msg->ReadStream(theStream);
    			}
    		}
    	}
    	DEBUG_OUT("Messages read (" << (void*)theStream->GetMarker() << ").", DEBUG_DETAIL);
    	// end v1.2b10 change
	} else {
    	DEBUG_OUT("Starmap read incomplete (" << (void*)theStream->GetMarker() << ").", DEBUG_ERROR);
	}
	// v2.1, we have now reached the end of the list of things that require use 
	// of the static method GalacticaShared::GetStreamingGame()
	StopStreaming();

	// now we need to recreate the needy list's pointers to the thingys
	if (mNeedy) {
		ThingMemInfoT info;
		LArrayIterator iter(*mNeedy);
		while (iter.Next(&info)) {
			ASSERT(info.info.action == action_Message);
			it = FindThingByID(info.info.thingID);
		  #ifdef DEBUG
			if (it) {
				it = ValidateMessage(it);
				ASSERT_STR(it != nil, "thingy "<<info.info.thingID<< "found in saved needy list, but was not a message");	// item should be a message
			}
		  #endif
			if (it) {
				info.thing = it;
				if (info.info.flags & thingInfoFlag_AutoShow) {	// replay all autoshows when reloaded
					info.info.flags &= ~(UInt16)thingInfoFlag_Handled;
				}
				mNeedy->AssignItemsAt(1, iter.GetCurrentIndex(), &info);
				DEBUG_OUT(it<<" re-established in saved needy list", DEBUG_TRIVIA | DEBUG_EOT | DEBUG_MESSAGE);
			} else {
				DEBUG_OUT(LongTo4CharStr(info.info.thingType) << " " << info.info.thingID << " [?] not found, but in saved needy list", DEBUG_ERROR | DEBUG_EOT | DEBUG_MESSAGE);
				mNeedy->RemoveItemsAt(1, iter.GetCurrentIndex());
			}
		}
		if (hasEventsWindow) {
			ThrowIfNil_(mEventsTable);
			mEventsTable->InstallArray(mNeedy, false);	//must re-install the array
		}
	}
	DEBUG_OUT("Needy List array rebuilt.", DEBUG_DETAIL);

	RecalcHighestValues();
	DEBUG_OUT("Highest Values Recalculated.", DEBUG_DETAIL);
	mStarMap->Enable();
	DEBUG_OUT("Starmap enabled.", DEBUG_DETAIL);
	if (selectedThingyID) {
	DEBUG_OUT("About to select thingy "<< selectedThingyID, DEBUG_DETAIL);
		it = FindThingByID(selectedThingyID);
		if (it) {
		    AGalacticThingyUI* ui = it->AsThingyUI();
		    if (ui) {
			    ui->Select();
			}
		}
   	DEBUG_OUT("Thingy Selected.", DEBUG_DETAIL);
	} else {
	   SwitchToButtonBar(0);
	   SetSelectedThingy(NULL);
	}
	try {
	    mFile->CloseDataFork();
	}
	catch (...) {
	   DEBUG_OUT("Unexpected exception closing file, trapped.", DEBUG_ERROR);
	}
	DEBUG_OUT("Done reading saved game file.", DEBUG_IMPORTANT);
	FinishGameLoad();
	SetUpdateCommandStatus(true);
	if (wasTimed) {
		mAutoEndTurnTimer.Resume();
	}
	UpdateTurnNumberDisplay();
	DEBUG_OUT("Exit ReadSavedTurnFile()", DEBUG_IMPORTANT);
}


void
GalacticaSingleDoc::DoRevert() {
	LWindow* aWindow = mWindow;
	mWindow = nil;	// these must be nil before things start to close down and reopen
	mPanelCaption = nil;
	mViewer = nil;
	mAutoEndTurnTimer.Clear();	// make sure the timer isn't still trying to run
	Galactica::Globals::GetInstance().getWindowMenu()->RemoveWindow( aWindow );
	mClosing = true;	// to avoid any problems when thingys are deleted
	delete aWindow;
	mClosing = false;
	mEverything.RemoveAllItemsAfter(0);	// clear out the everything array
	mSelectedThingy = nil;
	mOldSelectedThingy = nil;
	mButtonBar = nil;
	mPanel = nil;
	mViewNameCaption = nil;
	if (mNeedy) {
		delete mNeedy;
		mNeedy = nil;
	}
	if (mNeedyIterator) {
		delete mNeedyIterator;
		mNeedyIterator = nil;
	}
	MakeWindow();
	ReadSavedTurnFile();
	ShowWindow(kDontPlayMessages);
}

void
GalacticaSingleDoc::WritePlayerInfoToFile(LFileStream& inStream) {
	short count = mGameInfo.totalNumPlayers;
    PublicPlayerInfoT publicInfo;	// write out a count, followed by that number of player info records
    PrivatePlayerInfoT privateInfo;
	inStream.WriteBlock(&count, sizeof(short));
	for (int i = 1; i <= count; i++) {
	    GetPlayerInfo(i, publicInfo);
	    GetPrivatePlayerInfo(i, privateInfo);
        inStream.WriteBlock(&publicInfo, sizeof(PublicPlayerInfoT));
        inStream.WriteBlock(&privateInfo, sizeof(PrivatePlayerInfoT_v22));
	}
	DEBUG_OUT("Player Info for all players written (" << (void*)inStream.GetMarker() << ").", DEBUG_DETAIL);
}



void
GalacticaSingleDoc::DoEndTurn() {
	DEBUG_OUT("GameDoc::DoEndTurn TURN # " << mGameInfo.currTurn, DEBUG_IMPORTANT);
	if (mSelectedThingy) {
		ASSERT_STR(ValidateThingy(mSelectedThingy), "Selected thingy was invalid");	// extreme sanity check
		mOldSelectedThingy = mSelectedThingy;
	} else {
		mOldSelectedThingy = nil;
	}
	// tutorial, make sure they haven't set the shipbuilding level on their home star too low.
  #if TUTORIAL_SUPPORT
	if (Tutorial::TutorialIsActive()) {
		AGalacticThingy* it = FindThingByID(mPlayerInfo.homeID);
		CStar* home = dynamic_cast<CStar*>(it);
		if (home) {	
			SettingT* setting = home->GetSettingPtr(spending_Growth);
			if (mGameInfo.currTurn == 1) {	// set to specific levels that are just right
/*				setting[spending_Growth].desired = 400;
				setting[spending_Growth].value = 400;
				setting[spending_Ships].desired = 200;
				setting[spending_Ships].value = 200;
				setting[spending_Research].desired = 400;
				setting[spending_Research].value = 400; */
			} else if (mGameInfo.currTurn < 14) {
				setting[spending_Growth].desired = 334;
				setting[spending_Growth].value = 334;
				setting[spending_Ships].desired = 333;
				setting[spending_Ships].value = 333;
				setting[spending_Research].desired = 333;
				setting[spending_Research].value = 333;
			}
		} else {
			DEBUG_OUT("Tutorial: Couldn't find home star "<<mPlayerInfo.homeID, DEBUG_ERROR | DEBUG_TUTORIAL);
		}
	}
  #endif //TUTORIAL_SUPPORT
	SetSelectedThingy(nil);
	mDisallowCommands = true;	// v1.2b11d4, block from handling commands during EOT

	mAutoEndTurnTimer.Clear();				// clear the timer
	mEndTurnButton->Disable();
	mEndTurnButton->Refresh();
	mEndTurnButton->UpdatePort();
	StopAllMouseTracking();
	
#ifdef PROFILE_EOT
	ProfilerInit(collectDetailed, bestTimeBase, 1000, 100);
#endif
	AGalacticThingy	*aThingy = nil;
	LArrayIterator iterator(mEverything, LArrayIterator::from_Start); // Search list of all thingys created
	
	// ==============================
	//        COMPUTER MOVE
	// ==============================
	DEBUG_OUT("EOT: Doing Computer Move", DEBUG_DETAIL | DEBUG_EOT);
	long n = 0;
	while ( iterator.Next(&aThingy) ) {
		Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
	  #ifdef DEBUG
		aThingy = ValidateThingy(aThingy);
		ASSERT_STR(aThingy != nil, "Found invalid thingy at index "<<iterator.GetCurrentIndex()<<" of the everything list");
		if (!aThingy) {
			mEverything.RemoveItemsAt(1, iterator.GetCurrentIndex());
			DEBUG_OUT("Removed invalid thingy from index "<<iterator.GetCurrentIndex()<<" of the everything list", DEBUG_ERROR);
			continue;
		}
	  #endif
		if (aThingy->IsOwned() && PlayerIsComputer(aThingy->GetOwner()) ) {
			DEBUG_OUT("  Computer Move for " << aThingy, DEBUG_TRIVIA | DEBUG_EOT | DEBUG_AI);
			aThingy->DoComputerMove(mGameInfo.currTurn);
			n++;
			if ((n % 16)==0) {
				YieldTime();
			}
		}
	}
	DEBUG_OUT("EOT: Did computer move for "<<n<<" of "<<mEverything.GetCount()<<" thingys", DEBUG_DETAIL | DEBUG_EOT);

	// ==============================
	//    DELETE DEAD, CLEAR CHANGE
	// ==============================
	DEBUG_OUT("EOT: Deleting dead and clearing changed flags for Everything list", DEBUG_DETAIL | DEBUG_EOT | DEBUG_OBJECTS);
	n = 0;
	long nd = 0;
	iterator.ResetTo(LArrayIterator::from_Start); // reset for deletion of dead thingys
	while ( iterator.Next(&aThingy) ) {
		Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
	  #ifdef DEBUG
		aThingy = ValidateThingy(aThingy);
		ASSERT_STR(aThingy != nil, "Found invalid thingy at index "<<iterator.GetCurrentIndex()<<" of the everything list");
		if (!aThingy)
			continue;
	  #endif
		if (aThingy->IsToBeScrapped()) {
		    // we do this here rather than in PrepareForEndOfTurn to ensure that the scrapped things are immediately
		    // removed in the section below, rather than hanging around for the next turn
			DEBUG_OUT("  Killing Scrapped Object "<< aThingy, DEBUG_TRIVIA | DEBUG_EOT | DEBUG_OBJECTS);
			aThingy->Scrapped();
			aThingy->Die();
		}
		if (aThingy->IsDead()) {
			// Remove dead thingys here
			DEBUG_OUT("  Deleting Dead Object "<< aThingy, DEBUG_TRIVIA | DEBUG_EOT | DEBUG_OBJECTS);
			// remove the thing from anything that refers to it
			// 1.2b11d4, 6/11/99, this takes way to long in big games, so don't do it
			// v2.1, 12/16/01, the consequences of not doing it are too severe, so I put it back
			aThingy->RemoveSelfFromGame();
			delete aThingy;	// now we can delete the dead thingy
			nd++;
		} else if (aThingy->IsChanged()) {
			// these items could be written to the datastore here, but since it's a single player game, there's
			// no need to do that.
			DEBUG_OUT("  Clearing Change flag for " << aThingy, DEBUG_TRIVIA | DEBUG_EOT);
			aThingy->NotChanged();
			n++;
		}
		if ((n % 16)==0) {
			YieldTime();
		}
	}
	DEBUG_OUT("EOT: Deleted "<<nd<<" dead out of "<<mEverything.GetCount()<<" thingys", DEBUG_DETAIL | DEBUG_EOT | DEBUG_OBJECTS);
	DEBUG_OUT("EOT: Cleared Changed flag for "<<n<<" of "<<mEverything.GetCount()<<" thingys", DEBUG_DETAIL | DEBUG_EOT | DEBUG_OBJECTS);

	// ==============================
	//            EOT
	// ==============================
	PrepareForEndOfTurn(); 	// do anything needed by subclasses before EOT starts 
	DEBUG_OUT("EOT: Host processing EOT for Everything list", DEBUG_DETAIL | DEBUG_EOT);
	n = 0;
	iterator.ResetTo(LArrayIterator::from_Start); // now do eot
	while ( iterator.Next(&aThingy) ) {
		Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
	  #ifdef DEBUG
		aThingy = ValidateThingy(aThingy);
		ASSERT(aThingy != nil);
		if (!aThingy)
			continue;
	  #endif
		DEBUG_OUT("  EOT for " << aThingy, DEBUG_TRIVIA | DEBUG_EOT);
		aThingy->EndOfTurn(mGameInfo.currTurn);
		n++;
		if ((n % 16)==0) {
			YieldTime();
		}
	}
	DEBUG_OUT("EOT: Host did EOT for "<<n<<" of "<<mEverything.GetCount()<<" thingys", DEBUG_DETAIL | DEBUG_EOT);

	// ==============================
	//          FINISH EOT
	// ==============================
	DEBUG_OUT("EOT: Host finishing EOT for Everything list", DEBUG_DETAIL | DEBUG_EOT);
	n = 0;
	iterator.ResetTo(LArrayIterator::from_Start); // now do eot
	while ( iterator.Next(&aThingy) ) {
		Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
	  #ifdef DEBUG
		aThingy = ValidateThingy(aThingy);
		ASSERT(aThingy != nil);
		if (!aThingy)
			continue;
	  #endif
		DEBUG_OUT("  Finish EOT for " << aThingy, DEBUG_TRIVIA | DEBUG_EOT);
		aThingy->FinishEndOfTurn(mGameInfo.currTurn);
		n++;
		if ((n % 16)==0) {
			YieldTime();
		}
	}
	DEBUG_OUT("EOT: Host finished EOT for "<<n<<" of "<<mEverything.GetCount()<<" thingys", DEBUG_DETAIL | DEBUG_EOT);

	DEBUG_OUT("EOT: Host updating visibility for Everything list", DEBUG_DETAIL | DEBUG_EOT);
	n = 0;
	iterator.ResetTo(LArrayIterator::from_Start);	// go back and make sure all the ships are aimed right
	while (iterator.Next(&aThingy)) {
		NotifyBusy();					// put up our busy cursor.
	  #ifdef DEBUG
		aThingy = ValidateThingy(aThingy);
		ASSERT(aThingy != nil);
		if (!aThingy) {
			continue;
		}
	  #endif
	    // for things that are scanners and their scan range or location changed, see
	    // what new stuff they may have spotted
	    if (aThingy->HasScanners() && aThingy->HasScannerChanged()) {
		    aThingy->UpdateWhatIsVisibleToMe(); 
		}
		// for things that have moved, and thus possibly moved into or out of scan range
		// see what their new visibility is
		if (aThingy->NeedsRescan()) {
		    aThingy->CalculateVisibilityToAllOpponents();
		}
		n++;
		if ((n % 16)==0) {
			YieldTime();
		}
	}
	DEBUG_OUT("EOT: Host updated visibility for "<<n<<" of "<<mEverything.GetCount()<<" thingys", DEBUG_DETAIL | DEBUG_EOT);

	// must restore current player's info before starting new turn
//	LoadPlayerInfo(mPlayerNum);
	
	// ==============================
	//       CLEAR OLD NEEDY LIST
	// ==============================
	ThingMemInfoT info;
	if (mNeedyIterator != nil) {	// dump the old needy iterator
		delete mNeedyIterator;
		mNeedyIterator = nil;
	}
	DEBUG_OUT("EOT: Clearing out needy list", DEBUG_DETAIL | DEBUG_EOT | DEBUG_MESSAGE | DEBUG_OBJECTS);
	if (mNeedy != nil) {	// leave NOTHING in the needy list
		Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
		LArrayIterator needyIterator(*mNeedy);
	  #ifdef DEBUG
		int count = 0;
	  #endif
		while (needyIterator.Next(&info)) {
			aThingy = ValidateMessage(info.thing);	// only messages in needy list
			ASSERT(aThingy != nil);
			if (aThingy && !aThingy->SendDeleteInfo() && !aThingy->IsDead()) {
				if ((info.info.flags & thingInfoFlag_AutoShow) && mWantGameMessages) {	//v1.2b11d5, added mWantGameMessages
					// autoshow, they should all be dead if they were checked and messages weren't held
					DEBUG_OUT("  Killing living autoshow " << aThingy<<" in old Needy list", DEBUG_ERROR | DEBUG_OBJECTS | DEBUG_MESSAGE | DEBUG_EOT);
				} else {
					// it's a goto or we didn't want messages, so it can just be killed
					DEBUG_OUT("  Killing living " << aThingy<<" in old Needy list", DEBUG_DETAIL | DEBUG_OBJECTS | DEBUG_MESSAGE | DEBUG_EOT);
				}
				// v1.2b11d5, always kill old messages
				aThingy->Die();
			}
		  #ifdef DEBUG
			count++;
		  #endif
			DEBUG_OUT("  Removed " << aThingy << " from Needy list", DEBUG_TRIVIA | DEBUG_EOT | DEBUG_MESSAGE);
			mNeedy->RemoveItemsAt(1, needyIterator.GetCurrentIndex());
			if (aThingy->IsDead()) {
				DEBUG_OUT("  Deleting " << aThingy, DEBUG_DETAIL | DEBUG_OBJECTS | DEBUG_MESSAGE | DEBUG_EOT);
				delete aThingy;		// delete the dead message
			}
		}
		DEBUG_OUT(count << " items removed from old Needy list.", DEBUG_DETAIL | DEBUG_MESSAGE);
	}

	// ==============================
	//       UPDATE PLAYER INFO
	// ==============================
	long* playerPieceCount = new long[mGameInfo.totalNumPlayers+1];
	for (int i=0; i <= mGameInfo.totalNumPlayers; i++) {	// reset the piece count for the players
		playerPieceCount[i] = 0;
	}
	int livingHuman = -1;
	DEBUG_OUT("EOT: Updating player info", DEBUG_DETAIL | DEBUG_EOT);
	n = 0;
	nd = 0;
	long player;
	long type;
	CMessage* msg = nil;
	iterator.ResetTo(LArrayIterator::from_Start); // now do eot
	while ( iterator.Next(&aThingy) ) {
		Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
	  #ifdef DEBUG
		aThingy = ValidateThingy(aThingy);
		ASSERT_STR(aThingy != nil, "Found invalid thingy at index "<<iterator.GetCurrentIndex()<<" of the everything list");
		if (!aThingy)
			continue;
	  #endif
		type = aThingy->GetThingySubClassType();
		if ( !aThingy->IsDead() ) {
			if ( (type == thingyType_Ship) || (type == thingyType_Star) ) {
				DEBUG_OUT("  Counting living "<< aThingy << " toward player total", DEBUG_TRIVIA | DEBUG_EOT);
				player = aThingy->GetOwner();
				if (player > mGameInfo.totalNumPlayers) {
					DEBUG_OUT("Found thingy "<< aThingy << " with invalid owner ["<<player<<"]", DEBUG_ERROR | DEBUG_EOT);
					player = 0;
				}
				playerPieceCount[player]++;
				n++;
			} else if ( IS_ANY_MESSAGE_TYPE(type) ) {
				DEBUG_OUT("  Found message "<< aThingy << " to add to needy list", DEBUG_TRIVIA | DEBUG_MESSAGE | DEBUG_EOT);
				// this is a message, so build the information about it and put that info in
				// the needy list
				msg = (CMessage*)aThingy;
				AddMessageToNeedyList(msg);
				msg->BuildClientEvent();	// this must be done so that the message string is built
			} else {
				DEBUG_OUT("  Doing nothing with "<< aThingy << ": not a ship, star or message", DEBUG_TRIVIA | DEBUG_EOT);
			}
		} else {
			aThingy->RemoveSelfFromGame();	// removes from the game
			nd++;
		}
		if ((n % 16)==0) {
			YieldTime();
		}
	}
	DEBUG_OUT("EOT: "<<nd<<" of "<<mEverything.GetCount()<<" thingys died this turn", DEBUG_DETAIL | DEBUG_EOT);
	DEBUG_OUT("EOT: Counted "<<n<<" of "<<mEverything.GetCount()<<" living ships and stars", DEBUG_DETAIL | DEBUG_EOT);

	PrivatePlayerInfoT thePlayerInfo;
	for (int i = 1; i <= mGameInfo.totalNumPlayers; i++) {	//update players' changed items list
		Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up busy cursor.
		GetPrivatePlayerInfo(i, thePlayerInfo);
		DEBUG_OUT("EOT: Player "<<i<<" went from "<<thePlayerInfo.numPiecesRemaining<<" to "<<playerPieceCount[i]<<" thingys this turn", DEBUG_DETAIL | DEBUG_EOT);		

		if (thePlayerInfo.numPiecesRemaining) {
			thePlayerInfo.numPiecesRemaining = playerPieceCount[i];	// find out how many pieces the player has
			thePlayerInfo.lastTurnPosted = mGameInfo.currTurn;
			SetPrivatePlayerInfo(i, thePlayerInfo);

			if (thePlayerInfo.numPiecesRemaining == 0) {	// player just died
				DEBUG_OUT("== Player "<<i<<" was eliminated from the game ==", DEBUG_IMPORTANT | DEBUG_EOT);
				mGameInfo.numPlayersRemaining--;
				msg = CMessage::CreateEvent(this, event_PlayerDied, i, nil, kAllPlayers);
				if (msg) {
					msg->BuildClientEvent();	// this must be done so that the message string is built in single player
					AddMessageToNeedyList(msg);
				}
				if (i == GetMyPlayerNum()) {
					mGameInfo.numHumansRemaining--;
				} else {
					mGameInfo.numComputersRemaining--;
				}
			} else if (i == GetMyPlayerNum()) {
				livingHuman = i;	// keep track of who is alive
			}
		}
	}
	
	delete[] playerPieceCount;	// v1.2fc5, fix memory leak
	
	// v1.2b11d5, make sure brightness, etc get recalculated
	RecalcHighestValues();

	if (mGameInfo.gameState == state_GameOver) {
		DEBUG_OUT("== Game over: Now in Post Game mode ==", DEBUG_IMPORTANT | DEBUG_EOT);
		mGameInfo.gameState = state_PostGame;
	}
	if ( (mGameInfo.numHumansRemaining == 1) && (mGameInfo.numComputersRemaining<1)
		&& (mGameInfo.gameState == state_NormalPlay) && // don't give message during post game
		( (mGameInfo.numComputers > 0) || (mGameInfo.numHumans > 1) ) )  {	// or if started alone
		// we have a winner: send a notice to the player
		DEBUG_OUT("== Player "<<livingHuman<<" wins the game! ==", DEBUG_IMPORTANT | DEBUG_EOT);
		msg = CMessage::CreateEvent(this, event_PlayerWins, livingHuman, nil, kAllPlayers);
		if (msg) {
			msg->BuildClientEvent();	// this must be done so that the message string is built in single player
			AddMessageToNeedyList(msg);
		}
		mGameInfo.gameState = state_GameOver;
	}
	DEBUG_OUT("GameDoc::DoEndTurn: EOT COMPLETED", DEBUG_IMPORTANT | DEBUG_EOT);
	
	mGameInfo.currTurn++;		// start next turn
	
	LStr255 s = mGameInfo.currTurn;
	
			
#ifdef PROFILE_EOT
	ProfilerDump("\pprofile info");
	ProfilerTerm();	
#endif

	mEndTurnButton->Enable();// еее hack to hopefully fix problem with button not re-enabling itself
	mEndTurnButton->Refresh();
	mEndTurnButton->UpdatePort();
	mStarMap->Refresh();	// v1.2b11d4, refresh everything at once when finished
	
	static_cast<LApplication*>(mSuperCommander)->UpdateMenus();
	UpdateTurnNumberDisplay();
	mDisallowCommands = false;	// v1.2b11d4, block from handling commands during EOT
	if (mGameInfo.maxTimePerTurn) {
		mAutoEndTurnTimer.Reset(mGameInfo.nextTurnEndsAt);
	} else {
		mAutoEndTurnTimer.Clear();	// make timer read blank
	}

	DEBUG_OUT("EOT: Starting new turn, Showing Messages", DEBUG_DETAIL | DEBUG_EOT);
	
	ShowMessages();
	ShowNextNeedyThingy();
}


void
GalacticaSingleDoc::InitNewGameData(NewGameInfoT &inGameInfo) {
	DEBUG_OUT("GalacticaSingleDoc::InitNewGameData", DEBUG_IMPORTANT);
	FillSector(inGameInfo.density, inGameInfo.sectorSize);
	UserChangedSettings();
    FinishGameLoad();
}

void
GalacticaSingleDoc::CreateNew(NewGameInfoT &inGameInfo) {
	DEBUG_OUT("GalacticaSingleDoc::CreateNew", DEBUG_IMPORTANT);
	GalacticaProcessor::CreateNew(inGameInfo);
}

