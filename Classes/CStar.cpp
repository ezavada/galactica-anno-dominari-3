// CStar.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ

#include "CStar.h"
#include "CShip.h"
#include "CMessage.h"
#include "CFleet.h"
#include "GalacticaShared.h"
#include "GalacticaHost.h"

#ifndef GALACTICA_SERVER
#include "GalacticaPanes.h"
#include "GalacticaPanels.h"
#include "Galactica.h"
#include "CHelpAttach.h"
#include "CNewGWorld.h"
#include "AGalacticThingy.h"
#include "GalacticaTutorial.h"
#include "GalacticaDoc.h"
#include "GalacticaProcessor.h"
#include "GalacticaHostDoc.h"

#include <UDrawingUtils.h>
#endif // GALACTICA_SERVER

#include <stdio.h>
#include <cmath>

// lowest brightness of stars
#define LOWEST_STAR_BRIGHTNESS 0.1f

int CStar::sMaxShipNames = 0;

// **** STARS ****

CStar::CStar(LView *inSuperView, GalacticaShared* inGame)
: ANamedThingy(inSuperView, inGame, thingyType_Star),
  mShipRefs(kAllowShipsOnly) {
   // v2.1b11, get max ship names from resource
   if (sMaxShipNames == 0) {
       sMaxShipNames = GetNumSubStringsInStringResource(STRx_ShipNames);
   }
	mNumStarLanes = 0;
	mCostOfNextShip = 1;
	mPaidTowardNextTech = 0;
	mBuildHullType = -1; //class_Satellite;
	mPatrolStrength = 0;
	SetPopulation(0);	// no initial population
//	mOwnedBy = Rnd(6);
	int i;
	for (i = 0; i < MAX_STAR_LANES; i++) {
		mStarLanes[i] = nil;
	}
	for (i = 0; i < NUM_SHIP_HULL_TYPES; i++) {
		mPaidTowardNextShip[i] = 0.0;
	}
	// еее temp code to initialize settings for a star
	short valuePer = 1000 / MAX_STAR_SETTINGS;
	for (i = 0; i < MAX_STAR_SETTINGS; i++) {
		mSettings[i].desired = valuePer;
		mSettings[i].value = valuePer;
		mSettings[i].benefit = 6;//4 + Rnd(3);
		mSettings[i].economy = 0;	// don't worry about economy yet
		mSettings[i].rate = 60;	// 1.2fc1
//		mSettings[i].rate = 48 + Rnd(17);
		mSettings[i].locked = false;
	}
	int leftOver = 1000 - (valuePer * MAX_STAR_SETTINGS);
	mSettings[0].desired += leftOver;
	mSettings[0].value += leftOver;
  #if BALLOON_SUPPORT
	if (mGame && mGame->IsSinglePlayer()) {
	    CThingyHelpAttach* theBalloonHelp = new CThingyHelpAttach(this);
    	AddAttachment(theBalloonHelp);
	}
  #endif
    // stuff for fog of war
    mStealth = 1;
    mApparentSize = 100L; // match everything else;
    UInt32 scanRange = UThingyClassInfo::GetScanRangeForTechLevel(1, (mGame && mGame->IsFastGame()), (mGame && mGame->HasFogOfWar()));
    SetScannerRange(scanRange);
}

/*
CStar::CStar(LView *inSuperView, GalacticaShared* inGame, LStream *inStream)
: ANamedThingy(inSuperView, inGame, inStream, thingyType_Star),
  mShipRefs(kAllowShipsOnly) {
	ReadStarStream(*inStream);
  #if BALLOON_SUPPORT
	if (mGame && mGame->IsSinglePlayer()) {
    	CThingyHelpAttach* theBalloonHelp = new CThingyHelpAttach(this);
    	AddAttachment(theBalloonHelp);
	}
  #endif
}
*/

void
CStar::FinishInitSelf() {
	AGalacticThingy::FinishInitSelf();
	std::string s;
	LoadStringResource(s, STRx_StarNames, 1);	// Assign the name to the star
	char ns[256];
	std::sprintf(ns, "%s%ld", s.c_str(), mID);
	SetName(ns);
	DEBUG_OUT(this << " finished initing", DEBUG_DETAIL);
  #if TUTORIAL_SUPPORT
	if (Tutorial::TutorialIsActive()) {
		// is this star number 2, 3 or 4?
		if ((mID > 2) && (mID < 5)) {
			SetTechLevel(12);
			SetScannerRange(UThingyClassInfo::GetScanRangeForTechLevel(12, mGame->IsFastGame(), mGame->HasFogOfWar()));
			ScannerChanged();
			SetBuildHullType(class_Fighter);
			SetPercentPaidTowardNextShip(0.99);
			
		}
	}
  #endif //TUTORIAL_SUPPORT
//	SetDescriptor(mName);
}


void
CStar::ReadStream(LStream *inStream, UInt32 streamVersion) {
	ANamedThingy::ReadStream(inStream, streamVersion);
	ReadStarStream(*inStream);
	if (mApparentSize == 200) {
	    mApparentSize = 100;
	}
}

void
CStar::WriteStream(LStream *inStream, SInt8 forPlayerNum) {
	ANamedThingy::WriteStream(inStream);
	UInt8 numLanes = mNumStarLanes;
	*inStream << numLanes;
	ASSERT(numLanes == 0);
// this code removed until starlanes are put back into the game
//	for (int i = 0; i < numLanes; i++) {
//	    PaneIDT id;
//		id = mStarLanes[i]->GetPaneID();	// write out pane ID of each starlane
//		*inStream << id;
//	}
	for (int i = 0; i < NUM_SHIP_HULL_TYPES; i++) {	// write ship construction data
		*inStream << mPaidTowardNextShip[i];
	}
	mShipRefs.WriteStream(*inStream);
	*inStream << mCostOfNextShip << mBuildHullType << mPaidTowardNextTech << mPopulation
		<< mPatrolStrength << mNewShipDestination;
	for (int i = 0; i < MAX_STAR_SETTINGS; i++) {
		*inStream << mSettings[i];		// write out the current settings for the star
	}
}

#ifndef GALACTICA_SERVER
void
CStar::UpdateClientFromStream(LStream *inStream) {
    bool restore = false;
    GalacticaDoc* doc = GetGameDoc();
    if (doc) {
	    restore = ( (mOwnedBy == doc->GetMyPlayerNum()) && mChanged );
	} else {
	    DEBUG_OUT("called UpdateClientFromStream for "<<this<<" with no GalacticaDoc", DEBUG_ERROR);
	}
	int originalOwner = mOwnedBy;
	ANamedThingy::UpdateClientFromStream(inStream);
	Waypoint wp = mNewShipDestination;	// save and restore the destination for new ships
	if (originalOwner != mOwnedBy) {
	    wp = gNullPt3D;
	}
	SInt16 hullType = mBuildHullType;
	ReadStarStream(*inStream, restore);
	if (restore) {
		for (int i = 0; i < MAX_STAR_SETTINGS; i++) {
			mSettings[i].UpdateClientFromStream(inStream);
		}
		mBuildHullType = hullType;
    	mNewShipDestination = wp;
	}
}
#endif // GALACTICA_SERVER

void
CStar::UpdateHostFromStream(LStream *inStream) {
// this method is used to update the host from a packet sent by a client
// the client is not allowed to update most of the data, so many fields are
// ignored
	ANamedThingy::UpdateHostFromStream(inStream);
	ReadStarStream(*inStream, true);
	// make sure the user hasn't managed to set the total for desired
	// above the 1000 total (100.0%) since the host would then blindly
	// alter all the settings.
	short desiredTotal = 0;
	for (int i = 0; i < MAX_STAR_SETTINGS; i++) {
		mSettings[i].UpdateHostFromStream(inStream);
		if (mSettings[i].desired < 0) {
			mSettings[i].desired = 0;
		} else if (mSettings[i].desired > 1000) {
			mSettings[i].desired = 1000;
		}
		desiredTotal += mSettings[i].desired;
	}
	// don't worry about locks, this is to fix cheating
	// sometimes happens during normal gameplay though
	if (desiredTotal > 1000) {
		DEBUG_OUT("WARNING: Illegal star settings, redistributing "<<desiredTotal<<" points at "<<this, DEBUG_IMPORTANT);
		short distribute = desiredTotal - 1000;
		int j = 0;
		while ( (distribute > 0) && (j < MAX_STAR_SETTINGS) ) {
			if (distribute > mSettings[j].desired) {
				distribute -= mSettings[j].desired;
				mSettings[j].desired = 0;
			} else {
				mSettings[j].desired -= distribute;
				distribute = 0;
			}
			++j;
		}
	}
}

