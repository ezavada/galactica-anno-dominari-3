//  InternetInfo.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include <string>

#include "InternetInfo.h"
#if TARGET_OS_MAC && !TARGET_API_MAC_CARBON	// IC only available on MacOS
  #include "ICAPI.h"
#elif TARGET_OS_MAC && TARGET_API_MAC_CARBON
  #include <InternetConfig.h>
#endif
#include <Files.h>
#include <Folders.h>
#include <AEInteraction.h>
#include <Sound.h>
#include "DebugMacros.h"

#if TARGET_API_MAC_CARBON
  #include <CFURL.h>
#endif

#define NETSCAPE_PREF_PATH	"\p:Netscape Ä:Netscape Preferences"	// relative path to netscape
#define NETSCAPE_CREATOR	'MOSS'	// netscape's creator code

OSErr OpenNetscapeURL(const char* inURL, long inLen);
void PathNameFromFSSpec(FSSpec &inFileSpec, StringPtr fullPathName);
void PathNameFromDirID(long dirID, short vRefNum, StringPtr fullPathName);
#if !TARGET_API_MAC_CARBON
void PathNameFromWD(long vRefNum, StringPtr pathName);
#endif
void pstrcat(StringPtr dst, StringPtr src);
void pstrinsert(StringPtr dst, StringPtr src);



int
HasWebBrowser(void) {
	return (HasNetscape() || HasOtherBrowser());
}

OSErr 
OpenHttpWithBrowser(std::string inURL, bool inDoSecure) {
    if (HasOtherBrowser()) {
		return OpenHttpWithOtherBrowser(inURL, inDoSecure);
	} else if (HasNetscape()) {
		return OpenHttpWithNetscape(inURL, inDoSecure);
	} else {
		SysBeep(1);
		return dsBadLaunch;
	}
}

OSErr
OpenFileWithBrowser(FSSpec &inFileSpec) {
    char temp[256];
  #if TARGET_API_MAC_CARBON
    long response = 0;
    Gestalt(gestaltSystemVersion, &response);
    if (response >= 0x00001000) {
        // under OS X, use the new FSRefMakePath
        FSRef fileRef;
        if (FSpMakeFSRef(&inFileSpec, &fileRef) == noErr) {
           CFURLRef    tempURL;
           tempURL = ::CFURLCreateFromFSRef( NULL, &fileRef );
           CFStringRef outString = ::CFURLGetString( tempURL );
           CFStringGetCString (outString, temp, 255, kCFStringEncodingMacRoman);
        }
    }
    else
  #endif
    {
      Str255 s1;
    	PathNameFromFSSpec(inFileSpec, s1);
        Str255 fileUrlStr = "\pfile:///";
    	pstrinsert(s1, fileUrlStr);
    	pstrcat(s1, inFileSpec.name);
    	p2cstrcpy(temp, s1);
	}
   DEBUG_OUT("requesting open of URL <" << temp <<">", DEBUG_IMPORTANT);
   std::string s = temp;
   return OpenHttpWithBrowser(s, false);
}

// works by simple expedient of checking to see if there is a Netscape Preferences file
// in the System Folder
int
HasNetscape(void) {
	FSSpec prefs;
	long prefsFldrID;
	short sysVol;
	FindFolder(kOnSystemDisk, kPreferencesFolderType, false, &sysVol, &prefsFldrID);
	return (FSMakeFSSpec(sysVol, prefsFldrID, NETSCAPE_PREF_PATH, &prefs) == noErr);
}

// presumes that if Internet Config is installed, the user has an Internet Browser.
// Since MSIE installs IC, this isn't a bad assumption, and IC will give an appropriate
// error if browser launch fails
int
HasOtherBrowser(void) {
  #if TARGET_OS_MAC	// IC only available on MacOS
	ICInstance inst;
	OSStatus ierr = ICStart(&inst, '????');	// startup internet config
	if (ierr == noErr) {
		ICStop(inst);
	}
	return (ierr == noErr);
  #else
  	return false;
  #endif
}



// must be string in the form of "http://address.address/etc.."
OSErr
OpenHttpWithNetscape(std::string inURL, bool inDoSecure) {
	if (inDoSecure) {
	   inURL.replace(0, 4, "https");
	}
	return OpenNetscapeURL(inURL.c_str(), inURL.length());
}

