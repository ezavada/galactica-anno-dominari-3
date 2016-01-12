//	GalacticaProcessor.cpp		
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include "GalacticaProcessor.h"
#include "AGalacticThingy.h"
#include "CStar.h"
#include "GalacticaGlobals.h"
#include "Galactica.h" // for YieldTimeToForegroundApp()

#include <ctime>
#include <cmath>

#ifndef GALACTICA_SERVER
#warning TODO: remove dependencies in processor on GalacticaPanes & GalacticaSharedUI
#include "GalacticaPanes.h"
#include "GalacticaSharedUI.h"
#endif // GALACTICA_SERVER

#warning TODO: make all GalacticaProcessor.cpp lists use std::vector

//#define PROFILE_EOT
//#define PROFILE_AI

#if defined(PROFILE_EOT) || defined(PROFILE_AI)
#include <Profiler.h>
#endif

//  GalacticaProcessor::GalacticaProcessor();		// constructor
GalacticaProcessor::GalacticaProcessor() 
: GalacticaShared(),
  mEverythingIterator(mEverything) 
{
	DEBUG_OUT("New Processor", DEBUG_IMPORTANT);
	mComputerMovesRemainingToProcess = true;
	mDoingWork = false;
	for (int i = 0; i<MAX_PLAYERS_CONNECTED; i++) {
	    mPlayers[i].password[0] = 0;
	    mPlayers[i].lastTurnPosted = -1;
	    mPlayers[i].numPiecesRemaining = -1;
	    mPlayers[i].homeID = -1;
	    mPlayers[i].techStarID = -1;
	    mPlayers[i].hiStarTech = -1;
	    mPlayers[i].hiShipTech = -1;
	    mPlayers[i].builtColony = false;
	    mPlayers[i].builtScout = false;
	    mPlayers[i].builtSatellite = false;
	    mPlayers[i].hasTechStar = false;
	    mPlayers[i].compSkill = -1;
	    mPlayers[i].compAdvantage = -1;
    }
	mTurnsBetweenCallins = Galactica::Globals::GetInstance().getTurnsBetweenCallins();
	mLastStarIndex = 0;
}

GalacticaProcessor::~GalacticaProcessor() {
}

void
GalacticaProcessor::GetPrivatePlayerInfo(int inPlayerNum, PrivatePlayerInfoT &outInfo) {
    if ( (inPlayerNum > 0) && (inPlayerNum <= MAX_PLAYERS_CONNECTED) ) {
        outInfo = mPlayers[inPlayerNum - 1];
        DEBUG_OUT("GetPrivatePlayerInfo #"<<inPlayerNum<<": LTP=" << outInfo.lastTurnPosted 
                << " PR="<< outInfo.numPiecesRemaining << " HID=" << outInfo.homeID
                << " TSID="<< outInfo.techStarID << " HStT=" << outInfo.hiStarTech
                << " HSpT="<< outInfo.hiShipTech << " CS=" << (int)outInfo.compSkill
                << " CA=" << (int)outInfo.compAdvantage, DEBUG_TRIVIA);
    } else if (inPlayerNum == kAdminPlayerNum) {
        std::memset(&outInfo, 0, sizeof(outInfo)); 
        DEBUG_OUT("GetPrivatePlayerInfo for Administrator, returning blank info", DEBUG_DETAIL);
    } else {
        DEBUG_OUT("Requested player info for impossible player #"<<inPlayerNum, DEBUG_ERROR);
    }
}

void
GalacticaProcessor::SetPrivatePlayerInfo(int inPlayerNum, PrivatePlayerInfoT &inInfo) {
    if ( (inPlayerNum > 0) && (inPlayerNum <= MAX_PLAYERS_CONNECTED) ) {
        mPlayers[inPlayerNum - 1] = inInfo;
        DEBUG_OUT("SetPrivatePlayerInfo #"<<inPlayerNum<<": LTP=" << inInfo.lastTurnPosted 
                << " PR="<< inInfo.numPiecesRemaining << " HID=" << inInfo.homeID
                << " TSID="<< inInfo.techStarID << " HStT=" << inInfo.hiStarTech
                << " HSpT="<< inInfo.hiShipTech << " CS=" << (int)inInfo.compSkill
                << " CA=" << (int)inInfo.compAdvantage, DEBUG_DETAIL);
    } else if (inPlayerNum == kAdminPlayerNum) {
        DEBUG_OUT("SetPrivatePlayerInfo for Administrator - IGNORING", DEBUG_DETAIL);
    } else {
        DEBUG_OUT("Tried to set player info for impossible player #"<<inPlayerNum, DEBUG_ERROR);
    }
}

