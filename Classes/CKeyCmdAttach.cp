// ===========================================================================
//	CKeyCmdAttach.cp		©1996 Sacred Tree Software. All rights reserved.
// ===========================================================================
//
//	Accepts a list of up to four characters, and a single command that will
//	be generated when any of those keys are pressed. Attach it to any 
//	LCommander subclass 

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include "CKeyCmdAttach.h"
#include <LAttachable.h>
#include <LCommander.h>
#include <LEventDispatcher.h>


Boolean CKeyCmdAttach::sKeyCmdsEnabled = true;

// ---------------------------------------------------------------------------
//		¥ CKeyCmdAttach
// ---------------------------------------------------------------------------
//	Constructor

CKeyCmdAttach::CKeyCmdAttach(OSType inKeyChars, SInt32 inCmd) 
: LAttachment(msg_KeyPress, true) {
	mKeyChars = inKeyChars;
	mCommand = inCmd;
}



// ---------------------------------------------------------------------------
//		¥ ExecuteSelf
// ---------------------------------------------------------------------------
//	Checks to see if the key is one it handles, and if so calls HandleKeyPress()
//
//	ioParam = EventRecord* (KeyDown or AutoKey event)

void		
CKeyCmdAttach::ExecuteSelf(MessageT, void* ioParam) { // inMessage
	if (!CKeyCmdAttach::sKeyCmdsEnabled) {	// do nothing when keyboard shortcuts disabled
		return;
	}
	EventRecord* eventP = (EventRecord*)ioParam;
	char key = eventP->message & charCodeMask;
	if ( (key == (mKeyChars >> 24)) || (key == ((mKeyChars >> 16) & 0xff)) ||
		 (key == ((mKeyChars >> 8) & 0xff)) || (key == (mKeyChars & 0xff)) ) {
		 HandleKeyPress();
	}
}


// ---------------------------------------------------------------------------
//		¥ HandleKeyPress
// ---------------------------------------------------------------------------
//	Sends the command to the current target via ObeyCommand()

void		
CKeyCmdAttach::HandleKeyPress() {
	LCommander::GetTarget()->ObeyCommand(mCommand);
}


#pragma mark-
// ====================================================
// CBlockKeysAttachment
// ====================================================

// ---------------------------------------------------------------------------
//		¥ CBlockKeysAttachment
// ---------------------------------------------------------------------------
//	Constructor

CBlockKeysAttachment::CBlockKeysAttachment() 
: LAttachment(msg_KeyPress, false) {
}



// ---------------------------------------------------------------------------
//		¥ ExecuteSelf
// ---------------------------------------------------------------------------
//
//	ioParam = EventRecord* (KeyDown or AutoKey event)
void		
CBlockKeysAttachment::ExecuteSelf(MessageT, void*) { // inMessage  ioParam
//	EventRecord* eventP = (EventRecord*)ioParam;
}
