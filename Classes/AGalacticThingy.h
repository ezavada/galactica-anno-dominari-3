// AGalacticThingy.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ, 4/18/96 ERZ


#ifndef AGALACTIC_THINGY_H_INCLUDED
#define AGALACTIC_THINGY_H_INCLUDED

#include "GalacticaConsts.h"
#include "GalacticaUtils.h"
#include "ADataStore.h"
#include <ostream>
#include <vector>
#include <LPeriodical.h>
#include <CMemStream.h>

struct ThingyDataPacket;
class GalacticaClient;
class AGalacticThingyUI;
class LView;

enum EVisibilityLevels {
    visibility_None = 0,
    visibility_Speed = 1,
    visibility_Tech = 2,
    visibility_Class = 3,
    visibility_Details = 4,  // for a system this is ships at system, for ship undefined
    // add new levels here
    visibility_Max
};

enum {
    kConsiderVisibility = true,
    kIgnoreVisibility = false
};

class UThingyClassInfo : public UClassInfo {
public:
	UThingyClassInfo(long inClassType, EEventCode inDestructEvent = event_None, 
	                   UInt8 inMinVisibility = visibility_None, UInt8 inMaxVisibility = visibility_Details,
					   SInt16 inHotSpotSize = HOTSPOT_SIZE_DEFAULT, 
					   SInt16 inFrameSize = FRAME_SIZE_DEFAULT, bool inCanFleePursuit = false)
												: UClassInfo(inClassType) {
												  mSendDeleteInfo = true; 
												  mAllowWriteback = true;
												  mDestructEvent = inDestructEvent;
												  mFrameSize = inFrameSize;
												  mHotSpotSize = inHotSpotSize;
												  mCanFleePursuit = inCanFleePursuit;
												  mMaxVisibility = inMaxVisibility;
												  mMinVisibility = inMinVisibility; }
	UThingyClassInfo(long inClassType, bool inSendDelete, bool inAllowWrite) 
											    : UClassInfo(inClassType) {
												  mSendDeleteInfo = inSendDelete; 
												  mAllowWriteback = inAllowWrite;
												  mDestructEvent = 0;
												  mFrameSize = 0;
												  mHotSpotSize = 0;
												  mCanFleePursuit = false;
												  mMaxVisibility = visibility_Class;
												  mMinVisibility = visibility_None; }
	bool	GetSendDeleteInfo() const {return mSendDeleteInfo;}
	void	SetSendDeleteInfo(bool inSendIt) {mSendDeleteInfo = inSendIt;}
	bool	GetAllowWriteback() const {return mAllowWriteback;}
	void	SetAllowWriteback(bool inAllowIt) {mAllowWriteback = inAllowIt;}
	EEventCode	GetDestructionEventCode() const {return (EEventCode)mDestructEvent;}
	SInt16	GetFrameSize() const {return mFrameSize;}
	void	SetFrameSize(SInt16 inFrameSize) {mFrameSize = inFrameSize;}
	SInt16	GetHotSpotSize() const {return mHotSpotSize;}
	void	SetHotSpotSize(SInt16 inHotSpotSize) {mHotSpotSize = inHotSpotSize;}
	bool	GetCanFleePursuit() const {return mCanFleePursuit;}
	void	SetCanFleePursuit(bool inCanFleePursuit) {mCanFleePursuit = inCanFleePursuit;}
	UInt8   GetDefaultMaxVisibility() { return mMaxVisibility; }
	void    SetDefaultMaxVisibility(UInt8 visibility) { mMaxVisibility = visibility; }
	UInt8   GetDefaultMinVisibility() { return mMinVisibility; }
	void    SetDefaultMinVisibility(UInt8 visibility) { mMinVisibility = visibility; }
	static UInt32 GetScanRangeForTechLevel(UInt16 techLevel, bool isFastGame, bool hasFogOfWar) { 
	            // new formula, 1/2 original growth rate but same starting size
	            return hasFogOfWar ? (((techLevel + 7) * 1500UL) * (isFastGame ? 2UL : 1UL)) : 0;
//	            return ((techLevel + 3) * 3000UL) * (isFastGame ? 2UL : 1UL);
	        }
// stealth not based on tech level
//	static UInt8 GetStealthForTechLevel(UInt16 techLevel) {
//	            return (techLevel * 3UL / 4UL) + 1UL;
//	        } 
protected:
	bool	mSendDeleteInfo;
	bool	mAllowWriteback;
	bool    mCanFleePursuit;
	SInt16	mDestructEvent;
	SInt16	mFrameSize;
	SInt16	mHotSpotSize;
	UInt8   mMaxVisibility;
	UInt8   mMinVisibility;
};

