//  GalacticaHost.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include "pdg/sys/log.h"

#include "GalacticaConsts.h"

#include "GenericUtils.h"

#include "GalacticaGlobals.h"
#include "Galactica.h"
#include "CStar.h"
#include "GalacticaUtils.h"
#include "GalacticaHost.h"

#include "CVarDataFile.h"
#include "CMasterIndexFile.h"
#include "CMessage.h"

#include "OpenPlay.h"

#if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
#include "FullPath.h"
#endif

#ifndef NO_GAME_SMITH_SUPPORT
    #include "GameSmithAPI.h"
#endif // NO_GAME_SMITH_SUPPORT

#ifndef NO_GAME_RANGER_SUPPORT
  #define __NETSPROCKET__ // so GameRanger.h doesn't include it
  #include "GameRanger.h"
#endif // not defined NO_GAME_RANGER_SUPPORT

#ifndef NO_MATCHMAKER_SUPPORT
  #include "MatchMakerClient.h"
#endif

#ifndef GALACTICA_SERVER
  #include "UModalDialogs.h"
#endif

#include <string>
#include <stdio.h>
#include <string.h>
#include <ctime>

#define MAX_STAR_NAMES 63

//#define PROFILE_EOT

#define AS_NUMBER   pdg::log::number

GalacticaHost::GalacticaHost()
: GalacticaProcessor(), 
  #ifndef NO_MATCHMAKER_SUPPORT
    mMatchMaker(NULL), mMatchMakerGame(NULL),
  #endif
  mHostLog()
{
   ASSERT(MAX_CONNECTIONS > MAX_PLAYERS_CONNECTED);
   DEBUG_OUT("New GalacticaHost", DEBUG_IMPORTANT);
   mGameDataFile = NULL;
   mGameIndexFile = NULL;
   mQuitPending = false;
   mEndpoint = NULL;
   mStatusEndpoint = NULL;
   mAnyoneLoggedOn = false;
   // initialize the connected player endpoints
   for (int i = 0; i < MAX_PLAYERS_CONNECTED; i++) {
     mPlayerEndpoints[i] = NULL; // nobody logged on here
   }
   for (int i = 0; i < MAX_OBSERVERS_CONNECTED; i++) {
     mObservers[i] = NULL; // nobody logged on here
   }
    // clear list of reg codes in use
    for (int i = 0; i < MAX_PLAYERS_CONNECTED; i++) {
        mConnectedRegCodes[i][0] = 0;
    }
}

GalacticaHost::~GalacticaHost() {
   DEBUG_OUT("~GalacticaHost "<<(void*)this, DEBUG_IMPORTANT);
   StopIdling();
   StopRepeating();
   ShutdownServer();
   mAutoEndTurnTimer.Pause();
   mClosing = true;
   try {
      SaveAllPlayerInfo();
      SaveGameInfo(false);   // let the game know that the host has closed
   }
   catch (std::exception& e) {
      DEBUG_OUT("Exception "<<e.what()<<" trying to mark host closed", DEBUG_ERROR);
   }
   try {
      CloseHostFiles();
   }
   catch (std::exception& e) {
      DEBUG_OUT("Exception "<<e.what()<<" trying to close host files", DEBUG_ERROR);
   }
  #ifndef NO_MATCHMAKER_SUPPORT
   if (mMatchMakerGame) {
       delete mMatchMakerGame;
       mMatchMakerGame = NULL;
   }
   if (mMatchMaker) {
       delete mMatchMaker;
       mMatchMaker = NULL;
   }
  #endif // NO_MATCHMAKER_SUPPORT
}

