// CShip.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ

#include "CShip.h"

#include "CStar.h"
//#include "Galactica.h"
#include "GalacticaProcessor.h"

#include "CMessage.h"
#include "CFleet.h"
#include "CRendezvous.h"

#ifndef GALACTICA_SERVER
// GUI Only
#include "GalacticaPanes.h"
#include "GalacticaPanels.h"
#include "GalacticaDoc.h"
#include "GalacticaHostDoc.h"
#include "CHelpAttach.h"
#include <UDrawingUtils.h>
#endif // GALACTICA_SERVER

#define SHIP_NORMAL_BRIGHTNESS 0.75f
#define SHIP_IGNORE_BRIGHTNESS 0.05f

CShip::CShip(LView *inSuperView, GalacticaShared* inGame, long inThingyType)
:ANamedThingy(inSuperView, inGame, inThingyType) {
	InitShip();
}


/*
CShip::CShip(LView *inSuperView, GalacticaShared* inGame, LStream *inStream, long inThingyType) 
:ANamedThingy(inSuperView, inGame, inStream, inThingyType) {
	InitShip();
	ReadShipStream(*inStream);
}
*/

CShip::~CShip() {
}

void
CShip::InitShip() {
	mSpeed = mRemDist = mDamage = mPower = 0;
	mSelectedWPNum = 0;
	mClass = 0;
	mMoveDelta = gNullPt3D;
	mCourseChanged = false;	// no user changes yet
	mUserCallIn = false;
	mHasMoved = true;       // effectively all moves were complete
  #if BALLOON_SUPPORT
	CThingyHelpAttach* theBalloonHelp = new CThingyHelpAttach(this);
	AddAttachment(theBalloonHelp);
  #endif
}

void
CShip::ReadStream(LStream *inStream, UInt32 streamVersion) {
	ANamedThingy::ReadStream(inStream, streamVersion);
	ReadShipStream(*inStream);
}

void
CShip::WriteStream(LStream *inStream, SInt8 forPlayerNum) {
	mWantsAttention = mWantsAttention | mUserCallIn;	// make sure it gets attention if needed
	ANamedThingy::WriteStream(inStream);
	*inStream << mClass << mSpeed << mPower << mRemDist << mMoveDelta.x << mMoveDelta.y << mMoveDelta.z;
	*inStream << mCourse << mCourseMem;	// write out the waypoints list for the current course
	mCourseChanged = false;	// course changes for user have now been maintained
}

#ifndef GALACTICA_SERVER
void
CShip::UpdateClientFromStream(LStream *inStream) {
	CourseT course, courseMem;
	bool restore, callin, restoreCourse;
	SInt16 restoreCurr, restorePrev;

    GalacticaDoc* doc = GetGameDoc();
    if (!doc) {
        restore = false;
    } else {
	    restore = ( (mOwnedBy == doc->GetMyPlayerNum()) && mChanged );
	}
	// еее eventually this should be based on our access level to the object, because we may have 
	// rights to change things we don't own еее
	restoreCourse = (restore && mCourseChanged);
	if (restore) {
		if (restoreCourse) {
			courseMem = mCourseMem;	// save mem course for restoration
			course = mCourse;		// save current course for restoration
		}
		callin = mWantsAttention;
	}
	ANamedThingy::UpdateClientFromStream(inStream);
	ReadShipStream(*inStream);		// read in everything
	if (restore) {
		mWantsAttention = (mWantsAttention || callin);	// the call in button may be set by the host
		if (restoreCourse) {
		    // the host knows which waypoint we are currently on, and it may have switched
		    // from one to another this turn
			restoreCurr = mCourse.curr;	// these need to be able to change
			restorePrev = mCourse.prev;
		  #warning FIXME: need to fix course preservation code
		  // FIXME: if the old course has just swapped to the next item we want to switch to our next
		  // item in the newcourse, otherwise we should take the current course leg that is in the 
		  // new course. Unfortunately, that's not the way it works now
			mCourseMem = courseMem;
			mCourse = course;
			if (restoreCurr <= GetWaypointCount()) {
				mCourse.curr = restoreCurr;	// this is orchestrated by the host
			}
			if (restorePrev <= GetWaypointCount()) {
				mCourse.prev = restorePrev;
			}
			mCourseChanged = true;	// retain info: user changed the course
			WaypointsChanged(kDontRefresh);	// recalc course since it may have been changed by host
		}
	}
}
#endif // GALACTICA_SERVER

void
CShip::UpdateHostFromStream(LStream *inStream) {
// this method is used to update the host from a packet sent by a client
// the client is not allowed to update most of the data, so many fields are
// ignored
	ANamedThingy::UpdateHostFromStream(inStream);
    UInt32 theSpeed = mSpeed;
    UInt32 thePower = mPower;
    UInt8 theClass = mClass;
	ReadShipStream(*inStream);		// read in everything
	mSpeed = theSpeed;   // restore stuff that only changes on the host
	mPower = thePower;
	mClass = theClass;
	WaypointsChanged(kDontRefresh);	// recalc course since it may have been changed by the client
	   // this will recalc the mRemDist and mMoveDelta values, so it doesn't matter if they are
	   // altered at the client
}

void
CShip::ReadShipStream(LStream &inStream) {
    DEBUG_OUT("ReadShipStream "<< this, DEBUG_DETAIL | DEBUG_EOT | DEBUG_PACKETS | DEBUG_MOVEMENT | DEBUG_CONTAINMENT);
	mUserCallIn = false;	// clear user calling no matter what
    SInt32 x, y, z;
	inStream >> mClass >> mSpeed >> mPower >> mRemDist >> x >> y >> z;
    mMoveDelta.x = x;
    mMoveDelta.y = y;
    mMoveDelta.z = z;
	inStream >> mCourse >> mCourseMem;	// read in the waypoints list for the current course
	mCourseChanged = false;	// starting new turn, no user changes yet
}

void
CShip::Die() {
	Departing(mCurrPos);	// dead, so no longer at current position, be it fleet or star
	ANamedThingy::Die();
}

void
CShip::ToggleScrap() {
    SetScrap(!IsToBeScrapped());
    Changed();
}

void
CShip::SetScrap(bool inScrap) {
    if ( (mNeedsDeletion == 0) && inScrap) {
        mNeedsDeletion = -1;  // -1 in needs deletion is flag for scrap
		DEBUG_OUT("SetScrap(true) for "<<this, DEBUG_DETAIL | DEBUG_USER | DEBUG_MOVEMENT);
    } else if ( (mNeedsDeletion == -1) && !inScrap) {
		mNeedsDeletion = 0;
		DEBUG_OUT("SetScrap(false) for "<<this, DEBUG_DETAIL | DEBUG_USER | DEBUG_MOVEMENT);
    }
}

// new method v1.2fc1, 9/19/99
// does the actual scrapping of a ship on the host
void
CShip::Scrapped() {
	// add 1/4 own cost to top container's production.
	AGalacticThingy* it = GetTopContainer();
	if (it->GetThingySubClassType() == thingyType_Star) {
		EShipClass theShipClass = GetShipClass();
		float buildCost = GetBuildCostOfShipClass(theShipClass, mTechLevel);
		CStar* theStar = (CStar*) it;
		theStar->AddToPaidTowardNextShip(buildCost / 4.0);
		theStar->Changed();
		DEBUG_OUT(this<<" was scrapped. Added "<<buildCost/4.0<<" toward next ship at "<<theStar, DEBUG_DETAIL | DEBUG_CONTAINMENT);
	} else {
		DEBUG_OUT(this<<" was scrapped but its top container is not a star. Top = "<<it, DEBUG_ERROR | DEBUG_CONTAINMENT);
	}
}

bool
CShip::Departing(Waypoint &inFrom) { //  Remove ourself from any star, fleet, or other type of container we were at
	DEBUG_OUT(this << " departing from "<< mCurrPos, DEBUG_DETAIL | DEBUG_MOVEMENT | DEBUG_CONTAINMENT);
	AGalacticThingy *oldContainer = inFrom.GetThingy();
	if (oldContainer) {
		DEBUG_OUT("Removing "<<this << " from "<<oldContainer, DEBUG_TRIVIA | DEBUG_CONTAINMENT);
		oldContainer->RemoveContent(this, contents_Ships, kDontRefresh);
    	if (GetShipClass() == class_Scout) {
    	    UInt16 containerTechLevel = oldContainer->GetTechLevel();
    	    if (containerTechLevel > mTechLevel) {
    		    SetTechLevel(containerTechLevel);	// scouts/couriers can pick up tech
    		    DEBUG_OUT(this << " picked up tech "<<mTechLevel<<" from "<<oldContainer, DEBUG_DETAIL | DEBUG_MOVEMENT);
    		    // еее scouts will fail to pick up tech if inside a fleet
    		} else {
    		    DEBUG_OUT(this << " not picking up lower tech "<< containerTechLevel << " from "<<oldContainer, DEBUG_TRIVIA | DEBUG_MOVEMENT);
    		}
    	}
	}
	inFrom = inFrom.GetCoordinates();	// since item departed location, it is now in space
	return true;
}