void
GalacticaProcessor::AddThingy(AGalacticThingy* inNewbornThingy) {
	DEBUG_OUT("Adding to new list "<< inNewbornThingy, DEBUG_TRIVIA | DEBUG_OBJECTS);
	mNewThingys.InsertItemsAt(1, LArray::index_Last, inNewbornThingy);
	GalacticaShared::AddThingy(inNewbornThingy);
	if (inNewbornThingy->GetThingySubClassType() == thingyType_Star) {
	    // keep track of how many stars we have
		mLastStarIndex++;
	}
}

void
GalacticaProcessor::RemoveThingy(AGalacticThingy* inDeadThingy) {
	if (inDeadThingy->SendDeleteInfo()) {	// things like messages are deleted automatically by the
		ThingDBInfoT theInfo;					// client, so delete info doesn't need to be sent
		theInfo.thingID = inDeadThingy->GetID();
		theInfo.thingType = inDeadThingy->GetThingySubClassType();
		theInfo.action = action_Deleted;
    	DEBUG_OUT("Preparing delete info "<< inDeadThingy, DEBUG_TRIVIA | DEBUG_OBJECTS);
		mDeadThingys.InsertItemsAt(1, LArray::index_Last, theInfo);
	}
	if (inDeadThingy->GetThingySubClassType() == thingyType_Star) {
		mLastStarIndex--;
	}
    mNewThingys.Remove(inDeadThingy);   // take it out of the new list (only there if created this turn)
	GalacticaShared::RemoveThingy(inDeadThingy);
}

//  bool				GalacticaProcessor::ProcessOneComputerMove(bool inDoOpenHost);
// process a single movement of a computer player's thingy
// we do this one at a time so it doesn't interfere too much with event handling
// returns true if there are more things to process
bool
GalacticaProcessor::ProcessOneComputerMove() {
	DEBUG_OUT("Processor::ProcessOneComputerMove", DEBUG_TRIVIA);
	if (mClosing) {
		return false;
	}
	if (mComputerMovesRemainingToProcess) {	// check for computer moves left to process
	    AGalacticThingy *aThingy = nil;
	    if (!mEverythingIterator.Next(&aThingy) ) {
	        // reached the end of the array
	        mComputerMovesRemainingToProcess = false;
	        mEverythingIterator.ResetTo(LArrayIterator::index_BeforeStart);
	    } else {
			aThingy = ValidateThingy(aThingy);
			ASSERT(aThingy != nil);
		    if (aThingy == nil) {
				return true;
			}
			if ((aThingy->GetOwner() != 0) && PlayerIsComputer(aThingy->GetOwner())) {
				aThingy->DoComputerMove(mGameInfo.currTurn);
			}
		}
	}
	return (mComputerMovesRemainingToProcess);
}

void
GalacticaProcessor::FinishTurn() {
	DEBUG_OUT("Processor::FinishTurn", DEBUG_IMPORTANT);
	bool processed = true;	// assume no computer players, nothing to process
	mDoingWork = true;
	if (mGameInfo.numComputersRemaining > 0) {	// don't bother if there's nothing left
		processed = false;	// there are computer players, haven't finished yet
	  #ifdef PROFILE_AI
		ProfilerInit(collectDetailed, bestTimeBase, 100, 10);
      #endif
		while ( ProcessOneComputerMove() ) {
			// all work is done by call to ProcessOneComputerMove()
			NotifyBusy();		// put up our busy cursor.
			GalacticaApp::YieldTimeToForegoundTask();	//don't hog the CPU while in the background
			if (mClosing) {	// the user could have closed the document on us.
				break;
			}
		}
		if (!mClosing) { //!mClosing && 
			processed = true;	// now we've processed them all	
		}
	  #ifdef PROFILE_AI
		ProfilerDump("\pAI Profile Info");
		ProfilerTerm();	
	  #endif
	}
	if (processed) {	// don't do EOT if haven't processed all computers yet.
		DoEndTurn();
	}
	mDoingWork = false;
}

