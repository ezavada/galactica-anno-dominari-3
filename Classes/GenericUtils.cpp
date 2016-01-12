// GenericUtils.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 9/5/96 ERZ

#include "pdg/sys/platform.h"
#include "GenericUtils.h"
#include <DebugMacros.h>

#ifdef COMPILING_FOR_CARBON
#include <TextUtils.h>
#endif

// *** Utility Routines

#if PLATFORM_WIN32
    #define FILE_PATH_SEPARATOR '\\'
#elif PLATFORM_UNIX
    #define FILE_PATH_SEPARATOR '/'
#endif

#include <cstdlib>
#include <cctype>

long Rnd(long max) {
	long result = std::rand() % max;
	return (result < 0) ? -result : result;
}

long SignedRnd(long max) {
	long result = std::rand() % max;
	return result;
}

long Min(long a, long b) {
	return (a<b)?a:b;
}
				
long Max(long a, long b) {
	return (a>b)?a:b;
}

void
IncreaseByPercent(long &ioValue, const int inPercent) {
	if (inPercent == 0)
		return;
	long increase = (ioValue * inPercent) / 100;
	ioValue += increase;
}

void
IncreaseByPercent(float &ioValue, const int inPercent) {
	if (inPercent == 0)
		return;
	float increase = (ioValue * inPercent) / 100;
	ioValue += increase;
}

// **** String Utils

// load a generic resource in a new block of memory. You must call FreeResource() when
// done with it. Size of the resource is returned in outResourceSize. Pass in NULL for
// out resource size if you don't care about the size.
void* 
LoadResource(long inResType, short inResID, long *outResSize) {
    void* p = NULL;
    int resSize = 0;
#if defined( POSIX_BUILD ) || ( !defined( PLATFORM_MACOS ) && !defined( PLATFORM_MACOSX ) )
    // generic implementation using .rez files, one per resource
    char filename[255];
    std::sprintf(filename, "data%cr%lx-%hx.rez", FILE_PATH_SEPARATOR, inResType, inResID);
    // open the file
    std::FILE* fp = std::fopen(filename, "rb");
    if (fp) {
        // figure out the length
        std::fseek(fp, 0, SEEK_END);
        resSize = std::ftell(fp);
        // allocate a buffer that big
        p = std::malloc(resSize);
        if (p) {
            // read the file contents into the buffer
            std::fseek(fp, 0, SEEK_SET);
            if (resSize != (int)std::fread(p, 1, resSize, fp)) {
                std::free(p);
                p = NULL;
            }
        }
        std::fclose(fp);
        fp = NULL;
    }
#else
    // MacOS implementation using Mac Resource Manager
    Handle h = MacAPI::GetResource(inResType, inResID);
    if (h) {
        resSize = MacAPI::GetHandleSize(h);
        p = std::malloc(resSize);
        if (p) {
            MacAPI::HLock(h);
            std::memcpy(p, *h, resSize);
            MacAPI::HUnlock(h);
        }
        MacAPI::ReleaseResource(h);
    }
#endif // PLATFORM_MACOS
    if (outResSize) {
        *outResSize = resSize;
    }
    return p;
}

void 
UnloadResource(void* inResPtr) {
    std::free(inResPtr);
}