long
CShip::GetAttackStrength() const {
	return ((long)mPower * 50); // - (mDamage/8);
}

long
CShip::GetDefenseStrength() const {
	long power = mPower;
	if (power < 1) {
		power = 1;
	}
	return (power * 100) - mDamage;
}

long
CShip::GetOneAttackAgainst(AGalacticThingy* inDefender) {
	long attackDamage = AGalacticThingy::GetOneAttackAgainst(inDefender);
	if (IsDocked()) {
		if (IsOnPatrol()) {
			attackDamage = (attackDamage * 125) / 100;	// bonus for patroling ships
		}
	}
	if (attackDamage < 0) {
		attackDamage = 0;
	}
	return attackDamage;
}

long
CShip::TakeDamage(long inHowMuch) {
	if (IsDocked()) {
		if (IsOnPatrol()) {
			inHowMuch = (inHowMuch * 75) / 100;	// bonus for patroling ships
		}
	}
	return AGalacticThingy::TakeDamage(inHowMuch);
}

long
CShip::RepairDamage(long inHowMuch) {
#ifdef DEBUG
	if (inHowMuch < 0) {
		DEBUG_OUT("Negative damage ("<<inHowMuch<<") being repaired for "<<this, DEBUG_ERROR | DEBUG_COMBAT);
		inHowMuch = 0;
	}
#endif
	if (mDamage) {
		DEBUG_OUT("Repairing "<<this<<", which has "<<mDamage<<" damage.", DEBUG_DETAIL | DEBUG_COMBAT);
		if (inHowMuch < mDamage) {
			mDamage -= inHowMuch;
		} else {
			inHowMuch = mDamage;
			mDamage = 0;
		}
		DEBUG_OUT("Repaired "<< inHowMuch<<" damage to "<<this<<"; now has "<<mDamage<<" damage", DEBUG_DETAIL | DEBUG_COMBAT);
		Changed();
	} else {
		inHowMuch = 0;	// no damage repaired
	}
	return inHowMuch;	// return amount repaired
}


