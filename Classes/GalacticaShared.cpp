//   GalacticaShared.cpp      
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include "GalacticaShared.h"
#include "AGalacticThingy.h"
#include "GalacticaProcessor.h"
#include "CStar.h"
#include "CShip.h"

#warning TODO: make all these GalacticaShared FindXXX() methods work for client as well

#warning TODO: make GalacticaShared everything list use std::vector

#if TUTORIAL_SUPPORT
   #include "GalacticaTutorial.h"
#endif //TUTORIAL_SUPPORT

GalacticaShared*  GalacticaShared::sStreamingGame = NULL;

// this is currently only used by ThingyRefs when
// they read themselves off a stream and need to know
// what game object they are connected to
// static
GalacticaShared*
GalacticaShared::GetStreamingGame() {
   return sStreamingGame;
}

GalacticaShared::GalacticaShared() 
{
   DEBUG_OUT("New GalacticaShared", DEBUG_IMPORTANT);
   mGameInfo.recHeader.recID = -1;
   mGameInfo.currTurn = -1;
   mGameInfo.maxTimePerTurn = -1;
   mGameInfo.totalNumPlayers = -1;
   mGameInfo.numPlayersRemaining = -1;
   mGameInfo.numHumans = -1;
   mGameInfo.numComputers = -1;
   mGameInfo.numHumansRemaining = -1;
   mGameInfo.numComputersRemaining = -1;
   mGameInfo.hasHost = false;
   mGameInfo.gameTitle[0] = 0;
   for (int i = 0; i<MAX_PLAYERS_CONNECTED; i++) {
      mPublicPlayerInfo[i].isLoggedIn = false;
      mPublicPlayerInfo[i].isComputer = false;
      mPublicPlayerInfo[i].isAssigned = false;
      mPublicPlayerInfo[i].isAlive = false;
      mPublicPlayerInfo[i].name[0] = 0;
	  mScannersList[i] = 0;
   }
   mClosing = false;
   mSharedUI = NULL;
   mAdminOnline = false;
}

GalacticaShared::~GalacticaShared() {
	StartClosing(); // v2.1.3r5, make sure everyone knows we are going away
}

void
GalacticaShared::GetPlayerInfo(int inPlayerNum, PublicPlayerInfoT& outInfo) const {
   if ( inPlayerNum == kAdminPlayerNum) {
      DEBUG_OUT("GetPlayerInfo for Admin"
            << (char*)(mAdminOnline ? "Logged In" : ""), DEBUG_TRIVIA);
      outInfo.isAlive = false;
      outInfo.isComputer = false;
      outInfo.isAssigned = true;
      outInfo.isLoggedIn = mAdminOnline;
      std::strcpy(outInfo.name, ADMINISTRATOR_NAME);
   } else if ( (inPlayerNum > 0) && (inPlayerNum <= MAX_PLAYERS_CONNECTED) ) {
      outInfo = mPublicPlayerInfo[inPlayerNum - 1];
      DEBUG_OUT("GetPlayerInfo #"<<inPlayerNum<<": "
         << (char*)(outInfo.isLoggedIn ? "Logged In, " : "")
         << (char*)(outInfo.isAssigned ? "Assigned, " : "Unassigned, ")
         << (char*)(outInfo.isComputer ? "AI, " : "Human, ")
         << (char*)(outInfo.isAlive ? "Alive, " : "Defeated, ")
         << outInfo.name, DEBUG_TRIVIA);
      if (IsSinglePlayer()) {
         if (inPlayerNum == 1) {
            outInfo.isAssigned = true; // single player, slot one always assigned
         }
         outInfo.isLoggedIn = false;   // single player, never logged in
      }
   } else {
      DEBUG_OUT("Requested player info for impossible player #"<<inPlayerNum, DEBUG_ERROR);
   }
}

