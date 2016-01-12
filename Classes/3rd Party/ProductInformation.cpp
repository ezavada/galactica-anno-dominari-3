
// ProductInformation.c
// Defines for Accessing Product Information
// Tuesday, April 30, 1996

// Copyright © 1994, Teknosys, Inc.


#include "pdg/sys/platform.h"
#include <MacTypes.h>
#include "GenericUtils.h"
#include "ProductInformation.h"

#if defined( POSIX_BUILD ) || ( !defined( PLATFORM_MACOS ) && !defined( PLATFORM_MACOSX ) )
  #include <PortableMacTypes.h>
#endif 

long GetVersionNumber(short versionID)
{
	long result = 0;

	void* hVersion = LoadResource(kVersionResType, versionID);

	if (hVersion) {
		result = BigEndian32_ToNative(*(long*)hVersion);

		UnloadResource(hVersion);
	}
	return result;
}


short GetVersionNumberString(short versionID, std::string& str)
{
	short error = noErr;
	void* hVersion = LoadResource(kVersionResType, versionID);

	if (hVersion) {
		char* ptr = ((char*)hVersion + sizeof(NumVersion) + sizeof(short));
		str.assign(ptr+1, ptr[0]); 

		UnloadResource(hVersion);
	} else {
		error = resNotFound;
	}

	return error;
}


short GetVersionMessage(short versionID, std::string& message)
{
	short error = noErr;
	void* hVersion = LoadResource(kVersionResType, versionID);

	if (hVersion)
	{
		char* ptr = ((char*)hVersion + sizeof(NumVersion) + sizeof(short));
		message.assign(ptr+1, ptr[0]); 

		UnloadResource(hVersion);
	} else {
		error = resNotFound;
    }
    
	return error;
}
