// =============================================================================
// CVarDataFile.cp							 й1995-2002, Sacred Tree Software, inc.
// 
// Class for variable record length database files. Inherites from CDataFile
// Please refer to CDataFile & ADataStore for additional info
//
// Access:
// Has the following methods defined for access:
//	 SInt32		GetRecordCount();
//	 void		ReadRecord(DatabaseRec);
//	 bool   	FindRecord(DatabaseRec, LComparator);
//	 void		AddRecord(DatabaseRec);
//	 void		UpdateRecord(DatabaseRec);
//	 void		DeleteRecord(SInt32);
//	 SInt32		GetRecordSize(SInt32);
// Plus can be accessed sequentially as iterated list using:
//	 void		AttachIterator(LArrayIterator *inIterator);
//	 void		DetachIterator(LArrayIterator *inIterator);
//
// DatabaseRec is a structure that begins with recPos, recSize, recID, and has
// a other indeterminate data in recData. 
//
// LComparator is a PowerPlant object, see LComparator.cp for details
//
// Read is different than Find because Read has the id and fills in the data,
// and Find has the data and fills in the id that matches that data using the
// Comparator object specified.
//
// Different from ADataStore in that it REQUIRES an index file. This is because
// the overhead of linear searches on a non-indexed file would be too great.
//
// the BatchMode() function allows you to turn on and off a batch mode that
// defers all sorts until turned off. This allows batch additions and deletions
// to take place much faster. Some examples of this would be importing records
// or duplicating or deleting a set of records.
//
// The Read(), Update() and Delete() functions will all cause an exception if
// the requested record is not found. Find() will return false if not found.
//
// version 1.5.8
//
// created:   6/29/95, ERZ
// modified:  7/17/95, ERZ	Improved use of indexes and error handling
// modified:  7/18/95, ERZ	UpdateRecord() no longer calls Delete or Add
// modified:  7/25/95, ERZ	inherites from CDataFile, uses CMasterIndex
// modified: 11/18/95, ERZ	pass record pointer into GetNewRecordID() calls
// modified:  1/30/96, ERZ  CW 8 mods, mostly in LComparator related stuff
// modified:   3/3/96, ERZ  fixed initialize bug CVarDataFile(AliasHandle)
// modified:  3/29/96, ERZ	rewrote Initialization process
// modified:   4/3/96, ERZ	added override of FetchItemAt()
// modified:   6/6/96, ERZ	fixed bug in ReadRecord that ignored buffer size passed in
// modified:  7/24/96, ERZ	ver 1.5, simplified initialization process, platform independant
// modified:  11/9/97, ERZ	classifying debug output as DEBUG_ERROR or DEBUG_TRIVIA
// modified:  2/21/98, ERZ	ver 1.5.1, fixed write to Read Only files bug
// modified:  9/96/98, ERZ	ver 1.5.2, relaxed GetNewRecordID(), now permits multiple
//								calls before multiple adds.
// modified:  5/27/02, ERZ  v1.5.8, converted to bool from MacOS Boolean, removed class 
//                              typedefs, added CheckDatabaseIntegrity
// modified:  9/24/05, ERZ  v1.5.9, added Backup() to create backup version of files
//
// =============================================================================

#include <UException.h>

#include "DebugMacros.h"
#include "CVarDataFile.h"
#include "CMasterIndexFile.h"

#include <cstring>

#define kAllocationBlockSize	4096L	// allocate space in file in 4k blocks
#define kMinNewSlotSize			32L 	// min space needed to make new slot when a record shrinks

#define kVarDBFileSlotSizeOffset    0L  // offset from start of slot to location of slot size
#define kVarDBFileRecIDOffset       4L  // offset from start of slot to location of record ID
#define kVarDBFileRecDataOffset     8L  // offset from start of slot to location of record data

#define kSizeOfSlotSize             4L
#define kSizeOfRecID                4L

#define SlotSizeToRecSize(n)    (n+4L)
#define RecSizeToSlotSize(n)    (n-4L)
#define RecSizeToIOSize(n)      (n-8L)

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

typedef struct VarDataFileHeaderT {
	SInt32  allocatedBytes;	// space allocated to file
	SInt32  bytesUsed;		// space used in file
	SInt32  largestRecSize;	// size of the largest record in the datafile
	Bool8   fileIsDamaged;  // corruption of some kind has been detected
} PACKED_STRUCT VarDataFileHeaderT, *VarDataFileHeaderPtr, **VarDataFileHeaderHnd, **VarDataFileHeaderHdl;

#if defined( __MWERKS__ )
    #pragma options align=reset
#elif defined( _MSC_VER )
    #pragma pack(pop)
#endif


#if DB_V15	// include ver 1.5 stuff
CVarDataFile::CVarDataFile(SInt16 inStrcResID)
	: CDataFile(inStrcResID) {
	mItemSize = -1;								//variable length
	mAllocatedBytes = 0;
	mBytesUsed = 0;
	mLargestRecSize = 0;
	mAllocBlockSize = kAllocationBlockSize;		// use a number of bytes, not a number of slots
	mFileIsDamaged = false;
}
#else
CVarDataFile::CVarDataFile(AMasterIndex *inMasterIndex)
	: CDataFile(7, inMasterIndex) {	// size of 7 will make mItemSize = -1
	mAllocatedBytes = 0;
	mBytesUsed = 0;
	mLargestRecSize = 0;
	mAllocBlockSize = kAllocationBlockSize;		// use a number of bytes, not a number of slots
	mFileIsDamaged = false;
}
#endif	// no ver 1.5 extensions

bool
CVarDataFile::Open() {
    bool result = CDataFile::Open();
	DB_LOG("DataFile Header: lastRecID ["<<mLastRecID<<"] allocatedBytes ["<<mAllocatedBytes
	            <<"] bytesUsed ["<<mBytesUsed << "] largestRecSize ["<<mLargestRecSize
                <<"] itemSize ["<<mItemSize<<"] itemCount ["<<mItemCount
	            << "] allocatedSlots ["<<mAllocatedSlots<<"] firstItemPos ["<<mFirstItemPos
	            << "] numValidRecs ["<<mNumValidRecs<<"] allocBlockSize ["<<mAllocBlockSize
	            <<(const char*)(mFileIsDamaged ? "] DAMAGED" : "]"));
    CheckDatabaseIntegrity(kRepairProblems);
    return result;
}


void
CVarDataFile::Close() {
	CDataFile::Close();
	DB_LOG("DataFile Header: lastRecID ["<<mLastRecID<<"] allocatedBytes ["<<mAllocatedBytes
	            <<"] bytesUsed ["<<mBytesUsed << "] largestRecSize ["<<mLargestRecSize
                <<"] itemSize ["<<mItemSize<<"] itemCount ["<<mItemCount
	            << "] allocatedSlots ["<<mAllocatedSlots<<"] firstItemPos ["<<mFirstItemPos
	            << "] numValidRecs ["<<mNumValidRecs<<"] allocBlockSize ["<<mAllocBlockSize
	            <<(const char*)(mFileIsDamaged ? "] DAMAGED" : "]"));
}

