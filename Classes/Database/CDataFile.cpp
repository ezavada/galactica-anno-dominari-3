// =============================================================================
// CDataFile.cp								 й1995-2002, Sacred Tree Software, inc.
// 
// Class for fixed record length database files. Inherites from ADataStore.
// Please refer to ADataStore.cp for additional info
//
// Access:
// Has the following methods defined for access:
//	 SInt32		GetRecordCount();
//	 SInt32		GetNewRecordID();
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
// Read is different than Find because Read has the id and fills in the data,
// and Find has the data and fills in the id that matches that data using the
// Comparator object specified.
//
// A MasterIndex must be specified on construction, unless you are using vers 1.5 additions
//
// the BatchMode() function allows you to turn on and off a batch mode that
// defers all sorts until turned off. This allows batch additions and deletions
// to take place much faster. Some examples of this would be importing records
// or duplicating or deleting a set of records.
//
// The Read(), Update() and Delete() functions will all cause an exception if
// the requested record is not found. Find() will return false if not found.
//
// version 1.5.9
//
// created:	  6/16/95, ERZ
// modified:  6/30/95, ERZ	inherited FetchItemAt() from CDataStore &
//						 		removed ability to work without a MasterIndex
//						 		which simplified the object greatly
// modified:  7/14/95, ERZ	optimized for fewer index resorts & improved error handling
// modified: 11/18/95, ERZ	pass record pointer into GetNewRecordID() calls
// modified:  1/30/96, ERZ	CW8 mods, mostly to LComparator related stuff
// modified:  3/29/96, ERZ	rewrote Initialization process, added OpenNew() & OpenExisting()
// modified:  4/14/96, ERZ	added file permissions as optional params to Open methods
// modified:   4/3/96, ERZ	slight changes to FetchItemAt()
// modified:  7/25/96, ERZ	v1.5, simplified Initialization, made platform independant
// modified:  8/12/96, ERZ	changed OpenExisting() param order to match ::ResolveAlias()
// modified:  8/14/96, ERZ	Thread support
// modified:   9/5/96, ERZ	DebugNew support, uses new macro instead of new
// modified: 12/27/96, ERZ	removed inherited virtual method FetchItemAt(IndexT, void*)
// modified:  2/21/98, ERZ	v1.5.5, fixed write to Read Only files bug
// modified:  9/96/98, ERZ	v1.5.6, relaxed GetNewRecordID(), now permits multiple
//								calls before multiple adds. Added RecordExists(RecIDT)
//								added file privileges tracking code
// modified:  5/27/02, ERZ  v1.5.7, converted to bool from MacOS Boolean, removed class typedefs
// modified:  9/24/05, ERZ  v1.5.8, added Backup() to create backup version of files
// modified: 10/23/05, ERZ  v1.5.9, changed to use platform neutral CFileStream instead of LFileStream
//
// =============================================================================

#include <UException.h>

#include "CDataFile.h"
#include "CMasterIndexFile.h"

#if !defined( POSIX_BUILD) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
#include "FullPath.h"
#endif // mac

#define kAllocationBlockSize	4L	// number of slots to add at a time

typedef struct DataFileHeaderT {
	SInt32		itemSize;			// # bytes per item slot
	SInt32		itemCount;			// # items in use
	SInt32		allocatedSlots;		// # slots allocated to file
	SInt32		allocBlockSize;		// # slots to allocated at a time
	SInt32		firstItemPos;		// file position of first item slot
	SInt32		numValidRecs;		// # valid records in the datafile
} DataFileHeaderT, *DataFileHeaderPtr, **DataFileHeaderHnd, **DataFileHeaderHdl;

#if DB_V15
// new constructor just does most basic initialization tasks, the rest are in the open methods
CDataFile::CDataFile(SInt16 inStrcResID)	// ** NOT THREAD SAFE **
: ADataStore(inStrcResID), LFileStream(), AMasterIndexable() {
	mItemSize = 0;
	mItemCount = 0;								// no items, valid records, or slots in a new file
	mAllocatedSlots = 0;
	mAllocBlockSize = kAllocationBlockSize;		// number of bytes to allocate at a time
	mNumValidRecs = 0;
	mReadOnly = false;
	mPrivileges = 0;	// v1.5.6
	mFirstItemPos = kDataFileHeaderSize;		// set size of file header
	itsStream = (LStream*)this;					// save stream ptr for ADataStore to use
}

