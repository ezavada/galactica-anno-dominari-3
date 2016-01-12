// CFleet
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 9/11/97 ERZ
// 1/31/98, ERZ added FleetCombatInfo
// 9/7/98, ERZ eliminated FinishStreaming()

#ifndef CFLEET_H_INCLUDED
#define CFLEET_H_INCLUDED

#include "CShip.h"
#include "CThingyList.h"

struct FleetCombatInfoT {
	UInt32	fleetPower;
	UInt32	numShips;
	Bool8   isSubFleet;
};

class CFleet : public CShip {
public:
			CFleet(LView *inSuperView, GalacticaShared* inGame);
//			CFleet(LView *inSuperView, GalacticaShared* inGame, LStream *inStream);
			virtual ~CFleet();
			
	void InitFleet();

// ---- database/streaming methods ----
	virtual void	FinishInitSelf();	// used to set the fleet's default name
	virtual void	ReadStream(LStream *inStream, UInt32 streamVersion = version_SavedTurnFile);
	virtual void	WriteStream(LStream *inStream, SInt8 forPlayerNum = kNoSuchPlayer);
	virtual void	UpdateClientFromStream(LStream *inStream);	// doesn't alter user interface items
	virtual void	UpdateHostFromStream(LStream *inStream);	// doesn't alter anything controlled by host

// ---- mouse handling methods ----

// ---- selection methods ----

// ---- drawing methods ----

// ---- container methods ----
	virtual bool	SupportsContentsOfType(EContentTypes inType) const;
	virtual void*	GetContentsListForType(EContentTypes inType) const;
	virtual SInt32	GetContentsCountForType(EContentTypes inType) const;
	virtual bool	AddContent(AGalacticThingy* inThingy, EContentTypes inType, bool inRefresh);
	virtual void	RemoveContent(AGalacticThingy* inThingy, EContentTypes inType, bool inRefresh);
	virtual bool	HasContent(AGalacticThingy* inThingy, EContentTypes inType = contents_Any);

// ---- object info methods ----
    virtual const char* GetName(bool considerVisibility = kConsiderVisibility);
    virtual const char* GetShortName(bool considerVisibility = kConsiderVisibility);
	bool     IsSatelliteFleet() const;
	void	 BecomeSatelliteFleet();
	bool     HasShipClass(EShipClass inClass) const;

#ifndef GALACTICA_SERVER
	virtual StringPtr	GetDescriptor(Str255 outDescriptor) const;
#endif // GALACTICA_SERVER

// ---- movement/course methods ----
    virtual void   	SetScrap(bool inScrap);
	virtual void	SetPatrol(bool inPatrol);

// ---- host only turn processing methods ----
	void			RemoveDead();
    virtual void   	EndOfTurn(long inTurnNum);
	virtual void	FinishEndOfTurn(long inTurnNum);	// see details in AGalacticThingy.cp file for usage
	virtual long	RepairDamage(long inHowMuch);

// ---- miscellaneous methods ----
	void			CalcFleetInfo();
	virtual void   	ChangeIdReferences(PaneIDT oldID, PaneIDT newID);

// ---- special method for fleets in combat. Host only.
	FleetCombatInfoT*	GetCombatInfoPtr() {return &mCombatInfo;}
	void				RecordCombatInfo();
	void				RecordCombatResults();

protected:
	CThingyList			mShipRefs;		// v1.2b8
	FleetCombatInfoT	mCombatInfo;	// used only during combat cycle
	bool                mInCombat;      // true if we are between a RecordCombatInfo and RecordCombatResults call
private:
	void			ReadFleetStream(LStream &inStream, bool inUpdating);
};


#endif	// CFLEET_H_INCLUDED


