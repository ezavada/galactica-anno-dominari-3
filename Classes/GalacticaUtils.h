// GalacticaUtils.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 5/6/96 ERZ

#ifndef GALACTICA_UTILS_H_INCLUDED
#define GALACTICA_UTILS_H_INCLUDED

#include "GalacticaConsts.h"
#include "GalacticaTypes.h"
#include "GenericUtils.h"
#include <ostream>

//=======================================================================

// uncomment the following line to use include the debugging cheats
// the current cheats are:
// 1) using a Galactic Density of 1 will make only 1 star for each player and one free star
//		and will give everyone tech level 200
// 2) All of Player 1's planets (except the home planet) will be given tech level 50

//#define DO_DEBUGGING_CHEATS

// еее other debugging defines that are not specific to Galactica are found in GenericUtils.h

#include <LStream.h>
#include <LPeriodical.h>	// for use by the CTimer class
#include <LComparator.h>

#define Use_3D_Coordinates 0

// еее NOTE: the pixel stuff below is inaccurate because the code is assuming a 1x1 sector is
//           16 light years across, which leaves the stars much too densly packed to be realistic.
//           The stuff below defines a more realistic Galactic Unit (GU), and notes the differences
//			 with the current implementation ееее

// Galactic coordinates are expressed with the Point3D and translate directly into
// lightyears as follows:
// 1000 0000  (hex) = 1000.0 ly  (hiword is lightyears, like Fixed type)
// 0008 8000  (hex) = 8.5 ly
// 0000 0001  (hex) = 1/65536 of a light year = 140,000,000 km = 90,000,000 miles
// which is approximately the distance from the Earth to the Sun, 1 AU.
// This means that the largest galaxy that can be represented would be 65,536 ly across, which
// isn't too bad since the Milky Way galaxy, a rather average one as galaxies go, is approx.
// 80,000 ly across. So while we couldn't actually represent the entire Milky Way with this,
// we could at least come pretty close, only losing the tiniest bit around the edges.

// A size 1 sector is 64 ly on a side, and at normal (1:1) zoom has 1024 pixels, with each pixel 
// representing 1/16th of a lightyear. Thus Alpha Centuri would be 69 pixels (4.3 ly = 281,805 GU)
// away from Sol.
// еее currently assuming 16 ly on a side, so Alpha Centuri would be 275 pixels from Sol.

// at 1:1 zoom, GU to pixels is: pix = GU/4096; or: pix = GU>>12;
// еее currently, GU to pixels is: pixel = GU/1024; or: pix = GU>>10;

// starmap grids are thus:
// sector boundary lines (light blue) every 64 light years (4,194,304 GU / 0x400000 GU )
// gridlines (medium grey) every 4 light years (262,144 GU / 0x40000 GU)
// subgrid lines (dark grey) every 0.4 light years (26,214 GU / 0x6666 GU)
// еее currently every 16 ly, 1 ly, and 1/10th ly

// Clearly, in-system (Stellar) measurements will need to be based in something other than GU, 
// probably kilometers

// speed of light = 299,790,000 m/s
// sec/year = 31,536,000 s
// 1 light-year = 9,454,200,000,000,000 meters
// 1 galactic unit = 144,260,000 km (144259643)
// convert kilometers to galactica units: gu = km/144260000L;
// fast convert (pretty accurate) kilometers to galactic units: gu = km>>24 + km>>27;
// superfast (less accurate) convert kilometers to galactic units: gu = km>>27;

enum {
	kOwnedBy_NOBODY = 0 
};

using std::ostream;

// following moved to galactica types
/*#pragma options align=mac68k
typedef struct Point3D {
	long x;
	long y;
	long z;
	int operator== (const Point3D &p) {return ((x == p.x)&&(y==p.y)&&(z==p.z));}
} Point3D, *Point3DPtr;

#pragma options align=reset

extern const Point3D gNullPt3D;
extern const Point3D gNowherePt3D;
*/