void	// ** NOT THREAD SAFE **
CDataFile::SetStructure(UStructure* inStructure) {
	ADataStore::SetStructure(inStructure);
	if (mItemSize == 0) {	// set the item size to what's in the Strc res, minus the amount that
		mItemSize = mStructure->GetNewRecordSize() - 8L;	// we don't actually store	
	}
}

AMasterIndex*	// ** NOT THREAD SAFE **
CDataFile::MakeMasterIndex() {
	return new CMasterIndexFile();
}

#else
// inMasterIndex is the index used to find a record's position based on ID#
// inItemSize is the size of a DatabaseRecPtr that can hold a DatabaseRec with the data. 
CDataFile::CDataFile(SInt32 inItemSize, AMasterIndex *inMasterIndex)	// ** NOT THREAD SAFE **
: ADataStore(), CFileStream(), AMasterIndexable(inMasterIndex) {
	mItemSize = inItemSize - 8L;				// adjust size to fit what is actually stored
	mItemCount = 0;								// no items, valid records, or slots in a new file
	mAllocatedSlots = 0;
	mAllocBlockSize = kAllocationBlockSize;		// number of bytes to allocate at a time
	mNumValidRecs = 0;
	mReadOnly = false;
	mPrivileges = 0;	// v1.5.6
	mFirstItemPos = kDataFileHeaderSize;		// set size of file header
	itsStream = static_cast<LStream*>(this);	// save stream ptr for ADataStore to use
}
#endif

void
CDataFile::Backup(const char* appendToFilename) {
	char filename[kDBFilenameBufferSize];
	char ext[kDBFilenameBufferSize];
    char backupFolderName[kDBFilenameBufferSize];
	ext[0] = 0;
    std::strcpy(filename, GetName());
	int len = std::strlen(filename);
	char* p = std::strrchr(filename, '.');
	if (p) {
	    std::strcpy(ext, p);    // copy the extension for later use
	    *p = 0;    // drop the extension from the filename
	}
    std::strcpy(backupFolderName, filename); // save off the base filename as the backup folder name
	std::strncat(filename, appendToFilename, kDBFilenameBufferSize);
	filename[kDBFilenameBufferSize-1] = 0;
	std::strncat(filename, ext, kDBFilenameBufferSize);
	filename[kDBFilenameBufferSize-1] = 0;
	
    std::strncat(backupFolderName, " Backups", kDBFilenameBufferSize);
	backupFolderName[kDBFilenameBufferSize-1] = 0;
	
	std::string backupPath = GetPath();
	backupPath += '/';
	backupPath += backupFolderName;
	
	CFileStream* backupFile = new CFileStream(backupPath.c_str(), filename);

	ExceptionCode err;

  #if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
    // try to create the backup directory on the Mac
    FSSpec backupFolderSpec;
    err = FSpMakeFSSpecFromPosixFullPath(backupFile->GetPath(), backupFolderSpec);
	if (err == fnfErr) {
	  #warning FIXME: the call above is returning fnfErr even when the directory already exists, so we filter dupFNErr below when we should not
        FSSpec tempSpec;
        ExceptionCode err = FSpMakeFSSpecFromPosixFullPath(GetPathName(), tempSpec);
	    long dirID = tempSpec.parID;
	    // folder doesn't exist, we need to create it, and get the new folder's dir ID
	    err = MacAPI::FSpDirCreate(&backupFolderSpec, smSystemScript, &dirID);
	    if ((err != noErr) && (err != dupFNErr)) {
	        DEBUG_OUT("WARNING: CDataFile Backup problem, creating backup folder [" << backupFolderName
	                <<"] failed with OS error [" << err << "]", DEBUG_ERROR | DEBUG_DATABASE);
	        dirID = tempSpec.parID;
	    }
	}
  #endif // !PLATFORM_MACOS
    
    backupFile->CreateFile();		//create file
    // copy creator and type
	char ostype[5];
	char creator[5];
	GetFileAttribute(attrib_Type, ostype, 5);
	GetFileAttribute(attrib_Creator, creator, 5);
	backupFile->SetFileAttribute(attrib_Type, ostype);
	backupFile->SetFileAttribute(attrib_Creator, creator);
	// now open the file to copy contents
	backupFile->OpenFile(CFileStream::privileges_ReadWrite);
    SInt32 bytesToCopy = GetLength();
    SInt32 pos = 0;
    
  #define BLOCK_SIZE 65536L    // copy in 64k blocks
    void* buffer = std::malloc(BLOCK_SIZE);
    
	// duplicate the DataFile into the backup file
	while (bytesToCopy > 0) {
	    SInt32 bytes = bytesToCopy;
	    if (bytes > BLOCK_SIZE) {
	        bytes = BLOCK_SIZE;
	    }   
	    SetMarker(pos, streamFrom_Start);
	    backupFile->SetMarker(pos, streamFrom_Start);
	    err = GetBytes(buffer, bytes);
	    if (err != noErr) {
	        DEBUG_OUT("CDataFile Backup failed, getting ["<<bytes<<"] bytes from pos ["<<pos
	                    <<"] with error [" << err << "]", DEBUG_ERROR | DEBUG_DATABASE);
	        break;
	    }
	    err = backupFile->PutBytes(buffer, bytes);
	    if (err != noErr) {
	        DEBUG_OUT("CDataFile Backup failed, putting ["<<bytes<<"] bytes from pos ["<<pos
	                    <<"] with error [" << err << "]", DEBUG_ERROR | DEBUG_DATABASE);
	        break;
	    }
	    pos += bytes;
	    bytesToCopy -= bytes;	        
	}
	std::free(buffer);
	backupFile->CloseFile();
}