// return true if checks pass (or repairs successful)
bool        
CVarDataFile::CheckDatabaseIntegrity(bool inRepairProblems) {
//  #warning never repairing problems in CheckDatabaseIntegrity
//    inRepairProblems = false;  // we don't know that this works yet
    bool bResult = true;
    bool bRepairsWereAttempted = false;
  #warning FIXME: extensive checks completely disabled until user feedback provided for this long process
    bool bExtensiveChecks = false; //mFileIsDamaged; // always do extensive checks if we've detected problems
    try {
        if (IsReadOnly() && inRepairProblems) {
            inRepairProblems = false;  // we don't know that this works yet
    		DB_LOG("Integrity Check Warning: Database is read only");
        }
        // load all items that are in the master index and validate them against the db contents
        long fileLen = GetLength();
        if ((mAllocatedBytes + mFirstItemPos) != fileLen) {
            DB_LOG("INTEGRITY CHECK ERROR: Header thought file was "<<mAllocatedBytes<<" bytes long, but file is really "
                    <<fileLen <<" bytes long");
            if (inRepairProblems) {
        		bRepairsWereAttempted = true;
                mAllocatedBytes = fileLen - mFirstItemPos;
                WriteHeader(kFileIsOpen);
    		    DB_LOG("Integrity Check Repair: Told header that file is " << fileLen << " bytes long");
            }
        }
        long bytesUsed = mBytesUsed;
        if ( (bytesUsed < 0) || (bytesUsed > fileLen)) {
            bytesUsed = fileLen;
        }
        long largestSize = mLargestRecSize;
        if ( (largestSize < 0) || (largestSize > bytesUsed) ) {
            largestSize = bytesUsed;
        }
        // load all items that are in the master index and validate them against the db contents
        // we track an actualLargestSize as well as the largestSize, since the later refects a maximum
        // value that is physically possible, whereas the former tracks what we've found as we go through
        // the records 
        long actualLargestRecSize = 0;
        long actualLargestSlotSize = 0;
        long numRecords = itsMasterIndex->GetEntryCount();
        long lastIndex = numRecords;
    	SInt32 slotSize = 0;
    	RecIDT recID = kInvalidRecord;
		SInt32 recPos = 0;
        IndexEntryT entry;
        for (int i = lastIndex; i>=1; --i) {
            bool bBadIndexEntry = false;
            bool bBadSlotSize = false;
            slotSize = 0;
    		try {
                itsMasterIndex->FetchEntryAt(i, entry);  // can throw dbItemNotFound or read errors
                DB_DEBUG("Checking index entry "<<i<<": Record ID "<<entry.recID<<" size "<<entry.recSize
                          <<" dbpos "<<entry.recPos, DEBUG_TRIVIA);
    		    SInt32 nextRecPos = 0;
    		    if (bExtensiveChecks) {
        		       // look at what the index thinks the next slot is
                    if ( !itsMasterIndex->FindFirstSlotFromDatabasePos(entry.recPos+1L, nextRecPos) ) {
                        // this is the last record, make sure we update bytesUsed as necessary
                        // this should only happen once, might want to check that
                        if (bytesUsed < (entry.recPos + entry.recSize)) {
                            bytesUsed = entry.recPos + entry.recSize;
                            if (bytesUsed > fileLen) {
                                bytesUsed = fileLen;
                            }
                        }
                        nextRecPos = bytesUsed;
                    }
                } else {
                    // since we aren't doing extensive checking, we don't actually know exactly where
                    // the next record starts, but we know it isn't allowed to be closer than recSize
                    // bytes away
                    nextRecPos = entry.recPos + entry.recSize;
                } // extensive checks
            	recID = kInvalidRecord;
            	SetMarker(entry.recPos + kVarDBFileSlotSizeOffset, streamFrom_Start);
            	ReadBlock(&slotSize, kSizeOfSlotSize);
                ReadBlock(&recID, kSizeOfRecID);
              #if PLATFORM_LITTLE_ENDIAN
                slotSize = BigEndian32_ToNative(slotSize);
                recID = BigEndian32_ToNative(recID);
              #endif // PLATFORM_LITTLE_ENDIAN
                if (recID != entry.recID) {
    		        DB_LOG("INTEGRITY CHECK ERROR: Bad Entry at index slot "<<i<<" dbpos "<<entry.recPos
    		                <<"; slot contains record id "<<recID<<" ("<<slotSize<<"B) but index expected record id "
    		                <<entry.recID<<" ("<<entry.recSize<<"B)");
    		        bBadIndexEntry = true;
                }
                SInt32 nextRecOffset = nextRecPos - entry.recPos;
                if ( (slotSize < RecSizeToSlotSize(entry.recSize)) || ((entry.recPos + slotSize) > fileLen) ) {
                	if (!bExtensiveChecks) {
                		// we aren't doing extensive checks, so we don't know where the index believes the
                		// next record needs to be. However, this information is crucial for a proper repair
                		// so we will figure it out now
	                    if ( !itsMasterIndex->FindFirstSlotFromDatabasePos(entry.recPos+1L, nextRecPos) ) {
	                        // this is the last record, make sure we update bytesUsed as necessary
	                        // this should only happen once, might want to check that
	                        if (bytesUsed < (entry.recPos + entry.recSize)) {
	                            bytesUsed = entry.recPos + entry.recSize;
	                            if (bytesUsed > fileLen) {
	                                bytesUsed = fileLen;
	                            }
	                        }
	                        nextRecPos = bytesUsed;
	                    }
                    }
    		        DB_LOG("INTEGRITY CHECK ERROR: Damaged database file, affects index entry "<<i<<" dbpos "
    		                <<entry.recPos<<"; slot for record id "<<recID<<" ("<<entry.recSize<<"B) is an impossible "
    		                <<slotSize<<" bytes, but it should be "<<nextRecOffset);
    		        bBadSlotSize = true;
    		        slotSize = nextRecOffset;
                } else if (bExtensiveChecks) {
                    // these checks rely on the nextRecOffset being correct, which it might not be if
                    // we aren't doing extensive checks. So we skip them.
                    if (slotSize > nextRecOffset) {
        		        DB_LOG("INTEGRITY CHECK ERROR: Bad Entry at index slot "<<i<<" dbpos "<<entry.recPos
        		                <<"; index thinks record id "<<recID<<" ("<<entry.recSize
        		                <<"B) is at that position, co-located with next record at dbpos "<<nextRecPos
        		                <<" leaving only "<<nextRecOffset<<" bytes in this slot");
        		        bBadIndexEntry = true;
        		        bBadSlotSize = true;
        		        slotSize = nextRecOffset;
                    } else if (slotSize < nextRecOffset) {
        		        DB_LOG("INTEGRITY CHECK ERROR: Minor damage to database file, affects index entry "<<i<<" dbpos "
        		                <<entry.recPos<<"; slot for record id "<<recID<<" ("<<entry.recSize<<"B) should be "
        		                <<nextRecOffset<<" bytes, not " << slotSize);
        		        bBadSlotSize = true;
        		        slotSize = nextRecOffset;
                    }
                }
                // update the largest record size if this was a good index entry
                if ( !bBadIndexEntry && (actualLargestRecSize < entry.recSize) ) {
                    actualLargestRecSize = entry.recSize;
                }
                // calculate the largest slot size
                if ( actualLargestSlotSize < slotSize ) {
                    actualLargestSlotSize = slotSize;
                }
                
    		} // end Try
    		catch (...) {
    		    DB_LOG("INTEGRITY CHECK ERROR: Exception reading entry at index slot "<<i<<" dbpos "<<entry.recPos);
    		    bBadIndexEntry = true;
    		} // end Catch
    		if (bBadIndexEntry) {
    		    bResult = false;
    		    if (inRepairProblems) {
    		        bRepairsWereAttempted = true;
		            itsMasterIndex->DeleteEntry(entry.recID);
    		        --numRecords;
		            DB_LOG("Integrity Check Repair: Deleted index entry for record " << entry.recID);
    		        // we don't mark the slot deleted because this could be part of an entirely 
    		        // different record that is valid. Overwriting a zero into four bytes of it
    		        // would be damaging, so we don't screw with it
		            /*
    		        	// slot size is okay, so this is 
	                	RecIDT deletedRecID = Native_ToBigEndian32(kInvalidRecord);
	                	SetMarker(entry.recPos + kVarDBFileRecIDOffset, streamFrom_Start);
	                	WriteBlock(&deletedRecID, kSizeOfRecID);
			            DB_LOG("Integrity Check Repair: Deleted index entry and slot for record " << entry.recID);
			        */
    		    }
    		} else if (bBadSlotSize) {
    			// in this case, only the slot size was wrong, but the record pointed to the right
    			// place, so we will try to update that. There is no guarantee that the data is good though,
    			// most likely this was stomped by something else, but at least this seems to have a valid
    			// record id tag
    		    bResult = false;
		        if (inRepairProblems) {
       		        bRepairsWereAttempted = true;
    	            SetMarker(entry.recPos + kVarDBFileSlotSizeOffset, streamFrom_Start);
    	            SInt32 outSize = Native_ToBigEndian32(slotSize);
    	            WriteBlock(&outSize, kSizeOfSlotSize);
	                DB_LOG("Integrity Check Repair: Updated slot size for record " << entry.recID
	                        << " at dbpos "<<entry.recPos<<" to " << slotSize << " bytes");
		        }
    		}
        }
        
        // only do this if extensive checks have been done, since we are relying on all the slot sizes being
        // correct and the stuff above may have set some of them incorrectly.

        if (bExtensiveChecks) {
            // now that we know that the index is good, go through the database file and check
            // each item to see if it has a corresponding index entry.
        	SetMarker(mFirstItemPos + kVarDBFileSlotSizeOffset, streamFrom_Start);	// move to start of the 1st record
        	while (true) {		// look at all the records in the datafile
            	slotSize = 0;
            	recID = kInvalidRecord;
        		recPos = GetMarker();
        		ReadBlock(&slotSize, kSizeOfSlotSize);		// read the slot size into the pointer
        		ReadBlock(&recID, kSizeOfRecID);	        // read the record id
              #if PLATFORM_LITTLE_ENDIAN
                slotSize = BigEndian32_ToNative(slotSize);
                recID = BigEndian32_ToNative(recID);
              #endif // PLATFORM_LITTLE_ENDIAN
        		// basic checks: the slot size and recID reasonable?
        		if ( (slotSize < 0) || (slotSize < (SInt32)RecSizeToSlotSize(sizeof(DatabaseRec))) 
        		  || ((recID != kInvalidRecord) && (slotSize > (actualLargestRecSize + kSlotSizeSlop))) ) {
        		    DB_LOG("INTEGRITY CHECK ERROR: Slot at dbpos "<<recPos<<" is impossible ("<<slotSize<<"B)");
        		    bResult = false;
        		    // scan index for the next record that comes right after this one.
        		    SInt32 nextRecPos = 0;
                    itsMasterIndex->FindFirstSlotFromDatabasePos(recPos + kVarDBFileRecDataOffset, nextRecPos);
        		    slotSize = nextRecPos - recPos;
        		    if (inRepairProblems) {
                	    bRepairsWereAttempted = true;
        		        SetMarker(recPos + kVarDBFileSlotSizeOffset, streamFrom_Start);
   	                    SInt32 outSize = Native_ToBigEndian32(slotSize);
        		        WriteBlock(&outSize, kSizeOfSlotSize);
            		    DB_LOG("Integrity Check Repair: Set slot size at dbpos "<<recPos<<" to "<<slotSize<<" bytes");
        		    }
        		}
        		if ( (recID < 0) || (recID > mLastRecID) ) {
        		    DB_LOG("INTEGRITY CHECK ERROR: Slot at dbpos "<<recPos<<" has invalid record id ("<<recID<<")");
        		    bResult = false;
        		    recID = kInvalidRecord;  // this should be in the delete list, so we will check it in the next conditional
        		    if (inRepairProblems) {
                	    bRepairsWereAttempted = true;
        		        SetMarker(recPos + kVarDBFileRecIDOffset, streamFrom_Start);
        		        WriteBlock(&recID, kSizeOfRecID); // writing zero, don't worry about endian swap
            		    DB_LOG("Integrity Check Repair: Marked slot at dbpos "<<recPos<<" as deleted");
        		    }
        		}
        		if (recID == 0) {
        		    if (!itsMasterIndex->CheckDeletedSlot(recPos, slotSize)) {
            		    DB_LOG("INTEGRITY CHECK ERROR: Slot at dbpos "<<recPos << " ("<<slotSize
            		            <<"B) is marked deleted but is not in delete list");
            		    bResult = false;
            		    if (inRepairProblems) {
                    	    bRepairsWereAttempted = true;
            		        // add this slot to the delete list
            		        CDeleteList* deleteList = itsMasterIndex->GetDeleteList();
            		        deleteList->SlotWasDeleted(recPos, slotSize);
            		        DB_LOG("Integrity Check Repair: Added slot at dbpos "<<recPos<<" to delete list");
            		    }
        		    }
        		} else {
        		    try {
        		        itsMasterIndex->FetchEntry(recID, entry);
                		if (entry.recPos != recPos) {
                		    DB_LOG("INTEGRITY CHECK ERROR: Slot at dbpos "<<recPos<<" for record id "<<recID
                		            <<" ("<<slotSize<<"B) has an index entry that says it should be at dbpos "
                		            <<entry.recPos);
                		    bResult = false;  // Not sure how this could ever happen given the index validation
                		                      // we already did. Perhaps via a duplicate entry in database file
                		  #warning TODO: repair dbpos wrong in index problem in CheckDatabaseIntegrity()
                		}
        		    }
        		    catch (LException& e) {
        		        if (e.GetErrorCode() == dbItemNotFound) {
                		    DB_LOG("INTEGRITY CHECK ERROR: Slot at dbpos "<<recPos<<" for record id "<<recID
                		            <<" ("<<slotSize<<"B) has no index entry");
                		    bResult = false;
                		    if (inRepairProblems) {
                		        bRepairsWereAttempted = true;
        		                recID = kInvalidRecord;  // if it's not in the index, delete it, 
        		                                         // we don't have any other choice because the index
        		                                         // insists we add things in order
                		        SetMarker(recPos + kVarDBFileRecIDOffset, streamFrom_Start);
                		        WriteBlock(&recID, kSizeOfRecID); // writing zero, don't worry about endian swap
                    		    DB_LOG("Integrity Check Repair: Marked record "<< entry.recID <<" in slot at dbpos "<<recPos<<" as deleted");
            		        } // end repair problems
            		    } // end (e.GetErrorCode() == dbItemNotFound)
            		} // end catch
        		} // end (recID != 0)
        		if ( (recPos + slotSize) >= bytesUsed) {
        		    break;  // break out of the while loop
        		}
        	    SetMarker(recPos + slotSize, streamFrom_Start);	// move to start of the next record    		    
        	} // end while loop

    #warning TODO: Perhaps rebuild delete list from scratch?

    	} // end bExtensiveChecks
    
    	// fix the record count in the header
        if ((long)mNumValidRecs != numRecords) {
            DB_LOG("INTEGRITY CHECK ERROR: Header has incorrect record count ("<<mNumValidRecs<<"); correct count is "
                    <<numRecords);
            if (inRepairProblems) {
                bRepairsWereAttempted = true;
                mNumValidRecs = numRecords;
                WriteHeader(kFileIsOpen);
        	    DB_LOG("Integrity Check Repair: Set header record count to "<<mNumValidRecs);
            }
        }

    	// fix the record count in the header
        if (mBytesUsed != bytesUsed) {
            DB_LOG("INTEGRITY CHECK ERROR: Header has incorrect bytes used ("<<mBytesUsed<<"); correct count is "
                    <<bytesUsed);
            if (inRepairProblems) {
                bRepairsWereAttempted = true;
                mBytesUsed = bytesUsed;
                WriteHeader(kFileIsOpen);
        	    DB_LOG("Integrity Check Repair: Set header bytes used to "<<mBytesUsed);
            }
        }
        
    	// fix the largest rec size in the header
        if (mLargestRecSize != actualLargestRecSize) {
            DB_LOG("INTEGRITY CHECK WARNING: Header has incorrect largest rec size ("<<mLargestRecSize
                    <<"); correct largest size is " <<actualLargestRecSize);
            if (inRepairProblems) {
                mLargestRecSize = actualLargestRecSize;
                WriteHeader(kFileIsOpen);
        	    DB_LOG("Integrity Check Repair: Set header largest rec size to "<<mLargestRecSize);
            }
        }

    } // end main try block
    catch(...) {
    	DB_LOG("INTEGRITY CHECK ERROR: Unexpected exception, failing check");
        bResult = false;  // unknown error, so we fail
    }
    // if there were problems found and we attempted to fix them, recheck
    if (bRepairsWereAttempted) {
        CMasterIndexFile* index = static_cast<CMasterIndexFile*>(itsMasterIndex);
        index->WriteHeader(kFileIsClosed);     // this will update the delete list
        index->WriteHeader(kFileIsOpen);
        DB_LOG("================================================================================");
        DB_LOG("INTEGRITY CHECK COMPLETE. Verifying Repairs.");
        DB_LOG("--------------------------------------------------------------------------------");
        bResult = CheckDatabaseIntegrity(kCheckOnly);   // will not attempt repair on this pass
        DB_LOG("================================================================================");
        if (bResult == false) {
            mFileIsDamaged = true;
            WriteHeader(kFileIsOpen);
            DB_LOG("INTEGRITY CHECK FAILED. Marking file as damaged for extensive checks at next open.");
        }
    } else if (inRepairProblems) {
        DB_LOG("Integrity check passed.");
    }
  #warning TODO: Put the asserts below in the proper place or make them fix the file
    // haven't decided what to do with these yet
	ASSERT(mBytesUsed <= mAllocatedBytes);	// can't use more than we've allocated
	ASSERT((mAllocatedBytes+mFirstItemPos) == GetLength());	// LFileStream needs to be in synch
	ASSERT(mAllocatedBytes >= (SInt32)(mItemCount*sizeof(DatabaseRec)));// recs must have at least a header

    return bResult;
}