class CStar;
class CStarLane;
class CShip;
class CMessage;
class AGalacticThingy;
class AGalacticThingyUI;
class GalacticaDoc;
class GalacticaProcessor;
class GalacticaShared;

class LStr255;
class LPane;  // for CTimer display

AGalacticThingy*	ValidateThingy(const AGalacticThingy* inThingy);
CMessage*			ValidateMessage(const void* inMessage);
CShip*				ValidateShip(const void* inShip);

// type is type of waypoint
// waypoints of type wp_None have no additional data
// waypoints of type wp_Coordinates and beyond have a Point3D
// waypoints of type wp_Star and beyond also have a ThingyRef
enum EWaypointType {
    // start at 0x40 to for legacy reasons, can't change now or
    // we break the file format
	wp_None				= 0x40,
	wp_Coordinates		= 0x41,
	wp_Star				= 0x42,
	wp_Ship				= 0x43,
	wp_Wormhole			= 0x44,
	wp_StarViaStarLane  = 0x45,
	wp_Fleet  			= 0x46,
	wp_Rendezvous		= 0x47
};

const EWaypointType wp_FirstType = wp_Star;
const EWaypointType wp_LastType = wp_Rendezvous;


// a ThingyRef maintains a reference to another item within the game.
// it always tracks by id, and when the reference is up to date
// by pointer. It always tries to keep the reference up to date.

class ThingyRef {
public:
    static void ReportThingyRefLookupStatistics() throw();
    
	ThingyRef() : mThingyID(0), mThingy(NULL), mGame(NULL), mValidTurn(-1) {}	// default constructor
	
	// copy constructor
	ThingyRef(const ThingyRef &inRef)  {
										mThingyID = inRef.mThingyID; 
										mThingy = inRef.mThingy; 
										mGame = inRef.mGame; 
										mValidTurn = inRef.mValidTurn;
										}

	// construct from an existing Galactic Thingy
	ThingyRef(const AGalacticThingy *inThingy)  { 
										SetThingy(inThingy);}
	
	// construct from an id number, plus a game pointer
	ThingyRef(const PaneIDT inID, GalacticaShared * inGame) {
										SetThingyID(inID, inGame);}
													
	ThingyRef& operator= (const AGalacticThingy *inThingy)	{
										SetThingy(inThingy); 
										return *this;}
										
//	inline ThingyRef& operator= (const PaneIDT inID);
	int operator== (const PaneIDT inID) { return (inID == mThingyID);}
	int operator== (const ThingyRef &inRef) { return (inRef.mThingyID == mThingyID);}
	inline int operator== (const AGalacticThingy *inThingy);

	int operator!= (const PaneIDT inID) { return (inID != mThingyID);}
	int operator!= (const ThingyRef &inRef) { return (inRef.mThingyID != mThingyID);}
	inline int operator!= (const AGalacticThingy *inThingy);

	void SetThingy(const AGalacticThingy *inThingy);
	
	void SetThingyID(const PaneIDT inID, GalacticaShared * inGame);
	void SetGame(GalacticaShared* inGame);
	
	void ClearRef() {mThingyID = 0; mThingy = nil;}
	
	void InvalidateRef() {mThingy = nil;}
	
	GalacticaShared*	GetGame() const {return mGame;}
	GalacticaDoc*       GetGameDoc() const;
	GalacticaProcessor* GetHost() const;
	PaneIDT				GetThingyID() const {return mThingyID;}

	AGalacticThingy*	GetThingy() const;

	void 				ReadStream(LStream& inInputStream, UInt32 streamVersion = version_SavedTurnFile);
	
	void				WriteStream(LStream& inOutputStream) {
										inOutputStream << mThingyID;}

	void				WriteDebugInfo(ostream& cout) const {
							cout << mThingyID << " {"<<(void*)mThingy<<"} #" << mValidTurn; }

