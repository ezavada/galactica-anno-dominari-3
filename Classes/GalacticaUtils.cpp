// GalacticaUtils.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 5/6/96 ERZ

// еее explanations of Galactic coordinates in GalacticaUtils.h еее
 
#include "GalacticaUtils.h"

#include <LString.h>
#include <LStream.h>
#include <LArray.h>

//#include <TypeInfo.h>

#include "GalacticaConsts.h"
#include "GalacticaProcessor.h"
#include "CFleet.h"

#ifndef GALACTICA_SERVER
#include "GalacticaDoc.h"
#endif // GALACTICA_SERVER

#ifdef DEBUG
  #if PLATFORM_MACOS && COMPILER_MWERKS		// ERZ, v1.2b8, DebugNew only supported on Metrowerks MacOS
    #include <DebugNew.h>
  #endif
#endif // DEBUG

#include <ctime>
#include <cmath>

#if COMPILER_GCC
// gcc doesn't have std::hypot
namespace std {
    inline unsigned long hypot(long dx, long dy) {
        return (unsigned long) ::hypot( (double) dx, (double) dy);
    }
}
#endif // COMPILER_GCC

// if THINGYREF_REMEMBER_LOOKUPS is true, once a ThingyRef looks up the
// the thingy it references in the Everything list, it stores a pointer to
// it. Otherwise, it has to do a lookup by ID number every time. 
#define THINGYREF_REMEMBER_LOOKUPS	true


#define      FIXEDDECIMAL   ((char)(1))


bool	ThingyIsValid(const AGalacticThingy* inThingy);

#pragma mark === globals ===

const Point3D gNullPt3D = {0,0,0};
const Point3D gNowherePt3D = {-1,-1,-1};
short gGalacticaFontNum;


// these are defined in Galactica.cp
extern UInt32	gAGalacticThingyClassNum;
extern UInt32	gAGalacticThingyUIClassNum;
extern UInt32	gANamedThingyClassNum;
extern UInt32	gCShipClassNum;
extern UInt32	gCStarClassNum;
extern UInt32	gCMessageClassNum;
extern UInt32	gCDestructionMsgClassNum;
extern UInt32	gCStarLaneClassNum;
extern UInt32	gCFleetClassNum;
extern UInt32	gCWormholeClassNum;
extern UInt32	gCRendezvousClassNum;
extern UInt32	gTopMem;

typedef unsigned long *classNumPtr, **classNumPtrPtr;

bool
ThingyIsValid(const AGalacticThingy* inThingy) {
	bool result = false;
	if (inThingy && ((UInt32)inThingy < gTopMem)) {
		classNumPtr clp = *(classNumPtrPtr)inThingy;
		if ( clp && ((UInt32)clp < gTopMem) 
		         && ( (((UInt32)clp & 0x01) == 0) )  ) {
			unsigned long classNum = *clp;
			if (classNum == gCShipClassNum)
				result = true;
			else if (classNum == gCStarClassNum)
				result = true;
			else if (classNum == gCFleetClassNum)
				result = true;
			else if (classNum == gCRendezvousClassNum)
				result = true;
			else if (classNum == gCMessageClassNum)
				result = true;
			else if (classNum == gCDestructionMsgClassNum)
				result = true;
			else if (classNum == gAGalacticThingyClassNum)
				result = true;
			else if (classNum == gAGalacticThingyUIClassNum)
				result = true;
//			else if (classNum == gANamedThingyClassNum)
//				result = true; // named thingy is an abstract class, not valid
		}
	}
	return result;
}

AGalacticThingy* 
ValidateThingy(const AGalacticThingy* inThingy) {
	if (ThingyIsValid(inThingy)) {
		return const_cast<AGalacticThingy*>(inThingy);
	} else {
#ifdef DEBUG
		if (inThingy) {
			DEBUG_OUT("Invalid Thingy Detected {"<<(void*)inThingy<<"}", DEBUG_ERROR);
//		  #if TARGET_OS_MAC && !TARGET_API_MAC_CARBON
//			DebugNewValidatePtr((void*)inThingy);
//		  #endif
		}
#endif
		return NULL;
	}
}

CMessage* 
ValidateMessage(const void* inMessage) {
	if (ThingyIsValid((AGalacticThingy*)inMessage)) {
		long type = ((AGalacticThingy*)inMessage)->GetThingySubClassType();
		if ((type == thingyType_Message) || (type == thingyType_DestructionMessage)) { // explicitly check for known message types
			return (CMessage*)inMessage;
		} else {
			DEBUG_OUT((AGalacticThingy*)inMessage << " is not a message", DEBUG_TRIVIA | DEBUG_MESSAGE);
			return nil;
		}
	} else {
#ifdef DEBUG
		if (inMessage) {
			DEBUG_OUT("Invalid Message Detected {"<<(void*)inMessage<<"}", DEBUG_ERROR | DEBUG_MESSAGE);
//		  #if TARGET_OS_MAC && !TARGET_API_MAC_CARBON
		  		// ERZ v1.2b8, DebugNew only supported on MacOS
		  		// ERZ v2.1d4, DebugNewValidatePtr doesn't work under OS X
//			DebugNewValidatePtr((void*)inMessage);
//		  #endif
		} else {
//			DEBUG_OUT("NIL Message Detected", DEBUG_DETAIL | DEBUG_MESSAGE);
		}
#endif
		return nil;
	}
}

