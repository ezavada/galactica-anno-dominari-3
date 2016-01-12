/*--------------------------------------------------------------------
// LangModule.h
//
// Routines to simplify internationalization of MacOS programs
//
// copyright © 1996-97, Edmund R. Zavada <ezavada@kagi.com>
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

#ifndef LANGUAGE_MODULE_H_INCLUDED
#define LANGUAGE_MODULE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


#ifndef __MACTYPES__
  #include <MacTypes.h>
#endif
#ifndef __MACMEMORY__
  #include <MacMemory.h>
#endif
#ifndef __MENUS__
  #include <Menus.h>
#endif

/* =========================================================================================== */
/* STRUCTURES                                                                                  */
/* =========================================================================================== */

#define MODULE_FILE_TYPE	'LMod'
#define MODULE_FOLDER_NAME	"\pPlug-ins"
//#define UNLIKELY_FILE_NAME	"\pNon-Existant File"

#pragma export on

typedef struct LangModuleInfoT {
	short		countryCode;		/* as taken from the module's "vers" resource */
	short		smLangCode;			/* defined based on the countryCode */
	FSSpec		moduleFile;
	short		moduleFileRefNum;	/* only valid when open, -1 all other times */
	Str255		moduleLanguageName;	/* specified in STR# 0, index 1 */
	Str255		translatorsName;	/* in STR# 0, index 2 */
	Str15		version;			/* as taken from the module's "vers" resource */
} LangModuleInfoT, *LangModuleInfoPtr, **LangModuleInfoHnd, **LangModuleInfoHdl;

typedef struct LMdInfoListT {
	short			allocated;
	short			count;
	short			current;
	LangModuleInfoT	entry[];
} LMdInfoListT, *LMdInfoListPtr;

typedef struct ResListEntryT {
	ResType		type;
	short		id;
	Handle		memH;
	Handle		menuProc;			/* these two are used internally to adjust menus */
	long		menuEnableFlags;
} ResListEntryT, *ResListEntryPtr;

typedef struct ResListT {
	short			allocated;
	short			count;
	ResListEntryT	entry[];
} ResListT, *ResListPtr;

typedef pascal void (*RedrawProcPtr)(void);
typedef pascal void (*ModulePreOpenProcPtr)(const LangModuleInfoT *inLMdInfo);
typedef pascal void (*ModuleOpenProcPtr)(const LangModuleInfoT *inLMdInfo);
typedef pascal void (*ModuleCloseProcPtr)(const LangModuleInfoT *inLMdInfo);
typedef pascal Boolean (*ResExcludeProcPtr)(ResType rType, short rID);
typedef pascal void (*ResAdjustProcPtr)(ResListEntryT *inResInfo, long inResSize);



/* =========================================================================================== */
/* ROUTINES                                                                                    */
/* =========================================================================================== */
/* High level routines for general use */


pascal short LMdGetVersion(void);
				/* returns a version number in case your code depends on features
				   in a certain version of a LanguageModules Shared Library*/

pascal short LMdError(void);
				/* returns the last error that caused a failure in a LangModule routine,
				   you don't need to check this unless you have set gLMd_ExitOnFail to false */

pascal void LMdInitialize(short smLangCode);
				/* call once at the beginning of your program pass in code
				   for startup module, or -1 to use OS's current code */
				/* NOTE: there are some globals defined below that you can use to
				   configure the behavior of the LangModule system before initializing it */

pascal void LMdCleanup(void);
				/* call once at the end of your program, after any possible 
				   use of resources from the modules */

pascal MenuHandle LMdGetLanguageMenu(void);
				/* returns a reference to the current language menu that lists the
				   modules that are available, install this in a popup or in the
				   menu bar	*/

pascal void LMdHandleMenuSelect(short inMenuItemNum);
				/* does all the work needed to figure out which module to switch to,
				   dump the old module, load the new one, and refresh the windows and
				   menu bar */

pascal void LMdCheckForNewModules(void);	
				/* call when your app switched to front. Caches folder info and checks
				   the folder modification date first. Only rebuilds menu if folder changed */

pascal short LMdGetCurrentModuleInfo(LangModuleInfoT *outLMdInfo);	
				/* returns current language code, pass in nil if you don't want the full info
				   you'll probably use this to find out what module was last used when
				   you update your prefs file as you quit (or whenever) */

pascal void LMdGetModuleInfo(short smLangCode, LangModuleInfoT *outLMdInfo);	
				/* returns current info on a module specified by it's language code
				   returns -1 in both countryCode and smLangCode if not found.
				   you probably don't really need to use this unless you want to 
				   display the name of the current module file or something	*/

pascal void LMdGetIndModuleInfo(short inIndex, LangModuleInfoT *outLMdInfo);
				/* returns current info on a module specified by it's position in the list
				   you probably don't really need to use this unless you want to 
				   display the names of the current module files available or something	*/

