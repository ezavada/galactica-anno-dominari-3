// =============================================================================
// CDeleteList.cp                     ©1996-2002, Sacred Tree Software, inc.
// 
// CDeleteList.cp contains methods needed to maintain a list of deleted records
// in database file for a database management system. 
//
// General Purpose Access Methods:
//	UInt32		FindDeletedSlot(SInt32 *inNeededSlotSize = 0);
//							locates a free slot in and returns its position,
//							removes that slot from list of free slots
//	void		SlotWasDeleted(UInt32 slotPos, SInt32 inSlotSize = 0);
//							adds a newly deleted slot to the list of free slots
//
// version 1.5.8
//
// created:   7/29/95, ERZ
// modified:  5/30/96, ERZ  Got rid of gLongComp global which was causing crashes
// modified:  7/24/96, ERZ	Ver 1.5, cross platform handles
// modified:   9/5/96, ERZ	DebugNew support, uses new macro instead of new
// modified:  11/9/97, ERZ	classifying debug output as DEBUG_ERROR or DEBUG_TRIVIA
// modified:  5/27/02, ERZ  v1.5.8, converted to bool from MacOS Boolean, removed class typedefs
//
// =============================================================================

#include "CDeleteList.h"


UInt32 CalcItemSize(bool inTrackRecSize);
LLongComparator* MakeNewComp();


UInt32 
CalcItemSize(bool inTrackRecSize) {
	if (inTrackRecSize) {
		return (sizeof(DeleteListEntryT));
	} else {
		return (sizeof(UInt32));
    }
}

LLongComparator*
MakeNewComp() {
	return new LLongComparator();
}

CDeleteList::CDeleteList(bool inTrackRecSize)
	: LArray(CalcItemSize(inTrackRecSize), MakeNewComp(), inTrackRecSize) {	//only sort if sized
	
	mTrackRecSize = inTrackRecSize;
}

#if DB_V15
CDeleteList::CDeleteList(handle inListData, bool inTrackRecSize)
	: LArray(CalcItemSize(inTrackRecSize), inListData, MakeNewComp(), inTrackRecSize, inTrackRecSize) {	//only sort if sized
	
	mTrackRecSize = inTrackRecSize;
}
#else
#if ( defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX )) && ! defined( __PORTABLE_POWER_PLANT__)
CDeleteList::CDeleteList(Handle inListData, bool inTrackRecSize)
	: LArray(CalcItemSize(inTrackRecSize), inListData, MakeNewComp(), inTrackRecSize, inTrackRecSize) {	//only sort if sized
	
	mTrackRecSize = inTrackRecSize;
}
#else 
CDeleteList::CDeleteList(char* inListData, size_t inListDataSize, bool inTrackRecSize)
	: LArray(CalcItemSize(inTrackRecSize), inListData, inListDataSize, MakeNewComp(), inTrackRecSize, inTrackRecSize) {	//only sort if sized
	
	mTrackRecSize = inTrackRecSize;
}
#endif // PLATFORM_MACOS
#endif // DB_V15

// returns 0 if no suitable item found, or the slot position if found

UInt32
CDeleteList::FindDeletedSlot(SInt32 inNeededSlotSize) {
	UInt32 result = 0;
	DeleteListEntryT entry;
	entry.slotSize = 0;
	if (GetCount() > 0) {
		UInt32 idx = mItemCount;
		if (mTrackRecSize)	{	// entry of same size or within kSlotSizeSlop larger than requested
			entry.slotSize = inNeededSlotSize;		//find the item with the given slot size
			idx = FetchInsertIndexOf(&entry);
			if (idx > mItemCount) {
				return 0;		// no item was found that was big enough
			} else {
				PeekItem(idx, &entry);
				if (entry.slotSize <= (inNeededSlotSize + kSlotSizeSlop) ) {
					result = entry.slotPos;
				} else {
					return 0;	// all big enough items found were also too big
				}
			}
		} else {
			PeekItem(idx, &result);	// no size stored, just grab the last entry
		}
		RemoveItemsAt(1, idx);
	}
	DB_DEBUG("DeleteList::FindDeletedSlot("<<inNeededSlotSize<<"): "<<result<<"; size: "<<entry.slotSize<<" cnt: "<<GetCount(), DEBUG_TRIVIA);
	return result;
//	return 0;	// *** this was a hack to fix problems with the game ***
	/* The problem with the delete list is not actually a problem with the list itself, it is
		a concurrency problem. Since Galactica was using shared files and the delete list is
		maintained in memory, the data doesn't stay accurate between list when multiple clients
		are accessing the database.
		
		There are several ways to fix this:
		1. Have a shared access mode for the delete list that always dumps its data out to
			a shared file and reads it in from the shared file.
		2. Don't use shared files, so there is no concurrency problem. This is the way
		    Galactica has resolved the problem.
	*/
}

void
CDeleteList::SlotWasDeleted(UInt32 inSlotPos, SInt32 inSlotSize) {
	void *p;
	DeleteListEntryT entry;
	if (mTrackRecSize) {
		entry.slotSize = inSlotSize;
		entry.slotPos = inSlotPos;
		p = &entry;	// slot should be added in sorted order in list
		// *** need to merge with adjacent slots
	  #ifdef DEBUG
		#warning FIXME: SlotWasDeleted does not merge with adjacent slots
	  #endif
	} else {
		p = &inSlotPos;	// slot should be added to end of list
	}
	DB_DEBUG("SlotWasDeleted(); pos: "<<inSlotPos<<"; size: "<<inSlotSize<<" cnt: "<<GetCount(), DEBUG_TRIVIA);
	InsertItemsAt(1, index_Last, p);	// add it now		
}

// used for validation
bool
CDeleteList::CheckDeletedSlot(UInt32 inSlotPos, SInt32 inSlotSize) {
	DeleteListEntryT entry;
	entry.slotSize = 0;
	for (UInt32 i = 1; i <= mItemCount; i++) {
	    PeekItem(i, &entry);
	    if (inSlotPos == entry.slotPos) {
	        if (mTrackRecSize) {
	            if (inSlotSize == entry.slotSize) {
	                // we found a deleted slot that matches in size and position
	                return true;
	            } else {
	                DB_LOG("INTEGRITY CHECK ERROR: Found delete list entry for slot "
	                        << inSlotPos << " with size " << entry.slotSize
	                        << " but was expecting slot size to be " << inSlotSize);
	               #warning TODO: Repair problem in delete list
	            }
	        } else {
	            // we found a deleted slot that matches in position (size is fixed)
	            return true;
	        }
	    }
	}
	// no delete list entry matched in position and size
	return false;
}

bool
CDeleteList::FindFirstDeletedSlotFromDatabasePos(UInt32 inStartPos, UInt32 &outFoundSlotPos) {
	outFoundSlotPos = 0xFFFFFFFF;
	DeleteListEntryT entry;
	for (UInt32 i = 1; i <= mItemCount; i++) {
	    PeekItem(i, &entry);
	    if ( (entry.slotPos >= inStartPos) && (entry.slotPos < outFoundSlotPos) ) {
	        outFoundSlotPos = entry.slotPos;
	    }
	}
	return (outFoundSlotPos != 0xFFFFFFFF);
}

