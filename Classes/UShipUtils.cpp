// UShipUtils.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ

#include "UShipUtils.h"

#include "CShip.h"
#include "CFleet.h"

#include <LArrayIterator.h>

MoveFactorT gMoveFactors[NUM_SHIP_TYPE_MATRICIES][NUM_MOVE_FACTORS] = {
// ======= SCOUT SHIPS ========
//    ours     other     form fleet         description                        implemented
//     |  enemy  |   ship  |     factor #      |                                    |
//     |    |    |    |    |       |           |                                    |
{	{-50, -50, -50, -50, -50},	// 0. already moving toward a star or ship			X
	{-90,   0,   0,   0,   0},	// 1. no nearby owned star							X
	{ 20, -90,   0,   0,   0},	// 2. no nearby enemy star							X
	{ 20,   0, -90,   0,   0},	// 3. no nearby unowned star						X
	{  0,   0,   0, -90,   0},	// 4. no nearby enemy ship							X
	{  0,   0,   0,   0,   0},	// 5. nearby enemy ship exists						X
	{  0,   0,   0,   0,   0},	// 6. enemy ship is faster							X
	{  5,   0,   0,   0,   0},	// 7. we are closer to ship than any planet			X
	{  0,   0,  10,   0,   0},	// 8. our star already has a ship of this class		X
	{  0,   0,   0,   0,   0},	// 9. enemy ship more powerful						X
	{ 10, -90,   0, -90, -90},	//10. we are not at a star							X
	{-10, -90, -20, -90, -90},	//11. we are at a star								X
	{  0,   0,  10,   0,   0},	//12. enemy star more powerful						X
	{  0,   0,   0,   0,   0},	//13. enemy star already has a ship					X
	{  0,   0,   5,   0,   0},	//14. unowned star exists							X
	{  0,   0,  20,   0,   0},	//15. unowned star closer than enemy star			X
	{ 30,   0, -20,   0,   0},	//16. our star has lower tech level than this ship	X
	{  0,   0,   0,   0,   0},	//17. enemy star closest star						X
	{  0,   0, -50,   0,   0},	//18. unowned star already has ship					X
	{  0,   0,  10,   0,   0},	//19. enemy star has low population					X
	{ 50,   0,   0,   0,   0},	//20. target just changed owership (cancel out #0)
	{  1,   0,   0,   0,   0},	//21. relative our ship/enemy ship speed multiplier	X
	{  0,   0,   0,   0,   0},	//22. relative our star/enemy ship power multiplier	X
	{  0,   0,   0,   0,   0},	//23. relative our ship/enemy ship power multiplier	X
	{  0,   0,   0,   0,   0},	//24. relative our ship/enemy star power multiplier	X
	{  0,   0,   0,   0,   0},	//25. enemy star unprotected	                    X
	{  0,   0,   0,   0,   0},	//26. our star has no ship of this class            X
	{  0,   0,   0,   0,   0},	//27. owned star closer to enemy star than we are	X
	{  0,   0,   0,   0,   0},  //28. enemy ship is a scout                         X
	{  1,   0,   0,   0,   0},  //29. we are damaged                                X
	{  2,   0,   0,   0,   0},  //30. we are heavily damaged (> 50%)                X
	{  0,   0,   5,   0,   0},  //31. star we are at has other ships of our class   X
	{  5,   0,   5,   0,   0},  //32. we are at unowned star                        X
	{  0,   0,   0,   0,   0},  //33. no fleet exists at this star                  X   

	{  0,   0,   0,   0,   0}   // -- end --, insert items before here
}, 
// ======= COLONY SHIPS ========
//   ours     other      form fleet
//    |  enemy  |   ship  |
//    |    |    |    |    |
{	{-50, -50, -30, -50, -50},	// 0. already moving toward a star or ship
	{-90,   0,   0,   0,   0},	// 1. no nearby owned star
	{  0, -90,   0,   0,   0},	// 2. no nearby enemy star
	{  0,   0, -90,   0,   0},	// 3. no nearby unowned star
	{  0,   0,   0, -90,   0},	// 4. no nearby enemy ship
	{ 10,   5,  10,   0,   0},	// 5. nearby enemy ship exists
	{ 10,   0,   0,   0,   5},	// 6. enemy ship is faster
	{ 10,   0,  10,   0,   0},	// 7. we are closer to ship than any planet
	{  0,   0,  20,   0, -15},	// 8. our star already has a ship of this class
	{  0, -10,   0,   0,   5},	// 9. enemy ship more powerful
	{ 30,   0,  40, -40, -90},	//10. we are not at a star
	{-50, -20, -10, -40,  10},	//11. we are at a star
	{  0, -20,   5, -20,   5},	//12. enemy star more powerful
	{  0, -30,   0,   0,   5},	//13. enemy star already has a ship
	{  0,   0,  20,   0,   0},	//14. unowned star exists
	{  0,   0,  90,   0,   0},	//15. unowned star closer than enemy star
	{  0,   0,   0,   0,   0},	//16. our star has lower tech level than this ship
	{  0,  10,   0,   0,   5},	//17. enemy star closest star
	{  0,   0,   0,   0,   0},	//18. unowned star already has ship (¥¥enemy/ally check¥¥)
	{  0,  10,   0,   0,   0},	//19. enemy star has low population
	{ 50,  50,  30,  50,  50},	//20. target just changed owership (cancel out #0)
	{  0,   0,   0,   0,   0},	//21. relative our ship/enemy ship speed multiplier
	{ -2,   0,   2,   0,   0},	//22. relative our star/enemy ship power multiplier
	{  0,   0,   0,   1,   0},	//23. relative our ship/enemy ship power multiplier
	{  0,   4,   0,   0,   0},	//24. relative our ship/enemy star power multiplier	
	{  0,  10,   0,   0,  -1},	//25. enemy star unprotected	
	{  0,   0,   0,   0,   0},	//26. our star has no ship of this class
	{  0,   0,   0,   0,   0},	//27. owned star closer to enemy star than we are	
	{  0,   0,   0,   0,   0},  //28. enemy ship is a scout                         
	{  2, -10,   0, -10,   5},  //29. we are damaged                         
	{  4, -20,   0, -20,  10},  //30. we are heavily damaged (> 50%)                         
	{  0,   0,   0,   0,   0},  //31. star we are at has other ships of our class
	{  0,   0,   0,   0,   0},  //32. we are at unowned star
	{  0,   0,   0,   0,   0},  //33. no fleet exists at this star                      

	{  0,   0,   0,   0,   0}   // -- end --, insert items before here
}, 
// ======= BATTLESHIPS & FLEETS ========
//   ours     other      form fleet
//    |  enemy  |   ship  |
//    |    |    |    |    |
{	{-50, -20, -50, -30, -50},	// 0. already moving toward a star or ship
	{-90,   0,   0,   0,   0},	// 1. no nearby owned star
	{  0, -90,   0,   0,   5},	// 2. no nearby enemy star
	{  0,   0, -90,   0,   0},	// 3. no nearby unowned star
	{  0,   0,   0, -90,   0},	// 4. no nearby enemy ship
	{  0,   0,   0,  10,   0},	// 5. nearby enemy ship exists
	{  0,   0,   0,   0,   5},	// 6. enemy ship is faster
	{  0,   0,   0,  10,   0},	// 7. we are closer to ship than any planet
	{  0,  20,  10,  10,  10},	// 8. our star already has a ship of this class
	{ 10, -20,   0, -10,  10},	// 9. enemy ship more powerful
	{ 40,   0,   0,   0, -90},	//10. we are not at a star
	{-40, -10, -10, -10,   5},	//11. we are at a star
	{  0, -10,   0,   0,  10},	//12. enemy star more powerful
	{  0, -30,   0,   0,  10},	//13. enemy star already has a ship
	{  0,   0,  10,   0,   0},	//14. unowned star exists
	{  0,   0,  20,   0,   0},	//15. unowned star closer than enemy star
	{ 10,   0,   0,   0,   0},	//16. our star has lower tech level than this ship
	{  0,  20,   0,   0,   5},	//17. enemy star closest star
	{  0,   0, -20,   0,   0},	//18. unowned star already has ship
	{  0,  20,   0,   0,   0},	//19. enemy star has low population
	{ 50,  20,  50,  30,  50},	//20. target just changed owership (cancel out #0)
	{  0,   0,   0,   0,   0},	//21. relative our ship/enemy ship speed multiplier
	{ -2,   0,   0,  -1,  -2},	//22. relative our star/enemy ship power multiplier
	{  0,   0,   0,   1,  -2},	//23. relative our ship/enemy ship power multiplier
	{  0,   3,   0,   0,  -2},	//24. relative our ship/enemy star power multiplier	
	{  0,  20,   0,   0,   0},	//25. enemy star unprotected	
	{-10,   0,   0,   0,   0},	//26. our star has no ship of this class
	{ 30,   0,   0,   0,   0},	//27. owned star closer to enemy star than we are	
	{  0,   0,   0,  20,   0},  //28. enemy ship is a scout                         
	{ 10, -10,   0, -10,   0},  //29. we are damaged                         
	{ 20, -20,   0, -20,   0},  //30. we are heavily damaged (> 50%)                         
	{  0,   0,   0,   0,   5},  //31. star we are at has other ships of our class
	{  0,   0,   0,   0, -10},  //32. we are at unowned star
	{  0,   0,   0,   0, -20},  //33. no fleet exists at this star                      

	{  0,   0,   0,   0,   0}   // -- end --, insert items before here
} };

