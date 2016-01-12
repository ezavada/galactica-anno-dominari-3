//	GalacticaTypes.h		
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#ifndef GALACTICA_TYPES_H_INCLUDED
#define GALACTICA_TYPES_H_INCLUDED

#include "GalacticaConsts.h"
#include "DatabaseTypes.h"

class AGalacticThingy;

// these must be packed structures
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


typedef struct NewGameInfoT {
	Str255		gameTitle;
	SInt16		numHumans;
	SInt16		numComputers;
	SInt16		density;
	SInt16		sectorSize;
	SInt16		compSkill;
	SInt16		compAdvantage;
	SInt32		maxTimePerTurn; // seconds
	bool		hosting;
	bool		fastGame;
	bool		omniscient;
	bool        endEarly;
} PACKED_STRUCT NewGameInfoT;

typedef struct NewThingInfoT {
	RecIDT	id;
	OSType	type;
} PACKED_STRUCT NewThingInfoT;

typedef struct ThingRecInfoT {
	RecIDT	id;
	OSType	type;
	short	owner;	// we use this to help keep track of how many pieces are owned by whom
	short	info; // negative = dead, 0 - no info, 1 - view info, 0x7F - change info
} PACKED_STRUCT ThingRecInfoT;

typedef struct ThingDBInfoT {	// this is the thing info as actually written to disk
	RecIDT	thingID;
	OSType	thingType;
	SInt8	action;   // EAction
	char	UNUSED_PAD;
	UInt16	flags;
} PACKED_STRUCT ThingDBInfoT;

typedef struct ThingMemInfoT {	// this is the thing info in the needy list
	ThingDBInfoT		info;
	AGalacticThingy*	thing;
} PACKED_STRUCT ThingMemInfoT;

enum {
	thingInfoFlag_None		= 0x00,
	thingInfoFlag_Handled	= 0x01,	// set when the item has been processed once
	thingInfoFlag_AutoShow	= 0x02	// set before ShowMessages() for autoshow messages
};


// info that is publicly available about any player
struct PublicPlayerInfoT {
	Bool8		isLoggedIn;     // currently online
    Bool8       isComputer;     // computer player
    Bool8       isAssigned;     // assigned to a human player
    Bool8       isAlive;        // false if player has been defeated
	char		name[MAX_PLAYER_NAME_LEN+1]; // nul terminated c string
	char		UNUSED_PAD;
} PACKED_STRUCT;

// info that only the host knows about any player
struct PrivatePlayerInfoT_v22  {
	char        password[MAX_PASSWORD_LEN]; // no NUL terminator
	SInt32		lastTurnPosted;
	SInt32		numPiecesRemaining;	// when this is 0, player is dead
	PaneIDT		homeID;
	PaneIDT	    techStarID;		// id of computer player's main tech star (may be others)
	SInt16		hiStarTech;		// used to decide when to send a "new discovery" message
	SInt16		hiShipTech;		// used to decide when to send a "new class of ship" message
	Bool8		builtColony;	// for "new class of ship" message
	Bool8		builtScout;		// for "new class of ship" message
	Bool8		builtSatellite;	// for "new class of ship" message
	Bool8	    hasTechStar;	// true if this computer player has a tech star
	SInt8       compSkill;
	SInt8       compAdvantage;
} PACKED_STRUCT;

struct PrivatePlayerInfoT : public PrivatePlayerInfoT_v22 {
	// following added for v3.0
    UInt8		defaultBuildType;	// what type of ships to build by default
    UInt8		UNUSED_PAD;
    UInt32     	defaultGrowth;   	// what amount assigned to growth by default
    UInt32     	defaultTech;   		// what amount assigned to research by default
    UInt32		defaultShips;		// what amount assigned to shipbuilding by default
    UInt32		reserved[16];		// reserve some space for future expansion
} PACKED_STRUCT;

struct GameInfoT_v12 {
	DatabaseRec	recHeader;
	SInt32		currTurn;
	SInt32		maxTimePerTurn;	// in seconds
	union {
		UInt32		nextTurnEndsAt;	// Macintosh raw date time, seconds since 12:00 Jan 1, 1904
		UInt32		secsRemainingInTurn;	// only used when stored to disk with closed host
	};
	SInt16		totalNumPlayers;
	SInt16		numPlayersRemaining; // when this reaches 1, game is over unless totalNumPlayers = 1
	SInt8		numHumans;
	SInt8		numComputers;
	SInt8		numHumansRemaining; 	// when 0, host stops checking for turn complete
	SInt8		numComputersRemaining; 	// when 0, host stops trying to move computers
	Bool8		hasHost;
	SInt8		gameState; 		// EGameState 0 - normal play, 1 - game just ended, 2 - continuing past end 
	char		hostPassword[MAX_PASSWORD_LEN];	// no NUL terminator
  // ======  following added for v1.2 ============
	UInt32		sequenceNumber;		// pane id for last item created
	SInt16		compSkill;	
	SInt16		compAdvantage;
	SInt16		sectorDensity;
	SInt16		sectorSize;
	Bool8		omniscient;
	Bool8		fastGame;
	Str255		gameTitle;
} PACKED_STRUCT;