bool
CShip::Arriving(Waypoint &inTo) {
	DEBUG_OUT(this << " Arriving @ "<<inTo, DEBUG_DETAIL | DEBUG_MOVEMENT | DEBUG_COMBAT | DEBUG_CONTAINMENT);
	ASSERT(mGame->IsHost());
	bool success = true;
	ASSERT(!inTo.IsNull());
	GalacticaProcessor* gameProcessor = GetGameHost();
	ASSERT(gameProcessor);
	AGalacticThingy* navDst = inTo.GetThingy();
	AGalacticThingy* dst = nil;
	bool hasColony = false;
	if (navDst) {
    	DEBUG_OUT("ship::arriving, discover dst & owner", DEBUG_TRIVIA);
		dst = navDst->GetTopContainer();	// when we arrive at something, we actually arrive at its top container
    	DEBUG_OUT("Top container is "<< dst, DEBUG_TRIVIA);
		SInt32 dstOwner = dst->GetOwner();	// 1. Resolve various ownerships
			// 2. If combat, resolve it and create win/lose event(s)
		bool sendDefendersLost = false;
    	DEBUG_OUT("ship::arriving, combat check", DEBUG_TRIVIA);
		if (!dst->IsAllyOf(mOwnedBy)) { // non-allied means combat
			SInt16 attacker = mOwnedBy;
			SInt16 defender = 0;
			mCurrPos = dst;	// destroyed items will be reported at final dest, not coord nearby
				// A. Make list of all attackers and defenders and their reserves
			DEBUG_OUT("Doing Combat for " << this, DEBUG_IMPORTANT | DEBUG_COMBAT | DEBUG_MOVEMENT);
		#warning rewrite to use std::vector
			LArray attackers, defenders, attackersReserves, defendersReserves, attackersFleets, defendersFleets;
			AddEntireContentsToList(contents_Ships, attackers);		// get all the attackers in a list
			dst->AddEntireContentsToList(contents_Ships, defenders);	// get all the defenders in another list
			defender = RemoveAllies(&defenders, mOwnedBy);		// remove any of the attackers allies from the defenders list	
			SortOutReserves(&attackers, &attackersReserves, &attackersFleets); // find all the scouts and colony ships and put them on reserve
			SortOutReserves(&defenders, &defendersReserves, &defendersFleets);
			long defendersCount = defenders.GetCount() + defendersReserves.GetCount();
				// now that all the attackers and defenders are organized, we can do the battle-loop
			bool done = false;
			int count = 100;
			if (defendersCount > 0) {
				while (!done && (count-- > 0)) {	// keep going until someone loses: attackers attack first, then defenders attack
					bool done1, done2;
					DEBUG_OUT("Start Battle Loop, Iteration " << (100 - count), DEBUG_DETAIL | DEBUG_COMBAT);
					done1 = AttackLoop(&attackers, &attackersReserves, &defenders, &defendersReserves);
					done2 = AttackLoop(&defenders, &defendersReserves, &attackers, &attackersReserves);
					done = (done1 || done2);
					RemoveDead(&attackers);			// after the attacks, we dump those that died from the list
					RemoveDead(&defenders);			// we don't do it during combat, because we want every combatant
					RemoveDead(&attackersReserves);	// to have a chance to fire at least one shot
					RemoveDead(&defendersReserves);
				}
			} else {
				DEBUG_OUT(dst <<" is undefended, skipping space battle", DEBUG_DETAIL | DEBUG_COMBAT); 
			}
			
			if (((attackers.GetCount()+attackersReserves.GetCount()) > 0) 	// still attackers left?
				&& (dst->GetThingySubClassType() == thingyType_Star)	      // & defender is a star?
				&& dst->IsOwned() && !dst->IsAllyOf(attacker) ) {		      // & not allied with attacker?
				// defender is a star, so the population should now fight to resist the attackers
				CStar* dstStar = (CStar*)dst;
				UInt32 startPopulation = dstStar->GetPopulation();
				defendersReserves.InsertItemsAt(1, LArray::index_Last, &dst);
				done = false;
				count = 100;
				while (!done && (count-- > 0)) {	// keep going until someone loses: attackers attack first, then defenders attack
					DEBUG_OUT("Start Ground Attack Loop, Iteration " << (100 - count), DEBUG_DETAIL | DEBUG_COMBAT);
					done = AttackLoop(&attackers, &attackersReserves, &defenders, &defendersReserves);
					done = done || AttackLoop(&defenders, &defendersReserves, &attackers, &attackersReserves);
					RemoveDead(&attackers);			// after the attacks, we dump those that died from the list
					RemoveDead(&defenders);			// we don't do it during combat, because we want every combatant
					RemoveDead(&attackersReserves);	// to have a chance to fire at least one shot
					RemoveDead(&defendersReserves);
					if (dstStar->GetPopulation() <= 0) {	// special check for no population left
						DEBUG_OUT("Population of "<<dst<<" went to zero, making it neutral", DEBUG_DETAIL | DEBUG_COMBAT);
						sendDefendersLost = true;
						dst->SetOwner(0);			// colony now unpopulated, and thus unowned
						dstStar->SetNewShipDestination(gNullPt3D);  // remove the new ship destination
						done = true;
					}
				}
				// the "dead" were the population that were defeated. Now we need
				// to calculate the actual dead, which will be 10% of the defeated population.
				UInt32 popDiff = startPopulation - dstStar->GetPopulation();
				UInt32 endPopulation = startPopulation - ((popDiff/10) + 1);
				dstStar->SetPopulation(endPopulation);
			}
			long attackersInFleetLosses = FinalizeLosses(&attackersFleets, defender);
			long defendersInFleetLosses = FinalizeLosses(&defendersFleets, attacker);
			// this number for defendersInFleetLosses is not very useful, because the
			// defenders could easily have ships parked individually at a star, which 
			// would not be included in the count of ships destroyed. It works fine
			// though if the dst is a fleet

			DEBUG_OUT("Finished Combat for " << this, DEBUG_DETAIL | DEBUG_COMBAT);

			// the basic idea here is to be sure that people get appropriate messages to tell them
			// what they won or lost.
			if (((attackers.GetCount()+attackersReserves.GetCount()) <= 0)) {
				success = false;	// defenders won the battle, we lost
				if (dst->GetThingySubClassType() == thingyType_Star) {
					// determine how many ships were lost from the star
					// so we can tell the defenders (the winners) how many ships they 
					// lost in the battle and tell the attackers (the losers)
					// that they managed to destroy X number of enemy ships.
					long defendersRemaining = defenders.GetCount() + defendersReserves.GetCount();
					long defendersLost = defendersCount - defendersRemaining;
					if ( defendersLost > 0) {
						DEBUG_OUT("Sending star damage for "<<dst, DEBUG_DETAIL | DEBUG_COMBAT | DEBUG_MESSAGE);
						CMessage::CreateEvent(GetGameHost(), event_PlanetDamaged, attacker, dst, kAllPlayers,
											defender, attacker, defendersLost);
					
					}
				} else if (defendersInFleetLosses > 1) {	// dst is not a star
					// The defenders (the winners) will see this message to let them know
					// how many ships they lost in the battle. The attackers (the losers)
					// will see this message as a kind of consolation prize -- it tells
					// them that they did manage to destroy X number of enemy ships.
					DEBUG_OUT("Sending fleet damage (1)", DEBUG_DETAIL | DEBUG_COMBAT | DEBUG_MESSAGE);
					CMessage::CreateEvent(GetGameHost(), event_FleetDamaged, attacker, dst, kAllPlayers,
											defender, attacker, defendersInFleetLosses);
				}
			} else {
				if (attackersInFleetLosses > 1) {	// we won the battle, defenders lost
					// The attackers (the winners) will see this message to let them know
					// how many ships they lost in the battle. The defenders (the losers)
					// will see this message as a kind of consolation prize -- it tells
					// them that they did manage to destroy X number of enemy ships.
					DEBUG_OUT("Sending fleet damage (2)", DEBUG_DETAIL | DEBUG_COMBAT | DEBUG_MESSAGE);
					CMessage::CreateEvent(GetGameHost(), event_FleetDamaged, attacker, this, kAllPlayers,
											attacker, defender, attackersInFleetLosses);
				}
			}

			hasColony = HasShipClass(&attackersReserves, class_Colony);	// see if we have any attacking colony ships left
			
			if (GetThingySubClassType() == thingyType_Fleet) {	// fleets need to recalc now since their stats may have
				static_cast<CFleet*>(this)->CalcFleetInfo();	// changed with the loss of their ships
			}
		}
			// 3. Check for changed ownership through combat
    	DEBUG_OUT("ship::arriving, ownership change check", DEBUG_TRIVIA);
		if (sendDefendersLost) {
			DEBUG_OUT("Sending defenders lost for "<<dst, DEBUG_DETAIL | DEBUG_COMBAT | DEBUG_MESSAGE);
			CMessage::CreateEvent(GetGameHost(), event_DefendersLost, mOwnedBy, dst, dstOwner); // defeated the population				
		}
		SInt32 dstNewOwner = dst->GetOwner();
			// 4. for colony ships arriving at stars, try to colonize
    	DEBUG_OUT("ship::arriving, colony ship check", DEBUG_TRIVIA);
		if (hasColony && (dst->GetThingySubClassType() == thingyType_Star)) {
			if (dstOwner != dstNewOwner) {	// ownership changed (owner lost the planet)
				DEBUG_OUT("Sending colony lost for "<<dst, DEBUG_DETAIL | DEBUG_COMBAT | DEBUG_MESSAGE);
				CMessage::CreateEvent(GetGameHost(), event_ColonyCaptured, mOwnedBy, dst, kAllPlayers, mOwnedBy, dstOwner);
			}
			if (!dstNewOwner) {		// item is now unowned, colonize it
				dstNewOwner = mOwnedBy;
				dst->SetOwner(dstNewOwner);
				if (!dstOwner) {	// item was originally unowned, so treat it as a new colony
	   				// adjust the population 
	   				CStar *theStar = (CStar*) dst;
	   				UInt32 population = theStar->GetPopulation() + 1;
	   				if (population < 1) {
	   					population = 1;
	   				}
	   				// v3.0, for human players, use the default new star system settings
	   				if (gameProcessor->PlayerIsHuman(dstNewOwner)) {
	   					PrivatePlayerInfoT privateInfo;
	   					gameProcessor->GetPrivatePlayerInfo(dstNewOwner, privateInfo);
	   					theStar->SetBuildHullType(privateInfo.defaultBuildType - 1);
	   					theStar->GetSettingPtr(spending_Growth)->desired = privateInfo.defaultGrowth;
	   					theStar->GetSettingPtr(spending_Ships)->desired = privateInfo.defaultShips;
	   					theStar->GetSettingPtr(spending_Research)->desired = privateInfo.defaultTech;
	   				} else {
						theStar->SetBuildHullType(class_Satellite);
					}
					theStar->SetPopulation(population); // 1000 colonists/administrators
					DEBUG_OUT("Sending colonized new star for "<<theStar, DEBUG_DETAIL | DEBUG_MESSAGE);
					CMessage::CreateEvent(GetGameHost(), event_NewColony, mOwnedBy, theStar, kAllPlayers); // send a notice to the colonizer
					DEBUG_OUT("Sending adjust settings for "<<theStar, DEBUG_DETAIL | DEBUG_MESSAGE);
					CMessage::CreateEvent(GetGameHost(), event_RenameColony, mOwnedBy, theStar); // notify player they need to rename colony
				}
			}
		}
			// 5. If ship's owner is destination's ally: transfer tech library, etc..
    	DEBUG_OUT("ship::arriving, transfer tech check", DEBUG_TRIVIA);
		if (IsAllyOf(dstNewOwner)) {
			int	shipTechLevel = GetTechLevel();
			if (dst->GetThingySubClassType() == thingyType_Star) {	// make sure this is a star we are dealing with
				// ееее transfer tech library, may change later ееее
				if (dst->GetTechLevel() < shipTechLevel) {
					dst->SetTechLevel(shipTechLevel);
					dst->SetScannerRange(UThingyClassInfo::GetScanRangeForTechLevel(shipTechLevel, mGame->IsFastGame(), mGame->HasFogOfWar()));
					dst->ScannerChanged();  // changed tech level, also changes scanner range
                    // changing the tech level doesn't always update the star's frame correctly
                    // unless we call Calc Position
                  #ifndef GALACTICA_SERVER
                    if (GetGameDoc()) { // only do this on graphical versions
                        static_cast<CStar*>(dst)->CalcPosition(kRefresh);
                    }
                  #endif // GALACTICA_SERVER
					DEBUG_OUT(this << " gave tech "<<mTechLevel<<" to "<<dst, DEBUG_DETAIL | DEBUG_MOVEMENT);
				} else if (GetShipClass() == class_Scout) {
					SetTechLevel(dst->GetTechLevel());	// scouts/couriers can pick up tech
					DEBUG_OUT(this << " picked up tech "<<mTechLevel<<" from "<<dst, DEBUG_DETAIL | DEBUG_MOVEMENT);
					 // еее scouts will fail to pick up tech if inside a fleet
				}
				static_cast<CStar*>(dst)->SetBuildHullType(static_cast<CStar*>(dst)->GetBuildHullType());	// adjust for tech level.
			}
		}
			// 6. If destination is one of our own ships, or a rendezvous point, make a fleet, unless
			//    the destination is a dead ship
    	DEBUG_OUT("ship::arriving, make fleet check", DEBUG_TRIVIA);
		long dstType = navDst->GetThingySubClassType();
		if ((dstOwner == mOwnedBy) && !navDst->IsDead() // don't make fleet when arriving at dead thing
		  && ((dstType == thingyType_Ship) || (dstType == thingyType_Rendezvous))) {
			CFleet* dstFleet = nil;
			Waypoint wp = navDst->GetPosition();
        	DEBUG_OUT("ship::arriving:fleet, reuse fleet check ", DEBUG_TRIVIA);
			if (wp.GetType() == wp_Fleet) {				// if navdst item is already in fleet, use
                DEBUG_OUT("ship::arriving:fleet:reuse, use fleet, wp = " << wp, DEBUG_TRIVIA);
				dstFleet = (CFleet*)wp.GetThingy();		// that fleet instead of creating new one
			} else if (dstType == thingyType_Rendezvous) {
                DEBUG_OUT("ship::arriving:fleet:reuse, use rendezvous, navDst = " << navDst, DEBUG_TRIVIA);
				dstFleet = ((CRendezvous*)navDst)->GetFleet();	// get fleet that is meeting there
                DEBUG_OUT("ship::arriving:fleet:reuse, use rendezvous, dstFleet = {" << (void*)dstFleet <<"}", DEBUG_TRIVIA);
				if (dstFleet && dstFleet->IsDead()) {	// fleet might have just been killed, and we			
					dstFleet = nil;			// can't add ourselves to a dead fleet
				}
			}
        	DEBUG_OUT("ship::arriving:fleet, fleet -> rendezvous check ", DEBUG_TRIVIA);
			if (!dstFleet && (GetThingySubClassType() == thingyType_Fleet)) {	// Look for fleets moving to
				((CRendezvous*)navDst)->SetFleet((CFleet*)this);	// rendezvous pts that have no fleet
				navDst->Changed();							// and move them to coordinates 
				dstFleet = (CFleet*)this;					// & make them new fleet for rend. pt
			}
        	DEBUG_OUT("ship::arriving:fleet, create fleet check ", DEBUG_TRIVIA);
			if (!dstFleet) {		// no existing fleet to add to, have to create a new one.
				bool createEvent = false;
				dstFleet = (CFleet*) mGame->MakeThingy(thingyType_Fleet); //new CFleet(mSuperView, mGame);
				dstFleet->AssignUniqueID();
				dstFleet->SetOwner(mOwnedBy);
				dstFleet->SetPosition(wp, kDontRefresh);	// put the new fleet where the dest item was and
				if (dstType == thingyType_Ship) {	// we were headed to a ship, but we made it into a fleet
					dstFleet->SetCourse(((CShip*)navDst)->GetCourse());	 // give the new fleet the same course 
					dstFleet->SetCourseMem(((CShip*)navDst)->GetCourseMem());  // as the original dest ship
					dstFleet->AddContent(navDst, contents_Ships, kDontRefresh);// automatically clears course
				} else if (dstType == thingyType_Rendezvous) {	// when headed to rendezvous point and had to create
					((CRendezvous*)navDst)->SetFleet(dstFleet);	// fleet, save it in rendezvous pt.
					navDst->Changed();
					createEvent = true;
				}
				dstFleet->Persist();				// fleet saves itself into database
				if (createEvent) {
					DEBUG_OUT("Sending created fleet for "<<dstFleet, DEBUG_DETAIL | DEBUG_MESSAGE);
					CMessage::CreateEvent(GetGameHost(), event_NewFleet, mOwnedBy, dstFleet, kAllPlayers); // notify player
				}
			}
        	DEBUG_OUT("ship::arriving:fleet, recursive add fleet check ", DEBUG_TRIVIA);
			if (dstFleet == this) {				// when moving a fleet to a rendezvous point,
				inTo = navDst->GetPosition();	// put the fleet at the rendezvous point's location
				navDst = nil;					// coordinate-only destination
				dst = nil;
			} else {				// put this ship into the fleet
				navDst = dstFleet;	// treat the fleet as the destination from now on
			}
		}
		
			// 7. add to navDst container, unless:
			//	    a. navDst is nil
			//		b. navDst is owned by an enemy
			//		c. navDst cannot hold ships
			//	in which case we try dst (top level container)
			//  if dst is valid, add to dst container, unless:
			//		a. navDst is owned by an enemy
			//		b. navDst cannot hold ships
			//	in which case we add to inTo's (navDst's) coordinates
    	DEBUG_OUT("ship::arriving, put at destination", DEBUG_TRIVIA);
		if (success) {
			if (navDst) {
				if (navDst->SupportsContentsOfType(contents_Ships) && !navDst->IsDead()
				  && (!navDst->IsOwned() || navDst->IsAllyOf(mOwnedBy)) ) {
				    DEBUG_OUT("NavDst accepted "<<navDst, DEBUG_TRIVIA);
					dst = nil;	// navDst is living allied container, skip check of dst
				} else {
				    DEBUG_OUT("NavDst not accepted "<<navDst, DEBUG_TRIVIA);
					navDst = nil;	// navDst doesn't do it for us
				}
			}
			if (dst) {	// navDst wasn't a living allied container, check whatever navDst is in
				DEBUG_OUT("Checking top container "<<dst, DEBUG_TRIVIA);
				navDst = dst;
				if (navDst->IsDead()) {
				    DEBUG_OUT("Top container is dead "<<navDst, DEBUG_DETAIL);
					navDst = nil;	// don't want to put ourselves into a dead container
				} else if (!navDst->SupportsContentsOfType(contents_Ships)) {
					DEBUG_OUT("Container that holds a thingy but not a ship? in v1.2?", DEBUG_ERROR);
					navDst = nil;	// the original navDst's container can't contain ships
				} else if (!(!navDst->IsOwned() || navDst->IsAllyOf(mOwnedBy)) ) {
				    DEBUG_OUT("Top container is not acceptable "<<navDst, DEBUG_DETAIL);
					ASSERT_STR(navDst->GetThingySubClassType() != thingyType_Star, this
						<<" will probably be hidden behind "<<navDst);	
					// make sure call-in gets set in the calling method when this method returns
					navDst = nil;	// the original navDst's container is an enemy
				}					// so we can't add ourselves to that either
			}
			if (navDst) {
				inTo = navDst;				// living allied container found, put self into it
				success = navDst->AddContent(this, contents_Ships, kDontRefresh);
			} else {
				// since the calling function makes us call in if we have no further course
				// plotted and we aren't inside a fleet, we should be in good shape even if these
				// coordinates are behind a star
				inTo = inTo.GetCoordinates();	// no appropriate container, go to point in space
				DEBUG_OUT("Leaving ship at coordinates " << inTo, DEBUG_DETAIL);
			}
		}
	}
    DEBUG_OUT("ship::arriving, final success check", DEBUG_TRIVIA);
	if (success) {
		if (inTo.GetCoordinates() == gNullPt3D) {
			DEBUG_OUT("Moving to (0,0) " << this, DEBUG_ERROR | DEBUG_MOVEMENT);
		} else {
			mCurrPos = inTo;
		}
	}
    DEBUG_OUT("ship::arriving, done", DEBUG_TRIVIA);
	return success;
}