#define classInfo_NoWriteback false
#define classInfo_AllowWriteback true
#define classInfo_DontSendDeleteInfo false
#define classInfo_SendDeleteInfo true
#define classInfo_CanFleePursuit true
#define classInfo_CannotFleePursuit false

#define lastSeen_Never (unsigned)-1
#define lastSeen_ThisTurn (unsigned)-2

extern UThingyClassInfo gShipClassInfo;
extern UThingyClassInfo gStarClassInfo;
extern UThingyClassInfo gFleetClassInfo;
extern UThingyClassInfo gWormHoleClassInfo;
extern UThingyClassInfo gStarLaneClassInfo;
extern UThingyClassInfo gRendezvousClassInfo;
extern UThingyClassInfo gMessageClassInfo;
extern UThingyClassInfo gDestructionMsgClassInfo;

enum ESendEvent {
    autoDetermineSend = 2,
    neverSend = false,
    alwaysSend = true
};

class AGalacticThingy;

typedef std::vector<AGalacticThingy*> StdThingyList;

typedef bool (*ContainerSearchFunc)(const void* item, void* searchData); // return true if item is the one you want

class AGalacticThingy {
public:
	static AGalacticThingy*		MakeThingyFromSubClassType(LView* inSuper, GalacticaShared* inGame, long inThingyType);
	static UThingyClassInfo*	GetClassInfoFromType(long inThingyType);

			AGalacticThingy(GalacticaShared* inGame, long inThingyType);
			virtual ~AGalacticThingy();
			
	void	InitThingy();

// ---- identity methods ----
    virtual SInt32  GetID() const;
    virtual void    SetID(SInt32 inID);
    virtual const char* GetName(bool considerVisibility = kConsiderVisibility);
    virtual const char* GetShortName(bool considerVisibility = kConsiderVisibility);
    virtual AGalacticThingyUI* AsThingyUI();

// ---- persistence/streaming methods ----
	virtual void	FinishInitSelf();		// do anything that was waiting til pane id was assigned
	virtual void	ReadStream(LStream *inStream, UInt32 streamVersion = version_SavedTurnFile);
	virtual void	WriteStream(LStream *inStream, SInt8 forPlayerNum = kNoSuchPlayer);
	virtual void	UpdateClientFromStream(LStream *inStream);	// doesn't alter user interface items
	virtual void	UpdateHostFromStream(LStream *inStream);	// doesn't alter anything controlled by host
	void			AssignUniqueID();	//  called by Persist to assign a unique identifier
    void            Persist();          // called when totally new object first created

