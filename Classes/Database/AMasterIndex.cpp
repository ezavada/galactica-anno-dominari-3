// =============================================================================
// AMasterIndex.cp                     й1995-2002, Sacred Tree Software, inc.
// 
// AMasterIndex.cp contains methods needed for an abstract class of
// index file objects for a database management system. It's a mix-in class
// intended to be mixed with a LStream subclass
//
// depends upon UMemoryMgr.h, and UException.h
// UMemoryMgr are is OS Dependant
// only OS dependancy in this file is use of handles in ShiftItems()
//
// General Purpose Access Methods:
//	SInt32		FindEntry(RecIDT inRecID, SInt32 *outRecSize = NULL)
//							returns the position and size of specified record
//	SInt32		AddEntry(RecIDT inRecID, SInt32 inRecSize = 0)
//							adds an entry for the specified rec (pos & size)
//							returns final record position
//	SInt32		UpdateEntry(RecIDT inRecID, SInt32 inNewRecSize)
//							updates entry size info for the specified rec
//							returns final record position
//	SInt32		DeleteEntry(RecIDT inRecID)
//							marks the entry for the specified rec as deleted
//							returns position of deleted slot
//
// Other Useful Methods:
//	bool		SetBatchMode(bool inBatchMode)	** NOT THREAD SAFE **
//							prevents resorting of index during batch operations
//	SInt32		GetEntryCount() const
//							returns the number of valid entries in the index
//	bool		FetchEntryAt(SInt32 inAtIndex, IndexEntryT &outEntry)
//							returns the Nth index entry
//
// version 1.5.8
//
// created:   7/29/95, ERZ
// modified:  1/30/96, ERZ	CW8 mods, Added use of CDeleteList class, so the master index 
//								is now permanently sorted and contains only valid entries
// modified:   2/2/96, ERZ  Removed unused methods and fields, now does auto-detect of 
//								fixed vs. variable sized Indexables.
// modified:  4/10/96, ERZ	Added SetTypeAndVersion() method
// modified:  7/28/96, ERZ	v1.5, now uses cross platform handles
// modified:  8/14/96, ERZ	Thread support
// modified:   9/5/96, ERZ	DebugNew support, uses new macro instead of new
// modified: 12/27/96, ERZ	Added caching of last entry found
// modified:  2/21/98, ERZ	v1.5.4, fixed write to Read Only files bug
// modified:  9/28/98, ERZ	v1.5.5, debug cache hit counting only if DEBUG defined
// modified:  9/30/98, ERZ	v1.5.6, added check for out-of-sequence ids in AddRecord calls
// modified:  10/4/99, ERZ  v1.5.7, added ENABLE_INDEX_CACHE macro, and disabled it
// modified:  5/27/02, ERZ  v1.5.8, converted to bool from MacOS Boolean, removed class typedefs
//
// =============================================================================

#include <LStream.h>
#include <LArray.h>
#include <UException.h>

// uncomment the line below to enable caching of index entries.
// DO NOT do this if you have multiple processes accessing the database!!
#define ENABLE_INDEX_CACHE

#include "AMasterIndex.h"
#include "AMasterIndexable.h"

#define kCompResultLessThan		-1
#define kCompResultEqual     	0
#define kCompResultGreaterThan	1

#define kDeletedRecID	0

#define kAllocBlockSize		1024L			// number of index entries to allocate at once
#define kMinNewSlotSize		256L			// smallest number of bytes to make a new slot from
#define kSlotSizeExtra		16L				// add to each slot so record has room to grow

DEBUG_ONLY(
  long gDebugCache_Hits = 0;
  long gDebugCache_Misses = 0;
  long gDebugCache_NotFounds = 0;
)

// these must be packed structures
#define PACKED_STRUCT
#if defined( __MWERKS__ )
    #pragma options align=mac68k
#elif defined( _MSC_VER )
    #pragma pack(push, 2)
#elif defined( __GNUC__ )
	#undef PACKED_STRUCT
	#define PACKED_STRUCT __attribute__((__packed__))
#else
	#pragma pack()
#endif

typedef struct MasterIndexHeaderT {
//	OSType	typeCode;		these two are actually part of the header but not read in with this
//	UInt32	versionCode;	struct
	Bool8	fileOpen;
    char    UNUSED_PAD;
	SInt32	itemSize;
	UInt32	itemCount;
	UInt32	allocatedSlots;
	SInt32	firstItemPos;
	UInt32	numValidEntries;
	UInt32	deleteListStart;		//delete list is written at end of stream
} PACKED_STRUCT MasterIndexHeaderT;

