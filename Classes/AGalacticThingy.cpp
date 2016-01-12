// AGalacticThingy.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ 4/18/96
// 9/21/96, 1.0.8 fixes incorporated
// 1/11/98, 1.2, converted to CWPro1
// 9/14/98, 1.2b8, Changed to ThingyRefs, CWPro3
// 4/3/99,	1.2b9, Changed to support Single Player Unhosted Games, CWPro4

#include "AGalacticThingy.h"
#include "CStar.h"
#include "CMessage.h"
#include "CShip.h"
#include "CFleet.h"
#include "CRendezvous.h"
#include "TArrayIterator.h"
#include "GalacticaPackets.h"
#include "GalacticaHost.h"

#ifndef GALACTICA_SERVER
#include "GalacticaDoc.h"
#include "GalacticaHostDoc.h"
#include "GalacticaSingleDoc.h"
#include "GalacticaClient.h"
#endif // GALACTICA_SERVER

#include <UMemoryMgr.h>
#include <string>

#define DRAG_SLOP_PIXELS 4
// define as 1 to use a single handle as a reusable buffer, 0 to create new handle each time
#define SHARED_THINGY_IO_BUFFER	1

// lowest brightness of thingies other than stars
#define LOWEST_BRIGHTNESS 0.1


// these are defined in Galactica.cp
typedef long **classNumPtr;
extern UInt32	gAGalacticThingyClassNum;
extern UInt32	gANamedThingyClassNum;

// uncomment the following line to have all thingy's pane frames drawn
//#define Draw_Thingy_Frames

#ifdef DEBUG
ostream& operator << (ostream& cout, AGalacticThingy *it) {
    if (!ValidateThingy(it)) {
        cout << " INVALID "<<(void*)it;
        return cout;
    }
	long type = it->GetThingySubClassType();
	cout << LongTo4CharStr(type) << " " << it->GetID() 
		 << " [" << it->GetOwner() << "] {" << (void*)it << "}";
	if ((type == thingyType_Ship) || (type == thingyType_Star) || (type == thingyType_Fleet) || (type == thingyType_Rendezvous)) {
		cout << " \"" << it->GetName(kIgnoreVisibility) << "\"";
	}
	if (it->IsDead()) {
		cout << " DEAD";
	}
	return cout;
}
#endif



#ifndef GALACTICA_SERVER
#if defined(GAME_CONFIG_HOST)
static bool				gAllowDragAnything = true;
#elif defined(DEBUG)
static bool				gAllowDragAnything = true;
//static bool				gAllowDragAnything = false;
#endif
#endif // ndef GALACTICA_SERVER

CMemStream*		AGalacticThingy::sThingyIOStream = nil;


UThingyClassInfo gStarClassInfo(thingyType_Star, event_DefendersLost, visibility_None, visibility_Details);
UThingyClassInfo gShipClassInfo(thingyType_Ship, event_ShipLost, visibility_None, visibility_Class, HOTSPOT_SIZE_SHIP, FRAME_SIZE_DEFAULT, classInfo_CanFleePursuit);
UThingyClassInfo gFleetClassInfo(thingyType_Fleet, event_FleetLost, visibility_None, visibility_Class, HOTSPOT_SIZE_FLEET, FRAME_SIZE_DEFAULT, classInfo_CanFleePursuit);
UThingyClassInfo gWormholeClassInfo(thingyType_Wormhole);
UThingyClassInfo gStarLaneClassInfo(thingyType_StarLane);
UThingyClassInfo gRendezvousClassInfo(thingyType_Rendezvous, event_None, visibility_None, visibility_None, HOTSPOT_SIZE_RENDEZVOUS, FRAME_SIZE_RENDEZVOUS);
UThingyClassInfo gMessageClassInfo(thingyType_Message, classInfo_DontSendDeleteInfo, classInfo_NoWriteback);
UThingyClassInfo gDestructionMsgClassInfo(thingyType_DestructionMessage, classInfo_DontSendDeleteInfo, classInfo_NoWriteback);




// *** Utility Routines

UThingyClassInfo*
AGalacticThingy::GetClassInfoFromType(long inThingyType) {
	UThingyClassInfo* theInfo = nil;
	switch (inThingyType) {
		case thingyType_Star:
			theInfo = &gStarClassInfo;
			break;
		case thingyType_Ship:
			theInfo = &gShipClassInfo;
			break;
		case thingyType_Fleet:
			theInfo = &gFleetClassInfo;
			break;
		case thingyType_Wormhole:
			theInfo = &gWormholeClassInfo;
			break;
		case thingyType_StarLane:
			theInfo = &gStarLaneClassInfo;
			break;
		case thingyType_Rendezvous:
			theInfo = &gRendezvousClassInfo;
			break;
		case thingyType_Message:
			theInfo = &gMessageClassInfo;
			break;
		case thingyType_DestructionMessage:
			theInfo = &gDestructionMsgClassInfo;
			break;
	}
	return theInfo;
}

AGalacticThingy*
AGalacticThingy::MakeThingyFromSubClassType(LView* inSuper, GalacticaShared* inGame, long inThingyType) {
	AGalacticThingy* aThingy = NULL;
	DEBUG_OUT("Make "<< LongTo4CharStr(inThingyType), DEBUG_DETAIL | DEBUG_OBJECTS);
	switch (inThingyType) {
		case thingyType_Star:
			aThingy = new CStar(inSuper, inGame);
			break;
		case thingyType_Ship:
			aThingy = new CShip(inSuper, inGame);
			break;
		case thingyType_Fleet:
			aThingy = new CFleet(inSuper, inGame);
			break;
		case thingyType_Wormhole:
//			aThingy = new CWormhole(inSuper, inGame);  // wormholes aren't implemented yet
			break;
		case thingyType_StarLane:
//			aThingy = new CStarLane(inSuper, inGame);
			break;
		case thingyType_Rendezvous:
			aThingy = new CRendezvous(inSuper, inGame);
			break;
		case thingyType_Message:
//			ASSERT_STR(inSuper == nil, "Messages shouldn't have a superview");
			aThingy = new CMessage(inGame); 	//(inSuper);
			break;
		case thingyType_DestructionMessage:
//			ASSERT_STR(inSuper == nil, "Destruction Messages shouldn't have a superview");
			aThingy = new CDestructionMsg(inGame);	//(inSuper);
			break;
		default:
			DEBUG_OUT("Unknown Thingy Type "<< LongTo4CharStr(inThingyType), DEBUG_ERROR | DEBUG_OBJECTS);
			break;
	}
	return aThingy;
}



// **** GALACTIC THINGY ****


