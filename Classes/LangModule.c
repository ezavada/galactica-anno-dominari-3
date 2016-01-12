/*--------------------------------------------------------------------
// LanguageModule.c
//
// Routines to simplify internationalization of MacOS programs
//
// copyright й 1996-97, Edmund R. Zavada <ezavada@kagi.com>
// All Rights Reserved World Wide
//
// This code may be freely used in any MacOS program, and just as freely
// distributed. You may modify this code for use in your projects in any 
// way you see fit, but you may not redistribute the modified code.
//
// While this is freeware, I would appreciate a note at <ezavada@kagi.com>
// if you are using it. I'll be equally glad to receive any comments,
// criticism, and/or suggestions you may have. Finally, I'd appreciate a
// copy of the final product if that's possible.
//
// If the author every ceases to support this code, it will
// become public domain.
//
// Version 1.0.1
//
// modified: 1/4/97, ERZ, v1.0.1, Added LMdGetNumModules()
//--------------------------------------------------------------------*/


#include "LangModule.h"

/* =========================================================================================== */
/* These prototypes have been moved out of the header file to indicate that they are PRIVATE.
	If you are using the Shared Library version of these code, DON'T CALL THESE. If you do, your
	application will break as soon as I release a new Shared Library. Of course, if you are only
	using the 68k version, no problem.
 BE WARNED! While it is possible to simply include this code directly in a PowerPC program, I 
    strongly recommend against it. Please use the shared library for your PowerPC projects. Why? 
    I've had to make use of some structures and low memory variables to make this work. While
    I've made every effort to do things in the way Apple suggests for maximum compatiblity, there
    is no guarantee that this version of the LanguageModulesLib will work with Copland. But since
    it's a shared library, I can make a version that does work available, your customers can
	install it, and your application will work fine again.
/* =========================================================================================== */

/* reentrable low level versions of the high level routines. The high level routines call
	these with a global data block, so the high level routines are simpler, but not
	reentrable/threadable */

typedef struct LMdDataT {
	Boolean					installed;	/* false if there was a critical failure */
	MenuHandle				langMenu;
	Handle					menuBarH;	/* used to rebuild the menus */
	LMdInfoListPtr			modules;
	ResListPtr				currResList;
	RedrawProcPtr			redrawProcP;
	ModulePreOpenProcPtr	preOpenProcP;
	ModuleOpenProcPtr		openProcP;
	ModuleCloseProcPtr		closeProcP;
	ResExcludeProcPtr		excludeProcP;
	ResAdjustProcPtr		adjustProcP;
	FSSpec					moduleFolderSpec;
	unsigned long			lastFolderChange;	/* secs */
	long					refCon;		/* for user data */
	long					reserved;	/* for future expansion */
} LMdDataT;

void	LMd_BuildResourceList(short inRefNum, ResListPtr *outResList, ResExcludeProcPtr inExcludeProcP);
				/* builds list of the resources in a module/resource file */
void	LMd_PurgeOldResources(ResListPtr inResList);
				/* purges all the resources that were in the old module */

#if !TARGET_API_MAC_CARBON
void	LMd_LoadNewResources(ResListPtr *inCurrList, ResListPtr inNewList, ResAdjustProcPtr inAdjustProcP);
				/* installs the new resources in place of the old ones */
#endif

void 	LMd_Initialize(LMdDataT *inDataP, short smLangCode); 
void 	LMd_Cleanup(LMdDataT *inDataP);
void	LMd_HandleMenuSelect(LMdDataT *inDataP, short inMenuItemNum);
void	LMd_CheckForNewModules(LMdDataT *inDataP);	
short	LMd_GetCurrentModuleInfo(LMdDataT *inDataP, LangModuleInfoT *outLMdInfo);	
void	LMd_GetModuleInfo(LMdDataT *inDataP, short smLangCode, LangModuleInfoT *outLMdInfo);	
void	LMd_GetIndModuleInfo(LMdDataT *inDataP, short inIndex, LangModuleInfoT *outLMdInfo);


/* other low level routines, that you should never need to call for anything to do with the
	Language Modules, but that may be of use somewhere else */

void	LMd_InitDataStructs(LMdDataT *inDataP);
void 	LMd_FindModuleFolder(FSSpec *outModuleFolderSpec);
short  	LMd_HasFolderChangedSince(FSSpec *ioModuleFolderSpec, unsigned long *ioDateTime);
void 	LMd_BuildModuleList(const FSSpec *inModuleFolderSpec, LMdInfoListPtr *outList);
void	LMd_BuildMenuFromList(const LMdInfoListT* inList, MenuHandle *ioMenuH);
short	LMd_FindModuleInListByLangCode(LMdInfoListT *inList, short smLangCode);
void	LMd_OpenModule(LangModuleInfoT *inLMdInfo);
void	LMd_CloseModule(LangModuleInfoT *inLMdInfo);
long	LMd_FindDirForFileType(CInfoPBRec* cipbrp, short vRefNum, long parID, OSType fileType);
long	LMd_CountFilesOfTypeInDir(const FSSpec *inFolderSpec, OSType fileType);
long	LMd_GetDirIDFromFolderFSSpec(const FSSpec *inSpec);
short	LMd_GetLangFromCountryCode(short inCountryCode);
void	LMd_UnrecoverableError(Str255 line1, Str255 line2, short errCode);
void	LMd_AdjustWindowRect(Rect *ioRect);
short	LMd_ModulesAreSame(LangModuleInfoT *inLMdA, LangModuleInfoT *inLMdB);


/* =========================================================================================== */
/* END PRIVATE STUCTURES */
/* =========================================================================================== */



#ifndef __QUICKDRAW__
  #include <Quickdraw.h>
#endif
#ifndef __MACWINDOWS__
  #include <MacWindows.h>
#endif
#ifndef __LOWMEM__
  #include <LowMem.h>
#endif
#ifndef __SCRIPT__
  #include <Script.h>
#endif
#ifndef __TEXTUTILS__
  #include <TextUtils.h>
#endif
#ifndef __RESOURCES__
  #include <Resources.h>
#endif

/* =========================================================================================== */
/* MORE PRIVATE STUCTURES THAT WILL NEVER BE MADE PUBLIC */
/* =========================================================================================== */

typedef struct RsrcMapEntryT {
	short	id;
	short	nameListOffset;
	char	attribs;
	char	dataOffset[3];	/* must & with 0x00ffffff to strip attribs */
	Handle	resH;
} RsrcMapEntryT, *RsrcMapEntryPtr;

LMdDataT gLMd_Data;
Boolean		gLMd_ExitOnFail = true;	/* should the app exit on failure, or keep going */
Boolean		gLMd_ShowFail = true;	/* display error messages on failure? */
Boolean		gLMd_ShowInitialSplash = true;	/* display splash for first module loaded? */
short		gLMd_MenuID = 1;		/* id number to use for the language menu */
OSErr		gLMd_LastError = noErr;	/* used by LMdError() */

/* =========================================================================================== */
/* END MORE PRIVATE STUCTURES */
/* =========================================================================================== */

/* returns a version number in case your code depends on features
   in a certain version of a LanguageModules Shared Library*/
