//  GalacticaHostDoc.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include "GalacticaConsts.h"

#include "GenericUtils.h"

#include "GalacticaGlobals.h"
#include "Galactica.h"
#include "CStar.h"
#include "GalacticaUtils.h"
#include "GalacticaHostDoc.h"

#include "CVarDataFile.h"
#include "CMasterIndexFile.h"
#include "CMessage.h"
#include <LFileStream.h>
#include <UEnvironment.h>

// ==========================================================================
//          GALACTICA HOST DOC   -- GUI only
// ==========================================================================

// GUI Only
#include "GalacticaPanels.h"
#include "CGalacticSlider.h"
#include "GalacticaTables.h"
#include "GalacticaDoc.h"

// GUI Only
#include <LWindow.h>
#include <PP_Messages.h>
#include <PP_Resources.h>
#include <UDrawingState.h>
#include <LEditField.h>
#include <UModalDialogs.h>
#include <UWindows.h>

// GUI Only
#include "Slider.h"
#include "CTextTable.h"
#include "CWindowMenu.h"

// GUI Only
#include "CGameSetupAttach.h"
#include "CChangePlayerAttach.h"



GalacticaHostDoc::GalacticaHostDoc(LCommander *inSuper)
 : GalacticaHost(), LSingleDoc(inSuper)
{
   ASSERT(MAX_CONNECTIONS > MAX_PLAYERS_CONNECTED);
   DEBUG_OUT("New GalacticaHostDoc", DEBUG_IMPORTANT);
   mGameDataFile = NULL;
   mGameIndexFile = NULL;
   mQuitPending = false;
   mTurnNumDisplay = NULL;
   mNumHumansDisplay = NULL;
   mNumCompsDisplay = NULL;
   mNumThingysDisplay = NULL;
   mMessageDisplay = NULL;
   mEndpoint = NULL;
 #ifdef APPLETALK_SUPPORT
   mEndpointAppletalk = NULL;
 #endif // APPLETALK_SUPPORT
   mAnyoneLoggedOn = false;
   // initialize the connected player endpoints
   for (int i = 0; i < MAX_PLAYERS_CONNECTED; i++) {
      mPlayerEndpoints[i] = NULL; // nobody logged on here
   }
   // setup palette
   PaletteHandle ph = ::GetNewPalette(pltt_DefaultColors);
   for (int i = 0; i < kNumUniquePlayerColors; i++) {
      ::GetEntryColor(ph, i*2+1, &mColorArray[i]);
   }
   ::DisposePalette(ph);
}

GalacticaHostDoc::~GalacticaHostDoc() {
    DEBUG_OUT("~GalacticaHostDoc "<<(void*)this, DEBUG_IMPORTANT);
	StartClosing(); // v2.1.3r5, make sure everyone knows we are going away
	StopIdling();
	StopRepeating();
    if (mWindow) {
        Galactica::Globals::GetInstance().getWindowMenu()->RemoveWindow( mWindow );
    }
}


void
GalacticaHostDoc::SpendTime(const EventRecord& inMacEvent) {
   if (mDoingWork) {      // v2.0.4, we are in the middle of processing the EOT.
      return;
   }
   if (CStarMapView::IsAwaitingClick() ) {   // don't do any periodical processing while tracking mouse
      return;
   }
   if (mQuitPending) {   // we've just been waiting for a chance to quit
      ObeyCommand(cmd_Quit, NULL);
      return;
   }
   GalacticaHost::SpendTime(inMacEvent);
}


