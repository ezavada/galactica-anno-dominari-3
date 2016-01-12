// GenericUtils.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 9/5/96 ERZ

#ifndef GENERIC_UTILS_H_INCLUDED
#define GENERIC_UTILS_H_INCLUDED

#include <LString.h>
#include <string>


//=======================================================================

// uncomment the following line to have errors fixed rather than reported to the user

#define FIX_ERRORS

// comment out this line if you want to use the this in a project with without LArray
#define INCLUDE_LARRAY_TOOLS

// some debugging compiler variables have been moved to "Galactica_DebugHeaders.pch++"

#ifndef DEBUG_OUT
  #define DEBUG_OUT(x,n)
#endif


long	Rnd(long max);
long	SignedRnd(long max);
long	Min(long a, long b);
long	Max(long a, long b);
void	IncreaseByPercent(long &ioValue, const int inPercent);
void	IncreaseByPercent(float &ioValue, const int inPercent);

// load a generic resource in a new block of memory. You must call UnloadResource() when
// done with it. Size of the resource is returned in outResourceSize. Pass in NULL for
// out resource size if you don't care about the size.
void*           LoadResource(long inResType, short inResID, long *outResSize = NULL);
void            UnloadResource(void* inResPtr);

// load a string based on a string resource number or a string list resource number
// and index number into the string list
std::string&    LoadStringResource(std::string& inStr, int inStringNum, int inSubStringNum = 0);
int             GetNumSubStringsInStringResource(int inStringNum);

// some convenient conversions
std::string&    Uppercase(std::string& inStr);
std::string&    Lowercase(std::string& inStr);
std::string&    PadLeft(std::string& inStr, int inToLength, char inPadChar);
std::string&    PadRight(std::string& inStr, int inToLength, char inPadChar);
void            BinaryDump(char *outBuf, int outBufSize, char *inBuf, int inBufSize);

//LStr255&        Uppercase(LStr255& inStr);
//LStr255&        PadLeft(LStr255 &inString, int inToLength, char inPadChar);
//LStr255&        PadRight(LStr255 &inString, int inToLength, char inPadChar);

#define kRestrictTo7BitAscii true

LStr255&    ReplaceIllegalCharacters(LStr255 &inString, const char* inIllegalChars, char inReplacementChar, bool restrictToAscii = false);
char*       ReplaceIllegalCharacters(char* inString, const char* inIllegalChars, char inReplacementChar, bool restrictToAscii = false);

bool    Secs2TimeStr(unsigned long inRawDTSecs, std::string& inStr, int inFormat = 0);
char*   LongTo4CharStr(long n);	// displays as an OSType

int get_timezone();

#ifdef __cplusplus	// do we want to include C++ utility classes?

class UClassInfo {
public:
	UClassInfo(long inClassType = 0x2d2d2d2d) {mType = inClassType;} // 0x2d2d2d2d is '----'
	long	GetClassType() {return mType;}
	void	SetClassType(long inClassType) {mType = inClassType;}
protected:
	long	mType;
};

#ifdef INCLUDE_LARRAY_TOOLS
#include <LArray.h>

// this is an abstract class for performing an action on every element of an array
// override the DoAction() method to perform the desired action, and return true if the
// item should be written back into the array. It is perfectly fine to delete the item
// from the array, duplicate it in the array, and otherwise do anything you want to it.
// Don't try to change the size of the item, though, or you'll overwrite the buffer that
// is used to hold the current item.
// call UArrayAction::DoForEachElement(array, action) to perform the action
typedef class UArrayAction {
protected:
	virtual bool DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData) = 0;
public:
	static void DoForEachElement(LArray &inTargetArray, UArrayAction &inAction);
	virtual ~UArrayAction() {};
} UArrayAction;
#endif	//INCLUDE_LARRAY_TOOLS



#endif // __cplusplus

#endif // GENERIC_UTILS_H_INCLUDED