pascal short 
LMdGetVersion(void) {
	return 1;
}

/* returns the last error that caused a failure in a LangModule routine, you don't
   need to check this unless you have set gExitOnFail to false */
pascal short 
LMdError(void) {
	return gLMd_LastError;
}

/* A few access functions for globals you can set before calling LMdInitialize */

/* do we exit app on failure? defaults to true */
/* you can change this before or after initialization */
pascal void
LMd_SetExitOnFail(Boolean inExitOnFail) {
	gLMd_ExitOnFail = inExitOnFail;
}
pascal Boolean
LMd_GetExitOnFail(void) {
	return gLMd_ExitOnFail;
}

/* do we show msg on failure? defaults to true */
/* you can change this before or after initialization */
pascal void		LMd_SetShowFail(Boolean inShowFail) {
	gLMd_ShowFail = inShowFail;
}
pascal Boolean	LMd_GetShowFail(void) {
	return gLMd_ShowFail;
}

/* do we display splash for first module loaded? defaults to true */
/* changing this after initialization has absolutely no effect (except changing the value :-) */
pascal void		LMd_SetShowInitialSplash(Boolean inShowSplash) {
	gLMd_ShowInitialSplash = inShowSplash;
}
pascal Boolean	LMd_GetShowInitialSplash(void) {
	return gLMd_ShowInitialSplash;
}

/* id num of language MENU res, defaults to 1 */
/* this MENU res can be in the modules if you want to use a translated text title */
/* changing this after initialization might cause problems, better not to */
pascal void		LMd_SetLangMenuID(short inMenuID) {
	gLMd_MenuID = inMenuID;
}
pascal short	LMd_GetLangMenuID(void) {
	return gLMd_MenuID;
}


#pragma mark-
/* =========================================================================================== */
/* the following high level calls are for general use, but because they use global variables,
	they aren't suitable to be multithreaded or otherwise called rentrantly. Use the corresponding
	low level calls that accept a data block if you want to do manage multipule sets of modules. 
	Some of these have no low level equivalent because you can get the info directly from the
	data block. */
/* =========================================================================================== */

/* call once at the beginning of your program pass in code
   for startup module, or -1 to use OS's current code */
pascal void 
LMdInitialize(short smLangCode) {
	LMd_Initialize(&gLMd_Data, smLangCode);	/* just call the reentrable low level version */
}


/* call once at the end of your program, after any possible use of resources from the modules */
pascal void 
LMdCleanup(void) {
	LMd_Cleanup(&gLMd_Data); /* just call the reentrable low level version */
}


/* returns a reference to the current language menu that lists the
	modules that are available, install this in a popup or in the
	menu bar	*/
pascal MenuHandle
LMdGetLanguageMenu(void) {
	return gLMd_Data.langMenu;
}

/* does all the work needed to figure out which module to switch to,
	dump the old module, load the new one, and refresh the windows and
	menu bar */
pascal void 
LMdHandleMenuSelect(short inMenuItemNum) {
	LMd_HandleMenuSelect(&gLMd_Data, inMenuItemNum);	/* call the reentrable low level version */
}

/* call when your app switched to front. Caches folder info and checks
	the folder modification date first. Only rebuilds menu if folder changed 
	Even better is to check when a click is detected in the menu, so the menu
	is always up to date */
pascal void 
LMdCheckForNewModules(void) {
	LMd_CheckForNewModules(&gLMd_Data);	/* call the reentrable low level version */
}

/* returns current language code, pass in nil if you don't want the full info
	you'll probably use this to find out what module was last used when
	you update your prefs file as you quit (or whenever) */
pascal short 
LMdGetCurrentModuleInfo(LangModuleInfoT *outLMdInfo) {
	return LMd_GetCurrentModuleInfo(&gLMd_Data, outLMdInfo); /* call low level version */
}

/* returns current info on a module specified by it's language code
   returns -1 in both countryCode and smLangCode if not found.
   you probably don't really need to use this unless you want to 
   display the name of the current module file or something	*/
pascal void 
LMdGetModuleInfo(short smLangCode, LangModuleInfoT *outLMdInfo) {
	LMd_GetModuleInfo(&gLMd_Data, smLangCode, outLMdInfo); /* call low level version */
}

/* returns current info on a module specified by it's position in the list
   you probably don't really need to use this unless you want to 
   display the names of the current module files available or something	*/
pascal void 
LMdGetIndModuleInfo(short inIndex, LangModuleInfoT *outLMdInfo) {
	LMd_GetIndModuleInfo(&gLMd_Data, inIndex, outLMdInfo); /* call low level version */
}

pascal short
LMdGetNumModules(void) {
	return gLMd_Data.modules->count; /* call low level version */
}

pascal void 
LMdMakeCurrentModuleTop(void) {
	short currModuleNum = gLMd_Data.modules->current;
	LangModuleInfoT* curr = &(gLMd_Data.modules->entry[currModuleNum]);
	UseResFile(curr->moduleFileRefNum);
}

#pragma mark-

/* High level routines, for those who want a bit of customized behavior */


/* call if you want to replace the standard refresh method used when
	modules are switched, which is: go through the window manager and
	send refresh message to all windows. */
pascal void 
LMdInstallRedrawProc(RedrawProcPtr inRedrawProcP) {
	gLMd_Data.redrawProcP = inRedrawProcP;
}
				
/* install to do something when a module is first opened, the proc is called
	after the close of the old module, with the file open, but before anything
	is actually done with the module */
pascal void 
LMdInstallModulePreOpenProc(ModulePreOpenProcPtr inModulePreOpenProcP) {
	gLMd_Data.preOpenProcP = inModulePreOpenProcP;
}
				
/* install to do something when a module is first opened, the proc is called
	after the close of the old module, and call of PreOpenProc & RedrawProc
	and display of module splash screen. */
pascal void 
LMdInstallModuleOpenProc(ModuleOpenProcPtr inModuleOpenProcP) {
	gLMd_Data.openProcP = inModuleOpenProcP;
}
				
/* install to do something when a module is closed, called before file has
	actually been closed */
pascal void 
LMdInstallModuleCloseProc(ModuleCloseProcPtr inModuleCloseProcP) {
	gLMd_Data.closeProcP = inModuleCloseProcP;
}

pascal void
LMdInstallResExcludeProc(ResExcludeProcPtr inResExcludeProcP) {
	gLMd_Data.excludeProcP = inResExcludeProcP;
}

pascal void
LMdInstallResAdjustProc(ResAdjustProcPtr inResAdjustProcP) {
	gLMd_Data.adjustProcP = inResAdjustProcP;
}

#pragma mark-
/* Low level routines, which provide support for the high level routines above
// You shouldn't need to call these routines except from within custom procs	*/

/* this is what is defined as the redraw proc by default */
pascal void 
LMd_StandardRedraw(void) {
	GrafPtr savePort;
/*	RgnHandle greyRgn;
	WindowPtr topWindow;*/
	GetPort(&savePort);					/* save the old port */
/*	greyRgn = GetGrayRgn();				/* get the region of the entire screen */
/*	topWindow = FrontWindow();			/* now get the frontmost window */
/*	PaintBehind(topWindow, greyRgn);	/* invalidate the front window and all behind it. */
	LMd_RedrawMenuBar();				/* now redraw the menu bar */
	MacSetPort(savePort);				/* restore the old port */
}

