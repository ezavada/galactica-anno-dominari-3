// CFleet
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 9/11/97 ERZ
// 1/31/98, ERZ, 1.2b5, made fleets scrap self rather than die when empty
// 9/13/98, ERZ, 1.2b8, changed over to ThingyRefs and eliminated FinishStreaming()

#include "CStar.h"
#include "GalacticaShared.h"
//#include "Galactica.h"
#include "CMessage.h"

#include "CFleet.h"

#ifndef GALACTICA_SERVER
#include "CHelpAttach.h"
#include "GalacticaPanels.h"
#include "GalacticaPanes.h"
#include <UDrawingUtils.h>
#endif // GALACTICA_SERVER

#include <cstring>

class UShipsListEOTAction : public UArrayAction {
protected:
	virtual bool DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData);
public:
    UShipsListEOTAction(long inTurnNum) : mTurnNum(inTurnNum) {}
protected:
    long mTurnNum;
};

bool
UShipsListEOTAction::DoAction(LArray &, ArrayIndexT, void* inItemData) { // inAffectedArray, inItemIndex
	ThingyRef* aRefP = (ThingyRef*) inItemData;
	AGalacticThingy* aThingy = aRefP->GetThingy();
	aThingy = ValidateThingy(aThingy);
	if (aThingy) {
	    aThingy->EndOfTurn(mTurnNum);
	}
	return false;
}

CFleet::CFleet(LView *inSuperView, GalacticaShared* inGame)
: CShip(inSuperView, inGame, thingyType_Fleet),
  mShipRefs(kAllowShipsOnly) {
	InitFleet();
}


/*
CFleet::CFleet(LView *inSuperView, GalacticaShared* inGame, LStream *inStream) 
: CShip(inSuperView, inGame, inStream, thingyType_Fleet),
  mShipRefs(kAllowShipsOnly) {
	InitFleet();
	ReadFleetStream(*inStream, false);
}
*/

CFleet::~CFleet() {
}

void
CFleet::InitFleet() {
    mInCombat = false;
}

void
CFleet::FinishInitSelf() {
	AGalacticThingy::FinishInitSelf();
	char s[256];
	std::sprintf(s, "%ld", mID);
	SetName(s);
}

void
CFleet::ReadStream(LStream *inStream, UInt32 streamVersion) {
	CShip::ReadStream(inStream, streamVersion);
	ReadFleetStream(*inStream, false);
}

void
CFleet::WriteStream(LStream *inStream, SInt8 forPlayerNum) {
	CShip::WriteStream(inStream, forPlayerNum);
	mShipRefs.WriteStream(*inStream);
}


void
CFleet::UpdateClientFromStream(LStream *inStream) {
	CShip::UpdateClientFromStream(inStream);
	ReadFleetStream(*inStream, true);		// read in everything
}

void
CFleet::UpdateHostFromStream(LStream *inStream) {
    bool wasScrapped = IsToBeScrapped();
	CShip::UpdateHostFromStream(inStream);
	ReadFleetStream(*inStream, true);		// read in everything
	if ( (wasScrapped && !IsToBeScrapped()) || (!wasScrapped && IsToBeScrapped()) ) {
	    SetScrap(IsToBeScrapped());    // we toggled the scrapping on or off for this fleet, make sure everything inside matches
	}
	CalcFleetInfo();        // we don't trust the client to send us correct info
}