void
GalacticaHost::InitializeServer(const char* inHostName, int inPort) {

   GalacticaApp::ReadHostConfigFile(); // in case it has changed

   char nameStr[256];
   p2cstrcpy(nameStr, mGameInfo.gameTitle);

   // setup the host log file
   std::string logfilename(nameStr);
   logfilename += "-host";
   pdg::LogManager* theLogMgr = pdg::createLogManager();
   mHostLog.setLogManager(theLogMgr);
   theLogMgr->initialize(logfilename.c_str(),  pdg::LogManager::init_AppendToExisting);
   theLogMgr->setLogLevel(pdg::log::verbose);

   std::string theIPAddrStr;
   std::string thePortNumStr;
   NMType moduleType = kIPModuleType;  // assume TCP/IP
   PConfigRef config;
   char config_string[1024];

   const char* hostname = inHostName;
   if (inHostName == NULL) {
      hostname = Galactica::Globals::GetInstance().getDefaultHostname();
   }
   if (inPort == 0) {
      inPort = Galactica::Globals::GetInstance().getDefaultPort();
   }
   std::sprintf(config_string, "IPaddr=%s\tIPport=%d", hostname, inPort);
   
   // set the client id so we can detect hotseat games
   Galactica::Globals::GetInstance().setClientID(std::rand() & (LoginRequestPacket::JoinInfo_ClientIDMask >> LoginRequestPacket::JoinInfo_ClientIDShft));
   
   NMErr err = ProtocolCreateConfig(moduleType, type_Creator, nameStr, NULL, 0, config_string, &config);
   if (err != noErr) {
      DEBUG_OUT("OpenPlay ProtocolCreateConfig() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
     #ifdef GALACTICA_SERVER
      mHostLog << pdg::log::category("HOST") << pdg::log::fatal
         << "OpenPlay CreateConfig ["<<config_string<<"] failed with error ["<< (AS_NUMBER)err <<"]"
         << pdg::endlog;
      Throw_(err);
     #endif
   } else {
      ProtocolGetConfigString(config, config_string, 1024);
      DEBUG_OUT("Host about to open config ["<<config_string<<"]", DEBUG_DETAIL | DEBUG_PACKETS);
      err = ProtocolOpenEndpoint(config, HostCallback, this, &mEndpoint, kOpenNone);
      if (err != noErr) {
         DEBUG_OUT("OpenPlay ProtocolOpenEndpoint() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        #ifdef GALACTICA_SERVER
         mHostLog << pdg::log::category("HOST") << pdg::log::fatal
            << "OpenPlay OpenEndpoint ["<<config_string<<"] failed with error ["<< (AS_NUMBER)err <<"]"
            << pdg::endlog;
         Throw_(err);
        #endif
      } else {
         // we need to advertise so others can see our game
         ProtocolStartAdvertising(mEndpoint);
         DEBUG_OUT("Successfully created Game network host ["<<nameStr<<"] "
                   <<(void*)this, DEBUG_IMPORTANT);
      }
      err = ProtocolDisposeConfig(config);
      if (err != noErr) {
         DEBUG_OUT("OpenPlay ProtocolDisposeConfig() returned "<<err, DEBUG_ERROR);
      }
      // look for IPaddr and IPport in the config string, separated by tabs
      // e.g.:  IPaddr=207.126.114.188   IPport=4429
      char* token;
      token = std::strtok(config_string, "\t");
      while(token != NULL) {
         if (std::strncmp("IPaddr=", token, 7) == 0) {
            theIPAddrStr = &token[7];   // copy in the IP address
         } else if (std::strncmp("IPport=", token, 7) == 0) {
            thePortNumStr = &token[7];  // copy in the port number
         }
         token = std::strtok(NULL, "\t");
      }
      // get the endpoint identifier (remote IP address) from this endpoint
       std::strcpy(mHostIPStr, "0.0.0.0");
       NMErr err = ProtocolGetEndpointIdentifier(mEndpoint, mHostIPStr, 256);
       if (err != kNMNoError) {
            std::strncpy(mHostIPStr, theIPAddrStr.c_str(), 255);
       }
      mHostLog << pdg::log::category("HOST") << pdg::log::inform
         << "Opened ["<<nameStr<<"] on ["<< mHostIPStr << ":"<<thePortNumStr<<"]"
         << pdg::endlog;
      // status monitoring port
      int statusPort = Galactica::Globals::GetInstance().getStatusPort();
      if (statusPort != 0) {
         PConfigRef statusConfig;
         const char* statusHost;
         statusHost = Galactica::Globals::GetInstance().getStatusHostname();
         std::sprintf(config_string, "netSprocket=true\tIPaddr=%s\tIPport=%d", statusHost, statusPort);
         
         NMErr err = ProtocolCreateConfig(kIPModuleType, type_Creator, nameStr, NULL, 0, config_string, &statusConfig);
         if (err != noErr) {
            DEBUG_OUT("StatusPort: OpenPlay ProtocolCreateConfig() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
         } else {
            ProtocolGetConfigString(statusConfig, config_string, 1024);
            DEBUG_OUT("StatusPort: Host about to open config ["<<config_string<<"]", DEBUG_DETAIL | DEBUG_PACKETS);
            err = ProtocolOpenEndpoint(statusConfig, HostStatusCallback, this, &mStatusEndpoint, kOpenNone);
            if (err != noErr) {
               DEBUG_OUT("StatusPort: OpenPlay ProtocolOpenEndpoint() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
            } else {
               DEBUG_OUT("StatusPort: Successfully created listener " <<(void*)this, DEBUG_IMPORTANT);
            }
            err = ProtocolDisposeConfig(statusConfig);
            if (err != noErr) {
               DEBUG_OUT("StatusPort: OpenPlay ProtocolDisposeConfig() returned "<<err, DEBUG_ERROR);
            }
            // look for IPaddr and IPport in the config string, separated by tabs
            // e.g.:  IPaddr=207.126.114.188   IPport=4429
            char* token;
            std::string statusIPAddrStr;
            std::string statusPortNumStr;
            token = std::strtok(config_string, "\t");
            while(token != NULL) {
               if (std::strncmp("IPaddr=", token, 7) == 0) {
                  statusIPAddrStr = &token[7];   // copy in the IP address
               } else if (std::strncmp("IPport=", token, 7) == 0) {
                  statusPortNumStr = &token[7];  // copy in the port number
               }
               token = std::strtok(NULL, "\t");
            }
            mHostLog << pdg::log::category("HOST") << pdg::log::inform
               << "Opened Status Port ["<<nameStr<<"] on ["<< statusIPAddrStr << ":"<<statusPortNumStr<<"]"
               << pdg::endlog;
         }
      }
   }
   StartRepeating();  // make sure we are handling packets
  #ifndef NO_GAME_RANGER_SUPPORT
   GRHostReady();    // let GameRanger know that we started hosting the game
  #endif
  #ifndef NO_GAME_SMITH_SUPPORT
    GalacticaApp* app = static_cast<GalacticaApp*>(LCommander::GetTopCommander());
    app->InitializeGameSmith();
    while (kGS_ConnectBadUserID == GS_ServerConnect("", "", true, true)) {}
  #endif
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
      mMatchMakerGame = new MMGameRec();
      p2cstrcpy(mMatchMakerGame->mGameName, mGameInfo.gameTitle);
      GetInternalStatusString(mMatchMakerGame->mStatus, MM_MAX_STATUS_LEN);
      std::strncpy(mMatchMakerGame->mPrimaryIP, theIPAddrStr.c_str(), MM_MAX_IP_LEN);
      mMatchMakerGame->mPrimaryIP[MM_MAX_IP_LEN-1] = 0;
      std::strncpy(mMatchMakerGame->mPrimaryPort, thePortNumStr.c_str(), MM_MAX_PORT_LEN);
      mMatchMakerGame->mPrimaryPort[MM_MAX_PORT_LEN-1] = 0;
      std::strncpy(mMatchMakerGame->mSecondaryIP, theIPAddrStr.c_str(), MM_MAX_IP_LEN);
      mMatchMakerGame->mSecondaryIP[MM_MAX_IP_LEN-1] = 0;
      std::strncpy(mMatchMakerGame->mSecondaryPort, thePortNumStr.c_str(), MM_MAX_PORT_LEN);
      mMatchMakerGame->mSecondaryPort[MM_MAX_PORT_LEN-1] = 0;
      mMatchMakerGame->mPlayersMax = mGameInfo.numHumans;
      mMatchMakerGame->mPlayersCurrent = mGameInfo.numHumansRemaining;
      mMatchMakerGame->mComputers = mGameInfo.numComputers;
      mMatchMakerGame->mComputerSkill = mGameInfo.compSkill;
      mMatchMakerGame->mComputerAdvantage = mGameInfo.compAdvantage;
      mMatchMakerGame->mCurrentTurn = mGameInfo.currTurn;
      mMatchMakerGame->mSize = mGameInfo.sectorSize;
      mMatchMakerGame->mDensity = mGameInfo.sectorDensity;
      mMatchMakerGame->mFast = mGameInfo.fastGame;
      mMatchMakerGame->mFogOfWar = !mGameInfo.omniscient;
      mMatchMakerGame->mTimeLimitSecs = mGameInfo.maxTimePerTurn;
      mMatchMakerGame->mGameID = mGameInfo.gameID;
      char regCode[256];
      strncpy(regCode, Galactica::Globals::GetInstance().getRegistration(), MM_MAX_REG_CODE_LEN);
      regCode[MM_MAX_REG_CODE_LEN-1] = 0; // registration code can't be longer than 39 chars or MM will crash
      long err = mMatchMaker->postGame(mMatchMakerGame, regCode);
      if (err != noErr) {
         // error means system is unavailable, avoid frequent timeout delays by stopping reporting
         DEBUG_OUT("Host::InitializeServer Failed to post game to MatchMaker at ["<<urlStr.c_str()<<"]", DEBUG_ERROR);
         delete mMatchMakerGame;
         mMatchMakerGame = NULL;
         delete mMatchMaker;
         mMatchMaker = NULL;
      }
      mNextMatchMakerPingTime = 0; // ping MatchMaker real soon
   } else {
     DEBUG_OUT("Host couldn't post to matchmaker, no URL access available()", DEBUG_ERROR);
   }
  #endif
}

void
GalacticaHost::GetInternalStatusString(char* buffer, size_t bufferSize) {
    // figure out status to report
    switch (mGameInfo.gameState & 0x7) {
        case state_NormalPlay:
            if (mGameInfo.gameState & state_FullFlag) {
                std::strncpy(buffer, "full", bufferSize);
            } else {
                std::strncpy(buffer, "open", bufferSize);
            }
            break;
        case state_GameOver:
            std::strncpy(buffer, "over", bufferSize);
            break;
        case state_PostGame:
            std::strncpy(buffer, "post", bufferSize);
            break;
        default:
            std::strncpy(buffer, "", bufferSize);
            break;
    }
    buffer[bufferSize-1] = 0;
}

void
GalacticaHost::ShutdownServer() {
   OSErr err;
   // first stop listening for new connections
   if (mEndpoint) {
      DEBUG_OUT("ShutdownServer is shutting down network host", DEBUG_IMPORTANT | DEBUG_PACKETS);
      ProtocolStopAdvertising(mEndpoint);
      err = ProtocolCloseEndpoint(mEndpoint, true); // do an orderly shutdown of connection
      if (err != noErr) {
         DEBUG_OUT("OpenPlay ProtocolCloseEndpoint() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
      }
      mEndpoint = NULL;
      mHostLog << pdg::log::category("HOST") << pdg::log::inform
         << "Shutdown host" << pdg::endlog;
   }
   if (mStatusEndpoint) {
      DEBUG_OUT("ShutdownServer is shutting down status port", DEBUG_IMPORTANT | DEBUG_PACKETS);
      err = ProtocolCloseEndpoint(mStatusEndpoint, true); // do an orderly shutdown of connection
      if (err != noErr) {
         DEBUG_OUT("Status: OpenPlay ProtocolCloseEndpoint() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
      }
      mEndpoint = NULL;
      mHostLog << pdg::log::category("HOST") << pdg::log::inform
         << "Shutdown status port" << pdg::endlog;
   }
    // then disconnect all the current users
    if (AdministratorIsConnected()) {
        DisconnectPlayer(kAdminPlayerNum, reason_HostShuttingDown);
    }
    for (int i = 1; i <= MAX_PLAYERS_CONNECTED; i++) {
        DisconnectPlayer(i, reason_HostShuttingDown);
    }
  #ifndef NO_GAME_RANGER_SUPPORT
    GRHostClosed();    // let GameRanger know that we stopped hosting the game
  #endif    
}

bool
GalacticaHost::LoadGameInfo() {
   mGameInfo.recHeader.recID = GAME_INFO_REC_ID;   // game info always at record id # 1
   mGameInfo.recHeader.recSize = sizeof(GameInfoT);
   mGameDataFile->ReadRecord((DatabaseRecPtr)&mGameInfo);   // read the current info
   if (mGameInfo.recHeader.recSize == sizeof(GameInfoT_v12)) {  // legacy Game Info Record
       if (mGameInfo.maxTimePerTurn >= 86400) {    // one day in seconds
           // for very long turn limits, assume they don't want to end turn early
           mGameInfo.endTurnEarly = false;
       } else {
           mGameInfo.endTurnEarly = true;
       }
        mGameInfo.timeZone = get_timezone();
   }
  #if PLATFORM_LITTLE_ENDIAN
    mGameInfo.currTurn            = BigEndian32_ToNative(mGameInfo.currTurn);
    mGameInfo.maxTimePerTurn      = BigEndian32_ToNative(mGameInfo.maxTimePerTurn);
    mGameInfo.secsRemainingInTurn = BigEndian32_ToNative(mGameInfo.secsRemainingInTurn);
    mGameInfo.totalNumPlayers     = BigEndian16_ToNative(mGameInfo.totalNumPlayers);
    mGameInfo.numPlayersRemaining = BigEndian16_ToNative(mGameInfo.numPlayersRemaining);
    mGameInfo.sequenceNumber      = BigEndian32_ToNative(mGameInfo.sequenceNumber);
    mGameInfo.compSkill           = BigEndian16_ToNative(mGameInfo.compSkill);
    mGameInfo.compAdvantage       = BigEndian16_ToNative(mGameInfo.compAdvantage);
    mGameInfo.sectorDensity       = BigEndian16_ToNative(mGameInfo.sectorDensity);
    mGameInfo.sectorSize          = BigEndian16_ToNative(mGameInfo.sectorSize);
  #endif // PLATFORM_LITTLE_ENDIAN
    LogGameInfo();
// check to make sure game info hasn't been corrupted
   if (mGameInfo.totalNumPlayers > 198)   // еее TEMP, current limit in version 1
      Throw_(dbDataCorrupt);
   if (mGameInfo.totalNumPlayers < 0)
      Throw_(dbDataCorrupt);
   if (mGameInfo.totalNumPlayers < mGameInfo.numPlayersRemaining)
      Throw_(dbDataCorrupt);
   if (mGameInfo.currTurn < 0)
      Throw_(dbDataCorrupt);
   if (mGameInfo.totalNumPlayers != (mGameInfo.numHumans + mGameInfo.numComputers))
      Throw_(dbDataCorrupt);
   if (mGameInfo.numPlayersRemaining != (mGameInfo.numHumansRemaining + mGameInfo.numComputersRemaining))
      Throw_(dbDataCorrupt);
   if (mGameInfo.maxTimePerTurn) {
       if (mGameInfo.currTurn == 1) {
           DEBUG_OUT("Still on initial turn, skipping setting timer", DEBUG_DETAIL | DEBUG_EOT);
       } else {
          // TODO: this could be more sophisticated, it pauses whenever the host goes down
          mAutoEndTurnTimer.SetTimeRemaining(mGameInfo.secsRemainingInTurn);
          DEBUG_OUT("Set turn timer to end in  " << mGameInfo.secsRemainingInTurn << " secs", DEBUG_DETAIL | DEBUG_EOT);
          std::time_t currDT;         // get current date/time
          currDT = std::time(&currDT); //::GetDateTime(&currDT);
          mGameInfo.nextTurnEndsAt = (unsigned long) currDT + (unsigned long)mGameInfo.secsRemainingInTurn;
          DEBUG_OUT("Next turn ends at  " << mGameInfo.nextTurnEndsAt, DEBUG_DETAIL | DEBUG_EOT);
       }
   } else {
      mAutoEndTurnTimer.Clear();   // make timer read blank
   }
   return mGameInfo.hasHost;
}

void
GalacticaHost::SaveGameInfo(bool hasHost) {
    if (mGameDataFile != NULL) {
       mGameInfo.recHeader.recSize = sizeof(GameInfoT);
       mGameInfo.hasHost = hasHost;
       mGameInfo.sequenceNumber = mGameDataFile->GetLastRecordID();
        // need to save the end time, since it is a union with secs remaining.
        UInt32 endTime = mGameInfo.nextTurnEndsAt;
        // if this is a timed game be sure that the amount of time left in the turn is saved correctly
        if (mGameInfo.maxTimePerTurn) {
            mGameInfo.secsRemainingInTurn = mAutoEndTurnTimer.GetTimeRemaining();
           DEBUG_OUT("Saving turn timer with  " << mGameInfo.secsRemainingInTurn << " secs left", DEBUG_DETAIL | DEBUG_EOT);
        } else {
            mGameInfo.secsRemainingInTurn = 0;
        }
      #if PLATFORM_LITTLE_ENDIAN
        GameInfoT saveInfo = mGameInfo;
        mGameInfo.currTurn            = Native_ToBigEndian32(mGameInfo.currTurn);
        mGameInfo.maxTimePerTurn      = Native_ToBigEndian32(mGameInfo.maxTimePerTurn);
        mGameInfo.secsRemainingInTurn = Native_ToBigEndian32(mGameInfo.secsRemainingInTurn);
        mGameInfo.totalNumPlayers     = Native_ToBigEndian16(mGameInfo.totalNumPlayers);
        mGameInfo.numPlayersRemaining = Native_ToBigEndian16(mGameInfo.numPlayersRemaining);
        mGameInfo.sequenceNumber      = Native_ToBigEndian32(mGameInfo.sequenceNumber);
        mGameInfo.compSkill           = Native_ToBigEndian16(mGameInfo.compSkill);
        mGameInfo.compAdvantage       = Native_ToBigEndian16(mGameInfo.compAdvantage);
        mGameInfo.sectorDensity       = Native_ToBigEndian16(mGameInfo.sectorDensity);
        mGameInfo.sectorSize          = Native_ToBigEndian16(mGameInfo.sectorSize);
      #endif // PLATFORM_LITTLE_ENDIAN
       if (mGameDataFile->GetRecordCount() == 0) {   // creating a new database file
          mGameInfo.recHeader.recID = 0;
          ThrowIfNot_( mGameDataFile->AddRecord((DatabaseRecPtr)&mGameInfo) == GAME_INFO_REC_ID );   // add rec to DB
       } else {
          mGameInfo.recHeader.recID = GAME_INFO_REC_ID;   // game info always stored at ID 1
          mGameDataFile->UpdateRecord((DatabaseRecPtr)&mGameInfo);   // write the changes
       }
      #if PLATFORM_LITTLE_ENDIAN
        mGameInfo = saveInfo;
      #endif // PLATFORM_LITTLE_ENDIAN
        // restore the time at which the next turn ends
        if (mGameInfo.maxTimePerTurn) {
            mGameInfo.nextTurnEndsAt = endTime;
           DEBUG_OUT("Next turn ends at  " << mGameInfo.nextTurnEndsAt, DEBUG_DETAIL | DEBUG_EOT);
        }
        LogGameInfo();
    }
}

void
GalacticaHost::UpdateMaxTimePerTurn(UInt32 inNewMaxTimePerTurn) {
   if (mGameInfo.maxTimePerTurn != (long)inNewMaxTimePerTurn) {  // did we change the timer ?
      if (inNewMaxTimePerTurn == 0) {   // we stopped the timer
         mAutoEndTurnTimer.Clear();   // make timer read blank
      } else if (mGameInfo.maxTimePerTurn == 0) {  // there was no timer before
         mAutoEndTurnTimer.SetTimeRemaining(inNewMaxTimePerTurn);
         mAutoEndTurnTimer.Resume();
      }
   }
   mGameInfo.maxTimePerTurn = inNewMaxTimePerTurn;
}

void
GalacticaHost::LoadAllPlayerInfo() {
   PublicPlayerInfoT thePlayerInfo;
   PrivatePlayerInfoT thePrivateInfo;
   size_t recSize = sizeof(PublicPlayerInfoT) + sizeof(PrivatePlayerInfoT) + sizeof(DatabaseRec);
   DatabaseRec* tempRecP = (DatabaseRec*) std::malloc(recSize);   
   for (int i = 1; i <= mGameInfo.totalNumPlayers; i++) { // reserve records for player info
   	  std::memset(tempRecP, 0, recSize);  // clear the block
      tempRecP->recID = PUBLIC_REC_ID(i);
      tempRecP->recSize = sizeof(PublicPlayerInfoT)+sizeof(DatabaseRec);
      mGameDataFile->ReadRecord(tempRecP); // reserve blank player info slot
      std::memcpy(&thePlayerInfo, tempRecP->recData, sizeof(PublicPlayerInfoT));
   	  std::memset(tempRecP, 0, recSize);  // clear the block
      tempRecP->recID = PRIVATE_REC_ID(i);
      tempRecP->recSize = sizeof(PrivatePlayerInfoT)+sizeof(DatabaseRec);
      mGameDataFile->ReadRecord(tempRecP); // reserve blank player info slot
      std::memcpy(&thePrivateInfo, tempRecP->recData, sizeof(PrivatePlayerInfoT));
     #if PLATFORM_LITTLE_ENDIAN
      thePrivateInfo.lastTurnPosted     = BigEndian32_ToNative(thePrivateInfo.lastTurnPosted);
      thePrivateInfo.numPiecesRemaining = BigEndian32_ToNative(thePrivateInfo.numPiecesRemaining);
      thePrivateInfo.homeID             = BigEndian32_ToNative(thePrivateInfo.homeID);
      thePrivateInfo.techStarID         = BigEndian32_ToNative(thePrivateInfo.techStarID);
      thePrivateInfo.hiStarTech         = BigEndian16_ToNative(thePrivateInfo.hiStarTech);
      thePrivateInfo.hiShipTech         = BigEndian16_ToNative(thePrivateInfo.hiShipTech);
     #endif // PLATFORM_LITTLE_ENDIAN
      // set defaults for values that actually come from ClientSettingsPacket
      thePrivateInfo.defaultBuildType = 1;
      thePrivateInfo.defaultGrowth = 334;
      thePrivateInfo.defaultTech = 333;
      thePrivateInfo.defaultShips = 333;
      DEBUG_OUT("Read record set for player " << i , DEBUG_DETAIL | DEBUG_DATABASE);
      SetPlayerInfo(i, thePlayerInfo);
      SetPrivatePlayerInfo(i, thePrivateInfo);
   }
   std::free(tempRecP);
}

void
GalacticaHost::SaveAllPlayerInfo() {
   PublicPlayerInfoT thePlayerInfo;
   PrivatePlayerInfoT thePrivateInfo;
   DatabaseRec* tempRecP = (DatabaseRec*) std::malloc(sizeof(PublicPlayerInfoT) + sizeof(PrivatePlayerInfoT) + sizeof(DatabaseRec));   
   for (int i = 1; i <= mGameInfo.totalNumPlayers; i++) { // reserve records for player info
      GetPlayerInfo(i, thePlayerInfo);
      GetPrivatePlayerInfo(i, thePrivateInfo);
      #if PLATFORM_LITTLE_ENDIAN
      thePrivateInfo.lastTurnPosted     = Native_ToBigEndian32(thePrivateInfo.lastTurnPosted);
      thePrivateInfo.numPiecesRemaining = Native_ToBigEndian32(thePrivateInfo.numPiecesRemaining);
      thePrivateInfo.homeID             = Native_ToBigEndian32(thePrivateInfo.homeID);
      thePrivateInfo.techStarID         = Native_ToBigEndian32(thePrivateInfo.techStarID);
      thePrivateInfo.hiStarTech         = Native_ToBigEndian16(thePrivateInfo.hiStarTech);
      thePrivateInfo.hiShipTech         = Native_ToBigEndian16(thePrivateInfo.hiShipTech);
      #endif // PLATFORM_LITTLE_ENDIAN
      tempRecP->recID = PUBLIC_REC_ID(i);
      tempRecP->recSize = sizeof(PublicPlayerInfoT)+sizeof(DatabaseRec);
      std::memcpy(tempRecP->recData, &thePlayerInfo, sizeof(PublicPlayerInfoT));
      mGameDataFile->UpdateRecord(tempRecP); // update player info
      tempRecP->recID = PRIVATE_REC_ID(i);
      tempRecP->recSize = sizeof(PrivatePlayerInfoT)+sizeof(DatabaseRec);
      std::memcpy(tempRecP->recData, &thePrivateInfo, sizeof(PrivatePlayerInfoT));
      mGameDataFile->UpdateRecord(tempRecP); // update private player info
      DEBUG_OUT("Wrote record set for player " << i , DEBUG_DETAIL | DEBUG_DATABASE);
   }
   std::free(tempRecP);
}

void
GalacticaHost::BackupDatabaseFiles() {
    char appendStr[255];
    snprintf(appendStr, 255, " #%ld", mGameInfo.currTurn);
    mGameDataFile->Backup(appendStr);
}

bool
GalacticaHost::IsTurnComplete() {
   DEBUG_OUT("Host trying to check turn completion", DEBUG_TRIVIA);
   if (GetNumHumans() == 0) {    // in a computer only game, we just end turns constantly until only one computer is left
      if (mGameInfo.numComputersRemaining > 1) {  // there is more that 1 computer left in the game
         DEBUG_OUT("Robot game with robots remaining, turn #"<<mGameInfo.currTurn<<" complete", DEBUG_IMPORTANT);
         return true;    // the turn is complete
      }
   } else if (mGameInfo.numHumansRemaining > 0) { // if there are humans left in the game
// ignore logged in state completely. All players must post for end of turn to happen
// unless there was a time limit which has expired
/*      if (mGameInfo.endTurnEarly && !mAnyoneLoggedOn) {
         DEBUG_OUT("Nobody logged in, turn "<<mGameInfo.currTurn<<" cannot be over", DEBUG_TRIVIA);
         return false;
      } */
      if (mGameInfo.secsRemainingInTurn) { // check the timer first
         std::time_t currDT;         // get current date/time
         currDT = std::time(&currDT); //::GetDateTime(&currDT);
         bool timePassed = (unsigned long) currDT > (unsigned long) mGameInfo.nextTurnEndsAt;
         if (timePassed) {   // compare it with date/time for end of turn
            DEBUG_OUT("Time limit reached for Turn #"<<mGameInfo.currTurn, DEBUG_IMPORTANT);
            return true;      // if that time has passed, we have a completed turn,
         } else if (!mGameInfo.endTurnEarly) {
            unsigned long secsRemaining = mGameInfo.nextTurnEndsAt - currDT;
            DEBUG_OUT("Turn "<<mGameInfo.currTurn<<" not over, ends in " << secsRemaining << " seconds", 
                             DEBUG_TRIVIA);
            if (secsRemaining > (unsigned long)mGameInfo.maxTimePerTurn) {
                   // fix a busted clock
               mGameInfo.nextTurnEndsAt = (unsigned long)currDT + (unsigned long)mGameInfo.maxTimePerTurn;
               DEBUG_OUT("Turn timer is wrong. Resetting to "<< mGameInfo.maxTimePerTurn<<" seconds.",
                        DEBUG_ERROR);
            }
            return false;
         }
      }
      if (mGameInfo.gameState == state_NormalPlay) {
         PublicPlayerInfoT pubInfo;
         PrivatePlayerInfoT info;
         for (int i = 1; i <= mGameInfo.totalNumPlayers; i++) {
            GetPlayerInfo(i, pubInfo);
            if (pubInfo.isComputer) {   // computers don't post EOT
               continue;
            }
            if (!pubInfo.isAssigned) {
               DEBUG_OUT("Found unassigned human player "<<i<<" turn "<<mGameInfo.currTurn
                              <<" cannot be over", DEBUG_TRIVIA);
               return false;   // haven't even filled the human's slot yet, turn can't be over
            }
            if (!pubInfo.isAlive) {
               continue; // we don't care if dead humans post the turn or not
            }
            // ignore logged in state completely, all players must post for end of turn to happen
            // unless there was a time limit which has expired
/*          if (!pubInfo.isLoggedIn) {
               continue;   // if a humans isn't logged in, they don't get to post (unless no-one is logged in)
            } */
            GetPrivatePlayerInfo(i, info);
            ASSERT(!(info.lastTurnPosted > mGameInfo.currTurn));
            ThrowIf_(info.lastTurnPosted > mGameInfo.currTurn);
            if (info.lastTurnPosted < mGameInfo.currTurn)  {
               DEBUG_OUT("Turn #"<<mGameInfo.currTurn<<" not complete, waiting for player "<<i, 
                        DEBUG_IMPORTANT);
               return false;
            }         // so turn cannot be completed yet
         }
         return true;
      }
   } else if (mAnyoneLoggedOn && (mGameInfo.numComputersRemaining > 1) ) {    // no humans remaining
       return true;    // but someone is logged in watching computers battle it out
   }
   return false;
}

void
GalacticaHost::SpendTime(const EventRecord& ) { //inMacEvent
   if (mDoingWork) {      // v2.0.4, we are in the middle of processing the EOT.
      return;
   }
   // ====================== packet processing ===============================
   // pullpackets off the queue and process them
   for (int i = 0; i < MAX_CONNECTIONS; i++) {
        while ( mConnections[i].HasPacket() ) {
            DEBUG_OUT("Host SpendTime() processing one packet on connection "<<i, DEBUG_TRIVIA | DEBUG_PACKETS);
            Packet* p = mConnections[i].GetNextPacket();
            if (p == NULL) {
                DEBUG_OUT("Connection "<<i<<" reported HasPacket() == true, but GetNextPacket() returned NULL", DEBUG_ERROR | DEBUG_PACKETS);
                break;  // out of the while loop
            }
            HandlePacketFrom(mConnections[i].GetEndpointRef(), p);
            ReleasePacket(p);
        }
    }

   bool movesDone = true;
   if (    mComputerMovesRemainingToProcess 
        && (mGameInfo.gameState == state_NormalPlay) 
        && (mGameInfo.numComputersRemaining > 0)) {
          ProcessOneComputerMove();   // do a single computer move
   }
   if ( GetNumHumans() == 0) {
      mNextSpendTime = 0;      // robot only game, don't wait to check for EOT done
      movesDone = false;      // we will return true below, but all moves won't be done
   }
   std::time_t currDT;
   UInt32 tickCount = std::time(&currDT);
   if ( mNextSpendTime < tickCount ) {      // at specified interval:
      if (IsTurnComplete() == true) {         //  check for all players complete
         try {
              StopRepeating();    // wait until next time we get a TurnCompletePacket before we resume checking
             if (movesDone) {
                DoEndTurn();   // all computer moves are complete, just end the turn
             } else {
                FinishTurn();   // finish comp moves, then end turn
             }
         }
         catch (LException& e) {
             DEBUG_OUT("SpendTime() caught PP exception "<<e.what()<<" during turn processing", DEBUG_ERROR);
             if ( (e.GetErrorCode() == dbIndexCorrupt) || (e.GetErrorCode() == dbDataCorrupt) ) {
                 DEBUG_OUT("Exception means Database Corruption Detected!!", DEBUG_ERROR);
               #ifndef GALACTICA_SERVER
                 UModalAlerts::StopAlert(alert_FileDamaged);
               #else
                 DEBUG_OUT("Rethrowing exception to application.", DEBUG_ERROR);
                 throw e;
               #endif // GALACTICA_SERVER
             }
         }
         catch (std::exception& e) {
             DEBUG_OUT("SpendTime() caught PP exception "<<e.what()<<" during turn processing", DEBUG_ERROR);
         }
         catch(...) {
             DEBUG_OUT("SpendTime() caught unknown exception during turn processing", DEBUG_ERROR);
         }
         StartRepeating();   // make sure we start checking again
      }
      mNextSpendTime = (UInt32) std::time(&currDT) + kCheckGameStateInterval;
   }
  #ifndef NO_MATCHMAKER_SUPPORT
    // ping the MatchMaker to tell it we are still alive at a regular interval
    if (mMatchMakerGame && (mNextMatchMakerPingTime < tickCount)) {
        GetInternalStatusString(mMatchMakerGame->mStatus, MM_MAX_STATUS_LEN);
        long err = mMatchMaker->ping(mMatchMakerGame);
        if ( err != noErr ) {
            DEBUG_OUT("Host::SpendTime: MatchMaker Ping failed with error "<<err<<" -- posting game again", DEBUG_ERROR);
            // failure to ping may mean we took too long since last ping, try to recreate
            err = mMatchMaker->postGame(mMatchMakerGame, mMatchMakerGame->mRegCode);
            if (err != noErr) {
                // error means system is unavailable, avoid frequent timeout delays by stopping reporting
              #ifdef GALACTICA_SERVER
                DEBUG_OUT("Host::SpendTime: MatchMaker Post failed with error "<<err<<" -- delay ping for 1 hour", DEBUG_ERROR);
                // for a standalone server, it's critical we keep posting this, so just set a long delay
                mNextMatchMakerPingTime = (UInt32) std::time(&currDT) + 3600UL; // 3600 sec = 1 hour till next attempt
                return; // do nothing further
              #else
                DEBUG_OUT("Host::SpendTime: MatchMaker Post failed with error "<<err<<" -- halting post attempts", DEBUG_ERROR);
                delete mMatchMakerGame;
                mMatchMakerGame = NULL;
                delete mMatchMaker;
                mMatchMaker = NULL;
              #endif // !GALACTICA_SERVER
            }
        }
        mNextMatchMakerPingTime = (UInt32) std::time(&currDT) + kPingMatchMakerInterval;
    }
  #endif
}


void
GalacticaHost::SpecifyHost(FSSpec &inDataSpec, FSSpec &inIndexSpec) {
   mGameDataSpec = inDataSpec;
   mGameIndexSpec = inIndexSpec;
   mGameIndexFile = new CMasterIndexFile();         // a. Create a new AMasterIndex subclass
   mGameDataFile = new CVarDataFile(mGameIndexFile);   // b. create new ADataStore subclass with the new AMasterIndex
}

void
GalacticaHost::CloseHostFiles() {
   DEBUG_OUT("GalacticaHost::CloseHostFiles ", DEBUG_IMPORTANT | DEBUG_DATABASE);
   try {
       if (mGameIndexFile) {
          mGameIndexFile->Close();
          mGameIndexFile = NULL;
       }
       if (mGameDataFile) {
          mGameDataFile->Close();
          mGameDataFile = NULL;
       }
   }
   catch(...) {
       DEBUG_OUT("GalacticaHost::CloseHostFiles: unexpected exception", DEBUG_ERROR | DEBUG_DATABASE);
   }
}

void
GalacticaHost::ReadGameData() {      // read in entire database
   DEBUG_OUT("GalacticaHost::ReadGameData", DEBUG_IMPORTANT);
   // we already have the GameInfo
    LoadAllPlayerInfo();
   AGalacticThingy *aThingy;
   ThrowIf_(mEverything.GetCount() != 0);   // never call this once things have already been added
    
    // load all items that are in the master index
    long lowestRecID = PRIVATE_REC_ID(mGameInfo.totalNumPlayers) + 1;
    long lastIndex = mGameIndexFile->GetEntryCount();
    IndexEntryT entry;
    long recSize = sizeof(DatabaseRec) + sizeof(long);
    DatabaseRec* recP = (DatabaseRec*)std::malloc(recSize);
    StartStreaming();
    for (int i = 1; i<=lastIndex; i++) {
        mGameIndexFile->FetchEntryAt(i, entry);
        if (entry.recID >= lowestRecID) {
            DEBUG_OUT("Found entry for record "<<entry.recID<<" in Database Index", DEBUG_DETAIL | DEBUG_DATABASE);
            recP->recID = entry.recID;// this is a valid, non-player info recored, must be a Thingy
            recP->recSize = recSize;  // we are only going to read the first 16 bytes of the record
            long classType = 0;
         try {
             aThingy = NULL;
                mGameDataFile->ReadRecord(recP);  // to get the thingy subclass type tag
                classType = BigEndian32_ToNative(*((long*)recP->recData));
             aThingy = MakeThingy(classType);
             ASSERT(aThingy != NULL);
             if (aThingy) {
               aThingy->ReadFromDataStore(entry.recID);
            }
         } // end Try
         catch (LException& e) {
            DEBUG_OUT("Exception "<<e.what()<<" trying to read "
                <<LongTo4CharStr(classType)<<" "<<entry.recID
                <<" [?]", DEBUG_ERROR | DEBUG_DATABASE);
            if ( (e.GetErrorCode() != dbItemNotFound) && (e.GetErrorCode() != dbInvalidID) ) {
               DEBUG_OUT("...re-throwing Exception "<<e.what(), DEBUG_ERROR | DEBUG_DATABASE);
               throw e;
            } else {
               delete aThingy;
            }
         } // end Catch
        } // if (entry.recID >= lowestRecID)
    }
    StopStreaming();
    std::free(recP);
    DEBUG_OUT("Read all thingys from Database", DEBUG_IMPORTANT | DEBUG_DATABASE);
    FinishGameLoad();
   mNewThingys.RemoveAllItemsAfter(0);
}

void
GalacticaHost::ResumeTurnClock() {
    // start the clock on a new turn
   if (mGameInfo.maxTimePerTurn) {   // 1.2b7 changes, fix countdown on resume of saved game
      mAutoEndTurnTimer.Resume();
       DEBUG_OUT("Resumed Timer with " << mAutoEndTurnTimer.GetTimeRemaining() << " secs left", 
                   DEBUG_DETAIL | DEBUG_EOT);
      mNextSpendTime = 0;
      StartRepeating();
   }
}

void
GalacticaHost::InitNewGameData(NewGameInfoT &inGameInfo) {
   DEBUG_OUT("HostDoc::InitNewGameData "<<(void*)this, DEBUG_IMPORTANT);
   mGameDataFile->SetBatchMode(true);
   FillSector(inGameInfo.density, inGameInfo.sectorSize);
   FinishGameLoad();
   WriteChangesToHost();   // write all the data out to the data file
   mGameDataFile->SetBatchMode(false);
}

void
GalacticaHost::DoCreateNew(FSSpec &dataSpec, FSSpec &indexSpec, NewGameInfoT &inGameInfo) {
   DEBUG_OUT("GalacticaHost::DoCreateNew "<<(void*)this, DEBUG_IMPORTANT);
   NotifyBusy();      // put up our busy cursor.
   SpecifyHost(dataSpec, indexSpec);
#if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
   char filename[2048];
   int err = FSpGetPosixFullPath(&indexSpec, filename, 2048);
   if (err != fnfErr) {
        ThrowIfOSErr_(err);
   }
   static_cast<CMasterIndexFile*>(mGameIndexFile)->OpenNew("", filename, TYPE_CREATOR);   // c. open a new index file
   err = FSpGetPosixFullPath(&dataSpec, filename, 2048);
   if (err != fnfErr) {
        ThrowIfOSErr_(err);
   }
   static_cast<CVarDataFile*>(mGameDataFile)->OpenNew("", filename, TYPE_CREATOR);      // d. open a new data file
#else
   static_cast<CMasterIndexFile*>(mGameIndexFile)->OpenNew(indexSpec.path, indexSpec.name, TYPE_CREATOR);   // c. open a new index file	
   static_cast<CVarDataFile*>(mGameDataFile)->OpenNew(dataSpec.path, dataSpec.name, TYPE_CREATOR);      // d. open a new data file
#endif // MacOS
   mGameIndexFile->SetTypeAndVersion(type_GameIndexFile, version_IndexFile);   // we have exclusive write
   mGameDataFile->SetTypeAndVersion(type_GameDataFile, version_GameFile);      // permission on the DB
   mGameDataFile->SetBatchMode(true);      // we are about to perform a bunch of adds
   
   GalacticaProcessor::CreateNew(inGameInfo);
   SaveGameInfo(true);            // reserve record for game info, and let it know we have a host
   DEBUG_OUT("Wrote Game Info to database", DEBUG_DETAIL | DEBUG_DATABASE);

   PublicPlayerInfoT thePlayerInfo;
   PrivatePlayerInfoT thePrivateInfo;
   thePlayerInfo.isLoggedIn = false;
   thePlayerInfo.isComputer = false;
   thePlayerInfo.isAssigned = false;
   thePlayerInfo.isAlive = true;
   thePlayerInfo.name[0] = '\0';      // no player assigned yet
   thePrivateInfo.password[0] = '\0';
   thePrivateInfo.lastTurnPosted = 0;   // no turns posted yet either
   thePrivateInfo.numPiecesRemaining = 1;   // but they are alive, and will have 1 star
   thePrivateInfo.hiStarTech = 1;
   thePrivateInfo.hiShipTech = -5;      // ERZ 1.2b8d4 mod, show messages for built Traveler
   thePrivateInfo.builtColony = false;   // for "new class of ship" message
   thePrivateInfo.builtScout = false;   // for "new class of ship" message
   thePrivateInfo.builtSatellite = false;   // for "new class of ship" message
   thePrivateInfo.hasTechStar = false;   // for computer players only
   thePrivateInfo.compSkill = -1;
   thePrivateInfo.compAdvantage = -1;

   DatabaseRec* tempRecP = (DatabaseRec*) std::malloc(sizeof(PublicPlayerInfoT) + sizeof(PrivatePlayerInfoT) + sizeof(DatabaseRec));   
   for (int i = 1; i <= mGameInfo.totalNumPlayers; i++) { // reserve records for player info
      NotifyBusy();      // put up our busy cursor.
      if (PlayerIsComputer(i)) {
         thePlayerInfo.isComputer = true;
         thePlayerInfo.isAssigned = true;
         thePrivateInfo.compSkill = inGameInfo.compSkill;
         thePrivateInfo.compAdvantage = inGameInfo.compAdvantage;
      } else {
         thePlayerInfo.isComputer = false;
         thePrivateInfo.compSkill = -1;
         thePrivateInfo.compAdvantage = -1;
      }
      tempRecP->recID = 0;
      tempRecP->recSize = sizeof(PublicPlayerInfoT)+sizeof(DatabaseRec);
      std::memcpy(tempRecP->recData, &thePlayerInfo, sizeof(PublicPlayerInfoT));
      mGameDataFile->AddRecord(tempRecP); // reserve blank player info slot
      ASSERT(tempRecP->recID == PUBLIC_REC_ID(i));
      tempRecP->recID = 0;
      tempRecP->recSize = sizeof(PrivatePlayerInfoT)+sizeof(DatabaseRec);
      std::memcpy(tempRecP->recData, &thePrivateInfo, sizeof(PrivatePlayerInfoT));
      mGameDataFile->AddRecord(tempRecP); // reserve blank player info slot
      ASSERT(tempRecP->recID == PRIVATE_REC_ID(i));
      DEBUG_OUT("Created record set for player " << i , DEBUG_DETAIL | DEBUG_DATABASE);
   }
   std::free(tempRecP);
   DEBUG_OUT("Wrote all player info to database", DEBUG_DETAIL | DEBUG_DATABASE);
   mGameDataFile->SetBatchMode(false);      // we are done with the bunch of adds
   InitializeServer();
}


// note: if you change how the data is stored on the host, make sure you update
// the revert to saved turn portion of GalacticaDoc::ReadSavedTurnFile()
void
GalacticaHost::WriteChangesToHost() {
   DEBUG_OUT("HostDoc::WriteChangesToHost", DEBUG_IMPORTANT);
   AGalacticThingy   *aThingy;
   UInt32 numPanes, i;
   LArray deleteList(sizeof(AGalacticThingy*));

   // ==============================
   //    PREPARE FOR DELETION
   // ==============================
   DEBUG_OUT("Checking "<<mEverything.GetCount()<< " items for deletion prep", DEBUG_DETAIL | DEBUG_EOT);
   LArrayIterator iterator(mEverything, LArrayIterator::from_Start); // Search list of all thingys
   while (iterator.Next(&aThingy)) {
      bool dead;
      if (!ValidateThingy(aThingy)) {
          DEBUG_OUT("Found Invalid Thingy "<<aThingy<<" in Host everything list, removing...", DEBUG_ERROR | DEBUG_EOT);
          mEverything.Remove(aThingy);
          mNewThingys.Remove(aThingy);   // take it out of the new list (only there if created this turn)
          continue;   // this item is invalid, do nothing more with it lest we crash
      } else {
          dead = aThingy->IsDead();
      }
      if (dead) {
          aThingy->RemoveSelfFromGame();
      }
   }

   // ===================================
   //    INFORM CLIENTS OF NEW THINGYS
   // ===================================
   if (mNewThingys.GetCount() > 0) {      // start with the things that have been added
      numPanes = mNewThingys.GetCount();
      DEBUG_OUT("Sending "<<numPanes<<" action_Added items to client", DEBUG_DETAIL | DEBUG_EOT);
      for (i = 1; i <= numPanes; i++) {
         NotifyBusy();      // put up our busy cursor.
         mNewThingys.FetchItemAt(i, aThingy);
         if (!ValidateThingy(aThingy)) {
             DEBUG_OUT("Invalid Thingy in new Thingys list, skipping...", DEBUG_ERROR | DEBUG_EOT);
            continue;
         }
         if (aThingy->IsDead()) {
            DEBUG_OUT("Found dead "<<aThingy<<" in new thingys list, skipping...", DEBUG_IMPORTANT | DEBUG_EOT);
            continue;
         }
         EAction action = action_Added;
         long thingType = aThingy->GetThingySubClassType();
         if (IS_ANY_MESSAGE_TYPE(thingType)) {
            action = action_Message;
         } else {
            action = action_Added;
         }
         if (aThingy->IsChanged()) {
            try {
               aThingy->WriteToDataStore();   // no longer writing in AGalacticThingy::EndOfTurn()
               BroadcastThingy(aThingy, action);   // send the thingy to all connected clients
               aThingy->NotChanged();   // clear the changed flag so they aren't marked as updated too
            }
            catch (LException& e) {
               DEBUG_OUT("Exception "<<e.what()<<" trying to write "<<aThingy, DEBUG_ERROR | DEBUG_EOT);
               if ( (e.GetErrorCode() == dbItemNotFound) || (e.GetErrorCode() == dbInvalidID)) {
                    DEBUG_OUT(aThingy << " not in database, adding to delete list", DEBUG_ERROR | DEBUG_EOT);
                  deleteList.InsertItemsAt(1, LArray::index_Last, &aThingy);   // thingy isn't in database, dump it.
                  aThingy->Die();
                  aThingy->RemoveSelfFromGame();
               } else {
                  DEBUG_OUT("...re-throwing Exception "<<e.what(), DEBUG_ERROR | DEBUG_EOT);
                  throw e;   // some other error, pass it on
               }
            }
         } else if ( aThingy->GetThingySubClassType() == thingyType_Star ) {
            DEBUG_OUT(aThingy << " is a newly created star in new thingys list", DEBUG_TRIVIA | DEBUG_EOT);
         } else if ( action == action_Message ) {
            DEBUG_OUT("WARNING: "<< aThingy << " is unchanged in new thingys list", DEBUG_ERROR | DEBUG_EOT);
         } else if (!aThingy->IsDead()) {
            DEBUG_OUT(aThingy << " in host new thingys list neither changed nor deleted!!", DEBUG_ERROR | DEBUG_EOT);
         }
         GalacticaApp::YieldTimeToForegoundTask(); // give up time to forground process
      }
      mNewThingys.RemoveAllItemsAfter(0);   // don't need this list anymore
   }

   // =======================================
   //    INFORM CLIENTS OF UPDATED THINGYS
   // =======================================
   DEBUG_OUT("Checking for updated items to send to client", DEBUG_DETAIL | DEBUG_EOT);
   iterator.ResetTo(LArrayIterator::from_Start); // reset to search list of all thingys
   while (iterator.Next(&aThingy)) {
      NotifyBusy();      // put up our busy cursor.
      if (aThingy->IsDead()) {
         deleteList.InsertItemsAt(1, LArray::index_Last, &aThingy);   // make sure this gets into the delete list
      } else if (aThingy->IsChanged()) {
         aThingy->WriteToDataStore();   // no longer writing in AGalacticThingy::EndOfTurn()
         if (!IS_ANY_MESSAGE_TYPE(aThingy->GetThingySubClassType())) {   // no messages sending updates, handled in new adds
              BroadcastThingy(aThingy, action_Updated);   // send the update for the thingy to all connected clients
          } else {
            DEBUG_OUT(aThingy << " is a changed message everything list", DEBUG_ERROR | DEBUG_EOT);
         }
         aThingy->NotChanged();
      }
      GalacticaApp::YieldTimeToForegoundTask();   // give up time to forground process
   }

   // =======================================
   //    INFORM CLIENTS OF DELETED THINGYS
   // =======================================
   DEBUG_OUT("Deleting "<<deleteList.GetCount()<< " items from host.", DEBUG_DETAIL | DEBUG_EOT);
   for (i = 1; i <= deleteList.GetCount(); i++) {
      NotifyBusy();      // put up our busy cursor.
      deleteList.FetchItemAt(i, &aThingy);
      aThingy->DeleteFromDataStore();
      BroadcastThingy(aThingy, action_Deleted);   // send the deletion notice for the thingy to all connected clients
      delete aThingy;   // now we can delete the dead thingy
      aThingy = NULL;
   }
   ASSERT(deleteList.GetCount() == mDeadThingys.GetCount());  // sanity check
   if (mDeadThingys.GetCount() > 0) {
      numPanes = mDeadThingys.GetCount();
      DEBUG_OUT("Clearing "<<numPanes<< " deletion info recs from host dead thingys list", DEBUG_DETAIL | DEBUG_EOT);
      mDeadThingys.RemoveAllItemsAfter(0);   // don't need the dead things array anymore
   }

   // ==============================
   //       UPDATE PLAYER INFO
   // ==============================
      //      for now we just assume everyone can see the entire universe 
   long* playerPieceCount = new long[mGameInfo.totalNumPlayers+1];
   for (int i=0; i <= mGameInfo.totalNumPlayers; i++) {   // reset the piece count for the players
      playerPieceCount[i] = 0;
   }
   int livingHuman = -1;
   DEBUG_OUT("Updating player info for all clients", DEBUG_DETAIL | DEBUG_EOT);
   long n = 0;
   long player;
   long type;
   iterator.ResetTo(LArrayIterator::from_Start); // now player info
   while ( iterator.Next(&aThingy) ) {
      if ( aThingy->IsDead() ) {
          continue; // should never happen, we just removed these
      }
      type = aThingy->GetThingySubClassType();
      if ( (type == thingyType_Ship) || (type == thingyType_Star) ) {
         DEBUG_OUT("  Counting living "<< aThingy << " toward player total", DEBUG_TRIVIA | DEBUG_EOT);
         player = aThingy->GetOwner();
         if (player > mGameInfo.totalNumPlayers) {
            DEBUG_OUT("Found thingy "<< aThingy << " with invalid owner ["<<player<<"]", DEBUG_ERROR | DEBUG_EOT);
            player = 0;
         }
         playerPieceCount[player]++;
         n++;
      }
   }
   DEBUG_OUT("Counted "<<n<<" of "<<mEverything.GetCount()<<" living ships and stars", DEBUG_DETAIL | DEBUG_EOT);

   PrivatePlayerInfoT thePlayerInfo;
   for (int i = 1; i <= mGameInfo.totalNumPlayers; i++) {   //update players' changed items list
      GetPrivatePlayerInfo(i, thePlayerInfo);
      DEBUG_OUT("EOT: Player "<<i<<" went from "<<thePlayerInfo.numPiecesRemaining<<" to "
            <<playerPieceCount[i]<<" thingys this turn", DEBUG_DETAIL | DEBUG_EOT);      

      if (thePlayerInfo.numPiecesRemaining) {
         thePlayerInfo.numPiecesRemaining = playerPieceCount[i];   // find out how many pieces the player has
         SetPrivatePlayerInfo(i, thePlayerInfo);
         PublicPlayerInfoT pubInfo;
         GetPlayerInfo(i, pubInfo);  // mark them as dead
         if (thePlayerInfo.numPiecesRemaining == 0) {   // player just died
            pubInfo.isAlive = false;
            SetPlayerInfo(i, pubInfo);
            DEBUG_OUT("== Player "<<i<<" was eliminated from the game ==", DEBUG_IMPORTANT | DEBUG_EOT);
            mHostLog << pdg::log::category("LOSE") << pdg::log::inform
               << "Player ["<<(AS_NUMBER)i<<"] was defeated" << pdg::endlog;
            mGameInfo.numPlayersRemaining--;
            CMessage* msg = CMessage::CreateEvent(this, event_PlayerDied, i, nil, kAllPlayers);
            if (msg) {
                BroadcastThingy(msg, action_Message);  // inform all the connected players
            }
            if (pubInfo.isComputer) {
               mGameInfo.numComputersRemaining--;
            } else {
               mGameInfo.numHumansRemaining--;
            }
         } else if (!pubInfo.isComputer) {
            livingHuman = i;   // keep track of who is alive
         }
      }
   }

   delete[] playerPieceCount;   // v1.2fc5, fix memory leak
   
   // v1.2b11d5, make sure brightness, etc get recalculated
//   RecalcHighestValues();

   if (mGameInfo.gameState == state_GameOver) {
      DEBUG_OUT("== Game over: Now in Post Game mode ==", DEBUG_IMPORTANT | DEBUG_EOT);
      mGameInfo.gameState = state_PostGame;
   }
   if ( (mGameInfo.numHumansRemaining == 1) && (mGameInfo.numComputersRemaining<1)
      && (mGameInfo.gameState == state_NormalPlay) && // don't give message during post game
      ( (mGameInfo.numComputers > 0) || (mGameInfo.numHumans > 1) ) )  {   // or if started alone
      // we have a winner: send a notice to the player
      DEBUG_OUT("== Player "<<livingHuman<<" wins the game! ==", DEBUG_IMPORTANT | DEBUG_EOT);
      mHostLog << pdg::log::category("WIN") << pdg::log::inform
         << "Player ["<<(AS_NUMBER)i<<"] wins the game" << pdg::endlog;
      CMessage* msg = CMessage::CreateEvent(this, event_PlayerWins, livingHuman, nil, kAllPlayers);
      if (msg) {
         BroadcastThingy(msg, action_Message);
      }
      mGameInfo.gameState = state_GameOver;
   }
}

void
GalacticaHost::FinishTurn() {
    GalacticaProcessor::FinishTurn();
}

void
GalacticaHost::PrepareForEndOfTurn() {
   DEBUG_OUT("GalacticaHost::PrepareForEndOfTurn", DEBUG_IMPORTANT);
   BackupDatabaseFiles();
   // ==============================
   //    DELETE ALL MESSAGES
   // ==============================
   DEBUG_OUT("Checking "<<mEverything.GetCount()<< " items for messages to delete", DEBUG_DETAIL | DEBUG_EOT);
   LArrayIterator iterator(mEverything, LArrayIterator::from_Start); // Search list of all thingys
   AGalacticThingy* aThingy;
   while (iterator.Next(&aThingy)) {
      if (!ValidateThingy(aThingy)) {
          DEBUG_OUT("Found Invalid Thingy "<<aThingy<<" in Host everything list, removing...", DEBUG_ERROR | DEBUG_EOT);
          mEverything.Remove(aThingy);
          mNewThingys.Remove(aThingy);   // take it out of the new list (only there if created this turn)
          continue; // do nothing more with this invalid item lest we crash
      } else if ( IS_ANY_MESSAGE_TYPE(aThingy->GetThingySubClassType()) ) {
          DEBUG_OUT("Found Message to delete "<<aThingy<<" in Host everything list, deleting...", DEBUG_TRIVIA | DEBUG_EOT);          
          aThingy->DeleteFromDataStore();
          delete aThingy;
          aThingy = NULL;
      }
   }
   GalacticaProcessor::PrepareForEndOfTurn();
}

void
GalacticaHost::DoEndTurn() {
   mHostLog << pdg::log::category("EOT") << pdg::log::inform
      << "start processing end of turn [" << (AS_NUMBER)mGameInfo.currTurn << "]" << pdg::endlog;
   GalacticaProcessor::DoEndTurn();
   if (mGameInfo.maxTimePerTurn) {
       DEBUG_OUT("Reset turn timer to " << mGameInfo.nextTurnEndsAt, DEBUG_DETAIL | DEBUG_EOT);
      mAutoEndTurnTimer.Reset(mGameInfo.nextTurnEndsAt);
   } else {
      mAutoEndTurnTimer.Clear();   // make timer read blank
   }
   SaveAllPlayerInfo();
   SaveGameInfo();
   BroadcastGameInfo();
   TurnCompletePacket* p = CreatePacket<TurnCompletePacket>();
   p->turnNum = mGameInfo.currTurn;
   BroadcastPacket(p);
   ReleasePacket(p);
   mHostLog << pdg::log::category("EOT") << pdg::log::inform
      << "done processing end of turn" << pdg::endlog;
}

void
GalacticaHost::TimeIsUp() {   // from CTimed, called by CTimer when the turn time limit expires
   DEBUG_OUT("GalacticaHost::TimeIsUp was called!", DEBUG_ERROR | DEBUG_EOT);
   if (mAnyoneLoggedOn || ( (mGameInfo.endTurnEarly == false) && (mGameInfo.maxTimePerTurn > 0) ) ) { 
       // we will actually allow end of turn processing when time is up if someone is logged on
       // or if this time limited game that auto processes only when the time is up
       DoEndTurn();
   } else {
       DEBUG_OUT("GalacticaHost::Doing nothing since nobody is logged on", DEBUG_IMPORTANT | DEBUG_EOT);
   }
}


void
GalacticaHost::OpenHost() {
    DEBUG_OUT("GalacticaHost::OpenHost ", DEBUG_IMPORTANT | DEBUG_DATABASE);
    StopIdling();
    try {
#if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
        char filename[2048];
        int err = FSpGetPosixFullPath(&mGameIndexSpec, filename, 2048);
        ThrowIfOSErr_(err);
        static_cast<CMasterIndexFile*>(mGameIndexFile)->OpenExisting("", filename);// c. open the existing index file

        err = FSpGetPosixFullPath(&mGameDataSpec, filename, 2048);
        ThrowIfOSErr_(err);  
        static_cast<CVarDataFile*>(mGameDataFile)->OpenExisting("", filename);       // d. open the existing data file
#else
        static_cast<CMasterIndexFile*>(mGameIndexFile)->OpenExisting(mGameIndexSpec.path, mGameIndexSpec.name);// c. open the existing index file
        static_cast<CVarDataFile*>(mGameDataFile)->OpenExisting(mGameDataSpec.path, mGameDataSpec.name);       // d. open the existing data file
#endif 
    }
    catch(std::exception& e) {   // we retrap any exceptions here so we can deal with them below
        CloseHostFiles();   // Close files here rather than leave it to callers
        DEBUG_OUT("Exception "<<e.what()<<" trying to open host", DEBUG_ERROR | DEBUG_DATABASE);
        throw e;   // rethrow the error now that we've put things back the way they were
    }
}


void
GalacticaHost::HandleReceiveData(PEndpointRef inEndpoint) {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (mConnections[i].IsThisEndpoint(inEndpoint)) {
            ASYNC_DEBUG_OUT("Host found connection "<< i<< " for endpoint "<< inEndpoint, DEBUG_IMPORTANT | DEBUG_PACKETS);
            mConnections[i].Receive();
            return;
        }
    }
   ASYNC_DEBUG_OUT("GalacticaHost::HandleReceiveData could not find connected endpoint "<<inEndpoint, 
               DEBUG_ERROR | DEBUG_PACKETS);
}

void
GalacticaHost::MapEndpoint(PEndpointRef inEndpoint) {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!mConnections[i].HasEndpointRef()) {
            mConnections[i].SetEndpointRef(inEndpoint);
            ASYNC_DEBUG_OUT("Host mapped endpoint " << inEndpoint << " to connection " << i, DEBUG_IMPORTANT | DEBUG_PACKETS);
            return;
        }
    }
   ASYNC_DEBUG_OUT("GalacticaHost::HandleReceiveData could not find connection slot for endpoint "<<inEndpoint, 
               DEBUG_ERROR | DEBUG_PACKETS);
    ProtocolCloseEndpoint(inEndpoint, false);   // no choice but to disconnect them
}

void
GalacticaHost::HandleFlowClear(PEndpointRef inEndpoint) {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (mConnections[i].IsThisEndpoint(inEndpoint)) {
            ASYNC_DEBUG_OUT("Host found connection "<< i<< " for endpoint "<< inEndpoint, DEBUG_IMPORTANT | DEBUG_PACKETS);
            mConnections[i].SendWaitingPackets();
            return;
        }
    }
   ASYNC_DEBUG_OUT("GalacticaHost::HandleFlowClear could not find connected endpoint "<<inEndpoint, 
               DEBUG_ERROR | DEBUG_PACKETS);
}

// doing this because CW Pro 8 seemed to be generating bad code for the GalacticaClient::ClientEnumerationCallback 
// doesn't need to be here as far as we know, but putting it around all these static OpenPlay callbacks just in case

#if COMPILER_MWERKS
#pragma optimization_level 0
#endif // COMPILER_MWERKS

void
GalacticaHost::HostCallback (PEndpointRef inEndpoint, void* inContext, NMCallbackCode inCode, NMErr inError, void* inCookie) {
   GalacticaHost* theHostDoc = static_cast<GalacticaHost*>(inContext);
//   if (inError != noErr) {
//       ASYNC_DEBUG_OUT("HostCallback() got error code "<<inError, DEBUG_ERROR | DEBUG_PACKETS);
//    }
   switch(inCode)
   {
      case kNMConnectRequest:
         ASYNC_DEBUG_OUT("HostCallback "<<(void*)theHostDoc << " got connect request", DEBUG_IMPORTANT | DEBUG_PACKETS);
         theHostDoc->HandleConnectRequest(static_cast<PEndpointRef>(inCookie));
         break;
      case kNMDatagramData:
         break;
      case kNMStreamData:
         ASYNC_DEBUG_OUT("HostCallback receiving data on endpoint "<< inEndpoint <<" Error = "<<inError, DEBUG_DETAIL | DEBUG_PACKETS);
         theHostDoc->HandleReceiveData(inEndpoint);
         break;
      case kNMFlowClear:
         ASYNC_DEBUG_OUT("Host Got FlowClear. Error = " << inError, DEBUG_IMPORTANT | DEBUG_PACKETS);
         theHostDoc->HandleFlowClear(inEndpoint);
         break;
      case kNMAcceptComplete:
         ASYNC_DEBUG_OUT("Host Got AcceptComplete " << inCookie << " Error = " << inError, DEBUG_IMPORTANT | DEBUG_PACKETS);
         break;
      case kNMHandoffComplete:
         ASYNC_DEBUG_OUT("Host Got HandoffComplete " << inCookie << " Error = " << inError, DEBUG_IMPORTANT | DEBUG_PACKETS);
         theHostDoc->MapEndpoint(static_cast<PEndpointRef>(inCookie));
         break;
      case kNMEndpointDied:
         ASYNC_DEBUG_OUT("Host Got EndpointDied " << inEndpoint << " Error = " << inError, DEBUG_IMPORTANT | DEBUG_PACKETS);
         ProtocolCloseEndpoint(inEndpoint, false);
         theHostDoc->HandleDisconnect(inEndpoint);
         break;
      case kNMCloseComplete:
//         ASYNC_DEBUG_OUT("Host Got CloseComplete " << inEndpoint << " Error = " << inError, DEBUG_IMPORTANT | DEBUG_PACKETS);
         break;         
      default:
         ASYNC_DEBUG_OUT("Host Got unknown message "<< inCode << " " << inCookie << " Error = " << inError, DEBUG_ERROR | DEBUG_PACKETS);
         break;
   }   
   return;
}

void
GalacticaHost::HostStatusCallback (PEndpointRef inEndpoint, void* inContext, NMCallbackCode inCode, NMErr, void* inCookie) {
//   if (inError != noErr) {
//       ASYNC_DEBUG_OUT("HostCallback() got error code "<<inError, DEBUG_ERROR | DEBUG_PACKETS);
//    }
   switch(inCode)
   {
      case kNMConnectRequest: 
         {
            ASYNC_DEBUG_OUT("HostStatusCallback got connect request", 
                            DEBUG_IMPORTANT | DEBUG_PACKETS);
            NMErr err = ProtocolAcceptConnection(inEndpoint, inCookie, HostStatusCallback, inContext);
            if (err != noErr) {
               ASYNC_DEBUG_OUT("Status: Host ProtocolAcceptConnection returned "
                        << err, DEBUG_ERROR | DEBUG_PACKETS);
               return;
            }
            ASYNC_DEBUG_OUT("Status: Host Successfully accepted startis connection " << inCookie, 
                            DEBUG_IMPORTANT | DEBUG_PACKETS);
         }
         break;
      case kNMHandoffComplete:
         {
            GalacticaHost* theHost = static_cast<GalacticaHost*>(inContext);
            ASYNC_DEBUG_OUT("HostStatusCallback Got HandoffComplete " << inCookie, DEBUG_IMPORTANT | DEBUG_PACKETS);
            PEndpointRef endpoint = static_cast<PEndpointRef>(inCookie);
            char p[256];
            std::sprintf(p, "Galactica v2.3 Host OK\nTurn %ld\n", theHost->GetTurnNum());
            long sentBytes = ProtocolSend(endpoint, (void*)p, std::strlen(p), 0);
            if (sentBytes != (long)std::strlen(p)) {
               ASYNC_DEBUG_OUT("Status: failed to send entire reply" << inCookie, 
                             DEBUG_ERROR | DEBUG_PACKETS);
            }
            ProtocolCloseEndpoint(endpoint, true); // do an orderly shutdown so the socket can drain
         }
         break;
      default:
         break;
   }   
   return;
}

#if COMPILER_MWERKS
#pragma optimization_level reset
#endif // COMPILER_MWERKS



void
GalacticaHost::HandlePacketFrom(PEndpointRef fromRemoteEndpoint, Packet* inPacket) {
   // a connected client sent us a packet
   if (!inPacket) {
      return;
   }
   DEBUG_OUT("Host got packet "<<inPacket->GetName() << ", size = "<<inPacket->packetLen 
             << " from Connection "<< fromRemoteEndpoint, DEBUG_IMPORTANT | DEBUG_PACKETS);

   SInt8 playerNum = EndpointToPlayerNum(fromRemoteEndpoint);
   // admin packet handling
   if (inPacket->packetType >= packet_Admin) {
      if (playerNum != kAdminPlayerNum) {
         mHostLog << pdg::log::category("HACK") << pdg::log::error
            << "Player ["<<(AS_NUMBER)playerNum<<"] sent an Administrator Packet (ignored)" << pdg::endlog;
         return;   // ignore any admin packets that aren't from the admin
      } else {
         switch (inPacket->packetType) {
            case packet_AdminSetGameInfo:
                HandleAdminSetGameInfo(static_cast<AdminSetGameInfoPacket*>(inPacket));
                break;
            case packet_AdminSetPlayerInfo:
                HandleAdminSetPlayerInfo(static_cast<AdminSetPlayerInfoPacket*>(inPacket));
                break;
            case packet_AdminPauseGame:
                HandleAdminPauseGame();
                break;
            case packet_AdminResumeGame:
                HandleAdminResumeGame();
                break;
            case packet_AdminEndTurnNow:
                HandleAdminEndTurnNow();
                break;
            case packet_AdminEjectPlayer:
                HandleAdminEjectPlayer(static_cast<AdminEjectPlayerPacket*>(inPacket));
                break;
            default:
               break;
         }
      }
   }
   if (inPacket->packetType == packet_Ping) {
      // everyone is allowed to ping
      HandlePing(fromRemoteEndpoint, static_cast<PingPacket*>(inPacket));
   } else if (inPacket->packetType == packet_GameInfoRequest) {
      // everyone is allowed to send game info request
      HandleGameInfoRequest(fromRemoteEndpoint);
   } else if (inPacket->packetType == packet_PlayerInfoRequest) {
      // everyone is allowed to send player info request
      const bool kAlsoSendPlayerInfo = true;
      HandleGameInfoRequest(fromRemoteEndpoint, kAlsoSendPlayerInfo);
   } else if (playerNum == kNoSuchPlayer) {
      // not a logged in player, can only handle login request
      if (inPacket->packetType == packet_LoginRequest) {
         HandleLoginRequest(fromRemoteEndpoint, static_cast<LoginRequestPacket*>(inPacket));
      } else {
         DEBUG_OUT("Host got illegal "<<inPacket->GetName() << ", size = "<<inPacket->packetLen 
                  << " from Connection "<< fromRemoteEndpoint, DEBUG_ERROR | DEBUG_PACKETS);
      }
   } else {
        // logged in players can handle everything but login requests
        switch (inPacket->packetType) {
            case packet_PlayerToPlayerMessage:
                HandlePlayerToPlayerMessage(playerNum, inPacket);
                break;
            case packet_ThingyData:
                HandleThingyData(playerNum, static_cast<ThingyDataPacket*>(inPacket));
                break;
            case packet_TurnComplete:
                HandleTurnComplete(playerNum, static_cast<TurnCompletePacket*>(inPacket));
                break;
            case packet_CreateThingyRequest:
                HandleCreateThingy(playerNum, static_cast<CreateThingyRequestPacket*>(inPacket));
                break;
            case packet_ReassignThingyRequest:
                HandleReassignThingy(playerNum, static_cast<ReassignThingyRequestPacket*>(inPacket));
                break;
            case packet_ClientSettings:
				HandleClientSettings(playerNum, static_cast<ClientSettingsPacket*>(inPacket));
				break;
            case packet_Ignore:
            default:
                break;
        }
    }
}

void
GalacticaHost::SendPacketTo(PEndpointRef toRemoteEndpoint, Packet* inPacket) {
   for (int i = 0; i < MAX_CONNECTIONS; i++) {
      if (mConnections[i].IsThisEndpoint(toRemoteEndpoint)) {
         ASYNC_DEBUG_OUT("Host found outgoing connection "<< i<< " for endpoint "
                          << toRemoteEndpoint, DEBUG_TRIVIA | DEBUG_PACKETS);
         mConnections[i].SendPacket(inPacket);
         return;
      }
   }
   ASYNC_DEBUG_OUT("GalacticaHost::SendPacketTo could not find connected endpoint "<<toRemoteEndpoint, 
               DEBUG_ERROR | DEBUG_PACKETS);
}

void
GalacticaHost::BroadcastPacket(Packet* inPacket, bool inToBrowsersAlso) {
   // send packet to all logged in players, or everyone connected if inToBrowsersAlso is true
   if (inPacket == NULL) {
      DEBUG_OUT("Host BroadcastPacket failed, NULL Packet", DEBUG_ERROR | DEBUG_PACKETS);
      return;
   }
   DEBUG_OUT("Host BroadcastPacket "<<inPacket->GetName(), DEBUG_DETAIL | DEBUG_PACKETS);
   if (inToBrowsersAlso) {
      // this goes to absolutely everyone who is connected, logged in or not
      for (int i = 0; i < MAX_CONNECTIONS; i++) {
         if (mConnections[i].HasEndpointRef()) {
            mConnections[i].SendPacket(inPacket);
         }
      }
   } else {
      // this just goes to people who are logged in
      for (int i = 0; i < MAX_PLAYERS_CONNECTED; i++) {
         if (mPlayerEndpoints[i] != NULL) {
            SendPacketTo(mPlayerEndpoints[i], inPacket);
         }
      }
      for (int i = 0; i < MAX_OBSERVERS_CONNECTED; i++) {
         if (mObservers[i] != NULL) {
            SendPacketTo(mObservers[i], inPacket);
         }
      }
   }
}

void
GalacticaHost::HandleConnectRequest(PEndpointRef fromRemoteEndpoint) {
   NMErr err = ProtocolAcceptConnection(mEndpoint, fromRemoteEndpoint, HostCallback, this);
   if (err != noErr) {
      DEBUG_OUT("Host ProtocolAcceptConnection returned "
               << err, DEBUG_ERROR | DEBUG_PACKETS);
      return;
   }
   DEBUG_OUT("Host Successfully accepted Connection "
            << fromRemoteEndpoint, DEBUG_IMPORTANT | DEBUG_PACKETS);
}

void
GalacticaHost::HandleGameInfoRequest(PEndpointRef fromEndpoint, bool inAlsoSendPlayerInfo) {
   SendGameInfo(fromEndpoint);
   if (inAlsoSendPlayerInfo) {
      // send info on all players
      for (int i = 1; i <= mGameInfo.totalNumPlayers; i++) {
         SendPlayerInfo(fromEndpoint, i);
      }
   }
}

void
GalacticaHost::HandlePing(PEndpointRef fromEndpoint, PingPacket* inPacket) {
   if (!inPacket->isReply) {
      inPacket->isReply = true;
      SendPacketTo(fromEndpoint, inPacket);  // reply with the same ping, but with reply set to true
   }
   // might want to put some code to do something particular with the replies to pings we sent
}

void
GalacticaHost::HandleLoginRequest(PEndpointRef fromEndpoint, LoginRequestPacket* inPacket) {
   DEBUG_OUT("Host "<<(void*)this<<" handling login request from Connection "
            << fromEndpoint, DEBUG_IMPORTANT | DEBUG_PACKETS);
   LoginResponsePacket* lrp = CreatePacket<LoginResponsePacket>();
   lrp->loginResult = login_ResultGameFull;
   PublicPlayerInfoT pInfo;
   PrivatePlayerInfoT privateInfo;
   LoginRequestPacket* p = inPacket;
   RegCodeStrT regCode;
   
      // copy the reg code and unobscure it as you go
   for (int i = 0; i < strLen_RegCode; i++) {
      if (p->regCode[i] == 0) {
         regCode[i] = 0;
         break;
      }
      regCode[i] = p->regCode[i] ^ 0xef;
   }
   regCode[strLen_RegCode] = 0;
   char ipStr[256];
   std::strcpy(ipStr, "0.0.0.0");
   
   #warning FIXME: this OpenPlay call is clobbering memory (the debug macros out ptr)
   NMErr err = kNMNoError;//ProtocolGetEndpointIdentifier(fromEndpoint, ipStr, 255);
   
   if (err != kNMNoError) {
      std::sprintf(ipStr, "error %ld", err);
   }

   bool playingHotSeat = ( std::strncmp(mHostIPStr, ipStr, 255) == 0 );
   if (!playingHotSeat) {
        // even if the IPs don't match, this still may be a hot seat game if
        // the client IDs match, so let them in. This gives us 1 in 127 odds
        // of letting an unregistered user into the game.
        int ourID = Galactica::Globals::GetInstance().getClientID();
        int theirID = inPacket->clientID();
        if (theirID == ourID) {
            playingHotSeat = true;
        }
   }

   DEBUG_OUT("Login "<< (p->isNew() ? "new":"existing") << " player [" << p->playerName 
           << "] reg code ["<<p->regCode
        //   << "] password ["<<p->password // v2.1b11r5 don't log password
           << "], max players = "
           << (int)mGameInfo.numHumans, DEBUG_DETAIL | DEBUG_PACKETS);
   if (std::strcmp(p->copyrightStr, string_Copyright) != 0) {
      DEBUG_OUT("Login attempt with bad protocol, no copyright string", DEBUG_ERROR);
      mHostLog << pdg::log::category("HACK") << pdg::log::error
       #ifdef GALACTICA_LOG_REGISTRATION_CODES
         << "reg [" << regCode << "] "
       #endif // GALACTICA_LOG_REGISTRATION_CODES
         << "ip ["<< ipStr <<"] "
         << "Login attempt with bad protocol, no copyright string"
         << pdg::endlog;
      lrp->loginResult = login_ResultBadVersion;
   } else if (p->clientVers != version_HostProtocol) {
      DEBUG_OUT("Login attempt with bad protocol version "<<(void*)p->clientVers, DEBUG_ERROR);
      mHostLog << pdg::log::category("VERS") << pdg::log::error
       #ifdef GALACTICA_LOG_REGISTRATION_CODES
         << "reg [" << regCode << "] "
       #endif // GALACTICA_LOG_REGISTRATION_CODES
         << "ip ["<< ipStr <<"] "
         << "Login attempt with bad protocol version "<<(void*)p->clientVers 
         << pdg::endlog;
      lrp->loginResult = login_ResultBadVersion;
   } else if (p->playerName[0] == 0) {
      DEBUG_OUT("Login attempt with bad data", DEBUG_ERROR | DEBUG_PACKETS);
      mHostLog << pdg::log::category("HACK") << pdg::log::error
       #ifdef GALACTICA_LOG_REGISTRATION_CODES
         << "reg [" << regCode << "] "
       #endif // GALACTICA_LOG_REGISTRATION_CODES
         << "ip ["<< ipStr <<"] "
         << "Login attempt with bad data" 
         << pdg::endlog;
      lrp->loginResult = login_ResultBadRequest;
   } else {
      lrp->loginResult = login_ResultSuccess;
	  DEBUG_OUT("Got ["<<p->copyrightStr<<"] from client on endpoint "<< fromEndpoint, 
               DEBUG_IMPORTANT | DEBUG_PACKETS);
      // check to see if this is an administrator
      if (std::strcmp(p->playerName, ADMINISTRATOR_NAME) == 0) {
         if (Galactica::Globals::GetInstance().getAdminPassword()[0] == 0) {
            // no admin login allowed
            DEBUG_OUT("Login attempt for Administrator failed: no admin login allowed", 
                     DEBUG_ERROR | DEBUG_PACKETS);
            mHostLog << pdg::log::category("HACK") << pdg::log::error
             #ifdef GALACTICA_LOG_REGISTRATION_CODES
               << "reg [" << regCode << "] "
             #endif // GALACTICA_LOG_REGISTRATION_CODES
               << "ip ["<< ipStr <<"] "
               << "Login attempt for Administrator failed: no admin login allowed" 
               << pdg::endlog;
            lrp->loginResult = login_ResultDuplicateName;
         } else {
            // 
            if (std::strcmp(Galactica::Globals::GetInstance().getAdminPassword(), p->password) != 0) {
               DEBUG_OUT("Login attempt for Administrator failed: bad password", 
                        DEBUG_ERROR | DEBUG_PACKETS);
               mHostLog << pdg::log::category("HACK") << pdg::log::error
                #ifdef GALACTICA_LOG_REGISTRATION_CODES
                  << "reg [" << regCode << "] "
                #endif // GALACTICA_LOG_REGISTRATION_CODES
                  << "ip ["<< ipStr <<"] "
                  << "Login attempt for Administrator failed: bad password" 
                  << pdg::endlog;
               lrp->loginResult = login_ResultBadPassword;
            } else {
               DEBUG_OUT("Administrator logged in", 
                        DEBUG_IMPORTANT | DEBUG_PACKETS);
               lrp->playerNum = kAdminPlayerNum;
               mObservers[0] = fromEndpoint;
            }
         }
      } else {
         int nameMatches = 0;
         int newSlot = 0;
         // find which player this name matches
         for (int i = 1; i <= mGameInfo.totalNumPlayers; i++) {
            GetPlayerInfo(i, pInfo);
            if (pInfo.isComputer) {
               continue;   // skip computer slots
            }
            if (!pInfo.isAssigned && (newSlot == 0)) {
               newSlot = i;
            }
            if (pInfo.isAssigned && (std::strcmp(pInfo.name, p->playerName) == 0)) {
               nameMatches = i;
            }
         }
         if (p->isNew() && (newSlot == 0) ) {
            // no new slot was found for a new player
            DEBUG_OUT("Login attempt as new player ["<<p->playerName<<"] to full game", DEBUG_ERROR | DEBUG_PACKETS);
            lrp->loginResult = login_ResultGameFull;
         } else if (p->isNew() && (nameMatches != 0) ){
            // new player attempting login with existing name
            DEBUG_OUT("Login attempt as new player ["<<p->playerName
                        <<"] but name already used by player "<<nameMatches, DEBUG_ERROR | DEBUG_PACKETS);
            lrp->loginResult = login_ResultDuplicateName;
         } else if (!p->isNew() && (nameMatches == 0) ) {
            // couldn't find the player
            DEBUG_OUT("Login attempt as existing player ["<<p->playerName<<"] who doesn't exist", DEBUG_ERROR | DEBUG_PACKETS);
            lrp->loginResult = login_ResultPlayerNotFound;
         } else if (!p->isNew() && (nameMatches != 0) ) {
            // if name match was found, check for password match
            GetPrivatePlayerInfo(nameMatches, privateInfo);
            char playerPassword[MAX_PASSWORD_LEN+1];
            std::strncpy(playerPassword, privateInfo.password, MAX_PASSWORD_LEN);
            playerPassword[MAX_PASSWORD_LEN] = 0; // nul terminate string
            if (std::strcmp(playerPassword, p->password) != 0) {
               DEBUG_OUT("Login attempt for player ["<<p->playerName<<"] with bad password", DEBUG_ERROR | DEBUG_PACKETS);
               mHostLog << pdg::log::category("HACK") << pdg::log::error
                #ifdef GALACTICA_LOG_REGISTRATION_CODES
                  << "reg [" << regCode << "] "
                #endif // GALACTICA_LOG_REGISTRATION_CODES
                  << "ip ["<< ipStr <<"] "
                  << "Login attempt for player ["<<p->playerName<<"] with bad password" 
                  << pdg::endlog;
               lrp->loginResult = login_ResultBadPassword;
            } else {
               newSlot = nameMatches;
            }
         }
         if (p->isNew() && (lrp->loginResult == login_ResultSuccess)) {
             // they are about to get in, check admit and ban list
            // first check for reg code
            if (regCode[0] != 0) {  // they are a registered user
               if (!Galactica::Globals::GetInstance().IsUserAllowed(regCode)) {
                  DEBUG_OUT("Login attempt for player ["<<p->playerName<<"] not admitted", DEBUG_ERROR | DEBUG_PACKETS);
                  mHostLog << pdg::log::category("BAN") << pdg::log::error
                   #ifdef GALACTICA_LOG_REGISTRATION_CODES
                     << "reg [" << regCode << "] "
                   #endif // GALACTICA_LOG_REGISTRATION_CODES
                     << "ip ["<< ipStr <<"] "
                     << "Login attempt for player ["<<p->playerName<<"] not admitted" 
                     << pdg::endlog;
                  lrp->loginResult = login_ResultNotAdmitted;  // but they were banned or not on the admit list
               }
            } else if (!Galactica::Globals::GetInstance().getAdmitUnreg() // no unregistered users allowed
               || (Galactica::Globals::GetInstance().haveAllowedUsersList()) ) { // or have admit list
               DEBUG_OUT("Login attempt for player ["<<p->playerName<<"] unregistered", DEBUG_ERROR | DEBUG_PACKETS);
               mHostLog << pdg::log::category("UNREG") << pdg::log::error
                #ifdef GALACTICA_LOG_REGISTRATION_CODES
                  << "reg [" << regCode << "] "
                #endif // GALACTICA_LOG_REGISTRATION_CODES
                  << "ip ["<< ipStr <<"] "
                  << "Login attempt for player ["<<p->playerName<<"] unregistered" 
                  << pdg::endlog;
               lrp->loginResult = login_ResultUnregistered; // they have to be registered to play
            }
         }

        // unregistered
        if (regCode[0] == 0) {
            std::strcpy(regCode, "unregistered");
        }
        
        int endpointNum = newSlot - 1;
        
        // check that no-one with this registration code is already logged in
        // unless we are playing a "hot seat" game, with everyone playing on same machine
        if ( (lrp->loginResult == login_ResultSuccess) && !playingHotSeat) {
            for (int i = 0; i < MAX_PLAYERS_CONNECTED; i++) {
                if (endpointNum != i) {
                    if (std::strncmp(regCode, mConnectedRegCodes[i], strLen_RegCode) == 0) {
                        // duplicate reg code found
                        DEBUG_OUT("Login attempt for player ["<<p->playerName<<"] with duplicate reg code ["
                                    << regCode<<"] already in use by player ["<<i+1<<"]", DEBUG_ERROR | DEBUG_PACKETS);
                        mHostLog << pdg::log::category("UNREG") << pdg::log::error
                        #ifdef GALACTICA_LOG_REGISTRATION_CODES
                          << "reg [" << regCode << "] "
                        #endif // GALACTICA_LOG_REGISTRATION_CODES
                          << "ip ["<< ipStr <<"] "
                          << "Login attempt for player ["<<p->playerName<<"] with duplicate reg code"
                          << pdg::endlog;
                        lrp->loginResult = login_ResultDuplicateLogin;
                        break;
                    }
                }
            }
        }

         if (lrp->loginResult == login_ResultSuccess) {
            // good login
            int playerNum = newSlot;
            DEBUG_OUT("Host found login slot "<< endpointNum, DEBUG_DETAIL | DEBUG_PACKETS);
            if (mPlayerEndpoints[endpointNum] != NULL) {
               DEBUG_OUT("Login for player ["<<p->playerName<<"] is duplicate, disconnecting old player", DEBUG_ERROR | DEBUG_PACKETS);
               mHostLog << pdg::log::category("DUP") << pdg::log::error
                #ifdef GALACTICA_LOG_REGISTRATION_CODES
                  << "reg [" << regCode << "] "
                #endif // GALACTICA_LOG_REGISTRATION_CODES
                  << "ip ["<< ipStr <<"] "
                  << "Login for player ["<<p->playerName<<"] is duplicate, disconnecting old player" 
                  << pdg::endlog;
               // player already connected to this slot
               // disconnect them (last login wins)
               DisconnectPlayer(playerNum, reason_DuplicateLogin); // this will mark their player info as logged out
            }
            
            // found an open slot, map endpoint to player
            lrp->playerNum = playerNum;
            mPlayerEndpoints[endpointNum] = fromEndpoint;
            DEBUG_OUT("Host mapped player endpoint " << fromEndpoint <<" to slot "<< endpointNum, DEBUG_DETAIL | DEBUG_PACKETS);
            // update the info for this player in the database
            GetPlayerInfo(playerNum, pInfo);
            pInfo.isLoggedIn = true;
            mAnyoneLoggedOn = true;
            if (p->isNew()) { // new login
               // player in now alive and slot is assigned
               pInfo.isAssigned = true;
               pInfo.isAlive = true;
               // save the player name
               std::strcpy(pInfo.name, p->playerName);
               // also save the password in the private info
               GetPrivatePlayerInfo(lrp->playerNum, privateInfo);
               std::strncpy(privateInfo.password, p->password, MAX_PASSWORD_LEN);
               SetPrivatePlayerInfo(lrp->playerNum, privateInfo);
            }
            // add the registration code into the list of connected reg codes
            std::strncpy(mConnectedRegCodes[endpointNum], regCode, strLen_RegCode);
            mConnectedRegCodes[endpointNum][strLen_RegCode] = 0;    // make sure it is nul terminated

            SetPlayerInfo(lrp->playerNum, pInfo);
            DEBUG_OUT("Host updated database entry for player " << (int)lrp->playerNum, DEBUG_DETAIL | DEBUG_DATABASE);
         }
      }
   }
   if (lrp->loginResult == login_ResultSuccess) {
      lrp->rejoinKey = mGameInfo.gameID;
   } else {
      lrp->rejoinKey = 0;
   }
   SendPacketTo(fromEndpoint, lrp);
   if (lrp->loginResult == login_ResultSuccess) {
      DEBUG_OUT("Host sent player "<<(int)lrp->playerNum << " login accepted to connection "
                << fromEndpoint << " with rejoin key ["<<lrp->rejoinKey
                <<"]", DEBUG_IMPORTANT | DEBUG_PACKETS);
      mHostLog << pdg::log::category("LOGIN") << pdg::log::inform
       #ifdef GALACTICA_LOG_REGISTRATION_CODES
         << "reg [" << regCode << "] "
       #endif // GALACTICA_LOG_REGISTRATION_CODES
         << "ip ["<< ipStr <<"] "
         << "Player ["<<(AS_NUMBER)lrp->playerNum<<"] ["<<p->playerName<<"] login successful" 
         << pdg::endlog;
      SaveAllPlayerInfo();
      BroadcastGameInfo();
      SendGameDataTo(lrp->playerNum);
   } else {
      DEBUG_OUT("Host sent login rejected to connection "
                << fromEndpoint, DEBUG_IMPORTANT | DEBUG_PACKETS);
   }
   ReleasePacket(lrp);
}

void
GalacticaHost::SendGameInfo(PEndpointRef inEndpoint) {
    GameInfoPacket* p = CreatePacket<GameInfoPacket>( mGameInfo.gameTitle[0]+1 );
    p->hostVersion = version_HostProtocol;
    p->currTurn = mGameInfo.currTurn;
    p->maxTimePerTurn = mGameInfo.maxTimePerTurn;
    p->secsRemainingInTurn = mAutoEndTurnTimer.GetTimeRemaining();
    p->totalNumPlayers = mGameInfo.totalNumPlayers;
    p->numPlayersRemaining = mGameInfo.numPlayersRemaining;
    p->numHumans = mGameInfo.numHumans;
    p->numComputers = mGameInfo.numComputers;
    p->numHumansRemaining = mGameInfo.numHumansRemaining;
    p->numComputersRemaining = mGameInfo.numComputersRemaining;
    p->compSkill = mGameInfo.compSkill;
    p->compAdvantage = mGameInfo.compAdvantage;
    p->sectorDensity = mGameInfo.sectorDensity;
    p->sectorSize = mGameInfo.sectorSize;
    p->omniscient = mGameInfo.omniscient;
    p->fastGame = mGameInfo.fastGame;
    p->endTurnEarly = mGameInfo.endTurnEarly;
    p->timeZone = mGameInfo.timeZone;
    p->gameID = mGameInfo.gameID;
    p->isFull = true; // start by assuming game is full
    for (int i = 1; i <= mGameInfo.totalNumPlayers; i++) {
        // now check to see if there are any unassigned slots
        PublicPlayerInfoT info;
        GetPlayerInfo(i, info);
        if (!info.isAssigned && !info.isComputer) {
            p->isFull = false;
            break;
        }
    }
    p2cstrcpy(p->gameTitle, mGameInfo.gameTitle);
    LogGameInfo();
    DEBUG_OUT("GameInfoPacket isFull = "<<(int)p->isFull, DEBUG_IMPORTANT);
    SendPacketTo(inEndpoint, p);
    ReleasePacket(p);
}

void
GalacticaHost::SendPlayerInfo(PEndpointRef inEndpoint, SInt32 inPlayerNum) {
    PlayerInfoPacket* p = CreatePacket<PlayerInfoPacket>();
    p->playerNum = inPlayerNum;
    p->info.name[0] = 0;
    GetPlayerInfo(inPlayerNum, p->info);
    DEBUG_OUT("Host Sending Player Info for "<< inPlayerNum << " ["<<p->info.name<<"]", DEBUG_DETAIL | DEBUG_PACKETS);
    SendPacketTo(inEndpoint, p);
    ReleasePacket(p);
}

void
GalacticaHost::BroadcastGameInfo() {
    // send game info update to all players
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (mConnections[i].HasEndpointRef()) {
            HandleGameInfoRequest(mConnections[i].GetEndpointRef(), true);  // as if they asked for it, except we also send player info
        }
    }
  #ifndef NO_MATCHMAKER_SUPPORT
    std::time_t currDT;
    // good time to send an update to the MatchMaker
    if (mMatchMakerGame) {
        GetInternalStatusString(mMatchMakerGame->mStatus, MM_MAX_STATUS_LEN);
        mMatchMakerGame->mPlayersCurrent = mGameInfo.numHumansRemaining;
        mMatchMakerGame->mCurrentTurn = mGameInfo.currTurn;
//      mMatchMaker->ping(mMatchMakerGame); ERZ, 8/31/05, were doing this twice, not sure why, remove it if no problems appear
        long err = mMatchMaker->ping(mMatchMakerGame);
        if ( err != noErr ) {
            DEBUG_OUT("Host::BroadcastGameInfo: MatchMaker Ping failed with error "<<err<<" -- posting game again", DEBUG_ERROR);
            // failure to ping may mean we took to long since last ping, try to recreate
            err = mMatchMaker->postGame(mMatchMakerGame, mMatchMakerGame->mRegCode);
            if (err != noErr) {
                // error means system is unavailable, avoid frequent timeout delays by stopping reporting
              #ifdef GALACTICA_SERVER
                DEBUG_OUT("Host::SpendTime: MatchMaker Post failed with error "<<err<<" -- delay ping for 1 hour", DEBUG_ERROR);
                // for a standalone server, it's critical we keep posting this, so just set a long delay
                mNextMatchMakerPingTime = (UInt32) std::time(&currDT) + 3600UL; // 3600 sec = 1 hour till next attempt
                return; // do nothing further
              #else
                DEBUG_OUT("Host::SpendTime: MatchMaker Post failed with error "<<err<<" -- halting post attempts", DEBUG_ERROR);
                delete mMatchMakerGame;
                mMatchMakerGame = NULL;
                delete mMatchMaker;
                mMatchMaker = NULL;
              #endif // !GALACTICA_SERVER
            }
        }
        mNextMatchMakerPingTime = (UInt32) std::time(&currDT) + kPingMatchMakerInterval;
    }
  #endif
}