    ThingyDataPacket*   WriteIntoPacket(SInt8 forPlayerNum = kNoSuchPlayer);
    bool                ReadFromPacket(ThingyDataPacket* inPacket);

 
// ---- multiplayer host only database methods ----
	void			   WriteToDataStore();
	void			   ReadFromDataStore(RecIDT inNewID = 0);
	void			   DeleteFromDataStore();	// only the host should ever call this

// ---- container methods ----
	virtual bool	    IsOfContentType(EContentTypes inType) const;
	virtual bool	    SupportsContentsOfType(EContentTypes inType) const;
	virtual void*	    GetContentsListForType(EContentTypes inType) const;
//	StdThingyList*      GetContentsListForType(EContentTypes inType) const;
	virtual SInt32	    GetContentsCountForType(EContentTypes inType) const;
	virtual bool	    AddContent(AGalacticThingy* inThingy, EContentTypes inType, bool inRefresh);
	virtual void	    RemoveContent(AGalacticThingy* inThingy, EContentTypes inType, bool inRefresh);
	virtual bool	    HasContent(AGalacticThingy* inThingy, EContentTypes inType = contents_Any);
	void			    AddEntireContentsToList(EContentTypes inType, LArray& ioList);
	void	            AddEntireContentsToList(EContentTypes inType, StdThingyList& ioList) const;
	AGalacticThingy*	GetContainer() const {return mCurrPos.GetThingy();}
	AGalacticThingy*	GetTopContainer();
	virtual void*		SearchContents(EContentTypes inType, ContainerSearchFunc funcPtr, void* searchData) const;

// ---- object info methods ----
	UInt16			    GetOwner() const {return mOwnedBy;}
	void			    SetOwner(UInt16 inOwner) {mOwnedBy = inOwner;}
	bool			    IsMyPlayers() const;
	bool			    IsOwned() const {return mOwnedBy;}
	bool			    IsAllyOf(UInt16 inOwner) const;
	bool			    IsInSpace() const {return (mCurrPos.GetType() == wp_Coordinates);}
	bool			    IsAtStar() const {return (mCurrPos.GetType() == wp_Star);}
	Point3D			    GetCoordinates() const;
	void 			    SetCoordinates(const Point3D inCoordinates, bool inRecalc = true);
	Waypoint		    GetPosition() const {return mCurrPos;}
	void			    SetPosition(Waypoint wp, bool inRefresh) {mCurrPos = wp; ThingMoved(inRefresh);}
	long			    GetTechLevel() const {return mTechLevel;}
	void			    SetTechLevel(UInt16 inLevel) {mTechLevel = inLevel;}
	UInt32			    GetDamage() const {return mDamage;}
	void			    SetDamage(UInt32 inDamage) {mDamage = inDamage;}
	bool                HasScanners() const { return (mScannerRange > 0); }
	UInt32              GetScannerRange() const { return mScannerRange; }
	void                SetScannerRange(UInt32 range) { mScannerRange = range; }
	UInt32              GetApparentSize() const { return mApparentSize; }
	void                SetApparentSize(UInt32 apparentSize) { mApparentSize = apparentSize; }
	UInt8               GetStealth() const { return mStealth; }
	void                SetStealth(UInt8 stealth) { mStealth = stealth; }
	UInt8               GetMaxVisibility() const { return mMaxVisibility; }
	void                SetMaxVisibility(UInt8 visibility) { mMaxVisibility = visibility; }
	UInt8               GetMinVisibility() const { return mMinVisibility; }
	void                SetMinVisibility(UInt8 visibility) { mMinVisibility = visibility; }
	bool                IsVisibleTo(UInt16 inOpponent) const { return (GetVisibilityTo(inOpponent) > visibility_None); }
	UInt8               GetVisibilityTo(UInt16 inOpponent) const;
	void                SetVisibilityTo(UInt16 inOpponent, UInt8 visibility);
	UInt32              GetLastSeen(UInt16 inOpponent) const;
	void                SetLastSeen(UInt16 inOpponent, UInt32 turnNum);
	PaneIDT             GetLastSeenByID(UInt16 inOpponent) const;
	void                SetLastSeenByID(UInt16 inOpponent, PaneIDT thingyID);
	void                ScannerChanged() { mScannerChanged = true; }
	bool                HasScannerChanged() { return mScannerChanged; }
	void                FlagForRescan() { mNeedsRescan = true;}
	bool                NeedsRescan() { return mNeedsRescan; }