CShip* 
ValidateShip(const void* inShip) {
	if (ThingyIsValid((AGalacticThingy*)inShip)) {
		long type = ((AGalacticThingy*)inShip)->GetThingySubClassType();
		if ((type == thingyType_Ship) || (type == thingyType_Fleet)) {
			return (CShip*)inShip;
		} else {
			DEBUG_OUT((AGalacticThingy*)inShip << " is not a ship", DEBUG_TRIVIA);
			return nil;
		}
	} else {
#ifdef DEBUG
		if (inShip) {
			DEBUG_OUT("Invalid Ship Detected {"<<(void*)inShip<<"}", DEBUG_ERROR);
//		  #if TARGET_OS_MAC && !TARGET_API_MAC_CARBON
		  		// ERZ v1.2b8, DebugNew only supported on MacOS
		  		// ERZ v2.1d4, DebugNewValidatePtr doesn't work under OS X
//			DebugNewValidatePtr((void*)inShip);
//		  #endif
		} else {
//			DEBUG_OUT("NIL Ship Detected", DEBUG_DETAIL);
		}
#endif
		return nil;
	}
}

#pragma mark-

#ifdef DEBUG
	long gThingyRefCacheNils = 0;		// there was no item cached, this was just a nil reference
	long gThingyRefCacheHits = 0;		// num times the data we wanted was in the cache
	long gThingyRefCacheMisses = 0;		// num times we had to lookup an item. Should equal LookupOK + LookupNil
	long gThingyRefCacheInvalids = 0;	// would-be Hits that were Misses because of expired cache data
	long gThingyRefCacheGets = 0;		// num calls to GetThingy(). Should equal Nils + Hits + Misses 
	long gThingyRefCacheLookupOK = 0;		// num times lookup of item returned valid thingy
	long gThingyRefCacheLookupNil = 0;		// num times lookup of item returned nil (no item found)
#endif

// this had some serious problems (don't recall what), so I removed it, ERZ v2.1
/*inline ThingyRef& 
ThingyRef::operator= (const PaneIDT inID) {
    DEBUG_OUT("Illegal thingy ref assignment with NULL game", DEBUG_ERROR);
//	SetThingyID(inID, GalacticaShared::GetStreamingGame()); 
	return *this;
}
*/

inline int 
ThingyRef::operator== (const AGalacticThingy *inThingy) {
	return (inThingy->GetID() == mThingyID);
}

inline int 
ThingyRef::operator!= (const AGalacticThingy *inThingy) {
	return (inThingy->GetID() != mThingyID);
}

void 
ThingyRef::SetThingy(const AGalacticThingy *inThingy)  {
	ThrowIfNil_(inThingy);
	mThingyID = inThingy->GetID(); 
  #if THINGYREF_REMEMBER_LOOKUPS
	mThingy = const_cast<AGalacticThingy*>(inThingy); 
  #else
	mThingy = NULL; 
  #endif
	mGame = inThingy->GetGame();
	ThrowIfNil_(mGame);
	mValidTurn = mGame->GetTurnNum();	// find out when this was last guaranteed valid
}

void 
ThingyRef::SetThingyID(const PaneIDT inID, GalacticaShared * inGame) {
	mThingyID = inID; 
	mThingy = NULL; 
	mGame = inGame;
  #ifdef DEBUG
	ThrowIfNil_(mGame);
  #endif
	mValidTurn = mGame->GetTurnNum();
}

void 
ThingyRef::SetGame(GalacticaShared* inGame) {
	mThingy = NULL; 
	mGame = inGame;
  #ifdef DEBUG
	ThrowIfNil_(mGame);
  #endif
	mValidTurn = -1;
}

void 
ThingyRef::ReadStream(LStream& inInputStream, UInt32 streamVersion) {
	inInputStream >> mThingyID;
	mThingy = nil;
	mGame = GalacticaShared::GetStreamingGame();
	mValidTurn = -1;
  #ifdef DEBUG
    if (!mGame) {
	    DEBUG_OUT("Illegal Read ThingyRef with null game", DEBUG_ERROR);
	}
	ThrowIfNil_(mGame);
  #endif
}

// only used for saved or transmitted structures, stream handles byte swapping for us
void 
ThingyRef::ByteSwapOutgoing() throw() {
    mThingyID = Native_ToBigEndian32(mThingyID);
    // all other values can't be saved, zero them
    mThingy = 0;
    mValidTurn = 0;
    mGame = 0;
}

// only used for saved or transmitted structures, stream handles byte swapping for us
void
ThingyRef::ByteSwapIncoming() throw() {
    mThingyID = BigEndian32_ToNative(mThingyID);
    // all other values can't be saved, assert that they are zero
    ASSERT(mThingy == 0);
    ASSERT(mValidTurn == 0);
    ASSERT(mGame = 0);
}

AGalacticThingy* 
ThingyRef::GetThingy() const {
  #ifdef DEBUG
	gThingyRefCacheGets++;
  #endif
	if (mThingyID == 0) {	// no id, there is no thingy referenced
	  #ifdef DEBUG
		gThingyRefCacheNils++;
	  #endif
		return nil;
	} else if (mThingy) {
		if (mValidTurn != mGame->GetTurnNum()) {
		  #ifdef DEBUG
			gThingyRefCacheInvalids++;
		  #endif
			goto DO_LOOKUP;	// the reference was cached on a different turn, must look it up#ifdef DEBUG
		}
	  #ifdef DEBUG
		gThingyRefCacheHits++;
	  #endif
		return mThingy;
	} else {
  DO_LOOKUP:
	  #ifdef DEBUG
		gThingyRefCacheMisses++;
	  #endif
		AGalacticThingy* it =  mGame->FindThingByID(mThingyID);
	  #ifdef DEBUG
		if (it) {
			gThingyRefCacheLookupOK++;
		} else {
			gThingyRefCacheLookupNil++;
		}
	  #endif
	  #if THINGYREF_REMEMBER_LOOKUPS
		(const_cast<ThingyRef*>(this))->mThingy = it;
		(const_cast<ThingyRef*>(this))->mValidTurn = mGame->GetTurnNum();
	  #else
		(const_cast<ThingyRef*>(this))->mThingy = nil;
	  #endif
		return it;
	}
}