void
CStar::ReadStarStream(LStream &inStream, bool inUpdating) {
	UInt8 numLanes;
	bool updatingHost = inUpdating && mGame->IsHost();
	inStream >> numLanes;
	ASSERT(numLanes == 0);
	ASSERT(mNumStarLanes == 0);
	ThrowIf_(numLanes != 0);
// this code removed until starlanes are put back into the game
//	PaneIDT id;
//	mNumStarLanes = numLanes;
//	for (int i = 0; i < MAX_STAR_LANES; i++) {
//		if (i < numLanes) {
//			inStream >> id;
//			mStarLanes[i] = (CStarLane*)id;	// temporary assignment of id until we can
//		} else {							// find by id in FinishCreateSelf()
//			mStarLanes[i] = nil;
//		}
//	}
   float ignoreFloat;
   for (int i = 0; i < NUM_SHIP_HULL_TYPES; i++) {	// read ship construction data
      if (updatingHost) {
         inStream >> ignoreFloat;  // ignore ship construction data if updating host
      } else {
         inStream >> mPaidTowardNextShip[i];
      }
   }
   if (updatingHost) {
      // ignore host controlled items if updating host
        DEBUG_OUT("client->host update for "<< this<< ", ignoring ships list", DEBUG_CONTAINMENT | DEBUG_MOVEMENT | DEBUG_DETAIL);
        CThingyList ignoreRefs(kAllowShipsOnly);
        ignoreRefs.ReadStream(inStream);
        SInt16 ignore16; 
        UInt32 ignore32;
        SInt16 hullType;
        inStream >> ignoreFloat >> hullType >> ignore16 >> ignore32
                 >> ignore32 >> mNewShipDestination;
        if (mBuildHullType != hullType) {
         // they changed the hull type, so recompute
            SetBuildHullType(hullType);
        }
   } else {
  #warning TODO: do not read ship refs, rebuild them as ships are read
        mShipRefs.ReadStream(inStream);
        inStream >> mCostOfNextShip >> mBuildHullType >> mPaidTowardNextTech >> mPopulation
                 >> mPatrolStrength >> mNewShipDestination;
   }
   if (!inUpdating) {	// if we are not updating, allow UpdateXFromStream() to read in the settings
      for (int i = 0; i < MAX_STAR_SETTINGS; i++) {
         inStream >> mSettings[i];
      }
   }
}

void
CStar::DoComputerMove(long inTurnNum) {	// only called from host doc
    GalacticaProcessor* host = dynamic_cast<GalacticaProcessor*>(mGame);
	if (!host || mNeedsDeletion) {	// don't do anything if star destroyed
		return;
	}
	int compSkill = mGame->GetComputerSkill();
	if (compSkill <= compSkill_Expert) {
		PrivatePlayerInfoT pInfo;
		if (IsTechStar()) {	// is this already a designated tech planet?
			// make sure we are producing couriers
			// v1.2fc1, have tech stars produce both couriers and satellites
			// code to do this is in CStar::EndOfTurn()
/*			if (GetBuildHullType() != class_Scout) {
				DEBUG_OUT(this<<" is a tech star, told to make couriers.", DEBUG_DETAIL | DEBUG_AI);
				SetBuildHullType(class_Scout);
				Changed();
			} */
			// make sure that if we lost our main tech star, this one becomes the new main tech star
			host->GetPrivatePlayerInfo(mOwnedBy, pInfo);
			if (!pInfo.hasTechStar) {
				// make sure this player is recorded as having a tech star
				pInfo.hasTechStar = true;
				pInfo.techStarID = mID;
				host->SetPrivatePlayerInfo(mOwnedBy, pInfo);
			}
			return;
		} else if ((inTurnNum > 100) || ((inTurnNum % 6) == 0)) {	// every sixth turn, pick a new tech planet
			// v1.2fc1, check every turn after turn 100 (when loss of home planet more likely)
			host->GetPrivatePlayerInfo(mOwnedBy, pInfo);
			// make this a tech planet, if:
			// 1. It is has a tech level equal to the highest tech level
			if (GetTechLevel() < pInfo.hiStarTech) {
				DEBUG_OUT(this<<" rejected as tech star, tech "<< GetTechLevel() <<" not >= "<< pInfo.hiStarTech, DEBUG_DETAIL | DEBUG_AI);
				return;
			}
			CStar* found[3];
			mGame->FindClosestStars(this, found, 222222L);
			CStar *nearestOwnedStar = found[1];	// nearest star that belongs to us
			CStar *nearestEnemyStar = found[2];
			// 2. it doesn't have a nearby enemy planet
			if (nearestEnemyStar != NULL) {
				DEBUG_OUT(this<<" rejected as tech star, too close to enemy "<<nearestEnemyStar, DEBUG_DETAIL | DEBUG_AI);
				return;
			}
			// 3. It is close to another allied star
			if (nearestOwnedStar == NULL) {
				DEBUG_OUT(this<<" rejected as tech star, no nearby ally ", DEBUG_DETAIL | DEBUG_AI);
				return;
			}
			// 4. The nearby ally star is not an tech star
			if (nearestOwnedStar->GetSpending(spending_Research) > 400) {
				DEBUG_OUT(this<<" rejected as tech star, next to ally tech star "<<nearestOwnedStar, DEBUG_DETAIL | DEBUG_AI);
				return;
			}
			// passed all those tests, make it a tech star
			mSettings[spending_Growth].desired = 494;
			mSettings[spending_Ships].desired = 12;	// just enough to build a courier every 7 turns or so
			mSettings[spending_Research].desired = 494;
			DEBUG_OUT("Turn: "<< inTurnNum <<", made "<<this<<" into a tech star.", DEBUG_DETAIL | DEBUG_AI);
			Changed();
			if (!pInfo.hasTechStar) {
				// make sure this player is recorded as having a tech star
				pInfo.hasTechStar = true;
				pInfo.techStarID = mID;
				host->SetPrivatePlayerInfo(mOwnedBy, pInfo);
			}
		} else {	// not a tech star
			if ( (mSettings[spending_Growth].desired != 500) || (mSettings[spending_Ships].desired != 500)) {
				host->GetPrivatePlayerInfo(mOwnedBy, pInfo);
				if (pInfo.hasTechStar) {	// don't make more shipbuilding stars unless we have a tech star
					// make a shipbuilding star
					mSettings[spending_Growth].desired = 500;
					mSettings[spending_Ships].desired = 500;
					mSettings[spending_Research].desired = 0;
					DEBUG_OUT("Turn: "<< inTurnNum <<", made "<<this<<" into a shipbuilding star.", DEBUG_DETAIL | DEBUG_AI);
					Changed();
				}
			}
		}
	} else if ( (mSettings[spending_Growth].desired != 334) || (mSettings[spending_Ships].desired != 333)) {
		// passed all those tests, make it a tech star
		mSettings[spending_Growth].desired = 334;
		mSettings[spending_Ships].desired = 333;
		mSettings[spending_Research].desired = 333;
		DEBUG_OUT("Turn: "<< inTurnNum <<", reset "<<this<<" to default product levels", DEBUG_DETAIL | DEBUG_AI);
		Changed();
	}
}


bool	
CStar::IsOfContentType(EContentTypes inType) const {
	if ( (inType == contents_Stars) || (inType == contents_Any)) {
		return true;
	} else {
		return false;
	}
}

bool
CStar::SupportsContentsOfType(EContentTypes inType) const {
	if (inType == contents_Ships) {
		return true;
	} else {
		return false;
	}
}

void*
CStar::GetContentsListForType(EContentTypes inType) const {
	if (inType == contents_Ships) {
//		return &((CStar*)this)->mShips; 	// typecast away const
		return (void*) &(const_cast<CStar*>(this))->mShipRefs;
	} else {
		return nil;
	}
}

SInt32
CStar::GetContentsCountForType(EContentTypes inType) const {
	if (inType == contents_Course) {
		return mShipRefs.GetCount();
//		return mShips.GetCount();
	} else {
		return 0;
	}
}

bool
CStar::AddContent(AGalacticThingy* inThingy, EContentTypes inType, bool inRefresh) {
	if ((inType == contents_Any) || (inType == contents_Ships) || (inType == contents_SatelliteFleet)) {
		inThingy = ValidateShip(inThingy);
		ASSERT(inThingy != nil);
		if (inThingy) {
			DEBUG_OUT("Start adding "<<inThingy<<" to "<<this, DEBUG_TRIVIA | DEBUG_CONTAINMENT);
			AGalacticThingy* at = inThingy->GetPosition();	// remove ship from previous container
			if (at) {
				at->RemoveContent(inThingy, contents_Ships, inRefresh);
			}
			inThingy->SetPosition(this, inRefresh);	// do refresh
			ThingyRef aRef = inThingy;
			if (inType == contents_SatelliteFleet) {
				mShipRefs.InsertItemsAt(1, LArray::index_First, aRef);	// add satellite fleets to top of list
			} else {
				mShipRefs.InsertItemsAt(1, LArray::index_Last, aRef);
			}
			RecalcPatrolStrength();
			Changed();
		  #ifndef GALACTICA_SERVER
			if (inRefresh) {
				Refresh();
			}
		  #endif // GALACTICA_SERVER
			DEBUG_OUT("Done adding "<<inThingy<<" to "<<this, DEBUG_TRIVIA | DEBUG_CONTAINMENT);
		}
	}
	return true;
}