/* for DB_V15
// looks for a 'Strc' resource with the appropriate id and uses it to determine item size 
void	// ** NOT THREAD SAFE **
CDataFile::OpenNew(FSSpec &inFileSpec, OSType inCreator, SInt16 inPrivileges) {
	SetSpecifier(inFileSpec);
	mCreator = inCreator;
	char filename[kDBFilenameBufferSize];
	p2cstrcpy(filename, mMacFileSpec.name);
	DB_LOG("Create Database file \"" << filename << "\"");
	mReadOnly = false;		// v1.5.5 bug fix, don't write to read-only files
	mPrivileges = fsWrPerm;	// v1.5.6, must have Exclusive Read/Write privileges!
	CMasterIndexFile* theIndexFile = (CMasterIndexFile*)itsMasterIndex;
	if (theIndexFile == nil) {
		theIndexFile = (CMasterIndexFile*) MakeMasterIndex();
		SetMasterIndex(theIndexFile);
	}
	handle h = ::GetResource('Strc', mStrcResID);	// this will find the app's 'Strc' resource
	h.lock();
	UStructure* theStruct = MakeStructureObject((StrcResPtr)*h);
	SetStructure(theStruct);
	CreateNewFile( mCreator, 'DatF' );		// create file
  #if PLATFORM_MACOS						// in MacOS, we are going to add the Strc res to the
	OpenResourceFork(mPrivileges);			// file, so other apps can determine the structure
	::UseResFile(mResourceForkRefNum);		// just making sure we put it in the correct res fork
	::DetachResource(h);
	::AddResource(h, 'Strc', 0, "\p");		// add it as ID 0 so it will be easy to find
	::WriteResource(h);
	CloseResourceFork();
  #endif	// MacOS
	h.disposeResource();		// use disposeResource() rather than dispose()
	OpenDataFork(mPrivileges);
	SetTypeAndVersion('data', 0x10000000);	// defaults for new file
	ThrowIf_( Open() );			// Open() should always return false for a new file
	theIndexFile->OpenNew(inFileSpec, inCreator);	// now open the index file
}
// no 1.5 enhancements */

void	// ** NOT THREAD SAFE **
CDataFile::OpenNew(const char* inPath, const char* inName, const char* inCreator) {
	SetPath(inPath);
	SetName(inName);
	mReadOnly = false;		// v1.5.5 bug fix, don't write to read-only files
	mPrivileges = CFileStream::privileges_ReadWrite;
	ExceptionCode err = CreateFile();		//create file
	ThrowIfOSErr_(err);
	SetFileAttribute(attrib_Type, "DatF");
	SetFileAttribute(attrib_Creator, inCreator);
	err = OpenFile(mPrivileges);
	ThrowIfOSErr_(err);
	SetTypeAndVersion(kOSType_data, 0x10000000);		// defaults for new file 0
	ThrowIf_( Open() );			// Open() should always return false for a new file
}