void
GalacticaHost::HandleDisconnect(PEndpointRef inEndpoint) {
   SInt8 playerNum = EndpointToPlayerNum(inEndpoint);
   if (playerNum != kNoSuchPlayer) {
      int slotnum = playerNum-1;
      ASYNC_DEBUG_OUT("Host clearing slot "<< slotnum << " ["<<mPlayerEndpoints[slotnum]<<"]", DEBUG_DETAIL | DEBUG_PACKETS);
      // unmap the connection
      if ((playerNum>0) && (playerNum<=MAX_PLAYERS_CONNECTED)) {
         // player connection
         mPlayerEndpoints[playerNum-1] = NULL;
         PublicPlayerInfoT pInfo;
         GetPlayerInfo(playerNum, pInfo);
         pInfo.isLoggedIn = false;
         SetPlayerInfo(playerNum, pInfo);
         ASYNC_DEBUG_OUT("Host logged out Player "<< (int)playerNum, DEBUG_DETAIL | DEBUG_PACKETS | DEBUG_DATABASE);
         mHostLog << pdg::log::category("LOGIN") << pdg::log::inform
            << "Player [" <<(AS_NUMBER)playerNum<<"] logout" << pdg::endlog;
         // check to see if anyone is left
         mAnyoneLoggedOn = false;
         for (int i = 0; i < MAX_PLAYERS_CONNECTED; i++) {
            if (mPlayerEndpoints[i] != NULL) {
                mAnyoneLoggedOn = true; // we use this to know whether to short circuit broadcast commands
                break;
            }
         }
      } else if (playerNum == kAdminPlayerNum) {
         // admin connection
         mObservers[0] = NULL;
         ASYNC_DEBUG_OUT("Host logged out Administrator", DEBUG_DETAIL | DEBUG_PACKETS | DEBUG_DATABASE);
         mHostLog << pdg::log::category("LOGIN") << pdg::log::inform
            << "Adminstrator logout" << pdg::endlog;
      } else if (playerNum > MAX_PLAYERS_CONNECTED) {
         // observer connection
         int observerNum = playerNum - MAX_PLAYERS_CONNECTED;
         if ((observerNum > 0) && (observerNum < MAX_OBSERVERS_CONNECTED)) {
            mObservers[observerNum] = NULL;
            ASYNC_DEBUG_OUT("Host logged out Observer "<< (int)observerNum, DEBUG_DETAIL | DEBUG_PACKETS | DEBUG_DATABASE);
            mHostLog << pdg::log::category("LOGIN") << pdg::log::inform
               << "Observer [" <<(AS_NUMBER)observerNum<<"] logout" << pdg::endlog;
         }
      }
   }
   for (int i = 0; i < MAX_CONNECTIONS; i++) {
      if (mConnections[i].IsThisEndpoint(inEndpoint)) {
         mConnections[i].SetEndpointRef(NULL);
         ASYNC_DEBUG_OUT("Host unmapped endpoint " << inEndpoint << " at connection " << i, DEBUG_IMPORTANT | DEBUG_PACKETS);
//         break;
      }
   }
   if (playerNum != -1) {    // only rebroadcast game info if the disconnect was from a player
      BroadcastGameInfo();  // be sure to do this *after* we've unmapped the endpoint
   }
}

