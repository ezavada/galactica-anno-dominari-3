// CRendezvous
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 9/22/97 ERZ

#ifndef CRENDEZVOUS_H_INCLUDED
#define CRENDEZVOUS_H_INCLUDED

#include "CFleet.h"

typedef class CRendezvous : public ANamedThingy {
public:
			CRendezvous(LView *inSuperView, GalacticaShared* inGame);
//			CRendezvous(LView *inSuperView, GalacticaShared* inGame, LStream *inStream);
			virtual ~CRendezvous();
			
	void InitRendezvous();

// ---- database/streaming methods ----
	virtual void	ReadStream(LStream *inStream, UInt32 streamVersion = version_SavedTurnFile);
	virtual void	WriteStream(LStream *inStream, SInt8 forPlayerNum = kNoSuchPlayer);
	virtual void	UpdateClientFromStream(LStream *inStream);	// doesn't alter user interface items
	virtual void	UpdateHostFromStream(LStream *inStream);	// doesn't alter anything controlled by host

// ---- container methods ----

// ---- object info methods ----
	virtual bool   ClientCanMoveMeAround() const {return true;} // rendezvous points can be dragged anywhere by user
	virtual ResIDT	GetViewLayrResIDOffset();
	virtual void	SetFleet(CFleet* inFleet) {mFleetRef = inFleet;}
	virtual CFleet*	GetFleet() {return (CFleet*)mFleetRef;}

// ---- host only turn processing methods ----
	virtual void	FinishEndOfTurn(long inTurnNum);	// see details in AGalacticThingy.cp file for usage

// =========== GUI ONLY ===========

#ifndef GALACTICA_SERVER

// ---- mouse handling methods ----
	virtual Boolean	DragSelf(SMouseDownEvent &);
	virtual Boolean	IsHitBy(SInt32 inHorizPort, SInt32 inVertPort);

// ---- selection methods ----

// ---- drawing methods ----
	virtual void 	DrawSelf(void);

// ---- miscellaneous methods ----
	void	        RemoveSelfFromClient();

#endif // GALACTICA_SERVER

protected:
	TThingyRef<CFleet*>	mFleetRef;

	void			ReadRendezvousStream(LStream &inStream);
} CRendezvous, *CRendezvousPtr;


#endif // CRENDEZVOUS_H_INCLUDED