AGalacticThingy::AGalacticThingy(GalacticaShared* inGame, long inThingyType) 
 : mID(PaneIDT_Unspecified), 
   mClassInfo( AGalacticThingy::GetClassInfoFromType(inThingyType) ),
   mGame(NULL),        // SetGame() needs to know that this is NULL
   mCurrPos(),
   mLastChanged(0),	// when was this object last udated in the Datastore
   mOwnedBy(kOwnedBy_NOBODY),
   mTechLevel(0),
   mDamage(0),
   mNeedsDeletion(0),	// *** change to different type, currently PaneIDT
   mChanged(false),
   mWantsAttention(false),
	mLastCallin(-5),	// turn in which item last called in
	mScannerRange(0),
	mApparentSize(0),
	mStealth(0),
	mMaxVisibility(0),
	mMinVisibility(0)
{
	if (inGame) {
		SetGame(inGame);	// save a reference to the Game object
        DEBUG_OUT(this<<" mGame="<<(void*)mGame, DEBUG_TRIVIA);
	} else {
		SetGame(GalacticaShared::GetStreamingGame());
	  #ifdef DEBUG
		if (!mGame) {
    	    DEBUG_OUT("Initializing "<<this<<" without a game", DEBUG_ERROR);
	    }
	  #endif
	}
	InitThingy();
}

AGalacticThingy::~AGalacticThingy() {
	if (gAGalacticThingyClassNum == 0) {		// this is how we get the class number of a
		gAGalacticThingyClassNum = **((classNumPtr)this);	// partially constructed thingy
	}
	if (mGame) {
		if (!mGame->IsClosing()) {
			DEBUG_OUT("Deleted Object " << this << " @ "<< mCurrPos, DEBUG_DETAIL | DEBUG_OBJECTS | DEBUG_CONTAINMENT);
			mGame->RemoveThingy(this);
		}
	}
}

void
AGalacticThingy::InitThingy() {
	DEBUG_OUT("InitThingy " << this, DEBUG_DETAIL | DEBUG_OBJECTS);
	if (sThingyIOStream == nil) {
		sThingyIOStream = new CMemStream();
		ThrowIfNil_(sThingyIOStream);
		sThingyIOStream->SetLength(INITIAL_THINGY_BUFFER_SIZE);
	}
	// initialize Fog of war defaults
	mMinVisibility = mClassInfo->GetDefaultMinVisibility();
	mMaxVisibility = mClassInfo->GetDefaultMaxVisibility();
	for (int i = 0; i < MAX_PLAYERS_CONNECTED; i++) {
	    mVisibilityTo[i] = visibility_None;
	    mLastSeen[i] = (UInt32)lastSeen_Never;
	    mLastSeenByID[i] = kInvalidRecID;
	}
}

SInt32  
AGalacticThingy::GetID() const { 
    return mID; 
}

void    
AGalacticThingy::SetID(SInt32 inID) {
    mID = inID;
}

const char* 
AGalacticThingy::GetName(bool) { //considerVisibility
    return ""; 
}

const char* 
AGalacticThingy::GetShortName(bool) { //considerVisibility
    return ""; 
}

AGalacticThingyUI* 
AGalacticThingy::AsThingyUI() {
    return NULL; 
}

void
AGalacticThingy::FinishInitSelf() {
}

void
AGalacticThingy::ReadStream(LStream *inStream, UInt32 streamVersion) {
    DEBUG_OUT("ReadStream "<<this, DEBUG_TRIVIA | DEBUG_OBJECTS);
	SInt32 theID;
	UInt8 owner;
	Bool8 changed, attn, scrapped;
	*inStream >> theID >> owner;
	if (streamVersion <= version_v21b9_SavedTurnFile) {
	    // read legacy info level field, no longer used
	    UInt16 junkInfoLevel;
	    *inStream >> junkInfoLevel;
	}
	*inStream >> mCurrPos
		>> mLastChanged >> mTechLevel >> mDamage
		>> changed >> attn >> scrapped >> mLastCallin;
    if (streamVersion > version_v21b9_SavedTurnFile) {
        // read new stuff added for fog of war
		*inStream >> mScannerRange >> mApparentSize >> mStealth >> mMaxVisibility >> mMinVisibility;
    } else {
        // fill out new fields with info appropriate for a legacy game
        mScannerRange = 0;
        mApparentSize = 1;
        mStealth = 1;
        mMaxVisibility = visibility_Max;
        mMinVisibility = visibility_Max;
    }
    
	if ( (mCurrPos == gNullPt3D) && (mCurrPos.GetType() <= wp_Coordinates) ) {
		DEBUG_OUT("Read " << this << " at (0,0)", DEBUG_ERROR | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE | DEBUG_MOVEMENT);
	}

	SetID(theID);
	mOwnedBy = owner;
	mChanged = changed;
	mWantsAttention = attn;
	if (scrapped) {
		mNeedsDeletion = -1;			// restore scrapped status
	}
}

void
AGalacticThingy::WriteStream(LStream *inStream, SInt8 forPlayerNum) {
	UInt8 owner = mOwnedBy;
	Bool8 changed = mChanged;
	Bool8 attn = mWantsAttention;
	Bool8 scrapped = (mNeedsDeletion == -1);
	mLastChanged = mGame->GetTurnNum();

	if ( mCurrPos.IsNull() ) {
		DEBUG_OUT("Wrote " << this << " at (0,0)", DEBUG_ERROR | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE | DEBUG_MOVEMENT);
	}

	*inStream << mID  << owner //<< mInfoLevel 
	    << mCurrPos
		<< mLastChanged << mTechLevel << mDamage
		<< changed << attn << scrapped << mLastCallin
		<< mScannerRange << mApparentSize << mStealth << mMaxVisibility << mMinVisibility;
}

void
AGalacticThingy::UpdateClientFromStream(LStream *inStream) {
#ifndef GALACTICA_SERVER
   DEBUG_OUT("UpdateFromStream "<<this, DEBUG_TRIVIA | DEBUG_OBJECTS);
	SInt32 theID;
	UInt8 owner;
	Bool8 changed, attn, scraped;
	*inStream >> theID >> owner //>> mInfoLevel 
	    >> mCurrPos
		>> mLastChanged >> mTechLevel >> mDamage
		>> changed >> attn >> scraped >> mLastCallin
		>> mScannerRange >> mApparentSize >> mStealth >> mMaxVisibility >> mMinVisibility;

	if ( (mCurrPos == gNullPt3D) && (mCurrPos.GetType() <= wp_Coordinates) ) {
		DEBUG_OUT("Updated " << this << " at (0,0)", DEBUG_ERROR | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE | DEBUG_MOVEMENT);
	}

	mOwnedBy = owner;
	mChanged = changed;  // for client we clear the changed flag once the packet read is complete in HandleThingyData()
	mWantsAttention = attn;
	if (scraped) {
	   GalacticaClient* client = GetGameClient();
	   if (client && (mOwnedBy != client->GetMyPlayerNum()) ) {
		   mNeedsDeletion = -1;			// update scrapped status for other player's ships
		   // we don't update our own ships because this is under player's control
		}
	}
	if (theID != mID) {
		DEBUG_OUT("Updated " << this << " but got ID " << theID, DEBUG_ERROR | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE);
	}
#endif // ! GALACTICA_SERVER
}