    void                ByteSwapOutgoing() throw();
    void                ByteSwapIncoming() throw();
    
protected:
	PaneIDT				mThingyID;
	AGalacticThingy*	mThingy;	// this can be NULL if the ref hasn't been resolved
	GalacticaShared*	mGame;
	long				mValidTurn;	// turn number this reference was last validated
};

class	CThingyRefComparator : public LComparator {
public:
						CThingyRefComparator();
	virtual				~CThingyRefComparator();
			
	virtual SInt32		Compare(
								const void*			inItemOne,
								const void* 		inItemTwo,
								UInt32				inSizeOne,
								UInt32				inSizeTwo) const;
								
#if PP_NoBoolean
	virtual bool		IsEqualTo(
#else
	virtual Boolean		IsEqualTo(

#endif // PP_NoBoolean
								const void*			inItemOne,
								const void* 		inItemTwo,
								UInt32				inSizeOne,
								UInt32				inSizeTwo) const;
								
	virtual LComparator*	Clone();
								
	static CThingyRefComparator*		GetComparator();
	
protected:
	static CThingyRefComparator*	sThingyRefComparator;
};

template <class T> class TThingyRef : public ThingyRef {
public:
	TThingyRef() : ThingyRef() {}	// default constructor
	TThingyRef(const ThingyRef &inRef) : ThingyRef(inRef) {}
	TThingyRef(const PaneIDT inID, GalacticaDoc * inGame) : ThingyRef(inID, inGame) {}
	TThingyRef(const T inThingy) : ThingyRef(static_cast<AGalacticThingy*>(inThingy)) {}
	ThingyRef& operator= (const T inThingy)	{
										SetThingy(static_cast<AGalacticThingy*>(inThingy)); 
										return *this;}
	operator T() const {return static_cast<T>(GetThingy());}
};

inline LStream& operator >> (LStream& inInputStream, ThingyRef& inRef) {
	inRef.ReadStream(inInputStream);
	return inInputStream;
}

inline LStream& operator << (LStream& inOutputStream, ThingyRef& inRef) {
	inRef.WriteStream(inOutputStream);
	return inOutputStream;
}

inline ostream& operator << (ostream& cout, const ThingyRef& inRef) {
	inRef.WriteDebugInfo(cout);
	return cout;
}



class Waypoint {
public:
    Waypoint() : mType(wp_None), mCoord(gNullPt3D), mThingyRef(), mTurnDistance(0) {}
    Waypoint(const Point3D inCoord);
    Waypoint(const AGalacticThingy *inThingy);
//	Waypoint(const CStar *inStar, const CStarLane *viaLane);
    Waypoint(const Waypoint &inOriginal);
    Waypoint(LStream *inStream);

    operator Point3D() const {return GetCoordinates();}
    operator AGalacticThingy*() const {return GetThingy();}
    operator EWaypointType() const	{return mType;}