void
CFleet::ReadFleetStream(LStream &inStream, bool inUpdating) {
	bool updatingHost = inUpdating && mGame->IsHost();
	if (updatingHost) {
      // ignore host controlled items if updating host
        DEBUG_OUT("client->host update for "<< this<< ", ignoring ships list", DEBUG_CONTAINMENT | DEBUG_MOVEMENT | DEBUG_DETAIL);
        CThingyList ignoreRefs(kAllowShipsOnly);
        ignoreRefs.ReadStream(inStream);
	} else {
	    mShipRefs.ReadStream(inStream);
	}
	// check certain key bits of info about ships in the fleet
/*	if (IsToBeScrapped()) {
	    // force all ships in the fleet to be scrapped
	    SetScrap(true);
	} */
    if (IsSatelliteFleet() && IsInSpace()) {
	    // fix any satellite fleets that have become detached from their planets
        char name[255];
        snprintf(name, 255, "Satellites %d", mID);
	    SetName(name); // by changing the name this stops being a satellite fleet
        Changed();
    }
	if (IsOnPatrol()) {
	    // force all ships that don't have courses to be on patrol
	    SetPatrol(true);
	} else {
	    SetPatrol(false);
	    // force all ships to be off patrol
	}
}


bool
CFleet::SupportsContentsOfType(EContentTypes inType) const {
	if (inType == contents_Ships) {
		return true;
	} else {
		return CShip::SupportsContentsOfType(inType);
	}
}

void*
CFleet::GetContentsListForType(EContentTypes inType) const {
	if (inType == contents_Ships) {
		return &((const_cast<CFleet*>(this))->mShipRefs);
	} else {
		return CShip::GetContentsListForType(inType);
	}
}

SInt32
CFleet::GetContentsCountForType(EContentTypes inType) const {
	if (inType == contents_Ships) {
		return mShipRefs.GetCount();
	} else {
		return CShip::GetContentsCountForType(inType);
	}
}