void
CVarDataFile::Backup(const char* appendToFilename) {
    // override to make backup copy
    CDataFile::Backup(appendToFilename);
    CMasterIndexFile* index = static_cast<CMasterIndexFile*>(itsMasterIndex);
    index->Backup(appendToFilename);
}

bool	
CVarDataFile::FetchItemAt(IndexT inAtIndex, void *outItem, UInt32 &ioItemSize) {
	IndexEntryT indexEntry;
	DatabaseRecPtr recP = static_cast<DatabaseRecPtr>(outItem);	
	if ( !itsMasterIndex->FetchEntryAt(inAtIndex, indexEntry) ) { // get Nth index entry
		return false;
	}
	recP->recID = indexEntry.recID;								// return record ID #
	recP->recPos = indexEntry.recPos;							// return the position
	recP->recSize = indexEntry.recSize;							// return size of DatabaseRecPtr
	if ((SInt32)ioItemSize < recP->recSize) {
		recP->recSize = ioItemSize;
	} else {
		ioItemSize = recP->recSize;
	}
	SetMarker(indexEntry.recPos + kVarDBFileRecIDOffset, streamFrom_Start);		// move to start of record
	ReadBlock(&recP->recID, RecSizeToIOSize(ioItemSize));						// read the item into the pointer
  #if PLATFORM_LITTLE_ENDIAN
    recP->recID = BigEndian32_ToNative(recP->recID);
  #endif // PLATFORM_LITTLE_ENDIAN
	recP->recSize = indexEntry.recSize;							// return the correct size
	DB_DEBUG("FetchItemAt("<<inAtIndex<<"); id: "<<recP->recID<<" size: "<<recP->recSize<<" pos: "<<indexEntry.recPos<<" bufsize: "<<ioItemSize, DEBUG_TRIVIA);
	return true;
}