    EWaypointType       GetType() const {return mType;}
    AGalacticThingy*    GetThingy() const;
    Point3D             GetCoordinates() const;
    const char*         GetDescription(std::string& ioStr) const;
    ThingyRef&          GetThingyRef();
    void                WriteDebugInfo(ostream& cout) const;
    Point               GetLocalPoint() const;
    long                GetTurnDistance() const {return (mTurnDistance & 0x0000ffffL);}
    void                SetTurnDistance(short inTurns) {mTurnDistance = (mTurnDistance & 0xffff0000L) | (inTurns & 0x0000ffffL);}
    long                GetRemainingTurnDistance() const {return (mTurnDistance >> 16);}
    void                SetRemainingTurnDistance(short inTurns) {mTurnDistance = (mTurnDistance & 0x0000ffffL) | (inTurns<<16);}
    bool                IsDebris() const;
    void                MakeDebris(bool inIsDebris);
    bool                ChangeIdReferences(PaneIDT oldID, PaneIDT newID);  // returns true if changed
    bool                IsNull() const {return (mType == wp_None) || ((mType==wp_Coordinates)&&(mCoord.x==gNullPt3D.x)&&(mCoord.y==gNullPt3D.y));}
    bool                IsNowhere() const {return (mType == wp_Coordinates) && (mCoord.x==gNowherePt3D.x) && (mCoord.y==gNowherePt3D.y);}
    Waypoint&           Set(const Point3D inCoord);
    Waypoint&           Set(const AGalacticThingy *inThingy = nil);
    Waypoint&           Set(const CStar *inStar, const CStarLane *viaLane);
    Waypoint&           operator= (const Point3D &inCoord) {return Set(inCoord); }
    Waypoint&           operator= (const AGalacticThingy *inThingy) {return Set(inThingy); }
    Waypoint&           operator+= (const Point3D inCoord);
    int                 operator== (const Point3D &p) {return ((mCoord.x==p.x)&&(mCoord.y==p.y));}
    int                 operator== (const Waypoint &wp);
    int                 operator!= (const Point3D &p) {return ((mCoord.x!=p.x)&&(mCoord.y!=p.y));}
    int                 operator!= (const Waypoint &wp);
    UInt32              Distance(Point3D inCoord);
    UInt32              Distance(Waypoint &inWaypoint);
    UInt32              Distance(AGalacticThingy *inThingy);
    void                WriteStream(LStream *inStream);
    void                ReadStream(LStream *inStream, UInt32 streamVersion = version_SavedTurnFile);
    void                ByteSwapOutgoing() throw();
    void                ByteSwapIncoming() throw();
    bool                CanMoveSelf() const {return ((mType == wp_Ship) || (mType == wp_Fleet));}
    
private:
	EWaypointType       mType;	// types enumerated above
	mutable Point3D     mCoord;
	ThingyRef           mThingyRef;
	long                mTurnDistance;	// used in course to store # of turns for each leg
};

#define kAllowDeadThingys true
#define kDisallowDeadThingys false

LStream& operator >> (LStream& inStream, Waypoint& outWaypoint);
LStream& operator << (LStream& inStream, Waypoint& inWaypoint);

ostream& operator << (ostream& cout, Waypoint& inWaypoint);

typedef class SettingT {
public:	
	short value;	// current effective value
	short desired;	// desired effective value
	short rate;		// rate of change toward desired value per turn
	float benefit;	// benefit derived from one unit of value
	short economy;	// benefit relative to average benefit
	bool  locked;	// is this setting locked to prevent automatic movement?
	
	operator long()		{return (long)GetBenefits();}
	operator float()	{return GetBenefits();}

	float	GetBenefits() const {return (float)value*benefit;}
	float	GetProjectedBenefits() const {return (float)desired*benefit;}
	bool	AdjustValue();
	void	AdjustEconomy(float inAverageBenefit);
	void	ReadStream(LStream *inStream, UInt32 streamVersion = version_SavedTurnFile);
	void	WriteStream(LStream *inStream);
	void	UpdateClientFromStream(LStream *inStream);	// doesn't alter user interface items
	void	UpdateHostFromStream(LStream *inStream);	// doesn't alter anything controlled by host
} SettingT, *SettingPtr;

class CTimed;

class CTimer : public LPeriodical {
   friend class CTimed;
public:	
		CTimer();
		~CTimer();
	void            Reset(unsigned long inCountdownToDT);
	void            Clear(const char* inDisplayStr = NULL);
	void            Pause(const char* inDisplayStr = NULL);
	void            Mark(const char* inExtraStr);
	void            ClearMark();
	void            Resume();
	bool            IsCounting() {return mIsCounting;}
	UInt32          GetTimeRemaining();
	void            SetTimeRemaining(UInt32 inSecsRemaining);
	void            SetTimeRemainingWhilePaused(UInt32 inSecs) {mCountdownToDT = inSecs;}
	virtual void    SpendTime(const EventRecord &inMacEvent);
	void            SetDisplayPane(LPane* inTimeLeftPane) {mDisplayPane = inTimeLeftPane;}
	const char*     GetTimeRemainingString();
	void			UpdateTimerDisplay();
protected:
	UInt32          GetTimeRemainingWhilePaused() {return mCountdownToDT;}
 // don't call this, call CTimed::SetTimer()
 	void            SetTimed(CTimed* inTimedObj) {mTimed = inTimedObj;}
 	bool            MakeTimeRemainingString(unsigned long inCurrTime);
	unsigned long   mCountdownToDT;
	unsigned long   mLastDT;
	unsigned long   mPausedAtDT;
	CTimed*         mTimed;
	bool            mIsCounting;
	LPane*          mDisplayPane;
	std::string     mMarkString;
	std::string     mTimeString;
};

class CTimed {
public:
	         CTimed();
	virtual  ~CTimed();
	CTimer*        GetTimer() {return mTimer;}
	void           SetTimer(CTimer* inTimer);
	virtual void   TimeIsUp() = 0;
protected:
	CTimer* mTimer;
};

LStream& operator >> (LStream& inStream, SettingT& outSetting);
LStream& operator << (LStream& inStream, SettingT& inSetting);


#ifdef DEBUG

// this class will check for things that are still referenced
class UHasContainerAction : public UArrayAction {
protected:
	virtual bool DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData);
public:
	UHasContainerAction(AGalacticThingy *inReferencedThingy) {
		ASSERT(inReferencedThingy != nil);
		mReferencedThingy = inReferencedThingy; mHasContainer = false;
	}
	bool	HasContainer() {return mHasContainer;}
protected:
	AGalacticThingy* 	mReferencedThingy;
	bool				mHasContainer;
};