void
GalacticaProcessor::DoEndTurn() {
	DEBUG_OUT("Processor::DoEndTurn, End TURN # " << mGameInfo.currTurn, DEBUG_IMPORTANT);
	if (mGameInfo.numPlayersRemaining < 1)
		return;
#ifdef PROFILE_EOT
	ProfilerInit(collectDetailed, bestTimeBase, 100, 10);
#endif
	AGalacticThingy *aThingy = nil;
	SetStatusMessage(str_ProcessingEOT, "");	// get Processing end of turn string.
	mDoingWork = true;

	// ==============================
	//            EOT
	// ==============================
	PrepareForEndOfTurn(); 	// do anything needed by subclasses before EOT starts 
			// and all new fleets and rendezvous points created; this also re-reads
			// all the items changed by that player
	LArrayIterator *iterator = new LArrayIterator(mEverything, LArrayIterator::from_Start); // now let all the new items finish streaming
	while (iterator->Next(&aThingy)) {
		NotifyBusy();		// put up our busy cursor.
	  #ifdef DEBUG
		aThingy = ValidateThingy(aThingy);
		ASSERT_STR(aThingy != nil, "DoEndOfTurn() found invalid item in host everything list");
		if (!aThingy)
			continue;
	  #endif
		// we don't reset the changed flag here since changes made by clients need to
		// propagate to all the other clients
	  #ifndef GALACTICA_SERVER
		AGalacticThingyUI* ui = aThingy->AsThingyUI();
		if (!ui)
		    continue;
		ui->CalcPosition(kRefresh);
	  #endif // GALACTICA_SERVER
	}
	delete iterator;
	
	DEBUG_OUT("EOT: Host processing EOT for Everything list", DEBUG_DETAIL | DEBUG_EOT);
	iterator = new LArrayIterator(mEverything, LArrayIterator::from_Start); // now do eot
	while ( iterator->Next(&aThingy) ) {
		NotifyBusy();		// put up our busy cursor.
	  #ifdef DEBUG
		aThingy = ValidateThingy(aThingy);
		ASSERT(aThingy != nil);
		if (!aThingy)
			continue;
	  #endif
		aThingy->EndOfTurn(mGameInfo.currTurn);
	}

	// ==============================
	//          FINISH EOT
	// ==============================
	DEBUG_OUT("EOT: Host finishing EOT for Everything list", DEBUG_DETAIL | DEBUG_EOT);
	iterator->ResetTo(LArrayIterator::from_Start);	// go back and make sure all the ships are aimed right
	while (iterator->Next(&aThingy)) {
		NotifyBusy();					// put up our busy cursor.
	  #ifdef DEBUG
		aThingy = ValidateThingy(aThingy);
		ASSERT(aThingy != nil);
		if (!aThingy) {
			continue;
		}
	  #endif
		aThingy->FinishEndOfTurn(mGameInfo.currTurn);	// clean up any loose ends 
	}

	DEBUG_OUT("EOT: Host updating visibility for Everything list", DEBUG_DETAIL | DEBUG_EOT);
	iterator->ResetTo(LArrayIterator::from_Start);	// go back and make sure all the ships are aimed right
	while (iterator->Next(&aThingy)) {
		NotifyBusy();					// put up our busy cursor.
	  #ifdef DEBUG
		aThingy = ValidateThingy(aThingy);
		ASSERT(aThingy != nil);
		if (!aThingy) {
			continue;
		}
	  #endif
	    // for things that are scanners and their scan range or location changed, see
	    // what new stuff they may have spotted
	    if (aThingy->HasScanners() && aThingy->HasScannerChanged()) {
		    aThingy->UpdateWhatIsVisibleToMe(); 
		}
		// for things that have moved, and thus possibly moved into or out of scan range
		// see what their new visibility is
		if (aThingy->NeedsRescan()) {
		    aThingy->CalculateVisibilityToAllOpponents();
		}
	}
	delete iterator;

	mGameInfo.currTurn++;
	WriteChangesToHost();	// write changes to the data store
	if (mGameInfo.maxTimePerTurn) {
		std::time_t currDT;			// get current date/time
		currDT = std::time(&currDT); //::GetDateTime(&currDT);
		mGameInfo.nextTurnEndsAt = (unsigned long) currDT + (unsigned long) mGameInfo.maxTimePerTurn;	// decide when next turn ends
	}
	DEBUG_OUT("EOT: Saving Game info", DEBUG_DETAIL | DEBUG_EOT);
	mComputerMovesRemainingToProcess = true;
	SetStatusMessage(str_Idle, "");	// clear the processing string.
	DEBUG_OUT("HostDoc::DoEndTurn, Host EOT POSTED, Start TURN #" << mGameInfo.currTurn, DEBUG_IMPORTANT);
	mDoingWork = false;
#ifdef PROFILE_EOT
	ProfilerDump("\pEOT Profile Info");
	ProfilerTerm();	
#endif
}