// if the resource doesn't exist inStr will be unchanged
std::string&    
LoadStringResource(std::string& inStr, int inStringNum, int inSubStringNum) {
  #if ( defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ) ) && !defined ( POSIX_BUILD )
	char s[256];
	if (inSubStringNum > 0) {
    	Str255 theTempString;
	    MacAPI::GetIndString(theTempString, inStringNum, inSubStringNum);
    	p2cstrcpy(s, theTempString);
        inStr.assign(s);
	} else {
	    StringHandle h = MacAPI::GetString(inStringNum);
	    if (h) {
	    	p2cstrcpy(s, *h);
        	inStr.assign(s);
	    }
	}
  #else
    if (inSubStringNum > 0) {
        char* p = (char*)LoadResource('STR#', inStringNum); // read in Pascal String List
        if (p) {
          #if defined( PLATFORM_MACOSX ) && defined( PLATFORM_X86 )
            // OSX on Intel has already byte swapped this for us, so it's actually little endian
            int numStrs = p[1]*256 + p[0];  // first two bytes have little endian short integer string count
          #else
	        int numStrs = p[0]*256 + p[1];  // first two bytes have big endian short integer string count
          #endif
	        if (inSubStringNum > numStrs) {
	            inStr.assign("");   // out of range
	        } else {
	            p += sizeof(short);   // skip over string count to first Pascal string in list
	            for (int i = 1; i <= numStrs; i++) {
	                int strlen = *p;
	                if (i == inSubStringNum) {        // this is the one we want
	                    inStr.assign(&p[1], strlen);  // assign from pointer the number of bytes given by length byte
	                    return inStr;                 // pull an early exit now that we have our data
	                } else {
	                    p += (strlen + 1);    // skip to next Pascal string
	                }
	            }
	            inStr.assign("");   // didn't find it, must be a damaged resource
	        }
        }
    } else {
        char* p = (char*)LoadResource('STR ', inStringNum); // read in a Pascal String
        if (p) {
	        int strlen = p[0];
	        inStr.assign(&p[1], strlen);  // assign from pointer the number of bytes given by length byte
        }
    }
  #endif
    DEBUG_OUT("LoadStringResource "<<(void*)((inSubStringNum)?'STR#':'STR ')<<"-"<<(void*)inStringNum
                <<" = \""<<inStr.c_str()<<"\"", DEBUG_DETAIL);
    return inStr;
}

int
GetNumSubStringsInStringResource(int inStringNum) {
    int numStrs = 0;
    char* p = (char*) LoadResource('STR#', inStringNum);	// get num of substring in string list resource
    if (p) {
      #if defined( PLATFORM_MACOSX ) && defined( PLATFORM_X86 )
        // OSX on Intel has already byte swapped this for us, so it's actually little endian
        numStrs = p[1]*256 + p[0];  // first two bytes have little endian short integer string count
      #else
        numStrs = p[0]*256 + p[1];  // first two bytes have big endian short integer string count
      #endif
        UnloadResource(p);
    }
    return numStrs;
}

// some compilers/OS's declare std::toupper inline, which makes it impossible
// to use them in a call to std::transform
static char dotolower(char c);
static char dotoupper(char c);
char dotolower(char c) {
    return std::tolower(c);
}
char dotoupper(char c) {
    return std::toupper(c);
}

std::string&    
Uppercase(std::string& inStr) {
    std::transform( inStr.begin(), inStr.end(), inStr.begin(), dotoupper);
    return inStr;
}

std::string&    
Lowercase(std::string& inStr) {
    std::transform( inStr.begin(), inStr.end(), inStr.begin(), dotolower);
    return inStr;
}

std::string&    
PadLeft(std::string& inStr, int inToLength, char inPadChar) {
	int need = inToLength - inStr.size();
	if (need > 0) {
	    unsigned int pos = 0;
	    inStr.insert(pos, need, inPadChar);
	}
	return inStr;
}

std::string&    
PadRight(std::string& inStr, int inToLength, char inPadChar) {
	int need = inToLength - inStr.size();
	if (need > 0) {
	    inStr.append(need, inPadChar);
	}
	return inStr;
}

/*
LStr255&
Uppercase(LStr255 &inStr) {
	StringPtr p = inStr;
	for (int i = 1; i <= p[0]; i++) {
		std::toupper(p[i]);
	}
	return inStr;
}

LStr255& 
PadLeft(LStr255 &inString, int inToLength, char inPadChar) {
	int need = inToLength - inString.Length();
	if (need > 0) {
		for (int i = 0; i < need; i++)
			inString.Insert(&inPadChar, 1, 1);
	}
	return inString;
}

LStr255& 
PadRight(LStr255 &inString, int inToLength, char inPadChar) {
	int need = inToLength - inString.Length();
	if (need > 0) {
		for (int i = 0; i < need; i++)
			inString.Append(inPadChar);
	}
	return inString;
}
*/