// this converts an endpoint ref into a logged in player's player number
SInt8
GalacticaHost::EndpointToPlayerNum(PEndpointRef inRemoteEndpoint) {
   for (int i = 0; i < MAX_PLAYERS_CONNECTED; i++) {
      if (mPlayerEndpoints[i] == inRemoteEndpoint) {
         return i+1;
      }
   }
   if (inRemoteEndpoint == mObservers[0]) {
      return kAdminPlayerNum;
   }
   for (int i = 1; i < MAX_OBSERVERS_CONNECTED; i++) {
      if (mObservers[i] == inRemoteEndpoint) {
         return i + MAX_PLAYERS_CONNECTED;
      }
   }
   return kNoSuchPlayer;
}

// this gets the endpoint ref for a logged in player's from the player number
PEndpointRef
GalacticaHost::PlayerNumToEndpoint(SInt8 inPlayerNum) {
   if (inPlayerNum == kAdminPlayerNum) {
      return mObservers[0];
   } else if (inPlayerNum > MAX_PLAYERS_CONNECTED) {
      int observerNum = inPlayerNum - MAX_PLAYERS_CONNECTED;
      if ((observerNum > 0) && (observerNum < MAX_OBSERVERS_CONNECTED)) {
         return mObservers[observerNum];
      } else {
         return NULL;
      }
   } else if (inPlayerNum > 0) {
      return mPlayerEndpoints[inPlayerNum - 1];
   } else {
      return NULL;
   }
}