GalacticaDoc*       
ThingyRef::GetGameDoc() const {
  #ifndef GALACTICA_SERVER
    return dynamic_cast<GalacticaDoc*>(mGame);
  #else
    return NULL;
  #endif // GALACTICA_SERVER
}

GalacticaProcessor* 
ThingyRef::GetHost() const {
    return dynamic_cast<GalacticaProcessor*>(mGame);
}

void
ThingyRef::ReportThingyRefLookupStatistics() throw() {
    try {
        DEBUG_OUT("ThingyRef Lookup Statistics Report", DEBUG_IMPORTANT | DEBUG_PACKETS);
        DEBUG_OUT("=========================================================", DEBUG_IMPORTANT | DEBUG_PACKETS);
        DEBUG_OUT("Gets:      " << gThingyRefCacheGets, DEBUG_IMPORTANT | DEBUG_PACKETS);
        DEBUG_OUT("Nils:      " << gThingyRefCacheNils << " (" << (gThingyRefCacheNils*100)
                                /gThingyRefCacheGets << "%)", DEBUG_IMPORTANT | DEBUG_PACKETS);
        DEBUG_OUT("Hits:      " << gThingyRefCacheHits << " (" << (gThingyRefCacheHits*100)
                                /gThingyRefCacheGets << "%)", DEBUG_IMPORTANT | DEBUG_PACKETS);
        DEBUG_OUT("Misses:    " << gThingyRefCacheMisses << " (" << (gThingyRefCacheMisses*100)
                                /gThingyRefCacheGets << "%)", DEBUG_IMPORTANT | DEBUG_PACKETS);
        DEBUG_OUT("Found:     " << gThingyRefCacheLookupOK << " (" << (gThingyRefCacheLookupOK*100)
                                /gThingyRefCacheGets << "%)", DEBUG_IMPORTANT | DEBUG_PACKETS);
        DEBUG_OUT("Not Found: " << gThingyRefCacheLookupNil << " (" << (gThingyRefCacheLookupNil*100)
                                /gThingyRefCacheGets << "%)", DEBUG_IMPORTANT | DEBUG_PACKETS);
        DEBUG_OUT("=========================================================", DEBUG_IMPORTANT | DEBUG_PACKETS);
    }
    catch(...) {
    }
}

#pragma mark-
// ===========================================================================
//	е LLongComparator
// ===========================================================================
//	Compares items as long integer values

CThingyRefComparator*	CThingyRefComparator::sThingyRefComparator = nil;

CThingyRefComparator::CThingyRefComparator()
{
}


CThingyRefComparator::~CThingyRefComparator()
{
}


SInt32
CThingyRefComparator::Compare(
	const void*		inItemOne,
	const void*		inItemTwo,
	UInt32			/* inSizeOne */,
	UInt32			/* inSizeTwo */) const
{
	ThingyRef* inRefOne = (ThingyRef*)inItemOne;
	ThingyRef* inRefTwo = (ThingyRef*)inItemTwo;
	return ( inRefOne->GetThingyID() - inRefTwo->GetThingyID() );
}

#if PP_NoBoolean
bool
#else
Boolean
#endif

CThingyRefComparator::IsEqualTo(
	const void*		inItemOne,
	const void*		inItemTwo,
	UInt32			/* inSizeOne */,
	UInt32			/* inSizeTwo */) const
{
	ThingyRef* inRefOne = (ThingyRef*)inItemOne;
	ThingyRef* inRefTwo = (ThingyRef*)inItemTwo;
	return ( *inRefOne == *inRefTwo );
}


CThingyRefComparator*
CThingyRefComparator::GetComparator()
{
	if (sThingyRefComparator == nil) {
		sThingyRefComparator = new CThingyRefComparator;
	}
	
	return sThingyRefComparator;
}


LComparator*
CThingyRefComparator::Clone()
{
	return new CThingyRefComparator;
}

#pragma mark -
// **** WAYPOINT ****
// еее explanations of Galactic coordinates in GalacticaUtils.h еее

Waypoint::Waypoint(const Point3D inCoord) {
	Set(inCoord);
	mTurnDistance = 0;
}

Waypoint::Waypoint(const AGalacticThingy *inThingy) {
	Set(inThingy);
	mTurnDistance = 0;
}

/* Waypoint::Waypoint(const CStar *inStar, const CStarLane *viaLane) {
	Set(inStar, viaLane);
	mTurnDistance = 0;
}
*/

Waypoint::Waypoint(const Waypoint &inOriginal) {
	mType = inOriginal.mType;
	mCoord = inOriginal.mCoord;
	mThingyRef = inOriginal.mThingyRef;
	mTurnDistance = inOriginal.mTurnDistance;
}

Waypoint::Waypoint(LStream *inStream) {
	ReadStream(inStream);
}

AGalacticThingy*
Waypoint::GetThingy() const {
	AGalacticThingy* aThingy = mThingyRef.GetThingy();
	AGalacticThingy* resultThingy = ValidateThingy(aThingy);
	if (aThingy != resultThingy) {	// clean up waypoint if it is invalid
		DEBUG_OUT("Waypoint: invalidating thingy ref "<< mThingyRef, DEBUG_ERROR);
		const_cast<Waypoint*>(this)->mThingyRef.InvalidateRef();
	}
	return resultThingy;
}

