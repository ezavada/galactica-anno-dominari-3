//	CThingyList.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include "CThingyList.h"

#warning TODO: make CThingyList use std::vector

#if defined( POSIX_BUILD ) || defined ( PLATFORM_WIN32 ) 
enum {
    invalidIndexErr = -200
};
#endif

void
CThingyList::ReadStream(LStream& inInputStream, UInt32 streamVersion) {
  #warning TODO: should only update items that are not already in correct position in the list
	RemoveItemsAt(GetCount(), 1);	// clear all the item from the array
	UInt16 numThingys;
	inInputStream >> numThingys;	// read in the object count
	for (long i = 1; i <= numThingys; i++) {	// now read in that many objects
		ThingyRef aRef;
		inInputStream >> aRef;
		DEBUG_OUT(" |-- "<<aRef, DEBUG_TRIVIA | DEBUG_CONTAINMENT );
/*  #warning FIXME: this debug code is correcting errors that will kill a release version
	  #ifdef DEBUG
		ArrayIndexT idx = FetchIndexOf(aRef);
		if (idx != index_Bad) {	// make sure we don't have duplicate entries
			DEBUG_OUT(" |    Duplicate entry "<<aRef <<" in "<<this<<" -- skipped", DEBUG_ERROR | DEBUG_CONTAINMENT );
			continue;
		}
	  #else
	     #error critical debug code not included
	  #endif */
		InsertItemsAt(1, LArray::index_Last, aRef);
	}
}

void
CThingyList::WriteStream(LStream& inOutputStream) {
    ValidateList();
	UInt16 numThingys = GetCount();
	inOutputStream << numThingys;	// write the object count
	for (long i = 1; i <= numThingys; i++) {	// now write that many objects
		ThingyRef* aRefP = (ThingyRef*) GetItemPtr(i);
		DEBUG_OUT(" |-- "<<(*aRefP), DEBUG_TRIVIA | DEBUG_CONTAINMENT );
		inOutputStream << (*aRefP);
	}
}

AGalacticThingy*
CThingyList::FetchThingyAt(ArrayIndexT inAtIndex) const {
	if (!ValidIndex(inAtIndex)) {
		DEBUG_OUT("WARNING: ThingyList index out of range: "<<inAtIndex, DEBUG_IMPORTANT | DEBUG_CONTAINMENT );
		return NULL;
	}
	ThingyRef* aRefP = (ThingyRef*) GetItemPtr(inAtIndex);
	AGalacticThingy* aThingy = aRefP->GetThingy();
	aThingy = ValidateThingy(aThingy);
  #ifdef DEBUG
	if (aThingy == NULL) {
		DEBUG_OUT("ThingyList has invalid reference at index "<<inAtIndex << " to id "<<aRefP->GetThingyID()<<"; Not Removed", DEBUG_ERROR | DEBUG_CONTAINMENT );
	}
  #endif
	return aThingy;
}

ThingyRef&
CThingyList::FetchThingyRefAt(ArrayIndexT inAtIndex) const {
	ThingyRef* resultP = NULL;
	if (!ValidIndex(inAtIndex)) {
		DEBUG_OUT("Throwing: ThingyList index out of range: "<<inAtIndex, DEBUG_ERROR | DEBUG_CONTAINMENT );
		Throw_(invalidIndexErr);
	} else {
		resultP = (ThingyRef*) GetItemPtr(inAtIndex);
	}
	return *resultP;
}

void
CThingyList::ValidateList() {
	UInt32 numThingys = GetCount(); 
	for (int i = numThingys; i > 0; i--) { // start at the end so we can safely remove items
    	ThingyRef* aRefP = (ThingyRef*) GetItemPtr(i);
    	AGalacticThingy* aThingy = aRefP->GetThingy();
    	aThingy = ValidateThingy(aThingy);
    	if (aThingy == NULL) {
    		DEBUG_OUT("ThingyList has invalid reference at index "<<i << " to id "<<aRefP->GetThingyID()<<"; Removed", DEBUG_ERROR | DEBUG_CONTAINMENT );
            RemoveItemsAt(1, i);
    	} else if (mIsShipsOnly) {
    	    if ( ! ValidateShip(aThingy) ) {
        		DEBUG_OUT("Ship Only ThingyList has reference at index "<<i << " to non-ship item "<<aThingy<<"; Removed", DEBUG_ERROR | DEBUG_CONTAINMENT );
                RemoveItemsAt(1, i);
    	    }
    	}
	}  
}

