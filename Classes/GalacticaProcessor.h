//	GalacticaProcessor.h		
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#ifndef GALACTICA_PROCESSOR_H_INCLUDED
#define GALACTICA_PROCESSOR_H_INCLUDED

#include "GalacticaShared.h"
#include <LArray.h>
#include <LArrayIterator.h>
#include <vector>

class GalacticaProcessor : 
  #ifndef GALACTICA_SERVER
    virtual 
  #endif // !GALACTICA_SERVER
    public GalacticaShared {
public:
						GalacticaProcessor();		// constructor
	virtual 			~GalacticaProcessor();	// stub destructor

	virtual void		GetPrivatePlayerInfo(int inPlayerNum, PrivatePlayerInfoT &outInfo);
	virtual void		SetPrivatePlayerInfo(int inPlayerNum, PrivatePlayerInfoT &inInfo);
	virtual void		AddThingy(AGalacticThingy* inNewbornThingy);
	virtual void		RemoveThingy(AGalacticThingy* inDeadThingy);
	bool				ProcessOneComputerMove();
	void				FinishTurn();
	virtual void		DoEndTurn();
	virtual void		SetStatusMessage(int inMessageNum, const char* inExtra);
	virtual void		PrepareForEndOfTurn();
	virtual void		WriteChangesToHost();
	void		        FillSector(int inDensity, int inSectorSize);
    virtual void        CreateNew(NewGameInfoT &inGameInfo);
    virtual void        FinishGameLoad();   // do anything necessary when game is loaded

	SInt32				GetLastStarIndexInEverythingList() {return mLastStarIndex;}

	virtual bool	   IsHost() const {return true;}

	bool	PlayerHasNewHullType(int inPlayerNum, int inHullType, long inNewTechLevel);
	long	GetPlayersHighestShipTech(int inPlayerNum) const;
	bool	PlayerHasNewShipTech(int inPlayerNum, long inNewTech);
	long	GetPlayersHighestFighterClass(int inPlayerNum) const;
	bool	PlayerHasNewFighterClass(int inPlayerNum, long inNewTechLevel);
	long	GetPlayersHighestStarTech(int inPlayerNum) const;
	bool	PlayerHasNewStarTech(int inPlayerNum, long inNewTech);
	long	GetPlayersHighestStarClass(int inPlayerNum) const;
	bool	PlayerHasNewStarClass(int inPlayerNum, long inNewTechLevel);
	long    GetTurnsBetweenCallins() const {return mTurnsBetweenCallins;}

protected:
	TArray<AGalacticThingy*> mNewThingys;
	TArray<ThingDBInfoT>     mDeadThingys;
	LArrayIterator           mEverythingIterator;
	PrivatePlayerInfoT       mPlayers[MAX_PLAYERS_CONNECTED];
	bool		   mComputerMovesRemainingToProcess;
	bool		   mDoingWork;
	long        mTurnsBetweenCallins;
	long        mLastStarIndex;
};

#endif // GALACTICA_PROCESSOR_H_INCLUDED