void
CStar::RemoveContent(AGalacticThingy* inThingy, EContentTypes inType, bool inRefresh) {
	if ((inType == contents_Any) || (inType == contents_Ships)) {
		ThingyRef aRef = inThingy;
		if (mShipRefs.ContainsItem(aRef)) {
    		DEBUG_OUT("Start removing "<<inThingy<<" from "<<this, DEBUG_TRIVIA | DEBUG_CONTAINMENT);
    		mShipRefs.Remove(aRef);
    		RecalcPatrolStrength();
    		Changed();
		  #ifndef GALACTICA_SERVER
			if (inRefresh) {
				Refresh();
			}
		  #endif // GALACTICA_SERVER
    		DEBUG_OUT("Done removing "<<inThingy<<" from "<<this, DEBUG_TRIVIA | DEBUG_CONTAINMENT);
		}
	}
//	ANamedThingy::RemoveContent(inThingy, inType);
}

//	#ifdef DEBUG		// at this point nothing else should contain inThingy
//		UHasContainerAction checkContainers(inThingy);
//		mGame->DoForEverything(checkContainers);
//		if (checkContainers.HasContainer()) {
//			DEBUG_OUT("Removed " << inThingy<<" from "<<this<<" but other items still contain it.", DEBUG_ERROR);
//		}
//	#endif


bool
CStar::HasContent(AGalacticThingy* inThingy, EContentTypes inType) {
	if ((inType == contents_Any) || (inType == contents_Ships)) {
		ThingyRef aRef = inThingy;
		return mShipRefs.ContainsItem(aRef);
	}
	return false;	// default assumes we have no contents
}

long
CStar::GetProduction() const {
	long multiplier = 100; 
	if (mGame->PlayerIsComputer(mOwnedBy)) {
		multiplier += mGame->GetComputerAdvantage() * 20;
	}
	return ((long)(mSettings[spending_Ships].benefit + mSettings[spending_Research].benefit)) * multiplier;
}


// star attack strength is a factor of tech level and of population, so planets with
// larger populations are harder to take over. The function is (log x)^2, so the benefits
// of larger population decrease as population increases.
// Assuming a level 1 ship has an attack strength of 50, and a level 10 ship an attack strength
// of 500, the population defense curve is as follows:

// pop 10 thousand  = 30 -- a bit weaker than a single ship level one ship
// pop 100 thousand = 75 -- 1 & 1/2 ship strength
// pop 1 million   = 130 -- a bit weaker than 3 ships 
// pop 10 million  = 195 -- on par with 4 ships
// pop 100 million = 270 -- a bit stronger than 5 ships
// pop 1 billion   = 355 -- on par with 7 ships
// pop 10 billion  = 450 -- on par with 9 ships

long
CStar::GetAttackStrength() const {
   float factor;
   if (mPopulation > 10) {
      factor = std::log10((float)mPopulation * (float)1000);
   } else {
      factor = 4;
   }
   long strength = (factor * factor * 5) - 50;
   long result = ((long)mTechLevel * strength);
   DEBUG_OUT("Attack Strength for "<< this<<" (pop "<<mPopulation<<"k, tech "
            <<(long)mTechLevel<<") = "<< result, DEBUG_TRIVIA);
   return result;
}

long
CStar::GetDefenseStrength() const {
	long pop = GetPopulation()/10;
	if (pop < 1) {
	    return 0;
	} else {
	    return GetAttackStrength();
	}
}

long
CStar::GetDangerLevel() const {
	return 0;	// default returns no danger level
}

long
CStar::GetSpending(ESpending inWhichItem) const {// 
	return mSettings[inWhichItem].desired;	// this returns the user setting, "value" would return
}											// the current setting

float
CStar::GetBenefits(ESpending inWhichItem) const {// 
	return mSettings[inWhichItem].GetBenefits();
}

int
CStar::GetEstimatedTurnsToNextShip() const {
	float shipEffort = mSettings[spending_Ships].GetProjectedBenefits() / (float)16650;
	if (shipEffort > 0) {
		if (mGame->IsFastGame()) {			// ships built faster in fast game, v 1.2d2
			shipEffort *= 2;
		}
		if (mGame->PlayerIsComputer(mOwnedBy)) {
			IncreaseByPercent(shipEffort, mGame->GetComputerAdvantage()*20);
		}
		float effortRemaining = mCostOfNextShip - mPaidTowardNextShip[mBuildHullType];
		if (effortRemaining < 0) {
			DEBUG_OUT("Calculated "<<effortRemaining<<" effort remaining to build ship type "<<mBuildHullType, DEBUG_ERROR);
			return 1;
		} else {
			return ((int)(effortRemaining/shipEffort)) + 1;
		}
	} else {
		return -1;
	}
}

#warning CStar::GetName is not thread safe, must pass in buffer
const char* 
CStar::GetName(bool considerVisibility) {
    GalacticaDoc* game = GetGameDoc();
  #ifndef GALACTICA_SERVER
    if (considerVisibility && game) {
        int myPlayerNum = game->GetMyPlayerNum();
        if (!IsAllyOf(myPlayerNum) && (GetVisibilityTo(myPlayerNum) < visibility_Class)) {
    	    std::string s;
    	    LoadStringResource(s, STRx_StarNames, 1);	// Assign the name to the star
    	    static char ns[256];
    	    std::sprintf(ns, "%s%ld", s.c_str(), mID);
            return ns;  // Fog of War
        }
    }
  #endif // GALACTICA_SERVER
	return mName.c_str();
}

const char* 
CStar::GetShortName(bool considerVisibility) {
    return GetName(considerVisibility);
}

void
CStar::EndOfTurn(long inTurnNum) {
	int i;
	mPatrolStrength = 0;
    GalacticaProcessor* host = dynamic_cast<GalacticaProcessor*>(mGame);
	ASSERT_STR(mNeedsDeletion == false, "mNeedsDeletion flag set for "<<this<<". Shouldn't ever happen");
    ASSERT_STR(host, "EOT called for "<< this<<" from client");
	if (mOwnedBy && !mNeedsDeletion && host) {
		// ship repair: apply repairs to all ships that are not on patrol, based on tech level
		int numShips = mShipRefs.GetCount();
		int repairAmount = mTechLevel * 35;
		DEBUG_OUT(this<<" is offering "<<repairAmount<<" damage repair to all ships", DEBUG_DETAIL | DEBUG_COMBAT);
		if (numShips > 0) {
			CShip* aShip = NULL;
			for (i = 1; i <= numShips; i++) {
				aShip = static_cast<CShip*>(mShipRefs.FetchThingyAt(i));
				if (aShip == nil) {
					continue;
				}
				if (!aShip->IsOnPatrol() ) {
					if (!aShip->CoursePlotted() ) {	// v1.1.1, ERZ, 3/31/97, don't repair ships that
						long repaired = aShip->RepairDamage(repairAmount);	// are just passing through
						// v1.2b10, if we repaired the ship/fleet completely, send a message
						if ((repaired > 0) && (aShip->GetDamage() == 0) ) {
							if (mGame->PlayerIsHuman(mOwnedBy)) {	// but not to computer players
								if (aShip->GetThingySubClassType() == thingyType_Fleet) {
									CMessage::CreateEvent(GetGameHost(), event_FleetDamageFixed, mOwnedBy, aShip);
								} else {
									CMessage::CreateEvent(GetGameHost(), event_ShipDamageFixed, mOwnedBy, aShip);
								}
							}
							// v2.1d5, only repair until all repair work has been distributed
						    repairAmount -= repaired;
						    if (repairAmount <= 0) {
						        break;
						    }
						}
					}
				} else {
					mPatrolStrength += aShip->GetDefenseStrength();
				}
			}
		}
		// research
	  #warning TODO: should eventually include population in tech level growth calculations
		long paid = (long)(mSettings[spending_Research].GetBenefits() / (9 * mTechLevel));
		if (mGame->PlayerIsComputer(mOwnedBy)) {
			IncreaseByPercent(paid, mGame->GetComputerAdvantage()*20);
		}
		mPaidTowardNextTech += paid;
		long techNeeded;
		if (mGame->IsFastGame()) {			// ships built faster in fast game, v 1.2d2
			techNeeded = 750;
		} else {
			techNeeded = 1000;
		}
		if (mPaidTowardNextTech > techNeeded) {
			mPaidTowardNextTech -= techNeeded;
			mTechLevel++;
			if (host->PlayerHasNewStarClass(mOwnedBy, mTechLevel)) {	// send new stardrive message, v 1.2d2
				DEBUG_OUT("Player "<<mOwnedBy<<" has new drive tech " <<mTechLevel, 
							DEBUG_TRIVIA | DEBUG_MESSAGE);
				CMessage::CreateEvent(GetGameHost(), event_InventNewClass, mOwnedBy, this, kAllPlayers, 
										mOwnedBy, kNoOtherPlayers, mTechLevel, 
										Layr_LabBase + (mTechLevel/5));
			}
            // for fog of war
            UInt8 stealth = 1; // mGame->HasFogOfWar() ? ((mTechLevel * 3L / 4L) + 1L) : 1;
            SetStealth(stealth);
            UInt32 scanRange = UThingyClassInfo::GetScanRangeForTechLevel(mTechLevel, mGame->IsFastGame(), mGame->HasFogOfWar());
            /* use this if we want to use the 50% max scan for planets with no defense satellites 
         	CFleet* theSatelliteFleet = GetSatelliteFleet();
        	if ((theSatelliteFleet == NULL) || (theSatelliteFleet->GetContentsCountForType(contents_Ships) == 0)) {
        	    scanRange /= 2; // planet with no satellites has 50% scan range
        	} */
            SetScannerRange(scanRange);
            // changing the tech level doesn't always update the star's frame correctly
            // unless we call Calc Position
          #ifndef GALACTICA_SERVER
            if (GetGameDoc()) {
                CalcPosition(kRefresh);
            }
          #endif // GALACTICA_SERVER
            ScannerChanged();   // make sure we recheck what this thing can see
		}
		// shipbuilding
		float shipEffort = mSettings[spending_Ships].GetBenefits() / (float)16650;
		if (shipEffort > 0) {
			if (mGame->IsFastGame()) {			// ships built faster in fast game, v 1.2d2
				shipEffort *= 2;
			}
			if (mGame->PlayerIsComputer(mOwnedBy)) {
				IncreaseByPercent(shipEffort, mGame->GetComputerAdvantage()*20);
			}
			mPaidTowardNextShip[mBuildHullType] += shipEffort;
		  #if TUTORIAL_SUPPORT
		    if (Tutorial::TutorialIsActive()) {
		        // v2.1b9r11, check for processing turn 3 of the player's one star in a tutorial
		        // game, and make sure we always build the defense satellite.
		        // Do same for turn 8 building battleships, and turn 4 for the courier
	            bool ensureShipBuilt = false;
	            if (mID == tutorial_HomeStarID) {
	                if ( (mGame->GetTurnNum() == 3) || (mGame->GetTurnNum() == 4)
	                  ||(mGame->GetTurnNum() == 8) ) {
	                    ensureShipBuilt = true;
	                }
	            } else if (mID == tutorial_NewTerraStarID) {
	                if ( mGame->GetTurnNum() == 8 ) {
	                    ensureShipBuilt = true;
	                }
	            }
		        if (ensureShipBuilt) {
		            mPaidTowardNextShip[mBuildHullType] = mCostOfNextShip + 0.1;
		        }
		    }
		  #endif // TUTORIAL_SUPPORT
			while (mPaidTowardNextShip[mBuildHullType] > mCostOfNextShip) {
				mPaidTowardNextShip[mBuildHullType] -= mCostOfNextShip;
				BuildNewShip((EShipClass) mBuildHullType);
				// BuildNewShip() may have changed the ship type to build next
				SetBuildHullType(mBuildHullType);	// calc cost of next ship
			}
		}
		// growth
		for (i = 0; i < MAX_STAR_SETTINGS; i++) {
			mSettings[i].AdjustValue();
		}
		if (mSettings[spending_Growth].value > 0) {
			GrowPopulation();
			for (i = 1; i < MAX_STAR_SETTINGS; i++) {
				mSettings[i].benefit += mSettings[spending_Growth].GetBenefits() / 1665;
			}
		}
		for (i = 1; i < MAX_STAR_SETTINGS; i++) {
			if (mSettings[i].benefit < 6) {		// еее 1.2d8,  fix "irradiated" planet problem
				DEBUG_OUT("WARNING: "<<this<<" was Irradiated; benefit = "<<mSettings[i].benefit<<"; fixed", DEBUG_IMPORTANT);
				ASSERT_STR(mSettings[i].benefit>5, this<<" benefit = "<<mSettings[i].benefit);
				mSettings[i].benefit = 6;
			}
		}
		Changed();
	}
	AGalacticThingy::EndOfTurn(inTurnNum);
}

