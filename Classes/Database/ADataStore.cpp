// =============================================================================
// ADataStore.cp						  й1995-96, Sacred Tree Software, inc.
// 
// Abstract class for database files. 
//
// Since it inherites from LIteratedList, it can also use list iterations 
// for background/slow sorts of sequential stuff while records are being 
// inserted and removed. *** NOT CURRENTLY IMPLEMENTED ***
//
// Access:
// Has the following pure virtual methods defined for access: ** Thread Safe **
//	 RecIDT		GetNewRecordID();
//	 SInt32		GetRecordCount();
//	 void		ReadRecord(DatabaseRec);
//	 bool   	FindRecord(DatabaseRec, LComparator* = nil);
//	 RecIDT		AddRecord(DatabaseRec);
//	 void		UpdateRecord(DatabaseRec);
//	 void		DeleteRecord(RecIDT);
//	 void		GetRecordSize(RecIDT);
//
// New v1.5 hi level access methods:
//	CRecord*	MakeNewEmptyRecord();				// creates a CRecord object of proper Structure
//	CRecord*	ReadRecord(RecIDT);
//	RecIDT		AddRecord(CRecord*);				// adds only
//	void		UpdateRecord(CRecord*);				// updates only, must already exist
//	RecIDT		WriteRecord(CRecord*);				// updates, or adds if doesn't already exist
//	RecIDT		FindRecord(CRecord*, LComparator* = nil);
//	void		DeleteRecord(CRecord* &inRecP);		// deletes the record and the object too
//
// New platform independant calls (handle based access methods)
//	handle		ReadRecordHandle(RecIDT);
//	RecIDT		AddRecord(handle, RecIDT = 0);
//	void		UpdateRecord(RecIDT, handle);
//
// *** ITERATION NOT IMPLEMENTED ***
// Plus can be accessed sequentially as iterated list using:
//	 void		AttachIterator(LArrayIterator *inIterator);
//	 void		DetachIterator(LArrayIterator *inIterator);
// *** 							 ***
//
// DatabaseRec is a structure that begins with recSize, recID, and has
// a other indeterminate data in recData. 
//
// Read is different than Find because Read has the id and fills in the data,
// and Find has the data and fills in the id that matches that data using the
// Comparator object specified.
//
// NOTE: Subclasses must be sure to call ItemsInserted() and ItemsRemoved()
// when inserting and removing items to keep the list sychronized with
// its list iterators еее NOT CURRENTLY NECESSARY
//
// NOTE: Be sure to call Close() before deleting the object or allowing it to go out
// of scope.
//
// version 1.5.3
//
// created:   6/16/95, ERZ
// modified:  6/28/95, ERZ	Added Automatic assignment of CMasterIndexComparator
// 								to the DataStore's MasterIndex
// modified:  7/14/95, ERZ	Move implementation of FetchItemAt() to subclasses
// modified:  7/24/95, ERZ	Moved index related stuff to CMasterIndexable
// modified:  1/20/96, ERZ	Added MacOS Handle based methods
// modified:  1/30/96, ERZ	Modified for CW8's new PowerPlant version
// modified:   2/2/96, ERZ  fixed minor bug in SetBatchMode that returned wrong value
// modified:  3/29/96, ERZ	rewrote private class CRecIDComparator for CW8
// modified:  4/10/96, ERZ	Added SetTypeAndVersion() method
// modified:  4/11/95, ERZ	Fixed bug in ReadRecord(id) as a Handle
// modified:  7/24/96, ERZ	Ver 1.5, platform independant handles, CRecord calls
// modified:  8/14/96, ERZ	Thread support
// modified:   9/5/96, ERZ	DebugNew support, uses new macro instead of new, fixed 4 byte mem leak
// modified: 12/27/96, ERZ	added FetchItemHeaderAt(IndexT, void*) as non-virtual method
// modified:  11/9/97, ERZ	classifying debug output as DEBUG_ERROR or DEBUG_TRIVIA
// =============================================================================


#include "ADataStore.h"
#include <iostream>

#include "DebugMacros.h"

#if DB_LOG_ACTIONS
	std::ofstream db_log( DB_LOG_FILENAME, std::ios::app );
#endif

#if DB_DEBUG_LOG && !defined(DEBUG_MACROS_ENABLED)
	std::ofstream db_debug_log( DB_DEBUG_LOG_FILENAME );
#endif


// ============================ private class REC ID COMPARATOR =========================