Point3D
Waypoint::GetCoordinates() const {
	AGalacticThingy* aThingy = mThingyRef.GetThingy();
	aThingy = ValidateThingy(aThingy);
	if (aThingy) {
		// update the coordinates in the waypoint anytime we fetch them,
		// so that they will always stay current
		mCoord = aThingy->GetCoordinates();
	}
	return mCoord;
}

bool
Waypoint::IsDebris() const {
    bool isDebris = false;
    if (mType > wp_Coordinates) {
        AGalacticThingy* aThingy = mThingyRef.GetThingy();
        aThingy = ValidateThingy(aThingy);
        if (!aThingy) { // thing failed validation test it is debris
            isDebris = true;
        } else if (aThingy->IsDead()) { // dead thingys are debris
            isDebris = true;
        }
    }
    return isDebris;
}

void
Waypoint::MakeDebris(bool inIsDebris) {
    if (inIsDebris) {
        mThingyRef.InvalidateRef();
    } else {
        mType = wp_Coordinates;
        mThingyRef.ClearRef();
    }
}

bool
Waypoint::ChangeIdReferences(PaneIDT oldID, PaneIDT newID) {
    if (mThingyRef.GetThingyID() == oldID) {
        mThingyRef.SetThingyID(newID, mThingyRef.GetGame());
        return true;
    } else {
        return false;
    }
}

const char*
Waypoint::GetDescription(std::string& ioStr) const {
	Point3D p = GetCoordinates();
	std::string s;
	std::string s2;
	bool named = false;
	if (mType > wp_Coordinates) {
	    bool isDebris = IsDebris();
	    if (!isDebris) {
    		AGalacticThingy* aThingy = mThingyRef.GetThingy();
    		aThingy = ValidateThingy(aThingy);
    		if (aThingy) {	// this is not a dead thingy
    			s = aThingy->GetName();
    			named = (s.size() > 0);
    			if (!named) {
    				LoadStringResource(s2, STRx_General, str_At_);
    				s += s2;
    			}
    	    } else {
    	        isDebris = true; // should not be debris, but it is
    	    }
		}
		if (isDebris) {
			LoadStringResource(s, STRx_General, str_DebrisAt_);
		}
		int subString = str_Unknown_;
		switch (mType) {
			case wp_Star:
			case wp_StarViaStarLane:
				if (!named) {
				    subString = str_Star_;
				} else {
				    subString = 0;   // don't add "star" to the star systems with a name
				}
				break;
			case wp_Ship:
			    subString = str_Ship_;  // add type to everything else (ie: Ship Colony T21)
				break;
			case wp_Fleet:
			    subString = str_Fleet_;
				break;
			case wp_Rendezvous:
			    subString = str_Rendezvous_;
				break;
			case wp_Wormhole:
			    subString = str_Wormhole_;
				break;
            default:
                break;
		}
		if (subString != 0) {
		    LoadStringResource(s2, STRx_General, subString);
		    s = s2 + s;
		}
	}
	if (!named) {	// no coordinates in parenthesis for named items
		char coordStr[255];
		float xcoord = (float)p.x/0x10000;
		float ycoord = (float)p.y/0x10000;
	  #if Use_3D_Coordinates
		float zcoord = (float)p.z/0x10000;
		std::sprintf(coordStr, "%1.2f, %1.2f, %1.2f", xcoord, ycoord, zcoord);
	  #else
	  	std::sprintf(coordStr, "%1.2f, %1.2f", xcoord, ycoord);
	  #endif
		s += coordStr;
	}
	if (mType == wp_StarViaStarLane) {
		LoadStringResource(s2, STRx_General, str_viaStarlane);
		s += s2;
	}
	ioStr = s;
	return ioStr.c_str();
}

ThingyRef&
Waypoint::GetThingyRef() {
    return mThingyRef;
}

void
Waypoint::WriteDebugInfo(ostream& cout) const {
	cout << (int) (mType - wp_None) << ": ";
	if (mType < wp_None) {
		cout << "OLD ";
	}
	if (mType >= wp_None) {	// new waypoint type
		if (mType == wp_None) {	// null waypoint
			cout << "NIL";
		} else {
			if ( (mType >= wp_FirstType) && (mType <= wp_LastType) ) {
				cout << mThingyRef << ", ";
			}

			char coordStr[255];
			float xcoord = (float)mCoord.x/0x10000;
			float ycoord = (float)mCoord.y/0x10000;
		  #if Use_3D_Coordinates
			float zcoord = (float)mCoord.z/0x10000;
			std::sprintf(coordStr, "%1.2f, %1.2f, %1.2f", xcoord, ycoord, zcoord);
		  #else
		  	std::sprintf(coordStr, "%1.2f, %1.2f", xcoord, ycoord);
		  #endif
			cout << coordStr;

		}
	}
}

// еее explanations of Galactic coordinates in GalacticaUtils.h еее
Point
Waypoint::GetLocalPoint() const {
	Point lp;
	Point3D p = GetCoordinates();	// еее this code wrongly assumes 16x16 ly for a 1x1 sector
	lp.h = p.x/1024L; //calc coordinates as offset from local sector position
	lp.v = p.y/1024L;
// following code assumes a sector is 16 ly across, and that local coordinates are within sector
// it is commented out because under version 1.2, there are variable sector sizes from 16-80 ly
// could limit it above using (p.x & 0x5fffff) >> 10, but seems unnecessary.
//	lp.h = (p.x & 0xfffff) >> 10; //calc coordinates as offset from local sector position
//	lp.v = (p.y & 0xfffff) >> 10; //code is fast way to calc (x % 0x100000)/1024
	return lp;
}