#ifdef DEBUG
extern int gDebugBreakOnErrors;
#endif

// this is only called during EOT processing.
void
CStar::AddToSatelliteFleet(AGalacticThingy* inThingy) {
	// create the satellite fleet if one is not already present, then
	// add the item. A satellite fleet can be identified because it's
	// name will be a single *.

	CFleet* theSatelliteFleet = GetSatelliteFleet();
	if (theSatelliteFleet == NULL) {
	    // no satellite fleet present, create it
	    Waypoint wp;
	    wp.Set(gNowherePt3D);
		inThingy->SetPosition(wp, kDontRefresh); // temporarily put this at "nowhere" to avoid errors in the debug log
		inThingy->Persist();	// must do this to keep the id numbers in sequence
	
		// no fleet found, we have to create one
		theSatelliteFleet = static_cast<CFleet*>(mGame->MakeThingy(thingyType_Fleet));
		theSatelliteFleet->AssignUniqueID();
		theSatelliteFleet->SetPosition(this, kDontRefresh);	// fleet is here, don't refresh
		theSatelliteFleet->SetOwner(this->GetOwner());
		theSatelliteFleet->BecomeSatelliteFleet();
		theSatelliteFleet->Persist();
		theSatelliteFleet->SetPatrol(true);
		AddContent(theSatelliteFleet, contents_SatelliteFleet, kDontRefresh);	// add the satellite fleet to the top of the star's list
	}
	theSatelliteFleet->AddContent(inThingy, contents_Ships, kDontRefresh);	// now add the satellite to the fleet
	theSatelliteFleet->SetOwner(this->GetOwner());	// just to make sure the fleet is always owned by the player
}

// will return NULL if none found
CFleet*
CStar::GetSatelliteFleet() {
	UFindSatelliteFleetAction findSatelliteFleet;
	mShipRefs.DoForEntireList(findSatelliteFleet);
	if (findSatelliteFleet.WasFound()) {
		return findSatelliteFleet.GetSatelliteFleet();
	} else {
	   return NULL;
	}
}

void
CStar::FinishEndOfTurn(long inTurnNum) {
#pragma unused (inTurnNum)
	if (!mNewShipDestination.IsNull()) {	// if we have a new ship destination that is
		AGalacticThingy* it = mNewShipDestination;	// pointed to another thingy, check that
		if (it) {									// the destination thingy isn't dead.
			if (it->IsDead()) {
				DEBUG_OUT("Found dead production target at "<<this<<" -- cleared", DEBUG_DETAIL);
				Waypoint wp;					// if it is dead, clear it
				mNewShipDestination = wp;		// set to null point
				Changed();
			}
		}
	}
}