// hack to read saved game file into host
OSErr
GalacticaHostDoc::Open(FSSpec &inFileSpec) {
   OSErr result = noErr;
   DEBUG_OUT("SingleDoc::Open", DEBUG_IMPORTANT);
   try {
      FSSpec dataSpec, indexSpec;
      mFile = new LFileStream(inFileSpec);   // create the file object
      MakeWindow();
      LoadFromSavedTurnFile();
      ShowHostWindow();
      LStr255 s = mGameInfo.gameTitle;
      // ask user for name to save database as
      if (!GalacticaApp::AskNewGameFile(dataSpec, indexSpec, s) )   {
         delete this;
         return userCanceledErr;
      }
      UInt32 lastID = mGameInfo.sequenceNumber;
      UInt32 firstID;
      NewGameInfoT info;
      LStr255::CopyPStr(mGameInfo.gameTitle, info.gameTitle, 255);
      info.numComputers = mGameInfo.numComputers;
      info.density = mGameInfo.sectorDensity;
      info.sectorSize = mGameInfo.sectorSize;
      info.compSkill = mGameInfo.compSkill;
      info.compAdvantage = mGameInfo.compAdvantage;
      info.maxTimePerTurn = 0;
      info.hosting = true;
      info.fastGame = mGameInfo.fastGame;
      info.omniscient = mGameInfo.omniscient;
      SInt32 numHumans = mGameInfo.totalNumPlayers;
      UModalDialogs::AskForOneNumber(this, window_SetPopulation, 100, numHumans);
      info.numHumans = numHumans;
      if (info.numHumans > mGameInfo.totalNumPlayers) {
         info.numHumans = mGameInfo.totalNumPlayers;
      }
      info.numComputers = mGameInfo.totalNumPlayers - numHumans;
      DoCreateNew(dataSpec, indexSpec, info);   // create the host files
      // now write all the items out to the host
      firstID = mGameDataFile->GetLastRecordID() + 1;
      for (UInt32 currID = firstID; currID <= lastID; currID++) {
         AGalacticThingy* it = FindThingByID(currID);
         if (it) {
            DEBUG_OUT("===== Clearing pane id for " << it, DEBUG_DETAIL);
            it->SetID(PaneIDT_Unspecified);
            it->WriteToDataStore();
            ASSERT(it->GetID() == currID);
         } else {
            DEBUG_OUT("===== Creating bogus record id " << currID, DEBUG_DETAIL);
            it = new CStar(mStarMap, this);
            it->WriteToDataStore();
            it->DeleteFromDataStore();
            it->RemoveSelfFromGame();
            delete it;
         }
      }
      // remove the stuff that didn't make it into the
      // database because their IDs were too low.
      for (UInt32 currID = 0; currID < firstID; currID++) {
         AGalacticThingy* it = FindThingByID(currID);
         if (it) {
            DEBUG_OUT("===== Removing unsalvageable " << it, DEBUG_DETAIL);
            it->RemoveSelfFromGame();
            delete it;
         }
      }
      WriteChangesToHost();
      StartIdling();
   }
   catch(LException &e) {
      DEBUG_OUT("PP Exception "<<e.what()<<" trying to load host from saved game file", DEBUG_ERROR);
      delete this;
      result = e.GetErrorCode();
   }
   catch(std::exception &e) {
      DEBUG_OUT("std::exception "<<e.what()<<" trying to load host from saved game file", DEBUG_ERROR);
      delete this;
      result = -1;
   }
   return result;
}

OSErr
GalacticaHostDoc::Open(FSSpec &inDataSpec, FSSpec &inIndexSpec) {
   StDialogHandler readingGalaxyDialog(window_ReadingGalaxy, this);
   readingGalaxyDialog.GetDialog()->Draw(nil);
  #if TARGET_API_MAC_CARBON
   GrafPtr dstPort = readingGalaxyDialog.GetDialog()->GetMacPort();
   ::QDFlushPortBuffer(dstPort, nil);
  #endif
   OSErr result = noErr;
   DEBUG_OUT("GalacticHost::Open", DEBUG_IMPORTANT);
   try {
      SpecifyHost(inDataSpec, inIndexSpec);   // create the database file objects
      OpenHost();   // get exclusive write and mark ourselves as host
      LoadGameInfo();
      mGameIndexFile->SetTypeAndVersion(type_GameIndexFile, version_IndexFile);
      mGameDataFile->SetTypeAndVersion(type_GameDataFile, version_GameFile);
      SaveGameInfo(true);   // we have a host now
      InitializeServer();
      MakeWindow();
      ReadGameData();
      ResumeTurnClock();
      RecalcAndRedrawEverything();
      ShowHostWindow();
      StartIdling();
   }
   catch(LException &e) {
      DEBUG_OUT("PP Exception "<<e.what()<<" trying to create host", DEBUG_ERROR | DEBUG_DATABASE);
      delete this;
      result = e.GetErrorCode();
   }
   catch(std::exception &e) {
      DEBUG_OUT("std::exception "<<e.what()<<" trying to create host", DEBUG_ERROR | DEBUG_DATABASE);
      delete this;
      result = -1;
   }
   return result;
}