pascal short LMdGetNumModules(void);
				/* returns number of modules that are currently installed */
				
pascal void LMdMakeCurrentModuleTop(void);
				/* makes the resource file for the current module first in the resource
				   chain. You generally won't need this unless you have conflicts between
				   resource types and IDs in various resource files you have open */

/* =========================================================================================== */
/* CUSTOMIZATION ROUTINES                                                                      */
/* =========================================================================================== */
/* High level routines, for those who want a bit of customized behavior */
/* It is perfectly acceptable to install NIL for any of these procedures. Doing so simply
	means you don't want the Standard procs to be executed, and you don't have a replacement
	proc of your own. */
/* NOTE: These are NOT universal procedure pointers, so you CANNOT install PPC routines
	if you are using the 68k version, nor 68k routines with the PPC version. */


pascal void LMdInstallRedrawProc(RedrawProcPtr inRedrawProcP);
				/* call if you want to replace the standard refresh method used when
				   modules are switched, which is: tell the window mgr to redraw entire screen */

pascal void LMdInstallModulePreOpenProc(ModulePreOpenProcPtr inModulePreOpenProcP);
				/* install to do something when a module is first opened, the proc is called
				   after the close of the old module, with the file open, but before anything
				   is actually done with the module */

pascal void LMdInstallModuleOpenProc(ModuleOpenProcPtr inModuleOpenProcP);
				/* install to do something when a module is first opened, the proc is called
				   after the close of the old module, and call of PreOpenProc & RedrawProc
				   and display of module splash screen. */

pascal void LMdInstallModuleCloseProc(ModuleCloseProcPtr inModuleCloseProcP);
				/* install to do something when a module is closed, the proc is called before
				   file has actually been closed */

pascal void LMdInstallResExcludeProc(ResExcludeProcPtr inResExcludeProcP);
				/* install to do change the resources that are ignored when modules are switched.
				   Such resources will not be updated in memory even if they were loaded from 
				   a previous module. Your proc should return true for resources it wants to
				   exclude */

pascal void LMdInstallResAdjustProc(ResAdjustProcPtr inResAdjustProcP);
				/* install to do something after a resource that was in memory from an old
				   module has been replaced with a new one. The standard routine for this
				   recalculates the size of menus so they will draw correctly. */



/* A few access functions for globals you can set before calling LMdInitialize */


/* do we exit app on failure? defaults to true */
/* you can change this before or after initialization */
pascal void		LMd_SetExitOnFail(Boolean inExitOnFail);
pascal Boolean	LMd_GetExitOnFail(void);

/* do we show msg on failure? defaults to true */
/* you can change this before or after initialization */
pascal void		LMd_SetShowFail(Boolean inShowFail);
pascal Boolean	LMd_GetShowFail(void);

/* do we display splash for first module loaded? defaults to true */
/* changing this after initialization has absolutely no effect (except changing the value :-) */
pascal void		LMd_SetShowInitialSplash(Boolean inShowSplash);
pascal Boolean	LMd_GetShowInitialSplash(void);

/* id num of language MENU res, defaults to 1 */
/* this MENU res can be in the modules if you want to use a translated text title */
/* changing this after initialization might cause problems, better not to */
pascal void		LMd_SetLangMenuID(short inMenuID);
pascal short	LMd_GetLangMenuID(void);


/* Low level routines, which provide support for the high level routines above
   You shouldn't need to call these routines except from within custom procs	*/

pascal void LMd_StandardRedraw(void);	
				/* this is what is defined as the redraw proc by default
					just calls LMd_RedrawMenuBar() */
pascal void LMd_StandardOpen(const LangModuleInfoT *inLMdInfo);
				/* this is what is defined as the standard module open proc by default
					basically just calls LMd_ShowModuleSplash */
pascal Boolean LMd_StandardResExclude(ResType inType, short inID);
				/* this is the standard resExcludeProc, installed by default
					and called by LMd_BuildResourceList(). It keeps certain resources
					that are internal to the modules from being added to the
					resource list. If you create your own resExcludeProc, you
					should probably call this one from within it. */
pascal void LMd_StandardResAdjust(ResListEntryT *inResInfo, long inResSize);
				/* this is the standard resAdjustProc, installed by default
					and called by LMd_LoadNewResources(). It forces menus to
					recalculate their size so they will draw correctly. It 
					also takes care of adding everything to the new apple menu.
					If you create your own resAdjustProc, you should probably call 
					this one from within it. */

pascal void LMd_RedrawMenuBar(void);
				/* called from within LMd_StandardRedraw() */
pascal void LMd_ShowModuleSplash(void); // sounds like something from NASA, eh? :-)
				/* displays the PICT 0 resource, if any, from the module file */


#pragma export off

#ifdef __cplusplus
}
#endif

#endif // LANGUAGE_MODULE_H_INCLUDED