/* this is what is defined as the standard module open proc by default.
	It basically just calls LMd_ShowModuleSplash */
pascal void 
LMd_StandardOpen(const LangModuleInfoT *inLMdInfo) {
	#pragma unused (inLMdInfo)
	LMd_ShowModuleSplash();
}

/* builds list of the resources in a module */
/* ignores these resources: 
	custom file icon:
		icl4 -16455
		icl8 -16455
		iCN# -16455
		ics# -16455
		ics4 -16455
		ics8 -16455
	product and vendor info strings
		STR# 1	(product info)
		STR# 2	(vendor info)
	file version info strings
		vers 1	(appears in GetInfo)
		vers 2	(appears in GetInfo)
	finder help info resources
		hfdr -5696
		TEXT 1	(text that the hfdr should use to display the finder help balloon)
		styl 1	(style info for TEXT 1)
	module splash screen
		PICT 1
	additonal module info
		LMod 1	(reserved)
		LMod 2	(for client use, format undefined)
	all MBAR resources
*/

typedef struct ResInfoT {
	ResType	type;
	short	id;
} ResInfoT, *ResInfoPtr;

#define EXCLUDED_RES_COUNT	16

ResInfoT gLMd_ResExcludeList[EXCLUDED_RES_COUNT] = {	/* list is in ASCII sorted order to speed checking */
		{'ICN#', -16455},
		{'LMod', 1}, 
		{'LMod', 2}, 
		{'PICT', 1},
		{'STR#', 1},
		{'STR#', 2},
		{'TEXT', 1},
		{'hfdr', -5696},
		{'icl4', -16455},
		{'icl8', -16455},
		{'ics#', -16455},
		{'ics4', -16455},
		{'ics8', -16455},
		{'styl', 1},
		{'vers', 1},
		{'vers', 2}
};

/* this is the standard resExcludeProc, installed by default
	and called by BuildResourceList(). It keeps certain resources
	that are internal to the modules from being added to the
	resource list. If you create your own resExcludeProc, you
	should probably call this one from within it. */
pascal Boolean
LMd_StandardResExclude(ResType inType, short inID) {
	short i;
	for (i = 0; i < EXCLUDED_RES_COUNT; i++) {
		if (inType == 'MBAR') {
			return true;		/* exclude all MBAR resources */
		}
		if (gLMd_ResExcludeList[i].type == inType) {	/* if types are equal, check ids */
			if (gLMd_ResExcludeList[i].id == inID) {	/* if the ids are equal too, it is excluded */
				return true;
			} else if (gLMd_ResExcludeList[i].id > inID) {	/* if we passed where the id would */
				return false;								/* be, it isn't excluded, quit looking */
			}
		} else if (gLMd_ResExcludeList[i].type > inType) {	/* if we have passed where the type */
			return false;									/* would be, it isn't excluded */
		}
	}
	return false;	/* reached the end of the list and didn't find it, it's not excluded */
}


/* this is the standard resAdjustProc, installed by default and called by 
	LMd_LoadNewResources(). It forces menus to recalculate their size so they 
	will draw correctly. If you create your own resAdjustProc, you should 
	probably call this one from within it. */
pascal void
LMd_StandardResAdjust(ResListEntryT *inResInfo, long inResSize) {
	#pragma unused (inResSize)
	MenuHandle menuH = (MenuHandle)inResInfo->memH;
	if (inResInfo->type == 'MENU') {
	  #if TARGET_API_MAC_CARBON
		bool isAppleMenu = false;
	  #else
		(**menuH).menuProc = inResInfo->menuProc;
		(**menuH).enableFlags = inResInfo->menuEnableFlags;
		bool isAppleMenu = (**menuH).menuData[1] == '\024';
	  #endif
		if (isAppleMenu) {	/* is this the Apple Menu? */
			AppendResMenu(menuH, 'DRVR');	/* rebuild list of Apple Menu items */
		} else {
			MacInsertMenuItem(menuH, "\pJunk", 0);	/* add an item to the menu */
			DeleteMenuItem(menuH, 1);	/* then delete it again, this forces size recalc */
		}
	}
}

#pragma mark-

void 
LMd_BuildResourceList(short inRefNum, ResListPtr *outResList, ResExcludeProcPtr inExcludeProcP) {
	short	origRef, tCnt, rCnt, i, ii, rID;
	ResType	rType;
	Handle	rH;
	Str255	rName;
	long	resCount = 0, idx = 0;
	Boolean	realloc = false, excluded;
	origRef = CurResFile();	/* save original res file ref for later restoration */
	UseResFile(inRefNum);
	tCnt = Count1Types();
	for (i = 1; i <= tCnt; i++) {
		Get1IndType(&rType, i);
		rCnt = Count1Resources(rType);
		resCount += rCnt;
	}
	if (*outResList == nil) {	/* if the ResList hasn't yet been allocated, we need to do that now */
		realloc = true;
	} else if ((**outResList).allocated < resCount) { /* reallocate if the ResList is too small */
		realloc = true;
	}
	if (realloc) {
		*outResList = (ResListT*)NewPtr(sizeof(ResListT) + resCount * sizeof(ResListEntryT) );
		if (*outResList == nil) {
			LMd_UnrecoverableError("\pOut Of Memory","\pCannot create resource list.", MemError());
			return;
		}
		(**outResList).allocated = resCount;	/* record number of allocated slots */
	}
	SetResLoad(false);	/* since we will never exit the program from within this loop, this is ok */
	for (i = 1; i <= tCnt; i++) {
		Get1IndType(&rType, i);
		rCnt = Count1Resources(rType);
		for (ii = 1; ii <= rCnt; ii++) {
			rH = Get1IndResource(rType, ii);
			GetResInfo(rH, &rID, &rType, rName);	 /* get the type and id of the resource */
			if (inExcludeProcP == nil) {	/* no exclude proc available, include the resource */
				excluded = false;
			} else {
				excluded = inExcludeProcP(rType, rID);	/* do we need to exclude this resource? */
			}
			if (!excluded) { /* if it's not excluded, add it to the list */
				(**outResList).entry[idx].type = rType;
				(**outResList).entry[idx].id = rID;
				(**outResList).entry[idx].memH = nil;
				idx++;	/* we've now got one more resource to keep track of */
			}
			ReleaseResource(rH);	/* we aren't going to do anything more with it here */
		}
	}
	SetResLoad(true);			/* now the OS and the rest of the app can continue to function */
	(**outResList).count = idx;	/* record the actual number of resources we added */
	UseResFile(origRef);		/* restore our old res file */
}

