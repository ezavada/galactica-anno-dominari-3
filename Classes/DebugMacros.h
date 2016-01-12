/*	DebugMacros.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

	Debugger Macros used in Galactica. Be sure to check out DebugNew.h for info
	on catching memory leaks (with CodeWarrior on MacOS)

	These macros should be platform and compiler neutral.


Typical usage:

	#define DEBUG_MACROS_ENABLED	// if not defined, including header does nothing
	
	#include "DebugMacros.h"	// have to include the headers, and
								// be sure to add "DebugMacros.cp" to your
								// project
	
	main() {
		DEBUG_INITIALIZE();					// must initialize before using
		DEBUG_SET_LEVEL(DEBUG_SHOW_SOME);	// what level of debug reporting are we going to show
		...
		DoStuff();
		...
		DEBUG_TERMINATE();					// must do this to close files, etc.
	}

	void DoStuff() {
		DEBUG_OUT("Starting DoStuff()", DEBUG_IMPORTANT);	// typical program flow notice, you
			// should generally use these to help give context to error messages. You should feel
			// pretty comfortable with the logs generated when these are turned on, and be willing
			// to have this turned on constantly.

		ASSERT(globalPtr != NULL);	// typical assertion, gives DEBUG_ERROR message if not true.
			// ASSERTs are probably one of the best tools for debugging because you can use them
			// to verify your assumptions at every level of the code. ASSERTS, like DEBUG_ERROR
			// messages, will show the source file name and line number. In fact, ASSERT uses
			// DEBUG_OUT(x, DEBUG_ERROR) to actually output the message
		...
		int verifyCount = 0;
		while (bVerify) {
			verifyCount++;
			DEBUG_OUT("Verifying item #"<<verifyCount, DEBUG_TRIVIA);	// use DEBUG_TRIVIA for
							// stuff you know you won't want to have to wade through in the logs
							// every time, and you would only want to turn on for very short 
							// periods of time to track down a really stubborn localized bug.
							// It also might make sense to convert DEBUG_DETAIL to DEBUG_TRIVA
							// in a thoroughly debugged subsystem that is no longer undergoing
							// revision.
			...
		}
		DEBUG_OUT("Verified " << verifyCount << " items", DEBUG_DETAIL);	// typical message
			// for detailed reporting, use DEBUG_DETAIL for the kind of stuff that will be
			// helpful when you know a bug is there and you are trying to see more detailed
			// program flow or status
		...
	#ifdef DEBUG
			// we also have a convenient DEBUG macro defined so that we know this is
			// a debug build and can enable more extensive internal checks that we only
			// want in debug versions.
		this->ExtensiveDebuggingChecks();
	#endif
		..
		if (this->IsNotWhatIExpected()) {
			DEBUG_OUT("My detailed message of what went wrong.", DEBUG_ERROR);	// the debug
				// error will show the source file and line number along with the message.
				// Generally, you should use asserts to detect errors, but sometimes you want
				// to trap and handle an error that you can't stop from happening.
				// In those cases, your error repair code should always include a debug notice
				// that the error occured. Never let an error go un-reported in your debug builds,
				// even if you are positive you have handled it.
			...
			FixIt(this->Problem());
		}
	}

*/

#ifndef DEBUG_MACROS
#define DEBUG_MACROS

// these defines are used to set the global debugging level
#define DEBUG_SHOW_NONE		0L	// Not even errors are reported
#define DEBUG_SHOW_ERRORS	1L	// Only errors reported
#define DEBUG_SHOW_SOME		2L	// Error and Important messages reported
#define DEBUG_SHOW_MOST		3L	// Error, Important and Detail reporting
#define DEBUG_SHOW_ALL		4L	// Everything is reported, even Trivia

// these defines are used in DEBUG_OUT() macros to specify the type of output:
#define DEBUG_ERROR			0L	// highest priority, indicates an error or irregularity
#define DEBUG_IMPORTANT		1L	// Use this for overall program flow/status.
#define	DEBUG_DETAIL		2L	// Use for high level of detail.
#define DEBUG_TRIVIA		3L	// You wouldn't want to have to read all these
#define DEBUG_TRACE_MASK	4L	// Used for tracing program flow
// further masks should be defined as powers of 2, starting with 2^3

#define DEBUG_EOL           ((char)0x0A)