void
GalacticaProcessor::SetStatusMessage(int, const char*) {
    // default method does nothing
}


void
GalacticaProcessor::PrepareForEndOfTurn() {
	DEBUG_OUT("GalacticaProcessor::PrepareForEndOfTurn", DEBUG_IMPORTANT);
	// ==============================
	//    Prepare everything
	// ==============================
	long turnNum = GetTurnNum();
	LArrayIterator iterator(mEverything, LArrayIterator::from_Start); // Search list of all thingys
	AGalacticThingy* aThingy;
	while (iterator.Next(&aThingy)) {
		if (!ValidateThingy(aThingy)) {
		    DEBUG_OUT("Found Invalid Thingy "<<aThingy<<" in Host everything list, removing...", DEBUG_ERROR | DEBUG_EOT);
		    mEverything.Remove(aThingy);
		    continue; // do nothing more with this invalid item lest we crash
		} else if (aThingy->IsToBeScrapped()) {
			DEBUG_OUT("Killing Scrapped Object "<< aThingy, DEBUG_TRIVIA | DEBUG_EOT | DEBUG_OBJECTS);
			aThingy->Scrapped();
			aThingy->Die();
		} else {
		    DEBUG_OUT("Preparing "<<aThingy<<" for end of turn", DEBUG_TRIVIA | DEBUG_EOT);		    
		    aThingy->PrepareForEndOfTurn(turnNum);
		}
	}
//	Reset scanner lists which will be different for this turn
    ClearAllScannerLists();
}

void
GalacticaProcessor::WriteChangesToHost() {
}