//only call from host doc
void
CStar::BuildNewShip(EShipClass inHullType) {
    GalacticaProcessor* host = dynamic_cast<GalacticaProcessor*>(mGame);
    ASSERT_STR(host, "BuildNewShip called for "<< this<<" from client");
	CShip *newShip;
	bool notify = true;				// do we notify player of construction?
	bool canMove = true;				// can ship actually move?
	bool addToSatelliteFleet = false;	// does this belong in a special fleet?
	newShip = (CShip*)mGame->MakeThingy(thingyType_Ship); // new CShip(mSuperView, mGame);	// belongs in our starmap
	newShip->AssignUniqueID();			// this thing needs an ID right away
	newShip->SetOwner(mOwnedBy);		// its owner matches the owner of the star
	newShip->SetTechLevel(mTechLevel);	// and its tech level matches that of the star
// ship speed
	short speedIdx = mTechLevel/5;
	SInt32 power = mTechLevel;
	long speed = 10L + (speedIdx * 10L) + (mTechLevel%5);	// ship speed jump lots with class
	if (mGame->IsFastGame()) {		// ships move faster in fast game, v 1.2d2
		speed *= 2000L;
	} else {
		speed *= 1000L;
	}
	UInt32 apparentSize = 0;
	UInt32 percentRangeAdj = 10;    // 100%
// hull type adjustments
	DEBUG_OUT("Building "<<newShip<<" of hull type "<< (int)inHullType, DEBUG_TRIVIA | DEBUG_EOT);
	EShipClass classIdx = (EShipClass)inHullType;
	switch (classIdx) {
		case class_Satellite:
			speed = 0;
			newShip->SetPatrol(true);
			notify = false;
			canMove = false;
			addToSatelliteFleet = true;
			apparentSize = 100L; //200L + (5L * mTechLevel);
			percentRangeAdj = 0;    // no scanners, 0%
			DEBUG_OUT("Set satellite special data for "<< newShip, DEBUG_TRIVIA | DEBUG_EOT);
			break;
		case class_Scout:
			speed = (long)( (float)speed * 1.5 );
			power = 0;
			apparentSize = 100L; //50L - (mTechLevel/4L);
			percentRangeAdj = 8;    // 80%
			DEBUG_OUT("Set courier special data for "<< newShip, DEBUG_TRIVIA | DEBUG_EOT);
			break;
		case class_Colony:
			speed = (long)( (float)speed * 0.75 );
			apparentSize = 100L; //2000L + (20L * mTechLevel);
			percentRangeAdj = 0; // no scanners, was 8;    // 80%
			DEBUG_OUT("Set colony special data for "<< newShip, DEBUG_TRIVIA | DEBUG_EOT);
			break;
		case class_Fighter:
			classIdx = (EShipClass)(classIdx + mTechLevel/5);
			apparentSize = 100L; // + (100L * mTechLevel/5L);
			percentRangeAdj = 0; // no scanners, was 1;    // 10%
			DEBUG_OUT("Set battleship special data for "<< newShip, DEBUG_TRIVIA | DEBUG_EOT);
			break;
		default:
			DEBUG_OUT("Shouldn't be in this default case, classIdx = "<< (int)classIdx, DEBUG_ERROR);
		    break;
	}
	if (classIdx > sMaxShipNames) { // v2.1b11, get max ship names from resource
		classIdx = (EShipClass)sMaxShipNames;
	}
	newShip->SetSpeed(speed);
	newShip->SetPower(power);
// for fog of war
    newShip->SetApparentSize(apparentSize);
    UInt8 stealth = 1;
    if (mGame->HasFogOfWar() && (classIdx == class_Scout)) {
        stealth = 4;
    }
    newShip->SetStealth(stealth);
    UInt32 scanRange = UThingyClassInfo::GetScanRangeForTechLevel(mTechLevel, mGame->IsFastGame(), mGame->HasFogOfWar());
    // adjust scanner range by ship class
    scanRange *= percentRangeAdj;
    scanRange /= 10L;
    newShip->SetScannerRange(scanRange);
// ship class and name
	newShip->SetShipClass(classIdx);
	std::string s;
	int nameIdx = classIdx + 1;
	int maxIdx = GetNumSubStringsInStringResource(STRx_ShipNames);
	if (nameIdx > maxIdx) {
	    nameIdx = maxIdx;
	}
	LoadStringResource(s, STRx_ShipNames, nameIdx);  // build a name for the ships, along the lines of "Courier S32T13-102"
  #ifndef GALACTICA_NO_SHIP_SERIAL_NUMBERS
	char nameBuf[255];
	// MacOS DrawString() treats tab as a space, so we can use it as both a visual and a logical break
	// between the ship class name and the ship serial number
	std::sprintf(nameBuf, "%s\tS%ldT%ld-%ld", s.c_str(), mID, mGame->GetTurnNum(), newShip->GetID());
	newShip->SetName(nameBuf);
  #else
    newShip->SetName(s);
  #endif // GALACTICA_NO_SHIP_SERIAL_NUMBERS
//	mGame->LoadPlayerInfo(mOwnedBy);
	DEBUG_OUT("Finished Building "<<newShip<<" of class "<< (int)classIdx, DEBUG_TRIVIA | DEBUG_EOT);
	if (addToSatelliteFleet) {
		AddToSatelliteFleet(newShip);	// place ship into satellite fleet
		DEBUG_OUT("Added "<<newShip<<" to satellite fleet ", DEBUG_TRIVIA | DEBUG_EOT);
	} else {
		AddContent(newShip, contents_Ships, kDontRefresh);	// place ship initially at creator star
	}
// ship destination
	if (canMove && !mNewShipDestination.IsNull()) {	// set the destination for the ship
		DEBUG_OUT("Setting new ship "<<newShip<<" auto destination", DEBUG_TRIVIA | DEBUG_MOVEMENT);
		newShip->SetWaypoint(1, newShip->GetPosition(), kDontRefresh); // v2.0.9b3 add starting point
		newShip->SetWaypoint(2, mNewShipDestination, kDontRefresh);	// if there is a New Ship Dest
		newShip->SetFromWaypointNum(1);
		newShip->SetDestWaypointNum(2);
		newShip->CalcMoveDelta();
		newShip->SetHasMoved(false);  // allow it to move on the turn it is created
		notify = false;									// for the star system.
	}
//				newShip->CalcMoveDelta();			// make sure we recalculate ships movement
	if (classIdx == class_Satellite) {		// satellites are automatically on patrol
		RecalcPatrolStrength();				// so we need immediate update to star's patrol strength
	}
	newShip->ThingMoved(kDontRefresh);
	newShip->Persist();		// ship saves itself into database
	if (host->PlayerHasNewHullType(mOwnedBy, classIdx, mTechLevel)) {	// v1.2b8d2 change, fix out-of-sequence DB add
		DEBUG_OUT("Player "<<mOwnedBy<<" has new hull type: "<<newShip<<", tech "
					<<mTechLevel, DEBUG_TRIVIA | DEBUG_MOVEMENT | DEBUG_MESSAGE);
//		mGame->SavePlayerInfo();
		CMessage* msg = CMessage::CreateEvent(GetGameHost(), event_BuiltNewClass, mOwnedBy, 
						newShip, kAllPlayers, mOwnedBy, kNoOtherPlayers, 
						mTechLevel, Layr_StardockBase + classIdx, false);
		if (msg) {
			msg->SetFocus(this);	// the ship is starting from the star, not from the
									// point in space it is at (when NewShipDest set)
			msg->Persist();
			msg->NotChanged();	// don't want host to write it twice
		}
	}
	if (notify) {			// v1.2b8d2 change, fix out-of-sequence DB add
		CMessage::CreateEvent(GetGameHost(), event_BuiltShip, mOwnedBy, newShip);
	}
  #ifdef DEBUG
	classIdx = newShip->GetShipClass();
	if (classIdx == class_Satellite) {
		// v1.2b11d2 check for satellite not in satellite fleet
		if (!( (CFleet*)(newShip->GetContainer()))->IsSatelliteFleet()) {
			DEBUG_OUT(newShip<< " is a satellite inside " << newShip->GetContainer(), DEBUG_ERROR | DEBUG_EOT);
		}
		// v1.2b11d3, check for mobile satellite
		if (newShip->GetSpeed() != 0) {
			DEBUG_OUT(newShip<< " is a satellite that can move, speed = " << newShip->GetSpeed(), DEBUG_ERROR | DEBUG_EOT);
		}
	}
	if (classIdx == class_Scout) {
		// v1.2b11d2 check scout for attack capability
		if (newShip->GetPower() != 0) {
			DEBUG_OUT(newShip<< " is a scout with power " << newShip->GetPower(), DEBUG_ERROR | DEBUG_EOT);
		}
	}
  #endif // DEBUG	
/*			  #if true	// temporary debug code to test how game behaves with items that are
  			// destroyed on same turn they are created
	DEBUG_OUT("Killing new ship "<<newShip<<" for a debugging test.", DEBUG_ERROR);
	newShip->DestroyedInCombat(2);
  #endif */
// setup for construction of next ship
	if (mGame->PlayerIsComputer(mOwnedBy)) {
	  #if TUTORIAL_SUPPORT
	   if (Tutorial::TutorialIsActive()) {
	      mBuildHullType = class_Fighter;
	      DEBUG_OUT("Tutorial computer always building battleships", DEBUG_DETAIL | DEBUG_AI);
	   } else
	  #endif // TUTORIAL_SUPPORT
		if (IsTechStar()) {
			// tech stars alternate between building satellites and building scouts
			if (mBuildHullType == class_Satellite) {
				mBuildHullType = class_Scout;
			} else {
				mBuildHullType = class_Satellite;
			}
		} else {										// each time a computer finishes
			mBuildHullType = Rnd(5) + class_Satellite;	// building a ship, it should
			if (mBuildHullType > class_Fighter) {		// randomly pick a new type
				mBuildHullType = class_Fighter;			// weightings are:
			} else if (mBuildHullType == class_Scout) {	// satellites: 	 3 in 10
				if (Rnd(20) <= 10) {					// scouts: 		 1 in 10
					mBuildHullType = class_Satellite;	// colony ships: 2 in 10
				}										// battleships:  4 in 10
			}
			// after turn 50, start changing some scouts and colonies into battleships
			if (mGame->GetTurnNum() > 50) {
				if ( (mBuildHullType == class_Scout) || (mBuildHullType == class_Colony) ) {
					if (Rnd(20) <= (mGame->GetTurnNum() / 25) ) {
						DEBUG_OUT("Choose to build "<<mBuildHullType<< " at "<<this<<" building battleship instead", DEBUG_TRIVIA | DEBUG_AI);
						mBuildHullType = class_Fighter;
					}
				}
			}
		}
		DEBUG_OUT("Decided to build new ship of class "<<mBuildHullType<< " at "<<this, DEBUG_DETAIL | DEBUG_AI);
	}
}

void
CStar::SpendTime(const EventRecord &) {
	// еее twinkle here
}

// should only be called by the server Doc
void
CStar::CreateStarLanes(int inNumLanes) {
	int maxLanes = inNumLanes;
	CStar	*aNeighbor;
	long minDist = 0x8000;	// half a lightyear min distance
	long maxDist = 0x10000;	// 1 lightyear initial max distance
	for (int i = 0; i < maxLanes; i++) {
		bool found = false;
		do {
			aNeighbor = mGame->FindNeighboringStar(this, minDist, maxDist);
			if (aNeighbor != nil) {
				found = EstablishStarLaneTo(aNeighbor);
				maxDist = minDist = Distance(aNeighbor) + 1;	// don't find this one again
			}
			maxDist += 0x10000;	// + 1 lightyear
		} while ( (!found) && (maxDist < 0x60000) ); // don't establish starlanes longer than 6 ly
	}
}