MoveFactorT
NewDecisionMatrix() {
	MoveFactorT aDecisionMatrix;
	aDecisionMatrix.toOurStar 	= 0;
	aDecisionMatrix.toEnemyStar = 0;	
	aDecisionMatrix.toOtherStar = 0;
	aDecisionMatrix.toShip 		= 0;
	aDecisionMatrix.toFormFleet = 0;
	aDecisionMatrix.ourStar		= NULL;	// v1.2fc1, track the items
	aDecisionMatrix.enemyStar 	= NULL;
	aDecisionMatrix.otherStar 	= NULL;
	aDecisionMatrix.ship 		= NULL;
	return aDecisionMatrix;
}

void 
ConsiderFactor(int factor, int shiptype, MoveFactorT &ioDecisionMatrix) {
	ioDecisionMatrix.toOurStar 	 += gMoveFactors[shiptype][factor].toOurStar;
	ioDecisionMatrix.toEnemyStar += gMoveFactors[shiptype][factor].toEnemyStar;
	ioDecisionMatrix.toOtherStar += gMoveFactors[shiptype][factor].toOtherStar;
	ioDecisionMatrix.toShip 	 += gMoveFactors[shiptype][factor].toShip;
	ioDecisionMatrix.toFormFleet += gMoveFactors[shiptype][factor].toFormFleet;
	DEBUG_OUT("Considered Factor "<<factor, DEBUG_TRIVIA | DEBUG_AI);
}