void
GalacticaProcessor::FillSector(int inDensity, int inSectorSize) {
	if (inDensity == 0) {       // allow us to create empty starmaps
	    return;
	}
	DEBUG_OUT("GalacticaProcessor::FillSector", DEBUG_IMPORTANT);
	UInt32 numStars;
  #ifndef GALACTICA_SERVER
	AsSharedUI()->GetStarMap()->Disable();	// create stars
  #endif // GALACTICA_SERVER
  #ifdef DO_DEBUGGING_CHEATS	// debugging only
   if (inDensity == 1) {	// еее debugging, want a consistant, small galaxy
       LMSetRndSeed(1);
       numStars = mGameInfo.totalNumPlayers+1;
   }
   else 
  #endif
   {
       numStars = (Rnd(10) + Rnd(10) + 10) * inDensity + Rnd(5) * Rnd(2);
   }
   if (numStars < (UInt32)mGameInfo.totalNumPlayers) {
      numStars = mGameInfo.totalNumPlayers;	// must have at least one star per player
   }
   numStars *= (inSectorSize * inSectorSize);  // adjust for size

   // create the stars
   std::vector<CStar*> unused;	// holds stars not yet evaluated
   Point3D p;
	for (UInt32 i = 0; i < numStars; i++) {
		NotifyBusy();	// put up our busy cursor.
		p.x = Rnd(PIXELS_PER_SECTOR * inSectorSize) * 1024L;	// еее this assumes 16x16 ly per 1x1 sector
		p.y = Rnd(PIXELS_PER_SECTOR * inSectorSize) * 1024L;	// еее which is not correct
		p.z = 0;
		CStar* myStar = static_cast<CStar*>( MakeThingy(thingyType_Star) );
		myStar->SetCoordinates(p, kDontRefresh);
		myStar->Persist();	// star saves itself into database
		unused.insert(unused.end(), myStar);
	}
    DEBUG_OUT("Created "<<numStars<<" stars. Beginning ownership assignment", DEBUG_IMPORTANT);
   std::vector<CStar*> taken;// holds stars that have already been taken
   long sectorSize = PIXELS_PER_SECTOR * 1024L * inSectorSize;
   long edgeDist = sectorSize / 4; // furthest allowed dist from edge for 1st star
   long minEdgeDist = sectorSize / 8;	// nearest allowed dist from edge for 1st star
   long homeDist = (long)(sectorSize / std::sqrt((double)mGameInfo.totalNumPlayers));	// longest dist between players
   long minHomeDist = (long)((float)homeDist * 0.7F);	// shortest dist tween players
   numStars = unused.size();
   UInt32 numExtraStars = numStars - mGameInfo.totalNumPlayers;	// num stars we can reject
   int maxNumStarNames = GetNumSubStringsInStringResource(STRx_StarNames) - 1;	// don't count the "S-" as a name
   int maxCompNames = GetNumSubStringsInStringResource(STRx_CompNames);	// get num of computer names in resource
   PrivatePlayerInfoT pInfo;
   char* usage = new char[maxNumStarNames];
   int i, itemNum, nameNum;
   for (i = 0; i<maxNumStarNames; i++)	{ // clear the usage counter for each starname
    	usage[i] = 0;
   }
   i = 1;
   CStar *aStar = NULL;
   if (numExtraStars) {
    	for (itemNum = 0; (UInt32)itemNum < numStars; itemNum++) {	// find a good star for 1st player
    		if ( (aStar = unused[itemNum]) != NULL) {       // good star is one close enough to
    			p = aStar->GetCoordinates();				// edge of starmap, but not too close
    			if ( ( ( (p.x < edgeDist) || ((sectorSize - p.x) < edgeDist) ) // let the fun
    				|| ( (p.y < edgeDist) || ((sectorSize - p.y) < edgeDist) )	// begin!
    		  #if Use_3D_Coordinates	// gotta love this syntax :-)
    				|| ( (p.z < edgeDist) || ((sectorSize - p.z) < edgeDist) )
    		  #endif
    			  )	// that was close enough part, now check for too close
    			&& !(  ( (p.x < minEdgeDist) || ((sectorSize - p.x) < minEdgeDist) )
    				|| ( (p.y < minEdgeDist) || ((sectorSize - p.y) < minEdgeDist) )
    		  #if Use_3D_Coordinates
    				|| ( (p.z < minEdgeDist) || ((sectorSize - p.z) < minEdgeDist) )
    		  #endif
    		  	  ) ) { // that finishes too close check, and the entire mind-boggling compare
    				break;			// was a good star, go right into while loop below
    			} else {
    				aStar = NULL;	// not a good star, keep looking
    			}
    		}
    	}
   }
   if (!aStar)	 {		    // didn't find acceptable first star
    	aStar = unused[0];	// so just take first one from list
   }
    DEBUG_OUT("First acceptable star is "<<aStar, DEBUG_DETAIL);
	while (i <= mGameInfo.totalNumPlayers) {
		ThrowIfNil_(aStar);						// assign each player a home-world, and give the
		NotifyBusy();	                        // computer players names. Save all in player 
		GetPrivatePlayerInfo(i, pInfo);			// info save the home planet info
		taken.insert(taken.end(), aStar);	    // add star to taken list
		unused.erase(unused.begin() + itemNum); // remove star from unused list
		aStar->SetOwner(i);
		aStar->SetTechLevel(1);
		aStar->SetScannerRange(UThingyClassInfo::GetScanRangeForTechLevel(aStar->GetTechLevel(), IsFastGame(), HasFogOfWar()));
        aStar->ScannerChanged();
        aStar->UpdateWhatIsVisibleToMe();
		aStar->SetPopulation(1000000);			// 1 billion people on home planet
		aStar->SetBuildHullType(class_Colony);	// start off with a nearly built colony ship
		aStar->SetPercentPaidTowardNextShip(0.99);
		nameNum = Rnd(maxNumStarNames);			// now pick a name at random for the home star
        std::string starName;
		LoadStringResource(starName, STRx_StarNames, 2 + nameNum);  // skip "S-" + substring index is one based
		usage[nameNum]++;
		if (usage[nameNum] > 1) {				// add roman numerals to if name reused
			std::string num;
			LoadStringResource(num, STRx_StarNumbers, usage[nameNum]);
			starName += num;
		}
		aStar->SetName(starName.c_str());			    // name the home stars
		aStar->ThingMoved(kDontRefresh);
		if (i > mGameInfo.numHumans) {			// save computer's name in the player info
			std::string computerName;
			LoadStringResource(computerName, STRx_Names, str_TitleSyntax);
			int pos = computerName.find('*');
			if (pos != (int)std::string::npos) {
				computerName.replace(pos, 1, starName);	// replace * in string with home planet name
			}
			pos = computerName.find('%');	
			if (pos != (int)std::string::npos) {
			    std::string groupName;
				LoadStringResource(groupName, STRx_CompNames, Rnd(maxCompNames) + 1);// choose rnd word like "Pact"
				computerName.replace(pos, 1, groupName);	// replace % with the random word
			}
			// truncate the string to the max length if necessary
			if (computerName.size() > MAX_PLAYER_NAME_LEN) {
			    computerName.resize(MAX_PLAYER_NAME_LEN);
			}
			PublicPlayerInfoT compInfo;
			GetPlayerInfo(i, compInfo);
			compInfo.isAlive = true;
			compInfo.isComputer = true;
			std::strcpy(compInfo.name, computerName.c_str());
			SetPlayerInfo(i, compInfo);
		}
		pInfo.homeID = aStar->GetID();
		// v1.2b11d5, save expert player info
		pInfo.hasTechStar = false;
		pInfo.techStarID = 0;
		pInfo.defaultBuildType = 1;
		pInfo.defaultGrowth = 334;
		pInfo.defaultTech = 333;
		pInfo.defaultShips = 333;
		SetPrivatePlayerInfo(i, pInfo);
		i++;									// go on to next player
		DEBUG_OUT("Searching for star for player "<<i<<" far enough away from all others", DEBUG_DETAIL);
		if (i <= mGameInfo.totalNumPlayers) {	// are there more players who need home stars?
			bool found = false;
			while (!found) {
				NotifyBusy();
	         DEBUG_OUT("1. unused size = "<<unused.size(), DEBUG_TRIVIA);
				itemNum = Rnd(unused.size());		// pick a star at random
	         DEBUG_OUT("2. choosing "<<itemNum, DEBUG_TRIVIA);
				aStar = unused[itemNum];
	         DEBUG_OUT("3. got "<<aStar, DEBUG_TRIVIA);
				found = true;
				if (numExtraStars) {	// only check stars if still enough left to reject
	            DEBUG_OUT("4. checking against taken", DEBUG_TRIVIA);
					for (int i = 0; i < (int)taken.size(); i++) { // check dist to all enemies
						NotifyBusy();
	               DEBUG_OUT("5. fetching taken "<<i, DEBUG_TRIVIA);
						CStar* otherStar = taken[i];
	               DEBUG_OUT("6. got "<<otherStar, DEBUG_TRIVIA);
						long dist = aStar->Distance(otherStar);
	               DEBUG_OUT("7. distance to "<<otherStar<<" is "<<dist, DEBUG_TRIVIA);
						if (dist < minHomeDist) {	// too close to enemy?
	                  DEBUG_OUT("8. unusable, removing "<<aStar<<" from unused list", DEBUG_TRIVIA);
							unused.erase(unused.begin()+itemNum); // star can't be used
							numExtraStars--;				// one less rejectable star
	                  DEBUG_OUT("9. removed, leaving "<<numExtraStars<<" stars still unused", DEBUG_TRIVIA);
							found = false;
							break;							// find a different star
						}
					}
				} else {
					DEBUG_OUT("Not enough stars for optimal dispersal", DEBUG_ERROR);
				}
			} // when we break out of this loop, we have a good star for the next player
		}
	} // end of our player home assignment while loop
	delete[] usage;
	// create star lanes
	// еее temporary code
//	AGalacticThingy *aThingy;	// ееее STARLANES ARE BROKEN !!! ееее
	// create the starlanes
	/*
	LArray &theList = mStarMap->GetSubPanes();
	UInt32 numItems = theList.GetCount();
	for (UInt32 i = 1; i <= numItems; i++) {
		if (theList.FetchItemAt(i, &aThingy) ) {
			ThrowIfNil_(aThingy);
			if (aThingy->GetThingySubClassType() == thingyType_Ship)
				((CStar*)aThingy)->CreateStarLanes(1);
		}
	}
	*/
	// еее end temporary code
  #ifndef GALACTICA_SERVER
	AsSharedUI()->GetStarMap()->Enable();
  #endif // GALACTICA_SERVER
}