int
GalacticaHost::PlayerNumToArrayIndex(SInt8 inPlayerNum) {
    if (inPlayerNum == kAdminPlayerNum) {
        return kNoSuchPlayer;
    } else if (inPlayerNum > MAX_PLAYERS_CONNECTED) {
        return kNoSuchPlayer;
    } else if (inPlayerNum < 1) {
        return kNoSuchPlayer;
    } else {
        return inPlayerNum - 1;
    }
}

// these packet handlers are for packets that must come from a logged in player/admin/observer
// with an assigned player number
void
GalacticaHost::SendPacketTo(SInt8 toPlayerNum, Packet* inPacket) {
    // send packet to a logged in client
    PEndpointRef endpoint = PlayerNumToEndpoint(toPlayerNum);
    if (!endpoint) {
        DEBUG_OUT("Host SendPacket "<<inPacket->GetName() << " to player "<<(int)toPlayerNum
                    << " failed, player not logged on", DEBUG_ERROR | DEBUG_PACKETS);
        return;
    } else {
        SendPacketTo(endpoint, inPacket);
        DEBUG_OUT("Host Sent Packet "<<inPacket->GetName() << " to player " 
                    << (int)toPlayerNum, DEBUG_DETAIL | DEBUG_PACKETS);
    }
}


