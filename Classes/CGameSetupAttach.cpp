//	CGameSetupAttach.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Changes:
//	4/15/02, ERZ, 2.1b7, separated from Galactica.cpp

#include "CGameSetupAttach.h"
#include "GalacticaTutorial.h"
#include "pdg/sys/config.h"
#include "GalacticaGlobals.h"

#ifndef NO_GAME_RANGER_SUPPORT
  #include "OpenPlay.h"
  #define __NETSPROCKET__ // so GameRanger.h doesn't include it
  #include "GameRanger.h"
#endif // not defined NO_GAME_RANGER_SUPPORT


CGameSetupAttach::CGameSetupAttach(bool isHosting)
:CDialogAttachment(msg_AnyMessage, true, false), mCurrGameInfoP(NULL), mIsHosting(isHosting) {
}

CGameSetupAttach::CGameSetupAttach(GameInfoT* ioGameSettings)
:CDialogAttachment(msg_AnyMessage, true, false), mCurrGameInfoP(ioGameSettings), mIsHosting(true) {
}

#if BALLOON_SUPPORT
#define ATTACH_HELP(VAR_NAME, VAR_TYPE, PANE_ID, HELP_STR_NUM, EXTRA)		\
	VAR_NAME = (VAR_TYPE*)mDialog->FindPaneByID(PANE_ID);					\
	if (VAR_NAME) {															\
		aHelpAttach = new CHelpAttach(STRx_GameSetupHelp, HELP_STR_NUM);	\
		VAR_NAME->AddAttachment(aHelpAttach);								\
		EXTRA;																\
	}
#else
// no balloon support, we don't add any help text here
#define ATTACH_HELP(VAR_NAME, VAR_TYPE, PANE_ID, IGNORE_HELP_STR_NUM, EXTRA)		\
	VAR_NAME = (VAR_TYPE*)mDialog->FindPaneByID(PANE_ID);					\
	if (VAR_NAME) {															\
		EXTRA;																\
	}
#endif // BALLOON_SUPPORT