#if defined( __MWERKS__ )
    #pragma options align=reset
#elif defined( _MSC_VER )
    #pragma pack(pop)
#endif

AMasterIndex::AMasterIndex() {	// ** NOT THREAD SAFE **
	mTrackRecSize = false;		// initial default value, set for real by UseIndexable()
	mItemSize = 0;				// initial value, set for real by UseIndexable()
	mItemCount = 0;
	mAllocatedSlots = 0;
	mFirstItemPos = sizeof(MasterIndexHeaderT) + sizeof(OSType) + sizeof(UInt32);
	mNumValidEntries = 0;
	mBatchMode = false;
	mIndexOpen = false;
	itsStream = (LStream*)NULL;
	itsDeleteList = (CDeleteList*)NULL;
	mCachedEntry.recID = kInvalidRecID;
	mCachedEntryPos = 0;
	mReadOnly = false; // bug fix:  2/21/98, ERZ	v1.5.4, don't write to Read Only files
	mHighestRecID = 0;		// v1.5.4 addition, to check for out-of-sequence adds
  #if DB_THREAD_SUPPORT
  	if (UEnvironment::HasFeature(env_HasThreadsManager)) {
 		mAccessHeader = new LMutexSemaphore();
		mAccessData = new LMutexSemaphore();
		mChangeInfo = new LMutexSemaphore();
		ThrowIfNil_(mAccessHeader);
		ThrowIfNil_(mAccessData);
		ThrowIfNil_(mChangeInfo);
	} else {
		mAccessHeader = (LMutexSemaphore*)nil;
		mAccessData = (LMutexSemaphore*)nil;
		mChangeInfo = (LMutexSemaphore*)nil;
	}
  #endif
}

AMasterIndex::~AMasterIndex() {	// ** NOT THREAD SAFE **
  #if DB_THREAD_SUPPORT
  	if (UEnvironment::HasFeature(env_HasThreadsManager)) {
		delete mAccessHeader;
		delete mAccessData;
		delete mChangeInfo;
	}
  #endif
	if (itsDeleteList != NULL) {
		delete itsDeleteList;
	}
}

// only use after file has been opened
void	// ** NOT THREAD SAFE **
AMasterIndex::SetTypeAndVersion(OSType inType, UInt32 inVersion) {
	mStreamType = inType;
	mStreamVersion = inVersion;
	itsStream->SetMarker( 0, streamFrom_Start);
	itsStream->WriteBlock(&mStreamType, sizeof(OSType));
	itsStream->WriteBlock(&mStreamVersion, sizeof(UInt32));
}

void	// ** NOT THREAD SAFE **
AMasterIndex::UseIndexable(AMasterIndexable *inIndexable) {
	itsIndexable = inIndexable;
	if (inIndexable) {
		mTrackRecSize = !itsIndexable->IsFixedSize();
		if (mTrackRecSize) {
			mItemSize = sizeof(IndexEntryT);
		} else {
			mItemSize = sizeof(IndexEntryT) - sizeof(SInt32);
		}
	}
}

UInt32	// ** Thread Safe **
AMasterIndex::GetEntryCount() const {
	return mNumValidEntries;
}

bool	// ** Thread Safe **
AMasterIndex::FetchEntryAt(UInt32 inAtIndex, IndexEntryT &outEntry) {
	if (inAtIndex <= (UInt32)mItemCount) {
		PeekEntry(inAtIndex, outEntry);
		return true;
	} else {
		return false;
	}
}

UInt32	// ** Thread Safe **
AMasterIndex::FetchEntry(RecIDT inRecID, IndexEntryT &outEntry) {
	UInt32 entryPos;
	bool itemFound;
  #ifdef ENABLE_INDEX_CACHE
	if (mCachedEntry.recID == inRecID) {	// check the cached item first
		itemFound = true;
		entryPos = mCachedEntryPos;
		outEntry = mCachedEntry;
	    DEBUG_ONLY( gDebugCache_Hits++ );
	} else {
  #endif	// ENABLE_INDEX_CACHE
		itemFound = BinarySearch(inRecID, outEntry, entryPos);
		if (itemFound) {	// found the item, cache it
			mCachedEntry = outEntry;
			mCachedEntryPos = entryPos;
	        DEBUG_ONLY( gDebugCache_Misses++ );
		} else {
	        DEBUG_ONLY( gDebugCache_NotFounds++ );
		}
  #ifdef ENABLE_INDEX_CACHE
	}
  #endif	// ENABLE_INDEX_CACHE
	if (!itemFound) {
		Throw_(dbItemNotFound);		// not found is an exception
	}
	return entryPos;
}