void
GalacticaHost::HandlePlayerToPlayerMessage(SInt8 fromPlayer, Packet* inPacket) {
   PlayerToPlayerMessagePacket* p = static_cast<PlayerToPlayerMessagePacket*>(inPacket);
   int toPlayerNum = p->toPlayer;
   ASSERT_STR(fromPlayer == p->fromPlayer, "Possible attempt by "<<(int)fromPlayer
   <<" to forge a player-to-player message as coming from "<< (int)p->fromPlayer);
   if (fromPlayer != p->fromPlayer) {
      mHostLog << pdg::log::category("HACK") << pdg::log::error
               << "Possible attempt by "<<(AS_NUMBER)fromPlayer<<" to forge a player-to-player message "
               << "as coming from "<< (AS_NUMBER)p->fromPlayer << pdg::endlog;
   }
   p->fromPlayer = fromPlayer;  // prevents forgeries
   if (toPlayerNum == kAllPlayers) {
      mHostLog << pdg::log::category("YELL") << pdg::log::detail
               << "Player [" <<(AS_NUMBER)fromPlayer<<"] said [" << p->messageText <<"]" << pdg::endlog;
      // send to all players, except originator
      for (int i = 0; i < MAX_PLAYERS_CONNECTED; i++) {
         if ( (i+1 != fromPlayer) && (mPlayerEndpoints[i] != NULL) ) {
             SendPacketTo(mPlayerEndpoints[i], inPacket);
         }
      }
      // always copy to administrator if online
      if (PlayerNumToEndpoint(kAdminPlayerNum) != NULL) {
      	 SendPacketTo(kAdminPlayerNum, inPacket);
      }
   } else {
      // just send to one
      mHostLog << pdg::log::category("CHAT") << pdg::log::detail
               << "Player [" <<(AS_NUMBER)fromPlayer<<"] said [" << p->messageText <<"] to player [" 
               << (AS_NUMBER)toPlayerNum<<"]"<< pdg::endlog;
      if (PlayerNumToEndpoint(toPlayerNum) != NULL) {
         // player is online, send immediately
         SendPacketTo(toPlayerNum, p);
      } else {
         // player is offline, save as a message object, which they will get in their events at next login
         CMessage* msg = CMessage::CreateEvent(this, event_Message, toPlayerNum, nil, kNoOtherPlayers, 0, fromPlayer);
         if (msg) {
           #ifdef GALACTICA_SERVER
            msg->SetMessageText(p->messageText);
           #else
          #warning FIXME: remove this Mac Specific code
            int size = std::strlen(p->messageText);
            Handle h = NewHandle(size);
            HLock(h);
            std::memcpy(*h, p->messageText, size);
            HUnlock(h);
            msg->SetMessage(h, NULL);
           #endif // GALACTICA_SERVER
            msg->WriteToDataStore();
         }
      }
   }
}