void
GalacticaShared::SetPlayerInfo(int inPlayerNum, PublicPlayerInfoT& inInfo) {
   if ( inPlayerNum == kAdminPlayerNum) {
      mAdminOnline = inInfo.isLoggedIn;
      DEBUG_OUT("SetPlayerInfo for Admin "
            << (char*)(mAdminOnline ? "Logged In" : ""), DEBUG_TRIVIA);
   } else if ( (inPlayerNum > 0) && (inPlayerNum <= MAX_PLAYERS_CONNECTED) ) {
        mPublicPlayerInfo[inPlayerNum - 1] = inInfo;
        DEBUG_OUT("SetPlayerInfo #"<<inPlayerNum<<": "
            << (char*)(inInfo.isLoggedIn ? "Logged In, " : "")
            << (char*)(inInfo.isAssigned ? "Assigned, " : "Unassigned, ")
            << (char*)(inInfo.isComputer ? "AI, " : "Human, ")
            << (char*)(inInfo.isAlive ? "Alive, " : "Defeated, ")
            << inInfo.name, DEBUG_DETAIL);
    } else {
        DEBUG_OUT("Tried to set player info for impossible player #"<<inPlayerNum, DEBUG_ERROR);
    }
}

void
GalacticaShared::GetPlayerName(int inPlayerNum, std::string &outName) {
   if (inPlayerNum == kAdminPlayerNum) {
      outName.assign(ADMINISTRATOR_NAME);
      return;
   }
   if (inPlayerNum < 1) {   // player num less than 1 is unowned
      LoadStringResource(outName, STRx_Names, str_Uncontrolled);
      return;
   }                  // player num greater that num players in game is unknown
   if (inPlayerNum > mGameInfo.totalNumPlayers) {
      LoadStringResource(outName, STRx_General, str_Unknown_);
      return;
   }
   PublicPlayerInfoT thePlayerInfo;          // so read in the player info so we can
   GetPlayerInfo(inPlayerNum, thePlayerInfo);   // extract the name.
   outName.assign(thePlayerInfo.name);
}


// returns false if the player number is not a known player, which could be for any
// of the following reasons:
//    1. inPlayerNum was 0 (zero), representing nobody/unowned system owner
//    2. inPlayerNum was greater than total number of players
//    3. inPlayerNum is a human player slot that is not yet assigned to a player
//    4. inPlayerNum is the Administrator
bool
GalacticaShared::GetPlayerNameAndStatus(int inPlayerNum, std::string &outName) {
   PublicPlayerInfoT thePlayerInfo;          // so read in the player info so we can
   if (inPlayerNum == kAdminPlayerNum) {
      GetPlayerInfo(kAdminPlayerNum, thePlayerInfo);
      StatusFromPublicPlayerInfo(thePlayerInfo, outName);
      return false;  // not a real player though
   }
   if (inPlayerNum < 1) {   // player num less than 1 is unowned
      LoadStringResource(outName, STRx_Names, str_Uncontrolled);
      return false;
   }                  // player num greater that num players in game is unknown
   if (inPlayerNum > mGameInfo.totalNumPlayers) {
      LoadStringResource(outName, STRx_General, str_Unknown_);
      return false;
   }
   GetPlayerInfo(inPlayerNum, thePlayerInfo);   // extract the name.
   StatusFromPublicPlayerInfo(thePlayerInfo, outName);
   return (thePlayerInfo.isAssigned || thePlayerInfo.isComputer);
}

bool 
GalacticaShared::PlayerIsHuman(int inNum) const {
   return !PlayerIsComputer(inNum);
}

bool
GalacticaShared::PlayerIsComputer(int inNum) const {
   PublicPlayerInfoT thePlayerInfo;
   GetPlayerInfo(inNum, thePlayerInfo);
   return thePlayerInfo.isComputer;
}

// static method
void
GalacticaShared::StatusFromPublicPlayerInfo(PublicPlayerInfoT& inInfo, std::string &outStatus) {
  if (inInfo.isAssigned || inInfo.isComputer) {
      outStatus.assign(inInfo.name);
      if (!inInfo.isAlive) {
         outStatus.append(" (");
         std::string defeatedStr;
         LoadStringResource(defeatedStr, STRx_General, str_Defeated);
         outStatus.append(defeatedStr);
         if (!inInfo.isLoggedIn && !inInfo.isComputer) {
            outStatus.append(")");
         }
      }
      if (inInfo.isLoggedIn) {
         if (!inInfo.isAlive) {
            outStatus.append(", ");
         } else {
            outStatus.append(" (");
         }
         std::string onlineStr;
         LoadStringResource(onlineStr, STRx_General, str_Online);
         outStatus.append(onlineStr);
         outStatus.append(")");
      } else if (inInfo.isComputer) {
         if (!inInfo.isAlive) {
            outStatus.append(", ");
         } else {
            outStatus.append(" (");
         }
         std::string computerStr;
         LoadStringResource(computerStr, STRx_Names, str_Computer);
         outStatus.append(computerStr);
         // computer string has trailing space, eliminate it
         outStatus.replace(outStatus.length()-1, 1, ")");
      }
   } else {
      LoadStringResource(outStatus, STRx_General, str_Open);
   }
}