bool
CShip::IsOfContentType(EContentTypes inType) const {
	if (inType == contents_Ships) {
		return true;
	} else {
		return ANamedThingy::IsOfContentType(inType);
	}
}

bool
CShip::SupportsContentsOfType(EContentTypes inType) const {
	if (inType == contents_Course) {
		return true;
	} else {
		return false;
	}
}

void*
CShip::GetContentsListForType(EContentTypes inType) const {
	if (inType == contents_Course) {
		return const_cast<void*>(static_cast<const void*>(&GetWaypointsList()));
	} else {
		return NULL;
	}
}

SInt32
CShip::GetContentsCountForType(EContentTypes inType) const {
	if (inType == contents_Course) {
		return GetWaypointCount();
	} else {
		return 0;
	}
}


ResIDT
CShip::GetViewLayrResIDOffset() {
	ResIDT offset = GetShipClass();
	// adjust for maximum number of specific ship picts available
	if (offset > NUM_SHIP_HULL_TYPES + MAX_BATTLESHIP_PICTS) {
	    offset = NUM_SHIP_HULL_TYPES + MAX_BATTLESHIP_PICTS;
	}
  #ifndef GALACTICA_SERVER
	long owner = GetGameDoc()->GetColorIndex(GetOwner(), false) - 1;
	int myPlayerNum = GetGameDoc()->GetMyPlayerNum();
	if (!IsAllyOf(myPlayerNum) && (GetVisibilityTo(myPlayerNum) < visibility_Class)) {
	    return LayrID_UnknownTypeOffset + (owner * LayrID_PlayerOffset); // Fog of War
	}
	if (owner < MAX_OWNER_BACKDROPS) {
		offset += (owner * LayrID_PlayerOffset);
	}
  #endif // GALACTICA_SERVER
	return offset;
}


void
CShip::PrepareForEndOfTurn(long inTurnNum) {
    mHasMoved = false;   // clear the movement flag, so that we know this ship hasn't moved
                         // during this turn processing cycle
    AGalacticThingy::PrepareForEndOfTurn(inTurnNum);
}

