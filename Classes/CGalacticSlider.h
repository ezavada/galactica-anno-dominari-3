// Galactica Slider.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#ifndef GALACTICA_SLIDER_H_INCLUDED
#define GALACTICA_SLIDER_H_INCLUDED

#include "Slider.h"
#include "LListener.h"
#include "GalacticaConsts.h"

#include <LControl.h>

class SettingT;

// these must be packed structures
#ifndef PACKED_STRUCT
#define PACKED_STRUCT
#if defined( __MWERKS__ )
    #pragma options align=mac68k
#elif defined( _MSC_VER )
    #pragma pack(push, 2)
#elif defined( __GNUC__ )
	#undef PACKED_STRUCT
	#define PACKED_STRUCT __attribute__((__packed__))
#else
	#pragma pack()
#endif
#endif

typedef struct SliderInfoT {
	Slider*	theSlider;
	int		lastValue;
} SliderInfoT;

typedef struct tGSliderRec	{
	long		FillPos;
	long		FillInset;
	RGBColor	FillColor;
	long		Bar1Pos;
	short		Bar1Size;
	RGBColor	Bar1Color;
	long		Bar2Pos;
	short		Bar2Size;
	RGBColor	Bar2Color;
} PACKED_STRUCT tGSliderRec;

#if defined( __MWERKS__ )
    #pragma options align=reset
#elif defined( _MSC_VER )
    #pragma pack(pop)
#endif

class CGalacticSlider : public HorzSlider, public LListener {
public:
	enum { class_ID = 'GSld' };

	CGalacticSlider();
	CGalacticSlider(LStream *inStream);
	CGalacticSlider(ResIDT BasePictRes, ResIDT SliderPictRes);
    CGalacticSlider(ResIDT BasePictRes, ResIDT SliderPictRes, ResIDT SlideSelectPictRes);
	~CGalacticSlider();
	
	virtual void	FinishCreateSelf();
	virtual void 	BroadcastValueMessage();
	virtual void	ListenToMessage(MessageT inMessage, void *ioParam);
	
	virtual void	DrawSelf();
	void			SetBarOne(long Pos, short BarSize, RGBColor Color);
	void			SetBarTwo(long Pos, short BarSize, RGBColor Color);
	void			SetFill(RGBColor Color, long Inset, long Pos);
	void			SetBarOnePos(long Pos);
	void			SetBarTwoPos(long Pos);
	void			SetFillPos(long Pos);
	void			AttachToSetting(SettingT* inSetting);   // keeps pointer for changes
	void			UpdateFromSetting(SettingT& inSetting); // doesn't keep pointer for changes
	void			DetachSetting() {mSettings = nil;}
	SettingT*		GetSettingsPtr() {return mSettings;}
	virtual void 	SetSliderValue(long theValue);
	virtual long	GetSliderValue(void);
	void			SetLockToggleButton(LControl* inLocker) {mLocker = inLocker;}
	virtual void	SetSliderLock(Boolean inLock);
	
protected:
	long		mFillPos;
	long		mFillInset;
	RGBColor	mFillColor;

	long		mBar1Pos;
	short		mBar1Size;
	RGBColor	mBar1Color;

	long		mBar2Pos;
	short		mBar2Size;
	RGBColor	mBar2Color;

	SettingT*	mSettings;
	LControl*	mLocker;
};


class CSliderLinker : public LListener {
public:
		CSliderLinker(LListener *inRecipient = nil);	// recipient is notified 1st time a slider is changed
	void				AddSlider(Slider *aSlider);
	Boolean				BeginLinking();	//returns false if it deletes itself
	virtual void		ListenToMessage(MessageT inMessage, void *ioParam);
protected:
	SliderInfoT		mSliderList[MAX_LINKED_SLIDERS];
	int				mNumSliders;
//	Boolean			mSentChange;
	LListener*		mRecipient;
};

#endif // GALACTICA_SLIDER_H_INCLUDED