void 
ConsiderMultiplierFactor(int factor, int shiptype, long delta, MoveFactorT &ioDecisionMatrix) {
	register long decision = (long)gMoveFactors[shiptype][factor].toOurStar * delta;
	ioDecisionMatrix.toOurStar 	 += (decision/10);
	decision = (long)gMoveFactors[shiptype][factor].toEnemyStar * delta;
	ioDecisionMatrix.toEnemyStar += (decision/10);
	decision = (long)gMoveFactors[shiptype][factor].toOtherStar * delta;
	ioDecisionMatrix.toOtherStar += (decision/10);
	decision = (long)gMoveFactors[shiptype][factor].toShip * delta;
	ioDecisionMatrix.toShip 	 += (decision/10);
	decision = (long)gMoveFactors[shiptype][factor].toFormFleet * delta;
	ioDecisionMatrix.toFormFleet += (decision/10);
	DEBUG_OUT("Considered Factor "<<factor<<" ( x "<<delta<<")", DEBUG_TRIVIA | DEBUG_AI);
}

void
SortOutReserves(LArray* inForces, LArray* ioReserves, LArray* ioFleets) {
	AGalacticThingy* it;
	LArrayIterator iter(*inForces, LArrayIterator::from_Start);	// go through the attackers  list:
	while (iter.Next(&it)) {	// find all the scouts and colony ships and put them on reserve
		if (it->GetThingySubClassType() == thingyType_Ship) {
			EShipClass shipClass = ((CShip*)it)->GetShipClass();
			if ( (shipClass == class_Scout) || (shipClass == class_Colony) ) {
				DEBUG_OUT("Put " << it <<" on reserve", DEBUG_TRIVIA | DEBUG_COMBAT);
				inForces->Remove(&it);
				ioReserves->InsertItemsAt(1, LArray::index_Last, &it);
			}
		} else if (it->GetThingySubClassType() == thingyType_Fleet) {
			DEBUG_OUT("Removing " << it <<" from forces", DEBUG_TRIVIA | DEBUG_COMBAT);
			inForces->Remove(&it);
			EWaypointType wpType = it->GetPosition();
			if (wpType != wp_Fleet) {				// ignore nested fleets
				((CFleet*)it)->RecordCombatInfo();	// make the fleet record num of ships
			}										// and power it has now
			ioFleets->InsertItemsAt(1, LArray::index_Last, &it);
		}
	}
}

