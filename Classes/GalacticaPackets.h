// GalacticaPackets.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Initial version: 11/26/01, ERZ
//
//	This file contains packet definitions and type info

#ifndef GALACTICAPACKETS_H_INCLUDED
#define GALACTICAPACKETS_H_INCLUDED

//#define version_HostProtocol 0x0201cbB8   // v2.1b11r8
//#define version_HostProtocol 0x02130502   // v2.1.3r5d2
//#define version_HostProtocol 0x02200000   // v2.2 final (retail box release)
//#define version_HostProtocol   0x02030a04   // v2.3a4
#define version_HostProtocol   0x02030b04   // v2.3b4

//                        0        1         2         3         4         5
//                        12345678901234567890123456789012345678901234567890
#define string_Copyright "protocol copyright (c) 2001, Sacred Tree Software"
#define strLen_Copyright 49

#define strLen_Password  8  // maximum length of a password

#define strLen_RegCode   27   // number of characters in Reg code, not including nul termination

#include "GalacticaTypes.h"
#include "GalacticaUtils.h" // for Waypoint

enum {
   packet_Ignore                   = 0,      // never sent
   packet_PlayerToPlayerMessage    = 1,      // client->host->client
   packet_GameInfoRequest          = 2,      // client->host
   packet_GameInfo                 = 3,      // host->client
   packet_PlayerInfo               = 4,      // host->client
   packet_HostDisconnect           = 5,      // client fakes if detects dropped connection
   packet_LoginRequest             = 6,      // client->host
   packet_LoginResponse            = 7,      // host->client (in response to LoginRequest)
   packet_ThingyData               = 8,      // client->host & host->client
   packet_TurnComplete             = 9,      // client->host & host->client (terminates a series of ThingyData Packets)
   packet_PrivatePlayerInfo        = 10,     // host->client
   packet_CreateThingyRequest      = 11,     // client->host
   packet_CreateThingyResponse     = 12,     // host->client (optional in response to CreateThingyRequest)
   packet_ThingyCount              = 13,     // host->client
   packet_PlayerInfoRequest        = 14,     // client->host
   packet_Ping                     = 15,     // client->host->client
   packet_ReassignThingyRequest    = 16,     // client->host
   packet_ReassignThingyResponse   = 17,     // host->client (optional in response to ReassignThingyRequest)
   packet_ClientSettings		   = 18,	 // client->host

   // admin only commands here
   packet_Admin,  // marker for first Admin packet
   
   packet_AdminSetPlayerInfo = packet_Admin, // client->host
   packet_AdminSetGameInfo,                  // client->host
   packet_AdminEndTurnNow,                   // client->host
   packet_AdminPauseGame,                    // client->host
   packet_AdminResumeGame,                   // client->host
   packet_AdminEjectPlayer,                  // client->host

   // insert new types here
   packet_NumPacketTypes
};

//#ifdef DEBUG
// ERZ, don't want protocol incompatibility between debug/beta and final builds
  #define NUMBER_ALL_PACKETS
//#endif

typedef UInt16 PacketLenT;
typedef UInt8  PacketTypeT;

#define MAX_PACKET_LEN      0x8000
#define PACKET_DATA_OFFSET  sizeof(Packet)

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

// packet type added for compatibility with Version 3 host
// only used for testing
//#define V3FRAMING

#ifdef V3FRAMING
struct V3Packet {
    UInt32  v3packetLen;
    UInt32  v3packetNum;
} PACKED_STRUCT;    
#endif // V3FRAMING

struct Packet 
#ifdef V3FRAMING
 : V3Packet
#endif // V3FRAMING
{
   const char*  GetName() throw();
   bool         NeedsNulTermination() throw();
   static bool  ValidatePacket() throw();
   enum {PacketType = packet_Ignore};
   PacketLenT  packetLen;      // including this
   PacketTypeT packetType;
   UInt8	   UNUSED_PAD;
 #ifdef NUMBER_ALL_PACKETS
   UInt32      packetNum;      // the packet number, mainly used for debugging purposes
 #endif
} PACKED_STRUCT;