/* purges all the resources that were in the old module */
void 
LMd_PurgeOldResources(ResListPtr inResList) {
	int i;
	ResType rType;
	short	rID;
	Handle	rH;
	
	SetResLoad(false);
	for (i = 0; i < inResList->count; i++) {
		rType = inResList->entry[i].type;
		rID = inResList->entry[i].id;
		inResList->entry[i].memH = nil;	/* clear the reference to start off */
		rH = Get1Resource(rType, rID);
		if (rH != nil) {	/* res not found? let it slide, but will probably cause trouble later */
			if (*rH != nil) {	/* this res is loaded */
				// еее for debugging put a check for purgable resources here, and warn them ееее
				DetachResource(rH);				/* don't dump the handle, someone may be using it */
				inResList->entry[i].memH = rH;	/* store a reference to it for later reattachment */
				if (rType == 'MENU') {
				  #if TARGET_API_MAC_CARBON
					inResList->entry[i].menuProc = 0;           // not used in Carbon
					inResList->entry[i].menuEnableFlags = 0;
				  #else
					inResList->entry[i].menuProc = (**(MenuHandle)rH).menuProc;
					inResList->entry[i].menuEnableFlags = (**(MenuHandle)rH).enableFlags;
			      #endif
				}
			} else {
				ReleaseResource(rH);
			}
		}
	}
	SetResLoad(true);
}

#if !TARGET_API_MAC_CARBON
/* installs the new resources in place of the old ones, currList must have 
	the handles of the resources that are loaded from the old module */
void 
LMd_LoadNewResources(ResListPtr *inCurrList, ResListPtr inNewList, ResAdjustProcPtr inAdjustProcP) {
	int 			i, oldHState;
	ResType			rType;
	short			rID;
	Handle			rH, tempRH, mapH;
	long 			rSize, rNewSize;
	RsrcMapEntryPtr	resEntryP;

	if (*inCurrList == nil) {		/* special case, first time called, there was no module */
		*inCurrList = inNewList;	/* previously opened, so just assign the list and we are done */
		return;
	}
	
	SetResLoad(false);
	for (i = 0; i < (**inCurrList).count; i++) {
		if ((**inCurrList).entry[i].memH == nil)	/* ones marked as nil were never loaded */
			continue;							/* so we can skip right on to the next one */
		rType = (**inCurrList).entry[i].type;
		rID = (**inCurrList).entry[i].id;
		rH = (**inCurrList).entry[i].memH;
		tempRH = Get1Resource(rType, rID);
		if (tempRH != nil) {	/* res not found? almost certain to cause trouble later */
			/* We need to transfer the data from the old resource to the new one: */
			/* First, make sure the old handle is big enough to accomodate the new data. */
			rSize = GetHandleSize(rH);
			rNewSize = GetMaxResourceSize(tempRH);
			if (rNewSize > rSize) {	/* new data is bigger, need to adjust size */
				SetHandleSize(rH, rNewSize);	/* this may fail if the handle is locked, so */
				rNewSize = GetHandleSize(rH);	/* we need to find out how big it actually is */
			}
			/* Now, we transfer as much of the data as we can from the module file into the old
				resource handle. */
			oldHState = HGetState(rH);	/* Save handle's current lock state */
			HLock(rH);					/* Now make sure it's locked */
			ReadPartialResource(tempRH, 0, *rH, rNewSize);	/* Read new data into old handle */
			HSetState(rH, oldHState);	/* Restore handle's lock state */
			/* Lastly, we dump the empty handle of the new resource, insert the old handle
				(with the new contents) into the resource map entry, and set the resource
				attrib of the old handle so the mem manager knows how to deal with it. */
			mapH = LMGetTopMapHndl();	/* Get the current resource map, then find the resource's */
			resEntryP = (RsrcMapEntryPtr)(*mapH + RsrcMapEntry(tempRH)); /* entry in the map */
			DetachResource(tempRH);	/* We have to do this before we replace the entry or detach */
			resEntryP->resH = rH;	/* won't find the ref. Fortunately, detach doesn't move mem. */
			DisposeHandle(tempRH);	/* Now we can do the dispose which may move memory on us */
			HSetRBit(rH);			/* Finally, we restore the old handle's status as a resource */
			(**inCurrList).entry[i].memH = rH;
			if (inAdjustProcP) {
				inAdjustProcP(&(**inCurrList).entry[i], rNewSize);	/* do any adjustments necessary */
			}
		}
	}
	SetResLoad(true);
	
	DisposePtr((Ptr)*inCurrList);
	*inCurrList = inNewList;
}
#endif !TARGET_API_MAC_CARBON

/* called from within LMd_StandardRedraw() */
pascal void 
LMd_RedrawMenuBar(void) {
	MenuHandle menuH = NewMenu(0x7fff, "\px");	/* we do this to force a recalc of the menubar */
	MacInsertMenu(menuH, 0);
	MacDeleteMenu(0x7fff);
	DisposeMenu(menuH);
	MacDrawMenuBar();
}

/* displays the PICT 0 resource, if any, from the module file */
pascal void 
LMd_ShowModuleSplash(void) {
	PicHandle splashPict = nil;
	Rect r;
	WindowPtr splashWindow = nil;
	GrafPtr savePort;
	Str15 title;
	unsigned long endTicks;
	EventRecord theEvent;
	splashPict = (PicHandle) Get1Resource('PICT', 1);
	if (splashPict != nil) {
		MacLoadResource((Handle)splashPict);
		r = (**splashPict).picFrame;
		LMd_AdjustWindowRect(&r);
		title[0] = '\0';
		splashWindow = NewCWindow(nil, &r, title, true, plainDBox, nil, false, 0);
		if (splashWindow != nil) {
			GetPort(&savePort);
			SetPortWindowPort(splashWindow);
			MacOffsetRect(&r, -r.left, -r.top);
			MacLoadResource((Handle)splashPict);
			DrawPicture(splashPict, &r);
			endTicks = TickCount() + 240;
			while (TickCount() < endTicks) {
				if ( WaitNextEvent(mDownMask, &theEvent, 30, nil) ) {
					endTicks = 0;
				}
			}
			DisposeWindow(splashWindow);
		}
		ReleaseResource((Handle)splashPict);
	}
}

#pragma mark-
/* low level routines that provide reenterable/threadable versions of high level calls */

void
LMd_Initialize(LMdDataT *inDataP, short smLangCode) {
	short currModuleNum;
	LMd_InitDataStructs(inDataP);				/* init all data to startup nil values */
	inDataP->installed = true;
	inDataP->menuBarH = nil;
	if (gLMd_ShowInitialSplash) {
		inDataP->openProcP = LMd_StandardOpen;	/* then install the splash screen display */
	}
	inDataP->excludeProcP = LMd_StandardResExclude;	/* install the normal exclude proc */
	inDataP->adjustProcP = LMd_StandardResAdjust;	/* install the normal adjust proc */

		/* first find the folder with the modules */
	LMd_FindModuleFolder(&inDataP->moduleFolderSpec);

		/* then cache its modification date/time */
	LMd_HasFolderChangedSince(&inDataP->moduleFolderSpec, &inDataP->lastFolderChange);

		/* build list of the modules in the folder */
	LMd_BuildModuleList(&inDataP->moduleFolderSpec, &inDataP->modules);

		/* then find the module which matches the language specified */
	currModuleNum = LMd_FindModuleInListByLangCode(inDataP->modules, smLangCode);
	if (currModuleNum == -1) {	/* neither the requested module nor the system language */
		currModuleNum = 0;		/* module were found, just take the first one available */
	}
		/* and record its index as that of the current module */
	inDataP->modules->current = currModuleNum;

		/* open the current module */
	LMd_OpenModule(&inDataP->modules->entry[currModuleNum]);

		/* use the list to make the menu. done after module is open because module may have
			translation of the language MENU itself */
	LMd_BuildMenuFromList(inDataP->modules, &inDataP->langMenu);

		/* this is done AFTER the first open because we don't want to do a redraw when
			first setting up. Generally speaking, nothing should have actually been drawn
			yet, so a redraw would just give unnecessary flicker */
	inDataP->redrawProcP = LMd_StandardRedraw;
	inDataP->openProcP = LMd_StandardOpen;	/* (re)install the splash screen display */
}

