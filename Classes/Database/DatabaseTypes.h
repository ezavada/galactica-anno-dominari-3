// =============================================================================
// DatabaseTypes.h                         ©1995-96, Sacred Tree Software, inc.
// 
// Common definitions and structures for working with databases
//
// created:   7/23/95, ERZ
// modified:  7/29/96, ERZ	v1.5, macros for Win32, defined new error codes
// modified:  4/26/97, ERZ  v1.5.1, debugging macros added
// modified: 11/17/98, ERZ
// modified: 12/11/99, ERZ, changed enums to ExceptionCodes
// =============================================================================

#ifndef DATABASETYPES_H_INCLUDED
#define DATABASETYPES_H_INCLUDED

#include "pdg/sys/platform.h"
#include <PP_Prefix.h>

#ifdef DEBUG_MACROS_ENABLED
#include "DebugMacros.h"
#endif // DEBUG_MACROS_ENABLED

#include "GenericUtils.h"

// define as 1 to use new abilities in version 1.5 of the database engine
#ifndef DB_V15
  #define DB_V15	0
#endif

#if DB_THREAD_SUPPORT
  #ifndef __THREADS__
	#include <Threads.h>
  #endif
  #include <LMutexSemaphore.h>
  #include <LThread.h>
  #include <UThread.h>
  #include "StSafeMutex.h"
#else
  #undef DB_REQUIRE_THREADS
  #define DB_REQUIRE_THREADS 0
#endif

// define as 1 to bypass the data integrity checks that are normally performed upon opening a file
#ifndef DB_IGNORE_ERRORS
  #define DB_IGNORE_ERRORS 0
#endif

// define as 1 for full integrity checking of all actions, same as debug checking but without logging
#ifndef DB_INTEGRITY_CHECKING
  #define DB_INTEGRITY_CHECKING 1
#endif

// define as 1 for full debug checking of all actions
#ifndef DB_DEBUG_MODE
  #define DB_DEBUG_MODE 0
#endif
#if DB_DEBUG_MODE
	#undef DB_DEBUG_LOG	// debug mode automatically turns on debug logs
	#define DB_DEBUG_LOG 1
#endif

#ifndef DEBUG_DATABASE
	#define DEBUG_DATABASE (1<<2)	// defined so that we can use this with DebugMacros.h
#endif

// define as 1 to turn on debug logging
#ifndef DB_DEBUG_LOG
	#define DB_DEBUG_LOG 0
#endif
#if DB_DEBUG_LOG
//	#undef DB_LOG_ACTIONS			// logging is redundate with debug logging turned on
//	#define DB_LOG_ACTIONS 0
	#ifdef DEBUG_MACROS_ENABLED		// in case we are using DebugMacros.h
		#define DB_DEBUG(x,n)   DEBUG_OUT(x, n | DEBUG_DATABASE)
	#else
		#define DEBUG_ERROR 0L
		#define DEBUG_TRIVIA 1L
		#define DB_DEBUG_LOG_FILENAME "DB_Debug.log"
		#include <fstream>
		extern std::ofstream db_debug_log;
		#define DB_DEBUG(x,n)		db_debug_log << x << '/' << (void*)this << "\n"; db_debug_log.flush();
	#endif
#else
	#define DB_DEBUG(x,n)
#endif

// define as 1 to record all actions to a log file
#ifndef DB_LOG_ACTIONS
	#define DB_LOG_ACTIONS 0
	#define DB_LOG(x)
#elif DB_LOG_ACTIONS
	#include <stdio.h>
	#define DB_LOG_FILENAME "Database.log"
	#include <fstream>		
	extern std::ofstream db_log;
	#define DB_LOG(x)	db_log << x <<  '/' << (void*)this <<"\n"; db_log.flush();
#else
	#define DB_LOG(x)
#endif

#define RecIDT	SInt32
#define Bool32	UInt32		// platform independant booleans
#define Bool16	UInt16		
#define Bool8	UInt8

#define kBatchModeOn	true
#define kBatchModeOff	false

#define kFileIsOpen		true
#define kFileIsClosed	false

#define kInvalidRecID	0

#define kDataFileHeaderSize		0x40
#define kDBFilenameBufferSize   256 // max size of filename

const ResIDT kInvalidRecord = 0;

const unsigned long kOSType_data = 0x64617461; // 'data'
const unsigned long kOSType_indx = 0x696e6478; // 'indx'

#ifdef __MWERKS__
	#pragma options align=mac68k
#endif // __MWERKS__
typedef struct DatabaseRec {
	SInt32	recPos;		// this field is not always used
	SInt32	recSize;		// this field has allocation size of a DatabaseRecPtr to hold given rec
	RecIDT	recID;
	UInt8	recData[];	// indeterminate other data of unknown size
} DatabaseRec, *DatabaseRecPtr, **DatabaseRecHnd, **DatabaseRecHdl;
#ifdef __MWERKS__
	#pragma options align=reset
#endif // __MWERKS__

#define dbRecOffset_RecPos		0
#define dbRecOffset_RecSize		4
#define dbRecOffset_RecID		8
#define dbRecOffset_RecData		12
#define dbRecSize_RecHeader		12

/*
enum {
	dbIndexRequired = 30000,	// attempted open file while required index was unavailable
	dbIndexCorrupt 	= 30001,	// the index file contains invalid header information
	dbDataCorrupt 	= 30002,	// the data file contains invalid header information
	dbInvalidID		= 30003,	// tried to use an invalid ID number for a record
	dbItemNotFound	= 30004,	// couldn't find item with specified record ID
	dbRecNotFound	= 30004,	// (alternate name for above)
	dbIndexLeftOpen = 30005,	// the index file was not closed properly, and may be corrupt
	dbFileLeftOpen	= 30006,	// the dat file was not closed properly, and may be corrupt
	dbSizeMismatch	= 30007,	// the record size in the header is not what was expected
	dbNoSuchField	= 30008,	// the specified field does not exist	NOT USED
	dbInvalidView	= 30009,	// the specified view does not exist 	NOT USED
	dbDuplicateKey	= 30010,	// tried to add a duplicate key to a unique key index
	dbNoComparator	= 30011		// tried to search without a valid LComparator
};
*/

const ExceptionCode dbIndexRequired	= 30000;
const ExceptionCode dbIndexCorrupt 	= 30001;
const ExceptionCode	dbDataCorrupt 	= 30002;
const ExceptionCode	dbInvalidID		= 30003;
const ExceptionCode	dbItemNotFound	= 30004;
const ExceptionCode	dbRecNotFound	= 30004;	// alternate name
const ExceptionCode	dbIndexLeftOpen = 30005;
const ExceptionCode	dbFileLeftOpen	= 30006;
const ExceptionCode	dbSizeMismatch	= 30007;
const ExceptionCode	dbNoSuchField	= 30008;
const ExceptionCode	dbInvalidView	= 30009;
const ExceptionCode dbDuplicateKey	= 30010;	// tried to add a duplicate key to a unique key index
const ExceptionCode dbNoComparator	= 30011;	// tried to search without a valid LComparator


typedef UInt32 IndexT;

enum {
	index_Bad	= 0,
	index_First	= 1,
	index_Last	= 0x7FFFFFFF
};

#endif // DATABASETYPES_H_INCLUDED