Waypoint&
Waypoint::Set(const Point3D inCoord) {
	mType = wp_Coordinates;
	mCoord = inCoord;
	mThingyRef.ClearRef();
	return *this;
}

Waypoint&
Waypoint::Set(const AGalacticThingy *inThingy) {
	mCoord = gNullPt3D;
	AGalacticThingy* aThingy = ValidateThingy(inThingy);
	if (aThingy == nil) {
		mType = wp_None;
		mThingyRef.ClearRef();
	} else {
		switch (aThingy->GetThingySubClassType() ) {
			case thingyType_Star:
				mType = wp_Star;
				break;
			case thingyType_Ship:
				mType = wp_Ship;
				break;
			case thingyType_Fleet:
				mType = wp_Fleet;
				break;
			case thingyType_Wormhole:
				mType = wp_Wormhole;
				break;
			case thingyType_Rendezvous:
				mType = wp_Rendezvous;
				break;
			default:
				mType = wp_None;
				break;
		}
		mThingyRef = inThingy;
	}
	return *this;
}

Waypoint&	
Waypoint::Set(const CStar *inStar, const CStarLane *) {	// viaLane
	mType = wp_StarViaStarLane;
	mCoord = gNullPt3D;
	mThingyRef = (AGalacticThingy*)inStar;
	return *this;
}

int
Waypoint::operator== (const Waypoint &wp) {
	if (mThingyRef.GetThingyID()) {
		return (wp.mThingyRef.GetThingyID() == mThingyRef.GetThingyID());
	} else {
		return ((wp.mCoord.x == mCoord.x) && (wp.mCoord.y == mCoord.y));
	}
}

int
Waypoint::operator!= (const Waypoint &wp) {
	if (mThingyRef.GetThingyID()) {
		return (wp.mThingyRef.GetThingyID() != mThingyRef.GetThingyID());
	} else {
		return ((wp.mCoord.x != mCoord.x) && (wp.mCoord.y != mCoord.y));
	}
}

Waypoint&
Waypoint::operator+= (const Point3D inCoord) {
	switch (mType) {
		case wp_StarViaStarLane:
		case wp_Star:
		case wp_Ship:
		case wp_Fleet:
		case wp_Wormhole:
		case wp_Rendezvous:
        {
            AGalacticThingy* aThingy = nil;
			aThingy = ValidateThingy(mThingyRef.GetThingy());
			if (aThingy) {
				mCoord = aThingy->GetCoordinates();
			} else {	// this is not really an error, but we want to see if this was happening a lot
					// remove this code once we've established it was, since it almost certainly
					// was causing things to jump to 0,0 if they were heading to a waypoint that
					// had been killed.
				DEBUG_OUT("Dead Thingy Waypoint "<<this<<" NOT setting to 0,0", DEBUG_ERROR | DEBUG_MOVEMENT);
//				mCoord = gNullPt3D;
			}
			mType = wp_Coordinates;	// so change waypoint type
				// don't break so it will fall through and add coordinates
        }
		case wp_Coordinates:
			mCoord.x += inCoord.x;
			mCoord.y += inCoord.y;
#if Use_3D_Coordinates
			mCoord.z += inCoord.z;
#endif
			break;
		default:
			return *this;
	}
/*
	if (mStarLane != nil)
		mType = wp_ToStarViaStarLane;	// restore our in starlane indicator if needed
*/
	return *this;
}

// еее explanations of Galactic coordinates in GalacticaUtils.h еее
UInt32
Waypoint::Distance(Point3D inCoord) {
	Point3D coord = GetCoordinates();
	signed long dx = (signed long)inCoord.x - (signed long)coord.x;
	signed long dy = (signed long)inCoord.y - (signed long)coord.y;
	return std::hypot(dx, dy);
/*  #if Use_3D_Coordinates
	signed long dz = (signed long)inCoord.z - (signed long)coord.z;
	unsigned long sumsqrd = dx*dx + dy*dy + dz*dz;
  #else
	unsigned long dist = sqrt(sumsqrd);
  #endif
	unsigned long dist = sqrt(dx*dx + dy*dy);
	return dist;	*/
}

UInt32
Waypoint::Distance(Waypoint &inWaypoint) {
	return Distance(inWaypoint.GetCoordinates());
}				

UInt32
Waypoint::Distance(AGalacticThingy *inThingy) {
	return Distance(inThingy->GetCoordinates());
}

void
Waypoint::WriteStream(LStream *inStream) {
	SInt8 type = mType;
	*inStream << type;		// always write the type
	if (type != wp_None) {	// write coordinates if non-NULL
		if (mCoord == gNowherePt3D) {
			GetCoordinates();
		}
		*inStream << mTurnDistance << mCoord.x << mCoord.y << mCoord.z;	
		if (type > wp_Coordinates) {	// write thingy reference if waypoint
			*inStream << mThingyRef;	// is of type to have a thingy associated with it
		}
	}
}