AGalacticThingy*
GalacticaShared::MakeThingy(long inThingyType) {
   return AGalacticThingy::MakeThingyFromSubClassType(NULL, this, inThingyType);
}

void
GalacticaShared::AddThingy(AGalacticThingy* inNewbornThingy) {
   DEBUG_OUT("Adding to everything list "<< inNewbornThingy, DEBUG_TRIVIA | DEBUG_OBJECTS);
   bool putInFront = inNewbornThingy->GetThingySubClassType() == thingyType_Star;
  #if TUTORIAL_SUPPORT
   if (Tutorial::TutorialIsActive()) {
      putInFront = false;
   }
  #endif // TUTORIAL_SUPPORT
   if (putInFront) {
      // put stars in front
      mEverything.InsertItemsAt(1, LArray::index_First, inNewbornThingy);
   } else {
      // put everything else at the end
      mEverything.InsertItemsAt(1, LArray::index_Last, inNewbornThingy);
   }
}

void
GalacticaShared::RemoveThingy(AGalacticThingy* inDeadThingy) {
   mEverything.Remove(inDeadThingy);   // move to above check
#ifdef DEBUG
   if (!mClosing) {     // don't do this when the entire everything list is going away
       UHasReferencesAction checkRefs(inDeadThingy);
       DoForEverything(checkRefs);
       if (checkRefs.WasReferenced()) {
          DEBUG_OUT("Deleted " << inDeadThingy << " which was still referenced by other things", DEBUG_ERROR);
       }
   }
#endif
}

AGalacticThingy*
GalacticaShared::FindThingByID(PaneIDT inID) {
   AGalacticThingy   *theThing = NULL;
   AGalacticThingy   *it;
   int count = mEverything.GetCount();
   for (int i = 1; i<=count; i++) {
      it = *(AGalacticThingy**) mEverything.GetItemPtr(i);
      if (it->GetID() == inID) {
         theThing = it;
         break;
      }
   }
   return theThing;
}

void
GalacticaShared::ValidateEverything() {
   // validate all the items in the everything list, remove bad ones
   AGalacticThingy   *aThingy = NULL;
   LArrayIterator iterator(mEverything, LArrayIterator::from_Start); // Search list of all thingys created
   while (iterator.Next(&aThingy)) {
      aThingy = ValidateThingy(aThingy);
      ASSERT(aThingy != NULL);
    #ifndef DEBUG     // we allow the crash in debug mode, since we want to find
      if (!aThingy) {   // and fix this. In final build, skip the NULL thingy
         iterator.Current(&aThingy);
         mEverything.Remove(aThingy);
         continue;
      }
    #endif
   }
}

CStar*
GalacticaShared::FindNeighboringStar(AGalacticThingy *inThingy, long inMinDist, long inMaxDist) {
   AGalacticThingy *found = FindNearbyThingy(inThingy->GetCoordinates(), inMinDist, inMaxDist, contents_Stars);
   return static_cast<CStar*>(found);
}

CShip*
GalacticaShared::FindNearbyShip(AGalacticThingy *inThingy, long inMinDist, long inMaxDist) {
   Point3D fromCoord = inThingy->GetCoordinates();
   AGalacticThingy *found = FindNearbyThingy(fromCoord, inMinDist, inMaxDist, contents_Ships);
   return static_cast<CShip*>(found);
}