typedef class CRecIDComparator : public LComparator {
public:
	virtual SInt32		Compare(const void* inItemOne, const void* inItemTwo,
								UInt32 inSizeOne, UInt32 inSizeTwo) const;
	virtual	SInt32		CompareToKey(const void* inItem, UInt32 inSize,
								const void* inKey) const;
} CRecIDComparator, *CRecIDComparatorPtr;


SInt32
CRecIDComparator::Compare(const void* inItemOne, const void* inItemTwo, UInt32, UInt32) const {
	return (SInt32) ( ((DatabaseRec*)inItemOne)->recID - ((DatabaseRec*)inItemTwo)->recID );
}
								
SInt32
CRecIDComparator::CompareToKey(const void* inItem, UInt32, const void* inKey) const {
	return (SInt32) ( ((DatabaseRec*)inItem)->recID - *(UInt32*)inKey);
}

//=================================== DATA STORE =====================================


#if DB_V15
ADataStore::ADataStore(SInt16 inStrcResID) {	// ** NOT THREAD SAFE **
	ASSERT(inStrcResID >= 128);		// Strc ids 0..127 are reserved for internal use
	itsDefaultComparator = (LComparator*)nil;
	mBatchMode = false;
	mLastRecID = 0;
	mFileOpen = false;
	mOwnsComparator = false;
	itsStream = (LStream*)nil;
	SetDefaultComparator(new CRecIDComparator, true);	// use a ID comparator as initial default
	mStrcResID = inStrcResID;
	mStructure = (UStructure*)nil;
  #if DB_THREAD_SUPPORT
  	if (UEnvironment::HasFeature(env_HasThreadsManager)) {
  	 	mChangeID = new LMutexSemaphore();
		mAccessHeader = new LMutexSemaphore();
		mAccessData = new LMutexSemaphore();
		mChangeInfo = new LMutexSemaphore();
		ThrowIfNil_(mChangeID);
		ThrowIfNil_(mAccessHeader);
		ThrowIfNil_(mAccessData);
		ThrowIfNil_(mChangeInfo);
	} else {
		mChangeID = (LMutexSemaphore*)nil;
		mAccessHeader = (LMutexSemaphore*)nil;
		mAccessData = (LMutexSemaphore*)nil;
		mChangeInfo = (LMutexSemaphore*)nil;
	}
  #endif
  DB_DEBUG("Contructed v1.5 Datastore: struct id " << inStrcResID, DEBUG_TRIVIA);
}
#else
ADataStore::ADataStore() {	// ** NOT THREAD SAFE **
	itsDefaultComparator = (LComparator*)nil;
	mBatchMode = false;
	mLastRecID = 0;
	mFileOpen = false;
	itsStream = (LStream*)nil;
	SetDefaultComparator(new CRecIDComparator);		// use a ID comparator as initial default
  #if DB_THREAD_SUPPORT
  	if (UEnvironment::HasFeature(env_HasThreadsManager)) {
  	 	mChangeID = new LMutexSemaphore();
		mAccessHeader = new LMutexSemaphore();
		mAccessData = new LMutexSemaphore();
		mChangeInfo = new LMutexSemaphore();
		ThrowIfNil_(mChangeID);
		ThrowIfNil_(mAccessHeader);
		ThrowIfNil_(mAccessData);
		ThrowIfNil_(mChangeInfo);
	} else {
		mChangeID = (LMutexSemaphore*)nil;
		mAccessHeader = (LMutexSemaphore*)nil;
		mAccessData = (LMutexSemaphore*)nil;
		mChangeInfo = (LMutexSemaphore*)nil;
	}
  #endif
  DB_DEBUG("Contructed v1.0 Datastore", DEBUG_TRIVIA);
}
#endif


// must call Close() before deleting object
ADataStore::~ADataStore() {	// ** NOT THREAD SAFE **
	if (mOwnsComparator)
		delete itsDefaultComparator;
  #if DB_THREAD_SUPPORT
  	if (UEnvironment::HasFeature(env_HasThreadsManager)) {
  	 	delete mChangeID;
		delete mAccessHeader;
		delete mAccessData;
		delete mChangeInfo;
	}
  #endif
  DB_DEBUG("Deleted Datastore", DEBUG_TRIVIA);
}

bool
ADataStore::Open() {	// ** NOT THREAD SAFE **
	bool result;
	if (!mFileOpen && HeaderExists()) {
		result = ReadHeader();
	} else {
		result = false;
	}
	WriteHeader(kFileIsOpen);
	mFileOpen = true;
	DB_LOG("Opened Database");
	return result;
}