// ReadRecord( DatabaseRec* )
// IN: recID = valid ID# of existing record that you want read, 
//	  recSize = allocated size of buffer, or 0 if you are sure your buffer is big enough
// OUT: recPos = location stored in file, recSize = bytes of data read from disk

void
CVarDataFile::ReadRecord(DatabaseRec *ioRecP) {
	SInt32 recSize;
	SInt32 buffSize = ioRecP->recSize;
	ASSERT(itsMasterIndex != nil);
	SInt32 recPos = itsMasterIndex->FindEntry(ioRecP->recID, &recSize);	// find index entry
	ioRecP->recPos = recPos;
	ioRecP->recSize = recSize;					// allocated size of DatabaseRecPtr
	if (buffSize == 0) {
		buffSize = recSize;						// they were sure the record was big enough
	} else if (buffSize > recSize) {			// don't read past the end of the record
		buffSize = recSize;
	}
#if DB_DEBUG_MODE
	else if (buffSize < recSize) {
		DB_DEBUG("WARNING: Buffer too small. Only reading "<<buffSize<<" bytes of "<<recSize<<" for Rec "<<ioRecP->recID, DEBUG_TRIVIA);
	}
	ASSERT(mBytesUsed <= mAllocatedBytes);	// can't use more than we've allocated
	ASSERT((mAllocatedBytes+mFirstItemPos) == GetLength());	// LFileStream needs to be in synch
	if (recSize > mLargestRecSize) {
		DB_DEBUG("ERROR: Index says Rec "<<ioRecP->recID<<" is "<<recSize<<" bytes, but largest record is "<<mLargestRecSize, DEBUG_ERROR);
	}
#endif // DB_DEBUG_MODE only checks
#if DB_DEBUG_MODE || DB_INTEGRITY_CHECKING
	if (ioRecP->recID > mLastRecID) {
		DB_DEBUG("ERROR: Invalid Record ID "<<ioRecP->recID<<" requested. Last ID is "<<mLastRecID, DEBUG_ERROR);
		Throw_(dbItemNotFound);		// not found is an exception
	}
	if (recSize < (SInt32)sizeof(DatabaseRec)) {
		DB_DEBUG("ERROR: Index says Rec "<<ioRecP->recID<<" is "<<recSize<<" bytes, smaller than the record header alone.", DEBUG_ERROR);
		mFileIsDamaged = true;
		Throw_(dbIndexCorrupt);		// index is clearly damaged
		// we could clean this up by taking the size directly from the record
		// or perhaps we should remove this from the index
	}
	if (GetLength() < recPos) {
		DB_DEBUG("ERROR: Index returned offset "<<recPos<<" for Rec "<<ioRecP->recID<<", but datafile is only "<<GetLength()<<" bytes long.", DEBUG_ERROR);
		mFileIsDamaged = true;
		Throw_(dbIndexCorrupt);		// we don't know if the index is wrong or the file was truncated, blame the index
		// we could clean this up by removing this from the index
	}
	RecIDT actualID;
	SInt32 slotSize;
	SetMarker(recPos + kVarDBFileSlotSizeOffset, streamFrom_Start);	// debugging, check old ID
	ReadBlock(&slotSize, kSizeOfSlotSize);
	ReadBlock(&actualID, kSizeOfRecID);
  #if PLATFORM_LITTLE_ENDIAN
    slotSize = BigEndian32_ToNative(slotSize);
    actualID = BigEndian32_ToNative(actualID);
  #endif // PLATFORM_LITTLE_ENDIAN
	if (actualID != ioRecP->recID) {
	    if (actualID == 0) {
		    DB_DEBUG("ERROR: Asked index for Rec "<<ioRecP->recID<<" but got a deleted slot at pos "<< recPos << "; Removing from Index", DEBUG_ERROR);
            // the index must be wrong, but so we'll delete the index entry and do a "not found"
	    } else {
		    DB_DEBUG("ERROR: Asked index for Rec "<<ioRecP->recID<<" but got a slot with Rec "<< actualID << " at pos "<<recPos <<"; Removing from Index", DEBUG_ERROR);
            // the index could be wrong, or maybe the data was overwritten
            // in either case we'll delete the index entry and do a "not found"
		}
		mFileIsDamaged = true;
        itsMasterIndex->DeleteEntry(ioRecP->recID);
        mNumValidRecs--;
        WriteHeader();
	    DB_LOG("ERROR RECOVERY: Removed index entry for Rec " << ioRecP->recID);
        DB_DEBUG("ERROR RECOVERY: Removed index entry for Rec "<<ioRecP->recID, DEBUG_ERROR);
		Throw_(dbItemNotFound);		// not found is an exception
	}
	if (slotSize < RecSizeToSlotSize(recSize)) {
		DB_DEBUG("ERROR: Index says Rec "<<ioRecP->recID<<" is "<<recSize<<" bytes, but the record's slot size is only "<<slotSize<<" bytes", DEBUG_ERROR);
		mFileIsDamaged = true;
		Throw_(dbDataCorrupt);		// this is a serious problem
	}
#endif
	SetMarker(recPos + kVarDBFileRecIDOffset, streamFrom_Start);	// move to start of record data (skip slot size)
	ReadBlock(&ioRecP->recID, RecSizeToIOSize(buffSize));		    // read the item into the pointer
  #if PLATFORM_LITTLE_ENDIAN
    ioRecP->recID = BigEndian32_ToNative(ioRecP->recID);
  #endif // PLATFORM_LITTLE_ENDIAN
	DB_DEBUG("ReadRecord("<<ioRecP->recID<<"); size: "<<recSize<<" pos: "<<recPos<<" bufsize: "<<buffSize, DEBUG_TRIVIA);
}