// must be string in the form of "http://address.address/etc.."
OSErr
OpenHttpWithOtherBrowser(std::string inURL, bool inDoSecure) {
	if (inDoSecure) {
	   inURL.replace(0, 4, "https");
	}
  #if TARGET_OS_MAC	// IC only available on MacOS
	ICInstance inst;
	long selStart, selEnd;
	OSStatus ierr = ICStart(&inst, '????');	// startup internet config
	if (ierr == noErr) {
      #if TARGET_API_MAC_CARBON
        ierr = noErr;
      #else
		  ierr = ICFindConfigFile(inst, 0, nil);	// find the internet config file
      #endif
		if (ierr == noErr) {
			selStart = 0;
			selEnd = inURL.length();
			ierr = ICLaunchURL(inst, "\p", inURL.c_str(), inURL.length(), &selStart, &selEnd);
		}
	}
	ICStop(inst);
	return ierr;
  #else
  	return unimpErr;	// not implemented on target OS
  #endif
}


// send Open URL Apple Event to Netscape
OSErr 
OpenNetscapeURL(const char* inURL, long inLen) {
	AppleEvent	openEvent;	// the event to create
	AppleEvent	reply = {typeNull, nil};
	AEDesc	dst;	// descriptor for the target app 
	OSErr	err;
	ProcessSerialNumber psn;

	if (!LaunchNetscape(&psn)) return dsBadLaunch;	// try to launch Netscape if necessary
	err = AECreateDesc(typeProcessSerialNumber, &psn, sizeof(psn), &dst);
	if (err) return err;
	// Create an empty open apple event
	err = AECreateAppleEvent('GURL', 'GURL', &dst, kAutoGenerateReturnID, kAnyTransactionID, &openEvent);
	if (err) return err;
	// then add the document to be opened as a parameter to the Open Document event
	err = AEPutParamPtr(&openEvent, keyDirectObject, typeChar, inURL, inLen);
	if (err) return err;
	// send the event, and don't wait for a reply
	err = AESend(&openEvent, &reply, kAENoReply, kAENormalPriority, kAEDefaultTimeout, nil, nil);
	if (err) return err;
	AEDisposeDesc(&openEvent);
	return noErr;
}

// returns false if not running
int
GetNetscapePSN(ProcessSerialNumber* outPSN) {
	Boolean	foundRunning = false;
	// check the current processes to see if the creator app is already
	// running, and get its process serial number
	ProcessInfoRec currProcessInfo;
	currProcessInfo.processInfoLength = sizeof(ProcessInfoRec);
	currProcessInfo.processName = nil;
	currProcessInfo.processAppSpec = nil;
	
	foundRunning = false;
	outPSN->lowLongOfPSN = outPSN->highLongOfPSN = kNoProcess;
	while (GetNextProcess(outPSN) == noErr) {
		if (GetProcessInformation(outPSN, &currProcessInfo) == noErr) {
			if (currProcessInfo.processSignature == NETSCAPE_CREATOR) {
				foundRunning = true;
				break;
			}
		}
	}
	return foundRunning;
}