void
GalacticaHostDoc::RecalcAndRedrawEverything() {
   AGalacticThingy* aThingy;
   LArrayIterator iterator(mEverything, LArrayIterator::from_Start); // recalc position on all items
   bool isVisible = mWindow->IsVisible();   // v1.2b8, check for host window visible
   while (iterator.Next(&aThingy)) {
      NotifyBusy();      // put up our busy cursor.
      aThingy = ValidateThingy(aThingy);
      ASSERT(aThingy != nil);
      if (!aThingy) {
         continue;
      }
      if (isVisible) {   // v1.2b8 don't calc position unless window is visible
         AGalacticThingyUI* ui = aThingy->AsThingyUI();
         if (ui) {
            ui->CalcPosition(true);
         }
      }
   }
   GalacticaHostDoc::UpdateDisplay();
}

OSErr
GalacticaHostDoc::DoNewGame(NewGameInfoT &inGameInfo) {
   FSSpec dataSpec, indexSpec;
   LStr255 s = inGameInfo.gameTitle;
   // ask user for name to save database as
   if (!GalacticaApp::AskNewGameFile(dataSpec, indexSpec, s) )   {
      return userCanceledErr;
   }
   StDialogHandler buildingGalaxyDialog(window_BuildingGalaxy, this);
   buildingGalaxyDialog.GetDialog()->Draw(nil);
 #if TARGET_API_MAC_CARBON
   GrafPtr dstPort = buildingGalaxyDialog.GetDialog()->GetMacPort();
   ::QDFlushPortBuffer(dstPort, nil);
 #endif
   OSErr result = noErr;
   try {
      DoCreateNew(dataSpec, indexSpec, inGameInfo);   // create the host files
      MakeWindow();
      InitNewGameData(inGameInfo);   // could pass in the dialog to have updates occur
      UpdateDisplay();
      ShowHostWindow();
      StartIdling();
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
GalacticaHostDoc::ShowHostWindow() {
   if (mWindow) {
      AdjustWindowSize();
      Galactica::Globals::GetInstance().getWindowMenu()->InsertWindow( mWindow );
      mWindow->Show();
      LCommander::SwitchTarget(this);
      mStarMap->ZoomAllSubPanes(0);
   }
}

void
GalacticaHostDoc::AdjustWindowSize() {   // adjust the window to match the state of the Full Screen item
   WindowPtr windP = mWindow->GetMacWindow();
   Rect strucRect = UWindows::GetWindowStructureRect(windP);
   GDHandle dominantDevice = UWindows::FindDominantDevice(strucRect);
   Rect screenRect = (**dominantDevice).gdRect;
   Boolean fullScreen = Galactica::Globals::GetInstance().getFullScreenMode();
   if (dominantDevice == ::GetMainDevice()) {
      screenRect.top += ::GetMBarHeight(); // don't overwrite menu bar on main device
    #if !TARGET_OS_WIN32
      if (!fullScreen && (UEnvironment::GetOSVersion() >= 0x1000)) {  // On MacOS X we need to account for the dock
         screenRect.bottom -= 60; // just guessing that 80 pixels is good
      }
    #endif
   }
   ((GalacticaWindow*)mWindow)->UseFullScreen(fullScreen);      // let the window know how to behave
   if (fullScreen) {
      mWindow->DoSetBounds(screenRect);   // always resize to screen in full screen mode
   } else {            // otherwise, check to see if the window is within the screen because
      Boolean resize = ! ::MacPtInRect(topLeft(strucRect), &screenRect);   // we don't want to resize
      if (!resize) {                                       // it to the standard size
         resize = ! ::MacPtInRect(botRight(strucRect), &screenRect);   // if it already fits and we
      }                                                               // aren't in full screen mode
      if (resize) {                                       
         mWindow->CalcStandardBoundsForScreen(screenRect, strucRect);
         mWindow->DoSetBounds(strucRect);
      }
   }
}

void
GalacticaHostDoc::MakeWindow() {
   DEBUG_OUT("HostDoc::MakeWindow", DEBUG_IMPORTANT);
   StartStreaming();
   mWindow = LWindow::CreateWindow(window_HostWindow, this);
   StopStreaming();
   if (mGameDataFile) {
      LStr255 s;
      LStr255 hs(STRx_General, str_Host);
      c2pstrcpy(s, static_cast<CVarDataFile*>(mGameDataFile)->GetName());
      s += "\p - " + hs;
      mWindow->SetDescriptor(s);         // make the window title reflect the saved turn filename
   }
   mStarMap = (CStarMapView*) mWindow->FindPaneByID(view_StarMap);
   mStarMap->SetSectorSize(mGameInfo.sectorSize);
   mStarMap->SetDisplayMode(2);   // display tech levels
   static_cast<GalacticaWindow*>(mWindow)->SetStarMap(mStarMap);
   mTurnNumDisplay = mWindow->FindPaneByID(caption_TurnNum);
   ThrowIfNil_(mTurnNumDisplay);
   mNumHumansDisplay = mWindow->FindPaneByID(caption_NumHumans);
   ThrowIfNil_(mNumHumansDisplay);
   mNumCompsDisplay = mWindow->FindPaneByID(caption_NumComps);
   ThrowIfNil_(mNumCompsDisplay);
   mNumThingysDisplay = mWindow->FindPaneByID(caption_NumThings);
   ThrowIfNil_(mNumThingysDisplay);
   mMessageDisplay = mWindow->FindPaneByID(caption_HostMessage);
   ThrowIfNil_(mMessageDisplay);
   LCaption *timeCaption = (LCaption*) mWindow->FindPaneByID(caption_TimeDisplay);
   mAutoEndTurnTimer.SetDisplayPane(timeCaption);
   LButton *theZoomInButton = (LButton*) mWindow->FindPaneByID(cmd_ZoomIn);
   LButton *theZoomOutButton = (LButton*) mWindow->FindPaneByID(cmd_ZoomOut);   
   if (mStarMap != nil) { // make sure the starmap is ready to go
      if (theZoomInButton != nil) {
         theZoomInButton->AddListener(mStarMap);
      }
      if (theZoomOutButton != nil) {
         theZoomOutButton->AddListener(mStarMap);
      }
   }
}


void
GalacticaHostDoc::UpdateDisplay() {
   // find panes and set the appropriate values to reflect the state of the game
   UInt32 numThingys = mEverything.GetCount();
   mTurnNumDisplay->SetValue(mGameInfo.currTurn);
   mNumHumansDisplay->SetValue(mGameInfo.numHumansRemaining);
   mNumCompsDisplay->SetValue(mGameInfo.numComputersRemaining);
   mNumThingysDisplay->SetValue(numThingys);
}

RGBColor*
GalacticaHostDoc::GetColor(int owner) {
   if (owner < 0) owner = 0;
   if (owner >= kNumUniquePlayerColors) owner = kNumUniquePlayerColors-1;
   return &mColorArray[owner];
}

SInt32 
GalacticaHostDoc::GetColorIndex(int inOwner) {   // inSelected
   if (inOwner < 0) inOwner = 0;
   if (inOwner >= kNumUniquePlayerColors) inOwner = kNumUniquePlayerColors-1;
   return(inOwner);
}

void
GalacticaHostDoc::SetStatusMessage(int inMessageNum, const char* inExtra) {
   if (mMessageDisplay) {
      LStr255 s(STRx_Status, inMessageNum);   // get Processing computer player string.
      s += inExtra;
      mMessageDisplay->Refresh();
      mMessageDisplay->SetDescriptor(s);
      mMessageDisplay->UpdatePort();
   }
}

Boolean
GalacticaHostDoc::AttemptQuitSelf(SInt32 ) { //inSaveOption
   bool allowQuit = true;
   if (mDoingWork) {
      mQuitPending = true;   // we are doing work, can't quit yet
      allowQuit = false;      // but we will quit as soon as we are done
   }
   if (allowQuit) {
      delete this;
   }
   return allowQuit;
}

void
GalacticaHostDoc::FindCommandStatus(CommandT inCommand, Boolean &outEnabled, 
                        Boolean &outUsesMark, 
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
      case cmd_ChangeSettings:
      case cmd_ChangePlayer:
      case cmd_EndTurnNow:
//      case cmd_PauseResume:
         outEnabled = true;
         outUsesMark = false;
         outMark = ' ';
         break;
      case cmd_EndTurn:
      case cmd_Save:
      case cmd_SaveAs:
      case cmd_Print:
      case cmd_LoginAs:
      case cmd_Revert:
      case cmd_NewRendezvous:
      case cmd_ShowHideEvents:
      case cmd_GotoHome:
         outEnabled = false;
         outUsesMark = false;
         outMark = ' ';
         break;
      default:
         LSingleDoc::FindCommandStatus(inCommand, outEnabled, outUsesMark, outMark, outName);
         break;
   }
}

Boolean
GalacticaHostDoc::ObeyCommand(CommandT inCommand, void *ioParam) {
   bool cmdHandled = true;
   CStar* theStar;

   switch (inCommand) {

      case cmd_ChangeSettings: 
         {
            CGameSetupAttach theSetupAttach(&mGameInfo);
            ResIDT windID = window_HostNewGame;
            UInt32 oldMaxTime = mGameInfo.maxTimePerTurn;  // save this, we can't change it directly
            long response = MovableAlert(windID, this, 0, false, &theSetupAttach);   // display new game setup wind
            if (response == button_NewGameBegin) { // user pushed the update setting button
               UInt32 newMaxTime = mGameInfo.maxTimePerTurn;
               mGameInfo.maxTimePerTurn = oldMaxTime;    // restore this and change it the right way
               UpdateMaxTimePerTurn(newMaxTime);         // this is the right way to change max time
               SaveGameInfo();
               BroadcastGameInfo();
               mHostLog << pdg::log::category("ADMIN") << pdg::log::inform
                  << "Admin changing game info" << pdg::endlog;
            }
         }
         break;

      case cmd_ChangePlayer: 
         {
            CChangePlayerAttach theChangePlayerAttach(this);
            ResIDT windID = window_ChangePlayer;
            MovableAlert(windID, this, 0, false, &theChangePlayerAttach);   // display new game setup wind
         }
         break;

      case cmd_EndTurnNow: 
         {
   			bool bAskToSkip = false;
   			bool bCanEndTurn = true;
   			std::string skipNames;
   			int skipCount = 0;
    			for (int i = 1; i <= mGameInfo.totalNumPlayers ; i++) {
               PublicPlayerInfoT info;
               PrivatePlayerInfoT privateInfo;
               GetPlayerInfo(i, info);
               GetPrivatePlayerInfo(i, privateInfo);
               if (info.isAssigned && info.isAlive && !info.isComputer 
                 && (privateInfo.lastTurnPosted < GetTurnNum()) ) {
                  bAskToSkip = true;
                  if (skipCount > 0) {
                     skipNames += " & ";
                  }
                  skipCount++;
                  skipNames += info.name;
               }
    			}
    			if (bAskToSkip) {
   			   bCanEndTurn = false;
               MovableParamText(0, skipNames.c_str());
   				if ( MovableAlert(window_AskSkipPlayer) == 1 ) {
   			      bCanEndTurn = true;
   				}
   			}
   			if (bCanEndTurn) {
               HandleAdminEndTurnNow();
	         }
         }
         break;

      case cmd_SetPopulation:
      {
         theStar = (CStar*)mSelectedThingy;
         SInt32 pop = theStar->GetPopulation();
         if (UModalDialogs::AskForOneNumber(this, window_SetPopulation, 100, pop)) {
            theStar->SetPopulation(pop);
            theStar->Refresh();
            theStar->WriteToDataStore();
         }
         break;
      }
      case cmd_SetTechLevel:
      {
         theStar = (CStar*)mSelectedThingy;
         SInt32 level = theStar->GetTechLevel();
         if (UModalDialogs::AskForOneNumber(this, window_SetTechLevel, 100, level)) {
            theStar->SetTechLevel(level);
			theStar->SetScannerRange(UThingyClassInfo::GetScanRangeForTechLevel(level, IsFastGame(), HasFogOfWar()));
            theStar->ScannerChanged();
            theStar->UpdateWhatIsVisibleToMe();
			theStar->CalcPosition(kRefresh);
            theStar->WriteToDataStore();
         }
         break;
      }
      case cmd_SetOwner:
      {
         theStar = (CStar*)mSelectedThingy;
         SInt32 owner = theStar->GetOwner();
         if (UModalDialogs::AskForOneNumber(this, window_SetOwner, 100, owner)) {
            if (owner > mGameInfo.totalNumPlayers) {
               SysBeep(1);
            } else {
               theStar->SetOwner(owner);
               if (theStar->GetPopulation() == 0) {
                    theStar->SetPopulation(1);
               }
               if (theStar->GetTechLevel() == 0) {
                    theStar->SetTechLevel(1);
               }
               theStar->Refresh();
               theStar->WriteToDataStore();
            }
         }
         break;
      }
      case cmd_NewShipAtStar:
      {
         theStar = (CStar*)mSelectedThingy;
         SInt32 hull = theStar->GetBuildHullType() + 1;
         if (AskForOneValue(this, window_ChooseNewShipType, 100, hull)) {
            theStar->BuildNewShip((EShipClass)(hull - 1));      
            theStar->Refresh();
            theStar->WriteToDataStore();
         }
         break;
      }
      case cmd_Delete:   // delete a thingy
         if (mSelectedThingy) {
            mSelectedThingy->Refresh();
            if (mSelectedThingy->GetThingySubClassType() == thingyType_Star) {
                mSelectedThingy->DeleteFromDataStore();
                delete mSelectedThingy;
            } else {
                mSelectedThingy->Die();
            }
            SetSelectedThingy(nil);
         }
         break;


      case cmd_ZoomIn:
      case cmd_ZoomOut:
      case cmd_ZoomFill:
      case cmd_DisplayNone:
      case cmd_DisplayProduct:
      case cmd_DisplayTech:
      case cmd_DisplayDefense:
      case cmd_DisplayDanger:
      case cmd_DisplayGrowth:
      case cmd_DisplayShips:
      case cmd_DisplayResearch:
         mStarMap->ListenToMessage(inCommand, nil);
         break;

       default:
         cmdHandled = LSingleDoc::ObeyCommand(inCommand, ioParam);
         break;        
   }
   return cmdHandled;   
}


void
GalacticaHostDoc::DoEndTurn() {
   // do the actual end of turn
   GalacticaHost::DoEndTurn();
    // now do the UI stuff needed
//   mDisallowCommands = true; // v1.2b11d4, block from handling commands during EOT
   ASSERT(mStarMap != nil);
   UpdateDisplay();
   mStarMap->Refresh();
   SetUpdateCommandStatus(true);   // ¥¥¥Êhack to fix problem where all menus are disabled.
//   mDisallowCommands = false; // v1.2b11d4, block from handling commands during EOT
}


// this function is just here as a hack to allow reading a saved game file into a host
void
GalacticaHostDoc::LoadFromSavedTurnFile() {   
}