void
AGalacticThingy::UpdateHostFromStream(LStream *inStream) {
// this method is used to update the host from a packet sent by a client
// the client is not allowed to update most of the data, so many fields are
// ignored
   DEBUG_OUT("UpdateHostFromStream "<<this, DEBUG_TRIVIA | DEBUG_OBJECTS);
	PaneIDT ignoreID;
	UInt8 ignoreOwner;
	UInt16 ignoreTechLevel;
//	UInt16 ignoreInfoLevel;
	SInt32 ignoreDamage;
	Bool8 changed, attn, scraped;
	UInt32 ignoreScannerRange;
	UInt32 ignoreApparentSize;
	UInt8 ignoreStealth;
	UInt8 ignoreMaxVisibility;
	UInt8 ignoreMinVisibility;
	
	EWaypointType theHostWpType = mCurrPos.GetType();
	Waypoint wp;

	*inStream >> ignoreID >> ignoreOwner // >> ignoreInfoLevel 
	    >> wp
		>> mLastChanged >> ignoreTechLevel >> ignoreDamage
		>> changed >> attn >> scraped >> mLastCallin
		>> ignoreScannerRange >> ignoreApparentSize >> ignoreStealth 
		>> ignoreMaxVisibility >> ignoreMinVisibility;

    if (ClientCanMoveMeAround()) {
        mCurrPos = wp;      // client can move me around anywhere it wants
    } else {
        // client generally can't move me anywhere, check for certain kinds of legal moves
        bool legalPositionChange = false;
        // make sure we are actually switching between thingys, not moving coordinates
        // or just getting an update with the same thingy container
        if ( (theHostWpType != wp_Coordinates) && (theHostWpType != wp_None) 
            && (wp.GetThingyRef() != mCurrPos.GetThingyRef()) ) {
            if ((theHostWpType == wp_Star) && (wp.GetType() == wp_Fleet)) {
                // can move from a star into fleet, use position sent from client
                legalPositionChange = true;
            } else if ((theHostWpType == wp_Fleet)
              && ((wp.GetType() == wp_Fleet) || (wp.GetType() == wp_Star)) ) {
                // can move from a fleet to another or to a star, use position sent from client
                legalPositionChange = true;
            }
        }
        // no other client initiated moves are allowed, so the position sent from the client is
        // ignored
    #warning FIXME: check overall coordinates after move to be sure coordinates not changed
        if (legalPositionChange) {
            DEBUG_OUT("Legal update "<<this<<" pos from " << mCurrPos << " to "<< wp, DEBUG_DETAIL | DEBUG_MOVEMENT);
            AGalacticThingy* thingy = wp.GetThingy();
            AGalacticThingy* oldThingy = mCurrPos.GetThingy();
            if (thingy) {
                thingy->AddContent(this, contents_Any, kDontRefresh);   // make sure we put it into the new thingy
                thingy->WriteToDataStore();  // write change to new container to DB
            }
            if (oldThingy) {
                oldThingy->WriteToDataStore(); // make sure the old location is updated
            }
        }
    }

	mChanged = true;  // for host we set the changed flag once the packet read is complete in HandleThingyData()
	mWantsAttention = attn;
	if (scraped) {
		mNeedsDeletion = -1;			// restore scrapped status
	}
}

void
AGalacticThingy::AssignUniqueID() {
	PaneIDT theID = GetID();
	if (theID == PaneIDT_Unspecified) {	// make sure the pane has a proper record id
		if (mGame->IsHost() && !mGame->IsSinglePlayer()) {  // single player unhosted returns true for IsHost()
		    GalacticaHost* host = dynamic_cast<GalacticaHost*>(mGame);
		    if (host) {
    			ADataStore* theDataStore = host->GetDataStore();
    			ThrowIfNil_(theDataStore);
    			theID = theDataStore->GetNewRecordID();	// the datastore will give assign us an id 
			} else {
			    DEBUG_OUT("Thingy::AssignUniqueID - Cast to HostDoc failed!!", DEBUG_ERROR | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE);
			}
      #ifndef GALACTICA_SERVER
		} else if (mGame->IsSinglePlayer()) {			// in single player games, don't need to write to the database,
		    GalacticaSingleDoc* game = dynamic_cast<GalacticaSingleDoc*>(mGame);
		    if (game) {
    			game->mLastThingyID++;
    			theID = game->mLastThingyID;
			} else {
			    DEBUG_OUT("Thingy::AssignUniqueID - Cast to SingleDoc failed!!", DEBUG_ERROR | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE);
			}
		} else {
		    // cheesy hack to get a unique number on multiplayer client. We take the tick count, which is
		    // the 60ths of a second since startup, and we make it a negative number (if it's not already)
		    // To make sure we can never get duplicates, we wait one tick before getting the count.
		    // normally this would suck, but this is only used in direct response to a user action, so
		    // restricting it to 60 calls/sec shouldn't be a problem.
		    ::Delay(1, (unsigned long*)&theID);
		    if (theID > 0) {
		        theID = -theID;
		    }
	  #endif // ! GALACTICA_SERVER
		}
		SetID(theID);						// we can use for calls to AddRecord(), etc..
		DEBUG_OUT("Assigned ID "<< theID << " for "<< this, DEBUG_DETAIL | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE); 
		FinishInitSelf();			// do anything that was waiting until pane ID was assigned
	}
}

void
AGalacticThingy::Persist() {
	DEBUG_OUT("Thingy::Persist " << this, DEBUG_DETAIL);
	ASSERT(mGame != NULL);
	ThrowIfNil_(mGame);
	SInt32 theID = GetID();
    if (mGame->IsHost() && !mGame->IsSinglePlayer()) {  // single player unhosted returns true for IsHost()
        WriteToDataStore();         // for host of multiplayer games, we write into database
  #ifndef GALACTICA_SERVER
    } else if (mGame->IsSinglePlayer()) {
        // single player games just do an assignment of ID
		if (theID == PaneIDT_Unspecified) {		// in single player games, don't need to write to the database,
    		AssignUniqueID();                   // so we can just assign ID's to any new objects and be done with it
		}
    } else {
        GalacticaClient* client = dynamic_cast<GalacticaClient*>(mGame);
        ASSERT(client != NULL);
        ThrowIfNil_(client);
        if (theID == PaneIDT_Unspecified) {
            // persisting a new thing to host requires we request it's creation
            client->SendCreateThingyRequest(this);
            // its data will be saved to the host after the permanent id is
            // received, in GalacticaClient::HandleCreateThingyResponse()
        } else {
            // otherwise we just send the data in an update
            client->SendThingy(this);
        }
  #endif // GALACTICA_SERVER
    }
}