void
GalacticaProcessor::CreateNew(NewGameInfoT &inGameInfo) {
	DEBUG_OUT("Processor::CreateNew", DEBUG_IMPORTANT);
	mGameInfo.numHumans = inGameInfo.numHumans;
	mGameInfo.numHumansRemaining = inGameInfo.numHumans;
	mGameInfo.numComputers = inGameInfo.numComputers;
	mGameInfo.numComputersRemaining = inGameInfo.numComputers;
	mGameInfo.totalNumPlayers = inGameInfo.numHumans + inGameInfo.numComputers;
	mGameInfo.numPlayersRemaining = mGameInfo.totalNumPlayers;
	mGameInfo.maxTimePerTurn = inGameInfo.maxTimePerTurn;
	mGameInfo.currTurn = 1;			// just starting a new game
	mGameInfo.nextTurnEndsAt = 0;	// no a time limit on 1st turn; need to wait for logins
	mGameInfo.gameState = state_NormalPlay;
	mGameInfo.compSkill = inGameInfo.compSkill;
	mGameInfo.compAdvantage = inGameInfo.compAdvantage;
	mGameInfo.sectorDensity = inGameInfo.density;
	mGameInfo.sectorSize = inGameInfo.sectorSize;
	mGameInfo.omniscient = inGameInfo.omniscient;
	mGameInfo.fastGame = inGameInfo.fastGame;
	mGameInfo.endTurnEarly = inGameInfo.endEarly;
    mGameInfo.timeZone = get_timezone();
    
    // assign a game id, but don't let it be zero or -1
    do {  
        mGameInfo.gameID = std::rand();
    } while ( (mGameInfo.gameID == (UInt32)-1) || (mGameInfo.gameID == 0) );
    
	LString::CopyPStr(inGameInfo.gameTitle, mGameInfo.gameTitle);
	PrivatePlayerInfoT thePlayerInfo;
	for (int i = 1; i <= mGameInfo.totalNumPlayers; i++) { // reserve records for player info
	    GetPrivatePlayerInfo(i, thePlayerInfo);
    	thePlayerInfo.lastTurnPosted = 0;	// no turns posted yet either
    	thePlayerInfo.numPiecesRemaining = 1;	// but they are alive, and will have 1 star
    	thePlayerInfo.hiStarTech = 1;
    	thePlayerInfo.hiShipTech = -5;		// ERZ 1.2b8d4 mod, show messages for built Traveler
    	thePlayerInfo.builtColony = false;	// for "new class of ship" message
    	thePlayerInfo.builtScout = false;	// for "new class of ship" message
    	thePlayerInfo.builtSatellite = false;	// for "new class of ship" message
    	SetPrivatePlayerInfo(i, thePlayerInfo);
	}
}