// variable length, can't be allocated on stack
// sent: client->host when user does a Send Message.
// response: host->client, immediate rebroadcast to all intended recipient clients
struct PlayerToPlayerMessagePacket : Packet {
   bool ValidatePacket() throw();
   enum {PacketType = packet_PlayerToPlayerMessage};
   SInt8   fromPlayer;
   SInt8   toPlayer;
   char    messageText[];
} PACKED_STRUCT;
enum {MaxSize_PlayerToPlayerMessagePacket = sizeof(PlayerToPlayerMessagePacket) + MAX_TEXT_MESSAGE_LEN+1};

// sent: client->host
// response: host->client, GameInfo packet
struct GameInfoRequestPacket : Packet {
   enum {PacketType = packet_GameInfoRequest};
} PACKED_STRUCT;

// variable length, can't be allocated on stack
// sent: host->client in response to a GameInfo request, or anytime the game info changes, particularly at the end of each turn
// no response
struct GameInfoPacket : Packet {
   bool ValidatePacket() throw();
   void ByteSwapIncoming() throw();
   void ByteSwapOutgoing() throw();
   enum {PacketType = packet_GameInfo};
   SInt32   hostVersion;
   SInt32	currTurn;
   SInt32	maxTimePerTurn;	// in seconds
   UInt32	secsRemainingInTurn;
   SInt8    totalNumPlayers;
   SInt8    numPlayersRemaining; // when this reaches 1, game is over unless totalNumPlayers = 1
   SInt8    numHumans;
   SInt8    numComputers;
   SInt8    numHumansRemaining; 	// when 0, host stops checking for turn complete
   SInt8    numComputersRemaining; 	// when 0, host stops trying to move computers
   SInt16	compSkill;	
   SInt16	compAdvantage;
   SInt16	sectorDensity;
   SInt16	sectorSize;
   Bool8    omniscient;
   Bool8    fastGame;
   Bool8    isFull;
   Bool8    endTurnEarly;
   SInt8    timeZone;               // +/- GMT, only meaningful if endTurnEarly is false and maxTimerPerTurn >= 1 day
   SInt8	UNUSED_PAD;
   UInt32   gameID;
   char		gameTitle[]; // nul terminated c string
} PACKED_STRUCT;
enum {MaxSize_GameInfoPacket = sizeof(GameInfoPacket) + MAX_GAME_NAME_LEN+1};

// variable length, can't be allocated on stack
// sent: host->client in response to a GameInfo request, or anytime the game info changes, particularly at the end of each turn
// no response
struct PlayerInfoPacket : Packet {
   bool ValidatePacket() throw();
   enum {PacketType = packet_PlayerInfo};
   SInt8               	playerNum;  // which player is this info for
   SInt8				UNUSED_PAD;
   PublicPlayerInfoT   info;
} PACKED_STRUCT;
enum {MaxSize_PlayerInfoPacket = sizeof(PlayerInfoPacket) + MAX_PLAYER_NAME_LEN+1};

enum {
   reason_LostConnection = 0,  // network trouble, or maybe the host crashed
   reason_HostShuttingDown,    // host is quitting
   reason_OutOfSync,           // client posted the wrong turn
   reason_DuplicateLogin,      // some else logged in with same name and password
   reason_AdminEject,          // an administrator ejected player from the game
   reason_BadData              // client sent a packet with data that shows it doesn't have accurate view of universe
};

// sent: host->client, client may also fake receipt of this packet if it detects a dropped connection
// no response
struct HostDisconnectPacket : Packet {
   enum {PacketType = packet_HostDisconnect};
   SInt8       reasonCode;
} PACKED_STRUCT;


