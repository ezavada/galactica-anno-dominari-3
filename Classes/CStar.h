// CStar.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ
// modified: 10/12/97, ERZ v1.2 changes
// renamed from GalacticaThingys: 2/3/02, ERZ v2.1b3 changes


#ifndef CSTAR_H_INCLUDED
#define CSTAR_H_INCLUDED

#include "ANamedThingy.h"
#include "CThingyList.h"

class CShip;
class CStarLane;

typedef CStarLane* LaneInfoT;

class CStar : public ANamedThingy /*, public LPeriodical*/ {
public:
			CStar(LView *inSuperView, GalacticaShared* inGame);
//			CStar(LView *inSuperView, GalacticaShared* inGame, LStream *inStream);
			
// ---- database/streaming methods ----
	virtual void	FinishInitSelf();
	virtual void	ReadStream(LStream *inStream, UInt32 streamVersion = version_SavedTurnFile);
	virtual void	WriteStream(LStream *inStream, SInt8 forPlayerNum = kNoSuchPlayer);
	virtual void	UpdateHostFromStream(LStream *inStream);	// doesn't alter anything controlled by host
  #ifndef GALACTICA_SERVER
	virtual void	UpdateClientFromStream(LStream *inStream);	// doesn't alter user interface items
  #endif // GALACTICA_SERVER

// ---- container methods ----
	virtual bool	IsOfContentType(EContentTypes inType) const;
	virtual bool    SupportsContentsOfType(EContentTypes inType) const;
	virtual void*	GetContentsListForType(EContentTypes inType) const;
	virtual SInt32	GetContentsCountForType(EContentTypes inType) const;
	virtual bool	AddContent(AGalacticThingy* inThingy, EContentTypes inType, bool inRefresh);
	virtual void	RemoveContent(AGalacticThingy* inThingy, EContentTypes inType, bool inRefresh);
	virtual bool	HasContent(AGalacticThingy* inThingy, EContentTypes inType = contents_Any);
	void            AddToSatelliteFleet(AGalacticThingy* inThingy);
	CFleet*         GetSatelliteFleet(); // returns NULL if none present


// ---- object info methods ----
	virtual SInt32	GetProduction() const;
   virtual SInt32   GetAttackStrength() const;
	virtual SInt32  GetDefenseStrength() const;
	virtual SInt32  GetDangerLevel() const;
	virtual SInt32  GetSpending(ESpending inWhichItem) const;
	virtual float   GetBenefits(ESpending inWhichItem) const;
	SettingT*       GetSettingPtr(ESpending inSettingNum) {return &mSettings[inSettingNum];}
	UInt32          GetPopulation() const {return mPopulation;}
	void            SetPopulation(UInt32 inPopulation);
	void            GrowPopulation();
	bool            IsTechStar() {return (mSettings[spending_Research].desired > 400);}
	Waypoint        GetNewShipDestination() const {return mNewShipDestination;}
	void            SetNewShipDestination(Waypoint inDest) {mNewShipDestination = inDest;}
	int             GetEstimatedTurnsToNextShip() const;
	int             GetShipPercentComplete() const {return (int)((mPaidTowardNextShip[mBuildHullType]*100)/mCostOfNextShip);}
	void            SetPaidTowardNextShip(float inPaid) {mPaidTowardNextShip[mBuildHullType] = inPaid;}
	void            SetPercentPaidTowardNextShip(float inPercentPaid) {mPaidTowardNextShip[mBuildHullType] = inPercentPaid * mCostOfNextShip;}
	void            AddToPaidTowardNextShip(float inBonus) {mPaidTowardNextShip[mBuildHullType] += inBonus;}
	void            RecalcPatrolStrength();
	SInt32          GetTotalDefenseStrength() const;
	SInt32          GetNonSatelliteShipStrength() const;
	SInt32          GetPatrolStrength() const {return mPatrolStrength;}
	void            SetPatrolStrength(SInt32 inStrength) {mPatrolStrength = inStrength;}
	SInt16          GetBuildHullType() const {return mBuildHullType;}
	void            SetBuildHullType(SInt16 inHullType);
    virtual const char* GetName(bool considerVisibility = kConsiderVisibility);
    virtual const char* GetShortName(bool considerVisibility = kConsiderVisibility);


// ---- host only turn processing methods ----
	virtual void	DoComputerMove(long inTurnNum);		//only call from host doc
	virtual void 	EndOfTurn(long inTurnNum);			//only call from host doc
	virtual void	FinishEndOfTurn(long inTurnNum);	//only call from host doc
	virtual void    BuildNewShip(EShipClass inHullType);	//only call from host doc


// ---- miscellaneous methods ----
	virtual	void	SpendTime(const EventRecord &inMacEvent);
	void            CreateStarLanes(int inNumLanes);
	bool            EstablishStarLaneTo(CStar* inStar);
	bool            AddStarLaneFrom(CStar* inStar, CStarLane *inLane);
	virtual long    TakeDamage(SInt32 inHowMuch);
	virtual void    Die();
	virtual bool    RefersTo(AGalacticThingy *inThingy);
	virtual void    RemoveReferencesTo(AGalacticThingy *inThingy);
	virtual void    ChangeIdReferences(PaneIDT oldID, PaneIDT newID);

// ======== GUI ONLY ================

#ifndef GALACTICA_SERVER

// ---- mouse handling methods ----
	virtual void    DoubleClickSelf();
	virtual Boolean IsHitBy(SInt32 inHorizPort, SInt32 inVertPort);

// ---- selection methods ----


// ---- drawing methods ----
	virtual float   GetBrightness(short inDisplayMode);
	virtual Rect    GetCenterRelativeFrame(bool full = false) const;
	virtual float   GetScaleFactorForZoom() const;
	virtual bool    DrawSphereOfInfluence(int level = -1);
	virtual void    DrawSelf();

#endif // GALACTICA_SERVER

protected:
	CThingyList		mShipRefs;
	Waypoint		mNewShipDestination;
	SInt16			mNumStarLanes;
	SInt16			mBuildHullType;
	float			mCostOfNextShip;
	float			mPaidTowardNextShip[NUM_SHIP_HULL_TYPES];
	SInt16			mPaidTowardNextTech;
	UInt32			mPopulation;	// actual population / 1000
	SInt32			mPatrolStrength;
	LaneInfoT		mStarLanes[MAX_STAR_LANES];
	SettingT	    mSettings[MAX_STAR_SETTINGS];
	static int      sMaxShipNames;
private:
	void            ReadStarStream(LStream &inStream, bool inUpdating = false);
};

/*
typedef class CStarLane : public AGalacticThingy {
public:
			CStarLane(LView *inSuperView, GalacticaShared* inGame);
			CStarLane(LView *inSuperView, GalacticaShared* inGame, LStream *inStream);
			
	virtual void	ReadStream(LStream *inStream);
	virtual void	WriteStream(LStream *inStream);
	virtual void	UpdateClientFromStream(LStream *inStream);	// doesn't alter user interface items
	virtual void	UpdateHostFromStream(LStream *inStream);	// doesn't alter anything controlled by host
	void			RecordStarPositions(const CStar* fromStar, const CStar* toStar);
	CStar*			GetOtherStar(CStar* inStar);
	virtual void 	DrawSelf(void);
	
protected:
	bool		mLeft2Right;
	CStar*		mFromStar;
	CStar*		mToStar;
private:
	void			ReadStarLaneStream(LStream &inStream);	//internal use only!!!
} CStarLane, *CStarLanePtr, **CStarLaneHnd, **CStarLaneHdl;
*/

#endif //CSTAR_H_INCLUDED