bool
CGameSetupAttach::PrepareDialog() {
	CHelpAttach* aHelpAttach = nil;
	LControl *theControl;
	pdg::ConfigManager* config = Galactica::Globals::GetInstance().getConfigManager();

  #if !TARGET_API_MAC_CARBON
	LDefaultOutline *theOutline;
	// Setup buttons
	ATTACH_HELP(theOutline, LDefaultOutline,		// attach to default outline rather than
					PaneIDT_Unspecified,	1, );	// okay button or it won't show up.
  #endif // !TARGET_API_MAC_CARBON
	ATTACH_HELP(theControl, LControl,
					button_NewGameBegin,	1, );
   if (mCurrGameInfoP) {   // reconfiguring, change button to read update
      std::string buttonName;
      LoadStringResource(buttonName, STRx_General, str_Update);
      Str255 s;
      c2pstrcpy(s, buttonName.c_str());
      theControl->SetDescriptor(s);
   }
   
	ATTACH_HELP(theControl, LControl,
					button_NewGameCancel,	2, );

	// Setup text entry fields
	ATTACH_HELP(mGameTitle, LEditField,				// GAME TITLE ENTRY
					editText_GameTitle,		15, );
	ATTACH_HELP(mNumHumans, LEditField,				// NUM HUMAN PLAYERS ENTRY
					editText_NumHumans,		9, 
					mNumHumans->SetValue(2) );
	ATTACH_HELP(mNumComputers, LEditField,			// NUM COMP PLAYERS ENTRY
					editText_NumComputers,	10, );
	ATTACH_HELP(mMaxTime, LEditField, 				// NUM HUMAN PLAYERS ENTRY
					editText_MaxTurnLength,	11, 
					mMaxTime->SetValue(5) );

	// Setup popup menus
	ATTACH_HELP(mCompSkill, LStdPopupMenu,			// COMP SKILL POPUP
					popupMenu_CompSkill,		3, );
	ATTACH_HELP(mCompAdvantage, LStdPopupMenu,		// COMP ADVANTAGE POPUP
					popupMenu_CompAdvantage,	4, );
	ATTACH_HELP(mDensity, LStdPopupMenu,			// DENSITY POPUP
					popupMenu_Density,			6, );
	ATTACH_HELP(mSectorSize, LStdPopupMenu,			// SECTOR SIZE POPUP
					popupMenu_SectorSize,		5, );
	ATTACH_HELP(mTimeUnits, LStdPopupMenu,			// TIME UNITS POPUP
					popupMenu_TimeUnits,		7, );
	if (mTimeUnits) {
	    mTimeUnits->AddListener(this);
	}

	// Setup checkboxes
	ATTACH_HELP(mTimeLimit, LStdCheckBox,			// LIMIT TURN LENGTH CHECKBOX
					checkbox_LimitTurns,		8, );
	ATTACH_HELP(mFastGame, LStdCheckBox,			// FAST GAME CHECKBOX
					checkbox_FastGame,			13, );
	ATTACH_HELP(mFogOfWar, LStdCheckBox,			// OMNISCIENT CHECKBOX
					checkbox_FogOfWar,  		14, );
    // fog of war support
    bool tempBool;
    if (mFogOfWar) {
		config->getConfigBool(GALACTICA_PREF_FOG_OF_WAR, tempBool);
        mFogOfWar->SetValue(tempBool);
    }
    // read from config
    if (mFastGame) {
		config->getConfigBool(GALACTICA_PREF_FAST_GAME, tempBool);
        mFastGame->SetValue(tempBool);
    }
    SInt32 val;
	if (mCompSkill) {
		config->getConfigLong(GALACTICA_PREF_AI_SKILL, val);
		mCompSkill->SetValue(val);
	}
	if (mCompAdvantage) {
		config->getConfigLong(GALACTICA_PREF_AI_ADVANTAGE, val);
		mCompAdvantage->SetValue(val);
	}
	if (mDensity) {
		config->getConfigLong(GALACTICA_PREF_MAP_DENSITY, val);
		mDensity->SetValue(val);
	}
	if (mSectorSize) {
		config->getConfigLong(GALACTICA_PREF_MAP_SIZE, val);
		mSectorSize->SetValue(val);
	}

	// Other setup
	mTimeLimitView = (LView*)mDialog->FindPaneByID(view_TimeLimit);
	mEndEarly = (LStdCheckBox*)mDialog->FindPaneByID(checkbox_EndEarlyIfAllPosted);

  #if TUTORIAL_SUPPORT
	// v1.2fc6, tutorial, if user chooses New in response to page 2,
	// go ahead to next page
	if (Tutorial::TutorialIsActive()) {
		Tutorial* t = Tutorial::GetTutorial();
		if (t->GetPageNum() == tutorialPage_ChooseNewGame) {
			if (mDialog->GetPaneID() == window_NewGame) {
				t->NextPage();
			} else {
				t->GotoPage(tutorialPage_NoMultiplayerTutorial);
			}
		}
		// v2.0.6, always make tutorial be a fast game
        if (mFastGame) {
			mFastGame->SetValue(Button_On);
	    }
	    // v2.3, tutorial game is omniscient by default
	    if (mFogOfWar) {
	        mFogOfWar->SetValue(Button_Off);
	    }
	}
	// end tutorial
  #endif //TUTORIAL_SUPPORT
  
  #ifndef NO_GAME_RANGER_SUPPORT
    // see if this is a GameRanger host command
    if (GRIsHostCmd()) {
        DEBUG_OUT("Prepare Dialog: GameRanger host command detected", DEBUG_IMPORTANT);
        if (mNumHumans) {
            mNumHumans->SetValue(GRGetHostMaxPlayers());
            mNumHumans->Disable();
            DEBUG_OUT("GameRanger wanted " << GRGetHostMaxPlayers() <<" humans", DEBUG_IMPORTANT);
        }
        if (mGameTitle) {
            LStr255 s;
            c2pstrcpy(s, GRGetHostGameName());
            mGameTitle->SetDescriptor(s);
            mGameTitle->Disable();
            DEBUG_OUT("GameRanger wanted the game to be named \"" << GRGetHostGameName() <<"\"", DEBUG_IMPORTANT);
        }
    }
  #endif // NO_GAME_RANGER_SUPPORT
  
   // see if we are reconfiguring a game in progress
   if (mCurrGameInfoP) {
      DEBUG_OUT("Prepare Dialog: Changing game settings", DEBUG_IMPORTANT);
      if (mGameTitle) {
         mGameTitle->SetDescriptor(mCurrGameInfoP->gameTitle);
         mGameTitle->Disable();
      }
      if (mNumHumans) {
         mNumHumans->SetValue(mCurrGameInfoP->numHumans);
         mNumHumans->Disable();
      }
      if (mNumComputers) {
         mNumComputers->SetValue(mCurrGameInfoP->numComputers);
         mNumComputers->Disable();
      }
      if (mCompSkill) {
         mCompSkill->SetValue(mCurrGameInfoP->compSkill);
      }
      if (mCompAdvantage) {
        // computer advantage   menu item:  1  2  3  4  5  6  7
      	                        // value: +3 +2 +1  0 -1 -2 -3
         mCompAdvantage->SetValue(-(mCurrGameInfoP->compAdvantage - 4));
      }
      if (mDensity) {
         mDensity->SetValue(mCurrGameInfoP->sectorDensity);
         mDensity->Disable(); // can't change density
      }
      if (mSectorSize) {
         mSectorSize->SetValue(mCurrGameInfoP->sectorSize);
         mSectorSize->Disable(); // can't change sector size
      }
      if (mFastGame) {
         mFastGame->SetValue(mCurrGameInfoP->fastGame);
      }
      if (mFogOfWar) {
         mFogOfWar->SetValue(!mCurrGameInfoP->omniscient);
      }
      bool hasTimeLimit = (mCurrGameInfoP->maxTimePerTurn != 0);
      if (mTimeLimit) {
         mTimeLimit->SetValue(hasTimeLimit);
      }
      if (hasTimeLimit) {
   		int timeMul = 60;	// multiplier to convert from our desired units to seconds
   		int itemNum = 1;
         if ((mCurrGameInfoP->maxTimePerTurn / 604800) > 0) {  // weeks
            timeMul = 604800;
            itemNum = 4;
         } else if ((mCurrGameInfoP->maxTimePerTurn / 86400) > 0) {  // days
            timeMul = 86400;
            itemNum = 3;
         } else if ((mCurrGameInfoP->maxTimePerTurn / 3600) > 0) {   // hours
            timeMul = 3600;
            itemNum = 2;
         }
         if (mTimeUnits) {
            mTimeUnits->SetValue(itemNum);      // choose the min, hrs, days or weeks item
         }
         if (mMaxTime) {
            long timeVal = mCurrGameInfoP->maxTimePerTurn / timeMul;
            mMaxTime->SetValue(timeVal);
         }
         if (mEndEarly) {
            mEndEarly->SetValue(mCurrGameInfoP->endTurnEarly);
         }
      }
   }
	if (mTimeLimitView) {		// enable and disable turn limit stuff
		if (mTimeLimit->GetValue() == Button_On) {
			mTimeLimitView->Enable();
		} else {
			mTimeLimitView->Disable();
		}
	}
    
	return true;
}