void
Waypoint::ReadStream(LStream *inStream, UInt32 streamVersion) {
	SInt8 type;
	*inStream >> type;
	if (type >= wp_None) {	// new style waypoint
		if (type != wp_None) {			// read coordinates if non-NULL
            SInt32 x, y, z;
			*inStream >> mTurnDistance >> x >> y >> z;
            mCoord.x = x;
            mCoord.y = y;
            mCoord.z = z;	
			if (type > wp_Coordinates) {	// read thingy reference if waypoint
				*inStream >> mThingyRef;	// is of type to have a thingy associated with it
			} else {
				mThingyRef.ClearRef();
			}
		} else {
			mCoord = gNullPt3D;
			mTurnDistance = 0;
			mThingyRef.ClearRef();
		}
		
	} else {	// old style waypoint, for 1.2b7 --> 1.2b8 backward compatibility
	
	    DEBUG_OUT("Found obsolete waypoint type "<< (int)type, DEBUG_ERROR);
	    Throw_(dataVerErr);
/*		if (type == xwaypoint_None) {
			mCoord = gNullPt3D;
			mTurnDistance = 0;
			mThingyRef.ClearRef();
		} else {
			*inStream >> mTurnDistance;
			if (type <= xwaypoint_Coordinates) {	// coordinates and dead thingys
				*inStream >> mCoord.x;
				*inStream >> mCoord.y;
				*inStream >> mCoord.z;
				if (type<0) {
					type = -type;	// dump dead thingy indicator
				}
				mThingyRef.ClearRef();
			} else {						// living thingys
				mCoord = gNullPt3D;
				PaneIDT theID;
				*inStream >> theID;
			    DEBUG_OUT("creating old style ThingyRef"
				mThingyRef = ThingyRef(theID, GalacticaShared::GetStreamingGame());
			}
		}
		type += wp_None;	// convert to new type
*/
	}
	mType = (EWaypointType)type;
}

// only used for saved or transmitted structures, stream handles byte swapping for us
void 
Waypoint::ByteSwapOutgoing() throw() {
    mThingyRef.ByteSwapOutgoing();
    mCoord.x = Native_ToBigEndian32(mCoord.x);
    mCoord.y = Native_ToBigEndian32(mCoord.y);
    mCoord.z = Native_ToBigEndian32(mCoord.z);
    mTurnDistance = Native_ToBigEndian32(mTurnDistance);

}

// only used for saved or transmitted structures, stream handles byte swapping for us
void
Waypoint::ByteSwapIncoming() throw() {
    mThingyRef.ByteSwapIncoming();
    mCoord.x = BigEndian32_ToNative(mCoord.x);
    mCoord.y = BigEndian32_ToNative(mCoord.y);
    mCoord.z = BigEndian32_ToNative(mCoord.z);
    mTurnDistance = BigEndian32_ToNative(mTurnDistance);
}


#pragma mark -

// **** SettingT ****

	
bool
SettingT::AdjustValue() {
	bool changed = true;
	if (value < desired) {
		value += rate;
		if (value > desired) {
			value = desired;
		}
	} else if (value > desired) {
		value -= (rate << 2);	// rate of decline is 4 times the rate of increase
		if (value < desired) {
			value = desired;
		}
	} else {
		changed = false;
	}
	return changed;
}

void
SettingT::AdjustEconomy(float inAverageBenefit) {
	float ratio = benefit / inAverageBenefit;
	economy = (short)(ratio * 500 / 1000);	// 
}

void
SettingT::ReadStream(LStream *inStream, UInt32 streamVersion) {
	Bool8 bLock;
	*inStream >> value >> desired >> rate >> benefit >> economy >> bLock;
	locked = bLock;
}

void
SettingT::WriteStream(LStream *inStream) {
	Bool8 bLock = locked;
	*inStream << value << desired << rate << benefit << economy << bLock;
}

void	// doesn't alter user interface items
SettingT::UpdateClientFromStream(LStream *inStream) {
	short ignoreDesired;
	Bool8 ignoreLock;
	*inStream >> value >> ignoreDesired >> rate >> benefit >> economy >> ignoreLock;
}

void	// doesn't alter user interface items
SettingT::UpdateHostFromStream(LStream *inStream) {
// this method is used to update the host from a packet sent by a client
// the client is not allowed to update most of the data, so many fields are
// ignored
	short ignore16;
	float ignoreFloat;
	Bool8 bLock;
	*inStream >> ignore16 >> desired >> ignore16 >> ignoreFloat >> ignore16 >> bLock;
	locked = bLock;
}

#pragma mark -

// ***** TIMER CLASS *****

CTimer::CTimer() {
	mCountdownToDT = 0;
	mLastDT = 0;
	mTimed = nil;
	mDisplayPane = nil;
	mIsCounting = false;
}

CTimer::~CTimer() {
	if (mTimed) {
		mTimed->SetTimer(nil);	// we aren't here anymore, so we clear the reference
	}
}

void
CTimer::Reset(unsigned long inCountdownToDT) {
	mCountdownToDT = inCountdownToDT;
	mLastDT = 0;
	mPausedAtDT = 0;
	mIsCounting = true;
	ClearMark();
	StartRepeating();
	DEBUG_OUT("Reset Timer ["<<(void*)this<<"] "<<mCountdownToDT, DEBUG_DETAIL | DEBUG_EOT);
}

void
CTimer::Clear(const char* inDisplayStr) {
	StopRepeating();
	ClearMark();
	mIsCounting = false;
	mCountdownToDT = 0;
  #ifndef GALACTICA_SERVER
	if (mDisplayPane) {
		Str255 s = "\p";
	    if (inDisplayStr) {
		   c2pstrcpy(s, inDisplayStr);
		}
		mDisplayPane->SetDescriptor(s);
		mDisplayPane->UpdatePort();
	}
  #endif // GALACTICA_SERVER
	DEBUG_OUT("Cleared Timer ["<<(void*)this<<"] "<<mCountdownToDT, DEBUG_DETAIL | DEBUG_EOT);
}

