// UStarUtils.cp
// 4/15/99 ERZ

#include "UStarUtils.h"

#include "GalacticaThingys.h"
#include "CStar.h"


StarMoveFactorT gMoveFactors[NUM_SHIP_TYPE_MATRICIES][NUM_MOVE_FACTORS] = {
// ======= SCOUT SHIPS ========
//    ours     other     form fleet         description                        implemented
//     |  enemy  |   ship  |     factor #      |                                    |
//     |    |    |    |    |       |           |                                    |
{	{-50, -50, -50, -50, -50},	// 0. already moving toward a star or ship			X
	{-90,   0,   0,   0,  -5},	// 1. no nearby owned star							X
	{ 20, -90,   0,   0,  -5},	// 2. no nearby enemy star							X
	{ 20,   0, -90,   0,  -5},	// 3. no nearby unowned star						X
	{  0,   0,   0, -90,  -5},	// 4. no nearby enemy ship							X
	{  0,   0,   0,   0,  -5},	// 5. nearby enemy ship exists						X
	{  0,   0,   0,   0,  -5},	// 6. enemy ship is faster							X
	{  5,   0,   0,   0,  -5},	// 7. we are closer to ship than any planet			X
	{  0,   0,   0,   0,  -5},	// 8. our star already has a ship of this class		X
	{  0,   0,   0,   0,  -5},	// 9. enemy ship more powerful						X
	{ 10, -90,   0,   0, -90},	//10. we are not at a star							X
	{-10, -90, -20, -40,  -5},	//11. we are at a star								X
	{  0,   0,   0,   0,  -5},	//12. enemy star more powerful						X
	{  0,   0,   0,   0,  -5},	//13. enemy star already has a ship					X
	{  0,   0,   5,   0,  -5},	//14. unowned star exists							X
	{  0,   0,   0,   0,  -5},	//15. unowned star closer than enemy star			X
	{ 30,   0, -20,   0,  -5},	//16. our star has lower tech level than this ship	X
	{  0,   0,   0,   0,  -5},	//17. enemy star closest star						X
	{  0,   0, -50,   0,  -5},	//18. unowned star already has ship					X
	{  0,   0,   0,   0,  -5},	//19. enemy star has low population					X
	{ 50,   0,   0,   0,  -5},	//20. target just changed owership (cancel out #0)
	{  1,   0,   0,   0,   0},	//21. relative our ship/enemy ship speed multiplier	X
	{  0,   0,   0,   0,   0},	//22. relative our star/enemy ship power multiplier	X
	{  0,   0,   0,   0,   0},	//23. relative our ship/enemy ship power multiplier	X
	{  0,   0,   0,   0,   0}	//24. relative our ship/enemy star power multiplier	X
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
	{ 10,   0,  10,   0,   0},	// 5. nearby enemy ship exists
	{ 10,   0,   0,   0,  10},	// 6. enemy ship is faster
	{ 10,   0,  10,   0,   0},	// 7. we are closer to ship than any planet
	{  0,   0,  20,   0,   0},	// 8. our star already has a ship of this class
	{  0, -10,   0,   0,  10},	// 9. enemy ship more powerful
	{ 30,   0,  40, -40, -90},	//10. we are not at a star
	{-50, -20, -10, -40,  20},	//11. we are at a star
	{  0,   0,   0, -20,   0},	//12. enemy star more powerful
	{  0, -30,   0,   0,   0},	//13. enemy star already has a ship
	{  0,   0,  20,   0,   0},	//14. unowned star exists
	{  0,   0,  90,   0,   0},	//15. unowned star closesr than enemy star
	{  0,   0,   0,   0,   0},	//16. our star has lower tech level than this ship
	{  0,  10,   0,   0,  10},	//17. enemy star closest star
	{  0,   0,   0,   0,  10},	//18. unowned star already has ship (¥¥enemy/ally check¥¥)
	{  0,  10,   0,   0,   0},	//19. enemy star has low population
	{ 50,  50,  30,  50,  50},	//20. target just changed owership (cancel out #0)
	{  0,   0,   0,   0,   0},	//21. relative our ship/enemy ship speed multiplier
	{ -2,   0,   2,   0,   0},	//22. relative our star/enemy ship power multiplier
	{  0,   0,   0,   1,   0},	//23. relative our ship/enemy ship power multiplier
	{  0,   0,   0,   0,   0}	//24. relative our ship/enemy star power multiplier	
}, 
// ======= BATTLESHIPS & FLEETS ========
//   ours     other      form fleet
//    |  enemy  |   ship  |
//    |    |    |    |    |
{	{-50, -20, -50, -30, -50},	// 0. already moving toward a star or ship
	{-90,   0,   0,   0,   5},	// 1. no nearby owned star
	{  0, -90,   0,   0,   5},	// 2. no nearby enemy star
	{  0,   0, -90,   0,   5},	// 3. no nearby unowned star
	{  0,   0,   0, -90,   5},	// 4. no nearby enemy ship
	{  0,   0,   0,  10,   0},	// 5. nearby enemy ship exists
	{  0,   0,   0,   0,   0},	// 6. enemy ship is faster
	{  0,   0,   0,  10,   0},	// 7. we are closer to ship than any planet
	{-30,  20,  10,  10,  10},	// 8. our star already has a ship
	{ 10,   0,   0, -10,  10},	// 9. enemy ship more powerful
	{ 40,   0,   0,   0, -90},	//10. we are not at a star
	{-40, -10, -10, -10,   0},	//11. we are at a star
	{  0,   0,   0,   0,   0},	//12. enemy star more powerful
	{  0, -30,   0,   0,   0},	//13. enemy star already has a ship
	{  0,   0,  10,   0, -20},	//14. unowned star exists
	{  0,   0,  20,   0,   0},	//15. unowned star closesr than enemy star
	{ 10,   0,   0,   0,   0},	//16. our star has lower tech level than this ship
	{  0,  20,   0,   0,   0},	//17. enemy star closest star
	{  0,   0, -20,   0,   0},	//18. unowned star already has ship
	{  0,  20,   0,   0,   0},	//19. enemy star has low population
	{ 50,  20,  50,  30,  50},	//20. target just changed owership (cancel out #0)
	{  0,   0,   0,   0,   0},	//21. relative our ship/enemy ship speed multiplier
	{ -2,   0,   0,  -1,  -2},	//22. relative our star/enemy ship power multiplier
	{  0,   0,   0,   1,  -2},	//23. relative our ship/enemy ship power multiplier
	{  0,   1,   0,   0,  -2}	//24. relative our ship/enemy star power multiplier	
} };