ThingyDataPacket*
AGalacticThingy::WriteIntoPacket(SInt8 forPlayerNum) {
    ASSERT_STR(GetID() != PaneIDT_Unspecified, "Thingy::WriteIntoPacket precondition failed");
    // reset the stream and write the Thingy into it
   	sThingyIOStream->SetMarker(0, streamFrom_Start);
    if (forPlayerNum != kNoSuchPlayer) {
        // write data intended for a particular player
	    DEBUG_OUT("Thingy::WriteIntoPacket (for Player " << (int)forPlayerNum << ") " << this, DEBUG_DETAIL | DEBUG_PACKETS);
   	    WriteStream(sThingyIOStream);
	} else {
	    // write data intended for no particular player, for example intended for the server
	    DEBUG_OUT("Thingy::WriteIntoPacket " << this, DEBUG_DETAIL | DEBUG_PACKETS);
   	    WriteStream(sThingyIOStream);
    }
   	// figure out how much data there is and create the correctly sized packet
   	// shared I/O buffer, not thread safe
    long dataSize = sThingyIOStream->GetMarker();
    ThingyDataPacket* p = CreatePacket<ThingyDataPacket>(dataSize);
    if (!p) {
        DEBUG_OUT("Failed to allocate "<<dataSize<<" byte packet for "<<this, DEBUG_ERROR | DEBUG_PACKETS);
        return NULL;
    }
    // fill out the packet with the thingys data
    p->id = GetID();
    p->type = GetThingySubClassType();
    p->action = -1; // the caller should change this
    p->visibility = GetMaxVisibility();  // this will be ignored by the server
    if ((forPlayerNum != kNoSuchPlayer) && (forPlayerNum != mOwnedBy)) {
        // we are sending this data to a particular player that doesn't own us
        p->visibility = GetVisibilityTo(forPlayerNum);
    }

    char* dp = sThingyIOStream->GetDataPtr();
    std::memcpy(p->data, dp, dataSize);
  	return p;
}

bool
AGalacticThingy::ReadFromPacket(ThingyDataPacket* inPacket) {
	DEBUG_OUT("Thingy::ReadFromPacket " << this, DEBUG_DETAIL | DEBUG_PACKETS);
 #ifdef DUMP_ALL_PACKETS
    // do a hex dump of the bad packet
    char* outBuf = (char*)std::malloc(inPacket->packetLen * 5);
    if (outBuf) {
        BinaryDump(outBuf, inPacket->packetLen * 5, (char*) inPacket, inPacket->packetLen);
        DEBUG_OUT("--- start packet data ---\n" << outBuf << "\n---- end packet data ----", DEBUG_DETAIL);
        std::free(outBuf);
    }
 #endif // DUMP_ALL_PACKETS
    ASSERT(GetID() != PaneIDT_Unspecified);
    ASSERT(GetID() == inPacket->id);
    ASSERT(GetThingySubClassType() == (long)inPacket->type);
    ASSERT(inPacket->action != -1);
    // calculate how much data we have, and put it into the stream
    long dataSize = inPacket->packetLen - sizeof(ThingyDataPacket);
    SInt32 bufSize = sThingyIOStream->GetLength();
    if (dataSize > bufSize) {	// make sure the buffer is big enough
    	sThingyIOStream->SetLength(dataSize);
    }
    char* dp = sThingyIOStream->GetDataPtr();
    std::memcpy(dp, inPacket->data, dataSize);
    // reset the stream and read the thingy from it
    sThingyIOStream->SetMarker(0, streamFrom_Start);
    try {
    	GalacticaClient* client = GetGameClient();
        if (inPacket->action == action_Updated) {
            if (mGame->IsClient()) {
    	        UpdateClientFromStream(sThingyIOStream); // updates are handled in a special way
              #ifndef GALACTICA_SERVER
    	        if (client) {
    	            SetVisibilityTo(client->GetMyPlayerNum(), inPacket->visibility);
    	        }
    	      #endif // GALACTICA_SERVER
    	    } else {
    	        UpdateHostFromStream(sThingyIOStream);
    	    }
    	} else {
    	    ReadStream(sThingyIOStream); // everything other than update does overwrites
          #ifndef GALACTICA_SERVER
            if (client) {
    	        SetVisibilityTo(client->GetMyPlayerNum(), inPacket->visibility);
    	    }
  	      #endif // GALACTICA_SERVER
    	}
    }
    catch(std::exception e) {	// catch read errors here
    	    DEBUG_OUT("EXCEPTION "<< e.what() << " for " << this 
    	      << " usually indicates a we tried to read past end of record", 
    	      DEBUG_ERROR | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE);
        // do a hex dump of the bad packet
        char* outBuf = (char*)std::malloc(inPacket->packetLen * 5);
        if (outBuf) {
            BinaryDump(outBuf, inPacket->packetLen * 5, (char*) inPacket, inPacket->packetLen);
            DEBUG_OUT("--- start packet data ---\n" << outBuf << "\n---- end packet data ----", DEBUG_DETAIL);
            std::free(outBuf);
        }
        return false;
    }
    return true;
}

void
AGalacticThingy::WriteToDataStore() {
	DEBUG_OUT("Thingy::WriteToDataStore " << this << "; mGame="<<(void*)mGame, DEBUG_DETAIL);
	ASSERT(!mGame->IsSinglePlayer());
	ASSERT(!mGame->IsClient());
	ASSERT(mGame->IsHost());
	SInt32 theID = GetID();
    GalacticaHost* host = dynamic_cast<GalacticaHost*>(mGame);
    ThrowIfNil_(host);
   	bool update = false;
   	ADataStore* theDataStore = host->GetDataStore();
   	if (theID != PaneIDT_Unspecified) {		// We have to have a valid record id
   		if (theDataStore->RecordExists(theID)) {	// that exists in the database before we
   			update = true;					// can do an UpdateRecord()
   		}
   	} else {
   		AssignUniqueID();
   		theID = GetID();	// retrieve the new ID for later use
   	}
   	if (update) {
   		DEBUG_OUT("Thingy::WriteToDataStore - UpdateRecord " << this, DEBUG_TRIVIA | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE);
   	} else {
   		DEBUG_OUT("Thingy::WriteToDataStore - AddRecord " << this, DEBUG_TRIVIA | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE);
   	}
   #if SHARED_THINGY_IO_BUFFER
   	// this section of code does faster I/O because it doesn't have to allocate the
   	// handle or call the Handle based UpdateRecord() or AddRecord() calls, which have
   	// a lot of memory manager overhead. Since it shares a buffer, this makes it 
   	// UNSAFE for threads.
   	sThingyIOStream->SetMarker(dbRecSize_RecHeader + sizeof(long), streamFrom_Start);
   	WriteStream(sThingyIOStream);
    DatabaseRecPtr recP = (DatabaseRecPtr)sThingyIOStream->GetDataPtr();
   	recP->recID = theID;
   	recP->recSize = sThingyIOStream->GetMarker();
   	long* lp = (long*)recP->recData;
   	*lp = GetThingySubClassType();
   	if (update) {
   		theDataStore->UpdateRecord(recP);
   	} else {	// we didn't have a record, so we need to add it
   		theDataStore->AddRecord(recP);
   	}
   #else
   	// this section of code does slower I/O, which is thread safe since it allocates
   	// a unique memory block for each operation. It seems simple, but don't let that fool
   	// you. These Handle based UpdateRecord() and AddRecord() calls have a lot of Memory 
   	// Manager overhead.
   	LHandleStream theStream;
   	WriteStream(&theStream);
   	Handle h = theStream.GetDataHandle();
   	if (update) {
   		theDataStore->UpdateRecord(theID, h);
   	} else {	// we didn't have a record, so we need to add it
   		theDataStore->AddRecord(h, theID);
   	}
   #endif
}