void
CTimer::Pause(const char* inDisplayStr) {
    if (!mIsCounting) {
		DEBUG_OUT("Warning: Pausing Timer ["<<(void*)this<<"] that is not running", DEBUG_IMPORTANT);
    } else if (mCountdownToDT != 0) {
		mIsCounting = false;
		std::time_t currDT;			// get current date/time
		mPausedAtDT = (unsigned long)std::time(&currDT); //::GetDateTime(&currDT);
		mCountdownToDT -= mPausedAtDT;	// convert the value for end time to "seconds remaining"
      #ifndef GALACTICA_SERVER
		if (mDisplayPane && inDisplayStr) {
    		Str255 s;
    		c2pstrcpy(s, inDisplayStr);
			mDisplayPane->SetDescriptor(s);	// make countdown pane display a string
			mDisplayPane->UpdatePort();
		}
      #endif // GALACTICA_SERVER
		DEBUG_OUT("Paused Timer ["<<(void*)this<<"] "<<mCountdownToDT, DEBUG_DETAIL | DEBUG_EOT);
	}
    StopRepeating();
}

void
CTimer::Mark(const char* inExtraStr) {
    mMarkString.assign(inExtraStr);
	std::time_t currDT;			// get current date/time
	unsigned long dt = std::time(&currDT);
	MakeTimeRemainingString(dt);
    UpdateTimerDisplay();
}

void
CTimer::ClearMark() {
    mMarkString.assign("");
}

void
CTimer::Resume() {
    if (mIsCounting) {
		DEBUG_OUT("Warning: Resuming Timer ["<<(void*)this<<"] that is already running", DEBUG_IMPORTANT);
    } else if (mCountdownToDT != 0) {
        // CountdownTo was actually seconds remaining
        // while we were paused, so we add that to the
        // current time when we resume
        // that way we don't count the time we were paused
		std::time_t currDT;
		mCountdownToDT += (unsigned long)std::time(&currDT);
		mPausedAtDT = 0;
		mIsCounting = true;
		DEBUG_OUT("Resumed Timer ["<<(void*)this<<"] "<<mCountdownToDT, DEBUG_DETAIL | DEBUG_EOT);
	}
	StartRepeating();
}

UInt32
CTimer::GetTimeRemaining() {
	if (mIsCounting) {
		std::time_t currDT;			// get current date/time
		unsigned long dt = std::time(&currDT);;
		return (mCountdownToDT - dt);
	} else {
		return GetTimeRemainingWhilePaused();
	}
}

void
CTimer::SetTimeRemaining(UInt32 inSecsRemaining) {
    if (mIsCounting) {
		std::time_t currDT;			// get current date/time
		unsigned long dt = std::time(&currDT);;
        mCountdownToDT = dt + inSecsRemaining;
    } else {
        SetTimeRemainingWhilePaused(inSecsRemaining);
    }
}

void
CTimer::SpendTime(const EventRecord &) { // inMacEvent) {
	std::time_t currDT;			// get current date/time
	unsigned long dt = std::time(&currDT);
	bool firstTime = (mLastDT == 0);
	if (dt > mLastDT) {
	   mLastDT = dt;
		if (dt >= mCountdownToDT) {	// has the time expired?
			StopRepeating();
			mIsCounting = false;
			if (mTimed) {
				DEBUG_OUT("Timer ["<<(void*)this<<"] expired "<<mCountdownToDT, DEBUG_DETAIL | DEBUG_EOT);
				mTimed->TimeIsUp();	// call time up for the timed object if there was one.
			}
        #ifndef GALACTICA_SERVER
			if (mDisplayPane) {
				mDisplayPane->SetDescriptor("\p");	// clear the countdown pane
				mDisplayPane->UpdatePort();
			}
        #endif // GALACTICA_SERVER
		} else if (mDisplayPane) {	// do we have a display pane that needs updating?
			bool needsRedraw = MakeTimeRemainingString(dt);
			if (needsRedraw || firstTime) {	// update pane only when needed
				UpdateTimerDisplay();
/*			   Str255 tmpstr;
			   c2pstrcpy(tmpstr, mTimeString.c_str());
			   mDisplayPane->SetDescriptor(tmpstr);
			   mDisplayPane->UpdatePort(); */
			}
		}
	}
}

const char*
CTimer::GetTimeRemainingString() {
   return mTimeString.c_str();
}

// returns true if time string changed
bool
CTimer::MakeTimeRemainingString(unsigned long inCurrTime) {
	unsigned long dt = mCountdownToDT - inCurrTime;	// first calc time remaining
   long days = (dt/86400);
	std::string s;
	bool needsRedraw = ( (dt % 60) == 0 );	// definately update once a minute
	if (days>1) {
		LoadStringResource(s, STRx_General, str_Days);
		char tmpstr[256];
		std::sprintf(tmpstr, "%ld", days);
		mTimeString = tmpstr;
		mTimeString += s;
	} else {
		needsRedraw |= Secs2TimeStr(dt, mTimeString, 5);	// show seconds at 0:04:59 remaining
	}
	// append the mark if there is one
	if (mMarkString.length() > 0) {
	   mTimeString += ' ';
	   mTimeString += mMarkString;
	}
	return needsRedraw;
}