// should only be called by the server app
bool
CStar::EstablishStarLaneTo(CStar* ) { //inStar
//	CStarLane* theLane = nil;
/*	for (int i = 0; i < MAX_STAR_LANES; i++) {
		if (mStarLanes[i] == nil) {
			ASSERT(mSuperView != nil);
			theLane = new CStarLane(mSuperView, mGame);
			if ( inStar->AddStarLaneFrom(this, theLane) ) {
				mStarLanes[i] = theLane;
				theLane->RecordStarPositions(this, inStar);
				mNumStarLanes++;
				theLane->Persist(); // adds itself to the database
				return true;
			} else {
				delete theLane;
				return false;
			}
		} else if (mStarLanes[i]->GetOtherStar(this) == inStar) {
			return true;	// connection already established
		}
	} */
	return false;
}

bool
CStar::AddStarLaneFrom(CStar*, CStarLane *inLane) { // inStar
	for (int i = 0; i < MAX_STAR_LANES; i++) {
		if (mStarLanes[i] == nil) {
			mStarLanes[i] = inLane;
			mNumStarLanes++;
			return true;	// willing to establish connection
		} else if (mStarLanes[i] == inLane) {
			return true;	// connection already established
		}
	}
	return false;
}


long
CStar::TakeDamage(long inHowMuch) {
	long pop = GetPopulation();
	float popFactor = pop / 49000;					// higher populations mean higher concentration
	if (popFactor < 0.1) popFactor = 0.1;			// so more people killed with each attack
	pop -= (long)((float)(inHowMuch * 32) * popFactor);	// reduce population by amount of damage
	if (pop < 0) {
		pop = 0;
	}
	SetPopulation(pop);
	if (mTechLevel>0) {	// don't divide by zero, which would give NAN(001) and irradiate the planet
		for (int i = 1; i<MAX_STAR_SETTINGS; i++) {	// damage production capacity: mSettings[spending_Ships] & [spending_Research]
			mSettings[i].benefit -= (mSettings[i].benefit * 0.0001 * inHowMuch / mTechLevel);
			if (!(mSettings[i].benefit >= 6)) {		// еее 1.2d5, to fix "irradiated" planet problem
				DEBUG_OUT("Fixing Irradiated Planet "<<this<<"; benefit = "<<mSettings[i].benefit, DEBUG_DETAIL);	// see if it is still happening
				ASSERT_STR(mSettings[i].benefit>5, "Severely Irradiated Planet "<<this<<"; benefit = "<<mSettings[i].benefit);	// see if it is still happening
				mSettings[i].benefit = 6;
			}
		}
	}
	Changed();
	return 0;
}

// planets are never actually destroyed.
void
CStar::Die() { //inKiller) {
	mOwnedBy = 0;	// wipe out the owner's base
	mNeedsDeletion = 0;	// make sure we don't try to delete it
	mNewShipDestination = gNullPt3D; // stop autodestinations
	Changed();
}

// see if the star refers to the thingy in question in any way
bool
CStar::RefersTo(AGalacticThingy *inThingy) {
	ASSERT(inThingy != nil);
	bool refersToIt = (inThingy == mNewShipDestination);
	if (!refersToIt) {
		refersToIt = ANamedThingy::RefersTo(inThingy);
	}
	return refersToIt;
}

// remove all references we have to a thing
void
CStar::RemoveReferencesTo(AGalacticThingy *inThingy) {
	ASSERT(inThingy != nil);
	if (inThingy == mNewShipDestination) {
		mNewShipDestination = gNullPt3D;
		DEBUG_OUT("Removed reference to "<<inThingy<<" from "<<this, DEBUG_TRIVIA | DEBUG_MOVEMENT);
		ThingMoved(kDontRefresh);
	}
	ANamedThingy::RemoveReferencesTo(inThingy);
}


void
CStar::ChangeIdReferences(PaneIDT oldID, PaneIDT newID) {
    if (mNewShipDestination.ChangeIdReferences(oldID, newID)) {
        DEBUG_OUT("Updated autodestination of "<<this<<" (was to "<<oldID<<")", DEBUG_DETAIL | DEBUG_MOVEMENT);
    }
	long numRefs = mShipRefs.GetCount();
	for(int i = 1; i <= numRefs; i++) {				// go through the contained items list:
		ThingyRef& ref = mShipRefs.FetchThingyRefAt(i);
        if (ref.GetThingyID() == oldID) {
            DEBUG_OUT("Updated item contained in "<<this, DEBUG_DETAIL | DEBUG_CONTAINMENT);
            ref.SetThingyID(newID, mGame);
        }
    }
    ANamedThingy::ChangeIdReferences(oldID, newID);    
}


// inPopulation is actual population/1000
void
CStar::SetPopulation(unsigned long inPopulation) {
	mPopulation = inPopulation;
	if (inPopulation > 99900) {	// don't overrun the slider's population indicator,
		inPopulation = 99900;	// which stops at 99.9 million
	}
	mSettings[spending_Growth].economy = inPopulation/100;
}

#define kMaxPop         16.0
#define kLimit          4.0		// 1 / kLimitSqr = maximum growth rate from formula for > 1 billion
#define kLimitSqr       16.0    // kLimit * kLimit

void
CStar::GrowPopulation() {
	float   x,g,p1;
	g = (float)mSettings[spending_Growth].value / 500;
	p1 = (float)mPopulation/1000000;
	if(p1>1) {
		x = p1 / (kMaxPop*kLimit);
		x = (-(x*x)) + (1 / kLimitSqr);
		p1 = mPopulation*(x*g);
	} else {
		x = p1 / 0.2+5;
		x = 10 / (x*x+20);
		p1 = mPopulation*(x*g);
	}
	if (p1<1) {
		p1 += 1.5;
	}
	SetPopulation((long)(mPopulation + p1));
}

void
CStar::RecalcPatrolStrength() {
	mPatrolStrength = 0;
//	int numShips = mShips.GetCount();	1.2b8
	int numShips = mShipRefs.GetCount();
	if (numShips > 0) {
		CShip* aShip = nil;
		for (int i = 1; i <= numShips; i++) {
			aShip = (CShip*)mShipRefs.FetchThingyAt(i);
			if (aShip == nil) {
				continue;
			}
			if (aShip->IsOnPatrol()) {
				mPatrolStrength += aShip->GetDefenseStrength();
			}
		}
	}
}

SInt32
CStar::GetTotalDefenseStrength() const {
	SInt32 strength = GetDefenseStrength();
//	int numShips = mShips.GetCount(); 1.2b8
	int numShips = mShipRefs.GetCount();
	if (numShips > 0) {
		CShip* aShip = nil;
		for (int i = 1; i <= numShips; i++) {
			aShip = (CShip*) mShipRefs.FetchThingyAt(i);
			if (aShip == nil) {
				continue;
			}
			// 1.2b11d2, only add allies to strength
			if ((mOwnedBy == 0) || aShip->IsAllyOf(mOwnedBy)) {
				strength += aShip->GetDefenseStrength();
			}
		}
	}
	return strength;
}

SInt32
CStar::GetNonSatelliteShipStrength() const {
	SInt32 strength = 0;
	int numShips = mShipRefs.GetCount();
	if (numShips > 0) {
		CShip* aShip = NULL;
		for (int i = 1; i <= numShips; i++) {
			aShip = static_cast<CShip*>( mShipRefs.FetchThingyAt(i) );
			if (aShip == NULL) {
				continue;
			}
			if (aShip->GetThingySubClassType() == thingyType_Fleet) {
			   CFleet* aFleet = static_cast<CFleet*>( aShip );
			   if (aFleet->IsSatelliteFleet()) {
			      continue;   // don't count the satellite fleet
			   }
			}
			// 1.2b11d2, only add allies to strength
			if ((mOwnedBy == 0) || aShip->IsAllyOf(mOwnedBy)) {
				strength += aShip->GetDefenseStrength();
			}
		}
	}
	return strength;
}

void
CStar::SetBuildHullType(SInt16 inHullType) {
	mBuildHullType = inHullType;
	mCostOfNextShip = GetBuildCostOfShipClass((EShipClass)inHullType, mTechLevel);
	Changed();
}



#ifndef GALACTICA_SERVER

float
CStar::GetBrightness(short inDisplayMode) {
	float currLevel, highestLevel, lowestLevel;
    GalacticaDoc* game = GetGameDoc();
	bool canSeeOwner = true;
    if (game) {
        int myPlayerNum = game->GetMyPlayerNum();
        canSeeOwner = IsAllyOf(myPlayerNum) || (GetVisibilityTo(myPlayerNum) > visibility_None);
    }
	if ((GetOwner() == 0) || !canSeeOwner) {
		return 0.6;
	} 
	else if (inDisplayMode == displayMode_Defense) {	// v1.2b11d3, include strength of ships at star
		highestLevel = GetGameDoc()->GetHighest(inDisplayMode);	// the GameDoc pre-calcs these for each turn
		lowestLevel = GetGameDoc()->GetLowest(inDisplayMode);		// in GalacticaDoc::RecalcHighestValues()
		currLevel = GetTotalDefenseStrength();
		return CalcBrightnessFromRange(currLevel, lowestLevel, highestLevel, LOWEST_STAR_BRIGHTNESS);
	}
	else if ( (inDisplayMode >= displayMode_Spendings) && (inDisplayMode <= displayMode_Last) ) {
		register ESpending which = (ESpending) (inDisplayMode - displayMode_Spendings);
		currLevel = GetSpending(which);
		highestLevel = GetGameDoc()->GetHighest(inDisplayMode);	// the GameDoc pre-calcs these for each turn
		lowestLevel = GetGameDoc()->GetLowest(inDisplayMode);		// in GalacticaDoc::RecalcHighestValues()
		return CalcBrightnessFromRange(currLevel, lowestLevel, highestLevel, LOWEST_STAR_BRIGHTNESS);
	} else {
		return ANamedThingy::GetBrightness(inDisplayMode);
	}
}


