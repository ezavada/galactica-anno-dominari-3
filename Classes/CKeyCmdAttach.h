// ===========================================================================
//	CKeyCmdAttach.h		  ©1996 Sacred Tree Software. All rights reserved.
// ===========================================================================

#pragma once

#include <LAttachment.h>

class	CKeyCmdAttach : public LAttachment {
public:
				CKeyCmdAttach(OSType inKeyChars, SInt32 inCmd);
	static void EnableCmdKeys() {sKeyCmdsEnabled = true;}
	static void DisableCmdKeys() {sKeyCmdsEnabled = false;}
	
	static Boolean sKeyCmdsEnabled;
protected:
	OSType	mKeyChars;
	SInt32	mCommand;

	virtual void	ExecuteSelf(MessageT inMessage, void*ioParam);
	virtual void	HandleKeyPress();
};

class CBlockKeysAttachment : public LAttachment {
public:
	CBlockKeysAttachment();
protected:
	virtual void	ExecuteSelf(MessageT inMessage, void*ioParam);
};