// variable length, can't be allocated on stack
// sent: client->host, when a player tries to enter the game
// response: host->client, LoginReponse packet with results of login attempt. 
//           if login succeeded, host further responds by:
//             1) broadcasting GameInfo and PlayerInfo updates to all players
//             2) sending all game data to newly logged in client as follows:
//                  a) sends a PrivatePlayerInfo packet with the player's private data
//                  b) sends a ThingyCount packet with the number of items about to be transmitted
//                  c) sends a series of ThingyData packets, one for each AGalacticThingy in the mEverything list
//                  d) sends a Turn Complete packet tagged with the current turn number
struct LoginRequestPacket : Packet {
   bool        isNew() { return (joinInfo & JoinInfo_IsNewMask); }
   int         clientID() { return ((joinInfo & JoinInfo_ClientIDMask) >> JoinInfo_ClientIDShft); }
   void        setJoinInfo(bool isNew, int clientID) { joinInfo = ((UInt8)(clientID << JoinInfo_ClientIDShft) | (UInt8)isNew); }
    enum {
        JoinInfo_ClientIDMask = 0xfe,
        JoinInfo_ClientIDShft = 1,
        JoinInfo_IsNewMask    = 0x01
    };
   bool ValidatePacket() throw();
   void ByteSwapIncoming() throw();
   void ByteSwapOutgoing() throw();
   enum {PacketType = packet_LoginRequest};
   UInt32      clientVers;     // the version number of the client
   char        copyrightStr[strLen_Copyright+1];   // the copyright string for the client + NUL terminator
   char        password[strLen_Password+1];        // password string for the client + NUL terminator
   char        regCode[strLen_RegCode+1];          // the reg code for the copy of Galactica
   UInt8       joinInfo;       // client sends true in low bit if it is trying to join as a new player
                               // client sends 7 bit random client id number in high 7 bits, used to identify local games
   char        playerName[];   // nul terminated c string
} PACKED_STRUCT;
enum {
    MaxSize_LoginRequestPacket = sizeof(LoginRequestPacket) + MAX_PLAYER_NAME_LEN+1
};

enum {
   login_ResultSuccess = 0,
   login_ResultGameFull = 1,       // no space for new players
   login_ResultDuplicateLogin = 2, // player is already logged in with duplicate registration code
   login_ResultBadRequest = 3,     // the login request had invalid data
   login_ResultDuplicateName = 4,  // that name is already taken
   login_ResultBadVersion = 5,     // the host cannot accept connections from client of given version
   login_ResultBadPassword = 6,    // password doesn't match
   login_ResultPlayerNotFound = 7, // name not found for existing player
   login_ResultUnregistered = 8,   // you must be a registered user to play in this game
   login_ResultNotAdmitted = 9     // either were banned or it is a private game
           // if you expand beyond this, be sure to update str_HostDisconnect
};         // in GalacticaConsts.h, and put more space into the STR# 129 resource

// sent: host->client, in response to LoginRequest packet from client
// no response
struct LoginResponsePacket : Packet {
   enum {PacketType = packet_LoginResponse};
   void ByteSwapIncoming() throw();
   void ByteSwapOutgoing() throw();
   SInt8       playerNum;
   SInt8       loginResult;
   UInt32      rejoinKey;          // this is only meaningful if the login result was login_ResultSuccess
} PACKED_STRUCT;

// variable length, can't be allocated on stack
// sent client->host & host->client. The basic packet type for transfering data about an AGalacticThingy.
//          Always sent in a group, followed by a TurnComplete packet. Host sends as part of game login
//          or when End of Turn processing is complete. Client sends when posting turn, or when it wants
//          to persist users changes to host. Host always preceeds with ThingyCount packet so client knows 
//          how big a transfer to expect (host doesn't care).
// no response
struct ThingyDataPacket : Packet {
   bool ValidatePacket() throw();
   void ByteSwapIncoming() throw();
   void ByteSwapOutgoing() throw();
   enum {PacketType = packet_ThingyData};
   PaneIDT     id;     // id number of the thingy
   UInt32      type;   // type of thingy (thingyType_Star, thingyType_Ship, etc..)
   SInt8       action; // what needs to be done (action_Added, action_Deleted, etc..)
   UInt8       visibility;  // what is the visibility of this item at the destination
   char        data[]; // the stream data for the thingy
} PACKED_STRUCT;