/* reentrable low level call */
void
LMd_Cleanup(LMdDataT *inDataP) {
	short currModuleNum = inDataP->modules->current;
//	LMd_CloseModule(&inDataP->modules->entry[currModuleNum]);	/* close the current module */
	DisposeMenu(inDataP->langMenu);							/* then dispose of what we created */
	DisposePtr((Ptr)inDataP->modules);
	DisposePtr((Ptr)inDataP->currResList);
}

void 
LMd_HandleMenuSelect(LMdDataT *inDataP, short inMenuItemNum) {
	short currModuleNum = inDataP->modules->current;
	short selectedModuleNum = inMenuItemNum -1;
	if ( (selectedModuleNum < 0) || (selectedModuleNum > inDataP->modules->count) ) {
		return;		/* invalid value passed in, ignore it and do nothing */
	}
	if (selectedModuleNum == currModuleNum) {
		return;		/* selected the existing module, ignore it and do nothing */
	}
	MacCheckMenuItem(inDataP->langMenu, currModuleNum + 1, false);	/* clear old checkmark from menu */
	MacCheckMenuItem(inDataP->langMenu, selectedModuleNum + 1, true);	/* checkmark current item in menu */
	inDataP->modules->current = selectedModuleNum;	/* set the new current module number */
	LMd_CloseModule(&inDataP->modules->entry[currModuleNum]);	/* close the old module */
	LMd_OpenModule(&inDataP->modules->entry[selectedModuleNum]);	/* open the new module */
}

/* call when your app switched to front. Caches folder info and checks
	the folder modification date first. Only rebuilds menu if folder changed 
	Even better is to check when a click is detected in the menu, so the menu
	is always up to date */
void 
LMd_CheckForNewModules(LMdDataT *inDataP) {
	short currModuleNum = inDataP->modules->current;	/* we'll need it if we rebuild */
	LangModuleInfoT currLMdInfo = inDataP->modules->entry[currModuleNum];
	/* see if the modules folder has been changed */
	if ( LMd_HasFolderChangedSince(&inDataP->moduleFolderSpec, &inDataP->lastFolderChange) ) {	
			/* rebuild list of the modules in the folder */
		LMd_BuildModuleList(&inDataP->moduleFolderSpec, &inDataP->modules);
			/* then find the module which matches the old current language */
		currModuleNum = LMd_FindModuleInListByLangCode(inDataP->modules, currLMdInfo.smLangCode);
		if (currModuleNum == -1) {	/* neither the requested module nor the system language */
			currModuleNum = 0;		/* module were found, just take the first one available */
		}
		inDataP->modules->current = currModuleNum;	/* reset the current module number */
			/* now see if the module we just found is the same one (perhaps the user moved the file?) */
		if (!LMd_ModulesAreSame(&currLMdInfo, &inDataP->modules->entry[currModuleNum])) {
			LMd_CloseModule(&currLMdInfo);	/* close the old module */
			LMd_OpenModule(&inDataP->modules->entry[currModuleNum]);	/* open the new module */
		} else {	/* current module still there, all we need to do is restore the fileRefNum */
			inDataP->modules->entry[currModuleNum].moduleFileRefNum = currLMdInfo.moduleFileRefNum;
		}
			/* use the new module list to rebuild the menu */
		LMd_BuildMenuFromList(inDataP->modules, &inDataP->langMenu);
	}
}

/* returns current language code, pass in nil if you don't want the full info
	you'll probably use this to find out what module was last used when
	you update your prefs file as you quit (or whenever) */
short 
LMd_GetCurrentModuleInfo(LMdDataT *inDataP, LangModuleInfoT *outLMdInfo) {
	short currModuleNum;
	currModuleNum = inDataP->modules->current;
	if (currModuleNum < 0) {			/* if current module number hasn't been assigned */
		return -1;
	}
	if (outLMdInfo != nil) {
		*outLMdInfo = inDataP->modules->entry[currModuleNum];
	}
	return inDataP->modules->entry[currModuleNum].smLangCode;
}

/* returns current info on a module specified by it's language code	*/
/* returns -1 in both countryCode and smLangCode if not found */
void 
LMd_GetModuleInfo(LMdDataT *inDataP, short smLangCode, LangModuleInfoT *outLMdInfo) {
	short i;
	i = LMd_FindModuleInListByLangCode(inDataP->modules, smLangCode);
	if (i == -1) {
		outLMdInfo->smLangCode = -1;	/* didn't find the requested language */
		outLMdInfo->countryCode = -1;
	} else {
		*outLMdInfo = inDataP->modules->entry[i]; /* found it */
	}
}

/* returns current info on a module specified by it's position in the list
   you probably don't really need to use this unless you want to 
   display the names of all the current module files available or something	*/
void 
LMd_GetIndModuleInfo(LMdDataT *inDataP, short inIndex, LangModuleInfoT *outLMdInfo) {
	if ( (inIndex < 0) || (inIndex >= inDataP->modules->count) ) {
		outLMdInfo->smLangCode = -1;	/* no such entry */
		outLMdInfo->countryCode = -1;
	} else {
		*outLMdInfo = inDataP->modules->entry[inIndex]; /* found it */
	}
}

#pragma mark-
/* other low level routines, that shouldn't need to be called from client code*/

void
LMd_InitDataStructs(LMdDataT *inDataP) {
	inDataP->langMenu = nil;
	inDataP->modules = nil;
	inDataP->currResList = nil;
	inDataP->redrawProcP = nil;
	inDataP->preOpenProcP = nil;
	inDataP->openProcP = nil;
	inDataP->closeProcP = nil;
	inDataP->excludeProcP = nil;
	inDataP->lastFolderChange = 0;
	inDataP->reserved = 0;
	inDataP->installed = false;
}

/* Locates the module folder. First looks for something named "Modules" in the application's
	directory. If that's not found, it looks in the Application's folder for whatever folder
	contains a file of type 'LMod', and treats that folder as the module folder. If that fails
	then it does an ExitToShell */
