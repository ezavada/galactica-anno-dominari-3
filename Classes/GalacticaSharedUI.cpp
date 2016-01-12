//	GalacticaSharedUI.cpp		
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include "GalacticaSharedUI.h"
#include "AGalacticThingy.h"
#include "GalacticaGlobals.h"

#include "GalacticaPanes.h" // for CStarMapView
#include "LAnimateCursor.h" 

AGalacticThingy*
GalacticaSharedUI::MakeThingy(long inThingyType) {
    return AGalacticThingy::MakeThingyFromSubClassType(mStarMap, this, inThingyType);
}

void
GalacticaSharedUI::NotifyBusy(long , long inTotalUnits) { // inCurrUnit
    if (inTotalUnits == 0) {
        // just set a busy cursor
	    Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
    }
}
