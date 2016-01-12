// GenericUtils.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 9/5/96 ERZ

#ifndef GENERIC_UTILS_UI_H_INCLUDED
#define GENERIC_UTILS_UI_H_INCLUDED

#include <UKeyFilters.h>
#include <string>


//=======================================================================

#define MakeRGBColor(c, r, g, b) \
	{c.red = r; \
	c.green = g; \
	c.blue = b;}

#define SetRGBForeColor(r, g, b) \
	{RGBColor _RGBC_; \
	_RGBC_.red = r; \
	_RGBC_.green = g; \
	_RGBC_.blue = b; \
	::RGBForeColor(&_RGBC_);}

#define SetRGBBackColor(r, g, b) \
	{RGBColor _RGBC_; \
	_RGBC_.red = r; \
	_RGBC_.green = g; \
	_RGBC_.blue = b; \
	::RGBBackColor(&_RGBC_);} 

#define SetRGBOpColor(r, g, b) \
	{RGBColor _RGBC_; \
	_RGBC_.red = r; \
	_RGBC_.green = g; \
	_RGBC_.blue = b; \
	::OpColor(&_RGBC_);}

void BrightenRGBColor(RGBColor& c, float b);
void BrightenRGBColor(RGBColor& c, short b);

void ScaleRect(Rect *ioRect, float scale);
#define ScalePoint(p, scale) 			\
	{float _zz_scale = scale * p.h;		\
	p.h = _zz_scale; 					\
	_zz_scale = scale * p.v;			\
	p.v = _zz_scale;}

// text edit utils
SInt16		TEFind(TEHandle inTEH, const char* inFindText, SInt16 inLength, SInt16 inStartPos = 0);
inline SInt16	TEFindString(TEHandle inTEH, std::string inStr, SInt16 inStartPos = 0) { 
									return TEFind(inTEH, inStr.c_str(), inStr.size(), inStartPos);}
void		TEReplace(TEHandle inTEH, SInt16 inStartPos, SInt16 inCount, const char* inPtr, SInt16 inLength);
std::string TEGetTextString(TEHandle inTEH);

// keyboard utils
bool		IsOptionKeyDown();
bool		IsShiftKeyDown();
bool		IsCmdKeyDown();
bool		IsControlKeyDown();
int			GetKey();


EKeyStatus IPAddressField(TEHandle inMacTEH, UInt16 inKeyCode, UInt16& ioCharCode,
							EventModifiers inModifiers);

EKeyStatus CreditCardField(TEHandle inMacTEH, UInt16 inKeyCode, UInt16& ioCharCode,
							EventModifiers inModifiers);

EKeyStatus PhoneNumberField(TEHandle inMacTEH, UInt16 inKeyCode, UInt16& ioCharCode,
							EventModifiers inModifiers);


#ifdef __cplusplus	// do we want to include C++ utility classes?

#include "LListener.h"

class LCommander;

class UDelay {
public:
	virtual ~UDelay() {}
	virtual bool DoIdle(UInt32 ) {return false;}	// param is currTick, returns true to abort
	static bool	 UserAbortableDelayUntil(UInt32 inEndTick, UDelay* inIdler = NULL);
};

class UControlCommandListener : public LListener {
public:
    UControlCommandListener(LCommander* theCommander, CommandT commandBase = 0) : mCommander(theCommander), mCommandBase(commandBase) {}
    virtual void ListenToMessage(MessageT inMessage, void *ioParam);
protected:
    LCommander* mCommander;
    CommandT  mCommandBase;
};

#endif // __cplusplus


#endif // GENERIC_UTILS_UI_H_INCLUDED