int
LaunchNetscape(ProcessSerialNumber* outPSN) {
	OSErr err;
	Boolean running = GetNetscapePSN(outPSN);
	if (running)
		SetFrontProcess(outPSN);
	else {		// Netscape not running, find it on disk and launch it
		HParamBlockRec 	volPB;
		HVolumeParamPtr	pb = &volPB.volumeParam;
		FSSpec browserSpec;
		browserSpec.vRefNum = 0;
		pb->ioCompletion = nil;
//		Str31  volumeName;		// debugging
		pb->ioNamePtr = nil;//volumeName;	
		short  ioVolIndex = 1;
		pb->ioVRefNum = 0;
		pb->ioVolIndex = ioVolIndex;
		while (PBHGetVInfoSync(&volPB) == noErr) {	// loop over all mounted volumes
			if ( (pb->ioVDrvInfo > 0) && (pb->ioVDRefNum < 0) ) { // volume is online
				DTPBRec	dtppb;
				dtppb.ioCompletion = nil;
				dtppb.ioNamePtr = nil;
				dtppb.ioVRefNum = pb->ioVRefNum;
				err = PBDTGetPath(&dtppb);	// fills in dtppb.ioDTRefNum
				if (err)
					goto FAIL;
				dtppb.ioFileCreator = NETSCAPE_CREATOR;
				dtppb.ioNamePtr = browserSpec.name;
				dtppb.ioIndex = 0;  // most recent
				err = PBDTGetAPPLSync(&dtppb);	// find the browser
				if (err) {
					if (err != afpItemNotFound)	// not found error: check next volume
						goto FAIL;
				} else {
					browserSpec.vRefNum = pb->ioVRefNum;
					browserSpec.parID = dtppb.ioAPPLParID;

					LaunchParamBlockRec	launchParams;
					launchParams.launchAppParameters = nil;
					launchParams.launchBlockID		= extendedBlock;
					launchParams.launchEPBLength	= extendedBlockLen;
					launchParams.launchFileFlags	= nil;
					launchParams.launchControlFlags	= launchContinue + launchNoFileFlags;
					launchParams.launchAppSpec	= &browserSpec;
					err = ::LaunchApplication(&launchParams);

					if (err)
						goto FAIL;
//						long delay;
//						::Delay(120, &delay);
					running = GetNetscapePSN(outPSN);
//						if (running)
//							SetFrontProcess(outPSN);
					break;
				}
			}
			ioVolIndex++;
			pb->ioVRefNum = 0;
			pb->ioVolIndex = ioVolIndex;
		}
	}
FAIL:
	return running;
}




// Utilities from the Apple Q&A Stack showing how to get the full
// pathname of a file.  Note that this is NOT the recommended way
// to specify a file to Toolbox routines.  These routines should be
// used for displaying full pathnames only.


/*
 * Pascal string utilities
 */
/*
 * pstrcat - add string 'src' to end of string 'dst'
 */
void pstrcat(StringPtr dst, StringPtr src)
{
	/* copy string in */
	BlockMoveData(src + 1, dst + *dst + 1, *src);
	/* adjust length byte */
	*dst += *src;
}

/*
 * pstrinsert - insert string 'src' at beginning of string 'dst'
 */
void pstrinsert(StringPtr dst, StringPtr src)
{
	/* make room for new string */
	BlockMoveData(dst + 1, dst + *src + 1, *dst);
	/* copy new string in */
	BlockMoveData(src + 1, dst + 1, *src);
	/* adjust length byte */
	*dst += *src;
}

void PathNameFromFSSpec(FSSpec &inFileSpec, StringPtr fullPathName) {
	PathNameFromDirID(inFileSpec.parID, inFileSpec.vRefNum, fullPathName);
}

void PathNameFromDirID(long dirID, short vRefNum, StringPtr fullPathName)
{
	CInfoPBRec	block;
	Str255		directoryName;
	OSErr		err;

	fullPathName[0] = '\0';

	block.dirInfo.ioDrParID = dirID;
	block.dirInfo.ioNamePtr = directoryName;
	do {
			block.dirInfo.ioVRefNum = vRefNum;
			block.dirInfo.ioFDirIndex = -1;
			block.dirInfo.ioDrDirID = block.dirInfo.ioDrParID;
			err = PBGetCatInfoSync(&block);
            Str255 slash = "\p";
			pstrcat(directoryName, slash);
			pstrinsert(fullPathName, directoryName);
	} while (block.dirInfo.ioDrDirID != 2);
}

#if !TARGET_API_MAC_CARBON
/*
PathNameFromWD:
Given an HFS working directory, this routine returns the full pathname that
corresponds to it. It does this by calling PBGetWDInfo to get the VRefNum and
DirID of the real directory. It then calls PathNameFromDirID, and returns its
result.

*/
void PathNameFromWD(long vRefNum, StringPtr pathName)
{
	WDPBRec	myBlock;
	OSErr	err;

	myBlock.ioNamePtr = nil;
	myBlock.ioVRefNum = vRefNum;
	myBlock.ioWDIndex = 0;
	myBlock.ioWDProcID = 0;
/*
Change the Working Directory number in vRefnum into a real
vRefnum and DirID. The real vRefnum is returned in ioVRefnum,
and the real DirID is returned in ioWDDirID.
*/
	err = ::PBGetWDInfoSync(&myBlock);
	if (err != noErr)
			return;
	PathNameFromDirID(myBlock.ioWDDirID, myBlock.ioWDVRefNum, pathName);
}
#endif