UInt32	// ** Thread Safe **
AMasterIndex::FindEntry(RecIDT inRecID, SInt32 *outRecSize) {
	IndexEntryT entry;
	FetchEntry(inRecID, entry);
	if (NULL != outRecSize) {
		*outRecSize = entry.recSize;
	}
	return entry.recPos;
}


UInt32	// ** Thread Safe **
AMasterIndex::AddEntry(RecIDT inRecID, SInt32 inRecSize) {
	UInt32 entryPos;
	IndexEntryT entry;
	SInt32 neededSlotSize = 0;
	DB_DEBUG("Index::AddEntry("<<inRecID<<", "<<inRecSize<<")", DEBUG_TRIVIA);
	if (inRecID > mHighestRecID) {	// v1.5.4 addition, check for out-of-sequence adds
		mHighestRecID = inRecID;
	} else {
		DB_DEBUG("ERROR: Trying to add Index Entry for Rec "<<inRecID<<", but that ID is out of sequence.", DEBUG_ERROR);
		// out of sequence adds are a problem because this function assumes that every
		// item is added to the end of the index. If a smaller id gets added to the
		// end, this will cause some items to become unlocatable, since the index is
		// always presumed to be in sorted order.
		// If out of sequence adds are necessary, you can figure out where to put the
		// item and use ShiftItems to make space for it there.
		Throw_(dbInvalidID);
	}	// end v1.5.4 addition
	if (mTrackRecSize) {
		neededSlotSize = inRecSize + kSlotSizeExtra;
	}
	entryPos = AddSpace();	// add space in the index for a new entry
	{	// reduce scope for mutex variable so semaphore is released as soon as possible
	  #if DB_THREAD_SUPPORT
		StSafeMutex mutex(mChangeInfo);		// block changes to the internal object data
	  #endif
		entry.recPos = itsDeleteList->FindDeletedSlot(neededSlotSize);	// look for a deleted slot in indexed thing
	}
	if (entry.recPos == 0) {		// no empty slots available? Add one.
		entry.recPos = itsIndexable->AddNewEmptySlot(neededSlotSize);	// add slot in indexed thing
	}
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mChangeInfo);		// block changes to the internal object data
  #endif
	mNumValidEntries++;				// we added a valid entry
	entry.recID = inRecID;			// make sure new entry has correct record id
	entry.recSize = inRecSize;		// and store the allocation block size
	PokeEntry(entryPos, entry);		// write the entry
	if (!mBatchMode) {
		WriteHeader();				// update the header while we're at it		
	}
	return entry.recPos;			// return the recPos we saved in the index entry
}


UInt32	// ** Thread Safe **
AMasterIndex::DeleteEntry(RecIDT inRecID) {
	IndexEntryT entry;
	UInt32 entryPos = FetchEntry(inRecID, entry);
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mChangeInfo);		// block changes to the internal object data
  #endif
	DB_DEBUG("Index::DeleteEntry("<<inRecID<<") index entry pos: "<<entryPos, DEBUG_TRIVIA);
	if (mTrackRecSize) {
		entry.recSize = itsIndexable->GetSlotSize(entry.recPos);	// put the slot size in the entry
		itsDeleteList->SlotWasDeleted(entry.recPos, entry.recSize);
	} else {
		itsDeleteList->SlotWasDeleted(entry.recPos);
	}
	ShiftItems(entryPos + 1, index_Last, entryPos);	// get rid of deleted index entry
	mNumValidEntries--;				// we removed a valid index entry
	mItemCount--;
	if (!mBatchMode) {
		WriteHeader();				// update the header while we're at it
	}
	return entry.recPos;			// return the recPos of the deleted slot
}