void	// ** NOT THREAD SAFE **
CDataFile::OpenExisting(const char* inPath, const char* inName, int inPrivileges) {
	SetPath(inPath);
	SetName(inName);
	DB_LOG("Open Database file \"" << inPath << "/" << inName << "\"");
	mReadOnly = (inPrivileges == CFileStream::privileges_ReadOnly);		// v1.5.5 bug fix, don't write to read-only files
/*#if DB_V15	// don't include unless we want 1.5 enhancements
	CMasterIndexFile* theIndexFile = (CMasterIndexFile*)itsMasterIndex;
	if (theIndexFile == nil) {
		theIndexFile = (CMasterIndexFile*) MakeMasterIndex();
		SetMasterIndex(theIndexFile);
	}
	handle h;
  #if PLATFORM_MACOS							// in MacOS, there will be a Strc res with id 0
	OpenResourceFork(fsWrPerm);					// in the resource fork of the document, so
	h = ::GetResource('Strc', 0);				// look for it there first
  #endif	// MacOS
  	if (h == nil) {
		h = ::GetResource('Strc', mStrcResID);	// this will find the app's 'Strc' resource
	}
	h.lock();
	UStructure* theStruct = MakeStructureObject((StrcResPtr)*h); // define the record structure from the res
	SetStructure(theStruct);
  #if _MacOS	
	CloseResourceFork();		// MacOS, close res fork
  #endif
	h.disposeResource();		// use disposeResource() rather than dispose()
	theIndexFile->OpenExisting(inFileSpec, inPrivileges);	// now open the index file
#endif	// end 1.5 enhancements
*/
	ExceptionCode err = OpenFile(inPrivileges);
	ThrowIfOSErr_(err);
	mPrivileges = inPrivileges;	// v1.5.6, track privileges
	SInt32 theSize = mItemSize;
    Open();
  #if DB_IGNORE_ERRORS
	ASSERT ( mItemCount >= mNumValidRecs );
	ASSERT ( mItemSize == theSize );
	ASSERT ( mItemCount <= mAllocatedSlots );
	ASSERT ( mAllocatedSlots >= 0 );
	ASSERT ( mNumValidRecs == itsMasterIndex->GetEntryCount() );
  #else
	if ( mItemCount < mNumValidRecs ) {
		Throw_ (dbDataCorrupt);
	}
	if ( mItemSize != theSize ) {
		Throw_ (dbSizeMismatch);
	}
	if ( mItemCount > mAllocatedSlots ) {
		Throw_(dbDataCorrupt);
	}
	if ( mAllocatedSlots < 0 ) {
		Throw_(dbDataCorrupt);
	}
	if ( mNumValidRecs != itsMasterIndex->GetEntryCount() ) {
		Throw_(dbIndexCorrupt);
	}
  #endif
    // ERZ, 11/11/02, fix problem with highest record id getting out of sync. Since ID must
    // always be ascending, take highest value in case of disagreement.
    RecIDT indexHighestID = itsMasterIndex->GetHighestUsedRecID();
    if (mLastRecID < indexHighestID) {
        DB_LOG("Fixing Datafile lastRecID: was [" << mLastRecID << "] index says it must be ["
                << indexHighestID << "]");
        mLastRecID = indexHighestID;
        WriteHeader(kFileIsOpen);
    } else if (mLastRecID > indexHighestID) {
        DB_LOG("Fixing Index highestRecID: was [" << indexHighestID 
                << "] datafile says it must be [" << mLastRecID <<"]");
        itsMasterIndex->SetHighestUsedRecID(mLastRecID);
    }
}