void
ADataStore::Close() {	// ** NOT THREAD SAFE **
	if (mFileOpen) {
		WriteHeader(kFileIsClosed);
	}
	mFileOpen = false;
	DB_LOG("Closed Database");
}

// return true if checks pass (or repairs successful)
bool        
ADataStore::CheckDatabaseIntegrity(bool) { // inRepairProblems
    return true;
}

void
ADataStore::Backup(const char* ) { // appendToFilename
    // override to make backup copy
}

// NOTE: if you change the default comparator, the new one should be able to compare ID#s
void	// ** Thread Safe **
ADataStore::SetDefaultComparator(LComparator *inComparator, bool inOwnsIt) {
	itsDefaultComparator = inComparator;
	mOwnsComparator = inOwnsIt;
}


bool	// ** NOT THREAD SAFE **
ADataStore::SetBatchMode(bool inBatchMode) {
	bool oldBatchMode = mBatchMode;
	mBatchMode = inBatchMode;
	if (!mBatchMode) {
		WriteHeader();	// write the header now that we are done with batch mode
	}
	DB_DEBUG("SetBatchMode("<<inBatchMode<<")", DEBUG_TRIVIA);
	return(oldBatchMode);
}

// only use after file has been opened
void	// ** NOT THREAD SAFE **
ADataStore::SetTypeAndVersion(OSType inType, UInt32 inVersion) {
	mStreamType = inType;
	mStreamVersion = inVersion;
	itsStream->SetMarker(0, streamFrom_Start);
	*itsStream << mStreamType;
	*itsStream << mStreamVersion;
}

bool	//** Thread Safe **
ADataStore::FetchItemHeaderAt(IndexT inAtIndex, DatabaseRec *outItemHeader) {
	UInt32 size = sizeof(DatabaseRec);
	return FetchItemAt(inAtIndex, outItemHeader, size);
}

RecIDT	//** Thread Safe **
ADataStore::GetNewRecordID(DatabaseRec *) {	//inRecP
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mChangeID);
  #endif
	mLastRecID++;
	DB_LOG("GetNewRecordID(): " << mLastRecID);
	return mLastRecID;
}

bool	// ** NOT THREAD SAFE **
ADataStore::ReadHeader() {
	Bool8 fileOpen;
	itsStream->SetMarker(0, streamFrom_Start);
	*itsStream >> mStreamType;
	*itsStream >> mStreamVersion;
	*itsStream >> fileOpen;
	*itsStream >> mLastRecID;
	DB_DEBUG("ReadHeader(): "<< (fileOpen?"already open":"closed"), DEBUG_TRIVIA);
	return fileOpen;
}


void	//** Thread Safe **
ADataStore::WriteHeader(bool inFileOpen) {
	Bool8 fileOpen = inFileOpen;
  #if DB_THREAD_SUPPORT
	StSafeMutex mutex(mAccessHeader);
  #endif
	ASSERT(sizeof(RecIDT) == sizeof(UInt32));       // precondition for this to work correctly
	// don't write the type and version info each time
	itsStream->SetMarker(sizeof(OSType) + sizeof(UInt32), streamFrom_Start);
	*itsStream << fileOpen;	//***CW8	// platform independant files
	*itsStream << mLastRecID;
	DB_DEBUG("WriteHeader("<<(int)inFileOpen<<")", DEBUG_TRIVIA);
}

bool	//** Thread Safe **
ADataStore::HeaderExists() {
	return (itsStream->GetLength() > 8);
}


#if DB_V15

void	// ** NOT THREAD SAFE **
ADataStore::SetStructure(UStructure* inStructure) {
	ASSERT(inStructure != nil);	// еее pass by reference
	mStructure = inStructure;
}

// creates a CRecord object of proper Structure
CRecord*	//** Thread Safe **
ADataStore::MakeNewEmptyRecord() {
	DB_DEBUG("MakeNewEmptyRecord()");
	return MakeRecordObject((DatabaseRec*)nil);
}

// creates new CRecord object and fills it with the data of the record specified by inRecID
CRecord*	//** Thread Safe **
ADataStore::ReadRecord(RecIDT inRecID) {
	DB_DEBUG("ReadRecord("<<inRecID<<")");
	CRecord* theRec = (CRecord*)nil;
	long size = GetRecordSize(inRecID);
	if (size > 0) {
		temp_buffer(DatabaseRec, p, size);	// allocate a read buffer
		p->recID = inRecID;
		p->recSize = size;
		ReadRecord(p);
		theRec = MakeRecordObject(p);
	}
	return theRec;
}