bool
CGameSetupAttach::CloseDialog(MessageT inMsg) {
	Boolean canClose = false;		// this is called each time a control within the dialog
	switch (inMsg) {				// broadcasts a message
		case button_NewGameBegin:
			NewGameInfoT gameInfo;
			GetInfo(&gameInfo);
         if (mCurrGameInfoP != NULL) {
 		      DEBUG_OUT("CGameSetupAttach::CloseDialog: Accepted altered settings", DEBUG_IMPORTANT);
            mCurrGameInfoP->compSkill = gameInfo.compSkill;
            mCurrGameInfoP->compAdvantage = gameInfo.compAdvantage;
            mCurrGameInfoP->maxTimePerTurn = gameInfo.maxTimePerTurn;
            mCurrGameInfoP->fastGame = gameInfo.fastGame;
            mCurrGameInfoP->omniscient = gameInfo.omniscient;
            mCurrGameInfoP->endTurnEarly = gameInfo.endEarly;
            canClose = true;
         } else {
    			try {
    			    canClose = mDialog->ObeyCommand(cmd_DoNewGame, &gameInfo);
    			}
    			catch (...) {
    			    DEBUG_OUT("CGameSetupAttach::CloseDialog: Game Creation failed", DEBUG_ERROR);
    			}
			}
			break;
		case button_NewGameCancel:
			canClose = true;
			break;
		case checkbox_LimitTurns:
			if (mTimeLimitView) {		// enable and disable turn limit stuff
				if (mTimeLimit->GetValue() == Button_On) {
					mTimeLimitView->Enable();
				} else {
					mTimeLimitView->Disable();
				}
			}
			break;
/*		case popupMenu_CompSkill:		// may want to add some dynamic feedback at some point
		case popupMenu_CompAdvantage:
		case checkbox_FastGame:
		case checkbox_Omniscient:
		case popupMenu_Density:
		case popupMenu_SectorSize:
		case popupMenu_TimeUnits: */
		default:
			break;	// don't do anything with any other messages
	}
	return canClose;
}

