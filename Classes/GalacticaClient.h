//	GalacticaClient.h		
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#ifndef GALACTICA_CLIENT_H_INCLUDED
#define GALACTICA_CLIENT_H_INCLUDED

#include "GalacticaDoc.h"
#include "OpenPlay.h"
#include "GalacticaPackets.h"
#include "CMessage.h"  // for kAllPlayers
#include "LThermometerPane.h"
#include "pdg/sys/mutex.h"

#ifndef NO_MATCHMAKER_SUPPORT
  #include "MatchMakerClient.h"
#endif // NO_MATCHMAKER_SUPPORT

#include <queue>

enum GameType {
    gameType_LAN        = 0,
    gameType_Internet   = 1,
    gameType_GameRanger = 2,
    gameType_AppleTalk  = 3,
    gameType_MatchMaker = 4
};

enum {
    key_NotRejoining    = 0,
    key_ForceRejoin     = -1,
    gameID_Unknown      = 0
};

struct GameJoinInfo {
    char        gamename[MAX_GAME_NAME_LEN+1];
    char        hostname[MAX_HOSTNAME_LEN+1];
    char        playername[MAX_PLAYER_NAME_LEN+1];
    char        password[MAX_PASSWORD_LEN+1];
    UInt32      gameid;
    NMHostID    hostid;
    UInt32      key;
    UInt16      port;
    GameType    type;
    bool        valid; // true if there was an id match
    GameJoinInfo(const char* inGameName, const char* inHostName, const char* inPlayerName, 
                const char* inPassword, UInt32 inGameID, 
                NMHostID inOpenPlayHostID, UInt32 inRejoinKey, 
                GameType inGameType, UInt16 inPort = 0);
    GameJoinInfo();
    void    Clear();
};

class CJoinGameDlgHndlr;
class CChatHandler;

class GalacticaClient : public GalacticaDoc {
public:
						GalacticaClient(LCommander *inSuper);		// constructor
	virtual			    ~GalacticaClient();	// stub destructor

// Initialization Methods
	OSErr             	BrowseAndJoinGame();
   	OSErr             	JoinGame(const char* inPlayerName, const char* inGameAddress, 
                            GameType inGameType, unsigned short inGamePort, 
                            UInt32 inRejoinKey = key_NotRejoining);

// Informational Methods
	virtual int		   	GetMyPlayerNum() const {return mPlayerNum;}	
	TArray<GameJoinInfo>&   GetHostArray() {return mHostArray;}
	TArray<Str255>&   	GetHostNameArray() {return mHostNameArray;}
	
	// extended to update join dialogs
	virtual void	   	SetPlayerInfo(int inPlayerNum, PublicPlayerInfoT &inInfo);


// User Interface Methods
	virtual Boolean		ObeyCommand(CommandT inCommand, void* ioParam);	
	virtual void		FindCommandStatus(CommandT inCommand,
							Boolean &outEnabled, Boolean &outUsesMark,
							UInt16 &outMark, Str255 outName);
	void				   	DoChat(int toPlayer = kNoSuchPlayer);
	virtual void		SetSelectedThingy(AGalacticThingyUI* inThingy);
	virtual void		AttemptClose(Boolean inRecordIt);
	virtual Boolean		AttemptQuitSelf(SInt32 inSaveOption);
	virtual void		UserChangedSettings();

// Turn Processing Methods
	virtual void      	SpendTime(const EventRecord &inMacEvent);
   	virtual void      	TimeIsUp();
	virtual void		DoEndTurn();

// Communication Methods
   	bool              IsConnected() {return (mEndpoint != NULL);}
	OSErr             StartBrowsingForGames();
   	void              ShutdownClient();
   	OSErr             ConnectToHost(GameJoinInfo inJoinInfo);
   	OSErr             ConnectToNamedHost(const char* inHostName, GameType inGameType, unsigned short inPort = 4429);
   	void              CloseHostConnection(bool inOrderlyClose = false);
   	void              StopBrowsingForGames();

   static void       ClientCallback(PEndpointRef inEndpoint, void* inContext,
    	                      NMCallbackCode inCode, NMErr inError, void* inCookie);
   static void       ClientEnumerationCallback(void *inContext, 
                            NMEnumerationCommand inCommand, NMEnumerationItem *item);
#if TARGET_OS_MAC
   static void       ClientAppleTalkEnumerationCallback(void *inContext, 
                            NMEnumerationCommand inCommand, NMEnumerationItem *item);
#endif // TARGET_OS_MAC