// FindRecord( DatabaseRec* {, LComparator} )
//  Looks for a record using a sequential search (slow), that does not depend on the index
// IN: recData = data to search for using comparator (or default cmprtr if not passed in)
//	  recSize = 0 if you've allocated your buffer using GetRecordSize(0) to get size of 
//	  largest record, or recSize = allocated size of buffer.
// OUT: recID = id # of first match, recSize = bytes of buffer used to hold record
//	  recPos = position of record in datafile, recData = data of record
//	returns true if found

bool
CVarDataFile::FindRecord(DatabaseRec *ioRecP, LComparator *inComparator) {
#warning FIXME: pretty sure the loop below is broken and will stay stuck on the same record
	bool found = false;
	SInt32 slotSize;
	SInt32 recPos;
	ASSERT(mLargestRecSize > (SInt32)sizeof(DatabaseRec));	// this assertion will fail if no recs in file
	DatabaseRecPtr recP = (DatabaseRec*) std::malloc(mLargestRecSize);
	ThrowIfNil_(recP);
	if (NULL == inComparator) {
		inComparator = itsDefaultComparator;	// use one of the comparators
	}
	SetMarker(mFirstItemPos + kVarDBFileSlotSizeOffset, streamFrom_Start);	// move to start of the 1st record
	for (UInt32 i = 0; i<mItemCount; i++) {		// look at all the records in the datafile
		recPos = GetMarker();
		ReadBlock(&slotSize, kSizeOfSlotSize);				 // read the slot size into the pointer
      #if PLATFORM_LITTLE_ENDIAN
        slotSize = BigEndian32_ToNative(slotSize);
      #endif // PLATFORM_LITTLE_ENDIAN
		ReadBlock(&recP->recID, slotSize - kSizeOfSlotSize); // read the contents of the slot
      #if PLATFORM_LITTLE_ENDIAN
        recP->recID = BigEndian32_ToNative(recP->recID);
      #endif // PLATFORM_LITTLE_ENDIAN
		recP->recSize = SlotSizeToRecSize(slotSize);
		if (inComparator->IsEqualTo(ioRecP, recP, recP->recSize, recP->recSize) ) {	
			// compare the current rec to the source  еее CW 8 mod - added sizes
			found = true;
			ioRecP->recPos = recPos;			// return the found recPos in the DatabaseRecPtr
			if (SlotSizeToRecSize(slotSize) < ioRecP->recSize) {
				ioRecP->recSize = SlotSizeToRecSize(slotSize);	// return the found recSize in the DatabaseRecPtr
			}
			UInt8 *dst = (UInt8*)&ioRecP->recSize;
  			UInt8 *src = (UInt8*)&recP->recSize;			// include recSize & recID in copy
  			for (SInt32 n = RecSizeToSlotSize(ioRecP->recSize); n>0; n--) {	// copy the data to be returned
      			*dst++ = *src++;
      	    }
 			break;
		}
	}
#if DB_DEBUG_MODE
	if (found && (ioRecP->recID > mLastRecID)) {
		DB_DEBUG("ERROR: Invalid Record ID "<<ioRecP->recID<<" returned. Last ID is "<<mLastRecID, DEBUG_ERROR);
	}
#endif
    std::free(recP);    // free the temporary buffer
	return found;								// return true if found
}