void
GalacticaHost::SendThingyTo(SInt8 toPlayerNum, AGalacticThingy* inThingy, EAction inAction) {
   PEndpointRef endpoint = PlayerNumToEndpoint(toPlayerNum);
   if (!endpoint) {
      DEBUG_OUT("Host SendThingy "<< inThingy << " to player "<<(int)toPlayerNum
                 << " failed, player not logged on", DEBUG_ERROR | DEBUG_PACKETS);
      return;
   } else {
      ThingyDataPacket* p = inThingy->WriteIntoPacket(toPlayerNum);
      if (p) {
         p->action = inAction;
         SendPacketTo(endpoint, p);
         ReleasePacket(p);
         DEBUG_OUT("Host Sent thingy "<< inThingy << " to player " 
                     << (int)toPlayerNum, DEBUG_TRIVIA | DEBUG_PACKETS);
      }
   }
}

void
GalacticaHost::HandleThingyData(SInt8 fromPlayer, ThingyDataPacket* inPacket) {
   DEBUG_OUT("Host HandleThingyData from player "<<(int)fromPlayer, DEBUG_IMPORTANT | DEBUG_PACKETS);
   AGalacticThingy* aThingy = FindThingByID(inPacket->id);
   // Ignore anything that doesn't already exist on the host
   if (!aThingy) {
      DEBUG_OUT("Host HandleThingyData ignored, unknown ID "<<inPacket->id, DEBUG_ERROR | DEBUG_PACKETS);
      return;
   }
   // Ignore anything from a client who doesn't own it
   if (aThingy->GetOwner() != fromPlayer) {
      DEBUG_OUT("Host HandleThingyData ignored changes to "<< aThingy <<" from enemy player "
                 <<(int)fromPlayer, DEBUG_ERROR | DEBUG_PACKETS);
      return;
   }
   // CMessage objects should never be sent by the client to the host
   if ( IS_ANY_MESSAGE_TYPE(inPacket->type) ) {
      DEBUG_OUT("Host HandleThingyData ignored illegal message object id="<<inPacket->id <<" from "
                 <<(int)fromPlayer, DEBUG_ERROR | DEBUG_PACKETS);
      return;
   }
   if (inPacket->action == action_Updated) {
      // this is a valid update, read the thingy data out of the packet
      StartStreaming();
      if (aThingy->ReadFromPacket(inPacket)) {
        aThingy->WriteToDataStore();    // save the changes so if we are disconnected we don't have to make them again
        aThingy->Changed(); // so other players know it was changed
      } else {
        DEBUG_OUT("Bad Read of "<< aThingy<<" from Packet, attempting to restore from DB", DEBUG_ERROR);
        aThingy->ReadFromDataStore(inPacket->id);
      }
      StopStreaming();
   } else {
      DEBUG_OUT("Host HandleThingyData ignored illegal action "<<(int)inPacket->action
                 <<" from player "<<(int)fromPlayer, DEBUG_ERROR | DEBUG_PACKETS);
   }
}

void
GalacticaHost::HandleTurnComplete(SInt8 fromPlayer, TurnCompletePacket* inPacket) {
   DEBUG_OUT("GalacticaHost::HandleTurnComplete from player "<<(int)fromPlayer, DEBUG_IMPORTANT | DEBUG_PACKETS);
   mHostLog << pdg::log::category("EOT") << pdg::log::inform
      << "Player ["<<(AS_NUMBER)fromPlayer<<"] posted turn ["<<(AS_NUMBER)inPacket->turnNum<<"]" << pdg::endlog;
   if (inPacket->turnNum != mGameInfo.currTurn) {
      DEBUG_OUT("Something really screwy going on, turn complete = "<< inPacket->turnNum
                 << ", curr turn = "<<mGameInfo.currTurn<<" disconnecting player "
                 <<(int)fromPlayer, DEBUG_ERROR | DEBUG_PACKETS);
      DisconnectPlayer(fromPlayer, reason_OutOfSync);
      return;        
   }
   PrivatePlayerInfoT info;
   GetPrivatePlayerInfo(fromPlayer, info);
   info.lastTurnPosted = inPacket->turnNum;
   SetPrivatePlayerInfo(fromPlayer, info);
   mNextSpendTime = 0;      // check immediately
   StartRepeating();
}


void
GalacticaHost::HandleCreateThingy(SInt8 fromPlayer, CreateThingyRequestPacket* inPacket) {
   DEBUG_OUT("Host HandleCreateThingy", DEBUG_IMPORTANT | DEBUG_PACKETS);
   if ( ((inPacket->type == (UInt32)thingyType_Rendezvous) || (inPacket->type == (UInt32)thingyType_Fleet))
        && (fromPlayer > 0) && (fromPlayer <= MAX_PLAYERS_CONNECTED) ) {
      // make sure the game reference is set correctly.
      inPacket->where.GetThingyRef().SetGame(this);
      AGalacticThingy* it = MakeThingy(inPacket->type);
      // while it's possible for MakeThingyFromSubClassType to return NULL, it should be
      // impossible given that we know we are passing in either a rendezvous or a fleet as
      // the thingy type to make
      ASSERT(it != NULL);
      it->SetOwner(fromPlayer);
      DEBUG_OUT("Host about to set position of "<<it<<" for client "<<(int)fromPlayer<<" to "
                 << inPacket->where, DEBUG_DETAIL | DEBUG_PACKETS);
      it->SetPosition(inPacket->where, kDontRefresh);
      it->WriteToDataStore();  // this will assign a legitimate, permanent id
      AGalacticThingy* containerThingy = inPacket->where.GetThingy();
      if (containerThingy) {
            // now we add the new item (usually a fleet) into it's location (should be a star or fleet)
            containerThingy->AddContent(it, contents_Any, kDontRefresh);
      }
      CreateThingyResponsePacket* ctrp = CreatePacket<CreateThingyResponsePacket>();
      ctrp->tempID = inPacket->tempID; // the temporary ID the client knows this as
      ctrp->newID = it->GetID();   // the new id that the item has now been assigned
      SendPacketTo(fromPlayer, ctrp);
      DEBUG_OUT("Host created "<<it<<" for client "<<(int)fromPlayer<<" (temp id was "<<inPacket->tempID 
                 <<")", DEBUG_DETAIL | DEBUG_PACKETS);
      if (inPacket->type == (UInt32)thingyType_Fleet) {
            // for fleets, save the response packet to be sent later, because we will still need it.
            int idx = fromPlayer - 1;
            if (mLastFleetCreateResponse[idx]) {
                // don't let these packets leak
                ReleasePacket(mLastFleetCreateResponse[idx]);
                mLastFleetCreateResponse[idx] = 0;
            }
            mLastFleetCreateResponse[idx] = ctrp;
      } else {
            // for non-fleets, release the packet immediately
          ReleasePacket(ctrp);
      }
   } else {
      mHostLog << pdg::log::category("HACK") << pdg::log::error
               << "Possible attempt by "<<(AS_NUMBER)fromPlayer<<" to illegally create a thingy of type [ "
               << LongTo4CharStr(inPacket->type) << "]" << pdg::endlog;
      DEBUG_OUT("Host ignoring illegal create thingy request of type "<<inPacket->type << " for player "
                 << (int) fromPlayer, DEBUG_ERROR | DEBUG_PACKETS);
   }
}