bool
AttackLoop(LArray* attackers, LArray* attackersReserves, LArray* defenders, LArray* defendersReserves) {
	bool done = false;
	bool haveWinner = false;
	AGalacticThingy* attacker;
	AGalacticThingy* defender;
	SInt32	numDefenders = defenders->GetCount() + defendersReserves->GetCount();
	LArrayIterator* iter = nil;
	if (attackers->GetCount()) {	// decide whether to use the attackers regular forces or their reserves
		iter = new LArrayIterator(*attackers, LArrayIterator::from_Start);
		DEBUG_OUT("Attackers are using regular forces", DEBUG_TRIVIA | DEBUG_COMBAT);
	} else if (attackersReserves->GetCount()) {	// there were no regular forces, we must use the reserves
		iter = new LArrayIterator(*attackersReserves, LArrayIterator::from_Start);
		DEBUG_OUT("Attackers are using reserves", DEBUG_TRIVIA | DEBUG_COMBAT);
	}
	if (iter == nil) {
		DEBUG_OUT("No attackers remain", DEBUG_TRIVIA | DEBUG_COMBAT);
		return true;	// no attackers, battle must be over
	}
	LArrayIterator diter(*defenders, LArrayIterator::from_Start);
	LArrayIterator driter(*defendersReserves, LArrayIterator::from_Start);
	while (!done) {
		if (!iter->Next(&attacker)) {
			DEBUG_OUT("Completed Attack Loop", DEBUG_DETAIL | DEBUG_COMBAT);
			done = true;
		}
		if (!done) {
			long extraDamage = 0;
			int targets = attacker->GetTechLevel()/2;	// can hit number of targets equal to 1/2 our tech level
			// if targets is zero, not a problem -- item still gets 1 attack
			DEBUG_OUT(attacker<<" attacking up to "<<targets<< " targets", DEBUG_TRIVIA | DEBUG_COMBAT);
			do {	// we repeat this section so we can spread attacker's damage over multiple targets
				if (defenders->GetCount()) {	// get a regular defender
					DEBUG_OUT("Taking defender from regular forces", DEBUG_TRIVIA | DEBUG_COMBAT);
					if (!diter.Next(&defender)) {
						diter.ResetTo(LArrayIterator::from_Start);
						diter.Next(&defender);	
						DEBUG_OUT("Attacked all defenders, wrapping around", DEBUG_TRIVIA | DEBUG_COMBAT);
					}
				} else if (defendersReserves->GetCount()) { // or get a reserve defender
					DEBUG_OUT("Taking defender from reserves forces", DEBUG_TRIVIA | DEBUG_COMBAT);
					if (!driter.Next(&defender)) {
						driter.ResetTo(LArrayIterator::from_Start);
						driter.Next(&defender);	
						DEBUG_OUT("Attacked all defending reserves, wrapping around", DEBUG_TRIVIA | DEBUG_COMBAT);
					}
				} else {
					done = true;
					haveWinner = true;
					DEBUG_OUT("Defenders are wiped out", DEBUG_DETAIL | DEBUG_COMBAT);
				}
				if (!done) {	// have an attacker and a defender, do battle
					bool wasDead = defender->IsDead();
					DEBUG_OUT(defender<< " defending against "<< attacker, DEBUG_DETAIL | DEBUG_COMBAT);
					extraDamage = defender->DefendAgainst(attacker, 100, extraDamage);
					if (!wasDead && defender->IsDead()) {
						numDefenders--;	// one defender less
						DEBUG_OUT(defender << " was killed by "<< attacker<<", "<<numDefenders<<" remain", DEBUG_DETAIL | DEBUG_COMBAT);
						if (numDefenders <= 0) {
							done = true;
							haveWinner = true;
							DEBUG_OUT("Defenders are wiped out", DEBUG_DETAIL | DEBUG_COMBAT);
						}
					}
					targets--;
				}
			} while ( !done && (extraDamage > 0) && (targets > 0) );
		}
	}
	delete iter;
	return haveWinner;
}

void
RemoveDead(LArray *ioForces) {
	AGalacticThingy* it;
	LArrayIterator iter(*ioForces, LArrayIterator::from_Start);	// go through the list:
	while (iter.Next(&it)) {	// find all the dead things and remove them
		if (it->IsDead()) {
			DEBUG_OUT(it <<" is dead, removed from forces", DEBUG_TRIVIA | DEBUG_COMBAT);
			ioForces->Remove(&it);
		}
	}
}

