//   GalacticaShared.h      
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#ifndef GALACTICA_SHARED_H_INCLUDED
#define GALACTICA_SHARED_H_INCLUDED

#include "GalacticaTypes.h"
#include <TArray.h>

#include <vector>

class GalacticaSharedUI;

class CStar;
class CShip;

class GalacticaShared {
public:
                  GalacticaShared();      // constructor
   virtual        ~GalacticaShared();   // stub destructor

   virtual void   GetPlayerInfo(int inPlayerNum, PublicPlayerInfoT &outInfo) const;
   virtual void   SetPlayerInfo(int inPlayerNum, PublicPlayerInfoT &inInfo);
   virtual void   GetPlayerName(int inPlayerNum, std::string &outName);
   virtual bool   GetPlayerNameAndStatus(int inPlayerNum, std::string &outName);
   virtual void   AddThingy(AGalacticThingy* inNewbornThingy);
   virtual void   RemoveThingy(AGalacticThingy* inDeadThingy);
   LArray*        GetEverythingList() {return &mEverything;}   
   virtual AGalacticThingy*   MakeThingy(long inThingyType);
   AGalacticThingy*           FindThingByID(PaneIDT inID);
   void           ValidateEverything();   // validate all the items in the everything list, remove bad ones

   void           DoForEverything(UArrayAction &inAction) {
                                 UArrayAction::DoForEachElement(mEverything, inAction);}

   CStar*         FindNeighboringStar(AGalacticThingy *inThingy, long inMinDist, long inMaxDist);
   CShip*         FindNearbyShip(AGalacticThingy *inThingy, long inMinDist, long inMaxDist);
   AGalacticThingy*  FindNearbyThingy(Point3D fromCoord, long inMinDist, long inMaxDist, 
                                 EContentTypes inType = contents_Any);
   CStar*         FindNearestLowerTechAlliedStar(AGalacticThingy *inFrom, long maxDist);
   CStar*         FindWeakestEnemyStar(AGalacticThingy *inFrom, long maxDist);
   CStar*         FindNearestEnemyTechProductionStar(AGalacticThingy *inFrom, long maxDist);
   void           FindClosestStars(AGalacticThingy *inFrom, CStar* outFound[3], long maxDist = 0x7fffffff, long minDist[3] = NULL);
   CShip*         FindClosestEnemyShip(AGalacticThingy *inFrom, int inEnemyOf, long maxDist = 0x7fffffff, long *minDist = NULL);

    std::vector<AGalacticThingy*>* GetScannersForPlayer(int inPlayerNum);
    void            ClearAllScannerLists();

   virtual bool   IsSinglePlayer() const {return false;} // single player means no separate host, not just one human
   virtual bool   IsHost() const {return false;}
   virtual bool   IsClient() const {return false;}
   bool           IsClosing() const {return mClosing;}
   bool           HasUI() const { return (mSharedUI != NULL); }
   GalacticaSharedUI*   AsSharedUI() { return mSharedUI; }
   void           StartClosing() {mClosing = true;}
   bool           IsFastGame() const {return mGameInfo.fastGame;}
   bool           HasFogOfWar() const {return !(mGameInfo.omniscient);}
   int            GetComputerSkill() const {return mGameInfo.compSkill;}
   int            GetComputerAdvantage() const {return mGameInfo.compAdvantage;}
   int            GetNumPlayers() const {return mGameInfo.totalNumPlayers;}
   int            GetNumHumans() const {return mGameInfo.numHumans;}
   bool           PlayerIsHuman(int inNum) const;
   bool           PlayerIsComputer(int inNum) const;
   long           GetTurnNum() const {return mGameInfo.currTurn;}
   void           LogGameInfo(); 
   void           StartStreaming() {sStreamingGame = this;}
   void           StopStreaming() {sStreamingGame = NULL;}
   GameInfoT      GetGameInfo() {return mGameInfo;}
    virtual void        FinishGameLoad();   // do anything necessary when game is loaded
    
   // following is a UI method, but is needed here because many classes that aren't UI classes
   // need to provide feedback to the user that they are in the middle of a time consuming operation.
   // in a non-GUI situation, this will do nothing.
   // in a GUI situation, this will be extended by something that puts up a busy cursor or a progress
   // bar
   virtual void   NotifyBusy(long inCurrUnit = 0, long inTotalUnits = 0);
    
   static GalacticaShared*    GetStreamingGame();
   
   static void    StatusFromPublicPlayerInfo(PublicPlayerInfoT& inInfo, std::string &outStatus);

protected:
   TArray<AGalacticThingy*>   mEverything;
   GameInfoT                  mGameInfo;
   PublicPlayerInfoT          mPublicPlayerInfo[MAX_PLAYERS_CONNECTED];
   bool                       mClosing;// Shared UI member variables
   GalacticaSharedUI*         mSharedUI;
   bool                       mAdminOnline;
	std::vector<AGalacticThingy*>*  mScannersList[MAX_PLAYERS_CONNECTED];

private:
   static  GalacticaShared*  sStreamingGame;
};

#endif // GALACTICA_SHARED_H_INCLUDED