void
CShip::EndOfTurn(long inTurnNum) {
   if (mHasMoved) {  // don't move twice
      return;
   }
   mHasMoved = true; // set the movement flag
	if (IsToBeScrapped()) {
		DEBUG_OUT("Killing Scrapped "<< this, DEBUG_TRIVIA | DEBUG_EOT | DEBUG_OBJECTS);
		Scrapped();
		Die();
	}
	// ship may have died in section above
	if ( !IsDead() && !CoursePlotted() ) {	// v1.2b6, fix "peaceful co-existance" bug
		AGalacticThingy *at = mCurrPos.GetThingy();
		if (at) {
			Waypoint wp = mCurrPos;
			if (!at->IsAllyOf(mOwnedBy)) {	// are we at an enemy thingy (fleet or star)
				DEBUG_OUT(this<<" at enemy "<<at<<"; continuing attack", DEBUG_DETAIL | DEBUG_COMBAT | DEBUG_MOVEMENT);
				if (Departing(mCurrPos)) {	// depart and re-arrive to re-initiate the combat process
					Arriving(wp);
				}
			} else if (GetShipClass() == class_Scout) {	// v1.2b6, fix tech refresh bug, 3/1/98
				DEBUG_OUT(this<<" at own "<<at<<"; refreshing tech", DEBUG_DETAIL | DEBUG_MOVEMENT);
				if (Departing(mCurrPos)) {	// depart and re-arrive to re-initiate the tech transfer process
					Arriving(wp);
				}
			}
		}
	}
	// ship may have died in section above
	if ( !IsDead() && CoursePlotted() ) {
		bool isComputerShip = mGame->PlayerIsComputer(mOwnedBy);	// v1.2b9, don't do callin for computer's ships
		if ( Departing(mCurrPos) ) {
			Waypoint wp = GetDestination();
			EWaypointType wpType = wp;
			if (wpType == wp_None) {
				DEBUG_OUT(this<<" headed to nil waypoint", DEBUG_ERROR | DEBUG_MOVEMENT);
			}
			// make sure that thing we are pursuing can move first
            AGalacticThingy* navDst = wp.GetThingy();
            bool letDestMoveFirst = false;
            if (navDst) {
                CShip* dstShip;
                // first check the real destination, to see if it is going to move
                if (navDst->CanFleePursuit()) {
                    dstShip = ValidateShip(navDst);
                    if (dstShip->CoursePlotted() && !dstShip->HasMoved()) {
                       letDestMoveFirst = true;
                    } 
                }
                if (!letDestMoveFirst) {   // the targeted object isn't moving, check it's top container
                    navDst = navDst->GetTopContainer();	// when we arrive at something, we actually arrive at its top container
                    if (navDst->CanFleePursuit()) {
                        dstShip = ValidateShip(navDst);
                        if (dstShip->CoursePlotted() && !dstShip->HasMoved()) {
                           letDestMoveFirst = true;
                        } 
                    }
                }
            }
            if (letDestMoveFirst) {
                // we needed to let our target destination object move first
                navDst->EndOfTurn(inTurnNum);
            }
			CalcMoveDelta();  // v2.1b11r6, always recalculate distance
			if (mRemDist > mSpeed) { // won't reach there this turn, so keep moving
				mCurrPos += mMoveDelta;
			} else {	// we will arrive this turn, so just put us there
				if ( Arriving(wp) ) {	// update current position if needed
					if (IsDead()) {
						return;
					}
				    DEBUG_OUT(this<<" updating course", DEBUG_TRIVIA | DEBUG_MOVEMENT);
					int numWaypoints = GetWaypointCount();	// now start on next leg of course
					mCourse.prev = mCourse.curr;
					mCourse.curr++;
					if (mCourse.curr > numWaypoints) {	// have we reached the end of our course?
                        DEBUG_OUT(this<<" reached end of course", DEBUG_TRIVIA | DEBUG_MOVEMENT);
						if (CourseLoops() && (numWaypoints > 1)) {
							mCourse.curr = 1;
						} else {
                            DEBUG_OUT(this<<" clearing course", DEBUG_TRIVIA | DEBUG_MOVEMENT);
							ClearCourse(false, kDontRefresh);	// course complete, remove it
							SetPatrol(false);	// no longer on patrol
							if (!isComputerShip && (mCurrPos.GetType() != wp_Fleet) && !mCurrPos.IsDebris()) {
                                DEBUG_OUT(this<<" setting auto calling", DEBUG_TRIVIA | DEBUG_MOVEMENT | DEBUG_MESSAGE | DEBUG_USER);
								SetAutoCallIn();	// when non-looping course completes, and not in a fleet, call in
							}
						}
					}
			        DEBUG_OUT(this<<" done updating course", DEBUG_TRIVIA | DEBUG_MOVEMENT);
				} else if (IsDead()) {	// v1.2b11d3, don't do further calculations if ship was
	                DEBUG_OUT(this<<" is dead, EOT done", DEBUG_TRIVIA | DEBUG_MOVEMENT | DEBUG_COMBAT);
					return;				// destroyed while attempting to arrive.
				} else if (!isComputerShip) {
                    DEBUG_OUT(this<<" setting auto calling", DEBUG_TRIVIA | DEBUG_MOVEMENT | DEBUG_MESSAGE | DEBUG_USER);
					SetAutoCallIn();	// ship tried to arrive somewhere but couldn't, call in
				}
			}
            DEBUG_OUT(this<<" calculating next move", DEBUG_TRIVIA | DEBUG_MOVEMENT);
			CalcMoveDelta();	//figure out how we are going to move next time
			ThingMoved(kDontRefresh);
			FlagForRescan(); // make sure this item is checked for in scanner range, because it moved
            if (HasScanners()) {
                // if this item has scanners, make sure we recheck what it can see, because it moved
                ScannerChanged();
            }
		} else if (!isComputerShip) {
			SetAutoCallIn();		// ship has course laid in but cannot move, call in
		}
	}
    DEBUG_OUT(this<<" done with EOT", DEBUG_TRIVIA | DEBUG_EOT);
// v1.2b11d3, removed this unnecessary call to an empty function
//	AGalacticThingy::EndOfTurn(inTurnNum);
}

void
CShip::FinishEndOfTurn(long) { //inTurnNum
    DEBUG_OUT("Finish EOT "<<this, DEBUG_TRIVIA);
	bool isComputerShip = mGame->PlayerIsComputer(mOwnedBy);	// v1.2b9, don't do callin for computer's ships
	// we call CalcMoveDelta for all the ships that have a course plotted
	// to make sure that if their target has moved or been redrawn the course vector will be
	// drawn correctly. This has no impact on the actual behavior of the ships -- it's just
	// to make sure the user sees what the ship will actually do at the next EOT
	if (CoursePlotted()) {
		mWantsAttention = false;	// don't want attention while they are moving
		CalcMoveDelta();	// update the course info
	} else if (!isComputerShip && IsInSpace() && !IsDead()) {	// ships or fleets with no course sitting in space should callin
	    GalacticaProcessor* host = dynamic_cast<GalacticaProcessor*>(mGame);
	    if (!host) {
	        DEBUG_OUT("Calling Finish EOT for "<<this<<" from client", DEBUG_ERROR);
	        return;
	    }
		// but only if more than enough turns have passed
		if ( (mLastCallin + host->GetTurnsBetweenCallins()) <= host->GetTurnNum()) {
			mWantsAttention = true;	// should call in
			mLastCallin = host->GetTurnNum();
			mChanged = true;
		}
	}
	if (mUserCallIn) {
		mWantsAttention = true;
		//	mUserCallIn = false;	if the user set the callin, leave it there. They can cancel it themselves when
		//  they are done with it
	}
  #ifndef GALACTICA_SERVER
	CalcPosition(kDontRefresh);			// make sure position is correct
  #endif // GALACTICA_SERVER
	if (mWantsAttention && mOwnedBy && !isComputerShip && !IsDead()) {	// send an event for all the ships
		EEventCode code; 									// that are calling in and not dead.
		if (SupportsContentsOfType(contents_Ships)) {
			code = event_FleetCallin;
		} else {
			code = event_ShipCallin;
		}
		mWantsAttention = false;	// 1.2b10, stop unnecessary callins
		mChanged = true;
		CMessage::CreateEvent(GetGameHost(), code, mOwnedBy, this);
	}
}

long
CShip::GetDangerLevel() const {
	return 0;	// default returns no danger level
}

void
CShip::SetCourse(CourseT* inCourse) {
	inCourse->Duplicate(&mCourse);
	WaypointsChanged(kDontRefresh);
}

void
CShip::SetCourseMem(CourseT* inCourse) {
	inCourse->Duplicate(&mCourseMem);
	Changed();
}

// waypoint list is 1 based
void
CShip::AddWaypoint(int inWaypointNum, Waypoint inWaypoint, bool inRefresh) {
    --inWaypointNum;    // waypoint list is 1 based
	if (inWaypoint.GetThingy() == this) {	// make sure we never go to ourself
		inWaypoint = mCurrPos;				// goto the position we are at instead
	}
    std::vector<Waypoint>::iterator p = mCourse.waypoints.begin() + inWaypointNum + 1;
    if (inWaypointNum >= mCourse.waypoints.size()) {
        p = mCourse.waypoints.end();
    }
	mCourse.waypoints.insert(p, inWaypoint);
	WaypointsChanged(inRefresh);
}

// PRIVATE
// Low level: no bounds checking, no refresh, no add if past end,
// no recalc of course, no check for going to self. Just sets it.
// Waypoint list is 1 based
void
CShip::SetWaypoint(int inWaypointNum, Waypoint inWaypoint) {
    --inWaypointNum;    // waypoint list is 1 based
    mCourse.waypoints[inWaypointNum] = inWaypoint;
}
 
// waypoint list is 1 based
void
CShip::SetWaypoint(int inWaypointNum, Waypoint inWaypoint, bool inRefresh) {
	if (!CoursePlotted()) {
		AddWaypoint(kEndOfWaypointList, inWaypoint, inRefresh);
	} else {
        if ( (inWaypointNum > mCourse.waypoints.size()) || (inWaypointNum < 1) ) {
			AddWaypoint(kEndOfWaypointList, inWaypoint, inRefresh);
        } else {
			if (inWaypoint.GetThingy() == this)	{	// make sure we never go to ourself
				inWaypoint = mCurrPos;				// goto the position we are at instead
			}
			SetWaypoint(inWaypointNum, inWaypoint);
			WaypointsChanged(inRefresh);
		}
	}
}