// AddRecord( DatabaseRec* )
// IN: recSize = allocated size of DatabaseRecPtr, recID = new valid id #, or 0 for 
//	  auto assignment of new id number -- same as calling GetNewRecordID() and passing
//	  in result, recPos ignored
// OUT: recPos = location stored in file, recSize = unchanged
//	  recID = rec's ID#, same as value returned from function
// returns id# of record

RecIDT
CVarDataFile::AddRecord(DatabaseRec *inRecP) {
	SInt32 recID = inRecP->recID;
	SInt32 recSize = inRecP->recSize;			// actual data size
//	DB_DEBUG("Begin AddRecord("<<recID<<")", DEBUG_TRIVIA);
	if (0 == recID) {
		recID = GetNewRecordID(inRecP);		  	// get a new id# if needed
	} else if (recID > mLastRecID) {			// or reject if not last id given by GetNewRecID()
    	DB_LOG("ERROR: Add Rec " << recID << " " << recSize<<" FAILED, id not yet assigned");
		DB_DEBUG("ERROR: Trying to add Rec "<<recID<<", but that ID has not yet been assigned.", DEBUG_ERROR);
		Throw_ ( dbInvalidID );
	}
	SInt32 recPos = itsMasterIndex->AddEntry(recID, recSize);	// get or create an empty slot
#if DB_DEBUG_MODE || DB_INTEGRITY_CHECKING
//	DB_DEBUG("Start AddRecord("<<recID<<") debug checks", DEBUG_TRIVIA);
	if (recSize < (SInt32)sizeof(DatabaseRec)) {
    	DB_LOG("ERROR: Add Rec " << recID << " " << recSize << "B pos: "<<recPos<<" FAILED, record smaller than header");
		DB_DEBUG("ERROR: Trying to add Rec "<<recID<<" with size of "<<recSize<<" bytes, smaller than the record header alone.", DEBUG_ERROR);
        Throw_( dbDataCorrupt );
	}
	ASSERT(mBytesUsed <= mAllocatedBytes);	// can't use more than we've allocated
	ASSERT((mAllocatedBytes+mFirstItemPos) == GetLength());	// LFileStream needs to be in synch
	if (GetLength() < recPos) {
    	DB_LOG("ERROR: Add Rec " << recID << " " << recSize << "B pos: "<<recPos<<" FAILED, overran datafile length "<<GetLength()<<"B");
		DB_DEBUG("ERROR: Index wants to put new Rec "<<recID<<" at offset "<<recPos<<", but datafile is only "<<GetLength()<<" bytes long.", DEBUG_ERROR);
		mFileIsDamaged = true;
        Throw_( dbIndexCorrupt );
	}
	RecIDT oldID;
	SInt32 slotSize;
	SetMarker(recPos + kVarDBFileSlotSizeOffset, streamFrom_Start);	// debugging, check old ID
	ReadBlock(&slotSize, kSizeOfSlotSize);
	ReadBlock(&oldID, kSizeOfRecID);
  #if PLATFORM_LITTLE_ENDIAN
    slotSize = BigEndian32_ToNative(slotSize);
    oldID = BigEndian32_ToNative(oldID);
  #endif // PLATFORM_LITTLE_ENDIAN
	if (oldID != 0) {
    	DB_LOG("ERROR: Add Rec " << recID << " " << recSize << "B pos: "<<recPos<<" FAILED, overwriting Rec "<<oldID);
		DB_DEBUG("ERROR: Attempting to add into slot with a non-zero (valid) ID\n  Probably overwriting Rec "<<oldID, DEBUG_ERROR);
		mFileIsDamaged = true;
        Throw_( dbIndexCorrupt );
	}
	if (slotSize < RecSizeToSlotSize(recSize)) {
    	DB_LOG("ERROR: Add Rec " << recID << " " << recSize << "B pos: "<<recPos<<" FAILED, slot too small "<<slotSize<<"B");
		DB_DEBUG("ERROR: Attempting to add "<<RecSizeToSlotSize(recSize)<<" bytes into a "<<slotSize<<" byte slot at "<< recPos, DEBUG_ERROR);
		mFileIsDamaged = true;
        Throw_( dbDataCorrupt );
	}
//	DB_DEBUG("Done AddRecord("<<recID<<") debug checks", DEBUG_TRIVIA);
#endif
	SetMarker(recPos + kVarDBFileRecIDOffset, streamFrom_Start);		// move to start of slot's data, skipping size
//	DB_DEBUG("AddRecord("<<recID<<") set marker at "<< recPos+kVarDBFileRecIDOffset, DEBUG_TRIVIA);
	inRecP->recID = Native_ToBigEndian32(recID);			// return the new recID in the DatabaseRecPtr
    WriteBlock(&inRecP->recID, RecSizeToIOSize(recSize));	// write the new record data into the slot
	inRecP->recID = recID;	// restore native endianness
//	    DB_DEBUG("AddRecord("<<recID<<") wrote block size = " << RecSizeToIOSize(recSize), DEBUG_TRIVIA);
	mNumValidRecs++;							// one more valid record
	if (!mBatchMode) {
		WriteHeader();
    }
	inRecP->recPos = recPos;
	DB_LOG("Added Rec " << recID << " " << recSize << "B pos: "<<recPos);
	DB_DEBUG("AddRecord("<<recID<<"); size: "<<recSize<<" pos: "<<recPos, DEBUG_TRIVIA);
	return(recID);
}


