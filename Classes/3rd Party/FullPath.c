/*     File:       FullPath.c      Contains:   Routines for dealing with full pathnames... if you really must.      Version:    Originally: MoreFiles 1.5.2				 Extensively Modified to work with POSIX, and native				 paths by Ed Zavada      Copyright:  copyright (c) 2005 by Edmund R. Zavada, all rights reserved.				 portions (c) 1995-2001 by Apple Computer, Inc., all rights reserved.*//* ORIGINAL APPLE LICENSE:    You may incorporate this sample code into your applications without    restriction, though the sample code has been provided "AS IS" and the    responsibility for its operation is 100% yours.  However, what you are    not permitted to do is to redistribute the source as "DSC Sample Code"    after having made changes. If you're going to re-distribute the source,    we require that you make it clear in the source that the code was    descended from Apple Sample Code, but that you've made changes.    IMPORTANT NOTE:*/#if defined( COMPILING_FOR_CARBON ) || defined( __CARBON__) || defined( TARGET_API_MAC_CARBON )#include "FullPath.h"#include <MacTypes.h>#include <MacErrors.h>#include <MacMemory.h>#include <Files.h>#include <TextUtils.h>#include <Aliases.h>#include <string.h>#include <stdio.h>#include <stdlib.h>#ifdef __cplusplusextern "C" {#endif/*	IMPORTANT NOTE:		The use of full pathnames is strongly discouraged. Full pathnames are	particularly unreliable as a means of identifying files, directories	or volumes within your application, for two primary reasons:		* 	The user can change the name of any element in the path at virtually		any time.	*	Volume names on the Macintosh are *not* unique. Multiple		mounted volumes can have the same name. For this reason, the use of		a full pathname to identify a specific volume may not produce the		results you expect. If more than one volume has the same name and		a full pathname is used, the File Manager currently uses the first		mounted volume it finds with a matching name in the volume queue.		In general, you should use a file�s name, parent directory ID, and	volume reference number to identify a file you want to open, delete,	or otherwise manipulate.		If you need to remember the location of a particular file across	subsequent system boots, use the Alias Manager to create an alias record	describing the file. If the Alias Manager is not available, you can save	the file�s name, its parent directory ID, and the name of the volume on	which it�s located. Although none of these methods is foolproof, they are	much more reliable than using full pathnames to identify files.		Nonetheless, it is sometimes useful to display a file�s full pathname to	the user. For example, a backup utility might display a list of full	pathnames of files as it copies them onto the backup medium. Or, a	utility might want to display a dialog box showing the full pathname of	a file when it needs the user�s confirmation to delete the file. No	matter how unreliable full pathnames may be from a file-specification	viewpoint, users understand them more readily than volume reference	numbers or directory IDs. (Hint: Use the TruncString function from	TextUtils.h with truncMiddle as the truncWhere argument to shorten	full pathnames to a displayable length.)		The following technique for constructing the full pathname of a file is	intended for display purposes only. Applications that depend on any	particular structure of a full pathname are likely to fail on alternate	foreign file systems or under future system software versions.*//*****************************************************************************/OSErr FSpGetNativeFullPath(const FSSpec* spec, char* outFullPath, long bufferSize){    // start by getting posix path    OSErr err = FSpGetPosixFullPath(spec, outFullPath, bufferSize);    // need to convert from Posix to Native    if (outFullPath[0] == '/') {        // full path, must remove leading slash        memcpy(outFullPath, &outFullPath[1], bufferSize);    } else {        // relative path, must start with colon        memcpy(&outFullPath[1], outFullPath, bufferSize-1);        outFullPath[0] = ':';    }    char * p;    p = strchr(outFullPath, '/');     while (p) {        *p = ':';        p = strchr(p+1, '/');    }        return err;}/*****************************************************************************/OSErr FSpGetPosixFullPath(const FSSpec* spec, char* outFullPath, long bufferSize){	OSErr		result;	OSErr		realResult;	CInfoPBRec	pb;		char tmpStr[256];   // for pascal to C string conversions.	char* workBuffer = (char*) malloc(bufferSize);	if (workBuffer == NULL) 	{	    return memFullErr;	}	/* Default to noErr */	realResult = result = noErr;		if ( spec->parID == fsRtParID )	{		/* The object is a volume */				p2cstrcpy(tmpStr, spec->name);    #ifdef __MACH__		snprintf(outFullPath, bufferSize, "/Volumes/%s/", tmpStr);    #else		snprintf(outFullPath, bufferSize, "/%s/", tmpStr);    #endif ! __MACH__		outFullPath[bufferSize-1] = 0;	}	else	{		/* The object isn't a volume */		Str255 s;		memcpy(s, spec->name, spec->name[0]+1);				/* Is the object a file or a directory? */		pb.dirInfo.ioNamePtr = s;		pb.dirInfo.ioVRefNum = spec->vRefNum;		pb.dirInfo.ioDrDirID = spec->parID;		pb.dirInfo.ioFDirIndex = 0;		result = PBGetCatInfoSync(&pb);		// Allow file/directory name at end of path to not exist.		realResult = result;		if ( (result == noErr) || (result == fnfErr) )		{    		p2cstrcpy(tmpStr, s);			/* if the object is a directory, append a slash so full pathname ends with slash */			if ( (result == noErr) && (pb.hFileInfo.ioFlAttrib & kioFlAttribDirMask) != 0 )			{    		    snprintf(outFullPath, bufferSize, "%s/", tmpStr);			} 			else 			{    		    snprintf(outFullPath, bufferSize, "%s", tmpStr);			}    		outFullPath[bufferSize-1] = 0;    		    		// clear file not found so we can get the path		    if (result == fnfErr) {		        result = noErr;		    }						if ( result == noErr )			{				/* Get the ancestor directory names */				pb.dirInfo.ioNamePtr = (StringPtr)s;				pb.dirInfo.ioVRefNum = spec->vRefNum;				pb.dirInfo.ioDrParID = spec->parID;				do	/* loop until we have an error or find the root directory */				{					pb.dirInfo.ioFDirIndex = -1;					pb.dirInfo.ioDrDirID = pb.dirInfo.ioDrParID;					result = PBGetCatInfoSync(&pb);					if ( result == noErr )					{						/* Preprend next level up directory name */					    strncpy(workBuffer, outFullPath, bufferSize);    		            p2cstrcpy(tmpStr, s);    		            if (pb.dirInfo.ioDrDirID == fsRtDirID)     		            {    		                /* root directory, prefix with slash to indicate root path */                        #ifdef __MACH__                            /* if we are building for MACH-O, it's because we are on Mac OS X using some                               Carbon APIs. In that case the fsRtDirID doesn't really point to /,                                but rather to /Volumes/                            */    		                snprintf(outFullPath, bufferSize, "/Volumes/%s/%s", tmpStr, workBuffer);                        #else    		                snprintf(outFullPath, bufferSize, "/%s/%s", tmpStr, workBuffer);                        #endif // !__MACH__						}						else						{    		                snprintf(outFullPath, bufferSize, "%s/%s", tmpStr, workBuffer);						}					}				} while ( (result == noErr) && (pb.dirInfo.ioDrDirID != fsRtDirID) );			}		}	}		if ( result == noErr ) 	{		result = realResult;	// return realResult in case it was fnfErr	}	free(workBuffer);		return ( result );}/*****************************************************************************/OSErr FSpMakeFSSpecFromPosixFullPath(const char *fullPath, FSSpec &spec){    // need to convert from Posix to Native    char* nativePath = (char*) malloc(strlen(fullPath)+2);        if (fullPath[0] == '/') {    #ifdef __MACH__        /* if we are building for MACH-O, it's because we are on Mac OS X using some           Carbon APIs. In that the Posix Full Path might start with /Volumes/{VolumeName}/           and we need to change it to {VolumeName}:        */        if (strncmp(fullPath, "/Volumes/", 9) == 0) {            strcpy(nativePath, &fullPath[9]);        } else {            // this probably won't work since there won't be a volume name            strcpy(nativePath, &fullPath[1]);        }    #else        // full path, must remove leading slash        strcpy(nativePath, &fullPath[1]);    #endif // !__MACH__    } else {        // relative path, must start with colon        nativePath[0] = ':';        strcpy(&nativePath[1], fullPath);    }    char * p;    p = strchr(nativePath, '/');     while (p) {        *p = ':';        p = strchr(p+1, '/');    }	OSErr err = FSpMakeFSSpecFromNativeFullPath(nativePath, spec);	free(nativePath);	return err;}/*****************************************************************************/OSErr FSpMakeFSSpecFromNativeFullPath(const char *fullPath, FSSpec &spec){	AliasHandle	alias;	OSErr		result;	Boolean		wasChanged;	Str32		nullString;		/* Create a minimal alias from the full pathname */	nullString[0] = 0;	/* null string to indicate no zone or server name */	result = NewAliasMinimalFromFullPath(strlen(fullPath), fullPath, nullString, nullString, &alias);	if ( result == noErr )	{		/* Let the Alias Manager resolve the alias. */		result = ResolveAlias(NULL, alias, &spec, &wasChanged);				/* work around Alias Mgr sloppy volume matching bug */		if ( spec.vRefNum == 0 )		{			/* invalidate wrong FSSpec */			spec.parID = 0;			spec.name[0] =  0;			result = nsvErr;		}		DisposeHandle((Handle)alias);	/* Free up memory used */	}	return ( result );}/*****************************************************************************/#ifdef __cplusplus}#endif#endif // COMPILE_FOR_CARBON || __CARBON__