AGalacticThingy*
GalacticaShared::FindNearbyThingy(Point3D fromCoord, long inMinDist, long inMaxDist, 
                        EContentTypes inType) {
   AGalacticThingy   *goodNeighbor = nil;
   long currDist = inMaxDist + 1;
   AGalacticThingy   *aNeighbor;
   LArray* everything = GetEverythingList();
   int numItems = everything->GetCount();
   for (int i = 1; i <= numItems; i++) {
      everything->FetchItemAt(i, &aNeighbor);
      if (aNeighbor->IsDead()) {
         continue;   // skip this one, it's dead
      }
      if (aNeighbor->GetContainer() != NULL) {
          continue;   // skip this one, it's inside something else
      }
//      aNeighbor = aNeighbor->GetTopContainer(); // ERZ, 2.1b9r13, should no longer be necessary, 
//      since we are skipping things inside of other things
      if (!aNeighbor->IsOfContentType(inType)) {
         continue;   // wrong type of object
      }
      long dist = aNeighbor->Distance(fromCoord);
      if ( (dist <= inMaxDist) && (dist >= inMinDist) && (dist < currDist) ) {
         currDist = dist;
         goodNeighbor = aNeighbor;
      }
   }
   return goodNeighbor;
}


CShip*
GalacticaShared::FindClosestEnemyShip(AGalacticThingy *inFrom, int inEnemyOf, long maxDist, long *minDist) {
    GalacticaProcessor* host = dynamic_cast<GalacticaProcessor*>(this);
    if (!host) {
        DEBUG_OUT("CStarMapView::FindClosestEnemyShip() called from client", DEBUG_ERROR);
        return NULL;
    }
   CShip   *closest = nil;
   long currDist = maxDist;
   AGalacticThingy   *aNeighbor;
   Point3D fromCoord = inFrom->GetCoordinates();
   LArray* everything = GetEverythingList();
   int numItems = everything->GetCount();
   // v1.2b11d4, don't check stars, start just after last one
   int start = host->GetLastStarIndexInEverythingList() + 1;
  #ifdef DEBUG
    everything->FetchItemAt(start - 1, &aNeighbor);
    if (aNeighbor) {
        ASSERT(aNeighbor->GetThingySubClassType() == thingyType_Star);
    }
  #endif // DEBUG
   for (int i = start; i <= numItems; i++) {
      everything->FetchItemAt(i, &aNeighbor);
      long type = aNeighbor->GetThingySubClassType();
      if (aNeighbor->GetOwner() == inEnemyOf) {
         continue;   // it's a friendly
      }
      if ((type != thingyType_Ship) && (type != thingyType_Fleet)) {
          ASSERT(type != thingyType_Star);
         continue;   // not a ship
      }
      if (aNeighbor->IsDead()) {
         continue;   // skip this one, it's dead
      }
      if (HasFogOfWar() && !aNeighbor->IsVisibleTo(inEnemyOf)) {
        continue;   // skip this one, we can't see it
      }
      EWaypointType where = aNeighbor->GetPosition();
      if ( where == wp_Coordinates) {   // don't include ships at stars or in fleets
         long dist = aNeighbor->Distance(fromCoord);
         if ((dist < currDist) && ((minDist == NULL) || (dist > *minDist))) {
            currDist = dist;
            closest = static_cast<CShip*>(aNeighbor);
         }
      }
   }
   if (minDist != NULL) {
      *minDist = currDist;
   }
   return closest;
}

CStar*
GalacticaShared::FindNearestLowerTechAlliedStar(AGalacticThingy *inFrom, long maxDist) {
    GalacticaProcessor* host = dynamic_cast<GalacticaProcessor*>(this);
    if (!host) {
        DEBUG_OUT("CStarMapView::FindNearestLowerTechAlliedStar() called from client", DEBUG_ERROR);
        return NULL;
    }
   CStar   *nearest = nil;
   long tech = inFrom->GetTechLevel();
   long me = inFrom->GetOwner();
   AGalacticThingy   *aNeighbor;
   Point3D fromCoord = inFrom->GetCoordinates();
   LArray* everything = GetEverythingList();
//   int numItems = everything->GetCount();
   // v1.2b11d4, only check stars
   int last = host->GetLastStarIndexInEverythingList();
   for (int i = 1; i <= last; i++) {
      everything->FetchItemAt(i, &aNeighbor);
      if (!aNeighbor->IsAllyOf(me)) {
         continue;   // it's an enemy
      }
      if (aNeighbor->GetTechLevel() >= tech) {
         continue;   // tech level is not lower
      }
      if (aNeighbor->IsDead()) {
         continue;   // skip this one, it's dead
      }
      long type = aNeighbor->GetThingySubClassType();
      if (type != thingyType_Star) {
          DEBUG_OUT("Illegal "<< aNeighbor <<" in star section of everything list", DEBUG_ERROR);
         continue;   // not a star
      }
      long dist = aNeighbor->Distance(fromCoord);
      if (dist <= maxDist) {
         nearest = static_cast<CStar*>(aNeighbor);
         maxDist = dist;
      }
   }
   return nearest;
}