void        
GalacticaProcessor::FinishGameLoad()
{
    GalacticaShared::FinishGameLoad();
   // do anything necessary when game is loaded
}

// returns true if hull type has not been built before
bool	
GalacticaProcessor::PlayerHasNewHullType(int inPlayerNum, int inHullType, long inNewTechLevel) {
    if ((inPlayerNum > 0) && (inPlayerNum <= MAX_PLAYERS_CONNECTED)) {
    	bool isNew = false;
    	switch (inHullType) {
    		case class_Satellite:
    			if (!mPlayers[inPlayerNum - 1].builtSatellite) {
    				isNew = true;
    				mPlayers[inPlayerNum - 1].builtSatellite = true;
    			}
    			break;
    		case class_Scout:
    			if (!mPlayers[inPlayerNum - 1].builtScout) {
    				isNew = true;
    				mPlayers[inPlayerNum - 1].builtScout = true;
    			}
    			break;
    		case class_Colony:
    			if (!mPlayers[inPlayerNum - 1].builtColony) {
    				isNew = true;
    				mPlayers[inPlayerNum - 1].builtColony = true;
    			}
    			break;
    		default:
    			isNew = PlayerHasNewFighterClass(inPlayerNum, inNewTechLevel);
    			break;
    	}
    	return isNew;
    } else {
        return false;
    }
}