bool
CFleet::AddContent(AGalacticThingy* inThingy, EContentTypes inType, bool inRefresh) {
	if ((inType == contents_Any) || (inType == contents_Ships)) {
		inThingy = ValidateShip(inThingy);	// ее debugging
		ASSERT(inThingy != nil);
		ASSERT_STR(inThingy != this, "Added fleet to itself! This will cause a crash later");
		if (inThingy == this) {
			return false;		// avoid the crash
		}
	  #ifdef DEBUG
		if (IsSatelliteFleet()) {	// debug check for adding non-satellite to satellite fleet
			if ( ((CShip*)inThingy)->GetShipClass() != class_Satellite ) {
				DEBUG_OUT("Rejected attempt to add "<< inThingy<< " to Satellite Fleet "<< this, DEBUG_ERROR | DEBUG_CONTAINMENT);
				return false;
			}
		}
	  #endif
		if (inThingy->HasContent(this, contents_Ships)) {
			DEBUG_OUT("Add FAILED!" << inThingy << " contains " << this << " and would create circular reference.", DEBUG_ERROR | DEBUG_CONTAINMENT);
			return false;	// avoid the crash
		}
		if (inThingy) {
			DEBUG_OUT("Start adding "<<inThingy<<" to "<<this, DEBUG_TRIVIA | DEBUG_CONTAINMENT);
			AGalacticThingy* at = inThingy->GetPosition();	// remove ship from previous container
			if (at) {
				at->RemoveContent(inThingy, contents_Ships, inRefresh);
			}
			if (!inThingy->IsToBeScrapped()) {	// adding a non-scrapped ship to the fleet
				SetScrap(false);			// Clear the scrap flag if the fleet was to be scrapped
			}
			((CShip*)inThingy)->ClearCourse(true, inRefresh);	// v2.0.2, adding a ship to a fleet clears the course
			((CShip*)inThingy)->SetPatrol(IsOnPatrol());	// 1.2b11, ships should match fleet patrol status
			inThingy->SetPosition(this, inRefresh);	// do refresh
			ThingyRef aRef = inThingy;
			mShipRefs.InsertItemsAt(1, LArray::index_Last, aRef);
			CalcFleetInfo();
			DEBUG_OUT("CFleet::AddContent() "<<this<<"; calling Changed()", DEBUG_TRIVIA | DEBUG_CONTAINMENT);
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
CFleet::RemoveContent(AGalacticThingy* inThingy, EContentTypes inType, bool inRefresh) {
	if ((inType == contents_Any) || (inType == contents_Ships)) {
		ThingyRef aRef = inThingy;
		if (mShipRefs.ContainsItem(aRef)) {
    		DEBUG_OUT("Start removing "<<inThingy<<" from "<<this, DEBUG_TRIVIA | DEBUG_CONTAINMENT);
    		mShipRefs.Remove(aRef);
    		if (!IsSatelliteFleet()) {	// don't auto-scrap satellite fleets
    			if (mShipRefs.GetCount() < 1) { // fleets that no longer have items are automatically
    				SetScrap(true);			    // set to be scrapped
    			}
    		}
    		CalcFleetInfo();
    		DEBUG_OUT("CFleet::RemoveContent() "<<this<<"; calling Changed()", DEBUG_TRIVIA | DEBUG_CONTAINMENT);
    		Changed();
    	  #ifndef GALACTICA_SERVER
    		if (inRefresh) {
    			Refresh();
    		}
          #endif // GALACTICA_SERVER
    		DEBUG_OUT("Done removing "<<inThingy<<" from "<<this, DEBUG_TRIVIA | DEBUG_CONTAINMENT);
		}
	} else {
		DEBUG_OUT("Asked to remove unknown content type "<<(int)inType, DEBUG_ERROR | DEBUG_CONTAINMENT);
	}
//	CShip::RemoveContent(inThingy, inType);
}

bool
CFleet::HasContent(AGalacticThingy* inThingy, EContentTypes inType) {
	if ((inType == contents_Any) || (inType == contents_Ships)) {
		ThingyRef aRef = inThingy;
		return mShipRefs.ContainsItem(aRef);
    }
	return false;	// default assumes we have no contents
}

#warning CFleet::GetName is not thread safe, must pass in buffer
const char* 
CFleet::GetName(bool considerVisibility) {
	if (IsSatelliteFleet()) { // satellite fleets all have a simple name
	    static std::string s;
	    LoadStringResource(s, STRx_Names, str_Satellites);	// Assign the name to the star
	    return s.c_str();	
	} else {
		return CShip::GetName(considerVisibility);
	}
}

const char* 
CFleet::GetShortName(bool considerVisibility) {
    return GetName(considerVisibility); // short name is same as long name for fleets
}

bool
CFleet::IsSatelliteFleet() const {
	if (mName == "*") {
		return true;
	} else {
		return false;
	}
}

void
CFleet::BecomeSatelliteFleet() {
	SetName("*");
}

bool
CFleet::HasShipClass(EShipClass inClass) const {
   return ThingyListHasShipClass(&mShipRefs, inClass);
}


#ifndef GALACTICA_SERVER
StringPtr
CFleet::GetDescriptor(Str255 outDescriptor) const {
	if (IsSatelliteFleet()) {	// satellite fleets all have a simple name
		::GetIndString(outDescriptor, STRx_Names, str_Satellites);
		return outDescriptor;
	} else {
		return CShip::GetDescriptor(outDescriptor);
	}
}
#endif // GALACTICA_SERVER

// HOST ONLY
void
CFleet::RemoveDead() {
	CShip* ship;
	int numRefs = mShipRefs.GetCount();
	DEBUG_OUT("Removing Dead sub-fleets from "<<this, DEBUG_TRIVIA | DEBUG_COMBAT);
	for(int i = 1; i <= numRefs; i++) {				// go through the contained items list:
		ship = (CShip*) mShipRefs.FetchThingyAt(i);
		ASSERT_STR(ship != nil, this << " has an invalid item at index "<< i);
		if (!ship) {
			continue;
		}
		if (ship->GetThingySubClassType() == thingyType_Fleet) {
			DEBUG_OUT("Found sub-fleet "<<ship, DEBUG_TRIVIA | DEBUG_COMBAT);
			((CFleet*)ship)->RemoveDead();
			if (ship->IsToBeScrapped()) {	// empty fleets are marked to be scrapped
				DEBUG_OUT("Sub-fleet "<< ship << " is dead, removing from " << this, DEBUG_DETAIL | DEBUG_COMBAT);
				ship->Die();	// we want to kill them right away instead
				
				#warning NOTE: put code to send subfleet death notice here
				// main fleet (as opposed to subfleet) death notices 
				// are sent from FinalizeLosses() in CShip.cp
				
				--i;        // v2.0.8, go back one
				--numRefs;  // so we aren't skipping over the item after the dead one
			}
		} else {
			ASSERT_STR(!ship->IsDead(), "Found dead thingy "<<ship<<" in "<< this);
		}
	}	
}

void
CFleet::SetScrap(bool inScrap) {
	CShip::SetScrap(inScrap);
	CShip* it;
	int numRefs = mShipRefs.GetCount();
	for(int i = 1; i <= numRefs; i++) {				// go through the contained items list:
		it = (CShip*) mShipRefs.FetchThingyAt(i);	// and toggle their scrapped flag
		ASSERT_STR(it != nil, this << " has an invalid item at index "<< i);
		if (it) {
		    if (ValidateShip(it) ) { // v2.1b10, problems with getting stars inside fleets
    		    it->SetScrap(inScrap); // just set the contents to match
    		    // don't flag as changed on the client to reduce unnecessary ThingyData packets to host
    		    if (mGame->IsHost()) {
    		        it->Changed();
    		    }
		    } else {
		        DEBUG_OUT(this << " contains a non-ship item " << it << " at index "<<i, DEBUG_ERROR | DEBUG_CONTAINMENT);
		    }
		}
	}	
}


void
CFleet::SetPatrol(bool inPatrol) {
	CShip::SetPatrol(inPatrol);
	CShip* it;
	int numRefs = mShipRefs.GetCount();
	for(int i = 1; i <= numRefs; i++) {				// go through the contained items list:
		it = (CShip*) mShipRefs.FetchThingyAt(i);	// and set their patrol status
		ASSERT_STR(it != nil, this << " has an invalid item at index "<< i);
		if (it) {
		    if (ValidateShip(it) ) { // v2.1b10, problems with getting stars inside fleets
    		    if (!it->CoursePlotted()) {
        			it->SetPatrol(inPatrol);
        		    // don't flag as changed on the client to reduce unnecessary ThingyData packets to host
        			if (mGame->IsHost()) {
        			    it->Changed();
        			}
    			}
		    } else {
		        DEBUG_OUT(this << " contains a non-ship item " << it << " at index "<<i, DEBUG_ERROR | DEBUG_CONTAINMENT);
		    }
		}
	}	
}

void
CFleet::EndOfTurn(long inTurnNum) {
    // fleets need to make sure their contained items move before they do, so there are consistent
    // predictable rules for movement
    if (mHasMoved) {  // don't move twice
        return;
    }
    mHasMoved = true; // set the movement flag just in case we need it here
	DEBUG_OUT(this << " starting end of turn for contained items", DEBUG_DETAIL | DEBUG_CONTAINMENT | DEBUG_EOT);
	UShipsListEOTAction eotAction(inTurnNum);
	mShipRefs.DoForEntireList(eotAction);
	DEBUG_OUT(this << " done with end of turn for all contained items", DEBUG_DETAIL | DEBUG_CONTAINMENT | DEBUG_EOT);
    mHasMoved = false; // we need to clear this because CShip::EndOfTurn will exit immediately if we don't
	CShip::EndOfTurn(inTurnNum);
}

void
CFleet::FinishEndOfTurn(long inTurnNum) {
	CShip::FinishEndOfTurn(inTurnNum);
	CalcFleetInfo();
}

long
CFleet::RepairDamage(long inHowMuch) {
	long repaired = 0;
	if (mDamage) {
		CShip* it;
		DEBUG_OUT("Repairing damage for "<<this, DEBUG_TRIVIA | DEBUG_COMBAT);
		int numRefs = mShipRefs.GetCount();
		for(int i = 1; i <= numRefs; i++) {				// go through the contained items list:
			it = (CShip*) mShipRefs.FetchThingyAt(i);	// and repair their damage
			ASSERT_STR(it != nil, this << " has an invalid item at index "<< i);
			if (!it) {
				continue;
			}
			if (it->IsDead()) {
				continue;
			}
			repaired += it->RepairDamage(inHowMuch);
		}
	}
	mDamage -= repaired;	// quick and dirty way to update the damage
	return repaired;	// return total amount of damage repaired for all ships
	// don't need to recalc fleet info because this will only be called from the host
	// during the EOT phase, and the FinishEOT phase will then recalc all fleet info
}

void
CFleet::CalcFleetInfo() {
	UInt32 power = 0, damage = 0, tech = 0, scanrange = 0, x, origScanRange;
    UInt32 speed = (UInt32)-1;
    UInt32 lowStealth = (UInt32)-1;
    UInt32 hiStealth = 1;
	UInt8  shipClass = 0;
	UInt32 apparentsize = 100L; // fleets are apparent size 100, same as everything else
	bool bHasShips = false;
	DEBUG_OUT("CalcFleetInfo() for "<<this, DEBUG_TRIVIA | DEBUG_COMBAT | DEBUG_MOVEMENT | DEBUG_CONTAINMENT);
	ASSERT_STR(!(IsSatelliteFleet() && IsToBeScrapped()), this<<" is a satellite fleet marked to be scrapped.");
	CShip* it;
	int numRefs = mShipRefs.GetCount();
	origScanRange = GetScannerRange();
	for(int i = 1; i <= numRefs; i++) {				// go through the contained items list:
		it = (CShip*) mShipRefs.FetchThingyAt(i);	// and include them in the fleet info
		ASSERT_STR(it != nil, this << " has an invalid item at index "<< i);
		if (!it) {
			continue;
		}
		if (mOwnedBy != it->GetOwner()) {
			DEBUG_OUT(this<<" contains an enemy "<<it, DEBUG_ERROR | DEBUG_COMBAT | DEBUG_MOVEMENT | DEBUG_CONTAINMENT);
		}
		if (it->IsDead()) {
			continue;
		}
		if (it->GetThingySubClassType() == thingyType_Fleet) {	// sub-fleets need to recalc now 
		   CFleet* fleet = static_cast<CFleet*>(it); 
			fleet->CalcFleetInfo();			// since their stats may have changed
			if (it->GetContentsCountForType(contents_Ships) <= 0) {
				continue;	// don't included sub-fleets with no ships in the calculations
			}
		}
		bHasShips = true;
		power += it->GetPower();
		damage += it->GetDamage();
		x = it->GetSpeed();
		if (x < speed) {
			speed = x;
		}
		x = it->GetShipClass();
		if (x > shipClass) {
			shipClass = x;
		}
		x = it->GetTechLevel();
		if (x > tech) {
			tech = x;
		}
	    x = it->GetScannerRange();
	    if (x > scanrange) {
	        scanrange = x;
	    }
	    x = it->GetStealth();
	    if (x < lowStealth) {
	    	lowStealth = x;
	    }
	    if (x > hiStealth) {
	    	hiStealth = x;
	    }
// ERZ: v3.0 -- we are making everything except scouts be easily visible in scanner range,
// no changes based on apparent size
	    // make sure apparent size is at least as much as largest item
	    // and add 10% of size of each additional item
//	    if (apparentsize < it->GetApparentSize()) {
//	    	apparentsize = it->GetApparentSize();
//	    } else {
//	    	apparentsize += (it->GetApparentSize()/10);
//	    }
//	    apparentsize += it->GetApparentSize();
	}
	if (!bHasShips) {
	  #ifdef DEBUG
		if (!IsDead()) {
			DEBUG_OUT(this<<" has no ships, setting all values to zero.", DEBUG_TRIVIA);
			if (!IsSatelliteFleet() && !mInCombat) {
				ASSERT_STR(IsToBeScrapped(), this<<" has no ships but is not marked to be scrapped.");
			}
		}
	  #endif
		speed = 0;
		damage = 0;
		power = 0;
		tech = 0;
		shipClass = 0;
		scanrange = 0;
		apparentsize = 0;
	}
	SetPower(power);
	SetDamage(damage);
	SetSpeed(speed);
	SetShipClass((EShipClass)shipClass);
	SetTechLevel(tech);
	SetApparentSize(apparentsize);
	UInt8 stealth = (lowStealth > hiStealth) ? hiStealth : lowStealth;
	SetStealth(stealth);
	if (origScanRange != scanrange) {
	    SetScannerRange(scanrange);
	    ScannerChanged();
//	    if (GetGameDoc()) {
//	        CalcPosition(kRefresh);
//	    }
	}
}

void
CFleet::RecordCombatInfo() {
	CFleet* fleet;
	mCombatInfo.fleetPower = mPower;
	int shipCount = 0;
	int numRefs = mShipRefs.GetCount();
	for(int i = 1; i <= numRefs; i++) {			// go through the contained items list:
		fleet = (CFleet*) mShipRefs.FetchThingyAt(i);
		ASSERT_STR(fleet != nil, this << " has an invalid item at index "<< i);
		if (!fleet)
			continue;
		if (fleet->IsDead())
			continue;
		if (fleet->GetThingySubClassType() == thingyType_Fleet) {	// sub-fleets need to recalc now 
			fleet->RecordCombatInfo();					// since their stats may have changed
			shipCount+= fleet->mCombatInfo.numShips;
		} else {
			shipCount++;	// this is a ship,
		}
	}
	mCombatInfo.numShips = shipCount;
	if (mCurrPos.GetType() == wp_Fleet) {
	    mCombatInfo.isSubFleet = true;
	} else {
	    mCombatInfo.isSubFleet = false;
	}
    mInCombat = true;
	DEBUG_OUT("Recorded Combat Info for " << this <<": "<<mCombatInfo.numShips<<" ships, "
		<< mCombatInfo.fleetPower<< " power, " << ((mCombatInfo.isSubFleet) ? "subfleet" : "topfleet"), DEBUG_TRIVIA | DEBUG_COMBAT);
}

void
CFleet::RecordCombatResults() {
	CFleet* fleet;
	mCombatInfo.fleetPower = mPower;
	int shipCount = 0;
	int numRefs = mShipRefs.GetCount();
	for(int i = 1; i <= numRefs; i++) {			// go through the contained items list:
		fleet = (CFleet*) mShipRefs.FetchThingyAt(i);
		ASSERT_STR(fleet != nil, this << " has an invalid item at index "<< i);
		if (!fleet)
			continue;
		if (fleet->IsDead())
			continue;
		if (fleet->GetThingySubClassType() == thingyType_Fleet) {	// sub-fleets need to recalc now 
			fleet->RecordCombatInfo();					// since their stats may have changed
			shipCount+= fleet->mCombatInfo.numShips;
		} else {
			shipCount++;	// this is a ship,
		}
	}
	mCombatInfo.numShips = shipCount;
	// don't check fleet or subfleet here, we already did that in RecordCombatInfo()
    mInCombat = false;
	DEBUG_OUT("Recorded Combat Results for " << this <<": "<<mCombatInfo.numShips<<" ships, "
		<< mCombatInfo.fleetPower<< " power, " << ((mCombatInfo.isSubFleet) ? "subfleet" : "topfleet"), DEBUG_TRIVIA | DEBUG_COMBAT);
}


void
CFleet::ChangeIdReferences(PaneIDT oldID, PaneIDT newID) {
	long numRefs = mShipRefs.GetCount();
	for(int i = 1; i <= numRefs; i++) {				// go through the contained items list:
		ThingyRef& ref = mShipRefs.FetchThingyRefAt(i);
        if (ref.GetThingyID() == oldID) {
            DEBUG_OUT("Updated item contained in "<<this, DEBUG_DETAIL | DEBUG_CONTAINMENT);
            ref.SetThingyID(newID, mGame);
        }
    }
    CShip::ChangeIdReferences(oldID, newID);
}