UInt32	// ** Thread Safe **
AMasterIndex::UpdateEntry(RecIDT inRecID, SInt32 inNewRecSize) {
	IndexEntryT entry;
	UInt32 entryPos = FetchEntry(inRecID, entry);				// locate existing entry by ID
	UInt32 recPos = entry.recPos;
	DB_DEBUG("Index::UpdateEntry("<<inRecID<<", "<<inNewRecSize<<")", DEBUG_TRIVIA);
	if (entry.recSize == inNewRecSize) {
		return recPos;											// no change to size, return position
	}
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessData);		// block changes to the data
  #endif
	SInt32 slotSize = itsIndexable->GetSlotSize(recPos);			// find available size of old slot
	SInt32 neededSlotSize = inNewRecSize - 4 + kSlotSizeExtra;	// ignore recPos, leave extra space
	if ((inNewRecSize-4) > slotSize) {							// bigger: must find or create slot
		{	// reduce scope for mutex variable so semaphore will be released at proper time
		  #if DB_THREAD_SUPPORT
			StSafeMutex mutex(mChangeInfo);		// block changes to the internal object data
		  #endif
			itsDeleteList->SlotWasDeleted(recPos, slotSize);		// save the deleted slot pos & size
			entry.recPos = itsDeleteList->FindDeletedSlot(inNewRecSize);	// look for a deleted slot in indexed thing
          #if DB_DEBUG_MODE || DB_INTEGRITY_CHECKING
            if (entry.recPos) { // confirm that the slot is actually empty
                RecIDT slotRecID = itsIndexable->GetRecordAtSlot(entry.recPos);
                if (slotRecID != kInvalidRecID) {
                    DB_LOG("ERROR: Updating Rec "<<inRecID<<" tried BAD RELOC pos: "<<entry.recPos<<", overwrites Rec "<<slotRecID);
                    DB_DEBUG("ERROR: While updating "<<inRecID<< ", delete list gave slot "<<entry.recPos<<" which appears to contain "<<slotRecID, DEBUG_ERROR);
                    entry.recPos = 0;
                }
            }
          #endif
		}
		itsIndexable->MarkSlotDeleted(recPos, inRecID);					// mark the old slot itself, too
		if (entry.recPos == 0) {		// no empty slots available? Add one.
			entry.recPos = itsIndexable->AddNewEmptySlot(neededSlotSize);	// add slot in indexed thing
		}
		entry.recID = inRecID;			// make sure new entry has correct record id
		entry.recSize = inNewRecSize;	// and store the allocation block size
		PokeEntry(entryPos, entry);		// write the entry
		recPos = entry.recPos;			// make sure we return the updated position
	}
	else {														// the record still fits
		entry.recSize = inNewRecSize;							// save new size in entry for later
		PokeEntry(entryPos, entry);								// write out updated entry
/*		if (!mBatchMode) {										// don't check space during batch
			if ( (slotSize - neededSlotSize) >= kMinNewSlotSize) {	// is there space left over?
				UInt32 slotPos = recPos + neededSlotSize;
				{	// reduce scope so semaphore is released ASAP
				  #if DB_THREAD_SUPPORT
					StSafeMutex mutex(mChangeInfo);		// block changes to the internal object data
				  #endif
					itsDeleteList->SlotWasDeleted(slotPos, slotSize - neededSlotSize); // save the deleted slot pos & size
				}
				itsIndexable->MarkSlotDeleted(slotPos);		// mark the slot itself, too
				WriteHeader();									// and finally, update the header
			}
		}		*/
	}
	if (mCachedEntry.recID == inRecID) {	// update record entry cache
		mCachedEntry = entry;			    // position of entry doesn't change
	}                                   
	DB_DEBUG("exit Index::UpdateEntry("<<inRecID<<", "<<inNewRecSize<<")", DEBUG_TRIVIA);
	return recPos;						// return position of valid record (relocated or not)
}


bool	// ** NOT THREAD SAFE **
AMasterIndex::SetBatchMode(bool inBatchMode) {
	bool oldBatch = mBatchMode;
	mBatchMode = inBatchMode;
	if (!mBatchMode) {
		WriteHeader();	// write the header no that we are done with batch mode
    }
	return oldBatch;
	// **** do something with delete list, like sort it or turn off KeepSorted?
}