void 		
LMd_FindModuleFolder(FSSpec *outModuleFolderSpec) {
	OSErr err;
	long dirID;
	CInfoPBRec* cipbrp;
	err = FSMakeFSSpec(0, 0, MODULE_FOLDER_NAME, outModuleFolderSpec);
	if (err != noErr) {
		cipbrp = (CInfoPBRec*)NewPtr(sizeof(CInfoPBRec));
		if (cipbrp == nil) {
			err = memFullErr;
		} else {
			dirID = LMd_FindDirForFileType(cipbrp, outModuleFolderSpec->vRefNum, 
							outModuleFolderSpec->parID, MODULE_FILE_TYPE);
			if (dirID == 0) {
				err = fnfErr;
			} else {
				err = FSMakeFSSpec(0, dirID, "\p", outModuleFolderSpec);
			}
		}
	}
	if (err != noErr) {
		LMd_UnrecoverableError("\pUnrecoverable Error","\pCannot find language plugins.", err);
	}
}

short
LMd_HasFolderChangedSince(FSSpec *ioModuleFolderSpec, unsigned long *ioDateTime) {
	OSErr	err;
	CInfoPBRec	pb;
	DirInfo	*dpb = (DirInfo *)&pb;
	dpb->ioFDirIndex = 0;	/* checking for one specific item	*/
	dpb->ioVRefNum = ioModuleFolderSpec->vRefNum;
	dpb->ioDrDirID = ioModuleFolderSpec->parID;
	dpb->ioNamePtr = (StringPtr)&ioModuleFolderSpec->name[0];
	err = PBGetCatInfoSync(&pb);
	if ( err != noErr) {	/* error accessing the folder, perhaps someone moved it	*/
		LMd_FindModuleFolder(ioModuleFolderSpec);	/* try to find it again */
		dpb->ioVRefNum = ioModuleFolderSpec->vRefNum;
		dpb->ioDrDirID = ioModuleFolderSpec->parID;
		dpb->ioNamePtr = (StringPtr)&ioModuleFolderSpec->name[0];	/* just to be safe */
		err = PBGetCatInfoSync(&pb);
		if (err != noErr) {	/* an error should never occur here, since we just found the folder */
			DebugStr("\pOdd, this should never happen. g to resume, es to quit");
		}
	}
	if (dpb->ioDrMdDat > *ioDateTime) {	/* has been modified since we last loaded it */
		*ioDateTime = dpb->ioDrMdDat;
		return true;
	} else {
		return false;
	}
}

void
LMd_BuildModuleList(const FSSpec *inModuleFolderSpec, LMdInfoListPtr *outList) {
	LMdInfoListPtr	newListP;
	LangModuleInfoT	entry;			/* we use this for sorting */
	long		hh, stopH, step;	/* these too */
	CInfoPBRec	cipbr;
	FSSpec		spec;
	HFileInfo	*fpb = (HFileInfo *)&cipbr;	/* helpful pointer */
	OSErr 		err;
	short 		ii, modCnt, fileRef, origRef, vRef = inModuleFolderSpec->vRefNum;
	long		moduleFldrID;
	long		i, idx, count = 0;
	int			realloc = false;
	VersRecHndl versResH;
	Str255		name;					/* get name to use in making an FSSpec for the file*/

	/* find out how many modules we're actually talking about here */
	modCnt = LMd_CountFilesOfTypeInDir(inModuleFolderSpec, MODULE_FILE_TYPE);
	
	if (*outList == nil) {
		realloc = true;
	} else if (modCnt > (**outList).count) {
		DisposePtr((Ptr)*outList);	/* dispose of the old list, which we are no longer needing */
		realloc = true;
	}
	if (realloc) {	/* allocate the memory for a list to hold all the module info */
		newListP = (LMdInfoListPtr) NewPtrClear(sizeof(LMdInfoListT) + modCnt*sizeof(LangModuleInfoT));
	} else {
		newListP = *outList;	/* just use the existing list */
	}

	if (newListP == nil) {	/* opps, allocation failed, try again with just one module */
		newListP = (LMdInfoListPtr) NewPtrClear(sizeof(LMdInfoListT) + sizeof(LangModuleInfoT));
		if (newListP == nil) {	/* allocation still failed, this is unrecoverable */
			LMd_UnrecoverableError("\pOut Of Memory","\pCannot load language plugins.", MemError());
		} else {
			modCnt = 1;	/* whew!, we got enough mem for a single item, we'll work with it */
		}
	}

	/* get the actual dirID from the Module Folder FSSpec */
	moduleFldrID = LMd_GetDirIDFromFolderFSSpec(inModuleFolderSpec);

	/* now we can actually fill in the info */
	origRef = CurResFile();				/* save the current res file for later restoration */
	fpb->ioNamePtr = name; 	
	fpb->ioVRefNum = vRef;				/* appropriate volume */
	idx = 0;							/* which module we are setting */
	for(i = 1; idx < modCnt; i++) {		/* PB indexing loop, but don't overrun our allocated list */
		fpb->ioDirID = moduleFldrID;	/* must set on each loop */
		fpb->ioFDirIndex = i;
		err = PBGetCatInfoSync(&cipbr);
		if (err) break;					/* exit when no more entries, or any other error occurs */
		if (!(fpb->ioFlAttrib & 16)) {	/* ignore subdirectories */
			if ( fpb->ioFlFndrInfo.fdType == MODULE_FILE_TYPE) { 	 /* found a file of correct type */
				err = FSMakeFSSpec(vRef, moduleFldrID, name, &spec); /* make spec for the file */
				if (err == noErr) {		/* ignore the file if an error occured */
					fileRef = FSpOpenResFile(&spec, fsRdPerm);	/* only read from the module */
					err = ResError();
					if (err == noErr) {
						newListP->entry[idx].moduleFile = spec;
						newListP->entry[idx].moduleFileRefNum = -1;	/* we won't have the file open long */
						UseResFile(fileRef);
						versResH = (VersRecHndl) Get1Resource('vers', 1);	/* get the version info */
						if (versResH == nil) {
							newListP->entry[idx].countryCode = 0;	/* and country code for this module */
							newListP->entry[idx].version[0] = 0;
						} else {
							newListP->entry[idx].countryCode = (**versResH).countryCode;
							for (ii = 0; ii <= 15; ii++) {
								newListP->entry[idx].version[ii] = (**versResH).shortVersion[ii];
							}
						}
						newListP->entry[idx].smLangCode = LMd_GetLangFromCountryCode(newListP->entry[idx].countryCode);
						GetIndString(newListP->entry[idx].moduleLanguageName, 1, 2);
						GetIndString(newListP->entry[idx].translatorsName, 2, 1);
						CloseResFile(fileRef);
						idx++;			/* we've got one more item in the module list */
					}
				}
			}
		}
	}
	newListP->count = idx;
	newListP->current = -1;	/* not yet set */
	UseResFile(origRef);	/* restore old res file */
	*outList = newListP;	/* return the new list */
	
	/* now we need to sort the list so that everything's in alphabetical order for the menu*/
	/* we use a shell sort because it's reasonably quick, simple, and non-recursive */

	if (newListP->count <= 1) {
		return;			/* a single item is always sorted */
	}
	hh = 1;				/* Find starting "h" value */
	stopH = newListP->count / 9;
	while (hh < stopH) {
		hh = 3 * hh + 1;
	}	/* got "h" value, now do actual sort */
	for ( ; hh > 0; hh /= 3) {
		for (step = hh + 1; step <= newListP->count; step++) {
			entry = newListP->entry[step-1];	/* fetch the item into the buffer */
			for (i = step - hh - 1; i >= 0; i -= hh) {
				/* if buffered item is bigger or equal to the item at [i], check next i */
				if (StringOrder(entry.moduleLanguageName, 	/* compare the two language names */
						newListP->entry[i].moduleLanguageName,
						smSystemScript, smSystemScript,	/* еее for now, just compare with sys script */
						iuSystemCurLang, iuSystemCurLang) >= 0) {
					break;
				}
				newListP->entry[i+hh] = newListP->entry[i];/* store the item at i into the slot at i+hh */
			}
			newListP->entry[i+hh] = entry;	/* store the buffered item into the slot at i+hh */
		}
	}
}