// waypoint list is 1 based
void
CShip::DeleteWaypoint(int inWaypointNum, bool inRefresh) {
  #ifndef GALACTICA_SERVER
    if (inRefresh) {
	    Refresh();
	}
  #endif
    --inWaypointNum;    // waypoint list is 1 based
    if (inWaypointNum < mCourse.waypoints.size()) {
        mCourse.waypoints.erase(mCourse.waypoints.begin()+inWaypointNum);
    	if (mCourse.waypoints.size() == 0) {
    		ClearCourse(true, inRefresh);
    	} else {
    		WaypointsChanged(inRefresh);
    	}
	}
}

// waypoint list is 1 based
// waypoint 0 is current position
Waypoint
CShip::GetWaypoint(int inWaypointNum) const {
	if (inWaypointNum == 0) {
	    return mCurrPos;
	}
    --inWaypointNum;    // waypoint list is 1 based
    if (inWaypointNum < mCourse.waypoints.size()) {
		return mCourse.waypoints[inWaypointNum];
	} else {
	    return mCurrPos;
	}
}

Waypoint
CShip::GetDepartedFrom() const {
    return GetWaypoint(mCourse.prev);
}

Waypoint
CShip::GetDestination() const {
    return GetWaypoint(mCourse.curr);
}

void
CShip::ClearCourse(bool inRecalcMove, bool inRefresh) {
	if (CoursePlotted()) {
		mCourse.waypoints.erase(mCourse.waypoints.begin(), mCourse.waypoints.end());
	}
	mCourse.loop = false;
	mCourse.hunt = false;
	mCourse.curr = 0;
	mCourse.prev = 0;
	mSelectedWPNum = 0;
	if (inRecalcMove) {
		WaypointsChanged(inRefresh);
	}
}

void
CShip::SwapCourseWithCourseMem() {
	mSelectedWPNum = 0;		// the selection will be cleared whenever the course is swapped
	if (CourseInMem()) {		// if there is a course in memory, swap the two
		CourseT tempCourse = mCourse;
		mCourse = mCourseMem;
		mCourseMem = tempCourse;
//		tempCourse.waypoints = nil;	// so when the destructor is called it won't delete our list
		WaypointsChanged(kRefresh);
	} else { // if there is no course in memory, just put the current course there
		mCourse.Duplicate(&mCourseMem);
		mCourseMem.patrol = false;		// don't save patrol status
		Changed();		// the current course didn't change, so we don't call WaypointsChanged()
	}
}


void
CShip::CalcMoveDelta() {	//determine new movement delta for ship to reach its current destination
    DEBUG_OUT(this<<" starting CalcMoveDelta", DEBUG_TRIVIA | DEBUG_MOVEMENT);
	Point3D destCoord;
	float dist;
	bool calcDist = false;
	if (CoursePlotted()) {	
    	DEBUG_OUT(this<<" recalcing course", DEBUG_TRIVIA | DEBUG_MOVEMENT);
		calcDist = true;	// we should have to calc our course
		int count = GetWaypointCount();
		if (mCourse.curr <= 0) {	// have a new course, make sure we are on it
			mCourse.curr = 1;
		} else if (mCourse.curr > count ) {
			if (CourseLoops()) {
				mCourse.curr = 1;
			} else {
				calcDist = false;	// no course to calc, we have passed it all
			}
		}
		Waypoint wp;
		Waypoint lastWP;
		bool	calcRemainingDist = true;
		for (int i = 1; i <= count; i++) {
			if (i == 1) {
				if (CourseLoops()) {
					lastWP = GetWaypoint(count);
				} else if (mCourse.curr == 1) {
					lastWP = mCurrPos;
					calcRemainingDist = false;
				} else {
					lastWP = GetWaypoint(1);
				}
			} else {
				lastWP = wp;
			}
			wp = GetWaypoint(i);
			long dist = wp.Distance(lastWP);
			wp.SetRemainingTurnDistance(0);
			if (mSpeed) {
				wp.SetTurnDistance((dist/mSpeed)+1);
				if (calcRemainingDist && (mCourse.curr == i)) {	
						// if this is the current course leg, calc the distance
					dist = mCurrPos.Distance(wp);	// remaining on it as well
					wp.SetRemainingTurnDistance((dist/mSpeed)+1);
				}
			}
			SetWaypoint(i, wp);
		}
    	DEBUG_OUT(this<<" done recalcing course", DEBUG_TRIVIA | DEBUG_MOVEMENT);
	}
	if (calcDist) {	// have course, and thus movement
    	DEBUG_OUT(this<<" will keep moving", DEBUG_TRIVIA | DEBUG_MOVEMENT);
		destCoord = GetDestination();
		dist = mCurrPos.Distance(destCoord);
		Point3D delta;
		delta.x = destCoord.x - mCurrPos.GetCoordinates().x;
		delta.y = destCoord.y - mCurrPos.GetCoordinates().y;
//		delta.z = destCoord.z - mCurrPos.GetCoordinates().z;
		float rx = (float)(delta.x / dist);	// portion of distance that comes from x axis movement
		float ry = (float)(delta.y / dist);	// y axis
//		float rz = (float) delta.z / dist;	// z axis
		mMoveDelta.x = (long)(mSpeed * rx);	// can apply that x portion of our speed to x axis movement
		mMoveDelta.y = (long)(mSpeed * ry);	// y axis
//		mMoveDelta.z = (long)(mSpeed * rz); // z axis
	} else {	// no course, no movement
    	DEBUG_OUT(this<<" will do no further movement", DEBUG_TRIVIA | DEBUG_MOVEMENT);
		mMoveDelta = gNullPt3D;
		destCoord = mCurrPos;
		dist = 0;
	}
	mRemDist = (UInt32)dist;
   DEBUG_OUT(this<<" exiting CalcMoveDelta", DEBUG_TRIVIA | DEBUG_MOVEMENT);
}

// see if the ship refers to the thingy in question in any way
bool
CShip::RefersTo(AGalacticThingy *inThingy) {
	ASSERT(inThingy != nil);
	bool refersToIt = false;
	int count = GetWaypointCount();
	Waypoint wp;
	for (int i = 1; i <= count; i++) {
		wp = GetWaypoint(i);
		refersToIt = (inThingy == wp);
		if (refersToIt)
			break;	// quit checking if we find a single reference
	}
	if (!refersToIt) {
		refersToIt = ANamedThingy::RefersTo(inThingy);
	}
	return refersToIt;
}


// remove all references we have to a thing
void
CShip::RemoveReferencesTo(AGalacticThingy *inThingy) {
	if (!IsDead()) { // if this ship is dead, it's references won't be used, so skip removal
		ASSERT(inThingy != nil);
		int count = GetWaypointCount();
		Waypoint wp;
		bool changed = false;
		for (int i = 1; i <= count; i++) {
		    wp = GetWaypoint(i);
			if (inThingy == wp.GetThingy()) {
			    changed = true;
			    if (inThingy->GetThingySubClassType() == thingyType_Rendezvous) {
			        wp.MakeDebris(false); // rendezvous points don't leave debris
                } else {
			        wp.MakeDebris(true);  // everything else does
			    }
			  #warning TODO: figure out how to make ships call in if they had a waypoint die
// probably need to check that this is host, check that mWantsAttention is not already true,
// and the set it and create an appropriate event.
// we need all this checking because we don't want multiple callins, and all of the needs attention
// based callin events have already been generated by the time we get remove references calls
//			    mWantsAttention = true;   // we just changed the course of a ship, it will want attention
		        SetWaypoint(i, wp);
				DEBUG_OUT("Removed reference to "<<inThingy<<" from "<<this, DEBUG_TRIVIA | DEBUG_MOVEMENT);
			}
		}
		if (changed) {
		    WaypointsChanged(kDontRefresh);
		}
	}
	ANamedThingy::RemoveReferencesTo(inThingy);
}

// change all the references we have to a particular id to reflect the item's new id
// used when the client created something with a temporary ID and gets the permanent
// ID assignment back from the host
void
CShip::ChangeIdReferences(PaneIDT oldID, PaneIDT newID) {
	int count = GetWaypointCount();
	Waypoint wp;
	bool changed = false;
	for (int i = 1; i <= count; i++) {
		wp = GetWaypoint(i);
		if (wp.ChangeIdReferences(oldID, newID)) {
		    changed = true;
		    SetWaypoint(i, wp);
            DEBUG_OUT("Updated course element of "<<this<<" (was going to "<<oldID<<")", DEBUG_DETAIL | DEBUG_MOVEMENT);
		}
	}
	if (changed) {
	    WaypointsChanged(kDontRefresh);
	}
	ANamedThingy::ChangeIdReferences(oldID, newID);
}



