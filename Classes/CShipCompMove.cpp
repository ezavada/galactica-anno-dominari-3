// CShipCompMove.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Separated from rest of CShip code, 12/4/99 ERZ

#include "GalacticaProcessor.h"

#include "CShip.h"
#include "CStar.h"
#include "CFleet.h"


// actual weights matrix for ship movement decisions can be found in "UShipUtils.cpp"

void
CShip::DoComputerMove(long) {//inTurnNum) 	// only called from host doc
	if (mNeedsDeletion) {	// don't do anything if ship destroyed or scrapped
	    DEBUG_OUT(this<<" marked for deletion, no computer move allowed ", DEBUG_DETAIL | DEBUG_MOVEMENT | DEBUG_AI);
		return;
	}
	if (mClass == class_Satellite) {	// satellites don't do anything
	    DEBUG_OUT(this<<" is a satellite, no movement allowd ", DEBUG_TRIVIA | DEBUG_MOVEMENT | DEBUG_AI);
		return;
	}
	if (mCurrPos.GetType() == wp_Fleet) {
	    if (mCurrPos.IsDebris()) {  // v2.1, fix problem with ships at fleet debris
	        DEBUG_OUT(this<<" is at fleet debris " << mCurrPos, DEBUG_ERROR | DEBUG_MOVEMENT | DEBUG_AI);
	    } else {
	        DEBUG_OUT(this<<" moves with fleet " << mCurrPos, DEBUG_TRIVIA | DEBUG_MOVEMENT | DEBUG_AI);
		    return;		// fleets make decision for all ships in them
		}
	}
	bool amFleet = SupportsContentsOfType(contents_Ships);
	if (amFleet && IsDocked()) {
		if (GetContentsCountForType(contents_Ships) < 3) {
			DEBUG_OUT(this<<" is a docked fleet with "<<GetContentsCountForType(contents_Ships)<<" ships, no movement allowed", DEBUG_DETAIL | DEBUG_MOVEMENT | DEBUG_AI);
			return;
		}
	}
	
	// gather some info about our location
	bool hasFleetAtLocalStar = false;
	bool hasShipOfSameClassAtLocalStar = false;
	bool atUnownedStar = false;
	AGalacticThingy* top = GetTopContainer();	// get the star they are at
	AGalacticThingy* fleet = nil;
	if (top->GetThingySubClassType() == thingyType_Star) {
	    if (top->GetOwner() == 0) {
	        atUnownedStar = true;
	    }
	    StdThingyList shipsList;
	    top->AddEntireContentsToList(contents_Ships, shipsList);
	    StdThingyList::iterator iter;
	    for (iter = shipsList.begin(); iter != shipsList.end(); iter++) {
	        CShip* ship = ValidateShip(*iter);
	        if (ship && (ship != this)) {
	            if (ship->GetShipClass() == GetShipClass()) {
	                hasShipOfSameClassAtLocalStar = true;
	                DEBUG_OUT("Found "<<ship<<" as ship of same class at local star", DEBUG_TRIVIA | DEBUG_AI);
	                if (fleet) {
	                    break;  // we've already found a fleet, we can stop looking
	                }
	            }
	            if (!fleet && (ship->GetThingySubClassType() == thingyType_Fleet) && !dynamic_cast<CFleet*>(ship)->IsSatelliteFleet()) {
	                hasFleetAtLocalStar = true;
	                fleet = ship;
	                DEBUG_OUT("Found "<<fleet<<" as a fleet at local star", DEBUG_TRIVIA | DEBUG_AI);
	                if (hasShipOfSameClassAtLocalStar) {
	                    break; // we've already found a ship of same class, we can stop looking
	                }
	            }
	        }
	    }
    }
	    
/*	if (CoursePlotted()) {	// don't reprocess anything that already has a course
		Waypoint wp = GetDestination();
		if (!wp.IsDead()) {	// unless the curr dest has disappeared or is dead
			return;
		}
	} */

	Waypoint *to = nil;
	int compSkill = mGame->GetComputerSkill();
	long maxDist = 999999L - (111111L * compSkill);
	int shiptype = mClass - 1;
	bool ignoreUnoccupiedStar = false;		// override for colonies that want to colonize heavily defended stars
	if (shiptype >= NUM_SHIP_TYPE_MATRICIES) {	// we need a type to choose a decision matrix
		shiptype = NUM_SHIP_TYPE_MATRICIES - 1;
	}
	DEBUG_OUT("Doing computer move for "<<this<<"; type = "<<shiptype, DEBUG_DETAIL | DEBUG_AI);
	int repetitions = 1;
	if (compSkill == compSkill_Expert) {
		repetitions = 3;
	} else if (compSkill == compSkill_Good) {
		repetitions = 2;
	}
	// v1.2fc1, repeat the AI process as needed so we always check for weakest enemy and
	// nearest enemy tech star vs. closest when at high skill levels
	MoveFactorT  theFinalProsAndCons = NewDecisionMatrix();
	long distances[4] = {0,0,0,0};
    int tos, tes, tfs, ts_, tff; // declare these here so gcc doesn't complain about a jump crossing
                                 // an initialization
	for (int iteration = 0; iteration < repetitions; iteration++) {
		DEBUG_OUT("Beginning AI iteration "<<iteration+1, DEBUG_DETAIL | DEBUG_AI);
		CStar* found[3];
		for (int i = 0; i<4; i++) { distances[i] += 1; }    // make sure we don't get the same stars all over again
		mGame->FindClosestStars(this, found, maxDist, distances);
		CStar *nearestUnownedStar = found[0];
		CStar *nearestOwnedStar = found[1];	// nearest star that belongs to us
		CStar *nearestEnemyStar = found[2];
		CStar *temp = NULL;
		if (iteration == 1) {	// second pass through, only for good
			temp = mGame->FindWeakestEnemyStar(this, (mSpeed * 4));
		  #ifdef DEBUG
			if (temp) {
				DEBUG_OUT("Found weakest enemy "<<temp<<"; strength = "<<temp->GetTotalDefenseStrength(), DEBUG_DETAIL | DEBUG_AI);
			} else {
				DEBUG_OUT("No weakest enemy star found.", DEBUG_DETAIL | DEBUG_AI);
			}
		  #endif
		} else if (iteration == 2) {	// third pass through, only for expert
			temp = mGame->FindNearestEnemyTechProductionStar(this, (mSpeed * 8));
		  #ifdef DEBUG
			if (temp) {
				DEBUG_OUT("Found enemy tech star "<<temp<<"; tech spending = "<<temp->GetSpending(spending_Research), DEBUG_DETAIL | DEBUG_AI);
			} else {
				DEBUG_OUT("No nearby enemy tech star found.", DEBUG_DETAIL | DEBUG_AI);
			}
		  #endif
		}
		if (temp) {
			DEBUG_OUT("Evaluating "<<temp<<" as enemy star rather than "<<nearestEnemyStar, DEBUG_DETAIL | DEBUG_AI);
			nearestEnemyStar = temp;
			distances[2] = nearestEnemyStar->Distance(this);	// update distance to enemy star, which we need later
		}
        // v1.2b11d5, override for scout behavior, to properly seed planets
		if ((compSkill <= compSkill_Good) && (mClass == class_Scout)) {
			if (CoursePlotted()) {
				// make sure we haven't lost the planet since we last targeted it with this scout
				Waypoint wp = GetDestination();
				AGalacticThingy* dst = wp.GetThingy();
				if (dst == nil) {
					goto RECONSIDER;
				}
				if (dst->GetOwner() != mOwnedBy) {
					goto RECONSIDER;
				}
				return;		// do nothing if course already plotted
			}
		  RECONSIDER:
			if (!nearestOwnedStar && !mGame->HasFogOfWar()) {	// if no nearby ally, do nothing unless we are in FOW game
				DEBUG_OUT("Skilled "<< this<< " found no allied star.", DEBUG_DETAIL | DEBUG_AI);
				return;
			}
			// if higher tech, go there to pick up tech
			// if lower tech, go there to drop off tech
			// if equal, do nothing
			if (nearestOwnedStar && (nearestOwnedStar->GetTechLevel() == GetTechLevel())) {
				DEBUG_OUT("Skilled "<< this<< " found allied "<< nearestOwnedStar<<" with same tech "<<GetTechLevel()<<", looking further...", DEBUG_TRIVIA | DEBUG_AI);
				nearestOwnedStar = mGame->FindNearestLowerTechAlliedStar(this, maxDist);
				if (nearestOwnedStar == nil) {
					if (mGame->HasFogOfWar() && (nearestUnownedStar != nil)) {
					    DEBUG_OUT("Skilled "<< this<< " going scouting", DEBUG_DETAIL | DEBUG_AI);
					    to = new Waypoint(nearestUnownedStar);
					    goto SET_SHIP_COURSE;
					} else {
					    DEBUG_OUT("Skilled "<< this<< " found nothing better", DEBUG_DETAIL | DEBUG_AI);
					    return;
					}
				} else {
					DEBUG_OUT("Skilled "<< this<< " found lower tech "<< nearestOwnedStar<<" with tech "<<nearestOwnedStar->GetTechLevel(), DEBUG_DETAIL | DEBUG_AI);
				}
			}
			if (nearestOwnedStar) {
				// set as destination
				to = new Waypoint(nearestOwnedStar);
				// jump to the place were the course is set
				DEBUG_OUT("Skilled "<< this<< " found allied "<< nearestOwnedStar
	             <<" with different tech "<<nearestOwnedStar->GetTechLevel()
	             <<" as destination", DEBUG_DETAIL | DEBUG_AI);
				goto SET_SHIP_COURSE;
			}
		}
		// then find the nearest enemy ship
		CShip *nearestEnemyShip = mGame->FindClosestEnemyShip(this, mOwnedBy, maxDist>>2L, &distances[3]);
		// now do tests, using the move factor tables to influence decision
		MoveFactorT  theProsAndCons = NewDecisionMatrix();
		theProsAndCons.ourStar = nearestOwnedStar;
		theProsAndCons.enemyStar = nearestEnemyStar;
		theProsAndCons.otherStar = nearestUnownedStar;
		theProsAndCons.ship = nearestEnemyShip;
		if (CoursePlotted()) {
			Waypoint wp = GetDestination();
			EWaypointType wpType = wp;
			if ((wpType > wp_Coordinates) /* && (!wp.IsDead()) */ ) {
				ConsiderFactor(0, shiptype, theProsAndCons);
			}
		}
		SInt32 freeStarDist = distances[0];
		SInt32 ourStarDist = distances[1];
		SInt32 enemyStarDist = distances[2];
		SInt32 enemyShipDist = distances[3];
		SInt32 ourStarStrength = 0;
		if (mCurrPos.GetThingy() == nil) {
			ConsiderFactor(10, shiptype, theProsAndCons);	//10. we are not at a star
		} else {
			ConsiderFactor(11, shiptype, theProsAndCons);	//11. we are at a star
    	    if (hasShipOfSameClassAtLocalStar) {
    			ConsiderFactor(31, shiptype, theProsAndCons);	//31. star we are at has other ships of our class
    	    }
    	    if (atUnownedStar) {
    			ConsiderFactor(32, shiptype, theProsAndCons);	//32. we are at unowned star
    	    }
    	    if (!hasFleetAtLocalStar) {
    			ConsiderFactor(33, shiptype, theProsAndCons);	//33. no fleet exists at this star
    	    }
		}
		if (GetDamagePercent() > 0) {
		    ConsiderFactor(29, shiptype, theProsAndCons);   //29. we are damaged
		    if (GetDamagePercent() > 50) {
		        ConsiderFactor(30, shiptype, theProsAndCons); // 30. we are heavily damage (> 50%)
		    }
		}
		if (!nearestOwnedStar) {
			DEBUG_OUT("No nearby allied stars found", DEBUG_DETAIL | DEBUG_AI);
			ConsiderFactor(1, shiptype, theProsAndCons);	// 1. no nearby owned star
			ourStarDist = 0x7fffffff;			// so it must be really far away
		} else  {
			DEBUG_OUT("Examining allied " << nearestOwnedStar, DEBUG_DETAIL | DEBUG_AI);
			ASSERT(ourStarDist == nearestOwnedStar->Distance(this));
			ourStarStrength = nearestOwnedStar->GetTotalDefenseStrength();
			if (GetTechLevel() > nearestOwnedStar->GetTechLevel()) {
				ConsiderFactor(16, shiptype, theProsAndCons);	// 16. our star has lower tech level than this ship		
			}
			if (nearestOwnedStar->GetContentsCountForType(contents_Ships) > 0) {
				CThingyList* shipsAtStar = (CThingyList*) GetContentsListForType(contents_Ships);			
				if (ThingyListHasShipClass(shipsAtStar, (EShipClass)mClass)) {
					ConsiderFactor(8, shiptype, theProsAndCons);	// 8. our star already has a ship of this class
				} else {
					ConsiderFactor(26, shiptype, theProsAndCons);   // 26. our star does not have a ship of this class
				}
			}
		}
		if (!nearestEnemyStar) {
			DEBUG_OUT("No nearby enemy star found.", DEBUG_DETAIL | DEBUG_AI);
			ConsiderFactor(2, shiptype, theProsAndCons);		// 2. no nearby enemy star
			enemyStarDist = 0x7fffffff;							// so it must be really far away
		} else {
			DEBUG_OUT("Examining enemy " << nearestEnemyStar, DEBUG_DETAIL | DEBUG_AI);
			SInt32 enemyStarStrength = nearestEnemyStar->GetTotalDefenseStrength();
			SInt32 strengthDiff = GetDefenseStrength() - enemyStarStrength;
			ASSERT(enemyStarDist == nearestEnemyStar->Distance(this));
			if (strengthDiff<0) {
				ConsiderFactor(12, shiptype, theProsAndCons);	// 12. enemy star more powerful
			}
			if (nearestEnemyStar->GetContentsCountForType(contents_Ships) > 0) {
				ConsiderFactor(13, shiptype, theProsAndCons);	// 13. enemy star already has a ship
			} else {
			    ConsiderFactor(25, shiptype, theProsAndCons);   // 24. enemy star unprotected
			}
			if (nearestEnemyStar->GetPopulation() < 1000) {		// population < 1 million?
				ConsiderFactor(19, shiptype, theProsAndCons);	// 19. enemy star has low population
			}
			DEBUG_OUT("Natural strength diff "<<this<<" | "<<nearestEnemyStar<<" = "<<strengthDiff, DEBUG_TRIVIA | DEBUG_AI);
			strengthDiff /= 100L;
			if (strengthDiff) {
				DEBUG_OUT("Adjusted strength diff = "<<strengthDiff, DEBUG_TRIVIA | DEBUG_AI);
				ConsiderMultiplierFactor(24, shiptype, strengthDiff, theProsAndCons);	//24. relative Strength
			}
		}
		if (nearestEnemyStar && nearestOwnedStar) {
			DEBUG_OUT("Checking if " << nearestOwnedStar << " closer than we are to " << nearestEnemyStar, DEBUG_DETAIL | DEBUG_AI);
    		SInt32 intraStarDist = nearestOwnedStar->Distance(nearestEnemyStar);
			if (intraStarDist < enemyStarDist) {
			    ConsiderFactor(27, shiptype, theProsAndCons);   // 27. our other star is closer to the enemy than we are
			}
		}
		if (!nearestUnownedStar) {
			DEBUG_OUT("No nearby unowned star found.", DEBUG_DETAIL | DEBUG_AI);
			ConsiderFactor(3, shiptype, theProsAndCons);		// 3. no nearby unowned star
			freeStarDist = 0x7fffffff;							// so it must be really far away
		} else {
			DEBUG_OUT("Examining unowned " << nearestUnownedStar, DEBUG_DETAIL | DEBUG_AI);
			ConsiderFactor(14, shiptype, theProsAndCons);		//14. unowned star exists
			ASSERT(freeStarDist == nearestUnownedStar->Distance(this));
			if (freeStarDist < enemyStarDist) {
				ConsiderFactor(15, shiptype, theProsAndCons);	//15. unowned star closer than enemy star
			} else if (enemyStarDist < ourStarDist) {
				ConsiderFactor(17, shiptype, theProsAndCons);		//17. enemy star closest star
			}
			if (nearestUnownedStar->GetContentsCountForType(contents_Ships) > 0) {
				ConsiderFactor(18, shiptype, theProsAndCons);		// 18. unowned star already has ship
			}
			// v1.2b11d5, override for colony ships attacking heavily protected unowned planets
			if ((compSkill <= compSkill_Average) && (mClass == class_Colony)) {
				SInt32 unownedStarStrength = nearestUnownedStar->GetTotalDefenseStrength();
				SInt32 strengthDiff = GetDefenseStrength() - unownedStarStrength;
				DEBUG_OUT("Natural power diff "<<this<<" | "<<nearestUnownedStar<<" = "<<strengthDiff, DEBUG_TRIVIA | DEBUG_AI);
				if (strengthDiff <= 0) {
					DEBUG_OUT("Disallowing travel to unowned star.", DEBUG_TRIVIA | DEBUG_AI);
					ignoreUnoccupiedStar = true;
				}
			}
		}
		if (!nearestEnemyShip) {
			DEBUG_OUT("No nearby enemy ship found.", DEBUG_DETAIL | DEBUG_AI);
			ConsiderFactor(4, shiptype, theProsAndCons);	// 4. no nearby enemy ship
			enemyShipDist	= 0x7fffffff;					// so it must be really far away
		} else {
			DEBUG_OUT("Examining enemy " << nearestEnemyShip, DEBUG_DETAIL | DEBUG_AI);
			UInt32 esPower = nearestEnemyShip->GetPower();
			UInt32 esSpeed = nearestEnemyShip->GetSpeed() ;
			SInt32 powerDiff = mPower - esPower;
			SInt32 speedDiff = mSpeed - esSpeed;
			DEBUG_OUT("Natural power diff "<<this<<" | "<<nearestEnemyShip<<" = "<<powerDiff, DEBUG_TRIVIA | DEBUG_AI);
			DEBUG_OUT("Natural speed diff = "<<speedDiff, DEBUG_TRIVIA | DEBUG_AI);
			if (mGame->IsFastGame()) {		// ships move faster in fast game, v 1.2b7
				speedDiff /= 20000L;
			} else {
				speedDiff /= 10000L;
			}
			DEBUG_OUT("Adjusted speed diff = "<<speedDiff, DEBUG_TRIVIA | DEBUG_AI);
			SInt32 starStrengthDiff = ourStarStrength - nearestEnemyShip->GetDefenseStrength();
			ConsiderMultiplierFactor(21, shiptype, speedDiff, theProsAndCons);	//21. relative speeds
			if (ourStarStrength) {
				DEBUG_OUT("Natural strength diff "<<nearestOwnedStar<<" | "<<nearestEnemyShip<<" = "<<starStrengthDiff, DEBUG_TRIVIA | DEBUG_AI);
				starStrengthDiff /= 100L;
				DEBUG_OUT("Adjusted strength diff = "<<starStrengthDiff, DEBUG_TRIVIA | DEBUG_AI);
				ConsiderMultiplierFactor(22, shiptype, starStrengthDiff, theProsAndCons);	//22. our star vs enemy ship power
			}
			ConsiderMultiplierFactor(23, shiptype, powerDiff, theProsAndCons);	//23. relative power
			if (nearestEnemyStar) {
				enemyStarDist = nearestEnemyStar->Distance(this);
			} else {
				enemyStarDist = 0x7fffffff;		// no enemy star, so it must be really far away
			}
			ConsiderFactor(5, shiptype, theProsAndCons);		// 5. nearby enemy ship exists
			if (mSpeed < esSpeed) {
				ConsiderFactor(6, shiptype, theProsAndCons);	// 6. enemy ship is faster
			}
			ASSERT(enemyShipDist == nearestEnemyShip->Distance(this));
			if ( (enemyShipDist < freeStarDist) && (enemyShipDist < enemyStarDist) &&
					(enemyShipDist < ourStarDist) ) {
				ConsiderFactor(7, shiptype, theProsAndCons);	// 7. we are closer to ship than any planet
			}
			if (mPower < esPower) {
				ConsiderFactor(9, shiptype, theProsAndCons);	// 9. enemy ship more powerful
			} else {
				if (amFleet && (compSkill <= compSkill_Good)) {	// enemy ship less powerful than fleet
					if (mPower > (esPower*2)) {
						theProsAndCons.toShip = -1;	// don't attack ships with too powerful fleets.
					}
				}
			}
			if (mGame->HasFogOfWar() && (nearestEnemyShip->GetShipClass() == class_Scout)) {
			    ConsiderFactor(28, shiptype, theProsAndCons); // 28, enemy ship is a scout
			}
		}
		if (!IsDocked()) {						// 1.2b8d5
			theProsAndCons.toFormFleet = -1;	// Cannot form fleet if not docked at a star
		}
//		if (GetThingySubClassType() == thingyType_Fleet) {
//		    theProsAndCons.toFormFleet = -1;    // don't form fleet if this already is a fleet
//		}
		if (ignoreUnoccupiedStar) {				// v1.2b11d5
			theProsAndCons.toOtherStar = -1;	// don't attack heavily guarded unoccupied stars
		}
		// v2.0.9d3, make sure we don't go to any non-existent items
		if (theProsAndCons.ourStar == NULL) {
		    theProsAndCons.toOurStar = -1;
		}
		if (theProsAndCons.enemyStar == NULL) {
		    theProsAndCons.toEnemyStar = -1;
		}
		if (theProsAndCons.otherStar == NULL) {
		    theProsAndCons.toOtherStar = -1;
		}
		if (theProsAndCons.ship == NULL) {
		    theProsAndCons.toShip = -1;
		}
		if (iteration == 0) {	// first time though the loop
			theFinalProsAndCons = theProsAndCons;
		} else {
			// choose the highest values found so far
			if (theProsAndCons.toOurStar > theFinalProsAndCons.toOurStar) {
				theFinalProsAndCons.toOurStar = theProsAndCons.toOurStar;
				theFinalProsAndCons.ourStar = theProsAndCons.ourStar;
			}
			if (theProsAndCons.toEnemyStar > theFinalProsAndCons.toEnemyStar) {
				theFinalProsAndCons.toEnemyStar = theProsAndCons.toEnemyStar;
				theFinalProsAndCons.enemyStar = theProsAndCons.enemyStar;
			}
			if (theProsAndCons.toOtherStar > theFinalProsAndCons.toOtherStar) {
				theFinalProsAndCons.toOtherStar = theProsAndCons.toOtherStar;
				theFinalProsAndCons.otherStar = theProsAndCons.otherStar;
			}
			if (theProsAndCons.toShip > theFinalProsAndCons.toShip) {
				theFinalProsAndCons.toShip = theProsAndCons.toShip;
				theFinalProsAndCons.ship = theProsAndCons.ship;
			}
		}
	}	// end for(repetitions)
	DEBUG_OUT("Completed "<< repetitions << " AI iterations", DEBUG_DETAIL | DEBUG_AI);
	tos = theFinalProsAndCons.toOurStar;
	tes = theFinalProsAndCons.toEnemyStar;
	tfs = theFinalProsAndCons.toOtherStar;
	ts_ = theFinalProsAndCons.toShip;
	tff = theFinalProsAndCons.toFormFleet;
	DEBUG_OUT("Decision scores: "<<tos<<", "<<tes<<", "<<tfs<<", "<<ts_<<", "<<tff, DEBUG_TRIVIA | DEBUG_AI);
	if (compSkill > compSkill_Average) {
		long skillMultiplier = compSkill - compSkill_Average;
		tos += SignedRnd(12*skillMultiplier);
		tes += SignedRnd(12*skillMultiplier);
		tfs += SignedRnd(6*skillMultiplier);
		ts_ += SignedRnd(14*skillMultiplier);
		if (compSkill >= compSkill_Novice) {
			tff = -1;
		}
		DEBUG_OUT(" Revised scores: "<<tos<<", "<<tes<<", "<<tfs<<", "<<ts_<<", "<<tff, DEBUG_TRIVIA | DEBUG_AI);
	}
	if ((tff>0) && (tff > tos) && (tff > tfs) && (tff > ts_) && (tff > tes)) {
		DEBUG_OUT(this<<" decided to join fleet", DEBUG_DETAIL | DEBUG_AI);
		// add to fleet, or form one if none exist here
		CThingyList* shipsList = (CThingyList*) top->GetContentsListForType(contents_Ships);	// then get list of ships
		if (!shipsList) {
			DEBUG_OUT(this<<" is not docked at a star, but decided to join a fleet", DEBUG_ERROR | DEBUG_AI);
		} else {
			bool fleetCreated = false;
			if (!fleet) {	// no existing fleet found, create one
				DEBUG_OUT("No fleet at local star, must create one", DEBUG_DETAIL | DEBUG_AI);
				if (GetThingySubClassType() != thingyType_Fleet) {	// avoid excessive nesting of fleets
					fleet = mGame->MakeThingy(thingyType_Fleet); // new CFleet(mSuperView, mGame);
					fleetCreated = true;    // we will write it to database below
					fleet->AssignUniqueID();
					fleet->SetOwner(mOwnedBy);
					top->AddContent(fleet, contents_Ships, kDontRefresh);	// put the new fleet where this was
				#warning TODO: need to set a unique name for this computer controlled fleet
				} else {
					DEBUG_OUT(this << " is already a fleet, no reason to create a new one.", DEBUG_TRIVIA | DEBUG_AI);
				}
			}
			if (fleet) {
				if (!fleetCreated) {
					DEBUG_OUT("Adding ship to existing fleet "<<fleet<<" at local star", DEBUG_TRIVIA | DEBUG_AI);
				}
				ClearCourse(true, kDontRefresh);
				SetCallIn(false);
				AGalacticThingy* it = GetContainer();
				fleet->AddContent(this, contents_Ships, kDontRefresh);	// add this item to it
			    if (fleetCreated) {
    			    fleet->Persist();
			    }
                ASSERT(fleet->IsChanged());
                ASSERT(top->IsChanged());
                ASSERT(it->IsChanged());
                ASSERT(this->IsChanged());
			}
		}
	} else if ((tos>0) && (tos >= tes) && (tos >= tfs) && (tos >= ts_) && (tos > tff) ) {
		DEBUG_OUT(this<<" decided to travel to own star", DEBUG_DETAIL | DEBUG_AI);
		to = new Waypoint(theFinalProsAndCons.ourStar);
	} else if ((tes>0) && (tes >= tos) && (tes >= tfs) && (tes >= ts_) && (tes > tff) ) {
		DEBUG_OUT(this<<" decided to attack enemy star", DEBUG_DETAIL | DEBUG_AI);
		to = new Waypoint(theFinalProsAndCons.enemyStar);
	} else if ((ts_>0) && (ts_ >= tos) && (ts_ >= tes) && (ts_ >= tfs) && (ts_ > tff) ) {
		DEBUG_OUT(this<<" decided to attack enemy ship", DEBUG_DETAIL | DEBUG_AI);
		to = new Waypoint(theFinalProsAndCons.ship);
	} else if (tfs > 0) {
		DEBUG_OUT(this<<" decided to go to nearest unowned star", DEBUG_DETAIL | DEBUG_AI);
		to = new Waypoint(theFinalProsAndCons.otherStar);
	} else {
		DEBUG_OUT(this<<" decided to do nothing", DEBUG_DETAIL | DEBUG_AI);
	}
	if (to) {	// set course for star
	  SET_SHIP_COURSE:
		DEBUG_OUT(this<<" new destination is now "<<(AGalacticThingy*)(*to), DEBUG_TRIVIA | DEBUG_AI | DEBUG_MOVEMENT);
		ClearCourse(false, kDontRefresh);
		SetCallIn(false);
		SetPatrol(false);
		if (!to->IsNull()) {
			AddWaypoint(1, *to, kDontRefresh);
		} else {
			DEBUG_OUT(this<<"couldn't go to NIL waypoint "<<*to, DEBUG_TRIVIA | DEBUG_AI);
		}
		delete to;
	}
	// v1.2b9, check for ships/fleets that need to be put on patrol,
	// if compSkill is good enough
	if (compSkill <= compSkill_Good) {
		if (IsDocked() && !CoursePlotted() && (mClass != class_Scout)) {
			if (IsOnPatrol() && (GetDamage() > 0)) {
				// v1.2b11d5, check for ships/fleets that need to be taken off patrol
				// for repair if compSkill is good enough
				SetPatrol(false);
				mChanged = true;
			} else if (!IsOnPatrol() && (GetDamage() == 0)) {
				SetPatrol(true);
				mChanged = true;
			}
		}
	}
	
//	AGalacticThingy::DoComputerMove(inTurnNum);
}

