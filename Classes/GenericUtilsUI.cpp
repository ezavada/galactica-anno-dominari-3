// GenericUtils.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 9/5/96 ERZ

#include "pdg/sys/platform.h"
#include "DebugMacros.h"
#include "GenericUtilsUI.h"
#include <new>
#include <UMemoryMgr.h>
#include <LCommander.h>
#include <PP_KeyCodes.h>


#define MUL_F_PIN(a, b, ftemp) {			\
	ftemp = a;								\
	ftemp = ftemp * b;						\
	a = (ftemp > 0xffff) ? 0xffff : ftemp; }	\

#define MUL_S_PIN(a, b, temp) {				\
	temp = a;								\
	temp = temp * (unsigned) b;				\
	a = (temp > 0xffff) ? 0xffff : temp; }	\
	
void BrightenRGBColor(RGBColor& c, float b) {
	register float ftemp;
	MUL_F_PIN(c.red, b, ftemp);
	MUL_F_PIN(c.green, b, ftemp);
	MUL_F_PIN(c.blue, b, ftemp);
}

void BrightenRGBColor(RGBColor& c, short b) {
	register unsigned long temp;
	MUL_S_PIN(c.red, b, temp);
	MUL_S_PIN(c.green, b, temp);
	MUL_S_PIN(c.blue, b, temp);
}

void ScaleRect(Rect *ioRect, float scale) {
	float dim = ioRect->right * scale;
	ioRect->right = dim;
	dim = ioRect->left * scale;
	ioRect->left = dim;
	dim = ioRect->top * scale;
	ioRect->top = dim;
	dim = ioRect->bottom * scale;
	ioRect->bottom = dim;
}

#pragma mark-

// treats 0 as first character
SInt16
TEFind(TEHandle inTEH, const char* inFindText, SInt16 inLength, SInt16 inStartPos) {
	Handle h = ::TEGetText(inTEH);
	StHandleLocker lock(h);
	char* p = *h;
	SInt16	index = std::string::npos;					// Last possible index depends on									
	SInt16	lastChance = (**inTEH).teLength - inLength;	// relative lengths of the strings
	for (SInt16 i = inStartPos; i <= lastChance; i++) {	// Search forward from starting pos
		if (::CompareText(p + i, inFindText, inLength, inLength, NULL) == 0) {
			index = i;
			break;
		}
	}
	return index;
}

// treats 0 as first character
void
TEReplace(TEHandle inTEH, SInt16 inStartPos, SInt16 inCount, const char* inPtr, SInt16 inLength) {
	::TESetSelect(inStartPos, inStartPos+inCount, inTEH);
	::TEDelete(inTEH);
	::TEInsert(inPtr, inLength, inTEH);
}

std::string 
TEGetTextString(TEHandle inTEH) {
	Handle h = ::TEGetText(inTEH);
	StHandleLocker lock(h);
	char* p = *h;
	std::string resultStr(p);
	return resultStr;
}

#pragma mark-


bool 
IsOptionKeyDown() {
	KeyMapByteArray km;
	::GetKeys(*((KeyMap*)&km));
	return ((km[7] & 4) == 4);
}

bool 
IsShiftKeyDown() {
	KeyMapByteArray km;
	::GetKeys(*((KeyMap*)&km));
	return ((km[7] & 2) == 2) || ((km[7] & 1) == 1);
}

bool 
IsCmdKeyDown() {
	KeyMapByteArray km;
	::GetKeys(*((KeyMap*)&km));
	return ((km[6] & 128) == 128);
}

bool 
IsControlKeyDown() {
	KeyMapByteArray km;
	::GetKeys(*((KeyMap*)&km));
	return ((km[7] & 8) == 8);
}