    void    HandleReceiveData(PEndpointRef inEndpoint);
    void    HandleClientEnumeration(NMEnumerationCommand inCommand, NMEnumerationItem *item, long inType);
    void    HandlePacket(Packet* inPacket);
    void    SendPacket(Packet* inPacket);
    void    HandleQueuedPacket(Packet* inPacket);
    void    SendLoginRequest(const char* inPlayerName, const char* inPassword, bool inAsNewPlayer);

    void    HandlePlayerToPlayerMessage(SInt8 fromPlayer,SInt8 toPlayer, const char* text);
    void    HandleLoginResponse(SInt8 asPlayer, SInt8 inResult, UInt32 inRejoinKey);
    void    HandlePrivatePlayerInfo(SInt8 forPlayer, PrivatePlayerInfoT& inInfo);

    void	SendClientSettings();
    
    void    HandleGameInfo(GameInfoPacket* inPacket);
    void    HandlePlayerInfo(PlayerInfoPacket* inPacket);

    void    HandleThingyCount(ThingyCountPacket* inPacket);
    void    HandleThingyData(ThingyDataPacket* inPacket);
    void    HandleTurnComplete(TurnCompletePacket* inPacket);
    void    HandleHostDisconnect(HostDisconnectPacket* inPacket);

    void    SendThingy(AGalacticThingy* inThingy);
    void    SendAllChangedThingys();
    
    void    SendCreateThingyRequest(AGalacticThingy* inThingy);
    void    HandleCreateThingyResponse(CreateThingyResponsePacket* inPacket);
    
    void    SendReassignThingyRequest(AGalacticThingy* inThingy, AGalacticThingy* oldContainer);
    void    HandleReassignThingyResponse(ReassignThingyResponsePacket* inPacket);

    void    LoadKnownGamesList();   // pre-loads the HostArrays with data about known games
    void    SaveKnownGamesList();
    
    void    FetchMatchMakerGames();  // retrieves games from the Galactica match maker
    
    void    AddGameToKnownList(GameJoinInfo inJoinInfo);
    void    RemoveGameFromKnownList(GameJoinInfo inJoinInfo);
    
    // only call after ConnectToHost() or ConnectToNamedHost() has been called
    GameJoinInfo&   GetConnectedHostJoinInfo();

protected:
	SInt8			    mPlayerNum;
	long                mNumDownloadedThingys;
    LThermometerPane*   mProgressBar;
    LWindow*            mProgressWind;
    CChatHandler*       mChatHandler;
    bool                mJoining;
    bool                mDisconnecting;

	// used for choosing from among available hosts
	TArray<GameJoinInfo>    mHostArray;
	TArray<Str255>          mHostNameArray;
	CJoinGameDlgHndlr*  mJoinGameHandler;
	GameJoinInfo        mConnectedHostInfo;
	PEndpointRef        mEndpoint;
	PConfigRef          mConfig;
  #if TARGET_OS_MAC
    PConfigRef          mConfigAppletalk;
  #endif // TARGET_OS_MAC
	std::queue<Packet*> mPackets;
	std::queue<ThingyDataPacket*> mThingyPackets;
	std::queue<AGalacticThingy*>  mDeleteQueue;
    Packet*             mInProgressPacket;
    SInt32              mBytesRemainingForPacket;
    UInt32              mOffsetInPacket;
    Packet              mPacketInfo;
    pdg::CriticalSection  mPacketQueueCriticalSection;
    SInt32              mBytesRemainingForHeader;
    UInt32              mOffsetInHeader;
  #ifndef NO_MATCHMAKER_SUPPORT
    MatchMakerClient* mMatchMaker;
    MMGameList*       mMatchMakerGamesList;
    UInt32            mNextMatchMakerQueryTime;
  #endif
  	UInt32				mNextPingMs;
  	UInt8				mNextPingNum;
  	bool				mRejoiningGameInPostedTurn;  // true if we are rejoining a game for which we have already posted the current turn
};

#endif // GALACTICA_CLIENT_H_INCLUDED