long		
GalacticaProcessor::GetPlayersHighestShipTech(int inPlayerNum) const {
    if ((inPlayerNum > 0) && (inPlayerNum <= MAX_PLAYERS_CONNECTED)) {
        return mPlayers[inPlayerNum - 1].hiShipTech;
    } else {
        return 0;
    }
}

bool		
GalacticaProcessor::PlayerHasNewShipTech(int inPlayerNum, long inNewTech) {
    if ((inPlayerNum > 0) && (inPlayerNum <= MAX_PLAYERS_CONNECTED)) {
    	bool higher = inNewTech > mPlayers[inPlayerNum - 1].hiShipTech;
    	if (higher) {
    		mPlayers[inPlayerNum - 1].hiShipTech = inNewTech;
    	}
    	return higher;
    } else {
        return false;
    }
}

long		
GalacticaProcessor::GetPlayersHighestFighterClass(int inPlayerNum) const {
    if ((inPlayerNum > 0) && (inPlayerNum <= MAX_PLAYERS_CONNECTED)) {
        return mPlayers[inPlayerNum - 1].hiShipTech/5;
    } else {
        return 0;
    }
}

// yes, this expects a TECH LEVEL, not a class, as input parameter
// it wouldn't be useful otherwise, since it couldn't update the highest tech level
bool		
GalacticaProcessor::PlayerHasNewFighterClass(int inPlayerNum, long inNewTechLevel) {
    if ((inPlayerNum > 0) && (inPlayerNum <= MAX_PLAYERS_CONNECTED)) {
    	bool higher = (inNewTechLevel/5) > (mPlayers[inPlayerNum - 1].hiShipTech/5);
    	PlayerHasNewShipTech(inPlayerNum, inNewTechLevel);
    	return higher;
    } else {
        return false;
    }
}

long		
GalacticaProcessor::GetPlayersHighestStarTech(int inPlayerNum) const {
    if ((inPlayerNum > 0) && (inPlayerNum <= MAX_PLAYERS_CONNECTED)) {
        return mPlayers[inPlayerNum - 1].hiStarTech;
    } else {
        return 0;
    }
}

bool		
GalacticaProcessor::PlayerHasNewStarTech(int inPlayerNum, long inNewTech) {
    if ((inPlayerNum > 0) && (inPlayerNum <= MAX_PLAYERS_CONNECTED)) {
    	bool higher = inNewTech > mPlayers[inPlayerNum - 1].hiStarTech;
    	if (higher) {
    		mPlayers[inPlayerNum - 1].hiStarTech = inNewTech;
    	}
    	return higher;
    } else {
        return false;
    }
}

long		
GalacticaProcessor::GetPlayersHighestStarClass(int inPlayerNum) const {
    if ((inPlayerNum > 0) && (inPlayerNum <= MAX_PLAYERS_CONNECTED)) {
        return mPlayers[inPlayerNum - 1].hiStarTech/5;
    } else {
        return 0;
    }
}

// yes, this expects a TECH LEVEL, not a class, as input parameter
// it wouldn't be useful otherwise, since it couldn't update the highest tech level
bool		
GalacticaProcessor::PlayerHasNewStarClass(int inPlayerNum, long inNewTechLevel) {
    if ((inPlayerNum > 0) && (inPlayerNum <= MAX_PLAYERS_CONNECTED)) {
    	bool higher = (inNewTechLevel/5) > (mPlayers[inPlayerNum - 1].hiStarTech/5);
    	PlayerHasNewStarTech(inPlayerNum, inNewTechLevel);
    	return higher;
    } else {
        return false;
    }
}