#ifndef GALACTICA_SERVER

void
CShip::DoubleClickSelf() {
    if (!mGame->IsClient()) {
        return;
    }
	CBoxView* panel = GetGameDoc()->GetActivePanel();
	if (panel) {
		panel->SimulateButtonClick('bAdd');
	}
}

Boolean
CShip::DragSelf(SMouseDownEvent &inMouseDown) {
#ifndef DEBUG
	if (CStarMapView::IsAwaitingClick())	// if we are plotting a course, we don't
		return false;					// want to allow them to drag the ship
	Point currentPoint, firstPoint, lastPoint;
	Point clickOffset;  // pixels by which the click point was offset from the centerpoint
#endif
	Boolean dragging = false;
#ifdef DEBUG
	ANamedThingy::DragSelf(inMouseDown);	// debug, just pass on to inherited so host can
											// support dragging things around
#else
	FocusDraw();
	::GetMouse(&firstPoint);
	clickOffset.h = firstPoint.h - mDrawOffset.h;
	clickOffset.v = firstPoint.v - mDrawOffset.v;
	Boolean drawn = false;
	int deltaH, deltaV;
	while (::StillDown() && !dragging) {
		::GetMouse(&currentPoint);
		deltaH = currentPoint.h-firstPoint.h;
		deltaV = currentPoint.v-firstPoint.v;
		if ( (deltaH > DRAG_SLOP_PIXELS) || (deltaH < -DRAG_SLOP_PIXELS) || 
			 (deltaV > DRAG_SLOP_PIXELS) || (deltaV < -DRAG_SLOP_PIXELS) ) {
			dragging = true;
			lastPoint = firstPoint;
		}
	}
	if (dragging) {
		SysBeep(1);
/*		FocusDraw();
		SPoint32 lp;
		CStarMapView* map = (CStarMapView*)mSuperView;
		::PenMode(patXor);
		sDragging = true;
		while (::StillDown()) {
			::GetMouse(&currentPoint);
			if (*((long*)&lastPoint) != *((long*)&currentPoint)) {	// current point is mouse pos,
				if (drawn) {										// not drawing position.
					DrawSelf();		// erase
				}
				mDrawOffset.h = currentPoint.h - clickOffset.h;	// this makes it so the exact pixel
				mDrawOffset.v = currentPoint.v - clickOffset.v;	// that was clicked on in the target
				map->LocalToImagePoint(currentPoint, lp);		// remains under the mouse as we drag
				Point deltaPt = {0,0};
				sAutoscrolling = true;					// Autoscroll may do redraw, so setting
				map->AutoScrollDragging(lp, deltaPt);	// this flag tells CRendezvous::DrawSelf()
				sAutoscrolling = false;					// not to draw and thus avoid leaving tracks
				if (*((long*)&deltaPt) != 0) {	// if it was scrolled, then the Within view is
					FocusDraw();				// no longer in focus, so we need to set
					::PenMode(patXor);			// it up again
				}
				DrawSelf();
				drawn = true;
				lastPoint = currentPoint;
			}
		}
		if (drawn) {	// do final erase. Only needed because at high magnifications, the galactic
			DrawSelf();	// coordinates will not translate exactly to screen coords, so the item
		}				// we dragged may shift slightly.
		Point3D coord;
		float zoom = map->GetZoom();
//		map->PortToLocalPoint(lastPoint);
		lp.h = (long)mDrawOffset.h << 10L;	// draw offset always represents the exact center of the item,
		lp.v = (long)mDrawOffset.v << 10L;	// so we can use it to get the actual position in the starmap
		coord.x = (float)lp.h / zoom;	//еее incorrect assumptions about size
		coord.y = (float)lp.v / zoom;	// of sector, valid only for v1.2 and before еее
		coord.z = 0;
		mCurrPos = coord;
		sDragging = false;
		Refresh();	// redraw original position
		ThingMoved(kRefresh);
		// now adjust anything that refers to this, telling it to recalc its position, dst, etc..
		UMoveReferencedThingysAction recalcAction(this);
		mGame->DoForEverything(recalcAction);
	*/
	}
#endif	// DEBUG
	return dragging;
}

bool
CShip::ShouldDraw() const {
    GalacticaDoc* game = GetGameDoc();
    if (game) {
        // never draw if not visible to us
        int playerNum = game->GetMyPlayerNum();
        bool isAlly = IsAllyOf(playerNum) || game->FogOfWarOverride();
        if (!isAlly && (GetVisibilityTo(playerNum) == visibility_None)) {
            return false;
        }
        // always draw if selected
        if (game->GetSelectedThingy() == this) {
		    return true;
		}
		// never draw if DrawShips is off (unless selected)
		if (!game->DrawShips()) {
		    return false;
		}
		// don't draw ship docked at star
//		if (!isAlly && IsDocked()) {
//		    return false;
//		}
    }
    // don't draw if we are on patrol around a planet and aren't planning on leaving
    if (IsDocked() && IsOnPatrol() && !CoursePlotted()) {
		return false;
	}
	// draw if our superclass says so (we are in space) or if we are docked at a planet
	return ( ANamedThingy::ShouldDraw() || IsDocked() );
}

float
CShip::GetBrightness(short inDisplayMode) {
    // new algorithm, v2.0.8
    if (inDisplayMode == displayMode_Owner) { 
        return SHIP_NORMAL_BRIGHTNESS;
    } else if ( (inDisplayMode == displayMode_Tech) || (inDisplayMode == displayMode_Defense) ) {
        return ANamedThingy::GetBrightness(inDisplayMode);
    } else {
        return SHIP_IGNORE_BRIGHTNESS;
    }
/*	if ( (inDisplayMode < 2) || (inDisplayMode > 4) ) {
		return 0.75;
	} else  {
		return ANamedThingy::GetBrightness(inDisplayMode);
	} */
}