UInt32	// ** Thread Safe **
CDataFile::AddNewEmptySlot(SInt32) { // inSize
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mChangeInfo);	// block other changes to internal object info
  #endif
	mItemCount++;
	UInt32 newSlots = (mItemCount/mAllocBlockSize + 1) * mAllocBlockSize; // allocate in blocks
	if (mAllocatedSlots < newSlots) {		//# allocated slots changed?
		SInt32 fileSize = mFirstItemPos + newSlots * mItemSize;	// calc file size needed for slots
		SetLength(fileSize);				// expand file
		mAllocatedSlots = newSlots;			// update #slots
		if (!mBatchMode) {
			WriteHeader();					// write new # slots, etc.. in disk file header
		}
	}
	return  mFirstItemPos + (mItemCount-1) * mItemSize;
}

void	// ** Thread Safe **
CDataFile::MarkSlotDeleted(UInt32 inSlotPos, RecIDT) { // inRecID
	RecIDT recID = 0;							// use 0 as deleted slot marker
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessData);					// block other accesses to the data
  #endif
	SetMarker(inSlotPos, streamFrom_Start);		// move to position of recID in slot
	WriteBlock(&recID, 4);						// write the deletion maker
}	

RecIDT  
CDataFile::GetRecordAtSlot(UInt32 inSlotPos) {
    RecIDT recID = kInvalidRecID;
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessData);					// block other accesses to the data
  #endif
	SetMarker(inSlotPos, streamFrom_Start);		// move to position of recID in slot
	ReadBlock(&recID, 4);						// read the recID or deletion marker
  #if PLATFORM_LITTLE_ENDIAN
    recID = BigEndian32_ToNative(recID);
  #endif // PLATFORM_LITTLE_ENDIAN
    return recID;
}

bool	// ** NOT THREAD SAFE **
CDataFile::SetBatchMode(bool inBatchMode) {
#warning FIXME: batch mode is disabled since it might be contributing to DB Corruption
// ERZ, 8/31/02, disabled batch mode which may be contributing to DB Corruption
//	mBatchMode = inBatchMode;
//	if (!mBatchMode) {
//		WriteHeader();	// write the header now that we are done with batch mode
//	}
//	return itsMasterIndex->SetBatchMode(inBatchMode);
   return false;
}



UInt32	// ** Thread Safe **
CDataFile::GetRecordCount() const {
	return(mNumValidRecs);
}

// this method assumes the presence of a Master Index, override if you aren't using one
bool	// ** Thread Safe **
CDataFile::FetchItemAt(IndexT inAtIndex, void *outItem, UInt32 &ioItemSize) {
	IndexEntryT indexEntry;
	DatabaseRecPtr recP = (DatabaseRecPtr)outItem;	
	if ( !itsMasterIndex->FetchEntryAt(inAtIndex, indexEntry) ) { // get Nth index entry
		return(false);
	}
	recP->recID = indexEntry.recID;								// return record ID #
	recP->recPos = indexEntry.recPos;							// return the position
	recP->recSize = indexEntry.recSize;							// return size of DatabaseRecPtr
	if ((SInt32)ioItemSize < recP->recSize) {
		recP->recSize = ioItemSize;
	} else {
		ioItemSize = recP->recSize;
	}
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessData);					// block other accesses to the data
  #endif
	SetMarker(indexEntry.recPos, streamFrom_Start);				// move to start of record
	ReadBlock(&recP->recID, ioItemSize - 8L);					// read the item into the pointer
  #if PLATFORM_LITTLE_ENDIAN
    recP->recID = BigEndian32_ToNative(recP->recID);
  #endif // PLATFORM_LITTLE_ENDIAN
	recP->recSize = mItemSize + 8L;								// return the correct size
	return (true);
}

UInt32	// ** Thread Safe **
CDataFile::GetCount() const {
	return(mItemCount);
}

// ReadRecord( DatabaseRec* )
// IN: recID = valid ID# of existing record that you want read
// OUT: recPos = location stored in file, recSize = size of record, recData = the record

void	// ** Thread Safe **
CDataFile::ReadRecord(DatabaseRec *ioRecP) {
	SInt32 recPos = itsMasterIndex->FindEntry(ioRecP->recID);	// find index entry in master index
	ioRecP->recSize = mItemSize + 8L;			// allocated size of DatabaseRecPtr
	ioRecP->recPos = recPos;
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessData);					// block other accesses to the data
  #endif
	SetMarker(recPos, streamFrom_Start);		// move to start of record
	ReadBlock(&ioRecP->recID, mItemSize);		// read the item into the pointer
  #if PLATFORM_LITTLE_ENDIAN
    ioRecP->recID = BigEndian32_ToNative(ioRecP->recID);
  #endif // PLATFORM_LITTLE_ENDIAN
}