bool
CStar::DrawSphereOfInfluence(int level) {
	if (!ShouldDraw()) {
		return false;
    }
    if (mOwnedBy == 0) {  // ignore stars that are not owned
        return false;
    }
    GalacticaDoc* game = GetGameDoc();
    if (!game) {
        return false;
    }
    int myPlayerNum = game->GetMyPlayerNum();
	bool isAlly =  IsAllyOf(myPlayerNum);
	bool canSeeScanRange = isAlly || (GetVisibilityTo(myPlayerNum) >= visibility_Tech);
	if (!canSeeScanRange) {
	    return false;
	}
    // draw a sphere of influence
	Rect scanRect;
    if (level == -1) {	// drawing all levels at once
		EraseSelectionMarker();
	    RGBColor scanColor = *(game->GetColor(mOwnedBy, false));
		CStarMapView* theStarMap = static_cast<CStarMapView*>(mSuperView);
		float zoom = theStarMap->GetZoom();
	    long scanradius = (long)((float)(GetScannerRange() / 256L) * zoom);
	    float startingBrightness = (game->DrawStarmapNebulaBackground()) ? 0.125f : 0.0625f;
	    BrightenRGBColor(scanColor, startingBrightness);  //0.03125f
	    short penMode = addMax; // (game->DrawStarmapNebulaBackground()) ? addMax : srcCopy;
	    for (int i = 0; i < 3; i++) {
	        if (scanradius < 32000) {
	            // only draw if in QD coordinate space
	            MacAPI::PenMode(penMode);
	            scanRect.top = mDrawOffset.v - scanradius;
	            scanRect.bottom = mDrawOffset.v + scanradius;
	            scanRect.left = mDrawOffset.h - scanradius;
	            scanRect.right = mDrawOffset.h + scanradius;
	            MacAPI::RGBForeColor(&scanColor);
	//            Pattern pat;
	//            MacAPI::GetQDGlobalsLightGray(&pat);
	//            MacAPI::FillOval(&scanRect, &pat);
	            MacAPI::PaintOval(&scanRect);
	            MacAPI::PenMode(srcCopy);
	            BrightenRGBColor(scanColor, 1.5f);
	            scanradius /= 2;
	        }
	    }
    } else {
    	// drawing a single level as part of defining a region
		Point p = mCurrPos.GetLocalPoint();
		p.h /= 4;  // position at zoom = 0.25
		p.v /= 4;
	    long scanradius = GetScannerRange() / 1024L; // this gives us size at zoom = 0.25
    	if (level != 0) {
    		scanradius /= (2*level);
    	}
        scanRect.top = p.v - scanradius;
        scanRect.bottom = p.v + scanradius;
        scanRect.left = p.h - scanradius;
        scanRect.right = p.h + scanradius;
	    MacAPI::FrameOval(&scanRect);
    }
    return true;
}

void
CStar::DrawSelf() {
//    DrawSphereOfInfluence(); for debugging only
	if (!ShouldDraw())
		return;
	EraseSelectionMarker();
	Rect frame, pFrame;
//	CalcLocalFrameRect(frame);	// еее debugging
//	MacAPI::FrameRect(&frame);
	CStarMapView* theStarMap = static_cast<CStarMapView*>(mSuperView);
    GalacticaDoc* game = GetGameDoc();
    GalacticaHostDoc* host = NULL;
    bool isAlly = false;
    int myPlayerNum = 0;
    if (!game) {
        host = dynamic_cast<GalacticaHostDoc*>(GetGame());
    } else {
        myPlayerNum = game->GetMyPlayerNum();
	    isAlly = IsAllyOf(myPlayerNum) || game->FogOfWarOverride();
    }
	bool canSeeOwner = isAlly || (GetVisibilityTo(myPlayerNum) > visibility_None);
	bool canSeeScanRange = isAlly || (GetVisibilityTo(myPlayerNum) >= visibility_Tech);
	RGBColor c;
	if (game) {
    	if (canSeeOwner) {
    	    c = *(game->GetColor(mOwnedBy, false));
    	} else {
    	    c = *(game->GetColor(0, false));
    	}
	} else if (host) {
	    c = *(host->GetColor(mOwnedBy));
	    canSeeOwner = true; // we can always see the owner on the host
	    canSeeScanRange = false; // never show scan range on host
	} else {
	    MakeRGBColor(c, 0xffff,0xffff,0xffff);  // white if we can't see the owner
	}

	RGBColor scanColor = c;
	RGBColor c1 = c;
	int displayMode = theStarMap->GetDisplayMode();
	float starBrightness = GetBrightness(displayMode);
	BrightenRGBColor(c, starBrightness);
	float ringBrightness;
	if (game && (displayMode == displayMode_Defense)) {
		ringBrightness = (float) mPatrolStrength / game->GetHighest(displayMode_Defense);
	} else if (game && game->DrawStarmapNebulaBackground()) {
		// v3 brighter patrol ring when over nebula background
		ringBrightness = 0.6f;
	} else {
		// 1.2b11d5, use dim color for patrol rings
		ringBrightness = 0.4f;
	}
	// v1.2b11d5, enforce min & max brightness
	if (ringBrightness > starBrightness) {	// max, ring shouldn't be brighter than star
		ringBrightness = starBrightness;
	}
	if (ringBrightness < LOWEST_STAR_BRIGHTNESS) {
		ringBrightness = LOWEST_STAR_BRIGHTNESS;	// min, ring shouldn't be too dim to see
	}
	BrightenRGBColor(c1, ringBrightness);
	frame = GetCenterRelativeFrame(kSelectionFrame);
	MacAPI::MacOffsetRect(&frame, mDrawOffset.h, mDrawOffset.v);
	pFrame.top = frame.top - 3;
	pFrame.left = frame.left - 3;
	pFrame.right = frame.right + 3;
	pFrame.bottom = frame.bottom + 3;
	float zoom = theStarMap->GetZoom();
	bool selected = (this == mGame->AsSharedUI()->GetSelectedThingy());
	if (Hiliting()) {
		CMouseTracker* tracker = theStarMap->GetMouseTracker();
		if (tracker) {
			tracker->Hide(false);	// this will erase the mouse tracker so there are
		}							// no screen artifacts
	}
    // draw scanner range
    if (mOwnedBy && canSeeScanRange && selected && !Hiliting()) {
    	MacAPI::PenMode(srcCopy);
        long scanradius = (long)((float)(GetScannerRange() / 1024L) * zoom);
//        if (!selected) {
//            BrightenRGBColor(scanColor, 0.25f);
//        }
        for (int i = 0; i < 3; i++) {
            if (scanradius > 32000) {
                // only draw if in QD coordinate space
                break;  // do nothing further if not
            }
            Rect scanRect;
            scanRect.top = mDrawOffset.v - scanradius;
            scanRect.bottom = mDrawOffset.v + scanradius;
            scanRect.left = mDrawOffset.h - scanradius;
            scanRect.right = mDrawOffset.h + scanradius;
            MacAPI::RGBForeColor(&scanColor);
            MacAPI::FrameOval(&scanRect);
            BrightenRGBColor(scanColor, 0.5f);
            scanradius *= 2;
        }
    }
	if (selected) {
		MacAPI::ForeColor(cyanColor);
		if (isAlly && !mNewShipDestination.IsNull()) {
			Point p = mNewShipDestination.GetLocalPoint();
			theStarMap->GalacticToImagePt(p);
			MacAPI::MoveTo(mDrawOffset.h, mDrawOffset.v);	
			MacAPI::MacLineTo(p.h, p.v);
		}		
	}
    // draw star
	if (zoom < 0.125f) {
		MacAPI::RGBForeColor(&c);
		MacAPI::MoveTo(mDrawOffset.h, mDrawOffset.v);	// tiny, just draw single point
		MacAPI::Line(0,0);
	} else {
	    bool canSeePatrol = isAlly || (GetVisibilityTo(myPlayerNum) >= visibility_Details);
		if (canSeePatrol && (mPatrolStrength > 0) && (zoom > 0.25f)) {	// draw patrol orbit (v1.2b11d5, unless too small)
			MacAPI::RGBForeColor(&c1);
			MacAPI::MacInsetRect(&pFrame, 0, (3*(pFrame.bottom - pFrame.top))/8);
			MacAPI::FrameArc(&pFrame, 20, 320);
		}
	// Added 970703 JRW
		MacAPI::RGBForeColor(&c);
		CNewGWorld* stars = NULL;
		stars = GalacticaApp::GetStarGrid();
		if (!stars) {	// just draw a circle over the star if there is no star art available
			frame.top += 1;
			frame.left += 1;
			frame.bottom -= 3;
			frame.right -=3;
			MacAPI::PaintOval(&frame);
		} else {
			MacAPI::ForeColor(blackColor);
			MacAPI::BackColor(blackColor);
			float bright = GetBrightness(displayMode);
			bright = (bright - 0.3f)/0.7f;
			int brightIndex = 6 - 5*bright;
			int	colorIndex = 1;
			if (canSeeOwner) {
    			if (game) {
    			    colorIndex += game->GetColorIndex(mOwnedBy, false);
    			} else if (host) {
    			    colorIndex += host->GetColorIndex(mOwnedBy);
    			}
			}
			if (Hiliting() && !IsHilited()) {
				// special case: removing hiliting
				SetRGBOpColor(0, 0, 0);
				stars->CopySubImage(GetMacPort(), frame, brightIndex, colorIndex, subPin, nil);
				Refresh();
				FocusDraw();	// The refresh changes the focus to the window, so restore it
			} else {
				SetRGBOpColor(0xffff, 0xffff, 0xffff);
				stars->CopySubImage(GetMacPort(), frame, brightIndex, colorIndex, addPin, nil);
			}
		}
	// End add
	}
//	ANamedThingy::InternalDrawSelf(kIgnoreVisibility);
	ANamedThingy::InternalDrawSelf(kConsiderVisibility);
}