void
LMd_BuildMenuFromList(const LMdInfoListT* inList, MenuHandle *ioMenuH) {
	short	i;
	OSErr	err;
	if (inList->count > 0) {
		if (*ioMenuH == nil) {
			*ioMenuH = MacGetMenu(gLMd_MenuID);	/* we use GetMenu to allow a localized language menu */
		} else {
			while (CountMenuItems(*ioMenuH) > 0) {
				DeleteMenuItem(*ioMenuH, 1);	/* clear out all the old menu items */
			}
		}
		if (*ioMenuH == nil) {	/* couldn't find it, create one of our own here */
			*ioMenuH = NewMenu(gLMd_MenuID, "\p");	/* the character after \p is a diamond mark */
		}
		if (*ioMenuH == nil) {	/* still didn't make it? */
			err = MemError();
			if (err == noErr) err = memFullErr;
			LMd_UnrecoverableError("\pError.", "\pThe language menu could not be built.", err);
		} else {
			for (i = 0; i < inList->count; i++) {
				MacAppendMenu(*ioMenuH, inList->entry[i].moduleLanguageName);	/* put mod names into menu */
			}
			MacCheckMenuItem(*ioMenuH, inList->current + 1, true);	/* mark the current item with a check */
		}
	}
}

short
LMd_FindModuleInListByLangCode(LMdInfoListT *inList, short smLangCode) {
	short i;
	if (smLangCode == -1) {	/* check for special value -1, which means system language */
		smLangCode = GetScriptVariable(smSystemScript, smScriptLang);
	}
	for (i = 0; i < inList->count; i++) {
		if (inList->entry[i].smLangCode == smLangCode) {
			return i;	/* found what we were looking for, nothing more to do */
		}
	}
	return -1;	/* didn't find the requested language */
}

void		
LMd_OpenModule(LangModuleInfoT *inLMdInfo) {
	short fileRef;
	ResListPtr newResList = nil;
	fileRef = FSpOpenResFile(&inLMdInfo->moduleFile, fsRdPerm);	/* only read from the module */
	UseResFile(fileRef);
	inLMdInfo->moduleFileRefNum = fileRef;
	if (gLMd_Data.preOpenProcP != nil)		/* call the pre-open proc if installed */
		gLMd_Data.preOpenProcP(inLMdInfo);
	LMd_BuildResourceList(fileRef, &newResList, gLMd_Data.excludeProcP); /* find out what's in the new file */
  #if !TARGET_API_MAC_CARBON
	LMd_LoadNewResources(&gLMd_Data.currResList, newResList, gLMd_Data.adjustProcP); /* then make it work */
  #endif  
/*
	if (gLMd_Data.menuBarH != nil) {
		SetMenuBar(gLMd_Data.menuBarH);	/ * restore the menu bar that was in use * /
	}
*/
	if (gLMd_Data.redrawProcP != nil) {		/* call the redraw proc if there is one */
		gLMd_Data.redrawProcP();
	}
	if (gLMd_Data.openProcP != nil)	{		/* call the open proc if installed */
		gLMd_Data.openProcP(inLMdInfo);
	}
}

void		
LMd_CloseModule(LangModuleInfoT *inLMdInfo) {
	short fileRef = inLMdInfo->moduleFileRefNum;
	short oldRef = CurResFile();
	if (fileRef != -1) {
		UseResFile(fileRef);
		if (gLMd_Data.closeProcP != nil) {			/* call the close proc if installed */
			gLMd_Data.closeProcP(inLMdInfo);
		}
		LMd_PurgeOldResources(gLMd_Data.currResList);
		CloseResFile(fileRef);
		UseResFile(oldRef);
		inLMdInfo->moduleFileRefNum = -1;
		gLMd_Data.menuBarH = GetMenuBar();	/* get the menu bar that is currently in use */
	}
}

/* returns dirID of parent directory for file of desired type */
/* cipbrp is parameter block; must be pre-allocated */
long
LMd_FindDirForFileType(CInfoPBRec* cipbrp, short vRefNum, long parID, OSType fileType) {
	HFileInfo	*fpb = (HFileInfo *)cipbrp;	/* two helpful pointers */
	DirInfo		*dpb = (DirInfo *)cipbrp;
	OSErr 		err;
	short 		idx;
	long		foundID = 0;

	Str255		name;			/* don't really need the name */
	fpb->ioNamePtr = name; 		/* but we'll get it anyway to facilitate debugging */
	fpb->ioVRefNum = vRefNum;	/* default volume */

	for(idx = 1; foundID == 0; idx++) {	/* indexing loop */
		fpb->ioDirID = parID;		/* must set on each loop */
		fpb->ioFDirIndex = idx;

		err = PBGetCatInfoSync(cipbrp);
		if (err) break;	/* exit when no more entries, or any other error occurs */

		if (fpb->ioFlAttrib & 16) {	/* found a subdirectory */
			foundID = LMd_FindDirForFileType(cipbrp, vRefNum, dpb->ioDrDirID, fileType); /* recursive call */
			fpb->ioNamePtr = name;	/* recursive call has left block with an invalid ptr for name */
		} else if ( fpb->ioFlFndrInfo.fdType == fileType) {
			foundID = fpb->ioFlParID;
		}
	}
	return foundID;
}

long
LMd_CountFilesOfTypeInDir(const FSSpec *inFolderSpec, OSType fileType) {
	CInfoPBRec	cipbr;
	HFileInfo	*fpb = (HFileInfo *)&cipbr;	/* helpful pointer */
	OSErr 		err;
	short 		idx;
	long		count = 0, parID;
	Str255		name;						/* don't really need the name */
	parID = LMd_GetDirIDFromFolderFSSpec(inFolderSpec);
	fpb->ioNamePtr = name; 					/* but we'll get it anyway to facilitate debugging */
	fpb->ioVRefNum = inFolderSpec->vRefNum;	/* default volume */
	for(idx = 1; true; idx++) {				/* indexing loop */
		fpb->ioDirID = parID;				/* must set on each loop */
		fpb->ioFDirIndex = idx;
		err = PBGetCatInfoSync(&cipbr);
		if (err) break;					/* exit when no more entries, or any other error occurs */
		if (!(fpb->ioFlAttrib & 16)) {	/* ignore subdirectories */
			if ( fpb->ioFlFndrInfo.fdType == fileType) {
				count++;				/* found a file of correct type, increment count */
			}
		}
	}
	return count;			/* right back at'ya with the number of kind files we found */
}

