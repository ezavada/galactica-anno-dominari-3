//	DebugMacros.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
//	Code and variables used by debugger macros in DebugMacros.h
//
//	If DEBUG_MACRO_SIOUX is defined, this file becomes MacOS and CodeWarrior
//  specific. This is because it expects to use the SIOUX library for MacOS 
//	console i/o.
//
//	If DEBUG_NEW is defined at all, this becomes MacOS and CodeWarrior specific
//	at least, since it expects to use the debug versions of the C++ new and 
//	delete operators which are not likely to be available under other compilers.
//	and calls the MacOS DebugStr68k() function to report memory problems.
//
//	Otherwise, this should be platform and compiler neutral.
//
// modified, 9/16/98, ERZ - added SetInfoString() and GetDateTime() calls
// modified, 6/9/02,  ERZ - switch to new platform.h, loosely based on 
//                          Apple's ConditionalMacros.h


#include "DebugMacros.h"
#include "pdg/sys/platform.h"	// defines macros for each possible OS we could be targeting
#include "pdg/sys/log.h"    
#include <string.h>
#include <time.h>

// Constants that shouldn't change
const int TIME_MAX_SIZE         = 20;	// The max size for time that strftime will take
const int TIME_BUFFER_SIZE      = 20;	// Max size for the time buffer
const int DATE_BUFFER_SIZE      = 40;	// Max size for the date buffer
const int DEFAULT_FILENAME_SIZE = 40;	// The default filename size that get's initialized on initialization
const int FILENAME_SIZE         = 256;	// The regular filename size

#ifdef PLATFORM_MACOS
  #include <MacTypes.h>
  #if ( DEBUG_SEND_TO_STDERR )
  #include <SIOUX_Far.h>		// include SIOUX console i/o if needed for MacOS
#endif
#endif

#ifdef PLATFORM_WIN32
	#include <windows.h>
#endif

#ifdef DEBUG_MACROS_ENABLED

#if (defined(DEBUG_NEW) && !defined(CHECK_FOR_MEMORY_LEAKS))
	#define CHECK_FOR_MEMORY_LEAKS
#endif

#ifdef DEBUG_NEW
static void DebugNew_ErrorHandler(short err);
#endif
static void DebugMacros_BreakErrorString(char *s);

bool	gDebugMacrosInitialized = false;
int		gDebugLevel = DEBUG_SHOW_NONE;
long	gDebugErrorMask = 0;		// A set of bits to determine which errors are always
									// shown. Setting it to zero means nothing always shown.
char	gDebugMacros_TimeStrBuff[TIME_BUFFER_SIZE];
char	gDebugMacros_DateTimeStrBuff[DATE_BUFFER_SIZE];
int		gDebugBreakOnErrors;
char*	gDebugOutputFilename;
char*	gDebugOutputInfoStr;

pdg::Mutex* gDebugMacrosMutex = NULL;

pdg::log gDebugLog;

#if DEBUG_SEND_TO_FILE
std::ofstream* out = NULL;
#endif

char* DebugMacros_GetFilename() {
	return gDebugOutputFilename;
}

char* DebugMacros_GetInfoString() {
	return gDebugOutputInfoStr;
}

void DebugMacros_SetFilename(char* inStr) {
	static char s_debugoutfilename[FILENAME_SIZE];
	strncpy(s_debugoutfilename, inStr, FILENAME_SIZE-1);
	s_debugoutfilename[FILENAME_SIZE-1] = '\0';
	gDebugOutputFilename = s_debugoutfilename;
	if (out != NULL) {	// close down output file
		out->close();	// so it will be reopened with
		out = NULL;		// new name
	}
}

// will be first line in any log file
void DebugMacros_SetInfoString(char* inStr) {
	static char s_debugoutinfo[FILENAME_SIZE];
	strncpy(s_debugoutinfo, inStr, FILENAME_SIZE-1);
	s_debugoutinfo[FILENAME_SIZE-1] = '\0';
	gDebugOutputInfoStr = s_debugoutinfo;
}


void DebugMacros_Initialize() {
	if (gDebugMacrosInitialized) {
		return;
	}
	time_t lclTime;
	struct tm *now;
	char defaultfilename[DEFAULT_FILENAME_SIZE];
	gDebugLevel = DEBUG_SHOW_ERRORS;
	gDebugBreakOnErrors = false;
	lclTime = time(NULL);
	now = localtime(&lclTime);
    strftime(defaultfilename, TIME_MAX_SIZE*2, "debug_%Y%m%d_%H%M%S.out", now);
	DebugMacros_SetFilename(defaultfilename);
	DebugMacros_SetInfoString("");
	out = NULL;
	gDebugMacrosInitialized = true;
	gDebugMacrosMutex = new pdg::Mutex();
#ifdef DEBUG_NEW
  #if DEBUG_NEW != DEBUG_NEW_OFF
//  	DebugNewSetErrorHandler(DebugNew_ErrorHandler);
  #endif
#endif
#if DEBUG_MACRO_SIOUX						// MacOS and CodeWarrior specific
	SIOUXSettings.initializeTB 	= false;	// if we use the SIOUX library
	SIOUXSettings.standalone 	= false;
	SIOUXSettings.setupmenus 	= false;
#endif
}