// UpdateRecord( DatabaseRec* )
// IN: recSize = allocated size of DatabaseRecPtr, recID = valid id # of existing record
// OUT: recPos = location stored in file, recSize = unchanged

void
CVarDataFile::UpdateRecord(DatabaseRec *inRecP) {
	SInt32 recSize = inRecP->recSize;
	SInt32 recPos = itsMasterIndex->UpdateEntry(inRecP->recID, recSize);	// find index entry
	inRecP->recPos = recPos;
#if DB_DEBUG_MODE || DB_INTEGRITY_CHECKING
	if (inRecP->recID > mLastRecID) {
		DB_DEBUG("ERROR: Invalid Record ID "<<inRecP->recID<<" updated. Last ID is "<<mLastRecID, DEBUG_ERROR);
		Throw_( dbInvalidID );
	}
	if (recSize < (SInt32)sizeof(DatabaseRec)) {
		DB_DEBUG("ERROR: Trying to update Rec "<<inRecP->recID<<" with size of "<<recSize<<" bytes, smaller than the record header alone.", DEBUG_ERROR);
        Throw_( dbDataCorrupt );
	}
	ASSERT(mBytesUsed <= mAllocatedBytes);	// can't use more than we've allocated
	ASSERT((mAllocatedBytes+mFirstItemPos) == GetLength());	// LFileStream needs to be in synch
	if (GetLength() < recPos) {
		DB_DEBUG("ERROR: Index returned offset "<<recPos<<" for Rec "<<inRecP->recID<<", but datafile is only "<<GetLength()<<" bytes long.", DEBUG_ERROR);
		mFileIsDamaged = true;
        Throw_( dbIndexCorrupt );
	}
//	DB_DEBUG("in UpdateRecord("<< inRecP->recID <<"); size: "<<recSize<<" pos: "<<recPos, DEBUG_TRIVIA);
	RecIDT oldID;
	SInt32 slotSize;
	SetMarker(recPos + kVarDBFileSlotSizeOffset, streamFrom_Start);	// debugging, check old ID
	ReadBlock(&slotSize, kSizeOfSlotSize);
	ReadBlock(&oldID, kSizeOfRecID);
  #if PLATFORM_LITTLE_ENDIAN
    slotSize = BigEndian32_ToNative(slotSize);
    oldID = BigEndian32_ToNative(oldID);
  #endif // PLATFORM_LITTLE_ENDIAN
	if ( (oldID != 0) && (oldID != inRecP->recID) ) {
	    DB_LOG("ERROR: Update Rec " << inRecP->recID << " " << recSize << "B pos: "<<recPos<<" FAILED, overwriting Rec "<<oldID);
		DB_DEBUG("ERROR: Updating "<< inRecP->recID << " into wrong place [" << recPos << "] , overwriting Rec "<<oldID, DEBUG_ERROR);
		mFileIsDamaged = true;
		Throw_( dbIndexCorrupt );
	}
	if (slotSize < RecSizeToSlotSize(recSize)) {
	    DB_LOG("ERROR: Update Rec " << inRecP->recID << " " << recSize << "B pos: "<<recPos<<" FAILED, slot too small "<<slotSize<<"B");
		DB_DEBUG("ERROR: Writing "<< inRecP->recID <<" of "<<RecSizeToSlotSize(recSize)<<" bytes into a "<<slotSize<<" byte slot at "<< recPos, DEBUG_ERROR);
		mFileIsDamaged = true;
        Throw_( dbDataCorrupt );
	}
#endif
	SetMarker(recPos + kVarDBFileRecIDOffset, streamFrom_Start);	// move to start of slot's data, skipping size
    inRecP->recID = Native_ToBigEndian32(inRecP->recID);
	WriteBlock(&inRecP->recID, RecSizeToIOSize(recSize));			// write the new record data into the slot
	inRecP->recID = BigEndian32_ToNative(inRecP->recID);
	DB_LOG("Updated Rec " << inRecP->recID << " " << recSize << "B pos: "<<recPos);
	DB_DEBUG("UpdateRecord("<< inRecP->recID <<"); size: "<<recSize<<" pos: "<<recPos, DEBUG_TRIVIA);
}


SInt32
CVarDataFile::GetRecordSize(RecIDT inRecID) {
#if DB_DEBUG_MODE
	if (inRecID > mLastRecID) {
		DB_DEBUG("ERROR: Invalid Record ID "<<inRecID<<" requested. Last ID is "<<mLastRecID, DEBUG_ERROR);
	}
#endif
	if (0 == inRecID) {
		return(mLargestRecSize + kSlotSizeExtra + kSlotSizeSlop);	// return super-safe buffer size
	}
	SInt32 recSize;
	itsMasterIndex->FindEntry(inRecID, &recSize);	// find index entry
	return (recSize);								// return the allocation block size of the record
}


UInt32
CVarDataFile::AddNewEmptySlot(SInt32 inSize) {
	if (inSize > mLargestRecSize) {
		DB_DEBUG("New largest rec is "<<inSize<<" bytes. Previously "<<mLargestRecSize, DEBUG_TRIVIA);
		mLargestRecSize = inSize;				// keep track of largest record
	}
	mItemCount++;
	mAllocatedSlots = mItemCount + 1L;			// update #slots
	UInt32 oldBytesUsed = mBytesUsed;
	mBytesUsed += inSize;
	if (mBytesUsed >= mAllocatedBytes) {
		if (!mBatchMode) {
			mAllocatedBytes = mBytesUsed + kAllocationBlockSize;
		} else {
			mAllocatedBytes = mBytesUsed + kAllocationBlockSize*8L; // big blocks for batches
	    }
		ASSERT(mAllocatedBytes > mBytesUsed);
		ASSERT(mAllocatedBytes >= (SInt32)(mItemCount*RecSizeToSlotSize(sizeof(DatabaseRec))));	// must have at least a header
		UInt32 fileSize = mFirstItemPos + mAllocatedBytes;
		// fill the newly allocated stuff with FFFF
		UInt32 eraseStart = mFirstItemPos + oldBytesUsed;
		ASSERT(eraseStart < fileSize);
		UInt32 eraseLen = fileSize - eraseStart;
		void* p = std::malloc(eraseLen);
		SetLength(fileSize);					// expand file
		if (p) {
			std::memset(p, 0xff, eraseLen);
			SetMarker(eraseStart, streamFrom_Start);
			WriteBlock(p, eraseLen);			// fill the unused part of the data file
		} else {
			DB_DEBUG("Failed to malloc "<<eraseLen<<" bytes for new space erasure in data file", DEBUG_ERROR);
		}
		if (!mBatchMode) {
			WriteHeader();						// write new # slots, etc.. in disk file header
		}
	}
	UInt32 recPos = oldBytesUsed + mFirstItemPos;
	SetMarker(recPos + kVarDBFileSlotSizeOffset, streamFrom_Start);			// write the slot size in the slot
	inSize = Native_ToBigEndian32(inSize);
	WriteBlock(&inSize, kSizeOfSlotSize);
	inSize = BigEndian32_ToNative(inSize);  // restore endianness
	RecIDT tempID = 0;					// debugging, make sure the ID is cleared from new slots
	WriteBlock(&tempID, sizeof(RecIDT));
	DB_LOG("Add New Slot size: " << inSize << "B pos: "<<recPos <<" slots: " << mAllocatedSlots);
	DB_DEBUG("AddNewEmptySlot(); size: "<<inSize<<" pos: "<<recPos<<" slots: "<<mAllocatedSlots, DEBUG_TRIVIA);
	return recPos;
}


