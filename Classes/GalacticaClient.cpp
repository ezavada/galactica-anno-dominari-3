//	GalacticaClient.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Changes:
//	9/21/96, ERZ, 1.0.8 fixes integrated
//	1/3/97, ERZ, 1.1 fixes integrated
//  10/7/97, ERZ, 1.2 changes
//  12/11/01, ERZ, 2.1 changes, separated from GalacticaDoc.cp


#include "GalacticaRegistration.h"
#include "GalacticaGlobals.h"
#include "GalacticaClient.h"
#include "AGalacticThingyUI.h"
#include "GalacticaPanes.h"
#include "CStyledText.h"
#include "CTextTable.h"
#include "ProductInformation.h"
#include "CEndpoint.h"
#include "GalacticaUtilsUI.h"
#include <LBevelButton.h>
#include <UModalDialogs.h>
#include <UDesktop.h>
#include <PP_KeyCodes.h>
#include "CGameSetupAttach.h"
#include "CChangePlayerAttach.h"

#include "pdg/sys/config.h"
#include "pdg/sys/os.h"

#include "Galactica.h" // for SavePrefs()

#ifndef NO_GAME_RANGER_SUPPORT
  #define __NETSPROCKET__ // so GameRanger.h doesn't include it
  #include "GameRanger.h"
#endif // not defined NO_GAME_RANGER_SUPPORT

#include <cstdio>
#include <string.h>

#warning FIXME: must rewrite Galactica Client to use CEndpoint, since we have no outgoing packet queue

GameJoinInfo::GameJoinInfo(const char* inGameName, const char* inHostName, const char* inPlayerName, const char* inPassword,
    UInt32 inGameID, NMHostID inOpenPlayHostID, UInt32 inRejoinKey, GameType inGameType, UInt16 inPort)
{
   std::strncpy(gamename, inGameName, MAX_GAME_NAME_LEN);
   std::strncpy(hostname, inHostName, MAX_HOSTNAME_LEN);
   std::strncpy(playername, inPlayerName, MAX_PLAYER_NAME_LEN);
   std::strncpy(password, inPassword, MAX_PLAYER_NAME_LEN);
   gamename[MAX_GAME_NAME_LEN] = 0;
   hostname[MAX_HOSTNAME_LEN] = 0;
   playername[MAX_PLAYER_NAME_LEN] = 0;
   password[MAX_PASSWORD_LEN] = 0;
   gameid = inGameID;
   hostid = inOpenPlayHostID;
   key = inRejoinKey;
   type = inGameType;
   port = inPort;
}

GameJoinInfo::GameJoinInfo()
{
    Clear();   
}

void
GameJoinInfo::Clear()
{
    gamename[0] = 0;
    hostname[0] = 0;
    playername[0] = 0;
    password[0] = 0;
    gameid = 0;
    hostid = 0;
    key = 0;
    type = gameType_LAN;    
}



// =====================================================================================
// CJoinGameDlgHndlr & CChoosePlayerDlgHndlr class declarations
// =====================================================================================

class CChoosePlayerDlgHndlr : public CDialogAttachment, public LListener {
public:
	CChoosePlayerDlgHndlr(GalacticaClient* inGame): mGame(inGame), mDefaultSelection(0), mInfoChanged(false) {}
	virtual bool	PrepareDialog();
	virtual bool	CloseDialog(MessageT inMsg);	// false to prohibit closing
	virtual bool	AutoDelete() {return false;}		// don't let MovableAlert() delete this object
	virtual void	ExecuteSelf(MessageT inMessage, void* ioParam);
	virtual void	ListenToMessage(MessageT inMessage, void* ioParam);
	SInt16          GetSelectedPlayerIndex();
	void	        SetPlayerInfo(int inPlayerNum, PublicPlayerInfoT &inInfo);
	void            GetPlayerNameAndPassword(std::string& outNameStr, std::string& outPasswordStr, bool &outSavePwd);
    void            SetPlayerNameAndPassword(const char* inNameStr, const char* inPasswordStr);
    bool            GetPlayerIsNew() const {return mIsNewPlayer;}
protected:
	GalacticaClient* mGame;
	CTextTable*    mPlayerNamesTable;
    int            mDefaultSelection;
	bool           mInfoChanged;
	LStdButton*    mJoinButton;
	LEditField*    mPlayerName; 
	LEditField*    mPassword; 
	LStdCheckBox*  mSavePassword; 
	std::string    mNameStr;
	std::string    mPasswordStr;
	bool           mIsNewPlayer;
	bool           mSavePwd;
	TArray<Str255> mPlayerNameArray;
	TArray<PublicPlayerInfoT> mPlayerInfoArray;
};

class CJoinGameDlgHndlr : public CDialogAttachment, public LListener {
public:
	CJoinGameDlgHndlr(GalacticaClient* inGame):mGame(inGame),mArrayChanged(false),
	                                mGameInfoChanged(false), mIsConnecting(false), mJoinPending(false),
	                                mChoosePlayerHandler(NULL) {}
	virtual ~CJoinGameDlgHndlr();
	virtual bool	PrepareDialog();
	virtual bool	CloseDialog(MessageT inMsg);	// false to prohibit closing
	virtual bool	AutoDelete() {return false;}		// don't let MovableAlert() delete this object
	void            ArrayChanged() {mArrayChanged = true;} // this may be called at interrupt time
	void            GameInfoChanged() {mGameInfoChanged = true;}
	virtual void	ExecuteSelf(MessageT inMessage, void* ioParam);
	virtual void	ListenToMessage(MessageT inMessage, void* ioParam);
	SInt16          GetSelectedGameIndex();
	void	        SetPlayerInfo(int inPlayerNum, PublicPlayerInfoT &inInfo);
	void            GetPlayerNameAndPassword(std::string& outNameStr, std::string& outPasswordStr);
    void            SetPlayerNameAndPassword(const char* inNameStr, const char* inPasswordStr);
    bool            GetPlayerIsNew() const {return mIsNewPlayer;}
    bool            GetSavePassword() const {return mSavePwd;}
protected:
	GalacticaClient* mGame;
	CTextTable* mHostNamesTable;
	bool mArrayChanged;
	bool mGameInfoChanged;
	bool mIsConnecting;
	bool mJoinPending;
	UInt32 mRejoinKey;
	GameInfoT mGameInfo;
	LCaption* mGameName;
	LCaption* mNumHumans;
	LCaption* mNumComps;
	LCaption* mCompSkill;
	LCaption* mCompAdvantage;
	LCaption* mTurnLimit;
	LCaption* mGameStatus;
	LCaption* mSectorInfo;
	LCaption* mCurrTurn;
	LCaption* mTurnEnds;
	LCaption* mFastGame;
	LCaption* mFogOfWar;
	LStdButton* mJoinButton;
	LStdButton* mDeleteEntryButton;
	LStdButton* mSearchInternetButton;
	CChoosePlayerDlgHndlr* mChoosePlayerHandler;
	std::string mNameStr;
	std::string mPasswordStr;
	bool        mIsNewPlayer;
	bool        mSavePwd;
};



// =====================================================================================
// CJoinGameDlgHndlr
// =====================================================================================

CJoinGameDlgHndlr::~CJoinGameDlgHndlr() {
   if (mChoosePlayerHandler) {  // clean up any choose handler that's hanging about
      delete mChoosePlayerHandler;
      mChoosePlayerHandler = NULL;
   }
};

bool
CJoinGameDlgHndlr::PrepareDialog() {
   // build the host names table
	
   mHostNamesTable = dynamic_cast<CTextTable*> ( mDialog->FindPaneByID(listbox_GameServers) );
   DEBUG_OUT("CJoinGameDlgHndlr Found Host Names Table "<<(void*)mHostNamesTable, DEBUG_IMPORTANT | DEBUG_USER);
	
// captions
   mGameName      = dynamic_cast<LCaption*> ( mDialog->FindPaneByID(caption_GameName) );
   mNumHumans     = dynamic_cast<LCaption*> ( mDialog->FindPaneByID(caption_NumHumans) );
   mNumComps      = dynamic_cast<LCaption*> ( mDialog->FindPaneByID(caption_NumComps) );
   mCompSkill     = dynamic_cast<LCaption*> ( mDialog->FindPaneByID(caption_CompSkill) );
   mGameStatus    = dynamic_cast<LCaption*> ( mDialog->FindPaneByID(caption_GameStatus) );
   mTurnLimit     = dynamic_cast<LCaption*> ( mDialog->FindPaneByID(caption_TurnLimit) );
   mSectorInfo    = dynamic_cast<LCaption*> ( mDialog->FindPaneByID(caption_SectorInfo) );
   mCompAdvantage = dynamic_cast<LCaption*> ( mDialog->FindPaneByID(caption_CompAdvantage) );
   mCurrTurn      = dynamic_cast<LCaption*> ( mDialog->FindPaneByID(caption_CurrTurn) );
   mTurnEnds      = dynamic_cast<LCaption*> ( mDialog->FindPaneByID(caption_TurnEnds) );
   mFastGame      = dynamic_cast<LCaption*> ( mDialog->FindPaneByID(caption_FastGame) );
   mFogOfWar      = dynamic_cast<LCaption*> ( mDialog->FindPaneByID(caption_FogOfWar) );

// buttons
   mJoinButton           = dynamic_cast<LStdButton*>( mDialog->FindPaneByID(msg_OK) );
   mDeleteEntryButton    = dynamic_cast<LStdButton*>( mDialog->FindPaneByID(button_DeleteEntry) );
   if (mDeleteEntryButton) {
        mDeleteEntryButton->Disable();
   }
   mSearchInternetButton = dynamic_cast<LStdButton*>( mDialog->FindPaneByID(button_SearchForInternetGames) );
   if (mHostNamesTable) {
      DEBUG_OUT("CJoinGameDlgHndlr About to install Array", DEBUG_TRIVIA | DEBUG_USER);
      mHostNamesTable->InstallArray(&mGame->GetHostNameArray(), false);
      mHostNamesTable->AddListener(this);    
      DEBUG_OUT("CJoinGameDlgHndlr installed Array "<< (void*)mHostNamesTable->GetArray()
             << " item size = " <<mHostNamesTable->GetArray()->GetItemSize(0), DEBUG_TRIVIA | DEBUG_USER);
      mDoSysBeep = false;	// don't beep 
   } else {
      DEBUG_OUT("Failed to find Host Names Table ", DEBUG_ERROR | DEBUG_USER);
      //	    return false;
   }
	if (mJoinButton) {
	    mJoinButton->Disable();
	}
	return true;
}

void
CJoinGameDlgHndlr::GetPlayerNameAndPassword(std::string& outNameStr, std::string& outPasswordStr) {
    outNameStr = mNameStr;
    outPasswordStr = mPasswordStr;
}

void
CJoinGameDlgHndlr::SetPlayerNameAndPassword(const char* inNameStr, const char* inPasswordStr) {
    mNameStr = inNameStr;
    mPasswordStr = inPasswordStr;
}

SInt16
CJoinGameDlgHndlr::GetSelectedGameIndex() {
    DEBUG_OUT("CJoinGameDlgHndlr::GetSelectedGameIndex", DEBUG_IMPORTANT);
    if (mHostNamesTable) {
        TableCellT cell;
        mHostNamesTable->GetSelectedCell(cell);
        return cell.row;
    } else {
        return -1;
    }
}

bool
CJoinGameDlgHndlr::CloseDialog(MessageT inMsg) {
   DEBUG_OUT("CJoinGameDlgHndlr::CloseDialog", DEBUG_IMPORTANT);
	Boolean result = true;
	if (inMsg == button_EnterIPAddress) {	// user is entering an IP Address Manually
	    LStr255 ipAddrStr;
	    if (UModalDialogs::AskForOneString(mGame, window_EnterIPAddr, 100, ipAddrStr) )  {
            char s[256];
            p2cstrcpy(s, ipAddrStr);
            // look for port embedded in hostname
            char* hostname = std::strtok(s, ":");  // get hostname
            char* token = std::strtok(NULL, ":");  // see if there is anything following a colon separator
            UInt16 port = 0;
            if (token) {
               port = std::atoi(token);
            }
            GameJoinInfo gji("", s, "", "", 0, 0, 0, gameType_Internet, port);
            mGame->AddGameToKnownList(gji);
            ArrayChanged();
            mHostNamesTable->ArrayChanged();
            // now select the cell we just added, just as if the user clicked on it
            TableCellT cell;
            mHostNamesTable->GetTableSize(cell.row, cell.col);
            mHostNamesTable->SelectCell(cell);
	    }
	    result = false;
	} else if (inMsg == button_DeleteEntry) { // user wants to delete an entry from the saved list
	    if (!mIsConnecting) {     // can't do this while searching for a game
            GameJoinInfo theJoinInfo = mGame->GetConnectedHostJoinInfo();
            if (!theJoinInfo.valid) {
                // this game isn't found, so just delete it immediately
                mGame->RemoveGameFromKnownList(theJoinInfo);
                ArrayChanged();
                mGame->SaveKnownGamesList();
            }
        }
        result = false;
	} else if (inMsg == button_SearchForInternetGames) { // user wants to search the internet for available games
	    result = false;
	    mGame->FetchMatchMakerGames();
	    ArrayChanged();
	}
   if (result) {
      if (inMsg == msg_OK) {
         // we are about to close the window and join the game, so we need to select which player is logging in
         if (!mChoosePlayerHandler) {
            DEBUG_OUT("No ChoosePlayerHandler! as we close ChooseGame window", DEBUG_ERROR);
            return false;
         } else {
             // get the player info for this game
         	PlayerInfoRequestPacket* p = CreatePacket<PlayerInfoRequestPacket>();
            mGame->SendPacket(p);
            ReleasePacket(p);
            // and put up the player selection window
            mChoosePlayerHandler->SetPlayerNameAndPassword(mNameStr.c_str(), mPasswordStr.c_str());
         	SInt16 answer = MovableAlert(window_GameJoin, mGame, 0, false, mChoosePlayerHandler );
         	if (answer == msg_OK) {
               mIsNewPlayer = mChoosePlayerHandler->GetPlayerIsNew();
               mChoosePlayerHandler->GetPlayerNameAndPassword(mNameStr, mPasswordStr, mSavePwd);
               DEBUG_OUT("accepted player selection: "<< mNameStr.c_str() << " password ["
                        << mPasswordStr.c_str() << "] "
                        << (char*)(mSavePwd ? "savedpwd" : "nosavepwd"), DEBUG_IMPORTANT);
         	} else {
         	   result = false;
         	   DEBUG_OUT("canceled player selection", DEBUG_IMPORTANT);
         	}
         }
      }
   }
   if (result) {
      // we are closing the browse window, quit browsing
      mGame->StopBrowsingForGames();
   }
	return result;
}