LStr255&
ReplaceIllegalCharacters(LStr255 &inString, const char* inIllegalChars, char inReplacementChar, bool restrictToAscii) {
    char c;
    UInt8 pos;
    while ( (c = *inIllegalChars++) != 0 ) {
     	do {
     		pos = inString.Find(c);	// replace illegal character with replacement char
     		if (pos > 0) {
     			inString.Replace(pos, 1, inReplacementChar);
     		}
     	} while (pos>0);
    }
    if (restrictToAscii) {
		StringPtr strP = inString;
        for (int i = 1; i <= inString.Length(); i++) {
            if ((unsigned char)strP[i] > 0x7f) {
                strP[i] = '.';
            }
        }
    }
    return inString;
}

char*
ReplaceIllegalCharacters(char* inString, const char* inIllegalChars, char inReplacementChar, bool restrictToAscii) {
    char c;
    char* p;
    while ( (c = *inIllegalChars++) != 0 ) {
     	do {
     		p = std::strchr(inString, c);	// find illegal character
     		if (p) {
     			*p = inReplacementChar; // replace it
     		}
     	} while (p);
    }
    if (restrictToAscii) {
        for (int i = 0; i < (int)std::strlen(inString); i++) {
            if ((unsigned char)inString[i] > 0x7f) {
                inString[i] = '.';
            }
        }
    }
    return inString;
}

// available formats are 0 - never show seconds, 1 - always show seconds, and 
// 2 or greater means start showing seconds at (inFormat-1) minutes, so a format of 6
// would show 0:05 for 00:05:00, but would then switch to 4:59 to show 00:04:59
// if inFormat is negative, it is treated as it's absolute value, but leading zeros are shown
// inFormat is restricted to ±60, so the only way to display seconds at the same time as hours
// is with mode 1
// returns true if seconds were included in the string, false if they weren't
bool
Secs2TimeStr(unsigned long inRawDTSecs, std::string& inStr, int inFormat) {
	bool leading = (inFormat < 0);
	int format = (inFormat < 0) ? -inFormat : inFormat;	// strip sign from format
	if (format == 1) {
		inStr = " 0:00:00";	// we are in always show second mode
	} else {
		inStr = " 0:00";	// other modes never have hours & seconds together
	}
	if (leading) {				// negative mode,
		inStr[1] = '0';			// include leading zero
	}
	UInt16 seconds = inRawDTSecs % 60;
	UInt16 minutes = (inRawDTSecs/60) % 60;
	UInt32 hours = (inRawDTSecs/3600) % 100;	// we won't handle time bigger than 99:59:59
	bool useSeconds;
    bool useHours;
    if (inFormat == 0) {
	    useSeconds = false;
	    useHours = true;
    } else if (inFormat == 1) {
	    useSeconds = true;
	    useHours = true;
	} else if (inFormat > 1) {
	    if ( (hours || (minutes>(inFormat-1))) ) {
    	    // if we are skipping seconds because we've more than abs(inFormat)-1 minutes left, then
    	    // we are showing hours
    	    useSeconds = false;
    	    useHours = true;
    	 } else {
    	    // we have switched to showing minutes and seconds only
    	    useSeconds = true;
    	    useHours = false;
    	 }
	}
	int pos = 1;
	char hi;
	if (useHours) {
		hi = hours / 10;
		inStr[pos] = hours % 10 + '0';
		if (hi || leading) {
			inStr[pos-1] = hi + '0';
	    }
	    pos += 3;
	}
	// we always draw minutes
	hi = minutes / 10;
	inStr[pos] = minutes % 10 + '0';
	if (hi || leading) {
		inStr[pos-1] = hi + '0';
    }
    pos += 3;
	if (useSeconds) {	// do we need to worry about seconds?
		inStr[pos-1] = seconds / 10 + '0';
		inStr[pos] = seconds % 10 + '0';
		return true;	// we are showing seconds
	} else {
		return false;	// we are not showing seconds
	}
}


// displays as an OSType

char gLongTo4CharStr_buffer[5];