// See GalacticaDoc::ReadFromDataStore() for info on use of this method
void
AGalacticThingy::ReadFromDataStore(RecIDT inNewID) {
    DEBUG_OUT("Thingy:ReadFromDS " << this, DEBUG_DETAIL | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE);
	ASSERT(!mGame->IsSinglePlayer());
	ASSERT(!mGame->IsClient());
	ASSERT(mGame->IsHost());
	if (inNewID == 0) {
		inNewID = GetID();	// preserve the old ID if none passed in
	}
    GalacticaHost* host = dynamic_cast<GalacticaHost*>(mGame);
    ThrowIfNil_(host);
    ADataStore* theDataStore = host->GetDataStore();
  #if SHARED_THINGY_IO_BUFFER
    // this section of code does faster I/O because it doesn't have to allocate the
    // handle or call the Handle based ReadRecord(), which has a lot of memory manager
    // overhead. Since it shares a buffer, this makes it UNSAFE for threads.
    SInt32 recSize = theDataStore->GetRecordSize(inNewID);
    SInt32 bufSize = sThingyIOStream->GetLength();
    if (recSize > bufSize) {	// make sure the buffer is big enough
    	sThingyIOStream->SetLength(recSize);
    }
    DatabaseRecPtr recP = (DatabaseRecPtr)sThingyIOStream->GetDataPtr();
    recP->recID = inNewID;
    recP->recSize = 0;	// we are sure the buffer is big enough
    theDataStore->ReadRecord(recP);
    // set the stream marker at the beginning of the Thingy data, skipping the Record Header
    sThingyIOStream->SetMarker(dbRecSize_RecHeader + sizeof(long), streamFrom_Start);
    try {
    	ReadStream(sThingyIOStream);
    }  
  #else
    // this section of code does slower I/O, which is thread safe since it allocates
    // a unique memory block for each operation. It seems simple, but don't let that fool
    // you. This Handle based ReadRecord() has a lot of Memory Manager overhead.
    Handle h = theDataStore->ReadRecord(inNewID);
    LHandleStream theStream(h);	// handle is disposed of when stream goes out of scope
    try {
    	ReadStream(&theStream);
    //		ASSERT(inNewID == mPaneID);
    }
  #endif
    catch(std::exception& e) {	// catch read errors here
    	DEBUG_OUT("EXCEPTION "<< e.what() << " for " << this << " usually indicates a we tried to read past end of record", DEBUG_ERROR | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE);
    }
    if (inNewID != GetID()) {
    	DEBUG_OUT("ID requested is not ID loaded!! Probably a database error.", DEBUG_ERROR | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE);
    	DEBUG_OUT("  look in the database to see if record "<<inNewID<<" exists, and also", DEBUG_ERROR | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE);
    	DEBUG_OUT("  check record "<<mID<<" which seems to have overwritten it.", DEBUG_ERROR | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE);
    }
 	long* lp = (long*)recP->recData;
 	long thingyType = BigEndian32_ToNative(*lp);
 	ASSERT(thingyType == GetThingySubClassType());
}


// only call from host
void
AGalacticThingy::DeleteFromDataStore() {
    DEBUG_OUT("Thingy:DeleteFromDataStore " << this, DEBUG_DETAIL | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE);
	ASSERT(!mGame->IsSinglePlayer());
	ASSERT(!mGame->IsClient());
	ASSERT(mGame->IsHost());
    GalacticaHost* host = dynamic_cast<GalacticaHost*>(mGame);
    ThrowIfNil_(host);
    ADataStore* theDataStore = host->GetDataStore();
    ThrowIfNil_(theDataStore);
    try {
    	theDataStore->DeleteRecord(GetID());
    }
    catch(LException& e) {
    	if (e.GetErrorCode() != dbItemNotFound) {	// item not found is okay, because we were deleting it
    		throw e;				// anyway
    	} else {
    		DEBUG_OUT("EXCEPTION: Record " << GetID() << " Not Found.", DEBUG_ERROR | DEBUG_EOT | DEBUG_OBJECTS | DEBUG_DATABASE);
    	}
    }
}



void
AGalacticThingy::DoComputerMove(long) {	// only called from host doc
	// default move for the computer is to do nothing
}

// server should wait for all players to end turn, then process end of turn
// client should end turn, then poll the server periodically to ask if turn complete
//  when client learns turn is complete, it should ask for all the changes.

// makes sure the host processing is completed properly
// should only be called by the hosting game


void
AGalacticThingy::PrepareForEndOfTurn(long) { // inTurnNum
    // make sure we start the end of turn processing with these flags clean
    mScannerChanged = false;
    mNeedsRescan = false;
}

void
AGalacticThingy::EndOfTurn(long) { // inTurnNum
}

void
AGalacticThingy::FinishEndOfTurn(long) { // inTurnNum) {
	// this method is called after all the end of turn processing is complete and everything
	// has had a chance to move. It is called by the host doc after EndOfTurn() has been
	// called for each item, but before the changes have been written to the DataStore
	
	// in general, this should be used to do any minor changes that depend on everything
	// having moved already. Don't use this function to do things that affect other items
	// if those things can possibly be done in the EOT function
	
	// an example of the use of this function is to call CalcMoveDelta for all the ships
	// to make sure that if their target has moved or been redrawn the course vector will be
	// updated correctly. 
}