bool	// ** Thread Safe **
AMasterIndex::BinarySearch(RecIDT inRecID, IndexEntryT &outEntry, UInt32 &outEntryPos) {
	bool itemFound = false;
	if (0 == mItemCount) {
		outEntryPos = 1;
	} else {
		UInt32 currentIndex;
		UInt32 lowBound = 1;
		UInt32 highBound = mItemCount;
	  #if DB_THREAD_SUPPORT
		StSafeMutex mutex(mAccessData);		// block changes to the data
	  #endif
		do {
			currentIndex = (lowBound + highBound) >> 1;		// get the item index
			PeekEntry(currentIndex, outEntry);			// load the item
			if (outEntry.recID == inRecID) {
				itemFound = true;
			} else {
				if (outEntry.recID <= inRecID) {
					lowBound = currentIndex + 1;
				} else {
					highBound = currentIndex - 1;
				}
			}
		} while (!(itemFound || (lowBound > highBound)));
//		if (outEntry.recID <= inRecID)	// this will adjust the return result to always
//			currentIndex += 1;			// be the proper insert position
		outEntryPos = currentIndex;
	}
	return itemFound;
}


UInt32	// ** Thread Safe **
AMasterIndex::AddSpace() {
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mChangeInfo);
  #endif
	mItemCount++;
	SInt32 newSlots = (mItemCount/kAllocBlockSize + 1) * kAllocBlockSize; // allocate in blocks
	if (mAllocatedSlots < newSlots) {		//# allocated slots changed?
		UInt32 fileSize = mFirstItemPos + newSlots * mItemSize;
		itsStream->SetLength(fileSize);		// expand file еее CW8
		mAllocatedSlots = newSlots;			// write new # slots in disk file header
		if (!mBatchMode) {
			WriteHeader();					// don't call subclasses, just write num slots
		}
	}
	return mItemCount;
}


void	// ** Thread Safe **
AMasterIndex::ShiftItems(UInt32 inStartPos, UInt32 inEndPos, UInt32 inDestPos) {
	if (inEndPos == index_Last) {
		inEndPos = mItemCount;
	}
    DB_DEBUG("AMasterIndex::ShiftItems start "<<inStartPos<<" end "<<inEndPos<<" to "<<inDestPos, DEBUG_TRIVIA);
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex1(mAccessData);
	StSafeMutex mutex2(mChangeInfo);
  #endif
	mCachedEntry.recID = kInvalidRecID;	// invalidate entry position cache
	long bytesToMove = (inEndPos - inStartPos + 1) * mItemSize;		//calc bytes to move
    if (bytesToMove == 0) {
        return; // nothing to do
    }

	long bytesMoved = 0;
	long bytesRemaining;
	UInt32 srcFilePos;
	UInt32 dstFilePos;

	// allocate buffer 64k only: Apple Guidelines say this size best for Copland compatablity
	long buffSize = (bytesToMove > 65536) ? 65536 : bytesToMove;
	char* buffer = (char*)std::malloc(buffSize);
	DEBUG_ONLY (
	    if (buffer == 0) {
	        DB_DEBUG("failed to malloc "<<buffSize<<" byte buffer. Will throw exception 'nilP'", DEBUG_ERROR);
	    }
	)
	ThrowIfNil_(buffer);

	// move the data 
	while (bytesMoved < bytesToMove) {
		bytesRemaining = bytesToMove - bytesMoved;
		if (buffSize > bytesRemaining) {
			 buffSize = bytesRemaining;
		}
		srcFilePos = mFirstItemPos + (inStartPos - 1) * mItemSize;
		dstFilePos = mFirstItemPos + (inDestPos - 1) * mItemSize;
		if (inStartPos > inDestPos) {		// handle possible overlap while shifting toward front
			  srcFilePos += bytesMoved;
			  dstFilePos += bytesMoved;
		}
		else {	// handle possible overlap while shifting towards back
			srcFilePos += bytesRemaining - buffSize;
			dstFilePos += bytesRemaining - buffSize;
		}
		itsStream->SetMarker(srcFilePos , streamFrom_Start); // ***CW8 mods
		itsStream->ReadBlock(buffer, buffSize );
		itsStream->SetMarker(dstFilePos, streamFrom_Start);
		itsStream->WriteBlock(buffer, buffSize );			// *** end CW8 mods
		bytesMoved += buffSize;
	}
	std::free(buffer);
}