void
GalacticaHost::HandleReassignThingy(SInt8 fromPlayer, ReassignThingyRequestPacket* inPacket) {
    DEBUG_OUT("Host HandleReassignThingy", DEBUG_IMPORTANT | DEBUG_PACKETS);
    if ( ((inPacket->type == (UInt32)thingyType_Ship) || (inPacket->type == (UInt32)thingyType_Fleet))
        && (fromPlayer > 0) && (fromPlayer <= MAX_PLAYERS_CONNECTED) ) {
        // get the last create packet if there is one, and update the 
        // inPacket id to match the newly assigned id
        int idx = fromPlayer - 1;
        if (mLastFleetCreateResponse[idx]) {
            if (inPacket->newContainer == mLastFleetCreateResponse[idx]->tempID) {
                DEBUG_OUT("Found paired Create/ReassignThingyRequests for id "<< inPacket->newContainer << "; real id is "
                            << mLastFleetCreateResponse[idx]->newID, DEBUG_DETAIL | DEBUG_PACKETS);
                inPacket->newContainer = mLastFleetCreateResponse[idx]->newID;
            }
            // free the old packet to prevent a leak
            ReleasePacket(mLastFleetCreateResponse[idx]);
            mLastFleetCreateResponse[idx] = 0;
        }
        // locate the item be reassigned, and its new and old containers
        AGalacticThingy* it = FindThingByID(inPacket->id);
        AGalacticThingy* dst = FindThingByID(inPacket->newContainer);
        if (it && dst         // ensure that:
          && ((UInt32)it->GetThingySubClassType() == inPacket->type) // item is a fleet or star and is of type claimed
          && ( (dst->GetThingySubClassType() == thingyType_Fleet)  // new container is a fleet or star
            || (dst->GetThingySubClassType() == thingyType_Star) ) ) {
            // we've done enough validation that this is at least worthy of a response
            ReassignThingyResponsePacket* rtrp = CreatePacket<ReassignThingyResponsePacket>();
            rtrp->id = inPacket->id;    // we use the new id, because that's what the client will have now
            if (it->GetCoordinates() == dst->GetCoordinates()) {   // ensure new loc is at same coord as current loc
		        if (dst->AddContent(it, contents_Ships, kDontRefresh)) {	// add the ship to the fleet
                    AGalacticThingy* src = FindThingByID(inPacket->oldContainer);
                    if (src) {
                        // make sure thingy is not still contained in old container
                        src->RemoveContent(it, contents_Ships, kDontRefresh);
                    }
                    rtrp->success = true;
                
                    DEBUG_OUT("Host reassigned "<<it<<" for client "<<(int)fromPlayer, 
                                DEBUG_DETAIL | DEBUG_PACKETS | DEBUG_CONTAINMENT);
                } else {
                    // couldn't move because of an inexplicable host error, return failure
                    rtrp->success = false;
                    DEBUG_OUT("Host unable to complete valid reassign thingy request for "
                        << it << " (dst "<<dst<<") for player " << (int) fromPlayer, 
                        DEBUG_ERROR | DEBUG_PACKETS | DEBUG_CONTAINMENT);
                }
            } else {
                // couldn't move because of coordinate conflict, return failure
                rtrp->success = false;
                mHostLog << pdg::log::category("HACK") << pdg::log::error
                         << "Possible attempt by "<<(AS_NUMBER)fromPlayer<<" to illegally relocate ship or fleet"
                         << pdg::endlog;
                DEBUG_OUT("Host rejecting bad reassign thingy request for "<< it << " (dst "<<dst<<") for player "
                        << (int) fromPlayer, DEBUG_ERROR | DEBUG_PACKETS | DEBUG_CONTAINMENT);
            }
            SendPacketTo(fromPlayer, rtrp);
            ReleasePacket(rtrp);
        } else {
            // the data in the request didn't match the data on the host, client is out of synch or hacking
            DEBUG_OUT("Host ignoring bad reassign thingy request of type "<<inPacket->type << " for player "
                    << (int) fromPlayer, DEBUG_ERROR | DEBUG_PACKETS | DEBUG_CONTAINMENT);
            mHostLog << pdg::log::category("HACK") << pdg::log::error
                       << "Possible attempt by "<<(AS_NUMBER)fromPlayer<<" to send corrupt reassign thingy packet. "
                       << "Dumping them from game in case their client is out of sync."<< pdg::endlog;
            DisconnectPlayer(fromPlayer, reason_BadData);   // drop this player, they will get good data when
                                                            // they reconnect
        }
        // relocate the item, and return a success packet
    } else {
        mHostLog << pdg::log::category("HACK") << pdg::log::error
                 << "Possible attempt by "<<(AS_NUMBER)fromPlayer<<" to send illegal relocate packet for "
                 << "a thingy of type ["<< LongTo4CharStr(inPacket->type) << "]" << pdg::endlog;
        DEBUG_OUT("Host ignoring illegal reassign thingy request of type "<<inPacket->type << " for player "
                   << (int) fromPlayer, DEBUG_ERROR | DEBUG_PACKETS | DEBUG_CONTAINMENT);
    }
}

void    
GalacticaHost::HandleClientSettings(SInt8 playerNum, ClientSettingsPacket* inPacket) {
	DEBUG_OUT("Host HandleClientSettings", DEBUG_IMPORTANT | DEBUG_PACKETS);
	// load private player info for player
	PrivatePlayerInfoT thePrivateInfo;
	GetPrivatePlayerInfo(playerNum, thePrivateInfo);
	// update with client settings
	bool badData = false;
	if ((inPacket->defaultBuildType < 1) || (inPacket->defaultBuildType > NUM_SHIP_HULL_TYPES)) {
        DEBUG_OUT("Host found bad client setting packet with bad hull type ["<< inPacket->defaultBuildType
                <<"] for player " << (AS_NUMBER) playerNum, DEBUG_ERROR | DEBUG_PACKETS | DEBUG_CONTAINMENT);
		badData = true;
	}
	if ((inPacket->defaultGrowth > 1000) || (inPacket->defaultShips > 1000) || (inPacket->defaultTech > 1000)) {
        DEBUG_OUT("Host found bad client setting packet with slider > 1000 for player "
                << (AS_NUMBER) playerNum, DEBUG_ERROR | DEBUG_PACKETS | DEBUG_CONTAINMENT);
		badData = true;
	}
	UInt32 sum = inPacket->defaultGrowth + inPacket->defaultShips + inPacket->defaultTech;
	if (sum > 1000) {
        DEBUG_OUT("Host found bad client setting packet sum ["<<sum<<"] != 1000 for player "
                << (AS_NUMBER) playerNum, DEBUG_ERROR | DEBUG_PACKETS | DEBUG_CONTAINMENT);
		badData = true;
	} else {
		// the sum might be off by a little bit. Fix it if it is
		long toDistrib = 1000 - sum;
		if ((toDistrib + inPacket->defaultGrowth) <= 1000) {
			inPacket->defaultGrowth += toDistrib;
		} else if ((toDistrib + inPacket->defaultTech) <= 1000) {
			inPacket->defaultTech += toDistrib;
		} else {
			inPacket->defaultShips += toDistrib;
		}
	}
	if (!badData) {
		thePrivateInfo.defaultBuildType = inPacket->defaultBuildType;
		thePrivateInfo.defaultGrowth = inPacket->defaultGrowth;
		thePrivateInfo.defaultTech = inPacket->defaultTech;
		thePrivateInfo.defaultShips = inPacket->defaultShips;
		// save private player info
		SetPrivatePlayerInfo(playerNum, thePrivateInfo);
	} else {
        // the data in the client settings was invalid, client is buggy or hacking
        DEBUG_OUT("Host ignoring bad client setting packet for player "
                << (AS_NUMBER) playerNum, DEBUG_ERROR | DEBUG_PACKETS | DEBUG_CONTAINMENT);
        mHostLog << pdg::log::category("HACK") << pdg::log::error
                   << "Possible attempt by "<<(AS_NUMBER)playerNum<<" to send corrupt client settings packet."
                   << pdg::endlog;
	}
}

void
GalacticaHost::HandleAdminSetGameInfo(AdminSetGameInfoPacket* inPacket) {
   DEBUG_OUT("Admin changing Game Info", DEBUG_IMPORTANT | DEBUG_PACKETS);
   mHostLog << pdg::log::category("ADMIN") << pdg::log::inform
      << "Admin changing game info" << pdg::endlog;
   mGameInfo.compSkill = inPacket->compSkill;
   mGameInfo.compAdvantage = inPacket->compAdvantage;
   mGameInfo.fastGame = inPacket->fastGame;
   mGameInfo.omniscient = inPacket->omniscient;
   mGameInfo.endTurnEarly = inPacket->endTurnEarly;
   UpdateMaxTimePerTurn(inPacket->maxTimePerTurn);
   SaveGameInfo();
   BroadcastGameInfo();
}

void
GalacticaHost::HandleAdminSetPlayerInfo(AdminSetPlayerInfoPacket* inPacket) {
   DEBUG_OUT("Admin changing Player Info", DEBUG_IMPORTANT | DEBUG_PACKETS);
   SInt8 playerNum = inPacket->playerNum;
   mHostLog << pdg::log::category("ADMIN") << pdg::log::inform
      << "Admin changing player info for player [" <<(AS_NUMBER)playerNum<<"]" << pdg::endlog;

   PublicPlayerInfoT  oldInfo;
   PrivatePlayerInfoT oldPrivateInfo;
   GetPlayerInfo(playerNum, oldInfo);
   GetPrivatePlayerInfo(playerNum, oldPrivateInfo);
   
   bool save = false;
   if (inPacket->info.isComputer && !oldInfo.isComputer) {
      // changing from human to computer
      oldInfo.isComputer = true;
      oldInfo.isAssigned = false;
      oldInfo.isLoggedIn = false;
      if (!oldInfo.isAssigned) {  // humans who haven't been assigned aren't tagged as alive yet
         oldInfo.isAlive = true;  // so mark this new computer as alive
         std::strcpy(oldInfo.name, inPacket->info.name); // change the name, too, there was no name assigned
      }
      mGameInfo.numHumans--;
      mGameInfo.numComputers++;
      if (oldInfo.isAlive) { 
         mGameInfo.numHumansRemaining--;
         mGameInfo.numComputersRemaining++;
      }
      save = true;
      SaveGameInfo();
   } else if (!inPacket->info.isComputer && oldInfo.isComputer) {
      // changing from computer to human
      oldInfo.isAssigned = false;   // not assigned to a particular human yet
      oldInfo.isLoggedIn = false;
      oldInfo.isComputer = false;
      mGameInfo.numHumans++;
      mGameInfo.numComputers--;
      if (oldInfo.isAlive) {
         mGameInfo.numHumansRemaining++;
         mGameInfo.numComputersRemaining--;
      }
      save = true;
      SaveGameInfo();
   } else if (!inPacket->info.isAssigned && oldInfo.isAssigned) {
      // freeing a human slot
      oldInfo.isAssigned = false;
      save = true;
   } else if (std::strcmp(inPacket->info.name, oldInfo.name) != 0) {
      // changing name
      std::strcpy(oldInfo.name, inPacket->info.name); // change the name
      save = true;
   } else if (!inPacket->info.isAlive && oldInfo.isAlive) {
      // kill off a player
      // go through and reassign all of their territories to be neutral
      LArrayIterator iterator(mEverything, LArrayIterator::from_Start); // Search list of all thingys
      AGalacticThingy* aThingy;
      while (iterator.Next(&aThingy)) {
        #ifdef DEBUG
         aThingy = ValidateThingy(aThingy);
         ASSERT(aThingy != nil);
         if (!aThingy)
            continue;
        #endif
         if (aThingy->GetOwner() == playerNum) {  // if it belongs to them, make it neutral
             aThingy->SetOwner(0);
             aThingy->Changed(); // make sure we flag it as having changed.
             SendThingyTo(kAdminPlayerNum, aThingy, action_Updated); // immediate feedback to admin
         }
      }
      // now that we've sent all the thingys, send the turn complete packet
      // so that the client will know to process the updates
      TurnCompletePacket* p = CreatePacket<TurnCompletePacket>();
      p->turnNum = mGameInfo.currTurn;
      SendPacketTo(kAdminPlayerNum, p);
      ReleasePacket(p);
      // we let the end of turn processing detect that they are dead
   }
#warning FEATURE: change this when we allow comp AI levels to be set separately
/* can't really do this yet because there is no UI to provide input info or display it back to user  
   if (oldInfo.isComputer) {
      // update skill and advantage
      oldPrivateInfo.compSkill = inPacket->compSkill;
      oldPrivateInfo.compAdvantage = inPacket->compAdvantage;
      SetPrivatePlayerInfo(inPacket->playerNum, oldPrivateInfo);
   } */
   if (save) {
      SetPlayerInfo(playerNum, oldInfo);
      SaveAllPlayerInfo();
      BroadcastGameInfo();
   }
}

void
GalacticaHost::HandleAdminEndTurnNow() {
   DEBUG_OUT("Admin ending turn now", DEBUG_IMPORTANT | DEBUG_PACKETS);
   mHostLog << pdg::log::category("ADMIN") << pdg::log::inform
      << "Admin ending turn now" << pdg::endlog;
   DoEndTurn();
}

void
GalacticaHost::HandleAdminPauseGame() {
   DEBUG_OUT("Admin pausing game", DEBUG_IMPORTANT | DEBUG_PACKETS);
   mHostLog << pdg::log::category("ADMIN") << pdg::log::inform
      << "Admin pausing game" << pdg::endlog;
}

void
GalacticaHost::HandleAdminResumeGame() {
   DEBUG_OUT("Admin resuming game", DEBUG_IMPORTANT | DEBUG_PACKETS);
   mHostLog << pdg::log::category("ADMIN") << pdg::log::inform
      << "Admin resuming game" << pdg::endlog;
}

void
GalacticaHost::HandleAdminEjectPlayer(AdminEjectPlayerPacket* inPacket) {
   DEBUG_OUT("Admin ejecting player", DEBUG_IMPORTANT | DEBUG_PACKETS);
   mHostLog << pdg::log::category("ADMIN") << pdg::log::inform
      << "Admin ejecting player" << pdg::endlog;
   DisconnectPlayer(inPacket->playerNum, reason_AdminEject);
}

void
GalacticaHost::BroadcastThingy(AGalacticThingy* inThingy, EAction inAction) {
    if (mAnyoneLoggedOn) {
        ThingyDataPacket* p;
        // send to all players, except originator
        for (int i = 0; i < MAX_PLAYERS_CONNECTED; i++) {
            if (mPlayerEndpoints[i] != NULL) {
                SInt8 dstPlayerNum = EndpointToPlayerNum(mPlayerEndpoints[i]);
                p = inThingy->WriteIntoPacket(dstPlayerNum);
                p->action = inAction;
                SendPacketTo(mPlayerEndpoints[i], p);
                ReleasePacket(p);
            }
        }
        // always copy to administrator
        if (AdministratorIsConnected()) {
            p = inThingy->WriteIntoPacket(kAdminPlayerNum);
            p->action = inAction;
            SendPacketTo(kAdminPlayerNum, p);
            ReleasePacket(p);
        }
    }
}

void
GalacticaHost::SendGameDataTo(SInt8 toPlayerNum) {
   DEBUG_OUT("GalacticaHost::SendGameDataTo "<<(int)toPlayerNum, DEBUG_IMPORTANT | DEBUG_EOT | DEBUG_PACKETS);
   // start by sending their private player info
   PrivatePlayerInfoPacket* pip = CreatePacket<PrivatePlayerInfoPacket>();
   pip->playerNum = toPlayerNum;
   GetPrivatePlayerInfo(toPlayerNum, pip->info);
   SendPacketTo(toPlayerNum, pip);
   ReleasePacket(pip);
   // sent the message count for the number of messages we are about to send
   ThingyCountPacket* tcp = CreatePacket<ThingyCountPacket>();
   tcp->thingyCount = mEverything.GetCount();
   SendPacketTo(toPlayerNum, tcp);
   ReleasePacket(tcp);
    // then send the everything list
   LArrayIterator iterator(mEverything, LArrayIterator::from_Start); // Search list of all thingys
   AGalacticThingy* aThingy;
   while (iterator.Next(&aThingy)) {
     #ifdef DEBUG
      aThingy = ValidateThingy(aThingy);
      ASSERT(aThingy != nil);
      if (!aThingy)
         continue;
     #endif
      if (!aThingy->IsDead()) {  // don't send dead thingys to clients
          SendThingyTo(toPlayerNum, aThingy, action_Added);
      }
      GalacticaApp::YieldTimeToForegoundTask();
   }
   // now that we've sent all the thingys, send the turn complete packet
   TurnCompletePacket* p = CreatePacket<TurnCompletePacket>();
   p->turnNum = mGameInfo.currTurn;
   SendPacketTo(toPlayerNum, p);
   ReleasePacket(p);
   DEBUG_OUT("done GalacticaHost::SendGameDataTo "<<(int)toPlayerNum, DEBUG_IMPORTANT | DEBUG_EOT | DEBUG_PACKETS);
}

void
GalacticaHost::DisconnectPlayer(SInt8 inPlayerNum, SInt8 inReason) {

    PEndpointRef endpoint = PlayerNumToEndpoint(inPlayerNum);
    if (endpoint) {

        // send a host disconnect packet just before we drop the connection
        HostDisconnectPacket* p = CreatePacket<HostDisconnectPacket>();
        p->reasonCode = inReason;
        SendPacketTo(inPlayerNum, p);
        ReleasePacket(p);
    
        DEBUG_OUT("GalacticaHost::DisconnectPlayer " << (int)inPlayerNum << " for reason "
                << (int)inReason, DEBUG_IMPORTANT | DEBUG_USER | DEBUG_PACKETS);
        NMErr err = ProtocolCloseEndpoint(endpoint, true); // do an orderly shutdown of connection
        if (err != noErr) {
            DEBUG_OUT("OpenPlay ProtocolCloseEndpoint() returned "<<err, DEBUG_ERROR | DEBUG_PACKETS);
        }
        HandleDisconnect(endpoint);
    }
   
    // remove the registration code info too
    int idx = PlayerNumToArrayIndex(inPlayerNum);
    if (idx != kNoSuchPlayer) {
        mConnectedRegCodes[idx][0] = 0;
    }
}