void
AGalacticThingy::DestroyedInCombat(SInt32 inByPlayer, ESendEvent sendEvent) {
	Waypoint wp = mCurrPos;	// save the current position
	bool bSendEvent = false;
	DEBUG_OUT("Destroyed " << this, DEBUG_DETAIL | DEBUG_COMBAT | DEBUG_MOVEMENT | DEBUG_EOT | DEBUG_OBJECTS);
	SInt16 eventCode = GetDestructionEventCode();
	mNeedsDeletion = 1;
			// it is important that we tell the dead thing to it needs deletion BEFORE the 
			// destruction message has been created. That way the Subject Name will be 
			// correctly filled out before the event is sent to the client.
	EWaypointType type = mCurrPos;		// where were we when destroyed?
	if (sendEvent == autoDetermineSend) {
    	if (type != wp_Fleet) {				// only send death notice if not part of a fleet
    		if (type == wp_Star) {
    			AGalacticThingy* at = mCurrPos;				// if at a star, don't send the
    			if (at && (at->GetOwner() != mOwnedBy)) {	// death notice if it is our own star
    				bSendEvent = true;						// that way we will only see it if we
    			}											// are attacking an enemy star
    		} else {
    			bSendEvent = true;
    		}
    	}
	} else {
	    bSendEvent = (sendEvent == alwaysSend);
	}
	if (bSendEvent) {
		if ((type == wp_Ship) || (type == wp_Fleet)) {	// see if it was a ship. If so,
			mCurrPos = mCurrPos.GetCoordinates();		// we need to get its coordinates instead.
			// NOTE: it is critical that we restore the changed coordinates before we call Die(),
			// or the item will not be correctly removed from its container
		}
		CMessage::CreateEvent(GetGameHost(), eventCode, mOwnedBy, this, inByPlayer, inByPlayer, mOwnedBy);
	}
	mCurrPos = wp;	// restore the changed coordinates
	Die();			// now remove ourself from the current container
}

void	
AGalacticThingy::Scrapped() {
	// do nothing when scrapped by default
}

// inWeighting is the percent bonus to one or the other
long
AGalacticThingy::DefendAgainst(AGalacticThingy* inAttacker, int inWeighting, long rolloverDamage) {
//	CDestructionMsg *aDestruct = nil;
	if (!ValidateThingy(inAttacker)) {
		return 0;
	}
	long damage;
	if (rolloverDamage)	{			// if we have rollover damage from an earlier attack, don't generate a new attack,
		damage = rolloverDamage;	// just apply the rollover damage
	} else {
		damage = (inAttacker->GetOneAttackAgainst(this) * inWeighting) / 100;
	}
	if (damage < 0) {
		DEBUG_OUT("Attacker did negative damage!", DEBUG_ERROR | DEBUG_COMBAT);
	}
	if (!ValidateThingy(this)) {
		return damage;
	}
	if (IsDead()) {
		return damage;
	}
	long extraDamage = TakeDamage(damage);
	DEBUG_OUT(this <<" took "<<damage<<" damage from Attacker "<< inAttacker <<"; "<<extraDamage<<" points will rollover",
				DEBUG_TRIVIA | DEBUG_COMBAT);
	if (GetDefenseStrength() < 0) {
		DestroyedInCombat(inAttacker->GetOwner());	// this thingy was killed by its attacker
	}
	return extraDamage;	// return amount of excess damage that can be given to other ships
}

long
AGalacticThingy::GetOneAttackAgainst(AGalacticThingy* ) { //inDefender) {
	long attackDamage = (GetAttackStrength() * (Rnd(100)+Rnd(98)+Rnd(99)+3))/300;
	if (attackDamage <0) {
		attackDamage = 0;
	}
	return attackDamage;
}

long
AGalacticThingy::TakeDamage(long inHowMuch) {
	Changed();
	mDamage += inHowMuch;
	long strength = GetDefenseStrength();
	return (strength > 0) ? 0 : -strength;
}

UInt8 
AGalacticThingy::CalculateVisibilityToThingy(AGalacticThingy* inThingy) {
    if (GetMaxVisibility() == visibility_None) {
        // don't do any calculations in this case, this object is always invisible
        return visibility_None;
    }
    UInt8 vis;
    double base_vis = ((double)GetApparentSize() * (double)inThingy->GetScannerRange()) / ((double)Distance(inThingy) * (double)GetStealth());
    if (base_vis >= 200.0) {
    	vis = visibility_Details;
    } else if (base_vis >= 100.0) {
        vis = visibility_Class;
    } else if (base_vis >= 50.0) {
        vis = visibility_Tech;
    } else if (base_vis >= 25.0) {
        vis = visibility_Speed;
    } else {
        vis = visibility_None;
    }
    if (vis > GetMaxVisibility()) {
        vis = GetMaxVisibility();
    }
    if (vis < GetMinVisibility()) {
        vis = GetMinVisibility();
    }
    DEBUG_OUT("  CalculateVisibilityToThingy: "<<this<<" has visibility "<<(int)vis<<" to "<<inThingy, DEBUG_TRIVIA);
    return vis;
}

void
AGalacticThingy::CalculateVisibilityToOpponent(SInt16 opponent) {
  //    - get and cache list of all things for opponent, sorted in order of 
  //      scanner range
  //    - set vis & spotter to 0
  //    - for each item in list, started with highest scan strength
  //        - calc visibility of this as seen by list item
  //        - if visibility > max vis
  //            - set vis to max vis
  //            - save list item as spotter, RETURN
  //        - if visibility > current vis
  //            - save vis & spotter, next ITEM
    if (IsAllyOf(opponent) || !mGame || !mGame->HasFogOfWar()) {
        SetVisibilityTo(opponent, GetMaxVisibility());
        return;
    }
    std::vector<AGalacticThingy*>* list = mGame->GetScannersForPlayer(opponent);
    std::vector<AGalacticThingy*>::iterator i = list->begin();
    int turn = mGame->GetTurnNum();
    SetVisibilityTo(opponent, 0);
    SetLastSeenByID(opponent, kInvalidRecID);
    while (i != list->end()) {
        AGalacticThingy* aThingy = *i;
        UInt8 myVis = CalculateVisibilityToThingy(aThingy);
        if (myVis >= GetMaxVisibility()) {
    		DEBUG_OUT("CalculateVisibilityToOpponent found opponent "<<opponent<<" that can see "<<this<<" perfectly", DEBUG_DETAIL);
            SetVisibilityTo(opponent, myVis);
            SetLastSeen(opponent, turn);
            SetLastSeenByID(opponent, aThingy->GetID());
            return; // no one will be able to see us better than this, so stop looking
        }
        if (myVis > GetVisibilityTo(opponent)) {
    		DEBUG_OUT("CalculateVisibilityToOpponent found opponent "<<opponent<<" that can see "<<this<<" better", DEBUG_DETAIL);
            SetVisibilityTo(opponent, myVis);
            SetLastSeen(opponent, turn);
            SetLastSeenByID(opponent, aThingy->GetID());
        }
        i++;
    }
    
}

void
AGalacticThingy::CalculateVisibilityToAllOpponents() {
  // call when nothing has visibiltiy set
  // - for each opponent
  //    - call CalcVisToOpponent
    for (int i = 1; i <= mGame->GetNumPlayers(); i++) {
    	if (!IsAllyOf(i)) {
        	CalculateVisibilityToOpponent(i);
        }
    }
    mNeedsRescan = false;   // we don't need a rescan anymore
}