CStar*
GalacticaShared::FindWeakestEnemyStar(AGalacticThingy *inFrom, long maxDist) {
    GalacticaProcessor* host = dynamic_cast<GalacticaProcessor*>(this);
    if (!host) {
        DEBUG_OUT("CStarMapView::FindWeakestEnemyStar() called from client", DEBUG_ERROR);
         return NULL;
   }
   CStar   *weakest = nil;
   long currStrength = 0x7fffffff;
   long me = inFrom->GetOwner();
   AGalacticThingy   *aNeighbor;
   Point3D fromCoord = inFrom->GetCoordinates();
   LArray* everything = GetEverythingList();
//   int numItems = everything->GetCount();
   // v1.2b11d4, only check stars
   int last = host->GetLastStarIndexInEverythingList();
   for (int i = 1; i <= last; i++) {
      everything->FetchItemAt(i, &aNeighbor);
      if (!aNeighbor->IsOwned() || aNeighbor->IsAllyOf(me)) {
         continue;   // it's a friendly or unowned
      }
      if (aNeighbor->IsDead()) {
         continue;   // skip this one, it's dead
      }
#warning Uncomment this to keep AI from cheating in Fog of War games
//      if (HasFogOfWar() && !aNeighbor->IsVisibleTo(me)) {
//         continue;   // skip this one, we can't see it
//      }
      long type = aNeighbor->GetThingySubClassType();
      if (type != thingyType_Star) {
          DEBUG_OUT("Illegal "<< aNeighbor <<" in star section of everything list", DEBUG_ERROR);
         continue;   // not a star
      }
      long dist = aNeighbor->Distance(fromCoord);
      if (dist <= maxDist) {
         CStar* aStar = static_cast<CStar*>(aNeighbor);
         long strength = aStar->GetTotalDefenseStrength();
         if (strength < currStrength) {
            currStrength = strength;
            weakest = aStar;
         }
      }
   }
   return weakest;
}


CStar*
GalacticaShared::FindNearestEnemyTechProductionStar(AGalacticThingy *inFrom, long maxDist) {
    GalacticaProcessor* host = dynamic_cast<GalacticaProcessor*>(this);
    if (!host) {
        DEBUG_OUT("CStarMapView::FindNearestEnemyTechProductionStar() called from client", DEBUG_ERROR);
        return NULL;
    }
   CStar   *closest = nil;
   long currDist = maxDist;
   long me = inFrom->GetOwner();
   AGalacticThingy   *aNeighbor;
   Point3D fromCoord = inFrom->GetCoordinates();
   LArray* everything = GetEverythingList();
//   int numItems = everything->GetCount();
   // v1.2b11d4, only check stars
   int last = host->GetLastStarIndexInEverythingList();
   for (int i = 1; i <= last; i++) {
      everything->FetchItemAt(i, &aNeighbor);
      if (!aNeighbor->IsOwned() || aNeighbor->IsAllyOf(me)) {
         continue;   // it's a friendly or unowned
      }
      if (aNeighbor->IsDead()) {
         continue;   // skip this one, it's dead
      }
      long type = aNeighbor->GetThingySubClassType();
      if (type != thingyType_Star) {
         continue;   // not a star
      }
#warning Uncomment this to keep AI from cheating in Fog of War games
//      if (HasFogOfWar() && !aNeighbor->IsVisibleTo(me)) {
//         continue;   // skip this one, we can't see it
//      }
      long dist = aNeighbor->Distance(fromCoord);
      if ( (dist <= maxDist) && (dist < currDist)) {
         // are they spending more than twice as much for tech as they are for
         // shipbuilding?, if so, this is considered at tech planet
         CStar* aStar = static_cast<CStar*>(aNeighbor);
         if ((aStar->GetSpending(spending_Ships)*2) < aStar->GetSpending(spending_Research)) {
            currDist = dist;
            closest = aStar;
         }
      }
   }
   return closest;
}

