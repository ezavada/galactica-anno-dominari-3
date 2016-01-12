//   GalacticaHost.h      
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved.
// ===========================================================================

#ifndef GALACTICA_HOST_H_INCLUDED
#define GALACTICA_HOST_H_INCLUDED

#include "GalacticaUtils.h" // for CTimed
#include <LPeriodical.h>
#include "GalacticaProcessor.h"

#include "CEndpoint.h"   // for CEndpoint and OpenPlay stuff
#include "pdg/sys/log.h"  // for new logging code

class AMasterIndex;
class ADataStore;

#ifndef NO_MATCHMAKER_SUPPORT
  class MatchMakerClient;
  class MMGameRec;
#endif

// total number of connections of any kind we can have before refusing new connections
#define MAX_CONNECTIONS 128
typedef char RegCodeStrT[strLen_RegCode+1];

class GalacticaHost : public LPeriodical, public CTimed, public GalacticaProcessor {
public:
   static void    HostCallback(PEndpointRef inEndpoint, void* inContext,
                           NMCallbackCode inCode, NMErr inError, void* inCookie);

   static void    HostStatusCallback(PEndpointRef inEndpoint, void* inContext,
                           NMCallbackCode inCode, NMErr inError, void* inCookie);

                  GalacticaHost();      // constructor
                  ~GalacticaHost();   // stub destructor

   void           LoadAllPlayerInfo();
   void           SaveAllPlayerInfo();
   void           BackupDatabaseFiles();
   
   bool           LoadGameInfo();   //return true if game has a host
   void           SaveGameInfo(bool hasHost = true);
   void           UpdateMaxTimePerTurn(UInt32 inNewMaxTimePerTurn);

   virtual bool   IsModified() {return false;}   // host keeps datastore current
   bool           IsTurnComplete();
   virtual void   SpendTime(const EventRecord &inMacEvent);
   virtual void   TimeIsUp();   // overriden to do nothing when time expires
   void           FinishTurn();
   void           ReadGameData();
   void           ResumeTurnClock(); // start clock on new turn
   void           InitNewGameData(NewGameInfoT &inGameInfo);
   void           DoCreateNew(FSSpec &dataSpec, FSSpec &indexSpec, NewGameInfoT &inGameInfo);
   void           WriteChangesToHost();
   virtual void   PrepareForEndOfTurn();
   virtual void   DoEndTurn();
   void           GetInternalStatusString(char* buffer, size_t bufferSize);

// Server initialization and shutdown
   void           InitializeServer(const char* inAddress = NULL, int inPort = 0); // setup for true network server
   void           ShutdownServer(); // shutdown network server

// Database methods
   void           CloseHostFiles();
   ADataStore*    GetDataStore() const {return mGameDataFile;}
   virtual void   OpenHost();
   virtual void   SpecifyHost(FSSpec &inDataSpec, FSSpec &inIndexSpec);

   // generic packet transmission and receipt
   void    HandleReceiveData(PEndpointRef inEndpoint);
   void    MapEndpoint(PEndpointRef inEndpoint);
   void    HandleFlowClear(PEndpointRef inEndpoint);
   void    HandlePacketFrom(PEndpointRef fromRemoteEndpoint, Packet* inPacket);
   void    SendPacketTo(PEndpointRef toRemoteEndpoint, Packet* inPacket);
   void    BroadcastPacket(Packet* inPacket, bool inToBrowsersAlso = false); // sends to all logged in players

   // these packet handlers are for packets that don't come from a logged in player with an assigned
   // player number
   void    HandleConnectRequest(PEndpointRef fromRemoteEndpoint);
   void    HandleGameInfoRequest(PEndpointRef fromEndpoint, bool inAlsoSendPlayerInfo = false);
   void    HandleLoginRequest(PEndpointRef fromEndpoint, LoginRequestPacket* inPacket);
   void    HandleDisconnect(PEndpointRef inEndpoint);
   void    SendGameInfo(PEndpointRef inEndpoint);
   void    SendPlayerInfo(PEndpointRef inEndpoint, SInt32 inPlayerNum);
   void    BroadcastGameInfo();
   void    HandlePing(PEndpointRef fromEndpoint, PingPacket* inPacket);

   // this converts an endpoint ref into a logged in player's player number
   SInt8           EndpointToPlayerNum(PEndpointRef inRemoteEndpoint);
   PEndpointRef    PlayerNumToEndpoint(SInt8 inPlayerNum);
   int             PlayerNumToArrayIndex(SInt8 inPlayerNum);

   // these packet handlers are for packets that must come from logged in players
   void    SendPacketTo(SInt8 toPlayerNum, Packet* inPacket);
   void    HandlePlayerToPlayerMessage(SInt8 fromPlayer, Packet* inPacket);
   void    SendThingyTo(SInt8 toPlayerNum, AGalacticThingy* inThingy, EAction inAction);
   void    HandleThingyData(SInt8 fromPlayer, ThingyDataPacket* inPacket);
   void    HandleTurnComplete(SInt8 fromPlayer, TurnCompletePacket* inPacket);
   void    HandleCreateThingy(SInt8 fromPlayer, CreateThingyRequestPacket* inPacket);
   void    HandleReassignThingy(SInt8 playerNum, ReassignThingyRequestPacket* inPacket);
   void    HandleClientSettings(SInt8 playerNum, ClientSettingsPacket* inPacket);

   // administrator packet handlers
   bool    AdministratorIsConnected() { return mObservers[0] != NULL; }
   void    HandleAdminSetGameInfo(AdminSetGameInfoPacket* inPacket);
   void    HandleAdminSetPlayerInfo(AdminSetPlayerInfoPacket* inPacket);
   void    HandleAdminEndTurnNow();
   void    HandleAdminPauseGame();
   void    HandleAdminResumeGame();
   void    HandleAdminEjectPlayer(AdminEjectPlayerPacket* inPacket);

   void    BroadcastThingy(AGalacticThingy* inThingy, EAction inAction);
   void    SendGameDataTo(SInt8 toPlayerNum);

   void    DisconnectPlayer(SInt8 inPlayerNum, SInt8 inReason);

protected:
   bool              mQuitPending;
   UInt32            mNextSpendTime;
   CTimer            mAutoEndTurnTimer;

   ADataStore*       mGameDataFile;
   AMasterIndex*     mGameIndexFile;
   FSSpec            mGameDataSpec;
   FSSpec            mGameIndexSpec;
   PEndpointRef      mEndpoint;
   PEndpointRef      mStatusEndpoint;
   bool              mDumpStatus;
   bool              mAnyoneLoggedOn;
   PEndpointRef      mPlayerEndpoints[MAX_PLAYERS_CONNECTED];
   PEndpointRef      mObservers[MAX_OBSERVERS_CONNECTED];
   CEndpoint         mConnections[MAX_CONNECTIONS];
   CreateThingyResponsePacket* mLastFleetCreateResponse[MAX_PLAYERS_CONNECTED];
   RegCodeStrT       mConnectedRegCodes[MAX_PLAYERS_CONNECTED];
  #ifdef APPLETALK_SUPPORT
   PEndpointRef      mEndpointAppletalk;
  #endif
  #ifndef NO_MATCHMAKER_SUPPORT
   MatchMakerClient* mMatchMaker;
   MMGameRec*        mMatchMakerGame;
   UInt32            mNextMatchMakerPingTime;
  #endif
   pdg::log          mHostLog;
   char              mHostIPStr[256];
};

#endif // GALACTICA_HOST_H_INCLUDED