struct GameInfoT : GameInfoT_v12 {
  // ====== following added for v2.1 ==============
    Bool8       endTurnEarly;   // true if a turn can be ended when all players have posted
    SInt8       timeZone;       // GMT +/-
    UInt32      gameID;         // uniquely identifies a game (also used as rejoin key at the moment)
} PACKED_STRUCT;

#ifndef GALACTICA_SERVER
// v2.1, only used in legacy (v2.0.x and earlier) saved game files
struct DBPlayerInfoT {
	DatabaseRec	recHeader;
	SInt32		lastTurnPosted;
	SInt32		numPiecesRemaining;	// when this is 0, player is dead
	SInt32		score;
	PaneIDT		homeID;
	Bool8		isLoggedIn;
	Bool8		isNew;		// lets the login handler know if we are trying to add a new player
	char		name[25];   // pascal string[24]
	char		unused;
	union {
		char		password[MAX_PASSWORD_LEN];	// c string no nul terminator
		struct {
			PaneIDT	techStarID;		// id of main tech star (may be others)
			Bool8	hasTechStar;	// true if this planet has a tech star
			
		} PACKED_STRUCT expert;
	};
	SInt16		hiStarTech;		// used to decide when to send a "new discovery" message
	SInt16		hiShipTech;		// used to decide when to send a "new class of ship" message
	Bool8		builtColony;	// for "new class of ship" message
	Bool8		builtScout;		// for "new class of ship" message
	Bool8		builtSatellite;	// for "new class of ship" message
	char		unused2;
	
	void GetPublicInfo(PublicPlayerInfoT& ioPubInfo, const GameInfoT& inGameInfo, long inPlayerNum = 0) {
    		ioPubInfo.isLoggedIn = false;
    		if (inPlayerNum) {
    		   // it's no longer safe to assume that a player is a computer just because of
    		   // there position in the player list. However, in this case it is okay
    		   // since this is only used for reading legacy saved games, where the assumption
    		   // holds true.
    		    ioPubInfo.isComputer = (inPlayerNum > inGameInfo.numHumans);
    		} else {
    		    ioPubInfo.isComputer = false;
    		}
    		ioPubInfo.isAssigned = true;  // assuming true
    		ioPubInfo.isAlive = (numPiecesRemaining > 0);
    		p2cstrcpy(ioPubInfo.name, (StringPtr)name);
 	    }
 	
	void GetPrivateInfo(PrivatePlayerInfoT& ioPrivateInfo, const GameInfoT& inGameInfo) {
   		    std::memcpy(ioPrivateInfo.password, password, 8);
    		ioPrivateInfo.lastTurnPosted = lastTurnPosted;
    		ioPrivateInfo.numPiecesRemaining = numPiecesRemaining;
    		ioPrivateInfo.homeID = homeID;
    		ioPrivateInfo.techStarID = expert.techStarID;
    		ioPrivateInfo.hiStarTech = hiStarTech;
    		ioPrivateInfo.hiShipTech = hiShipTech;
    		ioPrivateInfo.builtColony = builtColony;
    		ioPrivateInfo.builtScout = builtScout;
    		ioPrivateInfo.builtSatellite = builtSatellite;
    		ioPrivateInfo.hasTechStar = expert.hasTechStar;
    		ioPrivateInfo.compSkill = inGameInfo.compSkill;
    		ioPrivateInfo.compAdvantage = inGameInfo.compAdvantage;
        }
} PACKED_STRUCT;
#endif // GALACTICA_SERVER

typedef struct Point3D {
	long x;
	long y;
	long z;
	int operator== (const Point3D &p) {return ((x == p.x)&&(y==p.y)&&(z==p.z));}
} PACKED_STRUCT Point3D, *Point3DPtr;

extern const Point3D gNullPt3D;
extern const Point3D gNowherePt3D;


struct Galactica12PrefsT {
	Bool8		wantSound;
	Bool8		fullScreen;
	Str27		registration;	// ie: GLDE-1234-1234-6789-5672-10
	Bool8		mouseLingerEnabled;	// added for v1.2fc2
	Str27		oldRegistration;
} PACKED_STRUCT;

struct GalacticaPrefsT : Galactica12PrefsT {
	Str32       defaultPlayerName;  // added for v2.1
} PACKED_STRUCT;

typedef GalacticaPrefsT **GalacticaPrefsHnd;

#if defined( __MWERKS__ )
    #pragma options align=reset
#elif defined( _MSC_VER )
    #pragma pack(pop)
#endif

#endif // GALACTICA_TYPES_H_INCLUDED