// sent client->host & host->client. Terminates a series of ThingyData Packets.
// no response
struct TurnCompletePacket : Packet {
   void ByteSwapIncoming() throw();
   void ByteSwapOutgoing() throw();
   enum {PacketType = packet_TurnComplete};
   SInt16      turnNum; // which turn
} PACKED_STRUCT;

// sent host->client during initial login and at the end of each turn
// no response
struct PrivatePlayerInfoPacket : Packet {
   void ByteSwapIncoming() throw();
   void ByteSwapOutgoing() throw();
   bool ValidatePacket() throw();
   enum {PacketType = packet_PrivatePlayerInfo};
   SInt8               playerNum;  // which player is this info for
   SInt8				UNUSED_PAD;
   PrivatePlayerInfoT  info;
} PACKED_STRUCT;

// sent client->host when user creates a new rendezvous poiont or fleet through the user interface
// response: optional host->client CreateThingyResponse packet with the permanent ID assigned to the new item
//           if request failed, no response is sent
struct CreateThingyRequestPacket : Packet {
   void ByteSwapIncoming() throw();
   void ByteSwapOutgoing() throw();
   enum {PacketType = packet_CreateThingyRequest};
   Waypoint    where;   // the location where the item was created
   PaneIDT     tempID;  // this is the ID number the thing has been assigned by the client
   UInt32      type;    // type of thingy (thingyType_Star, thingyType_Ship, etc..)
} PACKED_STRUCT;

// sent: host->client, in response to CreateThingyRequest
// no response
struct CreateThingyResponsePacket : Packet {
   void ByteSwapIncoming() throw();
   void ByteSwapOutgoing() throw();
   enum {PacketType = packet_CreateThingyResponse};
   PaneIDT     tempID;  // this is the ID number the thing has been assigned by the client
   PaneIDT     newID;   // this is the new id assigned. If rejected this is returned as PaneIDT_Undefined
} PACKED_STRUCT;

// sent host->client, to let client know how many things are about to be sent down so it can
//          put up a reasonable progress dialog.
// no response
struct ThingyCountPacket : Packet {
   void ByteSwapIncoming() throw();
   void ByteSwapOutgoing() throw();
   enum {PacketType = packet_ThingyCount};
   long        thingyCount;  // the number of thingys that will be sent
} PACKED_STRUCT;

// sent: client->host
// response: host->client, GameInfo packet followed by one PlayerInfo packet for each player (including AIs)
struct PlayerInfoRequestPacket : Packet {
   enum {PacketType = packet_PlayerInfoRequest};
} PACKED_STRUCT;

// sent: client->host->client
// response: ping is returned
struct PingPacket : Packet {
   enum {PacketType = packet_Ping};
   UInt8 pingNum;
   Bool8 isReply; // set to true if this ping was sent in response to another ping
} PACKED_STRUCT;

// sent client->host when user reassigns a ship to a different fleet through the user interface
// response: optional host->client ReassignThingyResponse packet with the permanent ID assigned to the new item
//           if request failed because the item wasn't allowed to be moved, reply with success = false will be sent
//           other failure conditions may not necessarily send a response
struct ReassignThingyRequestPacket : Packet {
   void ByteSwapIncoming() throw();
   void ByteSwapOutgoing() throw();
   enum {PacketType = packet_ReassignThingyRequest};
   PaneIDT     id;  // this is the ID number the thing being moved
   UInt32      type;    // type of thingy (thingyType_Fleet, thingyType_Ship, etc..)
   PaneIDT     oldContainer;   // the ship or star where the item was located previously
   PaneIDT     newContainer;   // the ship or star to which the item is being assigned
} PACKED_STRUCT;

// sent: host->client, in response to ReassignThingyRequest
// no response
struct ReassignThingyResponsePacket : Packet {
   void ByteSwapIncoming() throw();
   void ByteSwapOutgoing() throw();
   enum {PacketType = packet_ReassignThingyResponse};
   PaneIDT     id;  // this is the ID number of the thing that was reassigned
   Bool8       success; // true if the reassignment was allowed
} PACKED_STRUCT;