SInt32
CVarDataFile::GetSlotSize(UInt32 inSlotPos) {
#if DB_DEBUG_MODE || DB_INTEGRITY_CHECKING
	ASSERT(mBytesUsed <= mAllocatedBytes);	// can't use more than we've allocated
	ASSERT((mAllocatedBytes+mFirstItemPos) == GetLength());	// LFileStream needs to be in synch
	if (GetLength() < (SInt32)inSlotPos) {
		DB_DEBUG("ERROR: Trying to read from slot at offset "<<inSlotPos<<", but datafile is only "<<GetLength()<<" bytes long.", DEBUG_ERROR);
        Throw_( dbDataCorrupt );
	}
#endif
	SInt32 slotSize;
	SetMarker(inSlotPos + kVarDBFileSlotSizeOffset, streamFrom_Start);
	ReadBlock(&slotSize, kSizeOfSlotSize);
  #if PLATFORM_LITTLE_ENDIAN
    slotSize = BigEndian32_ToNative(slotSize);
  #endif // PLATFORM_LITTLE_ENDIAN
	return slotSize;
}	

void
CVarDataFile::MarkSlotDeleted(UInt32 inSlotPos, RecIDT inRecID) {
	RecIDT recID = 0;							// use 0 as deleted slot marker
#if DB_DEBUG_MODE || DB_INTEGRITY_CHECKING
	ASSERT(mBytesUsed <= mAllocatedBytes);		// can't use more than we've allocated
	ASSERT((mAllocatedBytes+mFirstItemPos) == GetLength());	// LFileStream needs to be in synch
	if (GetLength() < (SInt32)inSlotPos) {
	    DB_LOG("ERROR: Delete Rec " << inRecID << " pos: "<<inSlotPos <<" FAILED, overran datafile length "<<GetLength()<<"B");
		DB_DEBUG("ERROR: Trying to delete-mark slot at offset "<<inSlotPos<<", but datafile is only "<<GetLength()<<" bytes long.", DEBUG_ERROR);
		Throw_( dbIndexCorrupt );
	}
	SetMarker(inSlotPos + kVarDBFileRecIDOffset, streamFrom_Start);	// debugging, check old ID
	ReadBlock(&recID, kSizeOfRecID);
  #if PLATFORM_LITTLE_ENDIAN
    recID = BigEndian32_ToNative(recID);
  #endif // PLATFORM_LITTLE_ENDIAN
	DB_DEBUG("MarkSlotDeleted(); deleted_id: " << recID, DEBUG_TRIVIA);
	if (recID != inRecID) {
	    DB_LOG("ERROR: Delete Rec " << inRecID << " pos: "<<inSlotPos << " FAILED, slot has Record "<<recID);
		DB_DEBUG("ERROR: Tried to delete Record ID " << inRecID << " but record " <<recID
		    <<" found in slot at "<<inSlotPos, DEBUG_ERROR);
		Throw_( dbIndexCorrupt );
	}
	if (recID == 0) {
	    DB_LOG("WARNING: Delete Rec " << inRecID << " pos: "<<inSlotPos << " previously deleted");
		DB_DEBUG("NON-CRITICAL ERROR: Deleting a slot with a zero (deleted) ID", DEBUG_ERROR);
	}
	recID = 0;							
#endif
    // writing zero, don't worry about endian swap
	SetMarker(inSlotPos + kVarDBFileRecIDOffset, streamFrom_Start);	// move to position of recID in slot
	WriteBlock(&recID, kSizeOfRecID);						// write the deletion maker
	DB_LOG("Deleted Rec " << inRecID << " pos: "<<inSlotPos);
	DB_DEBUG("MarkSlotDeleted(); pos: " << inSlotPos, DEBUG_TRIVIA);
}


RecIDT  
CVarDataFile::GetRecordAtSlot(UInt32 inSlotPos) {
    RecIDT recID = kInvalidRecID;
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessData);					// block other accesses to the data
  #endif
	SetMarker(inSlotPos + kVarDBFileRecIDOffset, streamFrom_Start);	// debugging, check old ID
	ReadBlock(&recID, 4);						// read the recID or deletion marker
  #if PLATFORM_LITTLE_ENDIAN
    recID = BigEndian32_ToNative(recID);
  #endif // PLATFORM_LITTLE_ENDIAN
    return recID;
}


bool 
CVarDataFile::ReadHeader() {
	bool fileOpen = CDataFile::ReadHeader();
	VarDataFileHeaderT tempHeader;
	ReadBlock(&tempHeader, sizeof(VarDataFileHeaderT));
	mAllocatedBytes = BigEndian32_ToNative(tempHeader.allocatedBytes);
	mBytesUsed = BigEndian32_ToNative(tempHeader.bytesUsed);
	mLargestRecSize = BigEndian32_ToNative(tempHeader.largestRecSize);
	mFileIsDamaged = tempHeader.fileIsDamaged;
	return fileOpen;
}


void 
CVarDataFile::WriteHeader(bool inFileOpen) {
	if (mReadOnly) {
		return;	// v1.5.1 bug fix, don't write to read-only files
    }
    DB_DEBUG("CVarDataFile::WriteHeader", DEBUG_TRIVIA);
	CDataFile::WriteHeader(inFileOpen);
	VarDataFileHeaderT tempHeader;
	tempHeader.allocatedBytes = Native_ToBigEndian32(mAllocatedBytes);
	tempHeader.bytesUsed = Native_ToBigEndian32(mBytesUsed);
	tempHeader.largestRecSize = Native_ToBigEndian32(mLargestRecSize);
	tempHeader.fileIsDamaged = mFileIsDamaged;
	WriteBlock(&tempHeader, sizeof(VarDataFileHeaderT));
	FlushFile();
}