// returns player number of defenders
SInt16
RemoveAllies(LArray *ioForces, SInt16 inOwner) {
	SInt32	numAllies = 0;
	SInt16	defender = 0;
	AGalacticThingy* it;
	LArrayIterator iter(*ioForces, LArrayIterator::from_Start);	// go through the list:
	while (iter.Next(&it)) {	// find all the things in the list and remove allies
		if ((it->GetOwner() == 0) || it->IsAllyOf(inOwner)) {
			DEBUG_OUT(it<<" is an ally of player "<<inOwner<<", removed from forces", DEBUG_TRIVIA | DEBUG_COMBAT);
			ioForces->Remove(&it);
			numAllies++;
		} else {
			defender = it->GetOwner();
			DEBUG_OUT(it<<" is an enemy of player "<<inOwner, DEBUG_TRIVIA | DEBUG_COMBAT);
		}
	}
	DEBUG_OUT("Removed "<<numAllies<<" allies of player "<<inOwner<<" from defense forces", DEBUG_TRIVIA | DEBUG_COMBAT);
	return defender;
}

bool
HasShipClass(LArray *inForces, EShipClass inClass) {
    if (!inForces) { // v2.0.9d2, list could be null
        return false;
    }
	AGalacticThingy* it;
	CShip* ship;
	bool result = false;
	LArrayIterator iter(*inForces, LArrayIterator::from_Start);	// go through the list:
	while (iter.Next(&it)) {	// find all the dead things and remove them
		ship = ValidateShip(it);
		if (ship && (ship->GetShipClass() == inClass)) {
			DEBUG_OUT(it<<" is of the desired class ("<<(int)inClass<<")", DEBUG_TRIVIA | DEBUG_COMBAT);
			result = true;
			break;
		}
	}
	return result;
}


bool	
ThingyListHasShipClass(const CThingyList *inForces, EShipClass inClass, CShip* dontCountThisOne) {
    if (!inForces) { // v2.0.9d2, list could be null
        return false;
    }
	AGalacticThingy* it;
	CShip* ship;
	bool result = false;		// go through the list:
	int numThingys = inForces->GetCount();
	for (int i = 1; i <= numThingys; i++) {	// check for a item of the desired class
		it = inForces->FetchThingyAt(i);
		ship = ValidateShip(it);
		if (dontCountThisOne && (ship == dontCountThisOne)) {
		    continue;   // it should be clear by now that we aren't going to count this one
		}
		if (ship && (ship->GetShipClass() == inClass)) {
			DEBUG_OUT(it<<" is of the desired class ("<<(int)inClass<<")", DEBUG_TRIVIA | DEBUG_COMBAT);
			result = true;
			break;				// return true and stop looking if found
		}
	}
	return result;
}

float
GetBuildCostOfShipClass(EShipClass inClass, UInt16 inTechLevel) {
	float shipCost;
	if (inClass > class_Fighter) {
		inClass = class_Fighter;
	}
	switch (inClass) {
		case class_Satellite:
			shipCost = 0.4 + ((float)inTechLevel / 6.0);	// v1.2fc1, made satellites a little more costly
			break;												// was 8.0, changed to 6.0, and base cost from 0.2 to 0.4
		case class_Scout:
			shipCost = 0.1 + ((float)inTechLevel / 8.0);
			break;
		case class_Colony:
			shipCost = 4 + ((float)inTechLevel / 2.0);
			break;
		default:
			DEBUG_OUT("Unknown ship class "<<(int)inClass<<" queried", DEBUG_ERROR);
		case class_Fighter:
			shipCost = 1 + ((float)inTechLevel / 2.0);
			break;
	}
	return shipCost;
}

Point 	
GetDockedShipOffsetFromStarCenter(float inZoomScale) {
	Point p;
	if (inZoomScale > 1) {				// this little bit of math
		inZoomScale -= 1;				// makes zooming in to 8:1 give 12 pixel offset from star
		inZoomScale = inZoomScale/7;	// and makes 1:1 be at a 6 pixel offset, with a linear
		inZoomScale += 1;				// scale in between
	}
	p.h = (short)(6 * inZoomScale);
	p.v = (short)(-6 * inZoomScale);
	if (p.h < 2) {	// always keep at least 2 pixel offset
		p.h = 2;
		p.v = -2;
	}
	return p;
}