#ifdef DEBUG_MACROS_ENABLED

	#ifndef __cplusplus
		#error DebugMacros.h Requires C++
	#endif

	// provide overrides for some PDG Debug Macros
	#ifndef DEBUG_MACROS_DONT_OVERRIDE_PDG
	    #define DEBUG_ASSERT(cond, msg) ASSERT_STR(cond, msg)
		#define DEBUG_BREAK(msg) DEBUG_OUT(msg, DEBUG_ERROR)
		#define CHECK_PTR(ptr, block, block_size) if (((char*)(ptr)<(char*)(block))||((char*)(ptr)>=(char*)((char*)(block)+(block_size)))) { \
			DEBUG_OUT("PTR ERROR:" << __FILE__ << ':' << (int)__LINE__ << " [" << (void*) ptr << "] is outside block [" \
						<<(void*)block<<"] len ["<<block_size<<"]", DEBUG_ERROR); }
	#endif

	// following variables and functions declared in DebugMacros.cp
	extern int	gDebugLevel;
	extern long gDebugErrorMask;
	void	DebugMacros_Initialize();
	void	DebugMacros_Terminate();
	char*	DebugMacros_GetTime();			// HH:MM:SS, 24 hour clock
	char*	DebugMacros_GetDateTime();		// YYYYMMDD HH:MM:SS, 24 hour clock
	char*	DebugMacros_GetFilename();
	char*	DebugMacros_GetInfoString();
	void	DebugMacros_SetFilename(char* inStr);
	void	DebugMacros_SetInfoString(char* inStr);	// will be first line in any log file
	int		DebugMacros_HaveLevel(long n);
	void	DebugMacros_DoError(long n, char* message);
	
	#define DEBUG_LEVEL_MASK	0x3L

	#define DEBUG_INITIALIZE()		DebugMacros_Initialize()
	#define	DEBUG_TERMINATE()		DebugMacros_Terminate()
	#define DEBUG_SET_LEVEL(n)		if ((n)<=DEBUG_SHOW_ALL)			\
										gDebugLevel = (n); 				\
									else								\
										gDebugLevel = DEBUG_SHOW_ALL;
	#define DEBUG_SET_FILENAME(str)	DebugMacros_SetFilename(str)
	#define DEBUG_SET_INFO_STR(str)	DebugMacros_SetInfoString(str)
	#define DEBUG_ENABLE_ERROR(n)	gDebugErrorMask |= (n)
	#define DEBUG_DISABLE_ERROR(n)	gDebugErrorMask &= ~(n)

	// change this if you want the debugger output to go to the console instead of a file
	#ifndef DEBUG_SEND_TO_FILE
		#define DEBUG_SEND_TO_FILE		1	// true
	#endif
	#ifndef DEBUG_SEND_TO_STDERR
		#define DEBUG_SEND_TO_STDERR	0	// false
	#endif
	
	#include <cstdio>
	#include <iostream>
	#include <ios>
    
	#include "pdg/sys/mutex.h"
	extern pdg::Mutex* gDebugMacrosMutex;
	
	#if DEBUG_SEND_TO_FILE
		#define OUTPUT_FILENAME "debug.out"
		#include <fstream>
		extern std::ofstream* out;
	#endif

	#if (DEBUG_SEND_TO_STDERR && DEBUG_SEND_TO_FILE)
		#define DEBUG_OUT(x,n)	if (DebugMacros_HaveLevel(n)) {								\
								  pdg::AutoMutexNoThrow acs(gDebugMacrosMutex);         	\
							      if (acs.tryAquire(10)) {          						\
								  #if defined ( PLATFORM_MACOS )							\
									GrafPtr dbgPort;										\
									::GetPort(&dbgPort);								 	\
								  #endif													\
									if (out == NULL) {										\
										out = new std::ofstream(DebugMacros_GetFilename(), std::ios::binary);\
										*out << DebugMacros_GetInfoString();				\
									}														\
									*out << DebugMacros_GetTime();							\
									if (((n) & DEBUG_LEVEL_MASK) == DEBUG_ERROR) {			\
										cerr << DebugMacros_GetTime();						\
										*out<<"**File: "__FILE__", Line: "<<__LINE__<<": ";	\
										cerr<<"**File: "__FILE__", Line: "<<__LINE__<<": ";	\
									} else {												\
										clog << DebugMacros_GetTime();						\
									}														\
									*out<< x << DEBUG_EOL << '\0'; 							\
									out->flush();											\
									if (((n) & DEBUG_LEVEL_MASK) == DEBUG_ERROR) {			\
											cerr << x << '\n';								\
										} else {											\
											clog << x << '\n';								\
										}													\
								  #if defined ( PLATFORM_MACOS )							\
									::SetPort(dbgPort);										\
								  #endif													\
									if (((n) & DEBUG_LEVEL_MASK) == DEBUG_ERROR) {			\
										DebugMacros_DoError(n, "");							\
									}														\
								  }                                                         \
								}
	#elif DEBUG_SEND_TO_FILE	// to file only
		#define DEBUG_OUT(x,n)	if (DebugMacros_HaveLevel(n)) {								\
								  pdg::AutoMutexNoThrow acs(gDebugMacrosMutex);         	\
							      if (acs.tryAquire(10)) {          						\
									if (out == NULL) {										\
										out = new std::ofstream(DebugMacros_GetFilename(), std::ios::binary);\
										*out << DebugMacros_GetInfoString();				\
								    }                                                       \
									*out << DebugMacros_GetTime();							\
									if (((n) & DEBUG_LEVEL_MASK) == DEBUG_ERROR) {			\
										*out<<"**File: "__FILE__", Line: "<<__LINE__<<": "; \
									}														\
									*out << x << DEBUG_EOL << '\0'; 						\
									out->flush();											\
									if (((n) & DEBUG_LEVEL_MASK) == DEBUG_ERROR) {			\
										DebugMacros_DoError(n, "");							\
									}														\
								  }                                                         \
								}
	#elif DEBUG_SEND_TO_STDERR	// to stderr only
		#define DEBUG_OUT(x,n)	if (DebugMacros_HaveLevel(n)) {								\
								  pdg::AutoMutexNoThrow acs(gDebugMacrosMutex);         	\
							      if (acs.tryAquire(10)) {          						\
								  #if defined ( PLATFORM_MACOS )							\
									GrafPtr dbgPort;										\
									::GetPort(&dbgPort);								 	\
								  #endif													\
									if (((n) & DEBUG_LEVEL_MASK) == DEBUG_ERROR) {			\
										cerr << DebugMacros_GetTime();						\
										cerr <<"**File: "__FILE__", Line: "<<__LINE__<<": ";\
										cerr << x << '\n';									\
									} else {												\
										clog << DebugMacros_GetTime() << x << '\n';			\
									}														\
								  #if defined ( PLATFORM_MACOS )							\
									::SetPort(dbgPort);										\
								  #endif													\
									if (((n) & DEBUG_LEVEL_MASK) == DEBUG_ERROR) {			\
										DebugMacros_DoError(n, "");							\
									}														\
								  }                                                         \
								}
	#endif	// end DEBUG_SEND_TO_STDERR
	
	#define ASSERT(test)		if (!(test)) {												\
									DEBUG_OUT("ASSERT FAILED: " #test, DEBUG_ERROR);		\
								}
		// use ASSERT_STR if you want a better explanation of what happened to go into your
		// debugging logs than the simple "ASSERT FAILED" message that the plain ASSERT macro
		// gives
	#define ASSERT_STR(test, x)	if (!(test)) {												\
									DEBUG_OUT(x, DEBUG_ERROR);								\
								}
	
	#define DEBUG_TRACE_IN(function_name) DEBUG_OUT ("-Enter " #function_name, DEBUG_TRIVIA | DEBUG_TRACE_MASK)

	#define DEBUG_TRACE_OUT(function_name) DEBUG_OUT("-Leave " #function_name, DEBUG_TRIVIA | DEBUG_TRACE_MASK)

#else	// debug reports are not enabled, make all the macros do nothing
	#define DEBUG_INITIALIZE()
	#define	DEBUG_TERMINATE()
	#define DEBUG_SET_LEVEL(n)
	#define DEBUG_SET_FILENAME(str)
	#define DEBUG_SET_INFO_STR(str)
	#define DEBUG_ENABLE_ERROR(n)
	#define DEBUG_DISABLE_ERROR(n)
	#define DEBUG_OUT(x,n)
	#define DEBUG_SEND_TO_FILE		0	// false
	#define DEBUG_SEND_TO_STDERR	0
	#define ASSERT(test)
	#define ASSERT_STR(test, x)
	#define DEBUG_TRACE_IN(function_name)
	#define DEBUG_TRACE_OUT(function_name)
	#undef	DEBUG			//undefine the DEBUG macro
#endif	// DEBUG_REPORTS_ENABLED


#endif // DEBUG_MACROS