	virtual ResIDT	    GetViewLayrResIDOffset();
	virtual long	    GetProduction() const;
	virtual long	    GetAttackStrength() const;
	virtual long	    GetDefenseStrength() const;
	virtual long	    GetDangerLevel() const;
	virtual void       Changed(); 
	virtual void	    NotChanged() {mChanged = false;}
	bool			    IsChanged() const {return mChanged;}
	UInt32			    LastChanged() const {return mLastChanged;}
	bool			    WantsAttention() const {return mWantsAttention;}
	void			    SetWantsAttention(bool inWantsIt) {mWantsAttention = inWantsIt;}
	bool			    IsDead() const {return (mNeedsDeletion > 0);}
	bool			    IsToBeScrapped() const {return (mNeedsDeletion == -1);}
	long			    GetThingySubClassType() {return mClassInfo->GetClassType();}
	void			    SetGame(GalacticaShared* inGameDoc);
	GalacticaDoc*       GetGameDoc() const;         // only call from client
	GalacticaShared*    GetGame() const {return mGame;}
	GalacticaClient*    GetGameClient() const;
	GalacticaProcessor* GetGameHost() const;  // only call from host
	bool                CanFleePursuit() const {return mClassInfo->GetCanFleePursuit();}
	virtual bool        ClientCanMoveMeAround() const {return false;}

// ---- host only turn processing methods ----
	virtual void        DoComputerMove(long inTurnNum);	// only call from host doc
	virtual void        PrepareForEndOfTurn(long inTurnNum);
	virtual void 	    EndOfTurn(long inTurnNum);	//only call from host doc
	virtual void	    FinishEndOfTurn(long inTurnNum);	// see details in .cp file for usage
	bool			    SendDeleteInfo() {return mClassInfo->GetSendDeleteInfo();}	// tells host to inform client of my deletion
	bool			    CanWriteback() {return mClassInfo->GetAllowWriteback();}	// tells client it can write class back to host
	SInt16			    GetDestructionEventCode() {return mClassInfo->GetDestructionEventCode();}
	void			    DestroyedInCombat(SInt32 inByPlayer, ESendEvent sendEvent = autoDetermineSend);
	virtual void	    Scrapped();
	virtual long	    DefendAgainst(AGalacticThingy* inAttacker, int inWeighting = 100, long rolloverDamage = 0);
	virtual long	    GetOneAttackAgainst(AGalacticThingy* inDefender);
	virtual long	    TakeDamage(long inHowMuch);
	virtual void	    Die() {mNeedsDeletion = 1; Changed();}
	virtual UInt8       CalculateVisibilityToThingy(AGalacticThingy* inThingy);
    virtual void        CalculateVisibilityToOpponent(SInt16 opponent);
	virtual void        CalculateVisibilityToAllOpponents(); // call when nothing has visibiltiy set
	virtual void        UpdateWhatIsVisibleToMe();  // call when something moves or changes it's ability to see

// ---- miscellaneous methods ----
	virtual void	ThingMoved(bool inRefresh);
	SInt32			Distance(Point3D &inCoordinates);
	SInt32			Distance(AGalacticThingy *anotherThingy);
	virtual bool	RefersTo(AGalacticThingy *inThingy);
	virtual void	RemoveReferencesTo(AGalacticThingy *inThingy);
	virtual void    ChangeIdReferences(PaneIDT oldID, PaneIDT newID);
	virtual void    RemoveSelfFromGame(); // take something that has already been marked as dead completely out of the game

protected:
    SInt32              mID;
	UThingyClassInfo*	mClassInfo;		// holds info about class
	GalacticaShared*    mGame;
	Waypoint            mCurrPos;
	UInt32              mLastChanged;	// when was this object last udated in the Datastore
	UInt16              mOwnedBy;
	UInt16				mTechLevel;
	SInt32				mDamage;
	PaneIDT				mNeedsDeletion;
	bool			    mChanged;
	bool		        mWantsAttention;	// will present itself to the player next turn
	SInt32				mLastCallin;	// turn in which item last called in
	// new stuff added in 2.3 for Fog of War
	UInt32              mScannerRange;
	UInt32              mApparentSize;
	UInt8               mStealth;
	UInt8               mMaxVisibility;
	UInt8               mMinVisibility;
	// the following are only calculated and held in memory, not written to the game file
	UInt8               mVisibilityTo[MAX_PLAYERS_CONNECTED];
	UInt32              mLastSeen[MAX_PLAYERS_CONNECTED];
	PaneIDT             mLastSeenByID[MAX_PLAYERS_CONNECTED];
	bool                mNeedsRescan;
	bool                mScannerChanged;
	// end new stuff for Fog of War
private:
	static CMemStream*	sThingyIOStream;
};

#ifdef DEBUG
	ostream& operator << (ostream& cout, AGalacticThingy *outThingy);
#endif


#endif // AGALACTIC_THINGY_H_INCLUDED