long
FinalizeLosses(LArray* ioFleets, SInt16 opponentPlayerNum) {
	AGalacticThingy* it;
	CFleet* fleet;
	long numDead = 0;
	LArrayIterator iter(*ioFleets, LArrayIterator::from_Start);	// go through the list of fleets
	while (iter.Next(&it)) {		// and remove all the dead things from each fleet
		fleet = (CFleet*)ValidateShip(it);
		if (fleet && (fleet->GetThingySubClassType() == thingyType_Fleet)) {
			EWaypointType wpType = it->GetPosition();
			if (wpType != wp_Fleet) {			// ignore nested fleets
				DEBUG_OUT("Removing dead sub-fleets from "<<fleet, DEBUG_TRIVIA | DEBUG_COMBAT);
				fleet->RemoveDead();	// v1.2b11d3, this is also recursive, so it is only done to the master fleets
				FleetCombatInfoT* infoP = fleet->GetCombatInfoPtr();
				DEBUG_OUT("FinalizeLosses found "<< ((infoP->isSubFleet) ? "sub" : "Master ") << "fleet " << fleet 
				          << ". Checking for Destroyed", DEBUG_TRIVIA | DEBUG_COMBAT);
				UInt32 startingShips = infoP->numShips;
				fleet->RecordCombatResults();	            // see how many ships survived and subtract them
				UInt32 remainingShips = infoP->numShips;    // InfoP only changes; doesn't move
				numDead -= (startingShips - remainingShips);
				if (infoP->numShips < 1) {	    // is the fleet itself dead?
					fleet->DestroyedInCombat(opponentPlayerNum, (ESendEvent)!infoP->isSubFleet); // the opponent killed this fleet, don't send event if subfleet
				}
			}
		}
	  #ifdef DEBUG
		else {
			DEBUG_OUT("FinalizeLosses found a thingy "<<it<<" in the fleets list that isn't a fleet", DEBUG_ERROR);
		}
	  #endif
	}
	return numDead;
}

#pragma mark-

void
CourseT::WriteStream(LStream *inStream) {
	Waypoint wp;
	SInt16 numWPs = 0;
//	if (waypoints == nil)
//		*inStream << numWPs;
//	else {
		numWPs = waypoints.size();
		*inStream << numWPs;
		for (int i = 0; i < numWPs; i++) {
			wp = waypoints[i];
			ASSERT_STR(!wp.IsNull(), "NULL Waypoint in Ship or Fleet Course");
			*inStream << wp;
		}
//	}
	UInt8 tempByte = patrol;
	tempByte |= hunt << 1;
	tempByte |= loop << 2;
	*inStream << tempByte;
	*inStream << curr;
	*inStream << prev;
}

void
CourseT::ReadStream(LStream *inStream, UInt32 streamVersion) {
	UInt8 tempByte;
	Waypoint wp;
	SInt16 numWPs;
	*inStream >> numWPs;
//	if (waypoints) {
//		delete waypoints;
//		waypoints = nil;
//	} 
//	if (numWPs > 0) {
		waypoints.resize(numWPs);
		for (int i = 0; i < numWPs; i++) {
			*inStream >> wp;
			waypoints[i] = wp;
		}
//	}
	*inStream >> tempByte;
	patrol = tempByte & 0x01;
	hunt = (tempByte >> 1) & 0x01;
	loop = (tempByte >> 2) & 0x01;
	*inStream >> curr;
	*inStream >> prev;
}


void
CourseT::Duplicate(CourseT *outCourse) const {
	outCourse->loop = loop;
	outCourse->hunt = hunt;
	outCourse->patrol = patrol;
	outCourse->curr = curr;
	outCourse->prev = prev;
	outCourse->waypoints = waypoints;
/*	if (waypoints) {
		Handle h;
		h = waypoints->GetItemsHandle();
		ThrowIfOSErr_( ::HandToHand(&h) );
		outCourse->waypoints = new LArray(sizeof(Waypoint), h);
		
	} else {
		outCourse->waypoints = nil;
    } */
}


LStream& operator >> (LStream& inStream, CourseT& outCourse) {
	outCourse.ReadStream(&inStream);
	return inStream;
}

LStream& operator << (LStream& inStream, CourseT& inCourse) {
	inCourse.WriteStream(&inStream);
	return inStream;
}


void	
GetSpeedString(SInt32 inSpeed, std::string &outStr) {
	if (inSpeed == 0) {
		outStr = '-';
	} else {
		float speed = (float) inSpeed / (float)16380;
		char tmpstr[40];
		std::sprintf(tmpstr, "%1.1f", speed);
		outStr = tmpstr;
	}
}