// adds a new record with the data in the CRecordObject, returns the ID that was assigned to it
// use MakeNewEmptyRecord() to create the empty record, then you can fill in the fields and
// call this func to add it to the database
RecIDT	//** Thread Safe **
ADataStore::AddRecord(CRecord* inRecP) {
	ASSERT(inRecP != nil);	// еее pass by reference
	DB_DEBUG("AddRecord()");
	RecIDT id = 0;
	AutoLockRecord lock(inRecP);	// this will automatically unlock the record when it goes out
	DatabaseRec* p = inRecP->GetRecordDataPtr(); // of scope, whether normally or from an exception
	id = AddRecord(p);
	return id;
}

// updates an existing record with the data in the CRecord object
// use CRecord* ReadRecord(id) to read in the data, then modify it and call this func to update it
void	//** Thread Safe **
ADataStore::UpdateRecord(CRecord* inRecP) {
	ASSERT(inRecP != nil);	// еее pass by reference
	DB_DEBUG("UpdateRecord("<<inRecP->GetRecordID()<<")");
	AutoLockRecord lock(inRecP);	// automatically unlock the record as we exit this code block
	DatabaseRec* p = inRecP->GetRecordDataPtr();
	UpdateRecord(p);
}

// updates, or adds if doesn't already exist
RecIDT	//** Thread Safe **
ADataStore::WriteRecord(CRecord* inRecP) {
	ASSERT(inRecP != nil);	// еее pass by reference
	DB_DEBUG("WriteRecord("<<inRecP->GetRecordID()<<")");
	bool addIt = false;
	RecIDT id = inRecP->GetRecordID();
	AutoLockRecord lock(inRecP);	// automatically unlock the record as we exit this code block
	DatabaseRec* p = inRecP->GetRecordDataPtr();
	Try_{
		UpdateRecord(p);
	}
	Catch_(err) {
		if (err != dbItemNotFound) {
			Throw_(err);
		} else {
			addIt = true;
	    }
	}
	if (addIt) {
		id = AddRecord(p);
    }
	return id;
}

RecIDT	//** Thread Safe **
ADataStore::FindRecord(CRecord* inRecP, LComparator *) {	//inComparator
	ASSERT(inRecP != nil);
	RecIDT resultID = kInvalidRecID;
	int ignore; //*** ADataStore::FindRecord() NOT YET IMPLEMENTED ***
	//*** not sure how I want to implement this yet
	DB_DEBUG("FindRecord(): "<< resultID);
	return resultID;
}

// deletes the record and the object too and sets the pointer passed in to nil also catches
// the exception if the record was not found, since we were going to delete it anyway
void	//** Thread Safe **
ADataStore::DeleteRecord(CRecord* &inRecP) {
	ASSERT(inRecP != nil);
	DB_DEBUG("WriteRecord("<<inRecP->GetRecordID()<<")");
	Try_{
		DeleteRecord(inRecP->GetRecordID());
	}
	Catch_(err) {
		if (err != dbItemNotFound)
			Throw_(err);			// pass on the exception if it was something other than notFound
	}
	delete inRecP;
	inRecP = (CRecord*)nil;
}

// new platform independant calls (handle based access methods)

// handle created is yours to do with as you will
handle	//** Thread Safe **
ADataStore::ReadRecordHandle(RecIDT inRecID) {
	handle h;
	long fullsize = GetRecordSize(inRecID);
	long size = fullsize - sizeof(DatabaseRec);
	if (size > 0) {
		h.resize(fullsize);
		h.lock();
		DatabaseRec* p = (DatabaseRec*) *h;
		p->recID = inRecID;
		p->recSize = fullsize;
		Try_{
			ReadRecord(p);
		}
		Catch_(err) {
			h.dispose();	// clean up our mess
			Throw_(err);
		}
		::BlockMoveData(&p->recData[0], p, size);
		h.unlock();
		h.resize(size);
	}
	DB_DEBUG("ReadRecordHandle("<<inRecID<<"): " << (void*)h);
	return h;
}

// NOTE: handle not disposed of, you must call handle.dispose() yourself
RecIDT	//** Thread Safe **
ADataStore::AddRecord(handle inRecH, RecIDT inNewRecID) {
	RecIDT newID;
	long newSize = inRecH.size() + sizeof(DatabaseRec);
	temp_buffer(DatabaseRec, p, newSize); // allocate write buffer disposed automatically
	inRecH.copyto(&p->recData[0]);
	p->recID = inNewRecID;
	p->recSize = newSize;
	newID = AddRecord(p);
	return newID;
}