void	
CJoinGameDlgHndlr::ExecuteSelf(MessageT, void*) {
    // we do this now because we know it is safe, whereas the actual changes to the array could
    // have happened at interrupt time.
    if (mArrayChanged) {
        mArrayChanged = false;
        mHostNamesTable->ArrayChanged();
        mHostNamesTable->Refresh();
    }
    // the following section we do whenever the game info changes, which can happen because
    // we chose a different game or because there was an update to the game we have selected
    // (someone joined, turn ended, etc..)
   if (mGameInfoChanged) {
      mGameInfoChanged = false;
      mIsConnecting = false;
      DEBUG_OUT("Changing game info", DEBUG_IMPORTANT);
      mGameInfo = mGame->GetGameInfo();
      GameJoinInfo theJoinInfo = mGame->GetConnectedHostJoinInfo();
      if (!theJoinInfo.valid) {
         // if the game id's mismatched, then this is a different game, and can't be rejoined
         mGame->RemoveGameFromKnownList(theJoinInfo);
         theJoinInfo.key = key_NotRejoining;
         theJoinInfo.gameid = mGameInfo.gameID;
         mGame->AddGameToKnownList(theJoinInfo);
      }
      if (mGameName) {
         mGameName->SetDescriptor(mGameInfo.gameTitle);
      }
      // refresh all the info
      if (mNumHumans) {
         mNumHumans->SetValue(mGameInfo.numHumans);
      }
      if (mNumComps) {
         mNumComps->SetValue(mGameInfo.numComputers);
      }
      // map info
      if (mSectorInfo) {
         LStr255 sizeStr(mGameInfo.sectorSize);
         LStr255 densityStr(mGameInfo.sectorDensity);
         LStr255 infoStr(sizeStr);
         infoStr += "x";
         infoStr += sizeStr;
         infoStr += "x";
         infoStr += densityStr;
         mSectorInfo->SetDescriptor(infoStr);
      }
      if (mCompSkill) {
         std::string skillStr;
         LoadStringResource(skillStr, STRx_SkillLevels, mGameInfo.compSkill);
         Str255 s;
         c2pstrcpy(s, skillStr.c_str());
         mCompSkill->SetDescriptor(s);
      }
      if (mCompAdvantage) {
         mCompAdvantage->SetValue(mGameInfo.compAdvantage);
      }
      if (mCurrTurn) {
         mCurrTurn->SetValue(mGameInfo.currTurn);
      }
      // time per turn
      if (mTurnLimit) {
         UInt32 days = (mGameInfo.maxTimePerTurn / 86400);
      LStr255 s, s2;
      if (days>=1) {
      	s2.Assign(STRx_General, str_Days);
      	s = (long) days;
      	s += s2;
      }
      UInt32 leftover = (mGameInfo.maxTimePerTurn % 86400);
      if (leftover > 0) {
          std::string tmpstr;
      	bool ignore = Secs2TimeStr(leftover, tmpstr, 1);	// always show seconds
      	s.Append(tmpstr.c_str());
      }
         mTurnLimit->SetDescriptor(s);
         // Time when turn ends
         if (mTurnEnds) {
      	LStr255 s;
          std::string tmpstr;
      	bool ignore = Secs2TimeStr(mGameInfo.secsRemainingInTurn, tmpstr, 0);	// never show seconds
      	s.Append(tmpstr.c_str());
             mTurnEnds->SetDescriptor(s);
         }
      }
      // fast game y/n
      if (mFastGame) {
         if (mGameInfo.fastGame) {
             LStr255 fastStrY(STRx_General, str_Yes);
             mFastGame->SetDescriptor(fastStrY);
         } else {
             LStr255 fastStrN(STRx_General, str_No);
             mFastGame->SetDescriptor(fastStrN);
         }
      }
      // fog of war y/n
      if (mFogOfWar) {
         if (!mGameInfo.omniscient) {
             LStr255 fowStrY(STRx_General, str_Yes);
             mFogOfWar->SetDescriptor(fowStrY);
         } else {
             LStr255 fowStrN(STRx_General, str_No);
             mFogOfWar->SetDescriptor(fowStrN);
         }
      }

      bool gameFull = (mGameInfo.gameState & state_FullFlag);

      if (mGameStatus) {
         if (gameFull) {
             LStr255 fullStr(STRx_General, str_Full);
             mGameStatus->SetDescriptor(fullStr);
         } else {
             LStr255 openStr(STRx_General, str_Open);
             mGameStatus->SetDescriptor(openStr);
         }
         if (mJoinButton) {
             if (!gameFull) {
                 LStr255 joinStr(STRx_General, str_Join);
                 mJoinButton->SetDescriptor(joinStr);
             } else {
                 LStr255 rejoinStr(STRx_General, str_Rejoin);
                 mJoinButton->SetDescriptor(rejoinStr);
             }
         }
      }
      if (mJoinPending) {
         mJoinPending = false;
         // simulate a click on the Join button
         if (mJoinButton) {
             mJoinButton->SimulateHotSpotClick(1);
         }
      }
   }
   mExecuteHost = true;
}

void
CJoinGameDlgHndlr::ListenToMessage(MessageT inMessage, void* inParam) {
   if (inMessage == message_CellSelected) {
      // clear the display
      if (mGameName) {
         mGameName->SetDescriptor("\p");
      }
      if (mNumHumans) {
         mNumHumans->SetDescriptor("\p");
      }
      if (mNumComps) {
         mNumComps->SetDescriptor("\p");
      }
      if (mCompSkill) {
         mCompSkill->SetDescriptor("\p");
      }
      if (mTurnLimit) {
         mTurnLimit->SetDescriptor("\p");
      }
      if (mSectorInfo) {
         mSectorInfo->SetDescriptor("\p");
      }
      if (mCompAdvantage) {
         mCompAdvantage->SetDescriptor("\p");
      }
      if (mCurrTurn) {
         mCurrTurn->SetDescriptor("\p");
      }
      if (mTurnEnds) {
         mTurnEnds->SetDescriptor("\p");
      }
      if (mFastGame) {
         mFastGame->SetDescriptor("\p");
      }
      if (mFogOfWar) {
         mFogOfWar->SetDescriptor("\p");
      }
      if (mJoinButton) {
         mJoinButton->Disable();
      }
      CellInfoT* cp = static_cast<CellInfoT*>(inParam);
      if (cp->cell.row != 0) {
         if (mGameStatus) {
            LStr255 s(STRx_General, str_checking);
            mGameStatus->SetDescriptor(s);
            // redraw immediately for better user feeback, since Connect() calls will block
            LView* super = mGameStatus->GetSuperView();
            if (super) {
               super->Refresh();
               super->Draw(NULL);
            }
          #if TARGET_API_MAC_CARBON
            GrafPtr dstPort = mGameStatus->GetMacPort();
            ::QDFlushPortBuffer(dstPort, NULL);
          #endif
         }
         GameJoinInfo theJoinInfo;
         mGame->GetHostArray().FetchItemAt(cp->cell.row, theJoinInfo);
         mNameStr = theJoinInfo.playername;
         mPasswordStr = theJoinInfo.password;
         // connect to the host now that we have our join info
         OSErr err = mGame->ConnectToHost(theJoinInfo);
         // immediately send a request for the game info
         if (err == noErr) {
            mIsConnecting = true;
            // create a choose player handler so we will have something to listen to 
            // incoming player info messages
            if (mChoosePlayerHandler) {
               delete mChoosePlayerHandler;
               mChoosePlayerHandler = NULL;
            }
            mChoosePlayerHandler = new CChoosePlayerDlgHndlr(mGame);
            // then send the game info request
            GameInfoRequestPacket* p = CreatePacket<GameInfoRequestPacket>();
            mGame->SendPacket(p);
            ReleasePacket(p);
            if (mJoinButton) {
               mJoinButton->Enable();
            }
            // we can't delete games that are found
            if (mDeleteEntryButton) {
                mDeleteEntryButton->Disable();
            }
         } else {
            LStr255 s(STRx_General, str_NotFound);
            mGameStatus->SetDescriptor(s);
            if (mDeleteEntryButton) {
                if (theJoinInfo.type == gameType_Internet) {
                    mDeleteEntryButton->Enable(); // we can delete internet games that aren't found
                } else {
                    mDeleteEntryButton->Disable();
                }
            }
         }
      } else {
         // clicked onto empty cell, clear the status display
         if (mGameStatus) {
             mGameStatus->SetDescriptor("\p");
         }
         // can't delete from the list
         if (mDeleteEntryButton) {
             mDeleteEntryButton->Disable();
         }
         // remove the ChoosePlayerHandler if present
         if (mChoosePlayerHandler) {
             delete mChoosePlayerHandler;
             mChoosePlayerHandler = NULL;
         }
         mNameStr = "";
         mPasswordStr = "";
         mGame->CloseHostConnection(); // and disconnect from the host
      }
   } else if (inMessage == message_CellDoubleClicked) {
      if (mIsConnecting) {
         // don't allow double-click while we are waiting for reply during connection
         // instead, set a flag so that when the info arrives, we will do a join
         mJoinPending = true;
      } else {
         // simulate a click on the Join button
         if (mJoinButton) {
             mJoinButton->SimulateHotSpotClick(1);
         }
      }
   }
}

void
CJoinGameDlgHndlr::SetPlayerInfo(int inPlayerNum, PublicPlayerInfoT &inInfo) {
    if (mChoosePlayerHandler) {
        mChoosePlayerHandler->SetPlayerInfo(inPlayerNum, inInfo);
    }
}



// =====================================================================================
// CChoosePlayerDlgHndlr
// =====================================================================================


bool
CChoosePlayerDlgHndlr::PrepareDialog() {
	// build the host names table
	
   mPlayerNamesTable = dynamic_cast<CTextTable*> ( mDialog->FindPaneByID(listbox_Players) );
   DEBUG_OUT("CJoinGameDlgHndlr Found Player Names Table "<<(void*)mPlayerNamesTable, DEBUG_IMPORTANT | DEBUG_USER);

   // edit fields
   mPlayerName = dynamic_cast<LEditField*> ( mDialog->FindPaneByID(editText_LoginName) );
   mPassword   = dynamic_cast<LEditField*> ( mDialog->FindPaneByID(editText_Password) );

   // buttons
   mJoinButton   = dynamic_cast<LStdButton*>   ( mDialog->FindPaneByID(msg_OK) );
   mSavePassword = dynamic_cast<LStdCheckBox*> ( mDialog->FindPaneByID(checkbox_SavePassword) ); 

   if (mPlayerNamesTable) {
      DEBUG_OUT("CJoinGameDlgHndlr About to install Array", DEBUG_TRIVIA | DEBUG_USER);
      mPlayerNameArray.RemoveAllItemsAfter(0); // clear the array
      mPlayerInfoArray.RemoveAllItemsAfter(0);
      mPlayerNamesTable->InstallArray(&mPlayerNameArray, false);
      mPlayerNamesTable->AddListener(this);    
      DEBUG_OUT("CJoinGameDlgHndlr installed Array "<< (void*)mPlayerNamesTable->GetArray()
             << " item size = " <<mPlayerNamesTable->GetArray()->GetItemSize(0), DEBUG_TRIVIA | DEBUG_USER);
      mDoSysBeep = false;	// don't beep 
   } else {
      DEBUG_OUT("Failed to find Player Names Table ", DEBUG_ERROR | DEBUG_USER);
   }

   if (mJoinButton) {
      mJoinButton->Disable();
   }
   if (mSavePassword) {
      mSavePassword->Enable();
      mSavePassword->SetValue(mSavePwd);
   }
   return true;
}

bool
CChoosePlayerDlgHndlr::CloseDialog(MessageT inMsg) {
    if ( inMsg == checkbox_SavePassword ) {
        return false; // don't exit
    }
    DEBUG_OUT("CChoosePlayerDlgHndlr::CloseDialog", DEBUG_IMPORTANT);
    if ( (inMsg == msg_OK) && mPlayerName && mPassword) {
        Str255 pString;
        char buffer[256];
        // get player name
        mPlayerName->GetDescriptor(pString);
        p2cstrcpy(buffer, pString);
        mNameStr = buffer;
        // get password
        mPassword->GetDescriptor(pString);
        p2cstrcpy(buffer, pString);
        mPasswordStr = buffer;
        if (mSavePassword) {
            mSavePwd = mSavePassword->GetValue();
        }
    } else {
        mNameStr = "";
        mPasswordStr = "";
    }
    return true;
}

void
CChoosePlayerDlgHndlr::GetPlayerNameAndPassword(std::string& outNameStr, std::string& outPasswordStr, bool &outSavePwd) {
   outNameStr = mNameStr;
   outPasswordStr = mPasswordStr;
   // convert password to "encrypted" form
   for (int i = 0; i < (int)outPasswordStr.length(); i++) {
      outPasswordStr[i] = outPasswordStr[i] ^ 0xff;
   }
   outSavePwd = mSavePwd;
}

void
CChoosePlayerDlgHndlr::SetPlayerNameAndPassword(const char* inNameStr, const char* inPasswordStr) {
   mNameStr = inNameStr;
   mPasswordStr = inPasswordStr;
   if (mPasswordStr.size() > 0) {
      mSavePwd = true;
   } else {
      mSavePwd = false;
   }
}

void	
CChoosePlayerDlgHndlr::ExecuteSelf(MessageT, void*) {
    // we do this now because we know it is safe, whereas the actual changes to the array could
    // have happened at interrupt time.
    if (mInfoChanged) {
        mInfoChanged = false;
        mPlayerNamesTable->ArrayChanged();
        mPlayerNamesTable->Refresh();
        if (mDefaultSelection != 0) {
            // default selection indicated, choose it.
            TableCellT cell;
            cell.row = mDefaultSelection;
            cell.col = 1;
            mPlayerNamesTable->SelectCell(cell);
            mDefaultSelection = 0;
        }
    }
}

void
CChoosePlayerDlgHndlr::ListenToMessage(MessageT inMessage, void* inParam) {
    if (inMessage == message_CellSelected) {
        // clear the display
        if (mPlayerName) {
            mPlayerName->SetDescriptor("\p");
        }
        if (mPassword) {
            mPassword->SetDescriptor("\p");
        }
        bool rejoining = false;
        CellInfoT* cp = static_cast<CellInfoT*>(inParam);
        if (cp->cell.row != 0) {
            Str255 playerName;
            PublicPlayerInfoT playerInfo;
            mPlayerInfoArray.FetchItemAt(cp->cell.row, playerInfo);
            c2pstrcpy(playerName, playerInfo.name);
            if (!playerInfo.isComputer) {
                // join into an human slot
                if (mPlayerName) {
                    if (!playerInfo.isAssigned) {
                        // use the default player name for open slots
                        mPlayerName->SetDescriptor(Galactica::Globals::GetInstance().getDefaultPlayerNamePStr());
                        mIsNewPlayer = true;
                        if (mSavePassword) {
                            mSavePassword->SetValue(1);
                        }
                    } else {
                        // use the recorded player name for assigned slots
                        mPlayerName->SetDescriptor(playerName);
                        rejoining = true; // and this means we are rejoining
                        mIsNewPlayer = false;
                        if (mPassword) {
                            // use the saved password
                            Str255 passwd;
                            c2pstrcpy(passwd, mPasswordStr.c_str());
                            // convert password from "encrypted" form
                            for (int i = 1; i <= passwd[0]; i++) {
                              passwd[i] = passwd[i] ^ 0xff;
                            }
                            mPassword->SetDescriptor(passwd);
                        }
                    }
                }
                if (mJoinButton) {
                    mJoinButton->Enable();
                }
            } else {
                // don't allow join into computer slot
                if (mJoinButton) {
                    mJoinButton->Disable();
                }
                if (mPlayerName) {
                    // use the default player name for computer slots
                    mPlayerName->SetDescriptor(Galactica::Globals::GetInstance().getDefaultPlayerNamePStr());
                }
            }
        } else {
            // clicked onto empty cell, clear the name and password
            if (mPlayerName) {
                mPlayerName->SetDescriptor("\p");
            }
            if (mPassword) {
                mPassword->SetDescriptor("\p");
            }
            if (mJoinButton) {
                mJoinButton->Disable();
            }
        }
        if (!rejoining) {
            LStr255 joinStr(STRx_General, str_Join);
            mJoinButton->SetDescriptor(joinStr);
        } else {
            LStr255 rejoinStr(STRx_General, str_Rejoin);
            mJoinButton->SetDescriptor(rejoinStr);
        }
    } else if (inMessage == message_CellDoubleClicked) {
        if (mJoinButton) {
            mJoinButton->SimulateHotSpotClick(1);
        }
    }
}

void
CChoosePlayerDlgHndlr::SetPlayerInfo(int inPlayerNum, PublicPlayerInfoT &inInfo) {
    mInfoChanged = true;
    Str255 playerName;
    if (inPlayerNum > (int)(mPlayerNameArray.GetCount()+1)) {
        DEBUG_OUT("WARNING: setting player info for player "<<inPlayerNum
                  <<" but will go into position "<<mPlayerNameArray.GetCount(), DEBUG_ERROR);
    }
    std::string nameInfo;
    GalacticaShared::StatusFromPublicPlayerInfo(inInfo, nameInfo);
    c2pstrcpy(playerName, nameInfo.c_str());
/*    if (inInfo.isAssigned || inInfo.isComputer) {
        char nameBuffer[256];
        char temp[256];
        std::strcpy(nameBuffer, inInfo.name);
        if (!inInfo.isAlive) {
            std::strcat(nameBuffer, " (");
            LStr255 defeatedStr(STRx_General, str_Defeated);
            p2cstrcpy(temp, defeatedStr);
            std::strcat(nameBuffer, temp);
            if (!inInfo.isLoggedIn && !inInfo.isComputer) {
                std::strcat(nameBuffer, ")");
            }
        }
        if (inInfo.isLoggedIn) {
            if (!inInfo.isAlive) {
                std::strcat(nameBuffer, ", ");
            } else {
                std::strcat(nameBuffer, " (");
            }
            LStr255 onlineStr(STRx_General, str_Online);
            p2cstrcpy(temp, onlineStr);
            std::strcat(nameBuffer, temp);
            std::strcat(nameBuffer, ")");
        } else if (inInfo.isComputer) {
            if (!inInfo.isAlive) {
                std::strcat(nameBuffer, ", ");
            } else {
                std::strcat(nameBuffer, " (");
            }
            LStr255 computerStr(STRx_Names, str_Computer);
            p2cstrcpy(temp, computerStr);
            std::strcat(nameBuffer, temp);
            std::strcat(nameBuffer, ")");
        }
        c2pstrcpy(playerName, nameBuffer);
    } else {
        LStr255 unassignedStr(STRx_General, str_Open);
        LString::CopyPStr(unassignedStr, playerName);
    }
    */
    if ((int)mPlayerNameArray.GetCount() < inPlayerNum) {
        // adding new item
        mPlayerNameArray.InsertItemsAt(1, inPlayerNum, playerName);
        mPlayerInfoArray.InsertItemsAt(1, inPlayerNum, inInfo);
        // since we are adding, check to see if this player is the one we expect to be
        if (inInfo.isAssigned && !inInfo.isComputer && (mNameStr.size()>0) ) {
            if (std::strcmp(mNameStr.c_str(), inInfo.name) == 0) {
                // names match, this is the one
                mDefaultSelection = inPlayerNum;
            }
        }
        
    } else {
        // update to existing item
        mPlayerNameArray.AssignItemsAt(1, inPlayerNum, playerName);
        mPlayerInfoArray.AssignItemsAt(1, inPlayerNum, inInfo);
    }
}