char*
LongTo4CharStr(long n) {
	*((long*)&gLongTo4CharStr_buffer[0]) = Native_ToBigEndian32(n);
	gLongTo4CharStr_buffer[4] = 0;
	return &gLongTo4CharStr_buffer[0];
}


void BinaryDump(char *outBuf, int outBufSize, char *inBuf, int inBufSize) {
// do a nicely formatted binary dump of buf to string
#define BYTES_PER_LINE 20
    char hexbuf[3*BYTES_PER_LINE+1], ascbuf[BYTES_PER_LINE+1];
    unsigned char *p;
    int n, total;
    unsigned int ch;

    total = 0;
    p = (unsigned char *)inBuf;
    outBuf[0] = 0;  // start with a clean buffer

	const char truncMsg[21] = "<HEX DUMP TRUNCATED>";
	int offset = 0;

    while (total < inBufSize)
    {
        for (n = 0; total < inBufSize && n < BYTES_PER_LINE; total++, n++)
        {
            ch = *p++;
            std::sprintf(hexbuf + n*3, "%02x ", ch);
            ascbuf[n] = std::isprint(ch) ? ch : '.';
        }
        ascbuf[n] = '\0';

		if ((std::strlen(outBuf) + std::strlen(hexbuf) + std::strlen(ascbuf) + 5) > (unsigned)outBufSize) {
			if (std::strlen(outBuf) + std::strlen(truncMsg) > (unsigned)outBufSize) {
				offset = std::strlen(truncMsg);
			}
			std::sprintf(&outBuf[std::strlen(outBuf) - offset], "%s", truncMsg);
			return;
		}
		if( n < BYTES_PER_LINE )
		{
			for( ch = n; ch < BYTES_PER_LINE; ch++ ) {
				std::sprintf( hexbuf + ch*3, "   " );
			}
	        std::sprintf(&outBuf[std::strlen( outBuf )], "%s | %s", hexbuf, ascbuf);
		}
		else {
			std::sprintf(&outBuf[std::strlen( outBuf )], "%s | %s\n", hexbuf, ascbuf);
		}

    }
}


#include <ctime>

// this is only an approximation. There are some time zones which
// are on half hour intervals, and this won't detect those properly
int get_timezone() {
    std::time_t t = std::time(&t);
    std::tm local = *std::localtime(&t);
    std::tm gmt = *std::gmtime(&t);
    int zone = (local.tm_hour+24) - (gmt.tm_hour+24);
    if (local.tm_isdst) {
        zone -= 1;
    }
    return zone;
}



#ifdef INCLUDE_LARRAY_TOOLS

#include "DebugMacros.h"
#include <new>
#include <LArrayIterator.h>


// this is an abstract class for performing an action on every element of an array
// override the DoAction() method to perform the desired action, and return true if the
// item should be written back into the array. It is perfectly fine to delete the item
// from the array, duplicate it in the array, and otherwise do anything you want to it.
// Don't try to change the size of the item, though, or you'll overwrite the buffer that
// is used to hold the current item.
// call UArrayAction::DoForEachElement(array, action) to perform the action
void 
UArrayAction::DoForEachElement(LArray &inTargetArray, UArrayAction &inAction) {
	UInt32 itemSize = inTargetArray.GetItemSize(LArray::index_First);
	ASSERT(itemSize > 0);
	LArrayIterator iter(inTargetArray, LArrayIterator::from_Start);
	char* p = new (std::nothrow) char[itemSize];	// try to allocate a buffer to hold the item data
	if (p == NULL) {
		DEBUG_OUT("Couldn't allocate "<<itemSize<<" bytes for array processing.", DEBUG_ERROR);
		return;
	}
	while (iter.Next(p)) {	// read the next item into the data buffer, if there is a next item
		if (inAction.DoAction(inTargetArray, iter.GetCurrentIndex(), p)) {	// do the action
			// they returned true, so we should write the item back
			inTargetArray.AssignItemsAt(1, iter.GetCurrentIndex(), p);
		}
	}
	delete[] p;
}
#endif