// Found[0] = unowned, Found[1] = ours, Found[2] = enemy
void
GalacticaShared::FindClosestStars(AGalacticThingy *inFrom, CStar* outFound[3], long maxDist, long minDist[3]) {
    DEBUG_OUT("CStarMapView::FindClosestStars to "<<inFrom, DEBUG_TRIVIA);
    GalacticaProcessor* host = dynamic_cast<GalacticaProcessor*>(this);
    if (!host) {
        DEBUG_OUT("CStarMapView::FindClosestStars() called from client", DEBUG_ERROR);
        return;
    }
   int i;
   long currDist[3];
   Point3D fromCoord = inFrom->GetCoordinates();
   for (i = 0; i<3; i++) {
      outFound[i] = nil;
   }
   for (i = 0; i<3; i++) {
      currDist[i] = maxDist;
   }
   AGalacticThingy* aNeighbor;
   LArray* everything = GetEverythingList();
//   int numItems = everything->GetCount();
   // v1.2b11d4, only check stars
   int last = host->GetLastStarIndexInEverythingList();
   for (int i = 1; i <= last; i++) {
      everything->FetchItemAt(i, &aNeighbor);
      if (aNeighbor == inFrom) {  // v2.0.9, don't include thing we are searching from
          continue;
      }
      long type = aNeighbor->GetThingySubClassType();
      if (type == thingyType_Star) {
         EWaypointType where = aNeighbor->GetPosition();
         if ((where == wp_Coordinates) && !aNeighbor->IsDead()) {
            int which = aNeighbor->GetOwner();   // determine which kind of star we are looking
#warning Uncomment this to keep AI from cheating in Fog of War games
//      if (HasFogOfWar() && !aNeighbor->IsVisibleTo(me)) {
//         which = 0;   // we can't see who owns it
//      }
            if (which != 0) {               // at, unowned, ours or an enemy
               if (which == inFrom->GetOwner()) {   // are we the owner?
                  which = 1;   // ours
               } else {
                  which = 2;   // enemy
               }
            }
            long dist = aNeighbor->Distance(fromCoord);
            if (minDist == NULL) {
               if ((dist >= 0) && (dist < currDist[which])) {
                  currDist[which] = dist;
                  outFound[which] = static_cast<CStar*>(aNeighbor);
               } 
            } else {
               if ((dist >= minDist[which]) && (dist < currDist[which])) {
                  currDist[which] = dist;
                  outFound[which] = static_cast<CStar*>(aNeighbor);
               } 
            }
         }
      }
   }
   if (minDist != NULL) {
      for (i = 0; i<3; i++) {
         minDist[i] = currDist[i];
      }
   }
}

bool rangeSortCriterion(const AGalacticThingy* x, const AGalacticThingy* y);
bool rangeSortCriterion(const AGalacticThingy* x, const AGalacticThingy* y) {
    return (x->GetScannerRange() > y->GetScannerRange());
};

std::vector<AGalacticThingy*>*
GalacticaShared::GetScannersForPlayer(int inPlayerNum) {


    // returns a list of all the scanners for a given player, in
    // sorted order by range, highest to lowest.
    // the list is only generated if there isn't one already,
    // so the list must be cleared and deleted when something 
    // (such as an end of turn) invalidates it. This can be done
    // by calling GalacticaShared::ClearAllScannerLists()
    if ((inPlayerNum > MAX_PLAYERS_CONNECTED) || (inPlayerNum < 1)) {
        return 0;
    }
    std::vector<AGalacticThingy*>* theList = mScannersList[inPlayerNum-1];
    if (!theList) {
        theList = new std::vector<AGalacticThingy*>;
        mScannersList[inPlayerNum-1] = theList;
        // populate the list with everything belonging to player that has scanners
        AGalacticThingy* aThingy;
        LArray* everything = GetEverythingList();
        int numItems = everything->GetCount();
        for (int i = 1; i <= numItems; i++) {
            everything->FetchItemAt(i, &aThingy);
            // allies share their scanner info
            if ((aThingy->IsAllyOf(inPlayerNum)) && aThingy->HasScanners()) {
                theList->push_back(aThingy);
            }
        }
        // sort the list by scanner range
        std::sort(theList->begin(), theList->end(), rangeSortCriterion);
    }
    return theList;
}