void
AGalacticThingy::UpdateWhatIsVisibleToMe() {
  // call when something moves or changes it's ability to see
  // - for all items
  //    - if item.spotter == me
  //        - item.CalcVisToOpponent(me.owner)
  //        - next item
  //    - calc visibility of item as seen by me
  //    - if vis > item.getVis(me.owner)
  //        - save vis & spotter
    if (HasScanners() && mGame->HasFogOfWar()) {
    	DEBUG_OUT("AGalacticThingy::UpdateWhatIsVisibleToMe " << this, DEBUG_DETAIL);
    	LArrayIterator iterator(*mGame->GetEverythingList(), LArrayIterator::from_Start); // Search list of all thingys
    	AGalacticThingy* aThingy;
    	UInt32 turnNum = mGame->GetTurnNum();
    	while (iterator.Next(&aThingy)) {
    		if (!ValidateThingy(aThingy)) {
    		    DEBUG_OUT("UpdateWhatIsVisibleToMe Found Invalid Thingy "<<aThingy<<" in Host everything list, removing...", DEBUG_ERROR | DEBUG_EOT);
    		    mGame->GetEverythingList()->Remove(aThingy);
    		    continue; // do nothing more with this invalid item lest we crash
    		} else if (!aThingy->IsAllyOf(mOwnedBy)) {
    			// if this is an ally we can see it fully anyway
	    		if (aThingy->GetLastSeenByID(mOwnedBy) == mID) {
	    		    // if this item was last seen by me, then recalculate it completely and move on
	    		    DEBUG_OUT("UpdateWhatIsVisibleToMe Found thingy "<<aThingy<<" that we last spotted, recalcing", DEBUG_DETAIL);
	    		    aThingy->CalculateVisibilityToOpponent(mOwnedBy);
	    		} else {
	    		    // figure out how much we can see the thing
	    		    UInt8 visibility = aThingy->CalculateVisibilityToThingy(this);
	    		    if (visibility > aThingy->GetVisibilityTo(mOwnedBy)) {
	    		        DEBUG_OUT("UpdateWhatIsVisibleToMe Found thingy "<<aThingy<<" that we see better, updating", DEBUG_DETAIL);
	    		        // if we can see this better than anyone else who has looked at it so far
	    		        // then make us the spotter and update visibility
	    		        aThingy->SetLastSeen(mOwnedBy, turnNum);
	    		        aThingy->SetLastSeenByID(mOwnedBy, mID);
	    		        aThingy->SetVisibilityTo(mOwnedBy, visibility);
	    		    }
	    		}
	    	}
    	}
	}
	mScannerChanged = false;    // our scanner info has been updated
}

bool
AGalacticThingy::IsAllyOf(UInt16 inOwner) const {
    if (mOwnedBy == inOwner) return true;
    if (inOwner == (UInt16)kAdminPlayerNum) return true;
#ifndef GALACTICA_SERVER
  #ifdef DEBUG
    if (IsShiftKeyDown() && IsControlKeyDown()) return true;
  #endif
#endif // GALACTICA_SERVER
    return false;
}

Point3D
AGalacticThingy::GetCoordinates() const {
	return mCurrPos.GetCoordinates();
}

void
AGalacticThingy::SetCoordinates(const Point3D inCoordinates, bool inRecalc) {
	mCurrPos = inCoordinates;
	if (inRecalc) {
		ThingMoved(kDontRefresh);
	}
}

bool
AGalacticThingy::IsOfContentType(EContentTypes inType) const {
	return (inType == contents_Any);
}

bool
AGalacticThingy::SupportsContentsOfType(EContentTypes) const { //inType
	return false;
}

void*
AGalacticThingy::GetContentsListForType(EContentTypes) const { //inType
	return NULL;
}

SInt32
AGalacticThingy::GetContentsCountForType(EContentTypes) const { //inType
	return 0;
}

bool
AGalacticThingy::AddContent(AGalacticThingy*, EContentTypes, bool) {	//inThingy, inType, inRefresh
	return false;
	// default does nothing
}

void
AGalacticThingy::RemoveContent(AGalacticThingy*, EContentTypes, bool) {	//inThingy, inType, inRefresh
	// default does nothing
}

bool
AGalacticThingy::HasContent(AGalacticThingy*, EContentTypes) {	//inThingy, inType
	return false;	// default assumes we have no contents
}

#warning obsolete this method in favor of the std::vector implementation below
void
AGalacticThingy::AddEntireContentsToList(EContentTypes inType, LArray& ioList) {
	if (IsOfContentType(inType)) {							// if this object is the correct type
	    AGalacticThingy* it = this;
		ioList.InsertItemsAt(1, LArray::index_Last, &it);	// have it add itself to the output list
	}
	if (SupportsContentsOfType(inType)) {
		CThingyList* tempList = static_cast<CThingyList*>(GetContentsListForType(inType));	// get the standard list of items contained
		if (tempList) {									// of a particular type, if there are any
			AGalacticThingy* it;
			UInt32 numRefs = tempList->GetCount();
			for(UInt32 i = 1; i <= numRefs; i++) {				// go through the contained items list:
				it = tempList->FetchThingyAt(i);		// and get them to add all their contents.
				if (!it) {
					continue;
				}
				it->AddEntireContentsToList(inType, ioList);
			}								// note: this assumes no duplicate entries.
		}									// That is, no item can appear in more than one list
	}										// or appear more than once in a single list
}

void
AGalacticThingy::AddEntireContentsToList(EContentTypes inType, StdThingyList& ioList) const {
	if (IsOfContentType(inType)) {			// if this object is the correct type
		ioList.push_back(const_cast<AGalacticThingy*>(this));	            // have it add itself to the output list
	}
	if (SupportsContentsOfType(inType)) {
		CThingyList* tempList = static_cast<CThingyList*>(GetContentsListForType(inType));	// get the standard list of items contained
		if (tempList) {									// of a particular type, if there are any
			AGalacticThingy* it;
			UInt32 numRefs = tempList->GetCount();
			for(UInt32 i = 1; i <= numRefs; i++) {				// go through the contained items list:
				it = tempList->FetchThingyAt(i);		// and get them to add all their contents.
				if (!it) {
					continue;
				}
				it->AddEntireContentsToList(inType, ioList);
			}								// note: this assumes no duplicate entries.
		}									// That is, no item can appear in more than one list
	}										// or appear more than once in a single list
}

// return highest level container within which this thingy is found. If this thingy is
// not inside any other container, it will return itself.
AGalacticThingy*
AGalacticThingy::GetTopContainer() {	
	AGalacticThingy* last;
	AGalacticThingy* it = this;
	int count = 100;	// no more than 100 nested levels
	do {
		last = it;
		it = last->GetContainer();
	} while (it && count--);
	return last;
}

// implementation assumes that GetContentsListForType(inType) returns a CThingyList
// if not, you will need to override
void*
AGalacticThingy::SearchContents(EContentTypes inType, ContainerSearchFunc funcPtr, void* searchData) const {
	if (SupportsContentsOfType(inType)) {
		CThingyList* tempList = static_cast<CThingyList*>(GetContentsListForType(inType));
		if (tempList) {
			const AGalacticThingy* it;
			UInt32 numRefs = tempList->GetCount();
			for(UInt32 i = 1; i <= numRefs; i++) {		// go through the contained items list:
				it = tempList->FetchThingyAt(i);		// and see if they match
				if (!it) {
					continue;
				}
				if (funcPtr(it, searchData)) {
					return (void*)it;
				}
			}
		}
	}
	return 0; // no match found
}