bool	// ** NOT THREAD SAFE **
AMasterIndex::ReadHeader() {
	MasterIndexHeaderT tempHeader;
	itsStream->SetMarker(0, streamFrom_Start);
	*itsStream >> mStreamType;
	*itsStream >> mStreamVersion;
	itsStream->ReadBlock(&tempHeader, sizeof(MasterIndexHeaderT) ); // endian swapping below
	mItemSize = BigEndian32_ToNative(tempHeader.itemSize);
	mTrackRecSize = (mItemSize == sizeof(IndexEntryT));
	mItemCount = BigEndian32_ToNative(tempHeader.itemCount);
	mAllocatedSlots = BigEndian32_ToNative(tempHeader.allocatedSlots);
	mFirstItemPos = BigEndian32_ToNative(tempHeader.firstItemPos);
	mNumValidEntries = BigEndian32_ToNative(tempHeader.numValidEntries);
	if ((itsDeleteList == NULL) && (tempHeader.deleteListStart != 0xffffffff)) { // *** deletion list support, 1/26/96 ERZ
      #if PLATFORM_MACOS
		SInt32 oldPos = itsStream->GetMarker();	//save original file position marker
	  #endif // PLATFORM_MACOS
		SInt32 deleteListStart = BigEndian32_ToNative(tempHeader.deleteListStart);
		itsStream->SetMarker(deleteListStart, streamFrom_Start);
	  #if DB_V15
		handle theDeleteListData;	// avoid using MacSpecific call here
		long hSize;
		*itsStream >> hSize;
		if (hSize != 0) {
			theDeleteListData.resize(hSize);
			theDeleteListData.lock();
			itsStream->ReadBlock(*theDeleteListData, hSize);
			theDeleteListData.unlock();
		}
		itsDeleteList = new CDeleteList(theDeleteListData, mTrackRecSize);
	  #elif PLATFORM_MACOS
		Handle theDeleteListData;	//*** MACOS
		itsStream->ReadHandle(theDeleteListData);	// read in the stored deletion list
		itsStream->SetMarker(oldPos, streamFrom_Start);	//restore file position marker
		itsDeleteList = new CDeleteList(theDeleteListData, mTrackRecSize);
      #else
      #warning FIXME: not restoring saved Delete List for Win32
        itsDeleteList = new CDeleteList();
	  #endif
	}
	return (tempHeader.fileOpen);
}

void	// ** Thread Safe UNLESS inFileOpen == false **
AMasterIndex::WriteHeader(bool inFileOpen) {
	if (mReadOnly) {
		return; // bug fix:  2/21/98, ERZ	v1.5.4, don't write to Read Only files
    }
	MasterIndexHeaderT tempHeader;
	tempHeader.fileOpen = Native_ToBigEndian32(inFileOpen);
	tempHeader.itemSize = Native_ToBigEndian32(mItemSize);
	tempHeader.itemCount = Native_ToBigEndian32(mItemCount);
	tempHeader.allocatedSlots = Native_ToBigEndian32(mAllocatedSlots);
	tempHeader.firstItemPos = Native_ToBigEndian32(mFirstItemPos);
	tempHeader.numValidEntries = Native_ToBigEndian32(mNumValidEntries);
	UInt32 fileSize = mFirstItemPos + mAllocatedSlots * mItemSize;
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessHeader);
  #endif
	DB_DEBUG("Index::WriteHeader("<< (int)inFileOpen << "); fileLen = "<<fileSize, DEBUG_TRIVIA);
	itsStream->SetLength(fileSize);		// make sure we have the correct size index
	if ((!inFileOpen) && (itsDeleteList != NULL)) {	// don't save the delete list until we close file
		tempHeader.deleteListStart = Native_ToBigEndian32(itsStream->GetLength());	// *** deletion list support, 1/26/96 ERZ
		itsStream->SetMarker( tempHeader.deleteListStart, streamFrom_Start);
	  #if DB_V15
		handle h = itsDeleteList->GetItemsHandle();	// don't use MacOS specific handle
		*itsStream << (long) h.size();				// *** change this when we rewrite stream class
		if (h != nil) {
			h.lock();
			itsStream->WriteBlock(*h, h.size());
			h.unlock();
		}
	  #elif PLATFORM_MACOS
		itsStream->WriteHandle( itsDeleteList->GetItemsHandle());	
	  #endif
	} else {
		tempHeader.deleteListStart = Native_ToBigEndian32(0xffffffffL);	// *** end delete list support
	}
	itsStream->SetMarker(sizeof(OSType)+sizeof(UInt32), streamFrom_Start);// ***cw8
	itsStream->WriteBlock(&tempHeader, sizeof(MasterIndexHeaderT) );// ***cw8
}