MoveFactorT
NewDecisionMatrix() {
	MoveFactorT aDecisionMatrix;
	aDecisionMatrix.toOurStar 	= 0;
	aDecisionMatrix.toEnemyStar = 0;
	aDecisionMatrix.toOtherStar = 0;
	aDecisionMatrix.toShip 		= 0;
	aDecisionMatrix.toFormFleet = 0;
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
		if (it->GetThingySubClassType() == 'ship') {
			EShipClass shipClass = ((CShip*)it)->GetShipClass();
			if ( (shipClass == class_Scout) || (shipClass == class_Colony) ) {
				DEBUG_OUT("Put " << it <<" on reserve", DEBUG_TRIVIA | DEBUG_COMBAT);
				inForces->Remove(&it);
				ioReserves->InsertItemsAt(1, LArray::index_Last, &it);
			}
		} else if (it->GetThingySubClassType() == 'flet') {
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

Boolean
AttackLoop(LArray* attackers, LArray* attackersReserves, LArray* defenders, LArray* defendersReserves) {
	Boolean done = false;
	Boolean haveWinner = false;
	AGalacticThingy* attacker;
	AGalacticThingy* defender;
	Int32	numDefenders = defenders->GetCount() + defendersReserves->GetCount();
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
					Boolean wasDead = defender->IsDead();
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

void
RemoveAllies(LArray *ioForces, Int16 inOwner) {
	Int32	numAllies = 0;
	AGalacticThingy* it;
	LArrayIterator iter(*ioForces, LArrayIterator::from_Start);	// go through the list:
	while (iter.Next(&it)) {	// find all the things in the list and remove allies
		if ((it->GetOwner() == 0) || (it->GetOwner() == inOwner)) {
			DEBUG_OUT(it<<" is an ally of player "<<inOwner<<", removed from forces", DEBUG_TRIVIA | DEBUG_COMBAT);
			ioForces->Remove(&it);
			numAllies++;
		} else {
			DEBUG_OUT(it<<" is an enemy of player "<<inOwner, DEBUG_TRIVIA | DEBUG_COMBAT);
		}
	}
	DEBUG_OUT("Removed "<<numAllies<<" allies of player "<<inOwner<<" from defense forces", DEBUG_TRIVIA | DEBUG_COMBAT);
}

Boolean
HasShipClass(LArray *inForces, EShipClass inClass) {
	AGalacticThingy* it;
	CShip* ship;
	Boolean result = false;
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

Boolean	
ThingyListHasShipClass(CThingyList *inForces, EShipClass inClass) {
	AGalacticThingy* it;
	CShip* ship;
	Boolean result = false;		// go through the list:
	UInt32 numThingys = inForces->GetCount();
	for (int i = 1; i <= numThingys; i++) {	// check for a item of the desired class
		it = inForces->FetchThingyAt(i);
		ship = ValidateShip(it);
		if (ship && (ship->GetShipClass() == inClass)) {
			DEBUG_OUT(it<<" is of the desired class ("<<(int)inClass<<")", DEBUG_TRIVIA | DEBUG_COMBAT);
			result = true;
			break;				// return true and stop looking if found
		}
	}
	return result;
}

Point 	
GetDockedShipOffsetFromStarCenter(float inZoomScale) {
	Point p;
	if (inZoomScale > 1) {				// this little bit of math
		inZoomScale -= 1;				// makes zooming in to 8:1 give 12 pixel offset from star
		inZoomScale = inZoomScale/7;	// and makes 1:1 be at a 6 pixel offset, with a linear
		inZoomScale += 1;				// scale in between
	}
	p.h = 6 * inZoomScale;
	p.v = -6 * inZoomScale;
	if (p.h < 2) {	// always keep at least 2 pixel offset
		p.h = 2;
		p.v = -2;
	}
	return p;
}

long
FinalizeLosses(LArray* ioFleets, Int16 opponentPlayerNum) {
	AGalacticThingy* it;
	CFleet* fleet;
	Boolean result = false;
	long numDead = 0;
	LArrayIterator iter(*ioFleets, LArrayIterator::from_Start);	// go through the list of fleets
	while (iter.Next(&it)) {		// and remove all the dead things from each fleet
		fleet = (CFleet*)ValidateShip(it);
		if (fleet && (fleet->GetThingySubClassType() == 'flet')) {
			DEBUG_OUT("Removing dead sub-fleets from "<<fleet, DEBUG_TRIVIA | DEBUG_COMBAT);
			fleet->RemoveDead();	// ¥¥¥ this is also recursive, so it should only be done to the master fleets
			EWaypointType wpType = it->GetPosition();
			if (wpType != wp_Fleet) {			// ignore nested fleets
				DEBUG_OUT("FinalizeLosses found Master fleet " << fleet << ". Checking for Destroyed", DEBUG_TRIVIA | DEBUG_COMBAT);
				FleetCombatInfoT* infoP = fleet->GetCombatInfoPtr();
				numDead += infoP->numShips;	// count all ships originally in fleet as dead,
				fleet->RecordCombatInfo();	// then see how many survived and subtract them
				numDead -= infoP->numShips;	// from the count. InfoP only changes; doesn't move
				if (infoP->numShips < 1) {	// is the fleet itself dead?
//					ASSERT_STR(fleet->IsDead(), fleet << " has "<<infoP->numShips<< " ships, but thinks itself still alive")
					fleet->DestroyedInCombat(opponentPlayerNum); // the opponent killed this fleet
				}
			}
//			break;		¥¥¥ why was this here ?? ERZ, 2/21/98. Appears to assume that there is only one master fleet
//						in the fleets list, and so skips all the rest. This assumption should be true for now.
//						but I don't want to add back in the code yet, since it shouldn't hurt anything to check this
//						for all fleets. ERZ 12/30/98
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
	Int16 numWPs = 0;
	if (waypoints == nil)
		*inStream << numWPs;
	else {
		numWPs = waypoints->GetCount();
		*inStream << numWPs;
		for (int i = 0; i < numWPs; i++) {
			waypoints->FetchItemAt(i+1, &wp);
			ASSERT(!wp.IsNull());
			*inStream << wp;
		}
	}
	UInt8 tempByte = patrol;
	tempByte |= hunt << 1;
	tempByte |= loop << 2;
	inStream->WriteBlock(&tempByte, 1);
	inStream->WriteBlock(&curr, sizeof(Int16));
	inStream->WriteBlock(&prev, sizeof(Int16));
}

void
CourseT::ReadStream(LStream *inStream) {
	UInt8 tempByte;
	Waypoint wp;
	Int16 numWPs;
	*inStream >> numWPs;
	if (waypoints) {
		delete waypoints;
		waypoints = nil;
	}
	if (numWPs > 0) {
		waypoints = new LArray(sizeof(Waypoint));
		waypoints->AdjustAllocation(numWPs);
		for (int i = 0; i < numWPs; i++) {
			*inStream >> wp;
			waypoints->InsertItemsAt(1, i+1, &wp);
		}
	}
	inStream->ReadBlock(&tempByte, 1);
	patrol = tempByte & 0x01;
	hunt = (tempByte >> 1) & 0x01;
	loop = (tempByte >> 2) & 0x01;
	inStream->ReadBlock(&curr, sizeof(Int16));
	inStream->ReadBlock(&prev, sizeof(Int16));
}


void
CourseT::Duplicate(CourseT *outCourse) const {
	outCourse->loop = loop;
	outCourse->hunt = hunt;
	outCourse->patrol = patrol;
	outCourse->curr = curr;
	outCourse->prev = prev;
	if (waypoints) {
		Handle h;
		h = waypoints->GetItemsHandle();
		ThrowIfOSErr_( ::HandToHand(&h) );
		outCourse->waypoints = new LArray(sizeof(Waypoint), h);
		
	} else
		outCourse->waypoints = nil;
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
GetSpeedString(Int32 inSpeed, LString &outStr) {
	if (inSpeed == 0) {
		outStr.Assign('-');
	} else {
		Int32 speed = inSpeed / 1638;
		outStr.Assign(speed);
		if (outStr.Length() == 1) {		// add leading 0 in front of decimal point in speed if needed
			outStr.Insert((Uchar)'0', 1);
		}
		outStr.Insert((Uchar)'.', outStr.Length());
	}
}