// sent client->host when user wants to set game options
struct ClientSettingsPacket : Packet {
   void ByteSwapIncoming() throw();
   void ByteSwapOutgoing() throw();
   enum {PacketType = packet_ClientSettings};
   UInt8		defaultBuildType;	// what type of ships to build by default
   SInt8		UNUSED_PAD;
   UInt32     	defaultGrowth;   	// what amount assigned to growth by default
   UInt32     	defaultTech;   		// what amount assigned to research by default
   UInt32		defaultShips;		// what amount assigned to shipbuilding by default
} PACKED_STRUCT;

// ================================ ADMIN PACKETS =========================================================

// variable length, can't be allocated on stack
// sent: client->host
// response: host->client, PlayerInfo packet with updated info is broadcast
struct AdminSetPlayerInfoPacket : Packet {
   enum {PacketType = packet_AdminSetPlayerInfo};
   SInt8 playerNum;
	SInt8 compSkill;
	SInt8 compAdvantage;
   PublicPlayerInfoT  info;
} PACKED_STRUCT;
enum {MaxSize_AdminSetPlayerInfoPacket = sizeof(AdminSetPlayerInfoPacket) + MAX_PLAYER_NAME_LEN+1};

// variable length, can't be allocated on stack
// sent: client->host
// response: host->client, GameInfo packet with updated info is broadcast
struct AdminSetGameInfoPacket : GameInfoPacket {
   enum {PacketType = packet_AdminSetGameInfo};
} PACKED_STRUCT;
enum {MaxSize_AdminSetGameInfoPacket = MaxSize_GameInfoPacket};

// sent: client->host
// no response
struct AdminEndTurnNowPacket : Packet {
   enum {PacketType = packet_AdminEndTurnNow};
} PACKED_STRUCT;

// sent: client->host
// response: none?
struct AdminPauseGamePacket : Packet {
   enum {PacketType = packet_AdminPauseGame};
} PACKED_STRUCT;

// sent: client->host
// response: none? 
struct AdminResumeGamePacket : Packet {
   enum {PacketType = packet_AdminResumeGame};
} PACKED_STRUCT;


// sent: client->host
// response: affected player gets a dropped
struct AdminEjectPlayerPacket : Packet {
   enum {PacketType = packet_AdminResumeGame};
   SInt8 playerNum;
} PACKED_STRUCT;


#if defined( __MWERKS__ )
    #pragma options align=reset
#elif defined( _MSC_VER )
    #pragma pack(pop)
#endif

// ================================ Packet Functions =========================================================


// Use CreatePacket to create a packet when the type is not known at compile time.
// If inLen is passed in a zero, it will use the default length for that packet type
// inLen cannot be zero for variable length packets, they have no default length!
Packet*      CreatePacket(PacketTypeT inType, PacketLenT inLen = 0) throw();

// Use templatized CreatePacket<> to quickly create a packet when the type is known 
// at compile time. The packet will be created to the normal length for that type, 
// plus the number of bytes passed in as inExtraLength.
template<class T>
T* CreatePacket(PacketLenT inExtraLen = 0) throw() 
                { return (T*)CreatePacket(T::PacketType, sizeof(T)+inExtraLen); }

void         ByteSwapOutgoingPacket(Packet* inPacket);
void         ByteSwapIncomingPacket(Packet* inPacket);
void         ReleasePacket(Packet* inPacket) throw();
Packet*      ClonePacket(const Packet* inPacket) throw();
const char*  GetPacketNameForType(PacketTypeT inType) throw();
bool         PacketNeedsNulTermination(PacketTypeT inType) throw();

// Use this to validate a packet. Calls particular packet methods to validate.
// If packet invalid, sets type to packet_Ignore and returns false
bool         ValidatePacket(Packet* inPacket) throw();

// Use the following functions to track packet statistics
void         RecordPacketSent(Packet* inPacket) throw();
void         RecordPacketReceived(Packet* inPacket) throw();

void         ReportPacketStatistics() throw();

#endif //GALACTICAPACKETS_H_INCLUDED