int 
GetKey() {
	unsigned int i, j;
	KeyMapByteArray km;
	int lastKey;
	bool uppercase;
	bool option;
	unsigned char shiftCheck;
	lastKey = -1;
	::GetKeys(*((KeyMap*)&km));
	
	shiftCheck = km[7];
	option = ((shiftCheck & 4) == 4);
	uppercase = ((shiftCheck & 2) == 2) || ((shiftCheck & 1) == 1);
	for (j = 0; j < 16; j++ ) {
		if (km[j]) {
			for (i = 0; i < 8; i++) {
				if ( (km[j] >> i) & 0x01 ) {
					lastKey = j*8 + i;
					if (lastKey < 0x7F ) {	// don't return 0x7F (this sometimes sticks)
						if ( (lastKey < 55) || (lastKey > 59) )	{ //ignore modifier keys
							if (uppercase) lastKey |= 0x80;
							if (option) lastKey |= 0x100;
							return(lastKey);
						}
					}
				}
			}
		}
	}
	return(-1);
}

#ifdef __cplusplus	// do we want to include C++ utility classes?

//#include <LAnimateCursor.h>
//extern LAnimateCursor gBusyCursor;

// user can hit <tab> to end delay and continue, or <cmd><.> to abort
bool
UDelay::UserAbortableDelayUntil(UInt32 inEndTick, UDelay* inIdler) {
	bool bAborted = MacAPI::CheckEventQueueForUserCancel();
	if (!bAborted) {
		EventRecord macEvent;		// check for user Next (Tab) or Skip (cmd-.)
		UInt32 currTick;
		while ( (currTick = ::TickCount()) < inEndTick) {
			if (::WaitNextEvent(keyDownMask, &macEvent, 1, nil)) {
				if (UKeyFilters::IsCmdPeriod(macEvent)) {	// cmd-. means abort
					bAborted = true;
					break;
				} else if ( (macEvent.message & charCodeMask) == char_Tab) {
					break;						// Tab means halt this delay and continue
				}
			}
			if (inIdler) {		// give the idler a chance to do something if necessary
				bAborted = inIdler->DoIdle(currTick);
				if (bAborted) {
					break;
				}
			}
		}
	}
	return bAborted;
}

void 
UControlCommandListener::ListenToMessage(MessageT inMessage, void *ioParam) {
    SInt32 value = *static_cast<SInt32*>(ioParam);
    if (value) {
        CommandT cmd = mCommandBase + static_cast<CommandT>(inMessage);
        mCommander->ObeyCommand(cmd);
    }
}

#endif // __cplusplus

#pragma mark-


EKeyStatus
IPAddressField(
	TEHandle		inMacTEH,
	UInt16			inKeyCode,
	UInt16&			ioCharCode,
	EventModifiers	inModifiers)
{
	EKeyStatus	status = keyStatus_Input;
	
	if ( ioCharCode != char_Period ) {		//   dots are okay
		 
		status = UKeyFilters::AlphaNumericField(inMacTEH, inKeyCode, ioCharCode, inModifiers);
	}
	
	return status;
}

EKeyStatus
CreditCardField(
	TEHandle		inMacTEH,
	UInt16			inKeyCode,
	UInt16&			ioCharCode,
	EventModifiers	inModifiers)
{
	EKeyStatus	status = keyStatus_Input;
	
	if ( ioCharCode != char_Dash ) {		//   dashes okay
		 
		status = UKeyFilters::IntegerField(inMacTEH, inKeyCode, ioCharCode, inModifiers);
	}
	
	return status;
}

EKeyStatus
PhoneNumberField(
	TEHandle		inMacTEH,
	UInt16			inKeyCode,
	UInt16&			ioCharCode,
	EventModifiers	inModifiers)
{
	EKeyStatus	status = keyStatus_Input;
	
	if ( (ioCharCode != char_MinusSign) && (ioCharCode != char_Space) &&
		(ioCharCode != '(') && (ioCharCode != ')')  ) {		//   dash, () and space okay
		status = UKeyFilters::IntegerField(inMacTEH, inKeyCode, ioCharCode, inModifiers);
	}
	
	return status;
}