void
CStar::DoubleClickSelf() {
    if (!mGame->IsClient()) {
        return;
    }
    GalacticaDoc* game = GetGameDoc();
    if (!game) {
        return;
    }
    int myPlayerNum = game->GetMyPlayerNum();
    bool isAlly = IsAllyOf(myPlayerNum);
    if (isAlly || (GetVisibilityTo(myPlayerNum) >= visibility_Details)) {
    	if (mShipRefs.GetCount() == 1) {
    		CShip *it = (CShip*) mShipRefs.FetchThingyAt(1);
    		if (it) {
    			it->Select();
    		}
    	} else { // tell game to select ships at star panel
    		CBoxView* buttonBar = static_cast<CBoxView*>(game->GetCurrButtonBar());
    		if (buttonBar) {
    			buttonBar->SimulateButtonClick(cmd_SetPanelFirst+1);
    		}
    	}
	}
}

Boolean
CStar::IsHitBy(SInt32 inHorizPort, SInt32 inVertPort) {
	Boolean result = ANamedThingy::IsHitBy(inHorizPort, inVertPort);
	if (result) {	// check that this wasn't clicking on a ship
		long count = mShipRefs.GetCount();
		long i = 1;
		if (count>0) {
			while (i <= count) {
				CShip *it = (CShip*) mShipRefs.FetchThingyAt(i);
				if (it) {
					// if the ship was hit, then that means the star wasn't
					result = !(it->IsHitBy(inHorizPort, inVertPort));
					if (!result) {
						break;	// one of the ships was hit, we can stop checking
					}
				}
				i++;
			}
		}
	}
	return result;
}

float
CStar::GetScaleFactorForZoom() const {	// scaling is not linear for stars, 1.2d8
	float scale;
	if (mSuperView) {
		scale = ((CStarMapView*)mSuperView)->GetZoom();	// v1.2d8
	} else {
		scale = 1;
	}
	if (scale > 1) {			// this little bit of math
		scale -= 1;				// makes zooming in to 8:1 give 2:1 star size
		scale = scale/7;		// and makes 1:1 be at 1:1 star size, with a linear
		scale += 1;				// scale in between
	}
	return scale;
}

Rect
CStar::GetCenterRelativeFrame(bool full) const {
	Rect r = ANamedThingy::GetCenterRelativeFrame(full);
	if (full && mSuperView && !mNewShipDestination.IsNull()) {
		Point p2 = mCurrPos.GetLocalPoint();			// adjust frame to include line for new ship destinations
		Point p = mNewShipDestination.GetLocalPoint();
		p.h -= p2.h;					// get offset from current position to destination position
		p.v -= p2.v;
		((CStarMapView*)mSuperView)->GalacticToImagePt(p);	// scale to match zooming
		r.top = Min(p.v - 1, r.top);		// now adjust the Center Relative Frame to accomodate
		r.bottom = Max(p.v + 1, r.bottom);	// the line to the destination position
		r.left = Min(p.h - 1, r.left);
		r.right = Max(p.h + 1, r.right);
	}
	return r;
}


#endif // GALACTICA_SERVER

/*
// **** STARLANES ****


CStarLane::CStarLane(LView *inSuperView, GalacticaShared* inGame) 
: AGalacticThingyUI(inSuperView, inGame, thingyType_StarLane) { //, gNullPt3D) {
//	mActive = triState_Off;		// aren't selectable
	mEnabled = triState_Off;
	mFromStar = nil;
	mToStar = nil;
  #ifdef BALLOON_SUPPORT
	CThingyHelpAttach* theBalloonHelp = new CThingyHelpAttach(this);
	AddAttachment(theBalloonHelp);
  #endif
}

CStarLane::CStarLane(LView *inSuperView, GalacticaShared* inGame, LStream *inStream) 
: AGalacticThingyUI(inSuperView, inGame, inStream, thingyType_StarLane) {
	ReadStarLaneStream(*inStream);
  #ifdef BALLOON_SUPPORT
	CThingyHelpAttach* theBalloonHelp = new CThingyHelpAttach(this);
	AddAttachment(theBalloonHelp);
  #endif
}
*/

/*  Keep this around for now as an example of what we need to do to create starlanes
void
CStarLane::FinishCreateSelf() {
	PaneIDT id;
	id = (PaneIDT) mFromStar;
	mFromStar = (CStar*)mGame->FindThingByID(id);
	ASSERT_STR(mFromStar != nil, "star "<<id<<" from-star of "<<this<<" not found");
	id = (PaneIDT) mToStar;
	mToStar = (CStar*)mGame->FindThingByID(id);
	ASSERT_STR(mToStar != nil, "star "<<id<<" to-star of "<<this<<" not found");
//	AGalacticThingy::FinishCreateSelf(); // don't really want to call this anyway
	RecordStarPositions(mFromStar, mToStar);	// recalc pane size and position
}
*/

/*
void
CStarLane::ReadStream(LStream *inStream) {
	AGalacticThingy::ReadStream(inStream);
	ReadStarLaneStream(*inStream);
}

void
CStarLane::WriteStream(LStream *inStream) {
	AGalacticThingy::WriteStream(inStream);
	*inStream << mFromStar->GetPaneID() << mToStar->GetPaneID();
}

void
CStarLane::UpdateClientFromStream(LStream *inStream) {
	AGalacticThingy::UpdateClientFromStream(inStream);
	ReadStarLaneStream(*inStream);
}

void
CStarLane::UpdateHostFromStream(LStream *inStream) {
	AGalacticThingy::UpdateHostFromStream(inStream);
	ReadStarLaneStream(*inStream);
}

void
CStarLane::ReadStarLaneStream(LStream &inStream) {
	PaneIDT id1, id2;
	inStream >> id1 >> id2;
	mFromStar = (CStar*)id1;	//temp assignment of pane until we can finish streaming
	mToStar = (CStar*)id2;		// and be assured of finding CStar objects by Pane ID
}


void
CStarLane::RecordStarPositions(const CStar* fromStar, const CStar* toStar) {
	mFromStar = (CStar*)fromStar;
	mToStar = (CStar*)toStar;
	Point fromPt = fromStar->GetPosition();
	Point toPt = toStar->GetPosition();
	SInt32 top = (fromPt.v < toPt.v) ? fromPt.v : toPt.v;
	SInt32 left = (fromPt.h < toPt.h) ? fromPt.h : toPt.h;
	SInt32 width = (fromPt.h < toPt.h) ? toPt.h - fromPt.h : fromPt.h - toPt.h;
	SInt32 height = (fromPt.v < toPt.v) ? toPt.v - fromPt.v : fromPt.v - toPt.v;
	PlaceInSuperImageAt(left, top, false);
	ResizeFrameTo(width, height, false);
	if (fromPt.v < toPt.v) {
		mLeft2Right = (fromPt.h < toPt.h);
	} else {
		mLeft2Right = (fromPt.h > toPt.h);
	}
}


CStar*
CStarLane::GetOtherStar(CStar* inStar) {
	if (inStar == mToStar) {
		return mFromStar;
	} else {
		return mToStar;
	}
}

void
CStarLane::DrawSelf() {
	if (!GetGameDoc()->DrawStarLanes())
		return;
	Rect frame;
	CalcLocalFrameRect(frame);
	if (Hiliting()) {
		::ForeColor(magentaColor);
	} else {
		SetRGBForeColor(0x6000,0,0x8000);	//purple
	}
	if (mLeft2Right) {
		::MoveTo(frame.left, frame.top);
		::MacLineTo(frame.right, frame.bottom);
	} else {
		::MoveTo(frame.right, frame.top);
		::MacLineTo(frame.left, frame.bottom);
	}
}
*/