#endif // DEBUG

#ifdef DEBUG

// this class will check for things that are still referenced
class UHasReferencesAction : public UArrayAction {
protected:
	virtual bool DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData);
public:
	UHasReferencesAction(AGalacticThingy *inReferencedThingy) {
		ASSERT(inReferencedThingy != nil);
		mReferencedThingy = inReferencedThingy; mWasReferenced = false;
	}
	bool	WasReferenced() {return mWasReferenced;}
protected:
	AGalacticThingy* 	mReferencedThingy;
	bool				mWasReferenced;
};
#endif

#define REFRESH true
#define NO_REFRESH false

class UMoveReferencedThingysAction : public UArrayAction {
protected:
	virtual bool DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData);
public:
	UMoveReferencedThingysAction(AGalacticThingy *inReferencedThingy, bool inDoRefresh = REFRESH) {
		ASSERT(inReferencedThingy != nil);
		mReferencedThingy = inReferencedThingy; mDoRefresh = inDoRefresh;
	}
protected:
	AGalacticThingy* 	mReferencedThingy;
	bool				mDoRefresh;
};


class URemoveReferencesAction : public UArrayAction {
protected:
	virtual bool DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData);
public:
	URemoveReferencesAction(AGalacticThingy *inReferencedThingy, bool inDoRefresh = REFRESH) {
		ASSERT(inReferencedThingy != nil);
		mReferencedThingy = inReferencedThingy; mDoRefresh = inDoRefresh;
	}
protected:
	AGalacticThingy* 	mReferencedThingy;
	bool				mDoRefresh;
};

class UChangeIdReferencesAction : public UArrayAction {
protected:
	virtual bool DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData);
public:
	UChangeIdReferencesAction(PaneIDT oldID, PaneIDT newID) {
	    mOldId = oldID;
	    mNewId = newID;
	}
protected:
	PaneIDT				mOldId;
	PaneIDT				mNewId;
};

class CFleet;

class UFindSatelliteFleetAction : public UArrayAction {
public:
	UFindSatelliteFleetAction() {mFoundSatelliteFleet = nil;}
	bool            WasFound() {return (mFoundSatelliteFleet != nil);}
	CFleet*         GetSatelliteFleet() {return mFoundSatelliteFleet;}
protected:
	virtual bool    DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData);
	CFleet* mFoundSatelliteFleet;	
};


#endif // GALACTICA_UTILS_H_INCLUDED