// FindRecord( DatabaseRec* {, LComparator} )
// IN: recData = data to search for using comparator (or default comparator if non passed in)
// OUT: recID = id # of first match, recSize = record size
//	  recPos = position of record in datafile, recData = record data
//	returns true if found

bool	// ** Thread Safe **
CDataFile::FindRecord(DatabaseRec *ioRecP, LComparator *inComparator) {
	bool found = false;
	SInt32 recPos;
	DatabaseRec *recP = (DatabaseRec*)std::malloc(mItemSize+8);	// allocate a buffer
	ThrowIfNil_(recP);
	if (NULL == inComparator) {
		inComparator = itsDefaultComparator;	// use one of the comparators
	}
	ioRecP->recSize = mItemSize + 8L;			// sizes are the same
	recP->recSize = mItemSize + 8L;
	for (UInt32 i = 0; i<mItemCount; i++) {		// look at all the records in the datafile
		recPos = mFirstItemPos + i*mItemSize;	// calc position of record i
	    {
      #if DB_THREAD_SUPPORT
    	StSafeMutex mutex(mAccessData);					// block other accesses to the data
      #endif
		SetMarker(recPos, streamFrom_Start);	// move to start of record
		ReadBlock(&recP->recID, mItemSize);		// read the record into the pointer
		}
      #if PLATFORM_LITTLE_ENDIAN
        recP->recID = BigEndian32_ToNative(recP->recID);
      #endif // PLATFORM_LITTLE_ENDIAN
		recP->recPos = recPos;
		if (inComparator->IsEqualTo(ioRecP, recP, mItemSize, mItemSize) ) {
			// compared the current rec to the source еее CW8 mod ERZ - added sizes
			found = true;
			ioRecP->recPos = recPos;			// return the found recPos in the DatabaseRecPtr
			UInt8 *dst = (UInt8*)&ioRecP->recID;
  			UInt8 *src = (UInt8*)&recP->recID;	// sizes are already set to be equal, don't copy
			std::memcpy(dst, src, mItemSize);	// copy the data to be returned (including ID)
 			break;
		}
	}
	std::free(recP);
	return (found);								// return true if found
}


// IN: recID = new valid id #, or 0 for automatic assignment, recSize & recPos ignored
//	  assignment of an new id number -- same as calling GetNewRecordID() and passing in result
// OUT: recPos = location stored in file, recSize = allocated size of DatabaseRecPtr
//	  recID = rec's ID#, same as value returned from function
// returns id# of record

RecIDT	// ** Thread Safe **
CDataFile::AddRecord(DatabaseRec *inRecP) {
	SInt32 recID = inRecP->recID;
	if (0 == recID) {
		recID = GetNewRecordID(inRecP);		  		// get a new id# if needed 
	} else if (recID > mLastRecID ) {				// or reject id not from last GetNewRecID()
		Throw_(dbInvalidID);
	}
	SInt32 recPos = itsMasterIndex->AddEntry(recID);	// get or create an empty slot
	inRecP->recID = Native_ToBigEndian32(recID);	// return the new recID in the DatabaseRecPtr
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessData);	// block other accesses to the data
  #endif
	SetMarker( recPos, streamFrom_Start);			// move to start of slot
	WriteBlock(&inRecP->recID, mItemSize);			// write the new record data into the slot
	mNumValidRecs++;								// one more valid record
	inRecP->recID = recID;
	inRecP->recPos = recPos;
	inRecP->recSize = mItemSize + 8L;				// always allocate same size of data
	if (!mBatchMode) {
		WriteHeader();
	}
	return(recID);
}


// UpdateRecord( DatabaseRec* )
// IN: recSize = allocated size of DatabaseRecPtr, recID = valid id # of existing record
// OUT: recPos = location stored in file