// NOTE: handle not disposed of, you must call handle.dispose() yourself
void	//** Thread Safe **
ADataStore::UpdateRecord(RecIDT inRecID, handle inRecH) {
	long newSize = inRecH.size() + sizeof(DatabaseRec);
	temp_buffer(DatabaseRec, p, newSize); // allocate write buffer disposed automatically
	inRecH.copyto(&p->recData[0]);
	p->recID = inRecID;
	p->recSize = newSize;
	UpdateRecord(p);
}

// private to ADataStore, but virtual, can be overriden if you want to create a subclass
// of UStructure that tells the CRecords to handle their field storage differently
UStructure*
ADataStore::MakeStructureObject(StrcResPtr inStrcRecP) {
	return new UStructure(inStrcRecP);
}

// private to ADataStore, but virtual, can be overriden if you want the DataStore to use
// a subclass of CRecord. This should not be necessary very often as you can define whatever
// structure you like for the record using the Strc resource.
CRecord*
ADataStore::MakeRecordObject(DatabaseRec* inData) {
	if (inData)
		return new CRecord(mStructure, inData);
	else
		return new CRecord(mStructure);
}


#elif ( PLATFORM_MACOS || DB_WIN32_USING_QTML)
	// ========================== OLD CALLS =====================================

// *** MacOS dependant calls (handle based access methods)

// Handle created is yours to do with as you will
Handle	//** Thread Safe **
ADataStore::ReadRecord(RecIDT inRecID) {
	long fullsize = GetRecordSize(inRecID);
	long size = fullsize - sizeof(DatabaseRec);
	if (size > 0) {
		Handle theHandle = ::NewHandle(fullsize);
		::HLock(theHandle);
		DatabaseRecPtr p = (DatabaseRecPtr) *theHandle;
		p->recID = inRecID;
		p->recSize = fullsize;
		ReadRecord(	*(DatabaseRecHnd)theHandle );
		::BlockMoveData( &((**(DatabaseRecHnd)theHandle).recData[0]), *theHandle, size);
		::HUnlock(theHandle);
		::SetHandleSize(theHandle, size);	// optional instruction, wastes 12 bytes without it
		DB_DEBUG("ReadRecord("<<inRecID<<"): (Handle) "<<(void*)theHandle, DEBUG_TRIVIA);
		return theHandle;
	} else {
		DB_DEBUG("ReadRecord("<<inRecID<<"): (Handle) 0", DEBUG_TRIVIA);
		return nil;
	}
}

// NOTE: Handle not disposed of, you must call DisposeHandle() yourself
RecIDT	//** Thread Safe **
ADataStore::AddRecord(Handle inRecH, RecIDT inNewRecID) {
	DB_DEBUG("AddRecord( (Handle) "<<(void*)inRecH<<", "<<inNewRecID<<" )", DEBUG_TRIVIA);
	long size = (inRecH != nil) ? ::GetHandleSize(inRecH) : 0;
	long newSize = size + sizeof(DatabaseRec);
	DatabaseRecPtr p = (DatabaseRecPtr) ::NewPtr(newSize);
	if (inRecH) {
		::BlockMoveData( *inRecH, &p->recData[0], size);
	}
	p->recID = inNewRecID;
	p->recSize = newSize;
	RecIDT newID = AddRecord(p);
	::DisposePtr((Ptr)p);
	return newID;
}

// NOTE: Handle not disposed of, you must call DisposeHandle() yourself
void	//** Thread Safe **
ADataStore::UpdateRecord(RecIDT inRecID, Handle inRecH) {
	DB_DEBUG("UpdateRecord( (Handle) "<<(void*)inRecH<<", "<<inRecID<<" )", DEBUG_TRIVIA);
	long size = (inRecH != nil) ? ::GetHandleSize(inRecH) : 0;
	long newSize = size + sizeof(DatabaseRec);
	DatabaseRecPtr p = (DatabaseRecPtr) ::NewPtr(newSize);
	if (inRecH) {
		::BlockMoveData( *inRecH, &p->recData[0], size);
	}
	p->recID = inRecID;
	p->recSize = newSize;
	UpdateRecord(p);
	::DisposePtr((Ptr)p);
}
#endif	// MacOS