// =====================================================================================
// CChatHandler
// =====================================================================================

class CChatHandler : public LAttachment, public LListener, public LCommander {
public:
	CChatHandler(GalacticaClient* inGame) 
	   : LAttachment(), LCommander(inGame) { 
	      mGame = inGame; 
	      mWindow = 0;
	      AddAttachment(this); // we are our own attachment
	   }
	virtual Boolean AllowSubRemoval( LCommander* inSub );
	virtual void    ExecuteSelf(MessageT inMessage, void* ioParam);
	virtual void    ListenToMessage(MessageT inMessage, void* ioParam = 0);
	bool            PrepareWindow();
    void            AddChatToHistory(SInt8 fromPlayer, const char* chatText, bool toAll = false);
    void            AddNoticeToHistory(const char* noticeText);
    void            ActivateForPlayer(SInt8 inPlayer);
    void            UpdatePlayerStatus(SInt8 inPlayer);
    int             GetMenuItemNumForPlayer(SInt8 inPlayer);
    SInt8           GetPlayerNumFromMenuItem(int inMenuItem);
    bool            ChatWindowOpen() { return (mWindow != 0); }
    void			BringWindowToFront() { if (mWindow) { mWindow->Select(); } }
protected:
    LWindow* mWindow;
	GalacticaClient* mGame;
	LStdPopupMenu* mMenu;
};

Boolean
CChatHandler::AllowSubRemoval( LCommander* inSub ) {
   if (inSub == mWindow) {
      mWindow = 0;
      mMenu = 0;
      return true;
   } else {
      return LCommander::AllowSubRemoval(inSub);
   }
}

int
CChatHandler::GetMenuItemNumForPlayer(SInt8 inPlayer) {
   int menuItem = -1;
   if (inPlayer == kAdminPlayerNum) {
      menuItem = 3;
   } else if (inPlayer > 0) {
      menuItem = inPlayer + 4;
   } else {
      menuItem = 1;  // default to all players
   }
   return menuItem;
}

SInt8
CChatHandler::GetPlayerNumFromMenuItem(int inMenuItem) {
   SInt8 playerNum = inMenuItem - 4;	// which item was choosen
   if (playerNum == -1) {
      playerNum = kAdminPlayerNum;
   } else if (playerNum < 0) {
   	playerNum = kAllPlayers;
   }
   return playerNum;
}

bool
CChatHandler::PrepareWindow() {
 	mWindow = LWindow::CreateWindow(window_Chat, this);
 	if (!mWindow) {
 	   return false;
 	}
	// build the player names popup menu
	mMenu = dynamic_cast<LStdPopupMenu*> ( mWindow->FindPaneByID(popupMenu_Player) );
	if (!mMenu) {
	   return false;
	}
	mMenu->AddListener(this);
	MenuHandle theMenu = mMenu->GetMacMenuH();	// Get the menu handle
	int numPlayers = mGame->GetNumPlayers();		   // get number of human players
	int myNum = mGame->GetMyPlayerNum();		   // which am I?
	mMenu->SetMaxValue(numPlayers+4);	            // set number of items in the menu
	std::string name;
	Str255 tmpstr;
	for (int i = 1; i <= numPlayers; i++) {
		bool doItalics = false;
		bool knownPlayer = mGame->GetPlayerNameAndStatus(i, name);	// get the name of the Nth player
		if (!knownPlayer) {				         // did we find someone who is not logged in?
			LoadStringResource(name, STRx_General, str_Unknown_);	// give them the name "unknown"
			doItalics = true;					// and put it in italics
		}
		c2pstrcpy(tmpstr, name.c_str());
		::AppendMenuItemText(theMenu, tmpstr);    // add it to the end of the menu
		if (i == myNum)	{						      // indicate myself in italics
			doItalics = true;
		}
		if (mGame->PlayerIsComputer(i)) {
		   ::DisableMenuItem(theMenu, i+4);
		}
		if (doItalics) {
			::SetItemStyle(theMenu, i+4, italic);
		}
	}
	
// following section doesn't work, because clients don't have any info on the Admin
// so it always says "Administrator (Defeated)" rather than giving their online/offline status
// as desired

/*   mGame->GetPlayerNameAndStatus(kAdminPlayerNum, name); // get info on Administrator
	c2pstrcpy(tmpstr, name.c_str());
	::SetMenuItemText(theMenu, 3, tmpstr); */

	return true;
}

void
CChatHandler::ActivateForPlayer(SInt8 inPlayer) {
   if (!mWindow) {
      if (!PrepareWindow()) {
         return;
      }
   }
   mMenu->SetValue(GetMenuItemNumForPlayer(inPlayer));	
   
   LEditField* theText = dynamic_cast<LEditField*>( mWindow->FindPaneByID(editField_ChatText) );
   if (theText) {
//      theText->SetDescriptor("\p");
      LCommander::SwitchTarget(theText);
   }
   mWindow->Show();
   mWindow->Select();
} 

void
CChatHandler::UpdatePlayerStatus(SInt8 inPlayer) {
   // change status for a player going offline or coming online
   // or even possibly for a name change or removal from the game
   if (mMenu) {
      int menuItem = GetMenuItemNumForPlayer(inPlayer);
      if (menuItem != -1) {
   	   MenuHandle theMenu = mMenu->GetMacMenuH();	// Get the menu handle
      	std::string name;
      	bool knownPlayer = mGame->GetPlayerNameAndStatus(inPlayer, name);	// get the name of the Nth player
      	if (!knownPlayer) {				         // did we find someone who is not logged in?
      		LoadStringResource(name, STRx_General, str_Unknown_);	// give them the name "unknown"
      	   ::SetItemStyle(theMenu, menuItem, italic);   // italicize unknown players
      	} else if (inPlayer != mGame->GetMyPlayerNum()) {
      	   ::SetItemStyle(theMenu, menuItem, normal);   // normalize all known players but me
      	}
      	Str255 tmpstr;
      	c2pstrcpy(tmpstr, name.c_str());
      	::SetMenuItemText(theMenu, menuItem, tmpstr);    // add it to the end of the menu
   	}
	}
}


void
CChatHandler::AddChatToHistory(SInt8 fromPlayer, const char* chatText, bool toAll) {
   if (mWindow) {
      new CSoundResourcePlayer(snd_Message); // play the message arrival sound
      LTextEditView* theHistory = static_cast<LTextEditView*>( mWindow->FindPaneByID(textEditView_ChatHistory) );
      std::string playerName;
      mGame->GetPlayerName(fromPlayer, playerName);
      if (toAll) {
         std::string s;
         LoadStringResource(s, STRx_Messages, str_To);
   	   int pos = s.find('*');
   		if (pos != std::string::npos) {
            std::string s2;
            LoadStringResource(s2, STRx_Messages, str_AllPlayers);
   			s.replace(pos, 1, s2);
   		}
   		playerName += " (";
   		playerName += s;
   		playerName += ")";
      }
      StScrpHandle bh = GetStScrpHandleForStyle(bold);
      theHistory->Insert(playerName.c_str(), playerName.length(), bh, kDontRefresh);
      MacAPI::DisposeHandle((Handle)bh);
      theHistory->Insert(": ", 2, 0, kDontRefresh);
      StScrpHandle nh = GetStScrpHandleForStyle(normal);
      theHistory->Insert(chatText, std::strlen(chatText), nh, kDontRefresh);
      MacAPI::DisposeHandle((Handle)nh);
      theHistory->Insert("\r", 1, 0, kRefresh);
   }
}

void
CChatHandler::AddNoticeToHistory(const char* noticeText) {
   if (mWindow) {
      LTextEditView* theHistory = static_cast<LTextEditView*>( mWindow->FindPaneByID(textEditView_ChatHistory) );
      StScrpHandle h = GetStScrpHandleForStyle(bold | italic);
      theHistory->Insert(noticeText, std::strlen(noticeText), h, kDontRefresh);
      theHistory->Insert("\r", 1, 0, kRefresh);
      MacAPI::DisposeHandle((Handle)h);
   }
}

void
CChatHandler::ListenToMessage(MessageT inMessage, void* ) { // ioParam
   if (mMenu && (inMessage == popupMenu_Player)) {
      // for popup menu changes, post the name of the player as a notice
      int chatWith = GetPlayerNumFromMenuItem(mMenu->GetValue());
      std::string playerName;
      if (chatWith == kAllPlayers) {
         LoadStringResource(playerName, STRx_Messages, str_AllPlayers);
      } else {
         mGame->GetPlayerName(chatWith, playerName);
      }
      std::string s;
      LoadStringResource(s, STRx_Messages, str_ChattingWith);
	   int pos = s.find('*');
		if (pos != std::string::npos) {
			s.replace(pos, 1, playerName);
		}
      AddNoticeToHistory(s.c_str());      
   } 
}

void	
CChatHandler::ExecuteSelf(MessageT inMessage, void* ioParam) {
   if ( mWindow && mMenu && (inMessage == msg_KeyPress) ) {
      // for keypresses, check for Enter or Return
      EventRecord* eventP = (EventRecord*)ioParam;
	   char key = eventP->message & charCodeMask;
      if ( (key == char_Enter) || (key == char_Return) ) {
         // enter or return, send the message
   		LEditField* theText = static_cast<LEditField*>( mWindow->FindPaneByID(editField_ChatText) );
   		int sendTo = GetPlayerNumFromMenuItem(mMenu->GetValue());

   		#warning TODO: watch for chat exceeding 32k and chop off beginning
   		
         // add text to history, along with players name
         LStr255 messageText;
         theText->GetDescriptor(messageText);
    		int strlen = messageText.Length();
    		std::string text(messageText.ConstTextPtr(), strlen);
         AddChatToHistory(mGame->GetMyPlayerNum(), text.c_str());
          // clear the text entry area
         theText->SetDescriptor("\p");
         // now send the message
   		if (!mGame->IsSinglePlayer()) {
       			// in multiplayer games, create the message and send it to the host
       		PlayerToPlayerMessagePacket* p = CreatePacket<PlayerToPlayerMessagePacket>(strlen + 1);
       		if (p) {
           		std::memcpy(&p->messageText[0], messageText.ConstTextPtr(), strlen);
           	   p->messageText[strlen] = 0; // nul terminate the string
           	   p->fromPlayer = mGame->GetMyPlayerNum();
           	   p->toPlayer = sendTo;
               mGame->SendPacket(p);
               ReleasePacket(p);
       	   }
         }
         mExecuteHost = false;
      }
   } else {
      mExecuteHost = true;
   }
}

#pragma mark-


GalacticaClient::GalacticaClient(LCommander *inSuper)
: GalacticaDoc(inSuper) {
	DEBUG_OUT("New GalacticaClient", DEBUG_IMPORTANT | DEBUG_USER);
	mPlayerNum = 0;
	mNumDownloadedThingys = 0;
	mProgressBar = NULL;
	mProgressWind = NULL;
    mJoining = false;
    mDisconnecting = false;
    mChatHandler = NULL;

    // Initialize to NULL OpenPlay network stuff
    mEndpoint = NULL;
    mConfig = NULL;
  #if TARGET_OS_MAC
    mConfigAppletalk = NULL;
  #endif // TARGET_OS_MAC
    mJoinGameHandler = NULL;

    mInProgressPacket = NULL;
//    mHavePacketHeader = false;
    mOffsetInHeader = 0;

    // different default for multiplayer games
    mGotoEnemyMoves = false;
    mNextPingMs = 0;
}

GalacticaClient::~GalacticaClient() {
	DEBUG_OUT("\GalacticaClient::~GalacticaClient " << GetMyPlayerNum(), DEBUG_IMPORTANT | DEBUG_USER);
	StopIdling();	// stop checking for incoming data
	StopRepeating();
    if (mProgressWind) {
        mProgressWind->Hide();
        delete mProgressWind;
        mProgressWind = NULL;  
        mProgressBar = NULL;
    }
	ShutdownClient();
}

OSErr
GalacticaClient::BrowseAndJoinGame() {
    DEBUG_OUT("GalacticaClient::BrowseAndJoinGame", DEBUG_IMPORTANT);
    LoadKnownGamesList();
    OSErr err = StartBrowsingForGames();
    if (err == noErr) {
        mJoinGameHandler = new CJoinGameDlgHndlr(this);
        mJoining = true;  // we are joining starting now.
		SInt16 answer = MovableAlert(window_ConnectToGame, this, 0, false, mJoinGameHandler );
        DEBUG_OUT("returned from MovableAlert", DEBUG_IMPORTANT);
        CJoinGameDlgHndlr* joinHandler = mJoinGameHandler;
        mJoining = false; // done with the join dialogs (flag may be reused by login request handling)
 		mJoinGameHandler = NULL;    // we will delete it momentarily
		if (answer == msg_Cancel) {
		    err = userCanceledErr;
		} else {
		    // save the entered name as the default name if different from default
		    std::string playerName;
		    std::string playerPassword;
		    bool asNewPlayer = joinHandler->GetPlayerIsNew();
		    bool savePassword = joinHandler->GetSavePassword();
		    joinHandler->GetPlayerNameAndPassword(playerName, playerPassword);    // get the player name as entered
		    // save name and password into the connected host info
		    std::strncpy(mConnectedHostInfo.playername, playerName.c_str(), MAX_PLAYER_NAME_LEN);
		    if (savePassword) {
		        std::strncpy(mConnectedHostInfo.password, playerPassword.c_str(), MAX_PASSWORD_LEN);
		    } else {
		        mConnectedHostInfo.password[0] = 0;
		    }
		    // get the default name so we can update it if needed
		    char defaultName[256];
		    strcpy(defaultName, Galactica::Globals::GetInstance().getDefaultPlayerName());
		    if (playerName != defaultName) {
		        // update the default name if we are using something different
		        Galactica::Globals::GetInstance().setDefaultPlayerName(playerName.c_str());
               	GalacticaApp::SavePrefs();
		    }
		    SendLoginRequest(playerName.c_str(), playerPassword.c_str(), asNewPlayer);
		}
 		if (joinHandler) {
            DEBUG_OUT("deleting JoinHandler", DEBUG_IMPORTANT);
            delete joinHandler;
        }
    }
    DEBUG_OUT("checking for error", DEBUG_IMPORTANT);
    if (err != noErr) {
        delete this;
        if (err == userCanceledErr) { // don't worry about cancellation
            DEBUG_OUT("User Canceled BrowseAndJoin", DEBUG_IMPORTANT);
            err = noErr;
        } else {
            DEBUG_OUT("BrowseAndJoinGame reported error = "<<err, DEBUG_ERROR);
        }
    }
    return err;
}

void
GalacticaClient::SetPlayerInfo(int inPlayerNum, PublicPlayerInfoT &inInfo) {
    GalacticaDoc::SetPlayerInfo(inPlayerNum, inInfo);
    if (mJoinGameHandler != NULL) {
        mJoinGameHandler->SetPlayerInfo(inPlayerNum, inInfo);
    }
}