// one based index
void	// ** Thread Safe **
AMasterIndex::PeekEntry(UInt32 inAtIndex, IndexEntryT &outEntry) {
	UInt32 filePos = mFirstItemPos + (inAtIndex - 1) * mItemSize;
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessData);
  #endif
	itsStream->SetMarker( filePos, streamFrom_Start);
	itsStream->ReadBlock(&outEntry, mItemSize );  // endian swapping below
  #if PLATFORM_LITTLE_ENDIAN
    outEntry.recID = BigEndian32_ToNative(outEntry.recID);
    outEntry.recPos = BigEndian32_ToNative(outEntry.recPos);
    outEntry.recSize = BigEndian32_ToNative(outEntry.recSize);
  #endif // PLATFORM_LITTLE_ENDIAN
}

void	// ** Thread Safe **
AMasterIndex::PokeEntry(UInt32 inAtIndex, const IndexEntryT &inEntry) {
	if (index_Last == inAtIndex) {
		inAtIndex = mItemCount;
	}
	UInt32 filePos = mFirstItemPos + (inAtIndex - 1) * mItemSize;
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessData);
  #endif
	DB_DEBUG("Index::PokeEntry("<<inAtIndex<<"), filePos: "<<filePos, DEBUG_TRIVIA);
	itsStream->SetMarker( filePos, streamFrom_Start);	// *** CW8 mods
	IndexEntryT e;
	e.recID = Native_ToBigEndian32(inEntry.recID);
	e.recPos = Native_ToBigEndian32(inEntry.recPos);
	e.recSize = Native_ToBigEndian32(inEntry.recSize);
	itsStream->WriteBlock(&e, mItemSize );		// *** end mods
}

bool
AMasterIndex::FindFirstSlotFromDatabasePos(SInt32 inStartPos, SInt32 &outFoundSlotPos) {
    UInt32 firstDeleted = 0;
    if (itsDeleteList->FindFirstDeletedSlotFromDatabasePos(inStartPos, firstDeleted) ) {
        outFoundSlotPos = firstDeleted;
    } else {
        outFoundSlotPos = 0x7FFFFFFFL;
    }
    IndexEntryT entry;
    for (int i = 1; i<mItemCount; i++) {
        PeekEntry(i, entry);
        if ( (entry.recPos > inStartPos) && (entry.recPos < outFoundSlotPos) ) {
            outFoundSlotPos = entry.recPos;
        }
    }
    return (outFoundSlotPos != 0x7FFFFFFFL);
}

bool
AMasterIndex::CheckDeletedSlot(SInt32 inSlotPos, SInt32 inSlotSize) {
    return itsDeleteList->CheckDeletedSlot(inSlotPos, inSlotSize);
}
	

bool	// ** NOT THREAD SAFE **
AMasterIndex::Open() {
	bool result;
	UseIndexable(itsIndexable);
	if (!mIndexOpen && HeaderExists()) {
		result = ReadHeader();
		if (mNumValidEntries > 0) {		// v1.5.4 addition, to check for out-of-sequence adds
			IndexEntryT entry;
			PeekEntry(mNumValidEntries, entry);	// find out the highest record ID used
			mHighestRecID = entry.recID;
		}
	} else {
		result = false;
	}
	DB_LOG("Index Header: itemSize ["<<mItemSize<<"] itemCount ["<<mItemCount
            << "] allocatedSlots ["<<mAllocatedSlots<<"] firstItemPos ["<<mFirstItemPos
            << "] numValidEntries ["<<mNumValidEntries<<"] highestRecID ["<<mHighestRecID<<"]");
	WriteHeader(kFileIsOpen);	//this should mark stream as open
	if (itsDeleteList == NULL) {
		itsDeleteList = new CDeleteList(mTrackRecSize);	
	}	
	mIndexOpen = true;			//if it is a newly created index, itemSize = 0;
	return result;
}

void	// ** NOT THREAD SAFE **
AMasterIndex::Close() {
	mBatchMode = false;
	if (mIndexOpen) {
		WriteHeader(kFileIsClosed);
    	DB_LOG("Index Header: itemSize ["<<mItemSize<<"] itemCount ["<<mItemCount
	            << "] allocatedSlots ["<<mAllocatedSlots<<"] firstItemPos ["<<mFirstItemPos
	            << "] numValidEntries ["<<mNumValidEntries<<"] highestRecID ["<<mHighestRecID<<"]");    
	}
	mIndexOpen = false;
}