void
GalacticaShared::ClearAllScannerLists() {
    for (int i = 0; i<MAX_PLAYERS_CONNECTED; i++) {
        if (mScannersList[i]) {
            mScannersList[i]->clear();
            delete mScannersList[i];
        }
	    mScannersList[i] = 0;
    }
}


// do anything necessary when game is loaded
void
GalacticaShared::FinishGameLoad() {
	DEBUG_OUT("GalacticaShared::FinishGameLoad", DEBUG_IMPORTANT);
	// ==============================
    // go though the list of all things and calculate what they can see
	// ==============================
	LArrayIterator iterator(mEverything, LArrayIterator::from_Start); // Search list of all thingys
	AGalacticThingy* aThingy;
	while (iterator.Next(&aThingy)) {
		if (!ValidateThingy(aThingy)) {
		    DEBUG_OUT("FinishGameLoad Found Invalid Thingy "<<aThingy<<" in Host everything list, removing...", DEBUG_ERROR | DEBUG_EOT);
		    mEverything.Remove(aThingy);
		    continue; // do nothing more with this invalid item lest we crash
		} else {
		    DEBUG_OUT("Calculating visibility for "<<aThingy, DEBUG_TRIVIA | DEBUG_EOT);		    
		    aThingy->CalculateVisibilityToAllOpponents();
		}
	}
}

void
GalacticaShared::LogGameInfo() {   
  #ifdef DEBUG
   int len = mGameInfo.gameTitle[0];
   mGameInfo.gameTitle[len+1] = '\0';
   char stateStr[20];
   char flagStr[80];
   flagStr[0] = '\0';
   switch (mGameInfo.gameState & 0x7) {
      case state_NormalPlay:
          if (mGameInfo.gameState & state_FullFlag) {
              std::strcpy(stateStr, "Normal Full");
          } else {
             std::strcpy(stateStr, "Normal Open");
         }
         break;
      case state_GameOver:
         std::strcpy(stateStr, "GameOver");
         break;
      case state_PostGame:
         std::strcpy(stateStr, "PostGame");
         break;
      default:
         std::strcpy(stateStr, "Unknown");
         break;
   }
   if ( IsHost() ) {
      std::strcat(flagStr, "Host ");
   }
   if (mGameInfo.omniscient) {
      std::strcat(flagStr, "Omniscient ");
   }
   if (mGameInfo.fastGame) {
      std::strcat(flagStr, "Fast ");
   }
   if (mGameInfo.endTurnEarly) {
       std::strcat(flagStr, "EndEarly ");
   }
   DEBUG_OUT("GameInfo "<<(char*)&(mGameInfo.gameTitle[1])
             <<" T#:"<<mGameInfo.currTurn
             <<" TL:"<<mGameInfo.maxTimePerTurn
             <<" TE:"<<mGameInfo.nextTurnEndsAt
             <<" P:"<<mGameInfo.totalNumPlayers
             <<" PR:"<<mGameInfo.numPlayersRemaining
             <<" H:"<<(long)mGameInfo.numHumans
             <<" C:"<<(long)mGameInfo.numComputers
             <<" HR:"<<(long)mGameInfo.numHumansRemaining
             <<" CR:"<<(long)mGameInfo.numComputersRemaining
             <<" "<<stateStr
             <<" CS:"<<mGameInfo.compSkill
             <<" CA:"<<mGameInfo.compAdvantage
             <<" D:"<<mGameInfo.sectorDensity
             <<" S:"<<mGameInfo.sectorSize
             <<" S#:"<<mGameInfo.sequenceNumber
             <<" "<<flagStr
             <<" TZ:"<<(long)mGameInfo.timeZone
             <<" ID:"<<(long)mGameInfo.gameID
      , DEBUG_IMPORTANT | DEBUG_EOT | DEBUG_PACKETS);
  #endif
}

void
GalacticaShared::NotifyBusy(long , long ) {
    // this is a UI method, but is needed here because many classes that aren't UI classes
    // need to provide feedback to the user that they are in the middle of a time consuming operation.
    // in a non-GUI situation, this will do nothing.
    // in a GUI situation, this will be extended by something that puts up a busy cursor or a progress
    // bar
}