OSErr
GalacticaClient::JoinGame(const char* inPlayerName, const char* inGameAddress, 
                           GameType inGameType, unsigned short inGamePort, UInt32 inRejoinKey) {
   DEBUG_OUT("GalacticaClient::JoinGame ["<<inGameAddress<<":"<<inGamePort<<"]", DEBUG_IMPORTANT);
   OSErr err = ConnectToNamedHost(inGameAddress, inGameType, inGamePort);
   if (err == noErr) {
  #warning FIXME: Sending login request with no password
      bool asNewPlayer = (inRejoinKey == 0);
      SendLoginRequest(inPlayerName, "", asNewPlayer);
   }
   if (err != noErr) {
      DEBUG_OUT("JoinGame reported error = "<<err, DEBUG_ERROR);
      delete this;
   }
   return err;
}

Boolean
GalacticaClient::ObeyCommand(CommandT inCommand, void *ioParam) {
    bool cmdHandled = true;
#warning TODO: figure out if we need disallow commands?
/*	if (mDisallowCommands) {
		SysBeep(1);
		return true;
	}
*/
	switch (inCommand) {
		case cmd_SendMessage:
			DoChat(kAllPlayers);
			if (mChatHandler) {
				mChatHandler->BringWindowToFront();
			}
			break;

      case cmd_ChangeSettings:
         {
            CGameSetupAttach theSetupAttach(&mGameInfo);
            ResIDT windID = window_HostNewGame;
            long response = MovableAlert(windID, this, 0, false, &theSetupAttach);   // display new game setup wind
            if (response == button_NewGameBegin) { // user pushed the update setting button
               AdminSetGameInfoPacket* p = CreatePacket<AdminSetGameInfoPacket>( mGameInfo.gameTitle[0]+1 );
               p->maxTimePerTurn = mGameInfo.maxTimePerTurn;
               p->numHumans = mGameInfo.numHumans;
               p->numComputers = mGameInfo.numComputers;
               p->compSkill = mGameInfo.compSkill;
               p->compAdvantage = mGameInfo.compAdvantage;
               p->omniscient = mGameInfo.omniscient;
               p->fastGame = mGameInfo.fastGame;
               p->endTurnEarly = mGameInfo.endTurnEarly;
               p2cstrcpy(p->gameTitle, mGameInfo.gameTitle);
               SendPacket(p);
               ReleasePacket(p);
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
    			for (int i = 1; i <= mGameInfo.totalNumPlayers ; i++) {
               PublicPlayerInfoT info;
               GetPlayerInfo(i, info);
               if (info.isAssigned && info.isAlive && !info.isComputer 
                 && !info.isLoggedIn ) {
                  bAskToSkip = true;
               }
    			}
    			if (bAskToSkip) {
   			   bCanEndTurn = false;
               MovableParamText(0, "any players who haven't yet posted their turns.\r\r\r\r\r\r");
   				if ( MovableAlert(window_AskSkipPlayer) == 1 ) {
   			      bCanEndTurn = true;
   				}
   			}
   			if (bCanEndTurn) {
               AdminEndTurnNowPacket* p = CreatePacket<AdminEndTurnNowPacket>();
               SendPacket(p);
               ReleasePacket(p);
	         }
         }
         break;

		default:
			cmdHandled = GalacticaDoc::ObeyCommand(inCommand, ioParam);
			break;
	}
	return cmdHandled;
}

void
GalacticaClient::FindCommandStatus(CommandT inCommand, Boolean &outEnabled, Boolean &outUsesMark,
								UInt16 &outMark, Str255 outName) {
	outMark = noMark;
	bool amAdmin = (GetMyPlayerNum() == kAdminPlayerNum);
	if (mEndpoint == NULL) { // there is no host available do the following
	    if (inCommand == cmd_EndTurn) {            // can't post an end of turn
	        outEnabled = false;
	        return;
	    } else if (inCommand == cmd_SendMessage) { // can't send a message
	        outEnabled = false;
	        return;
	    }
	}
	if (amAdmin) {
	   if ( (inCommand == cmd_EndTurn) || (inCommand == cmd_ShowSystems) || (inCommand == cmd_ShowFleets) ) {
	      // these normal commands never available to admins
	      outEnabled = false;
	      return;
	   } else if ( (inCommand == cmd_ChangeSettings) || (inCommand == cmd_ChangePlayer) 
	     || (inCommand == cmd_EndTurnNow) //|| (inCommand == cmd_PauseResume) 
	     ) {
	      // this admin commands always available to admins
	      outEnabled = true;
	      return;
	   }
	}
	switch (inCommand) {
		// Return menu item status according to command messages.
		// Any that you don't handle will be passed to LApplication
		case cmd_SendMessage:
			outEnabled = IsConnected();
			break;
		// admin commands
      case cmd_SetPopulation:
      case cmd_SetTechLevel:
      case cmd_SetOwner:
      case cmd_NewShipAtStar:
         outEnabled = false;
         outUsesMark = false;
         outMark = ' ';
         if (mSelectedThingy != nil) {
            if (mSelectedThingy->GetThingySubClassType() == thingyType_Star) {
               outEnabled = ((GetTurnNum() == 1) && amAdmin);
            }
         }
         break;
		default:
			GalacticaDoc::FindCommandStatus(inCommand, outEnabled, outUsesMark, outMark, outName);
			break;
	}
}


void
GalacticaClient::SpendTime(const EventRecord &inEvent) {

	// ====================== packet processing ===============================
	// pullpackets off the queue and process them
	if (!CStarMapView::IsAwaitingClick() ) {	// don't process UI packets while tracking mouse
    	while (!mPackets.empty()) {
    	    DEBUG_OUT("Client SpendTime() processing one queued packet", DEBUG_TRIVIA | DEBUG_PACKETS);
    	    
    	    Packet* p;
    	    {
        	    pdg::AutoCriticalSection crit(&mPacketQueueCriticalSection);
        	    p = mPackets.front();
        	    mPackets.pop();
    	    }
    	    HandleQueuedPacket(p);
    	    ReleasePacket(p);
    	}
    }


	// give time to OpenPlay to look for hosts on the network so we can get ClientEnumerationCallbacks
	if (mConfig) {
    	OSErr err = ProtocolIdleEnumeration(mConfig);
    	if (err){
            DEBUG_OUT("OpenPlay ProtocolIdleEnumeration returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        }
    }
    // ping the host once in a while to make sure we haven't lost connection
    if ((mConnectedHostInfo.gameid != 0) && (mNextPingMs != 0) && (mNextPingMs < pdg::OS::getMilliseconds())) {
		PingPacket* pp = CreatePacket<PingPacket>();
		pp->pingNum = mNextPingNum++;
		DEBUG_OUT("Client SpendTime() sending PingPacket for ping number " << pp->pingNum, DEBUG_DETAIL | DEBUG_PACKETS);
		SendPacket(pp);
		ReleasePacket(pp);
		mNextPingMs = pdg::OS::getMilliseconds() + GALACTICA_CLIENT_PING_INTERVAL;
	}
  #if TARGET_OS_MAC
	// give time to OpenPlay to look for hosts on the network so we can get ClientEnumerationCallbacks
	if (mConfigAppletalk) {
    	OSErr err = ProtocolIdleEnumeration(mConfigAppletalk);
    	if (err){
            DEBUG_OUT("OpenPlay ProtocolIdleEnumeration (Appletalk) returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        }
    }
  #endif // TARGET_OS_MAC

    // update progess bar if necessary
    if (mProgressBar) {
       mProgressBar->SetCurrentValue(mNumDownloadedThingys);
    }

    GalacticaDoc::SpendTime(inEvent);

}

void
GalacticaClient::DoChat(int inToPlayer) {
    bool wasClosed = false;
    if (!mChatHandler) {
	    mChatHandler = new CChatHandler(this);
	    wasClosed = true;
	} else {
	    wasClosed = !mChatHandler->ChatWindowOpen();
	}
	if (wasClosed || (inToPlayer != kAllPlayers)) {
	    // switch player unless we are already open in a private chat
	    mChatHandler->ActivateForPlayer(inToPlayer);
	}
}

void
GalacticaClient::SetSelectedThingy(AGalacticThingyUI* inThingy) {
    AGalacticThingyUI* prev = mSelectedThingy;
    if ( (prev != inThingy) && (prev != NULL) ) {
        // the selection changed away from a non-null thingy -- this may be of interest to us
        if (prev->IsChanged() && (prev->GetOwner() == GetMyPlayerNum()) && prev->CanWriteback()) {
            // the old selection was marked as changed and we own it
            SendThingy(prev);
            prev->NotChanged(); // we've sent it, so clear the changed flag
        }
    }
    GalacticaDoc::SetSelectedThingy(inThingy);
}

void
GalacticaClient::AttemptClose(Boolean inRecordIt) { 
	FSSpec		fileSpec = {0, 0, "\p"};		// Invalid specifier
	if (inRecordIt) {
		try {
			SendAEClose(kAENo, fileSpec, ExecuteAE_No);
		}

		catch (...) { }
	}
    SendAllChangedThingys();
    Close();
}


// ---------------------------------------------------------------------------
//		¥ AttemptQuitSelf
// ---------------------------------------------------------------------------
//	Try to close a Document when quitting the program.
//
//	Return false if the user cancels when asked whether to save changes
//	to a modified Document.
Boolean
GalacticaClient::AttemptQuitSelf(SInt32 ) { //inSaveOption
    SendAllChangedThingys();
	return true;
}

void
GalacticaClient::UserChangedSettings() {
	GalacticaDoc::UserChangedSettings();
	// when the user changes settings, we must notify the host of the new settings
	SendClientSettings();
}

void
GalacticaClient::TimeIsUp() {	// from CTimed, called by CTimer when the turn time limit expires
	DEBUG_OUT("GalacticaClient::TimeIsUp()", DEBUG_IMPORTANT);
}

void
GalacticaClient::DoEndTurn() {
	DEBUG_OUT("GalacticaClient::DoEndTurn " << GetMyPlayerNum() << " End of TURN # " << mGameInfo.currTurn, DEBUG_IMPORTANT);
	if (mSelectedThingy) {
		ASSERT(ValidateThingy(mSelectedThingy));	// extreme sanity check
	}
	mLastTurnPosted = mGameInfo.currTurn;
//	mAutoEndTurnTimer.Clear();				// clear the timer
	mEndTurnButton->Disable();
	mEndTurnButton->Refresh();

   SendAllChangedThingys();

	mPlayerInfo.lastTurnPosted = mGameInfo.currTurn;
	// now that we've sent all the thingys, send the turn complete packet
	TurnCompletePacket* tcp = CreatePacket<TurnCompletePacket>();
	tcp->turnNum = mGameInfo.currTurn;
	DEBUG_OUT("GalacticaClient::DoEndTurn sending TurnCompletePacket for turn number " << tcp->turnNum, DEBUG_DETAIL);
	SendPacket(tcp);
	ReleasePacket(tcp);
	StartIdling();	// begin checking for end of turn processing completed
	std::string postedStr;
	LoadStringResource(postedStr, STRx_General, str_Posted);
	if (mGameInfo.maxTimePerTurn > 0) {
	   mAutoEndTurnTimer.Mark(postedStr.c_str());	// make timer read time remaining + "POSTED"
	} else {
	   mAutoEndTurnTimer.Clear(postedStr.c_str());  // make timer read just "POSTED"
    }
	DEBUG_OUT("GalacticaClient::DoEndTurn " << GetMyPlayerNum() << " EOT POSTED", DEBUG_IMPORTANT);
}


#pragma mark-

OSErr
GalacticaClient::StartBrowsingForGames() {
    DEBUG_OUT("GalacticaClient::StartBrowsingForGames", DEBUG_IMPORTANT | DEBUG_PACKETS);
    NMType moduleType = kIPModuleType;  // assume TCP/IP
    const char* nameStr = "Galactica";
	NMErr err = ProtocolCreateConfig(moduleType, type_Creator, nameStr, NULL, 0, NULL, &mConfig);
	if (err != noErr) {
        DEBUG_OUT("Client OpenPlay ProtocolCreateConfig() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        mConfig = NULL; // just to be sure
        return err;
    }
    // start active enumeration of available hosts
 	err = ProtocolStartEnumeration(mConfig, ClientEnumerationCallback, this, true);
	if (err != noErr) {
        DEBUG_OUT("Client OpenPlay ProtocolStartEnumeration() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        return err;
    }
  #if TARGET_OS_MAC
    // for MacOS, also set up an Appletalk endpoint
	moduleType = kATModuleType;
	NMErr err2 = ProtocolCreateConfig(moduleType, type_Creator, nameStr, NULL, 0, NULL, &mConfigAppletalk);
	if (err2 != noErr) {
        DEBUG_OUT("Client OpenPlay ProtocolCreateConfig() (Appletalk) returned "<<err2, DEBUG_ERROR | DEBUG_PACKETS);
        mConfigAppletalk = NULL; // just to be sure
    } else {
        // start active enumeration of available hosts
    	err2 = ProtocolStartEnumeration(mConfigAppletalk, ClientAppleTalkEnumerationCallback, this, true);
    	if (err2 != noErr) {
            DEBUG_OUT("Client OpenPlay ProtocolStartEnumeration() (Appletalk) returned "<<err2, DEBUG_ERROR | DEBUG_PACKETS);
        }
    }
  #endif // TARGET_OS_MAC
    return err;
}

void
GalacticaClient::ShutdownClient() {
    DEBUG_OUT("GalacticaClient::ShutdownClient", DEBUG_IMPORTANT | DEBUG_PACKETS);
    // v2.1, close down OpenPlay stuff
    CloseHostConnection(true); // do an orderly close of the socket
    if (mConfig) {
        // if there is a config, it means we are still enumerating
    	OSErr err = ProtocolEndEnumeration(mConfig);
    	if (err != noErr) {
            DEBUG_OUT("Client OpenPlay ProtocolEndEnumeration() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        }
    	err = ProtocolDisposeConfig(mConfig);
    	if (err != noErr) {
            DEBUG_OUT("OpenPlay ProtocolDisposeConfig() returned "<<err, DEBUG_ERROR);
        }
        mConfig = NULL;
    }
  #if TARGET_OS_MAC
    // for MacOS, also shutdown Appletalk endpoint
    if (mConfigAppletalk) {
        // if there is a config, it means we are still enumerating
    	OSErr err = ProtocolEndEnumeration(mConfigAppletalk);
    	if (err != noErr) {
            DEBUG_OUT("Client OpenPlay ProtocolEndEnumeration() (Appletalk) returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        }
    	err = ProtocolDisposeConfig(mConfigAppletalk);
    	if (err != noErr) {
            DEBUG_OUT("OpenPlay ProtocolDisposeConfig() (Appletalk) returned "<<err, DEBUG_ERROR);
        }
        mConfigAppletalk = NULL;
    }
  #endif // TARGET_OS_MAC
}

// info.hostid is taken from OpenPlay, info.type is the open play protocol used (e.g.: IP or AppleTalk)
OSErr
GalacticaClient::ConnectToHost(GameJoinInfo inJoinInfo) {
    DEBUG_OUT("GalacticaClient::ConnectToHost "<<inJoinInfo.hostid, DEBUG_IMPORTANT | DEBUG_PACKETS);

    NMType theHostType = 0;
    // if this is an Internet game, connect by name, but be sure to save all the connection info
    if ( (inJoinInfo.type == gameType_Internet) || (inJoinInfo.type == gameType_MatchMaker)) {
        OSErr result = ConnectToNamedHost(inJoinInfo.hostname, gameType_Internet, inJoinInfo.port);
        mConnectedHostInfo = inJoinInfo;
        return result;
    } else if ( (inJoinInfo.type == gameType_LAN) && (inJoinInfo.hostid != 0) ) {
        theHostType = kIPModuleType;
    } else if ( (inJoinInfo.type == gameType_AppleTalk) && (inJoinInfo.hostid != 0) ) {
        theHostType = kATModuleType;
    } else {
        DEBUG_OUT("GalacticaClient::ConnectToHost called with invalid game type "<<(long)inJoinInfo.type, DEBUG_ERROR);
    }

    if (mConnectedHostInfo.hostid == inJoinInfo.hostid) {
        // already connected to this host
        return noErr;
    }
    if (mEndpoint != NULL) {
      // we want to close the previously open endpoint
        CloseHostConnection();
    }
    OSErr err;
    bool failed = false;
    PConfigRef theConfig = mConfig;
    if (theHostType == kATModuleType) {
        DEBUG_OUT("AppleTalk game, using AppleTalk config", DEBUG_IMPORTANT | DEBUG_PACKETS);
        theConfig = mConfigAppletalk;
    }
	err = ProtocolBindEnumerationToConfig(theConfig, inJoinInfo.hostid);
	if (err != noErr) {
        DEBUG_OUT("Client OpenPlay ProtocolBindEnumerationToConfig() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        failed = true; // this is fatal
    } else { // log success
    	char config_string[1024];
        ProtocolGetConfigString(theConfig, config_string, 1024);
        DEBUG_OUT("Client bound config ["<<config_string<<"]", DEBUG_IMPORTANT | DEBUG_PACKETS);
    }
    if (!failed) {
        err = ProtocolOpenEndpoint(theConfig, ClientCallback, this, &mEndpoint, kOpenActive);
    	if (err != noErr) {
            DEBUG_OUT("Client OpenPlay ProtocolOpenEndpoint() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
            mEndpoint = NULL;
            mConnectedHostInfo.gameid = 0;
            mConnectedHostInfo.hostid = 0;
            mConnectedHostInfo.hostname[0] = 0;
            mConnectedHostInfo.playername[0] = 0;
            mConnectedHostInfo.key = 0;
            mConnectedHostInfo.type = gameType_LAN;
            failed = true;  // most definitely fatal
        } else {
            mConnectedHostInfo = inJoinInfo;
        }
    }
    if (!failed) {
        DEBUG_OUT("Client Successfully connected to host", DEBUG_IMPORTANT | DEBUG_PACKETS);
        err = noErr;
    }
    return err;
}

OSErr
GalacticaClient::ConnectToNamedHost(const char* inHostName, GameType inGameType, unsigned short inPort) {
    DEBUG_OUT("GalacticaClient::ConnectToNamedHost", DEBUG_IMPORTANT | DEBUG_PACKETS);
    OSErr err;
    bool failed = false;
    NMType moduleType = kIPModuleType;  // assume TCP/IP
    if (inPort == 0) {
      inPort = 4429;   // use the default port if none specified
    }
    char* nameStr = "";
    char portStr[8];
    std::sprintf(portStr, "%d", inPort);
    PConfigRef config;
    std::string configStr;
    configStr = "IPaddr=";
    configStr += inHostName;
    configStr += "\tIPport=";
    configStr += portStr;
    err = ProtocolCreateConfig(moduleType, type_Creator, nameStr, NULL, 0, (char*)configStr.c_str(), &config);
	if (err != noErr) {
        DEBUG_OUT("Client OpenPlay ProtocolCreateConfig() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        failed = true; // this is fatal
    } else { // log success
    	char config_string[1024];
        ProtocolGetConfigString(config, config_string, 1024);
        DEBUG_OUT("Client created config ["<<config_string<<"]", DEBUG_IMPORTANT | DEBUG_PACKETS);
    }
    if (mEndpoint != NULL) {
        // we want to close the previously open endpoint
        CloseHostConnection();
    }
    if (!failed) {
        DEBUG_OUT("Attempting to establish OpenPlay connection", DEBUG_IMPORTANT | DEBUG_PACKETS);
        err = ProtocolOpenEndpoint(config, ClientCallback, this, &mEndpoint, kOpenActive);
    	if (err != noErr) {
            DEBUG_OUT("Client OpenPlay ProtocolOpenEndpoint() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
            mEndpoint = NULL;
            mConnectedHostInfo.gameid = 0;
            mConnectedHostInfo.hostid = 0;
            mConnectedHostInfo.hostname[0] = 0;
            mConnectedHostInfo.playername[0] = 0;
            mConnectedHostInfo.key = 0;
            mConnectedHostInfo.type = gameType_LAN;
            failed = true;  // most definitely fatal
        } else {
            mConnectedHostInfo.gameid = 0;
            mConnectedHostInfo.hostid = 0;
            std::strcpy(mConnectedHostInfo.hostname, inHostName);
            mConnectedHostInfo.playername[0] = 0;
            mConnectedHostInfo.key = 0;
            mConnectedHostInfo.type = inGameType;
        }
    }
	if (ProtocolDisposeConfig(config) != noErr) { // don't assign to err because we want to return err from Open()
        DEBUG_OUT("OpenPlay ProtocolDisposeConfig() returned "<<err, DEBUG_ERROR);
    }
    if (!failed) {
        DEBUG_OUT("Client Successfully connected to host", DEBUG_IMPORTANT | DEBUG_PACKETS);
        err = noErr;
    }
    return err;
}

void
GalacticaClient::CloseHostConnection(bool inOrderlyClose) {
    DEBUG_OUT("GalacticaClient::CloseHostConnection", DEBUG_IMPORTANT | DEBUG_PACKETS);
    if (mEndpoint) {
        OSErr err = ProtocolCloseEndpoint(mEndpoint, inOrderlyClose);
    	if (err != noErr) {
            DEBUG_OUT("OpenPlay ProtocolCloseEndpoint() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        }
        mConnectedHostInfo.gameid = 0;
        mConnectedHostInfo.hostid = 0;
        mConnectedHostInfo.hostname[0] = 0;
        mConnectedHostInfo.playername[0] = 0;
        mConnectedHostInfo.key = 0;
        mConnectedHostInfo.type = gameType_LAN;
        mEndpoint = NULL;
      #ifndef NO_GAME_RANGER_SUPPORT
        GRGameEnd();    // let GameRanger know that we ended the game
      #endif
    }
}

void
GalacticaClient::StopBrowsingForGames() {
    DEBUG_OUT("GalacticaClient::StopBrowsingForGames", DEBUG_IMPORTANT | DEBUG_PACKETS);
    // stop enumeration of hosts
    if (mConfig) {
        OSErr err = ProtocolEndEnumeration(mConfig);
        if (err != noErr) {
            DEBUG_OUT("Client OpenPlay ProtocolEndEnumeration() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        }
        // dispose of the configuration used
        err = ProtocolDisposeConfig(mConfig);
        if (err != noErr) {
            DEBUG_OUT("Client OpenPlay ProtocolDisposeConfig() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        }
        mConfig = NULL; // just making sure
    }
  #if TARGET_OS_MAC
    // also stop enumeration of Appletalk hosts
    if (mConfigAppletalk) {
        OSErr err = ProtocolEndEnumeration(mConfigAppletalk);
        if (err != noErr) {
            DEBUG_OUT("Client OpenPlay ProtocolEndEnumeration() (Appletalk) returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        }
        // dispose of the configuration used
        err = ProtocolDisposeConfig(mConfigAppletalk);
        if (err != noErr) {
            DEBUG_OUT("Client OpenPlay ProtocolDisposeConfig() (Appletalk) returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        }
        mConfigAppletalk = NULL; // just making sure
    }
  #endif
}

// have to do this because CW Pro 8 seems to generate bad code here if the optimization level is not zero
// which causes crashes inside the OpenPlay ProtocolIdleEnumeration() call of GalacticaClient::SpendTime()
// this only needs to be around GalacticaClient::ClientEnumerationCallback as far as we know, but putting it
// around all these static OpenPlay callbacks just in case
#if COMPILER_MWERKS
#pragma optimization_level 0
#endif // COMPILER_MWERKS

//static
void
GalacticaClient::ClientCallback (PEndpointRef inEndpoint, void* inContext,
	NMCallbackCode inCode, NMErr inError, void* inCookie) {

	GalacticaClient* theGame = static_cast<GalacticaClient*>(inContext);
#warning NOTE: when quitting the above can produce a kNMCloseComplete with a context that was deleted 

//	ASSERT(theGame != NULL);
	
//	if (inError != noErr) {
//	    DEBUG_OUT("ClientCallback() got error code "<<inError, DEBUG_ERROR | DEBUG_PACKETS);
//    }

	switch(inCode)
	{
		case kNMConnectRequest:
			ASYNC_DEBUG_OUT("ClientCallback() got unexpected connect request", DEBUG_ERROR | DEBUG_PACKETS);
			ProtocolRejectConnection(inEndpoint, inCookie);
			break;

		case kNMDatagramData:
/*			{
				NMErr err;
				char buffer[1024];
				unsigned long data_length= sizeof(buffer);
				NMFlags flags;
				
				err= ProtocolReceivePacket(inEndpoint, buffer, &data_length, &flags);
				interrupt_safe_log_text("Got kNMDatagramData: Err %d Data: 0x%x Size: 0x%x flags: 0x%x",
					err, buffer, data_length, flags);
				buffer[data_length]= 0;
				interrupt_safe_log_text("Got kNMDatagramData: **%s**", buffer);
			} */
			break;
			
		case kNMStreamData:
	        ASYNC_DEBUG_OUT("ClientCallback() receiving "<<inError<<" bytes of data", DEBUG_DETAIL | DEBUG_PACKETS);
		    theGame->HandleReceiveData(inEndpoint);
			break;

		case kNMFlowClear:
		    ASYNC_DEBUG_OUT("Client Got FlowClear", DEBUG_IMPORTANT | DEBUG_PACKETS);

		case kNMEndpointDied:
		    ASYNC_DEBUG_OUT("Client Got EndpointDied " << inEndpoint << " Error = " << inError, DEBUG_IMPORTANT | DEBUG_PACKETS);
            ProtocolCloseEndpoint(inEndpoint, false);
            if (inEndpoint == theGame->mEndpoint) {
                theGame->mEndpoint = NULL;
                theGame->mConnectedHostInfo.gameid = 0;
                theGame->mConnectedHostInfo.hostid = 0;
                theGame->mConnectedHostInfo.hostname[0] = 0;
                theGame->mConnectedHostInfo.playername[0] = 0;
                theGame->mConnectedHostInfo.key = 0;
                theGame->mConnectedHostInfo.type = gameType_LAN;
                HostDisconnectPacket* hdp = CreatePacket<HostDisconnectPacket>();
                hdp->reasonCode = inError;
                theGame->HandlePacket(hdp);
                ReleasePacket(hdp);
            }
			break;

		case kNMCloseComplete:
		    // normal, but nothing to do about it, this is the last we'll ever hear from this connection
			break;
			
		case kNMAcceptComplete:
		case kNMHandoffComplete:
		default:
		    ASYNC_DEBUG_OUT("Client Got illegal message "<< inCode << " " << inCookie << " Error = " << inError, DEBUG_ERROR | DEBUG_PACKETS);
			break;
	}

	return;
}

//static
void
GalacticaClient::ClientEnumerationCallback(void *inContext, NMEnumerationCommand inCommand, NMEnumerationItem *item) {
	GalacticaClient* theGame = static_cast<GalacticaClient*>(inContext);
	theGame->HandleClientEnumeration(inCommand, item, kIPModuleType);
}

#if TARGET_OS_MAC
//static
void
GalacticaClient::ClientAppleTalkEnumerationCallback(void *inContext, NMEnumerationCommand inCommand, NMEnumerationItem *item) {
	GalacticaClient* theGame = static_cast<GalacticaClient*>(inContext);
	theGame->HandleClientEnumeration(inCommand, item, kATModuleType);
}
#endif // TARGET_OS_MAC

#if COMPILER_MWERKS
#pragma optimization_level reset
#endif // COMPILER_MWERKS

void
GalacticaClient::HandleReceiveData(PEndpointRef inEndpoint) {
	NMErr err = kNMNoError;
	UInt32 dataLen;
    void* dataPtr;
	NMFlags flags;
	
    // repeat until we get an error code (no data left is returned as an error)
	while (err == kNMNoError) {
        // --------------------------------------------
        // Header reassembly and deframing section
        // --------------------------------------------
        if (!mInProgressPacket) {  // there is no partially received packet, so we are starting at the header
            if (mOffsetInHeader > 0) {
                // there is a partially received header
                dataPtr = (void*)( (char*)&mPacketInfo + mOffsetInHeader );
                ASYNC_DEBUG_OUT("Continuing receipt of partial packet header, dataPtr = " << dataPtr
                                << ", offsetInHeader = " << mOffsetInHeader << ", byteRemainingForHeader ="
                                << mBytesRemainingForHeader, DEBUG_TRIVIA | DEBUG_PACKETS);
            } else {
                // no partially received header, fetch the whole thing
                mBytesRemainingForHeader = sizeof(mPacketInfo);
        	    dataPtr = &mPacketInfo;
        	}
            dataLen = mBytesRemainingForHeader;
           	err = ProtocolReceive(inEndpoint, dataPtr, &dataLen, &flags);
//           		ASYNC_DEBUG_OUT("Client ProtocolReceive returned "<<err
//           		            << ": mPacketInfo.packetType = "<<(int)mPacketInfo.packetType
//           		            << ", mPacketInfo.packetLen = "<<mPacketInfo.packetLen
//           		            << ", dataLen = "<<dataLen <<", flags = "<<(int)flags, DEBUG_TRIVIA | DEBUG_PACKETS);
            if (err == kNMNoError) {

                if (dataLen != mBytesRemainingForHeader) {
                    ASYNC_DEBUG_OUT("Client ProtocolReceive() only returned "<<dataLen<<" bytes, not the "
                                << mBytesRemainingForHeader << " bytes required for the header", 
                                DEBUG_TRIVIA | DEBUG_PACKETS);
                    err = kNMNoDataErr;  // don't try to get more data for this packet now
                }
                mBytesRemainingForHeader -= dataLen;
                mOffsetInHeader += dataLen;
                ASYNC_ASSERT(mBytesRemainingForHeader >= 0);  // just making sure, Open Play shouldn't do this
            } else if (err != kNMNoDataErr) {  // don't report when we got no data
               ASYNC_DEBUG_OUT("Client HandleReceiveData ProtocolReceive for packet header returned "<<err, 
                               DEBUG_ERROR | DEBUG_PACKETS);
               return;
            }
            // Check for packet header complete
            if (mBytesRemainingForHeader == 0) {
              #ifdef V3FRAMING
                mInProgressPacket = CreatePacket(mPacketInfo.packetType, mPacketInfo.v3packetLen);
              #else
                mInProgressPacket = CreatePacket(mPacketInfo.packetType, mPacketInfo.packetLen);
              #endif
//        				ASYNC_DEBUG_OUT("Client CreatePacket returned "<<(void*)mInProgressPacket
//        				            << ": mInProgressPacket->packetType = "<<(int)mInProgressPacket->packetType
//        				            << ", mInProgressPacket->packetLen = "<<mInProgressPacket->packetLen, 
//                                    DEBUG_TRIVIA | DEBUG_PACKETS);
                if (!mInProgressPacket) {
                   ASYNC_DEBUG_OUT("Client HandleReceiveData failed to allocate incoming packet data, len = "
                               <<mPacketInfo.packetLen, DEBUG_ERROR | DEBUG_PACKETS);
                   return;
                }
              #ifdef NUMBER_ALL_PACKETS
                mInProgressPacket->packetNum = mPacketInfo.packetNum;
              #endif
                mBytesRemainingForPacket = (SInt32)mInProgressPacket->packetLen - sizeof(mPacketInfo);
                mOffsetInPacket = PACKET_DATA_OFFSET;
                mOffsetInHeader = 0;  // reset for header of next packet
            }
        }
        // --------------------------------------------
        // Packet body reassembly section
        // --------------------------------------------
    	if (err == kNMNoError) {
//        				ASYNC_DEBUG_OUT("Client bytes remaining for packet = " << mBytesRemainingForPacket
//    				                << ", mInProgressPacket->packetLen = "<<mInProgressPacket->packetLen,
//                                    DEBUG_TRIVIA | DEBUG_PACKETS);
    	    if (mBytesRemainingForPacket > 0) {  // handle packets that are just the header
    		    dataLen = mBytesRemainingForPacket;
    	        dataPtr = (void*)( (char*)mInProgressPacket + mOffsetInPacket );
//            				    ASYNC_DEBUG_OUT("Client datPtr = " << dataPtr
//        				                << ": (void*)( (char*)mInProgressPacket + mOffsetInPacket ) ["
//        				                <<(void*)mInProgressPacket << "] + ["
//        				                <<mOffsetInPacket<<"]", DEBUG_TRIVIA | DEBUG_PACKETS);
    		    err = ProtocolReceive(inEndpoint, dataPtr, &dataLen, &flags);
//            				ASYNC_DEBUG_OUT("Client ProtocolReceive returned "<<err
//            				            << ", dataLen = "<<dataLen <<", flags = "<<(int)flags,
//            				            DEBUG_TRIVIA | DEBUG_PACKETS);
                if (err == kNMNoError) {
                    if (dataLen != mBytesRemainingForPacket) {
                        ASYNC_DEBUG_OUT("Client ProtocolReceive() only returned "<<dataLen<<" bytes, not the "
                                    << mBytesRemainingForPacket << " bytes required", 
                                    DEBUG_DETAIL | DEBUG_PACKETS);
                    }
                    mBytesRemainingForPacket -= dataLen;
                    mOffsetInPacket += dataLen;
                    ASYNC_ASSERT(mBytesRemainingForPacket >= 0);  // just making sure. OpenPlay should never do this
                } else if (err != kNMNoDataErr) {
           	        ASYNC_DEBUG_OUT("Client HandleReceiveData ProtocolReceive for packet body returned "<<err, 
           	                        DEBUG_ERROR | DEBUG_PACKETS);
                    return;
                }
     		}
     		if (mBytesRemainingForPacket == 0) {
     		    // we got all the data for the packet
        	    HandlePacket(mInProgressPacket);
        	    ReleasePacket(mInProgressPacket);
        	    mInProgressPacket = NULL;
    	    }
    	}
	}
}

void
GalacticaClient::HandleClientEnumeration(NMEnumerationCommand inCommand, NMEnumerationItem *item, long inProtocolType) {
    ASYNC_DEBUG_OUT("HandleClientEnumeration", DEBUG_IMPORTANT | DEBUG_PACKETS);
    GameType theGameType = gameType_LAN;
    if (inProtocolType == kATModuleType) {
        theGameType = gameType_AppleTalk;
    }
    switch(inCommand) {
        case kNMEnumAdd: {
                ASYNC_DEBUG_OUT("Client got enumeration ADD command", DEBUG_DETAIL | DEBUG_PACKETS);
                GameJoinInfo gji(item->name, "", "", "", 0, item->id, 0, theGameType);
                AddGameToKnownList(gji);
                if (mJoinGameHandler) {
                    mJoinGameHandler->ArrayChanged();
                }
            }
            break;
        case kNMEnumDelete: {
                ASYNC_DEBUG_OUT("Client got enumeration DELETE command", DEBUG_DETAIL | DEBUG_PACKETS);
                GameJoinInfo gji(item->name, "", "", "", 0, item->id, 0, theGameType);
                RemoveGameFromKnownList(gji);
                if (mJoinGameHandler) {
                    mJoinGameHandler->ArrayChanged();
                }
            }
            break;
        case kNMEnumClear:
            ASYNC_DEBUG_OUT("Client got enumeration CLEAR command", DEBUG_DETAIL | DEBUG_PACKETS);
/*          ERZ, do nothing with a clear so we don't erase existing items
            mHostNameArray.RemoveAllItemsAfter(0);
            mHostIdArray.RemoveAllItemsAfter(0);
            if (mJoinGameHandler) {
                mJoinGameHandler->ArrayChanged();
            }
*/
            break;
        default:
            ASYNC_DEBUG_OUT("Client got unknown enumeration command ["<<(int)inCommand<<"]", DEBUG_ERROR | DEBUG_PACKETS);
            break;
    }
  #ifdef DEBUG
    if (item) {
        ASYNC_DEBUG_OUT("Client enumeration data: name="<<item->name<<", id="<<item->id
                <<", customDataLen="<<item->customEnumDataLen, DEBUG_DETAIL | DEBUG_PACKETS);
    }
  #endif
}

void
GalacticaClient::HandlePacket(Packet* inPacket) {
    // host sent us a packet
    RecordPacketReceived(inPacket);
    ASYNC_DEBUG_OUT("Client got packet "<<inPacket->GetName() << " [" << inPacket->packetNum << "] from Host", 
                    DEBUG_IMPORTANT | DEBUG_PACKETS);
    switch (inPacket->packetType) {
        case packet_PlayerToPlayerMessage:
        case packet_HostDisconnect:
        case packet_LoginResponse:
        case packet_TurnComplete:
        case packet_PrivatePlayerInfo:
        case packet_GameInfo:
        case packet_PlayerInfo:
        case packet_CreateThingyResponse:
        case packet_ThingyCount:
        case packet_ReassignThingyResponse:
            {
                // these packets need to be queued and handled at idle time
                Packet* packet = ClonePacket(inPacket);
                pdg::AutoCriticalSection crit(&mPacketQueueCriticalSection);
                mPackets.push(packet);
            }
            break;
        case packet_ThingyData:
            {
                // we push these into a special queue which holds everything until we get a TurnCompletePacket
                Packet* packet = ClonePacket(inPacket);
                pdg::AutoCriticalSection crit(&mPacketQueueCriticalSection);
                mThingyPackets.push(static_cast<ThingyDataPacket*>(packet));
                mNumDownloadedThingys++;
            }
            break;
        case packet_Ping: // ignore the ping response from the host
        case packet_Ignore:
        default:
            break;
    }
}

void
GalacticaClient::SendPacket(Packet* inPacket) {
    // send packet to host
    if (mEndpoint == NULL) {
        DEBUG_OUT("Client SendPacket "<<inPacket->GetName() << " failed, not connected", DEBUG_ERROR | DEBUG_PACKETS);
        return;
    }
    if (inPacket->packetLen >= MAX_PACKET_LEN) {
        DEBUG_OUT("Client SendPacket "<<inPacket->GetName() << " failed, too large, size = " 
                    << inPacket->packetLen, DEBUG_ERROR | DEBUG_PACKETS);
        return;
    }
    if (inPacket->packetLen < sizeof(Packet) ) {
        DEBUG_OUT("Client SendPacket "<<inPacket->GetName() << " failed, size not set, size = " 
                    << inPacket->packetLen, DEBUG_ERROR | DEBUG_PACKETS);
    }
#ifdef V3FRAMING
    // for communication with a version 3 host, which uses the Pixel Dust framework's network layer
    // we need to put the packet length into a new location, and adjust the packet length of the
    // galactica packet to remove the extra length added by the PDGF network framing
    inPacket->v3packetLen = inPacket->packetLen;
    inPacket->packetLen -= sizeof(V3Packet);
    long sentBytes = ProtocolSend(mEndpoint, (void*)inPacket, inPacket->v3packetLen, 0);
#else  
    long sentBytes = ProtocolSend(mEndpoint, (void*)inPacket, inPacket->packetLen, 0);
#endif // V3FRAMING
    if (sentBytes != inPacket->packetLen) {
        DEBUG_OUT("Client SendPacket "<<inPacket->GetName() << " failed to send entire contents, size = "
                    <<inPacket->packetLen<<", sent = " << sentBytes, DEBUG_ERROR | DEBUG_PACKETS);
    }
    RecordPacketSent(inPacket);
    DEBUG_OUT("Client sent "<<inPacket->GetName() << " to host", DEBUG_DETAIL | DEBUG_PACKETS);
}


void
GalacticaClient::HandleQueuedPacket(Packet* inPacket) {
    // host sent us a packet
    if (!inPacket) {
        DEBUG_OUT("Client had NULL queued packet", DEBUG_ERROR | DEBUG_PACKETS);
        return;
    }
    DEBUG_OUT("Client handling queued packet "<<inPacket->GetName() << " ["<<inPacket->packetNum<<"]", DEBUG_IMPORTANT | DEBUG_PACKETS);
    switch (inPacket->packetType) {
        case packet_PlayerToPlayerMessage:
            {
                PlayerToPlayerMessagePacket* p = static_cast<PlayerToPlayerMessagePacket*>(inPacket);
                HandlePlayerToPlayerMessage(p->fromPlayer, p->toPlayer, p->messageText);
            }
            break;
        case packet_LoginResponse:
            {
                LoginResponsePacket* p = static_cast<LoginResponsePacket*>(inPacket);
                HandleLoginResponse(p->playerNum, p->loginResult, p->rejoinKey);
            }
            break;
        case packet_PrivatePlayerInfo:
            {
                PrivatePlayerInfoPacket* p = static_cast<PrivatePlayerInfoPacket*>(inPacket);
                HandlePrivatePlayerInfo(p->playerNum, p->info);
            }
            break;
        case packet_TurnComplete:
            HandleTurnComplete(static_cast<TurnCompletePacket*>(inPacket));
            break;
        case packet_HostDisconnect:
            HandleHostDisconnect(static_cast<HostDisconnectPacket*>(inPacket));
            break;
        case packet_GameInfo:
            HandleGameInfo(static_cast<GameInfoPacket*>(inPacket));
            break;
        case packet_PlayerInfo:
            HandlePlayerInfo(static_cast<PlayerInfoPacket*>(inPacket));
            break;
        case packet_CreateThingyResponse:
            HandleCreateThingyResponse(static_cast<CreateThingyResponsePacket*>(inPacket));
            break;
        case packet_ThingyCount:
            HandleThingyCount(static_cast<ThingyCountPacket*>(inPacket));
            break;
        case packet_ReassignThingyResponse:
            HandleReassignThingyResponse(static_cast<ReassignThingyResponsePacket*>(inPacket));
            break;
        default:
            break;
    }
}

// must already be connected to a game host via ConnectToHost() or ConnectToNamedHost()
void 
GalacticaClient::SendLoginRequest(const char* inPlayerName, const char* inPassword, bool inAsNewPlayer) {
   mPlayerName = inPlayerName;   // save the player name so it can be added to the window title
   // we should already be connected, do a login request
   LoginRequestPacket* lrp = CreatePacket<LoginRequestPacket>(std::strlen(inPlayerName)+1);
   if (lrp) {
      std::strcpy(lrp->playerName, inPlayerName);
      std::strncpy(lrp->password, inPassword, MAX_PASSWORD_LEN);
      lrp->password[MAX_PASSWORD_LEN] = 0; // nul terminate the password
      std::strcpy(lrp->copyrightStr, string_Copyright);
      lrp->clientVers = version_HostProtocol;
      if (!Galactica::Globals::GetInstance().sRegistered) {
         lrp->regCode[0] = 0;
      } else {
         // copy the reg code and obscure it as you go
         for (int i = 0; i< strLen_RegCode; i++) {
            lrp->regCode[i] = Galactica::Globals::GetInstance().sRegistration[i+1] ^ 0xef;
         }
         lrp->regCode[strLen_RegCode] = 0;
      }
      // include the client id so we know if this is a hotseat game or not
      lrp->setJoinInfo(inAsNewPlayer, Galactica::Globals::GetInstance().getClientID());
      DEBUG_OUT("Galactica Client sending login request: version ["<<(void*)lrp->clientVers
            << "], copyright string ["<<lrp->copyrightStr<<"], player name ["
            << lrp->playerName << "], password [" << lrp->password << "]"
            << " isNew ["<< (lrp->isNew() ? "true]":"false]"),
            DEBUG_IMPORTANT | DEBUG_PACKETS);
      SendPacket(lrp);
      ReleasePacket(lrp);
      mJoining = true;    // some messages will not be shown when joining
   }
}


void
GalacticaClient::HandlePlayerToPlayerMessage(SInt8 fromPlayer, SInt8 toPlayer, const char* text) {
   DEBUG_OUT("Client handling Player to Player message", DEBUG_DETAIL | DEBUG_PACKETS | DEBUG_MESSAGE);
   bool toAll = (toPlayer == 0);
   if (toAll) {
      // if we get a message to all players, switch to group chat
      DoChat(kAllPlayers);      // this will open the window if it isn't already open
   } else {
      DoChat(fromPlayer);
   }
   mChatHandler->AddChatToHistory(fromPlayer, text, toAll );
}

void
GalacticaClient::HandleLoginResponse(SInt8 asPlayer, SInt8 inResult, UInt32 inRejoinKey) {
    if (inResult == login_ResultSuccess) {
    	DEBUG_OUT("Got join game success packet for player "<<(int)asPlayer<<", completing join", 
    	        DEBUG_IMPORTANT | DEBUG_PACKETS);
    	try {
    		// update the known games list with the rejoin key
    		mConnectedHostInfo.key = inRejoinKey;
    		mNextPingMs = pdg::OS::getMilliseconds() + GALACTICA_CLIENT_PING_INTERVAL;
    		mNextPingNum = 0;
    		AddGameToKnownList(mConnectedHostInfo);
            SaveKnownGamesList();
    		mPlayerNum = asPlayer;
    		MakeWindow();
    		ShowWindow(kPlayMessages);			// and put up the window
    	  #ifndef NO_GAME_RANGER_SUPPORT
    	    GRGameBegin();  // let GameRanger know that we've started playing
    	  #endif
    	}
    	catch (LException& e) {
    		DEBUG_OUT("Exception "<<e.what()<<" trying to create new game", DEBUG_ERROR | DEBUG_DATABASE);
    		delete this;
    		if (e.GetErrorCode() != permErr) {
    			DEBUG_OUT("...informing user of Exception "<<e.what(), DEBUG_ERROR | DEBUG_DATABASE | DEBUG_USER);
    			LStr255 errStr(STRx_Errors, str_ErrorOccured);
    			LStr255 errNumStr = (SInt32)e.GetErrorCode();
    			errStr.Append(errNumStr);
    			errStr.Append(" (");
    			errStr.Append(e.what());
    			errStr.Append(")");
    			::ParamText(errStr, "\p", "\p", "\p");
    			UModalAlerts::StopAlert(alert_Error);
    		}
    	}
    } else {
    	DEBUG_OUT("Error "<<(int)inResult<<" trying to join game", DEBUG_ERROR | DEBUG_PACKETS);
    	LStr255 loginErrorStr(STRx_Errors, (short)inResult);
    	::ParamText(loginErrorStr, "\p", "\p", "\p");
    	UModalAlerts::StopAlert(alert_Error);
    }
}

void
GalacticaClient::HandlePrivatePlayerInfo(SInt8 forPlayer, PrivatePlayerInfoT& inInfo) {
    DEBUG_OUT("HandlePrivatePlayerInfo for Player "<<(int)forPlayer 
                <<": LTP=" << inInfo.lastTurnPosted 
                << " PR="<< inInfo.numPiecesRemaining << " HID=" << inInfo.homeID
                << " TSID="<< inInfo.techStarID << " HStT=" << inInfo.hiStarTech
                << " HSpT="<< inInfo.hiShipTech << " CS=" << (int)inInfo.compSkill
                << " CA=" << (int)inInfo.compAdvantage, DEBUG_IMPORTANT | DEBUG_PACKETS);
    if (forPlayer == GetMyPlayerNum()) {
        mPlayerInfo = inInfo; // set our private player info
        DEBUG_OUT("New info for Player "<<(int)forPlayer 
                    <<": LTP=" << mPlayerInfo.lastTurnPosted 
                    << " PR="<< mPlayerInfo.numPiecesRemaining << " HID=" << mPlayerInfo.homeID
                    << " TSID="<< mPlayerInfo.techStarID << " HStT=" << mPlayerInfo.hiStarTech
                    << " HSpT="<< mPlayerInfo.hiShipTech << " CS=" << (int)mPlayerInfo.compSkill
                    << " CA=" << (int)mPlayerInfo.compAdvantage, DEBUG_IMPORTANT | DEBUG_PACKETS);
    	if (GetMyPlayerNum() != kAdminPlayerNum) {
    		if (mPlayerInfo.lastTurnPosted == mGameInfo.currTurn) {
    			mRejoiningGameInPostedTurn = true;
    		} else {
    			mRejoiningGameInPostedTurn = false;
    		}
	    	if (mPlayerInfo.lastTurnPosted < (mGameInfo.currTurn-1)) {
	    	   // don't inform admin of missed turns, they never post
	    		DEBUG_OUT("Informing user of Missed turns: last = "<<mPlayerInfo.lastTurnPosted
	    		            << ", curr = "<< mGameInfo.currTurn, DEBUG_ERROR | DEBUG_DATABASE | DEBUG_USER);
	    		LStr255 lastStr = (SInt32) mPlayerInfo.lastTurnPosted;
	    		LStr255 currStr = (SInt32) mGameInfo.currTurn;
	    	    MovableParamText(0, lastStr);
	    	    MovableParamText(1, currStr);
	    	    MovableAlert(window_MissedTurns, NULL, 10);  // autoclose after 5 seconds (10 half-seconds)
    	    }
    	    // tell the server what our defaults are for new systems
   			SendClientSettings();
    	}
    } else {
        DEBUG_OUT("Client "<<GetMyPlayerNum() <<" got a Private Player Info record for "
                    <<(int)forPlayer, DEBUG_ERROR | DEBUG_PACKETS);
    }
}

void
GalacticaClient::SendClientSettings() {
    // tell the server what our defaults are for new systems
	ClientSettingsPacket* csp = CreatePacket<ClientSettingsPacket>();
    long tempLong;
    pdg::ConfigManager* config = Galactica::Globals::GetInstance().getConfigManager();
	config->getConfigLong(GALACTICA_PREF_DEFAULT_BUILD, tempLong);
	csp->defaultBuildType = tempLong;
	config->getConfigLong(GALACTICA_PREF_DEFAULT_GROWTH, tempLong);
	csp->defaultGrowth = tempLong;
	config->getConfigLong(GALACTICA_PREF_DEFAULT_TECH, tempLong);
	csp->defaultTech = tempLong;
	config->getConfigLong(GALACTICA_PREF_DEFAULT_SHIPS, tempLong);
	csp->defaultShips = tempLong;
	SendPacket(csp);
}

void
GalacticaClient::HandleGameInfo(GameInfoPacket* inPacket) {
   DEBUG_OUT("HandleGameInfo packet.isFull = "<<(int)inPacket->isFull<< " currTurn = "
             <<inPacket->currTurn, DEBUG_IMPORTANT | DEBUG_PACKETS);
   long turnNum = mGameInfo.currTurn;
   mGameInfo.currTurn = inPacket->currTurn;
   bool newTurn = mGameInfo.currTurn > turnNum;   // detect a new turn
   mGameInfo.maxTimePerTurn = inPacket->maxTimePerTurn;
   if (newTurn) {
      mAutoEndTurnTimer.ClearMark();
   }
   mAutoEndTurnTimer.SetTimeRemaining(inPacket->secsRemainingInTurn);
   mGameInfo.secsRemainingInTurn = mAutoEndTurnTimer.GetTimeRemaining();
   mGameInfo.totalNumPlayers = inPacket->totalNumPlayers;
   mGameInfo.numPlayersRemaining = inPacket->numPlayersRemaining;
   mGameInfo.numHumans = inPacket->numHumans;
   mGameInfo.numComputers = inPacket->numComputers;
   mGameInfo.numHumansRemaining = inPacket->numHumansRemaining;
   mGameInfo.numComputersRemaining = inPacket->numComputersRemaining;
   mGameInfo.compSkill = inPacket->compSkill;
   mGameInfo.compAdvantage = inPacket->compAdvantage;
   mGameInfo.sectorDensity = inPacket->sectorDensity;
   mGameInfo.sectorSize = inPacket->sectorSize;
   mGameInfo.omniscient = inPacket->omniscient;
   mGameInfo.fastGame = inPacket->fastGame;
   mGameInfo.endTurnEarly = inPacket->endTurnEarly;
   mGameInfo.timeZone = inPacket->timeZone;
   mGameInfo.gameID = inPacket->gameID;
   if (inPacket->numPlayersRemaining > 1) {
      if (inPacket->isFull) {
         mGameInfo.gameState = (EGameState)(state_NormalPlay + state_FullFlag);
      } else {
         mGameInfo.gameState = state_NormalPlay;
      }
   } else {
      mGameInfo.gameState = state_PostGame;
   }
   c2pstrcpy(mGameInfo.gameTitle, inPacket->gameTitle);
   // if there is a join game handler (ie: we are browsing for a game 
   // to join) then we must notify it that the game data has changed
   if (mJoinGameHandler) {
        // we just got the game id as reported by the host, there are a couple of
        // possibilities here:
        // 1) this could be a known game from the gamelist.txt, so we want to
        //    verify that the ids match
        // 2) this could be a game newly added to the join list, which is
        //    is just now getting the information, so we need to assign the id
        if (mConnectedHostInfo.gameid == gameID_Unknown) {
            mConnectedHostInfo.gameid = mGameInfo.gameID;
            mConnectedHostInfo.valid = true;
        } else if (mConnectedHostInfo.gameid == mGameInfo.gameID) {
            mConnectedHostInfo.valid = true;
        } else {
            mConnectedHostInfo.gameid = mGameInfo.gameID;
            mConnectedHostInfo.valid = false;
        }
      #warning TODO: change game ip address in join list to actual game name
        mJoinGameHandler->GameInfoChanged();
   }
   LogGameInfo();
	if (mGameInfo.maxTimePerTurn) {
		mAutoEndTurnTimer.Resume();
	} else {
		mAutoEndTurnTimer.Clear();	// make timer read blank
	}
}


void
GalacticaClient::HandlePlayerInfo(PlayerInfoPacket* inPacket) {
    DEBUG_OUT("Client HandlePlayerInfo", DEBUG_IMPORTANT | DEBUG_PACKETS);
    // check for players entering and leaving
    PublicPlayerInfoT oldInfo;
    GetPlayerInfo(inPacket->playerNum, oldInfo);
    SetPlayerInfo(inPacket->playerNum, inPacket->info);
  #ifndef NO_GAME_RANGER_SUPPORT
    // check to see if this is someone going out of the game, and report it to GameRanger
    if (oldInfo.isAlive && !inPacket->info.isAlive) {
        if (inPacket->playerNum == GetMyPlayerNum()) {
            DEBUG_OUT("Reporting I Lost to GameRanger for player "<< GetMyPlayerNum(),
                    DEBUG_IMPORTANT);
            // if this was us, we need to report it differently
            GRStatScore(0, GetMyPlayerNum());  // zero score means player lost
        } else {
            DEBUG_OUT("Reporting to GameRanger that player "<< (int)inPacket->playerNum << " lost",
                    DEBUG_IMPORTANT);
            GRStatOtherScore(0, inPacket->playerNum);
        }
    }
    // also, check to see if the game is over this turn, and report whoever is alive
    // as the winner
    if (mGameInfo.gameState == state_PostGame) {
        if (inPacket->info.isAlive) {
            if (inPacket->playerNum == GetMyPlayerNum()) {
                DEBUG_OUT("Reporting I Won to GameRanger for player "<< GetMyPlayerNum(),
                        DEBUG_IMPORTANT);
                // if this was us, we need to report it differently
                GRStatScore(1, GetMyPlayerNum());  // score=1 means player won
            } else {
                DEBUG_OUT("Reporting to GameRanger that player "<< (int)inPacket->playerNum 
                        << " won", DEBUG_IMPORTANT);
                GRStatOtherScore(1, inPacket->playerNum);
            }
        }
    }
  #endif
    // check to see if the chat handler is alive, this could be notification that
    // it needs to update the chat display
    if (mChatHandler) {
       mChatHandler->UpdatePlayerStatus(inPacket->playerNum);
    }
    // check to see if this is a notification about another assigned
    // human player during normal game operation (rather than while we
    // are just entering ourselves). If so, we inform user of the change
    if (!mJoining && (inPacket->playerNum != GetMyPlayerNum()) && inPacket->info.isAssigned) { // if this player is assigned
        if (oldInfo.isLoggedIn != inPacket->info.isLoggedIn) { // login status changed
            MovableParamText(0, inPacket->info.name);
            if (oldInfo.isLoggedIn) {   // they were logged in, so they have been disconnected
                MovableAlert(window_PlayerDisconnected, NULL, 6);
            } else { // they weren't logged in, so they have just joined
                MovableAlert(window_PlayerConnected, NULL, 6);
            }
        }
    }
}

void
GalacticaClient::HandleThingyCount(ThingyCountPacket* inPacket) {
   DEBUG_OUT("Client HandleThingyCount", DEBUG_IMPORTANT | DEBUG_PACKETS);
   mNumDownloadedThingys = 0;
   try {
      mProgressWind = LWindow::CreateWindow(window_ReadingGalaxy, this);
   }
   catch(...) {
      DEBUG_OUT("Caught exception in CreateWindow()", DEBUG_ERROR);
   }
   if (mProgressWind) {
      DEBUG_OUT("Created Progress Window ", DEBUG_DETAIL | DEBUG_USER | DEBUG_PACKETS);
      mProgressBar = static_cast<LThermometerPane*>( mProgressWind->FindPaneByID(progressBar_ThingyProgress) );
      if (mProgressBar) {
         DEBUG_OUT("Set Progress Bar MaxValue = "<<inPacket->thingyCount, DEBUG_DETAIL | DEBUG_USER | DEBUG_PACKETS);
         mProgressBar->SetMaxValue(inPacket->thingyCount * 2);
         mProgressWind->Show();
      } else {
         DEBUG_OUT("Failed to find progress bar "<<(int)progressBar_ThingyProgress, DEBUG_ERROR | DEBUG_USER | DEBUG_PACKETS);
      }
   } else {
      DEBUG_OUT("Failed to create Progress Window "<<(int)window_ReadingGalaxy, DEBUG_ERROR | DEBUG_USER | DEBUG_PACKETS);
   }
}

void
GalacticaClient::HandleThingyData(ThingyDataPacket* inPacket) {
    DEBUG_OUT("Client HandleThingyData", DEBUG_IMPORTANT | DEBUG_PACKETS);
    AGalacticThingy* aThingy = dynamic_cast<AGalacticThingyUI*>(FindThingByID(inPacket->id));
    ASSERT(inPacket->action != -1);
    if (inPacket->action == action_Deleted) {
        // things that are to be deleted will be pushed into a queue for deletion later
        aThingy = FindThingByID(inPacket->id);
        if (aThingy) {
            mDeleteQueue.push(aThingy);
        }
        return;
    } else if (inPacket->action == action_Added) {
        // if it needed to be added, but was already there, it would be an error
        // unless it was a rendezvous point or a fleet
        if ( (inPacket->type != thingyType_Rendezvous) && (inPacket->type != thingyType_Fleet)) {
            ASSERT(aThingy == NULL);
        }
    }
    if (!aThingy) {
        aThingy = MakeThingy(inPacket->type);
        ASSERT(aThingy != NULL);
        if (aThingy) {
            aThingy->SetID(inPacket->id);
        } 
    }
    // now read the thingy data out of the packet if there is a thingy
    if (aThingy) {
        bool setChanged = ((inPacket->action == action_Updated) && aThingy->IsChanged());
        aThingy->SetID(inPacket->id);   // really only needed for 
	    if (aThingy->ReadFromPacket(inPacket)) {
    	    AGalacticThingyUI* ui = aThingy->AsThingyUI();
    	    if (ui) {
    	        ui->CalcPosition(kRefresh);
    	        // Fog of War
    	       #warning TODO: should really check to see if position changed
    	        if (aThingy->HasScanners()) {
    	            aThingy->ScannerChanged();
    	        }
    	        aThingy->FlagForRescan();
    	    }
    	    if (setChanged) {
    	        // this will happen when the user made further changes to something
    	        // after having posted the turn. We preserve their changes when reading
    	        // from the stream, and we need to preserve the change flag too so
    	        // their changes will be written on the next turn
    	        aThingy->Changed();
    	    } else {
    	        aThingy->NotChanged();  // normally we just clear the changed flag
    	    }
            // deal with messages
            if (IS_ANY_MESSAGE_TYPE(inPacket->type)) {
                AddMessageToNeedyList((CMessage*)aThingy);
            }
        } else {
            DEBUG_OUT("Bad ReadFromPacket "<< aThingy<<" deleting", DEBUG_ERROR);
            delete aThingy;
        }
	}
}

void
GalacticaClient::HandleTurnComplete(TurnCompletePacket* inPacket) {
    DEBUG_OUT("Client HandleTurnComplete", DEBUG_IMPORTANT | DEBUG_PACKETS);
    bool wasJoining = mJoining;  // remember whether this was part of join or not
    mJoining = false;    // last packet to be received when joining a game
    if (inPacket->turnNum < mGameInfo.currTurn) {
        DEBUG_OUT("Something really screwy going on, turn complete = "<< inPacket->turnNum
                    << ", curr turn = "<<mGameInfo.currTurn, DEBUG_ERROR | DEBUG_PACKETS);
        // dump the entire thingy packet queue
        while (!mThingyPackets.empty()) {
            pdg::AutoCriticalSection crit(&mPacketQueueCriticalSection);
            ReleasePacket(mThingyPackets.front());
            mThingyPackets.pop();
        }
        // and dump the delete queue too
        while (!mDeleteQueue.empty()) {
            mDeleteQueue.pop();
        }
        return;
    }
    // pull all the queued thingy data packets off the queue and process them
    StartStreaming();
    int numStreamed = 0;
    while (!mThingyPackets.empty()) {
        DEBUG_OUT("Client HandleTurnComplete() processing one queued thingy data packet", DEBUG_DETAIL | DEBUG_PACKETS);
        ThingyDataPacket* p;
        {
            pdg::AutoCriticalSection crit(&mPacketQueueCriticalSection);
            p = mThingyPackets.front();
            mThingyPackets.pop();
        }
        HandleThingyData(p);
        numStreamed++;
	    // update progess bar if necessary
	    if (((numStreamed%10)==0) && (mProgressBar)) {
           int count = mNumDownloadedThingys + numStreamed;
	       mProgressBar->SetCurrentValue(count);
	    }
        ReleasePacket(p);
    }
    StopStreaming();
    // get rid of the download progress window
    if (mProgressWind) {
        DEBUG_OUT("Cleaning up progress window", DEBUG_DETAIL | DEBUG_USER | DEBUG_PACKETS);
        mProgressWind->Hide();
        delete mProgressWind;
        mProgressBar = NULL;
        mProgressWind = NULL;
    }
    // v2.1b7, put up a window and play a sound to tell us that the new turn is starting
	bool weAreFrontWindow = (UDesktop::FetchTopRegular() == mWindow);
	bool appInBackground = !LCommander::GetTopCommander()->IsOnDuty();
    LStr255 s;
	s = mGameInfo.currTurn;
	if (appInBackground || !weAreFrontWindow) {	// play a sound if we are in the background
		new CSoundResourcePlayer(snd_Warning);  // play sound to attract user's attention		    
	}
	if (weAreFrontWindow) {
      StopAllMouseTracking(); // in case they were in the middle of setting a course or something
		MovableParamText(0, s);
		MovableParamText(1, GetMyPlayerName());
		MovableAlert(window_BeginNewTurn, NULL, 3);	// autoclose after 1.5 seconds
	}
   // and delete all the deleted thingys
   while (!mDeleteQueue.empty()) {
        DEBUG_OUT("Client HandleTurnComplete() deleting one queued thingy from delete list", DEBUG_DETAIL | DEBUG_PACKETS);
        AGalacticThingyUI* it = ValidateThingyUI(mDeleteQueue.front());
        mDeleteQueue.pop();
        if (it) {
          #warning FIXME: this assert is firing every time
            ASSERT(it->IsDead());
            it->PutInside(NULL, false);
        } else {
            DEBUG_OUT("Found invalid item in client delete list "<<(void*)it, DEBUG_ERROR | DEBUG_PACKETS);
        }
   }
	DEBUG_OUT("EOT: Client updating visibility for Everything list", DEBUG_DETAIL | DEBUG_EOT);
	AGalacticThingy *aThingy;
    LArrayIterator iterator(mEverything, LArrayIterator::from_Start);
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
   }
   // update controls for new turn
   UpdateTurnNumberDisplay();
   mEndTurnButton->Enable();
   mEndTurnButton->Refresh();
   mEndTurnButton->UpdatePort();
   	// Show "POSTED" if rejoining a turn that's already been posted
   	if (mRejoiningGameInPostedTurn) {
		std::string postedStr;
		LoadStringResource(postedStr, STRx_General, str_Posted);
		if (mGameInfo.maxTimePerTurn > 0) {
			mAutoEndTurnTimer.Mark(postedStr.c_str());	// make timer read time remaining + "POSTED"
		} else {
			mAutoEndTurnTimer.Clear(postedStr.c_str());  // make timer read just "POSTED"
		}
	}
    // v2.1b7, do message playback
    if (appInBackground || !weAreFrontWindow) {
		mShowMessagesPending = true;	// another app or Window is in the front. Mark as pending ShowMessages().
    } else {
    	// figure out if we are rejoining a turn we've already
    	// posted. If so, we don't want to play messages here.
		bool oldWantMessages;
	   	if (mRejoiningGameInPostedTurn) {
			oldWantMessages = SetWantsMessages(false);
		}
		ShowMessages();
		ShowNextNeedyThingy(wasJoining); // v2.1b12, select home on first join 
	   	if (mRejoiningGameInPostedTurn) {
			SetWantsMessages(oldWantMessages);
			mRejoiningGameInPostedTurn = false; // don't keep this flag around or it will screw up next turn
		}
    }
}

void
GalacticaClient::HandleHostDisconnect(HostDisconnectPacket* ) {  // inPacket
    DEBUG_OUT("Client HandleHostDisconnected", DEBUG_IMPORTANT | DEBUG_PACKETS);
    if (!mDisconnecting) {
        mDisconnecting = true;
        if (mEndTurnButton) {
            mEndTurnButton->Disable();
            mEndTurnButton->Refresh();
            LStr255 errorStr(STRx_Errors, str_HostDisconnect);
            ::ParamText(errorStr, "\p","\p","\p");
            UModalAlerts::StopAlert(alert_Error);
            ObeyCommand(cmd_Close, NULL);
        }
    } else {
        DEBUG_OUT("Client skipping disconnect since we are already disconnecting", DEBUG_IMPORTANT | DEBUG_PACKETS);
    }
}


void
GalacticaClient::SendThingy(AGalacticThingy* inThingy) {
    ThingyDataPacket* p = inThingy->WriteIntoPacket();
    if (p) {
        p->action = action_Updated;
        SendPacket(p);
        ReleasePacket(p);
        DEBUG_OUT("Sent thingy "<< inThingy << " to host", DEBUG_TRIVIA | DEBUG_PACKETS);
    }
}

void
GalacticaClient::SendAllChangedThingys() {
	AGalacticThingy	*aThingy = nil;
	LArrayIterator iterator(mEverything, LArrayIterator::from_Start); // Search list of all thingys created
//	long startTicks = ::TickCount() + kTillTheCowsComeHome;
//	StDialogHandler *postingGalaxyDialog = nil;
	ValidateEverything();
	while (iterator.Next(&aThingy)) {
//			gBusyCursor->Set();
//		if (!postingGalaxyDialog && (::TickCount() > startTicks)) {
//			postingGalaxyDialog = new StDialogHandler(window_PostingGalaxy, this);
//			postingGalaxyDialog->GetDialog()->Draw(nil);
//  #warning need to flush window buffer in Carbon
//		}
		if ( aThingy->IsDead() ) {	        // look for items that are dead and delete them
			delete aThingy;
			continue;
		}
		if ( aThingy->IsChanged() ) {       // only send changed items
		    if ( (aThingy->GetOwner() == GetMyPlayerNum()) && aThingy->CanWriteback() ) {
			    SendThingy(aThingy);        // item is ours and expects writeback
		    }
			aThingy->NotChanged();		    // clear the changed flag
		}
	}
//	if (postingGalaxyDialog) {
//		delete postingGalaxyDialog;
//	}
}

void
GalacticaClient::SendCreateThingyRequest(AGalacticThingy* inThingy) {
    inThingy->AssignUniqueID();  // this will give us a temporary id
    CreateThingyRequestPacket* ctrp = CreatePacket<CreateThingyRequestPacket>();
    ctrp->where = inThingy->GetPosition();
    ctrp->tempID = inThingy->GetID();
    ctrp->type = inThingy->GetThingySubClassType();
    SendPacket(ctrp);
    ReleasePacket(ctrp);
    DEBUG_OUT("Client requested host creation of "<<this, DEBUG_IMPORTANT | DEBUG_PACKETS);
}

void
GalacticaClient::HandleCreateThingyResponse(CreateThingyResponsePacket* inPacket) {
    DEBUG_OUT("Client HandleCreateThingyResponse", DEBUG_IMPORTANT | DEBUG_PACKETS);
    AGalacticThingy* it = FindThingByID(inPacket->tempID);
    if (it) {
        it->SetID(inPacket->newID);
        DEBUG_OUT("Client reassigned new id to "<<it<< " (old id was "<<inPacket->tempID
                    <<")", DEBUG_DETAIL | DEBUG_PACKETS);
        UChangeIdReferencesAction changeIdsAction(inPacket->tempID, inPacket->newID);
        DoForEverything(changeIdsAction);
        SendAllChangedThingys();
    } else {
        DEBUG_OUT("No item with temporary id "<<inPacket->tempID<<" was found!", DEBUG_ERROR | DEBUG_PACKETS);
    }
}

void
GalacticaClient::SendReassignThingyRequest(AGalacticThingy* inThingy, AGalacticThingy* oldContainer) {
    ReassignThingyRequestPacket* rtrp = CreatePacket<ReassignThingyRequestPacket>();
    rtrp->id = inThingy->GetID();
    rtrp->type = inThingy->GetThingySubClassType();
    AGalacticThingy* newContainer = inThingy->GetPosition().GetThingy();
    PaneIDT oldID = (oldContainer != 0) ? oldContainer->GetID() : kInvalidID;
    PaneIDT newID = (newContainer != 0) ? newContainer->GetID() : kInvalidID;
    rtrp->oldContainer = oldID;
    rtrp->newContainer = newID;
    SendPacket(rtrp);
    ReleasePacket(rtrp);
    DEBUG_OUT("Client requested host reassignment of "<<inThingy<<" from container id "<< oldID 
                << " to container id " << newID, DEBUG_IMPORTANT | DEBUG_PACKETS);
}

void
GalacticaClient::HandleReassignThingyResponse(ReassignThingyResponsePacket* inPacket) {
    DEBUG_OUT("Client HandleReassignThingyResponse", DEBUG_IMPORTANT | DEBUG_PACKETS);
    if (inPacket->success) {
        DEBUG_OUT("Host successfully reassigned thingy id "<<inPacket->id<<" to new container", DEBUG_DETAIL | DEBUG_PACKETS | DEBUG_CONTAINMENT);
    } else {
        DEBUG_OUT("Host failed to reassign thingy id "<<inPacket->id<<" to new container", DEBUG_ERROR | DEBUG_PACKETS | DEBUG_CONTAINMENT);
    }
}

void
GalacticaClient::FetchMatchMakerGames() {
    // load games from the Galactica Match Maker
  #ifndef NO_MATCHMAKER_SUPPORT
    // inform the matchmaker host that we are hosting a game
    if (MatchMakerClient::matchmakerAvailable()) {
        std::string urlStr;
        LoadStringResource(urlStr, STR_MatchMakerURL);
        if (urlStr.length() == 0) {
            urlStr = DEFAULT_MATCHMAKER_URL;
        }
        // Create a match maker client
        mMatchMaker = new MatchMakerClient();
        // Set the URL of the match maker
        try {
            mMatchMaker->setMatchMakerURL(urlStr.c_str());
        }
        catch(...) {
        }
        char regCode[256];
        strncpy(regCode, Galactica::Globals::GetInstance().getRegistration(), MM_MAX_REG_CODE_LEN);
        regCode[MM_MAX_REG_CODE_LEN-1] = 0; // registration code can't be longer than 39 chars or MM will crash
        int err = mMatchMaker->requestGames(mMatchMakerGamesList, regCode); // creates the game list
        mNextMatchMakerQueryTime = 0;
        if (err == noErr) {
            // go through the list of games and add them to the browser list
            for (int i = 0; i<mMatchMakerGamesList->getNumGames(); i++) {
                MMGameRec* theGame = mMatchMakerGamesList->getGame(i);
                GameJoinInfo gji(theGame->mGameName, theGame->mSecondaryIP, "", "", theGame->mGameID, 0, 0, gameType_MatchMaker, std::atoi(theGame->mSecondaryPort));
                AddGameToKnownList(gji);
            }
        }
        delete mMatchMakerGamesList;
        mMatchMakerGamesList = NULL;
        delete mMatchMaker;
        mMatchMaker = NULL;
    } else {
        DEBUG_OUT("Client couldn't query matchmaker, no URL access available()", DEBUG_ERROR);
    }
  #endif // NO_MATCHMAKER_SUPPORT
}

void
GalacticaClient::LoadKnownGamesList() {
   // pre-loads the HostArrays with data about known games
   std::FILE* fp = std::fopen("Gamelist.txt", "r");
   if (fp == NULL) {
      return;
   }
   while (!std::feof(fp)) {
      char theName[1024];
      NMHostID theHostID = 0;  // games that had an ID were found by browsing, and can't have been saved
      UInt32 theKey;
      UInt32 theGameID;
      char c;
      char thePlayerName[1024];
      char thePassword[1024];
      char theDisplayName[1024];
      theName[0] = 0;
      thePassword[0] = 0;
      theDisplayName[0] = 0;
      UInt16 thePort = 0;
      if (std::fscanf(fp, "%[^\t]", theName) > 0) {
         std::fscanf(fp, "%ld", &theKey);
         std::fscanf(fp, "%c", &c);
         if (c == '\t') {
            std::fscanf(fp, "%ld\t%[^\n\r\t]", &theGameID, thePlayerName);
            std::fscanf(fp, "%c", &c);
            if (c == '\t') { // read saved password, only in 2.1b9r13 and later
               std::fscanf(fp, "%[^\n\r\t]",thePassword);
               std::fscanf(fp, "%c", &c);
            }
            if (c == '\t') { // read saved game display name, only in 2.3 and later
               std::fscanf(fp, "%[^\n\r\t]",theDisplayName);
               std::fscanf(fp, "%c", &c);
            }
            // 2.1b12, theKey is really the port
            if (theKey != theGameID) {   // in old Gamelist.txt files, theKey and theGameID are same
               thePort = theKey;
               theKey = theGameID;
            }
         } else {
            // handle gamelist.txt from 2.1b6
            strncpy(thePlayerName, Galactica::Globals::GetInstance().getDefaultPlayerName(), MAX_PLAYER_NAME_LEN);
            thePlayerName[MAX_PLAYER_NAME_LEN+1] = 0;
            theGameID = theKey;
         }
         if (theName[0] != 0) {
            DEBUG_OUT("Found Saved Game name [" << theDisplayName << ":" << theName << "] game id [" << (long)theGameID <<
                  "] key [" << theKey << "] player [" << thePlayerName << "] password ["
                  << thePassword << "] in gamelist.txt", DEBUG_USER | DEBUG_DETAIL);
            // determine the game type by whether there was a display name saved or not
            GameType theGameType = gameType_Internet; //(std::strlen(theDisplayName) > 0) ? gameType_MatchMaker : gameType_Internet;
            GameJoinInfo gji(theDisplayName, theName, thePlayerName, thePassword, theGameID, theHostID, theKey, theGameType, thePort);
            AddGameToKnownList(gji);
         }
      }
   }
   std::fclose(fp);
}

void
GalacticaClient::SaveKnownGamesList() {
    std::FILE* fp = std::fopen("Gamelist.txt", "w");
    if (fp == NULL) {
        return;
    }
    GameJoinInfo* infoP = NULL;
    mHostArray.Lock();
    for (int i = 1; i <= (int)mHostArray.GetCount(); i++) {
        infoP = mHostArray.FetchItemPtr(i);
        // only save games that weren't found by browsing
        if ((infoP->key != 0) && ((infoP->type == gameType_Internet) || (infoP->type == gameType_MatchMaker))) {
            std::fprintf(fp, "%s\t%d\t%ld\t%s\t%s\t%s\n", infoP->hostname, infoP->port, infoP->gameid, 
                        infoP->playername, infoP->password, infoP->gamename);
            DEBUG_OUT("Saved Game name [" << infoP->gamename << ":" << infoP->hostname << "] port [" << infoP->port 
                     << "] gameid [" << (long)infoP->gameid << "] key [" << infoP->key 
                     << "] player [" << infoP->playername << "] password ["
                     << infoP->password << "] into gamelist.txt", 
                     DEBUG_USER | DEBUG_DETAIL);
        }
    }
    mHostArray.Unlock();
    std::fclose(fp);
}

void
GalacticaClient::AddGameToKnownList(GameJoinInfo inGameJoinInfo) {
   GameJoinInfo* infoP = NULL;
   bool bDoAdd = true;
   mHostArray.Lock();
   for (int i = 1; i <= (int)mHostArray.GetCount(); i++) {
      infoP = mHostArray.FetchItemPtr(i);
      // we could be matching the Name & Host ID to set the rejoin key and/or game id
      if ( (std::strcmp(inGameJoinInfo.hostname, infoP->hostname) == 0)
       && (std::strcmp(inGameJoinInfo.gamename, infoP->gamename) == 0) 
       && (inGameJoinInfo.port == infoP->port)
       && (infoP->hostid == inGameJoinInfo.hostid) 
       && ( (infoP->type == inGameJoinInfo.type) 
            || ((infoP->type == gameType_Internet) && (inGameJoinInfo.type == gameType_MatchMaker)) 
          ) 
       ) {
         if (inGameJoinInfo.key != 0) {
            infoP->key = inGameJoinInfo.key;
         }
         if (inGameJoinInfo.gameid != 0) {
            infoP->gameid = inGameJoinInfo.gameid;
         }
         if (std::strlen(inGameJoinInfo.playername) > 0) {
            std::strcpy(infoP->playername, inGameJoinInfo.playername);
            std::strcpy(infoP->password, inGameJoinInfo.password);
         }
         bDoAdd = false;
         DEBUG_OUT("Found Game name [" << inGameJoinInfo.gamename << ":" << inGameJoinInfo.hostname << "] hostid [" << 
                 inGameJoinInfo.hostid << "] in list, updated key [" << infoP->key << "] gameid [" << 
                 (long)infoP->gameid << "] player [" <<  infoP->playername << "] password [" << infoP->password << "]", 
                 DEBUG_USER | DEBUG_DETAIL);
         break;
      }
   }
   mHostArray.Unlock();
   if (bDoAdd) {
      Str255 displayname;
      char s[256];
      if ( (std::strlen(inGameJoinInfo.gamename) > 0) && (std::strlen(inGameJoinInfo.hostname) > 0) ) {
         // this is for matchmaker games have both the game name and IP address
         std::sprintf(s, "%s (%s)", inGameJoinInfo.gamename, inGameJoinInfo.hostname);
         c2pstrcpy(displayname, s);  // matchmaker actually knows the game name, include the IP for reference
      } else if ( (inGameJoinInfo.type == gameType_Internet) && (inGameJoinInfo.port != 0) ) {
         // manually entered internet games just have an IP (or FQDN) and port, but no game name
         std::sprintf(s, "%s:%d", inGameJoinInfo.hostname, inGameJoinInfo.port);
         c2pstrcpy(displayname, s);
      } else if (inGameJoinInfo.type == gameType_LAN) {
         std::sprintf(s, "%s (LAN)", inGameJoinInfo.gamename);
         c2pstrcpy(displayname, s);
      } else {
         c2pstrcpy(displayname, inGameJoinInfo.hostname);
      }
      mHostNameArray.AddItem(displayname);
      mHostArray.AddItem(inGameJoinInfo);
      DEBUG_OUT("Added Game name [" << inGameJoinInfo.gamename << ":"  << inGameJoinInfo.hostname << "] gameid [" <<
             (long)inGameJoinInfo.gameid << "] hostid [" << inGameJoinInfo.hostid << "] key [" << inGameJoinInfo.key << 
             "] player [" << inGameJoinInfo.playername << "] password [" << inGameJoinInfo.password << "] to join list", 
             DEBUG_USER | DEBUG_DETAIL);
   }
}

void
GalacticaClient::RemoveGameFromKnownList(GameJoinInfo inGameJoinInfo) {
    GameJoinInfo* infoP = NULL;
    int removeIdx = -1;
    mHostArray.Lock();
    for (int i = 1; i <= (int)mHostArray.GetCount(); i++) {
        infoP = mHostArray.FetchItemPtr(i);
        // both Name & Host ID must match
        if ((std::strcmp(inGameJoinInfo.hostname, infoP->hostname) == 0)  && (inGameJoinInfo.port == infoP->port)
          && (infoP->hostid == inGameJoinInfo.hostid) && (infoP->type == inGameJoinInfo.type)) {
            removeIdx = i;
            break;
        }
    }
    mHostArray.Unlock();
    if (removeIdx != -1) {
        mHostNameArray.RemoveItemsAt(1, removeIdx);
        mHostArray.RemoveItemsAt(1, removeIdx);
        DEBUG_OUT("Removed Game name [" << inGameJoinInfo.hostname << "] hostid [" << inGameJoinInfo.hostid <<
                "] key [" << inGameJoinInfo.key << "] from join list", DEBUG_USER | DEBUG_DETAIL);
    } else {
        DEBUG_OUT("No match for game name [" << inGameJoinInfo.hostname << "] id [" << inGameJoinInfo.hostid <<
                "] key [" << inGameJoinInfo.key << "]. Can't remove from join list", DEBUG_USER | DEBUG_ERROR);
    }
}


// only call after ConnectToHost() or ConnectToNamedHost() has been called
GameJoinInfo&   
GalacticaClient::GetConnectedHostJoinInfo() {
    return mConnectedHostInfo;
}