void
CGameSetupAttach::ExecuteSelf(MessageT inMessage, void* ioParam)
{
    if (inMessage == msg_CommandStatus) {
    	mExecuteHost = true;
    	if ((static_cast<SCommandStatusP>(ioParam))->command == cmd_GalacticaTutorial) {
    									// This is our command, enable it
    		*(static_cast<SCommandStatusP>(ioParam))->enabled = true;
    		mExecuteHost = false;		// We have enabled the command, so don't
    									//   bother to ask the host
    	}
	} else {
	    CDialogAttachment::ExecuteSelf(inMessage, ioParam);
	}
}

void
CGameSetupAttach::ListenToMessage(MessageT inMessage, void* ioParam) {
    if (inMessage == popupMenu_TimeUnits) {
        SInt32 value = *((SInt32*)(ioParam));
        if (mEndEarly) {
            if (value > 1) {  // check for time units being set to hours, days or weeks
                mEndEarly->SetValue(Button_Off);
            } else {
                mEndEarly->SetValue(Button_On);
            }
        }
    }
}

void
CGameSetupAttach::GetInfo(NewGameInfoT *outInfo) {
	outInfo->hosting = mIsHosting;	// is this a multiplayer game or not?
	// number of human players
	if (mGameTitle) {
		mGameTitle->GetDescriptor(outInfo->gameTitle);
	} else {
		outInfo->gameTitle[0] = 0;
	}
	if (mNumHumans) {
		outInfo->numHumans = mNumHumans->GetValue();
		if (outInfo->numHumans > 255) {		// limited to 255 humans
			outInfo->numHumans = 255;
		} else if (outInfo->numHumans < 0) {	// limited to 255 humans
			outInfo->numHumans = 0;
		}
	} else {
		outInfo->numHumans = 1;
	}
	// number of computer opponents
	if (mNumComputers) {
		outInfo->numComputers = mNumComputers->GetValue();	
		if (outInfo->numComputers > 255) {	// limited to 255 computers
			outInfo->numComputers = 255;
		} else if (outInfo->numComputers < 0) {
			outInfo->numComputers = 0;
		}
	} else {
		outInfo->numComputers = 0;
	}
	// computer skill
	if (mCompSkill) {
		outInfo->compSkill = mCompSkill->GetValue();
	} else {
		outInfo->compSkill = 0;
	}
	// computer advantage    menu item:  1  2  3  4  5  6  7
	if (mCompAdvantage) {	//	 value: +3 +2 +1  0 -1 -2 -3
		int item = mCompAdvantage->GetValue();
		outInfo->compAdvantage = -item + 4;
	} else {
		outInfo->compAdvantage = 0;
	}
	// density of galaxy
	if (mDensity) {
		outInfo->density = mDensity->GetValue();
	} else {
		outInfo->density = 3;
	}
	// sector size
	if (mSectorSize) {
		outInfo->sectorSize = mSectorSize->GetValue();
	} else {
		outInfo->sectorSize = 1;
	}
	// fast game
	if (mFastGame && (mFastGame->GetValue() == Button_On) ) {
		outInfo->fastGame = true;
	} else {
    	// v2.0.6, always make tutorial be a fast game
      #if TUTORIAL_SUPPORT
    	if (Tutorial::TutorialIsActive()) {
    	    outInfo->fastGame = true;
    	} else
      #endif
		outInfo->fastGame = false;
	}
	// omniscient
	if (mFogOfWar && (mFogOfWar->GetValue() == Button_Off) ) {
		outInfo->omniscient = true;
	} else {
		outInfo->omniscient = false;
	}
	// time limit per turn
	if (mTimeLimit && mMaxTime) {
		if (mTimeLimit->GetValue() == Button_Off) {
			outInfo->maxTimePerTurn = 0;
			outInfo->endEarly = true;
		} else {
			int	timeMul = 60;	// multiplier to convert from our desired units to seconds
			if (mTimeUnits) {
				switch (mTimeUnits->GetValue()) {
					case 2:
						timeMul = 3600;	// hours
						break;
					case 3:
						timeMul = 86400;	// days
						break;
					case 4:
						timeMul = 604800;	// weeks
					default:
						timeMul = 60;	// minutes
						break;
				}
			}
			outInfo->maxTimePerTurn = mMaxTime->GetValue() * timeMul;
			if (outInfo->maxTimePerTurn < 0) {
				outInfo->maxTimePerTurn = 1;	// 1 tick per turn
			}
			if (mEndEarly) {
			    outInfo->endEarly = (mEndEarly->GetValue() == Button_On);
			}
		}
	} else {
		outInfo->maxTimePerTurn = 0;
		outInfo->endEarly = true;
	}
}