void	// ** Thread Safe **
CDataFile::UpdateRecord(DatabaseRec *inRecP) {
	SInt32 recPos = itsMasterIndex->FindEntry(inRecP->recID);	// find index entry in master index
	inRecP->recSize = mItemSize + 8L;			// allocated size of DatabaseRecPtr
	inRecP->recPos = recPos;
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessData);	// block other accesses to the data
  #endif
	RecIDT recID = inRecP->recID;   // save for endian conversion
	inRecP->recID = Native_ToBigEndian32(inRecP->recID);
	SetMarker(recPos, streamFrom_Start);		// move to start of slot
	WriteBlock(&inRecP->recID, mItemSize);		// write the updated record data into the slot
    inRecP->recID = recID;
}


void	// ** Thread Safe **
CDataFile::DeleteRecord(RecIDT inRecID) {
	SInt32 recPos = itsMasterIndex->DeleteEntry(inRecID);		// find & delete index entry in master index
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessData);	// block other accesses to the data
  #endif
	MarkSlotDeleted(recPos, inRecID);			// delete the slot using our standard method
	mNumValidRecs--;							// one less valid record
	if (!mBatchMode) {
		WriteHeader();
	}
}


SInt32	// ** Thread Safe **
CDataFile::GetRecordSize(RecIDT) {	// inRecID
	return(mItemSize + 8L); //allocation size of DatabaseRecPtr for rec
}


bool	// ** Thread Safe **
CDataFile::RecordExists(RecIDT inRecID) {
	bool found = false;
	try {				// try to look up the item in the index
		IndexEntryT theEntry;
		itsMasterIndex->FetchEntry(inRecID, theEntry);
		found = true;	// succeeded, so we know it exists
	}
	catch (LException& e) {		// caught an exception, so lookup failed
		DEBUG_OUT("Caught PowerPlant exception "<<e.what()<<" in RecordExists("
					<<inRecID<<")", DEBUG_TRIVIA);
		if (e.GetErrorCode() != dbItemNotFound) {
			DEBUG_OUT("Rethrowing PowerPlant exception "<<e.what()<<
						" in RecordExists("<<inRecID<<")", DEBUG_ERROR);
			throw e;	// hmmm... wasn't the NotFound error, must be more serious,
		}					// so we will re-throw the exception.
	}
	catch(...) {	// catch any other exceptions
		DEBUG_OUT("Caught C++ exception in RecordExists("<<inRecID<<")", DEBUG_ERROR);
	}
	return found;
}


bool	// ** NOT THREAD SAFE **
CDataFile::ReadHeader() {
	bool result = ADataStore::ReadHeader();
	DataFileHeaderT tempHeader;
	ReadBlock(&tempHeader, sizeof(DataFileHeaderT));
	mItemSize = BigEndian32_ToNative(tempHeader.itemSize);		
	mItemCount = BigEndian32_ToNative(tempHeader.itemCount);
	mAllocatedSlots = BigEndian32_ToNative(tempHeader.allocatedSlots);
	mAllocBlockSize = BigEndian32_ToNative(tempHeader.allocBlockSize);
	mFirstItemPos = BigEndian32_ToNative(tempHeader.firstItemPos);
	mNumValidRecs = BigEndian32_ToNative(tempHeader.numValidRecs);
	return (result);
}


void	// ** Thread Safe **
CDataFile::WriteHeader(bool inFileOpen) {
	if (mReadOnly) {
		return;				// v1.5.5 bug fix, don't write to read-only files
	}
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessHeader);	// block other accesses to the header
  #endif
	ADataStore::WriteHeader(inFileOpen);
	DataFileHeaderT tempHeader;
	tempHeader.itemSize = Native_ToBigEndian32(mItemSize);	
	tempHeader.itemCount = Native_ToBigEndian32(mItemCount);
	tempHeader.allocatedSlots = Native_ToBigEndian32(mAllocatedSlots);
	tempHeader.allocBlockSize = Native_ToBigEndian32(mAllocBlockSize);
	tempHeader.firstItemPos = Native_ToBigEndian32(mFirstItemPos);
	tempHeader.numValidRecs = Native_ToBigEndian32(mNumValidRecs);
	WriteBlock(&tempHeader, sizeof(DataFileHeaderT));
}


void	// ** NOT THREAD SAFE **
CDataFile::Close() {
	ADataStore::Close();
  #if DB_V15
	itsMasterIndex->Close();
	delete itsMasterIndex;
	itsMasterIndex = (AMasterIndex*) nil;
  #endif
	CloseFile();
}