/* return the DirID of a folder specified by an FSSpec record */
/* returns 0 if FSSpec is not an existing folder */
long
LMd_GetDirIDFromFolderFSSpec(const FSSpec *inFolderSpec) {
	CInfoPBRec	cipbr;
	DirInfo		*dpb = (DirInfo*)&cipbr;
	OSErr 		err;
	dpb->ioNamePtr = (StringPtr)&inFolderSpec->name[0]; /* folder (or file's) name */
	dpb->ioVRefNum = inFolderSpec->vRefNum;		/* default volume */
	dpb->ioDrDirID = inFolderSpec->parID;		/* parent id of folder (or file) */
	dpb->ioFDirIndex = 0;						/* get one specific item */
	err = PBGetCatInfoSync(&cipbr);
	if ( (err == noErr) && (dpb->ioFlAttrib & 16) ) {	/* is a directory */
		return dpb->ioDrDirID;	/* return the directory id */
	} else {
		return 0; /* some kind of problem: maybe an OS error, or perhaps the item wasn't a folder */
	}
}




#define NUM_COUNTRIES	maxCountry - minCountry

short gLMd_CountyLangCodes[NUM_COUNTRIES] = { 
	langEnglish, 		/* verUS */
	langFrench, 		/* verFrance */
	langEnglish, 		/* verBritain */
	langGerman, 		/* verGermany */
	langItalian, 		/* verItaly */
	langDutch, 			/* verNetherlands */
	langFrench, 		/* verFrBelgiumLux,				/* French for Belgium & Luxembourg */
	langSwedish, 		/* verSweden */
	langSpanish, 		/* verSpain */
	langDanish, 		/* verDenmark */
	langPortuguese, 	/* verPortugal */
	langFrench, 		/* verFrCanada */
	langNorwegian, 		/* verNorway */
	langHebrew, 		/* verIsrael */
	langJapanese, 		/* verJapan */
	langEnglish, 		/* verAustralia */
	langArabic, 		/* verArabic,						/* synonym for verArabia */
	langFinnish, 		/* verFinland */
	langFrench, 		/* verFrSwiss,						/* French Swiss */
	langGerman, 		/* verGrSwiss,						/* German Swiss */
	langGreek, 			/* verGreece */
	langIcelandic, 		/* verIceland */
	langMaltese, 		/* verMalta */
	langGreek, 			/* verCyprus */
	langTurkish, 		/* verTurkey */
	langCroatian, 		/* verYugoCroatian,					/* Croatian system for Yugoslavia */
	langDutch, 			/* verNetherlandsComma */
	langFrench, 		/* verBelgiumLuxPoint */
	langEnglish, 		/* verCanadaComma */
	langEnglish, 		/* verCanadaPoint */
	langPortuguese, 	/* vervariantPortugal */
	langNorwegian, 		/* vervariantNorway */
	langDanish, 		/* vervariantDenmark */
	langHindi, 			/* verIndiaHindi,					/* Hindi system for India */
	-1, 				/* verPakistan */
	langTurkish, 		/* verTurkishModified */
	langRomanian, 		/* verRomania */
	-1, 				/* verGreekAncient */
	langLithuanian, 	/* verLithuania */
	langPolish, 		/* verPoland */
	langHungarian, 		/* verHungary */
	langEstonian, 		/* verEstonia */
	langLatvian, 		/* verLatvia */
	langLappish, 		/* verLapland */
	-1, 				/* verFaeroeIsl */
	langPersian, 		/* verIran */
	langRussian, 		/* verRussia */
	langEnglish, 		/* verIreland,				/* English-language version for Ireland */
	langKorean, 		/* verKorea */
	langTradChinese, 	/* verChina */
	langSimpChinese, 	/* verTaiwan */
	langThai, 			/* verThailand */
	langCzech, 			/* verCzech */
	langSlovak, 		/* verSlovak */
	-1, 				/* verGenericFE *//* Generic Far East... and what language would that be?*/
	-1, 				/* verMagyar */
	langBengali, 		/* verBengali */
	langByelorussian, 	/* verByeloRussian */
	langUkrainian, 		/* verUkrania */
	langItalian, 		/* verItalianSwiss */
	langGerman, 		/* verAlternateGr */
	langCroatian  		/* verCroatia */
};


short
LMd_GetLangFromCountryCode(short inCountryCode) {
	if ( (inCountryCode >= minCountry) && (inCountryCode <= maxCountry) )
		return gLMd_CountyLangCodes[inCountryCode];
	else {	/* not in the list?, return the system's language code */
		return GetScriptVariable(smSystemScript, smScriptLang);
	}
}

/* Deal with an unrecoverable error */
void
LMd_UnrecoverableError(Str255 line1, Str255 line2, short err) {
	Str255 		errCodeStr;
	Rect		wBounds;
	WindowPtr	windowP;
	gLMd_LastError = err;
	if (gLMd_ShowFail) {
		wBounds.left = 0;
		wBounds.top = 0;
		wBounds.right = 300;
		wBounds.bottom = 100;
		LMd_AdjustWindowRect(&wBounds);
		windowP = NewWindow(nil, &wBounds, "\p", true, dBoxProc, (WindowPtr) -1, false, 0);
		SetPortWindowPort(windowP);
		TextFont(0);
		MoveTo(20, 22);
		DrawString(line1);
		MoveTo(20, 38);
		DrawString(line2);
		MoveTo(20, 54);
		NumToString(err, errCodeStr);
		DrawString(errCodeStr);
		MoveTo(50, 86);
		if (gLMd_ExitOnFail) {
			DrawString("\p(Click the mouse to exit)");
		} else {
			DrawString("\p(Click the mouse to continue)");
		}
		
		while (!Button()) {};
	}
	if (gLMd_ExitOnFail) {
		ExitToShell();
	}
// еее╩need to fix the following line ееее
//	gLMd_Installed = false;	/* obviously this never gets executed if we ExitToShell above */
}

/* Adjust a rectangle to the appropriate alert position on the main monitor */
void
LMd_AdjustWindowRect(Rect *ioRect) {
	Rect sr;
	short width, height, left, top;
	GDHandle gdh = nil;
	gdh = GetMainDevice();
	sr = (**gdh).gdRect;
	width = ioRect->right - ioRect->left;
	left = (sr.right - sr.left - width)/2 + sr.left;
	height = ioRect->bottom - ioRect->top;
	top = (sr.bottom - sr.top - height)/3 + sr.top;
	MacOffsetRect(ioRect, left - ioRect->left, top - ioRect->top);
}

/* compare two language module info recs to see if they are for the same module */
short
LMd_ModulesAreSame(LangModuleInfoT *inLMdA, LangModuleInfoT *inLMdB) {
	if (inLMdA->countryCode != inLMdB->countryCode) {
		return false;
	}
	if (inLMdA->smLangCode != inLMdB->smLangCode) {
		return false;
	}
	if (!EqualString(inLMdA->moduleLanguageName, inLMdB->moduleLanguageName, true, true) ) {
		return false;
	}
	if (!EqualString(inLMdA->version, inLMdB->version, true, true) ) {
		return false;
	}
	return true;
}