UInt8 
AGalacticThingy::GetVisibilityTo(UInt16 inOpponent) const {
    if ((inOpponent > 0) && (inOpponent <= MAX_PLAYERS_CONNECTED)) {
        return mVisibilityTo[inOpponent-1];
    } else {
        return visibility_None;
    }
}

void 
AGalacticThingy::SetVisibilityTo(UInt16 inOpponent, UInt8 visibility) {
    if ((inOpponent > 0) && (inOpponent <= MAX_PLAYERS_CONNECTED)) {
        UInt8 oldVis = mVisibilityTo[inOpponent-1];
        if (visibility >= visibility_Max) {
            visibility = visibility_Max - 1;
        }
        if (visibility > GetMaxVisibility()) {
            visibility = GetMaxVisibility();
        }
        if (visibility < GetMinVisibility()) {
            visibility = GetMinVisibility();
        }
        mVisibilityTo[inOpponent-1] = visibility;
        if (visibility != oldVis) {
            Changed(); // make sure we know this thing was changed 
        }
    }
}

UInt32 
AGalacticThingy::GetLastSeen(UInt16 inOpponent) const {
    if ((inOpponent > 0) && (inOpponent <= MAX_PLAYERS_CONNECTED)) {
        return mLastSeen[inOpponent-1];
    } else {
        return (UInt32)lastSeen_Never;
    }
}

void                
AGalacticThingy::SetLastSeen(UInt16 inOpponent, UInt32 turnNum) {
    if ((inOpponent > 0) && (inOpponent <= MAX_PLAYERS_CONNECTED)) {
        if (turnNum == lastSeen_ThisTurn) {
            turnNum = mGame->GetTurnNum();
        }
        mLastSeen[inOpponent-1] = turnNum;
    }
}

SInt32              
AGalacticThingy::GetLastSeenByID(UInt16 inOpponent) const {
    if ((inOpponent > 0) && (inOpponent <= MAX_PLAYERS_CONNECTED)) {
        return mLastSeenByID[inOpponent-1];
    } else {
        return kInvalidRecID;
    }
}

void                
AGalacticThingy::SetLastSeenByID(UInt16 inOpponent, SInt32 thingyID) {
    if ((inOpponent > 0) && (inOpponent <= MAX_PLAYERS_CONNECTED)) {
        mLastSeenByID[inOpponent-1] = thingyID;
    }
}

ResIDT
AGalacticThingy::GetViewLayrResIDOffset() {
	return 0;	// default returns no offset
}

long
AGalacticThingy::GetProduction() const {
	return 0;	// default returns no production
}

long
AGalacticThingy::GetAttackStrength() const {
	return ((long)mTechLevel * 50) - (mDamage/4);
}

long
AGalacticThingy::GetDefenseStrength() const {
	return ((long)mTechLevel * 100) - mDamage;
}

long
AGalacticThingy::GetDangerLevel() const {
	return 0;	// default returns no danger level
}


bool
AGalacticThingy::IsMyPlayers() const {
  #ifndef GALACTICA_SERVER
    GalacticaDoc* game = GetGameDoc();
    if (game) {
        return (mOwnedBy == game->GetMyPlayerNum());
    } else {
        return false;
    }
  #else
    return false;  // never owned by the server
  #endif // GALACTICA_SERVER
}

void
AGalacticThingy::Changed() {
    mChanged = true;
} 

void
AGalacticThingy::SetGame(GalacticaShared* inGameDoc) {
//	ASSERT_STR(mGame == nil, "Warning: Called SetGame() for "<<this<<"; already assigned to game");
	if (mGame == NULL) { 
		mGame = inGameDoc;
		if (mGame) {
			mGame->AddThingy(this);	// make sure the game knows we exist
		}
	}
#ifdef DEBUG
	else if (mGame != inGameDoc) {
	 	DEBUG_OUT("Attempted illegal reassignment of "<<this<<" to different game. Currently in "<<(void*)mGame, DEBUG_ERROR);
	}
#endif
}

GalacticaDoc*
AGalacticThingy::GetGameDoc() const {
  #ifndef GALACTICA_SERVER
    return dynamic_cast<GalacticaDoc*>(mGame);
  #else
    return NULL;
  #endif // GALACTICA_SERVER
}

// only call from host
GalacticaProcessor*
AGalacticThingy::GetGameHost() const {
    return dynamic_cast<GalacticaProcessor*>(mGame);
}

// only call from client
GalacticaClient*
AGalacticThingy::GetGameClient() const {
  #ifndef GALACTICA_SERVER
    return dynamic_cast<GalacticaClient*>(mGame);
  #else
    return NULL;
  #endif // GALACTICA_SERVER
}

void
AGalacticThingy::ThingMoved(bool) {  // inRefresh
	if (!IsDead()) {
		DEBUG_OUT("ThingMoved() "<<this<<"; calling Changed()", DEBUG_TRIVIA | DEBUG_MOVEMENT | DEBUG_EOT);
		Changed();
	}
}

bool
AGalacticThingy::RefersTo(AGalacticThingy *inThingy) {
	ASSERT(inThingy != nil);
	return (inThingy == mCurrPos);
}

// remove all references we have to a thing
void
AGalacticThingy::RemoveReferencesTo(AGalacticThingy *inThingy) {
    RemoveContent(inThingy, contents_Any, kDontRefresh);
}

void
AGalacticThingy::ChangeIdReferences(PaneIDT oldID, PaneIDT newID) {
    if (mCurrPos.ChangeIdReferences(oldID, newID)) {
        DEBUG_OUT("Updated position of "<<this<<" (was at "<<oldID<<")", DEBUG_DETAIL | DEBUG_CONTAINMENT);
    }
}

// take something that has already been marked as dead completely out of the game,
// including removing all references to it from other items.
// This is usually just before deleting an item.
void
AGalacticThingy::RemoveSelfFromGame() {
	DEBUG_OUT("Remove from Game " << this << " @ "<< mCurrPos, DEBUG_DETAIL | DEBUG_OBJECTS | DEBUG_CONTAINMENT);
    ASSERT(IsDead());
	// now remove self from anything that refers to this
	URemoveReferencesAction removeRefsAction(this);
	mGame->DoForEverything(removeRefsAction);
	// set our position to our location in space
	mCurrPos = mCurrPos.GetCoordinates();
}

SInt32
AGalacticThingy::Distance(Point3D &inCoordinates) {
	return mCurrPos.Distance(inCoordinates);
}

SInt32
AGalacticThingy::Distance(AGalacticThingy *anotherThingy) {
	return mCurrPos.Distance(anotherThingy);
}



