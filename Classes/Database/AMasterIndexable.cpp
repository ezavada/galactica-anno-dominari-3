// =============================================================================
// AMasterIndexable.cp                       ©1995-2002, Sacred Tree Software, inc.
// 
// Abstract mix-in class for items that use a master index.
//
//  There is only one main method that your subclasses MUST override:
//
// SInt32 AddNewEmptySlot(SInt32 inSize)		is called by the master index when
//	it decides an item in the indexed file needs to be added because there are no
//	free slots available in the indexed file. It should be overridden to add a new
//	empty slot of the size specified and return the position it was added.
//
//	There are also two other methods your subclass may need to override if they
//	use variable sized items:
//
// SInt32 GetSlotSize(SInt32 inSlotPos)		is called by the index when it needs
//	to see if an item has outgrown its old slot. It should be overridden to return
//	the size allocated to the slot located at inSlotPos in the indexed file. If
//	all your slots are the same size you don't need to override this.
//
// void MarkSlotDeleted(SInt32 inSlotPos)	is called by the index when it has
//	choosen a new slot for an existing item because the item has outgrown its old
//	slot. This is only called from CMasterIndex::UpdateEntry() when sizes are changed,
//	so if you are using fixed size items you don't need to override this.  		
//
// version 1.5.0
//
// created:   7/24/95, ERZ
// modified:   2/2/96, ERZ	Added inTrackRec size to constructor to automate rec size handling
// modified:  3/29/96, ERZ	Removed inTrackRecSize and made IsFixedSize pure virtual
//							and removed destructor
// modified:  7/25/96, ERZ	Vers 1.5, Added default constucture to simplify initialization
// modified:  5/27/02, ERZ  v1.5.8, converted to bool from MacOS Boolean, removed class typedefs
//
// =============================================================================

#include "AMasterIndexable.h"

#ifndef DATABASE_V15

AMasterIndexable::AMasterIndexable() {
	itsMasterIndex = (AMasterIndex*)nil;
}

#endif

AMasterIndexable::AMasterIndexable(AMasterIndex *inMasterIndex) {
	itsMasterIndex = inMasterIndex;
	if (nil == inMasterIndex)
		Throw_( dbIndexRequired );
	if (itsMasterIndex)
		itsMasterIndex->SetIndexable(this);
}
	
void
AMasterIndexable::SetMasterIndex(AMasterIndex *inMasterIndex) {
	itsMasterIndex = inMasterIndex;
	if (nil == inMasterIndex)
		Throw_( dbIndexRequired );
	itsMasterIndex->UseIndexable(this);
}

SInt32 
AMasterIndexable::GetSlotSize(UInt32) { // inSlotPos
	return 0;
}

RecIDT  
AMasterIndexable::GetRecordAtSlot(UInt32) { // inSlotPos
    // does nothing, override to return id number of record in slot at inSlotPos
    return kInvalidRecID;
}

void 
AMasterIndexable::MarkSlotDeleted(UInt32, RecIDT) { // inSlotPos
}