void
CShip::DrawSelf() {
	if ( !ShouldDraw() ) {
		return;
	}
	EraseSelectionMarker();
	if (mCurrPos.GetType() != wp_Coordinates) {
		CalcPosition(kDontRefresh);	// always recalc our frame and position if we are not in open space
	}
	bool docked = IsDocked();
	bool selected = (this == mGame->AsSharedUI()->GetSelectedThingy());
	bool amFleet = SupportsContentsOfType(contents_Ships);
	int playerNum = 0;
	bool showCourse = false;
	bool isAlly = false;
	bool drawRangeCircles = false;
	RGBColor c;
	CStarMapView* starmap = mGame->AsSharedUI()->GetStarMap();
	ASSERT(starmap != nil);
    GalacticaDoc* game = GetGameDoc();
    GalacticaHostDoc* host = NULL;
    if (!game) {
        host = dynamic_cast<GalacticaHostDoc*>(GetGame());
    }
	if (game) {
    	playerNum = game->GetMyPlayerNum();
	    isAlly = IsAllyOf(playerNum) || game->FogOfWarOverride();
	    // if this is a scout with a looping course, we consider it a courier and don't draw
	    // the course unless DrawCourierCourses is set
	    // otherwise, we draw the cours if ShowAllCourses is set
    	showCourse = ((mClass == class_Scout) && mCourse.loop) ? game->DrawCourierCourses() : game->IsShowAllCoursesOn();
    	showCourse = showCourse && ((playerNum==0) || isAlly);
	    c = *(game->GetColor(mOwnedBy, IsHilited()));
	    drawRangeCircles = game->DrawRangeCircles();
	} else if (host) {
	    c = *(host->GetColor(mOwnedBy));
	}
	float zoom = starmap->GetZoom();
    // draw scanner range
    if (!Hiliting() && (GetScannerRange() > 0) && ((drawRangeCircles && !mCourse.loop) || selected)) {
    	MacAPI::PenMode(srcCopy);
    	RGBColor scanColor = c;
        long scanradius = (long)((float)(GetScannerRange() / 1024L) * zoom);
        if (!selected) {
            BrightenRGBColor(scanColor, 0.25f);
        }
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
            if (!selected) {
                break;  // only draw first circle unless courier is selected
            }
        }
    }
	BrightenRGBColor(c, GetBrightness(starmap->GetDisplayMode()));
	Point p;
	int count = GetWaypointCount();
	MacAPI::PenMode(srcCopy);
	if ((selected || showCourse) && CoursePlotted() && isAlly) {
		bool drawOld = !mCourse.loop;
		bool drawFirst = false;
		int destWPNum = GetDestWaypointNum();
		for (int i = 1; i <= count; i++) {
			p = GetWaypoint(i).GetLocalPoint();
			starmap->GalacticToImagePt(p);	// scale to match zooming
			if (i == 1) {
				MacAPI::MoveTo(p.h, p.v);			// don't draw a line from ship to first point
			} else {
				bool isOld = (drawOld && (i <= destWPNum));
				if (selected) {				// use special color scheme for selected ship
					if (i == mSelectedWPNum) {
						SetRGBForeColor(0,0xffff,0);	// bright green for selected waypoint
					} else if (isOld) {
						MacAPI::ForeColor(blueColor);			// regular blue for old points
				    } else if (mCourse.loop) {
						SetRGBForeColor(0xaaaa, 0x1111, 0xffff);	// light purple for young looping waypoints
					} else {
						MacAPI::ForeColor(cyanColor);			// cyan for non-selected young points
					}
				} else {
					if (isOld) {
						SetRGBForeColor(0, 0, 0x3333);	// very dark blue for old points
            		} else if (mCourse.loop) {
            			SetRGBForeColor(0x5555, 0, 0x8888);	// dark purple for looping untraveled waypoints
					} else {
						SetRGBForeColor(0, 0x3333, 0x9999);	// dark cyan for untraveled points
					}
				}
 				if (i == destWPNum) {	// skip line to dest point, do it later
					Point p1 = mCurrPos.GetLocalPoint();		// instead, draw only to ship's position.
					starmap->GalacticToImagePt(p1);	// scale to zoom
					if (!isOld || selected) {
						MacAPI::MacLineTo(p1.h, p1.v);
					}
					MacAPI::MoveTo(p.h, p.v);			// now continue from destination waypoint
				} else if (isOld && !selected) {
					MacAPI::MoveTo(p.h, p.v); // don't draw old course lines unless selected
				} else {
					MacAPI::MacLineTo(p.h, p.v);
				}
			}
		}
		if (!drawOld) {	// course loops, so draw a line back to the first point
			if (selected) {
				if (1 == mSelectedWPNum) {			// ensure the loopback leg is selected with 1st pt
					SetRGBForeColor(0,0xffff,0);	// bright green for selected waypoint
				} else if (mCourse.loop) {
					SetRGBForeColor(0x8888,0,0xffff);	// purple for looping waypoints
				} else {
					MacAPI::ForeColor(cyanColor);
				}
			} else if (mCourse.loop) {
				SetRGBForeColor(0x5555, 0, 0x8888);	// dark purple for looping untraveled waypoints
			} else {
				SetRGBForeColor(0, 0x3333, 0x9999);	// dark cyan for untraveled points
			}
			p = GetWaypoint(1).GetLocalPoint();
			starmap->GalacticToImagePt(p);	// scale to match zooming
			MacAPI::MacLineTo(p.h, p.v);
		}
		if (selected) {
			if (destWPNum == mSelectedWPNum) {	// now fill in the remaining portion of the current
				SetRGBForeColor(0,0xffff,0);	// leg with bright green for selected waypoint
			} else if (mCourse.loop) {
				SetRGBForeColor(0x8888,0,0xffff);	// purple for looping waypoints
			} else {
				MacAPI::ForeColor(cyanColor);			// cyan for non-selected young points
			}
			MacAPI::PenSize(2,2);
		} else if (mCourse.loop) {
			SetRGBForeColor(0x5555, 0, 0x8888);	// dark purple for looping untraveled waypoints
		} else {
			SetRGBForeColor(0, 0x3333, 0x9999);	// dark cyan for untraveled points	
		}
		MacAPI::MoveTo(mDrawOffset.h, mDrawOffset.v);
		p = GetWaypoint(destWPNum).GetLocalPoint();	// now draw the part of the course we haven't traveled yet
		starmap->GalacticToImagePt(p);	// scale to match zooming
		MacAPI::MacLineTo(p.h, p.v);
		MacAPI::PenSize(1,1);
	}
	if (CoursePlotted() && !docked) {
		//draw in movement vector, if the ship is not docked at a planet
		MacAPI::MoveTo(mDrawOffset.h, mDrawOffset.v);
		p.h = mMoveDelta.x / 1024L;
		p.v = mMoveDelta.y / 1024L;
		starmap->GalacticToImagePt(p);
		MacAPI::RGBForeColor(&c);
		MacAPI::Line(p.h, p.v);
	}
	// еее put code to draw a better ship here
	MacAPI::RGBForeColor(&c);
	if (zoom < 0.5) {
		MacAPI::MoveTo(mDrawOffset.h, mDrawOffset.v);	// tiny, draw single pt
		MacAPI::Line(0,0);
		if ( (zoom > 0.124) || selected) {
			if (docked) {
				MacAPI::Move(1, -1);	// tiny triangle facing left
				MacAPI::Line(0,2);	// for docked ships
			} else {
				MacAPI::Move(-1, 1);	// tiny triangle pointing up
				MacAPI::Line(2,0);
			}
		}
	} else {
		PolyHandle pH = ::OpenPoly();
		if (zoom > 0.75) {	// largest size triangle
			if (docked) {
				MacAPI::MoveTo(mDrawOffset.h - 5, mDrawOffset.v);	// triangle facing left 
				MacAPI::Line(10, 4);								// for docked ships
				MacAPI::Line(0, -8);
			} else {
				MacAPI::MoveTo(mDrawOffset.h, mDrawOffset.v - 5);	// triangle pointing up
				MacAPI::Line(4, 10);
				MacAPI::Line(-8, 0);
			}
		} else {	// medium sized triangle
			if (docked) {
				MacAPI::MoveTo(mDrawOffset.h - 3, mDrawOffset.v);	// triangle facing left 
				MacAPI::Line(5, 2);								// for docked ships
				MacAPI::Line(0, -4);
			} else {
				MacAPI::MoveTo(mDrawOffset.h, mDrawOffset.v - 3);	// triangle pointing up
				MacAPI::Line(2, 5);
				MacAPI::Line(-4, 0);
			}
		}
		MacAPI::ClosePoly();
		if (amFleet) {
			MacAPI::OffsetPoly(pH, -1, -1);
			MacAPI::PenSize(2,2);
			MacAPI::FramePoly(pH);
			MacAPI::PenSize(1,1);
		}
		MacAPI::PaintPoly(pH);
		MacAPI::KillPoly(pH);
	}
	if ((!docked || selected) 
	  && (isAlly || (GetVisibilityTo(playerNum) >= visibility_Class))) {
	    // don't draw ship names unless class is visible
        if (selected && (zoom >= 1.0)) {
    		p = mDrawOffset;	// draw a health bar for selected ships
    		p.v -= 10;
    		DrawHealthBar(p, 40, 100 - GetDamagePercent() );
		}
		ANamedThingy::DrawSelf();	// only draw name if not docked, or if docked and selected
	}
}

void
CShip::MoveToNamePos(const Rect &frame) {
//	MacAPI:: MoveTo(frame.right + 2, frame.top + (frame.bottom - frame.top)/2 + 4);
	MacAPI:: MoveTo(frame.right, frame.top + (frame.bottom - frame.top)/2 + 4);
}

Point
CShip::GetDrawOffsetFromCenter() const {
	Point p;
	if (mSuperView && IsDocked() && !(IsOnPatrol() && !CoursePlotted())) {
		float scale  = ((CStarMapView*)mSuperView)->GetZoom();	// v1.2d8
		p = GetDockedShipOffsetFromStarCenter(scale);
	} else {
		p.h = p.v = 0;	// unless of course, we aren't at a star, in which case there is no offset
	}
	return p;
}

Rect
CShip::GetCenterRelativeFrame(bool full) const {
	Rect r;
	ASSERT(mSuperView != NULL);
	if (mSuperView == NULL) {  // 2.0.9, 11/22/01, this seems to be an issue
	    SetRect(&r,0,0,0,0);
	    return r;
	}
	r = AGalacticThingyUI::GetCenterRelativeFrame(full);
	Point p;
	if (full) {
		p.h = mMoveDelta.x / 1024L;
		p.v = mMoveDelta.y / 1024L;
		static_cast<CStarMapView*>(mSuperView)->GalacticToImagePt(p);
		r.top = Min(p.v - 1, r.top - 2);
		r.bottom = Max(p.v + 1, r.bottom + 2);
		r.left = Min(p.h - 20, r.left);
		r.right = Max(p.h + 20, r.right + 4 + mNameWidth);
		int count = GetWaypointCount();
		Point p2 = mCurrPos.GetLocalPoint();	// adjust frame to include line for all ship destinations
		for (int i = 1; i <= count; i++) {
			p = GetWaypoint(i).GetLocalPoint();
			p.h -= p2.h;					// get offset from current position to destination position
			p.v -= p2.v;
			static_cast<CStarMapView*>(mSuperView)->GalacticToImagePt(p);	// scale to match zooming
			r.top = Min(p.v - 7, r.top);		// now adjust the Center Relative Frame to accomodate
			r.bottom = Max(p.v + 8, r.bottom);	// the line to the destination position
			r.left = Min(p.h - 7, r.left);
			r.right = Max(p.h + 7, r.right);
		}
	}
	return r;
}

#endif // GALACTICA_SERVER