void
CTimer::UpdateTimerDisplay() {
#ifndef GALACTICA_SERVER
	Str255 tmpstr;
	c2pstrcpy(tmpstr, mTimeString.c_str());
	mDisplayPane->SetDescriptor(tmpstr);
	mDisplayPane->UpdatePort();
#endif // GALACTICA_SERVER
	/*
	mDisplayPane->FocusDraw();
	mDisplayPane->DrawSelf();
	if ( !UEnvironment::IsRunningOSX() ) {
	if ( (dt % 5) != 0 ) {	// permit refresh every 5 seconds
		mDisplayPane->DontRefresh();
	}
	}
	*/
}

#pragma mark -

CTimed::CTimed() {
	mTimer = nil;
}

CTimed::~CTimed() {
	if (mTimer) {
		mTimer->SetTimed(NULL);	// make sure the timer doesn't try to tell us of it's death
		delete mTimer;
	}
}

void
CTimed::SetTimer(CTimer* inTimer) {
	mTimer = inTimer;
	if (mTimer) {
		mTimer->SetTimed(this);
	}
}

#pragma mark -
// **** STREAM ADDITIONS ****

LStream& operator >> (LStream& inStream, Waypoint& outWaypoint) {
	outWaypoint.ReadStream(&inStream);
	return inStream;
}

LStream& operator << (LStream& inStream, Waypoint& inWaypoint) {
	inWaypoint.WriteStream(&inStream);
	return inStream;
}

#ifdef DEBUG
ostream& operator << (ostream& cout, Waypoint& inWaypoint) {
	inWaypoint.WriteDebugInfo(cout);
	return cout;
}
#endif

LStream& 
operator >> (LStream& inStream, SettingT& outSetting) {
	outSetting.ReadStream(&inStream);
	return inStream;
}

LStream& 
operator << (LStream& inStream, SettingT& inSetting) {
	inSetting.WriteStream(&inStream);
	return inStream;
}

/*
LStream& operator >> (LStream& inStream, LArray& outList);
LStream& operator << (LStream& inStream, LArray& inList);
*/

bool
UMoveReferencedThingysAction::DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData) {
#pragma unused (inAffectedArray, inItemIndex)
	AGalacticThingy* it = *((AGalacticThingy**)inItemData);
	it = ValidateThingy(it);
	ASSERT(it != nil);
	if (!it) {
		return false;
	}
	if (it->RefersTo(mReferencedThingy)) {
		if (ValidateShip(it)) {	// Recalc Position
			((CShip*)it)->CalcMoveDelta();	// ships need to recalc their move delta as well.
		}
	  #ifndef GALACTICA_SERVER
		if (it->AsThingyUI() != NULL) {
		    it->AsThingyUI()->CalcPosition(mDoRefresh);
		}
	  #endif // GALACTICA_SERVER
	}
	return false;
}

bool
URemoveReferencesAction::DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData) {
#pragma unused (inAffectedArray, inItemIndex)
	AGalacticThingy* it = *((AGalacticThingy**)inItemData);
	it = ValidateThingy(it);
	ASSERT(it != nil);
	if (it) {
		it->RemoveReferencesTo(mReferencedThingy);
	}
	return false;
}

bool
UChangeIdReferencesAction::DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData) {
#pragma unused (inAffectedArray, inItemIndex)
	AGalacticThingy* it = *((AGalacticThingy**)inItemData);
	it = ValidateThingy(it);
	ASSERT(it != nil);
	if (it) {
		it->ChangeIdReferences(mOldId, mNewId);
	}
	return false;
}


// this class will check for a satellite fleet
bool
UFindSatelliteFleetAction::DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData) {
#pragma unused (inAffectedArray, inItemIndex)
	if (mFoundSatelliteFleet == nil) {						// hasn't been found yet
		ThingyRef* aRef = (ThingyRef*)inItemData;			// since this is the ships list, this will be a ref
		if (aRef->GetThingyID() != PaneIDT_Undefined) {		// ignore undefined items to save time
			CFleet* aFleet = dynamic_cast<CFleet*>( aRef->GetThingy() );	// lookup the item from the reference
			if (aFleet && (aFleet->GetThingySubClassType() == thingyType_Fleet)) {
				if (aFleet->IsSatelliteFleet()) {	
					mFoundSatelliteFleet = aFleet;
				}
			}
		}
	}
	return false;
}

#ifdef DEBUG

// this class will check for things that are contained by other objects
bool
UHasContainerAction::DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData) {
#pragma unused (inAffectedArray, inItemIndex)
	AGalacticThingy* it = *((AGalacticThingy**)inItemData);
	it = ValidateThingy(it);
	ASSERT(it != nil);
	if (it == nil) {
		return false;
	}
	if (it->HasContent(mReferencedThingy)) {
		DEBUG_OUT(it<<" contains "<<mReferencedThingy, DEBUG_ERROR);
		mHasContainer = true;
	}
	return false;
}
#endif

#ifdef DEBUG

bool
UHasReferencesAction::DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData) {
#pragma unused (inAffectedArray, inItemIndex)
	AGalacticThingy* it = *((AGalacticThingy**)inItemData);
	it = ValidateThingy(it);
	ASSERT(it != nil);
	if (it == nil) {
		return false;
	}
	if (it->IsDead()) {
		DEBUG_OUT(it << " is dead, skipping reference check", DEBUG_TRIVIA | DEBUG_MOVEMENT);
	} else if (it->RefersTo(mReferencedThingy)) {
		DEBUG_OUT(it << " refers to deleted thingy "<<mReferencedThingy, DEBUG_ERROR);
		it->RemoveReferencesTo(mReferencedThingy); 
		mWasReferenced = true;
	}
	return false;
}
#endif