void DebugMacros_Terminate() {
    delete gDebugMacrosMutex;
    gDebugMacrosMutex = 0;
#if SEND_DEBUGGING_TEXT_TO_FILE
	if (out != NULL)
		out->close();
#endif
//#ifdef CHECK_FOR_MEMORY_LEAKS
//	DebugNewReportLeaks();
//#endif
}



// will return the date and time as "hh:mm:ss "
char* DebugMacros_GetTime() {	// get time using std c libs
	time_t lclTime;
	struct tm *now;
	lclTime = time(NULL);
	now = localtime(&lclTime);
	strftime(gDebugMacros_TimeStrBuff, TIME_MAX_SIZE, "%H:%M:%S ", now);
	return gDebugMacros_TimeStrBuff;
}


// will return the date and time as "yyyymmdd hh:mm:ss "
char* DebugMacros_GetDateTime() {	// get time using std c libs
	time_t lclTime;
	struct tm *now;
	lclTime = time(NULL);
	now = localtime(&lclTime);
    strftime(gDebugMacros_DateTimeStrBuff, TIME_MAX_SIZE*2, "%Y%m%d %H:%M:%S ", now);
	return gDebugMacros_DateTimeStrBuff;
}


int DebugMacros_HaveLevel(long n) {
// Check to see if the Debug level is high enough to show this kind of error. 
	if (gDebugLevel > (n & DEBUG_LEVEL_MASK)) {
		return true;	// error was of proper level
	} else if (gDebugLevel && ((gDebugErrorMask & n)>DEBUG_LEVEL_MASK)) {
		return true;	// error is of type that will always be shown
	} else {
		return false;
	}
}

void DebugMacros_DoError(long n, char* message) {
#pragma unused(n)
	if (gDebugBreakOnErrors) {		// break on error if
		DebugMacros_BreakErrorString(message);
	} else if (gDebugLevel == DEBUG_SHOW_ERRORS) {	// automatically turn up debug
		gDebugLevel = DEBUG_SHOW_SOME;				// level if just showing errors
	}												// since now we found one
}


static void DebugMacros_BreakErrorString(char *s) {
	if (!gDebugBreakOnErrors) {
		return;
	}
  #ifdef PLATFORM_MACOS	// MacOS specific
	char 	s2[FILENAME_SIZE];
	int len = strlen(s);
  	if (len > FILENAME_SIZE-1) {	// make sure we don't overwrite our vstring buffer
  		s[FILENAME_SIZE-1] = 0;
  		len = FILENAME_SIZE-1;
  	}
	s2[0] = len;		// convert to a Pascal vstring so we can
	strcpy(&s2[1], s);	// send it to MacsBug
	MacAPI::DebugStr((StringPtr)&s2[0]);
  #elif defined(PLATFORM_WIN32)
	WinAPI::MessageBox( WinAPI::GetFocus(), s , "Error", MB_OK|MB_SYSTEMMODAL );
  #elif defined(PLATFORM_UNIX)
    // no idea how to do a debug break under unix, short of doing an abort()
  #endif
}


#if defined(DEBUG_NEW) && (DEBUG_NEW != DEBUG_NEW_OFF)
  
static void DebugNew_ErrorHandler(short err) {
	char *s;
	switch (err) {

		case dbgnewNullPtr:
			s = "DebugNew: null pointer";
			break;

		case dbgnewTooManyFrees:
			s = "DebugNew: more deletes than news";
			break;

		case dbgnewPointerOutsideHeap:
			s = "DebugNew: delete or validate called for pointer outside application heap";
			break;

		case dbgnewFreeBlock:
			s = "DebugNew: delete or validate called for free block";
			break;

		case dbgnewBadHeader:
			s = "DebugNew: unknown block, or block header was overwritten";
			break;

		case dbgnewBadTrailer:
			s = "DebugNew: block trailer was overwritten";
			break;

		case dbgnewBlockNotInList:
			s = "DebugNew: block valid but not in block list (internal error)";
			break;

		case dbgnewFreeBlockOverwritten:
			s = "DebugNew: free block overwritten, could be dangling pointer";
			break;

//		case dbgnewErrLeakingBlock:
//			s = "DebugNew: block was previously leaked in leaks.log";
//			break;

/*		case dbgnewMismatchedNewDelete:
			s = "DebugNew: mismatched regular/array new and delete (probably delete on new[] block)";
			break;
				*/				
		default:
			s = "DebugNew: undefined error";
			break;
	}
	DEBUG_OUT(s, DEBUG_ERROR);
	if (gDebugBreakOnErrors) {
		DebugMacros_BreakErrorString(s);
	}

}
#endif	//DEBUG_NEW

#endif // DEBUG_MACROS_ENABLED



