// GalacticSlider.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include "CGalacticSlider.h"
#include "GalacticaUtilsUI.h"
#include "LStream.h"
#include "CHelpAttach.h"

CGalacticSlider::CGalacticSlider()
{
	mBar1Pos = 0;
	mBar2Pos = 0;
	mSettings = nil;
	mLocker = nil;
}

CGalacticSlider::CGalacticSlider(LStream *inStream) 
: HorzSlider(inStream)
{
 	short helpIndex = mUserCon;
 	if (helpIndex < 0)
 		helpIndex = -helpIndex;
	CHelpAttach* theBalloonHelp = new CHelpAttach(STRx_StarmapHelp, helpIndex);
	AddAttachment(theBalloonHelp);
	tGSliderRec	SliderInfo;

	inStream->ReadData(&SliderInfo, sizeof(tGSliderRec));  // Endian conversion below

	mFillPos = EndianS32_BtoN(SliderInfo.FillPos);
	mFillInset = EndianS32_BtoN(SliderInfo.FillInset);
    mFillColor.red = EndianU16_BtoN(SliderInfo.FillColor.red);
    mFillColor.green = EndianU16_BtoN(SliderInfo.FillColor.green);
    mFillColor.blue = EndianU16_BtoN(SliderInfo.FillColor.blue);

	mBar1Pos = EndianS32_BtoN(SliderInfo.Bar1Pos);
	mBar1Size = EndianS16_BtoN(SliderInfo.Bar1Size);
    mBar1Color.red = EndianU16_BtoN(SliderInfo.Bar1Color.red);
    mBar1Color.green = EndianU16_BtoN(SliderInfo.Bar1Color.green);
    mBar1Color.blue = EndianU16_BtoN(SliderInfo.Bar1Color.blue);

	mBar2Pos = EndianS32_BtoN(SliderInfo.Bar2Pos);
	mBar2Size = EndianS16_BtoN(SliderInfo.Bar2Size);
    mBar2Color.red = EndianU16_BtoN(SliderInfo.Bar2Color.red);
    mBar2Color.green = EndianU16_BtoN(SliderInfo.Bar2Color.green);
    mBar2Color.blue = EndianU16_BtoN(SliderInfo.Bar2Color.blue);

	SetPicts(800, 801, 802);
	SetValueMessage(mPaneID);
	SetCursorFlag(false);
	
	mSettings = nil;
	mLocker = nil;
}


CGalacticSlider::CGalacticSlider(ResIDT BasePictRes, ResIDT SliderPictRes)
: HorzSlider(BasePictRes, SliderPictRes)
{
	mBar1Pos = 0;
	mBar2Pos = 0;
	mFillPos = 0;
	mSettings = nil;
	mLocker = nil;
}

CGalacticSlider::CGalacticSlider(	ResIDT BasePictRes,
				ResIDT SliderPictRes,
				ResIDT SlideSelectPictRes)
				: HorzSlider(BasePictRes, SliderPictRes, SlideSelectPictRes)

{
	mBar1Pos = 0;
	mBar2Pos = 0;
	mFillPos = 0;
	mSettings = nil;
	mLocker = nil;
}

CGalacticSlider::~CGalacticSlider()
{
}

void
CGalacticSlider::FinishCreateSelf() {
//	MakeSlider();
}


void CGalacticSlider::DrawSelf()
{
	long		total;
 	float		tempvalue;
	long		Height;
	Rect		r;
	
	if (!IsVisible()) {
	    return;
	}
	
	if (mBaseGWorld && mWorkGWorld) {
		// copy base GWorld to temp GWorld

		mWorkGWorld->BeginDrawing();
		mBaseGWorld->CopyImage((GrafPtr)mWorkGWorld->GetMacGWorld(), mBasePictRect, srcCopy, 0);

		Height = mBasePictRect.bottom - mBasePictRect.top;
		total = (mMaxSliderValue - mMinSliderValue);
		
		if(mFillPos != 0) {
			RGBForeColor(&mFillColor);
			r = mBasePictRect;
			tempvalue = ((float)(mBaseLength) / (float)total) * (float)mFillPos;
			r.right = (short) tempvalue;
			::MacInsetRect(&r,0,mFillInset);
			r.left += 1;
			PaintRect(&r);
		}

		if(mBar1Pos != 0) {
			RGBForeColor(&mBar1Color);
			PenSize(mBar1Size, mBar1Size);
			tempvalue = ((float)(mBaseLength) / (float)total) * (float)mBar1Pos;
//		 	tempvalue += mMinSliderValue;
			MoveTo(mBasePictRect.left + (short)tempvalue, mBasePictRect.top);
			Line(0,Height);
		}
		
		if(mBar2Pos != 0) {
			::RGBForeColor(&mBar2Color);
			::PenSize(mBar2Size, mBar2Size);
			tempvalue = ((float)(mBaseLength) / (float)total) * (float)mBar2Pos;
//		 	tempvalue += mMinSliderValue;
			::MoveTo(mBasePictRect.left + (short)tempvalue, mBasePictRect.top);
			::Line(0, Height);
		}

        bool drawSliderBar = true;
		if (mSelected) {
			SetRGBForeColor(0xFFFF, 0xFFFF, 0x2222);
			::PenSize(3, 1);
		} else if (mEnabled == triState_On) {
			SetRGBForeColor(0xFFFF, 0xCCCC, 0);
			::PenSize(2, 1);
		} else {
//		    drawSliderBar = false;
			SetRGBForeColor(0xCCCC, 0x5555, 0);
			::PenSize(2, 1);
		}
		if (drawSliderBar) {
    		int pos = mBasePictRect.left+mSliderRect.left;
    		if (pos > mBasePictRect.right-2) {
    			pos = mBasePictRect.right-2;
    	    }
    		::MoveTo(pos, mBasePictRect.top);
    		::Line(0,Height);
        }
		::PenNormal();
		::ForeColor(blackColor);
		::BackColor(whiteColor);
		mWorkGWorld->EndDrawing();

	}

/*	
	if (mSelected && mSliderSelectGWorld)		// Is it selected and does a Select LGWorld exist?
	{
		if (mSliderSelectGWorld && mWorkGWorld) {
			// copy Select slider GWorld to temp GWorld
			mWorkGWorld->BeginDrawing();
			mSliderSelectGWorld->CopyImage((GrafPtr)mWorkGWorld->GetMacGWorld(), mSliderRect, srcCopy, 0);
			mWorkGWorld->EndDrawing();
		}
	} else {
		if (mSliderGWorld && mWorkGWorld) {
			// copy slider GWorld to temp GWorld
			mWorkGWorld->BeginDrawing();
			mSliderGWorld->CopyImage((GrafPtr)mWorkGWorld->GetMacGWorld(), mSliderRect, srcCopy, 0);
			mWorkGWorld->EndDrawing();
		}
	}
*/
	if (mWorkGWorld) {
		::PenNormal();
		::ForeColor(blackColor);
		::BackColor(whiteColor);
		mWorkGWorld->CopyImage(UQDGlobals::GetCurrentPort(), mBasePictRect, srcCopy, 0);// display it
      #if TARGET_API_MAC_CARBON
        ::QDFlushPortBuffer(UQDGlobals::GetCurrentPort(), nil);
      #endif
	} else {
		::MacFrameRect(&mBasePictRect);				// If all else fails, draw a box
	}
}

void CGalacticSlider::SetBarOne(long Pos, short BarSize, RGBColor Color)
{
	mBar1Pos = Pos;
	mBar1Size = BarSize;
	mBar1Color = Color;
}

void CGalacticSlider::SetBarTwo(long Pos, short BarSize, RGBColor Color)
{
	mBar2Pos = Pos;
	mBar2Size = BarSize;
	mBar2Color = Color;
}

void 
CGalacticSlider::SetFill(RGBColor Color, long Inset, long Pos) {
	mFillColor = Color;
	mFillInset = Inset;
	mFillPos = Pos;
}

void 
CGalacticSlider::SetBarOnePos(long Pos) {
	mBar1Pos = Pos;
}

void
CGalacticSlider::SetBarTwoPos(long Pos) {
	mBar2Pos = Pos;
}

void
CGalacticSlider::SetFillPos(long Pos) {
	mFillPos = Pos;
}

void
CGalacticSlider::BroadcastValueMessage() {
	if (mSettings) {
//		mSettings->desired = mValue;
		RGBColor color;
		int diff = mSettings->value - mValue;	// compare the desired (mValue) to the current
		if ( (diff > 8) || (diff < -2) ) {
			MakeRGBColor(color, 0xffff, 0, 0);
		} else {
			MakeRGBColor(color, 0, 0x6666, 0);
		}
		if (mEnabled == triState_Off) {
			color.red/=2;
			color.green/=2;
			color.blue/=2;
		}
		SetFill(color, 5, mSettings->value);
		FocusDraw();
		DrawSelf();
	}
	if (mValueMessage == msg_SliderMoved) {
		BroadcastMessage(msg_SliderMoved, (void *) this);
	} else {
		Slider::BroadcastValueMessage();
    }
}

// pointer to settings to be used, or null if no settings
void
CGalacticSlider::AttachToSetting(SettingT *inSetting) {
	mSettings = inSetting;
	if (inSetting) {
	    UpdateFromSetting(*inSetting);
	    mSettings = inSetting; // call just cleared the settings pointer, so update
	}
}

void			
CGalacticSlider::UpdateFromSetting(SettingT& inSetting) {
	StopBroadcasting();
	RGBColor color;
	MakeRGBColor(color, 0x6666, 0x6666, 0xffff);
	SetBarOne((long)inSetting.benefit + 1, 1, color);
	MakeRGBColor(color, 0xAAAA, 0xdddd, 0xffff);
	SetBarTwo(inSetting.economy, 1, color);
	SetFillPos(inSetting.value);
	SetSliderValue(inSetting.desired);
	SetSliderLock(inSetting.locked);
	if (mLocker) {
		StopListening();
		mLocker->SetValue(inSetting.locked);	// update the lock toggle to reflect our new state
		StartListening();
	}
	BroadcastValueMessage();	//this will set the appropriate fill color
	StartBroadcasting();
	mSettings = 0;
}

void 
CGalacticSlider::SetSliderValue(long theValue) {
	HorzSlider::SetSliderValue(theValue);
	if (mSettings)
		mSettings->desired = theValue;
}

// this method returns the value stored in the settings. This is slightly different than the
// value calculated by HorzSlider::GetSliderValue(), and is usually more accurate
// NOTE: this does not always reflect the current position of the user movable handle. If you want
// the slider value as reflected in the current handle position, call CalcSliderValue() instead
long 
CGalacticSlider::GetSliderValue(void) {
	if (mSettings) 
		return mSettings->desired;
	else
		return HorzSlider::GetSliderValue();
}

void 
CGalacticSlider::SetSliderLock(Boolean inLock) {
	mLocked = inLock;
	if (mSettings)
		mSettings->locked = inLock;
}

void
CGalacticSlider::ListenToMessage(MessageT inMessage, void *ioParam) {
	long value = *((long*) ioParam);
	if (inMessage == msg_AdjustSliderLock) {
		SetSliderLock(value);
	}
}


#pragma mark -

CSliderLinker::CSliderLinker(LListener *inRecipient) 
: LListener() {
	mNumSliders = 0;
	mRecipient = inRecipient;
	for (int i = 0; i < MAX_LINKED_SLIDERS; i++) {
		mSliderList[i].theSlider = nil;
	}
}

void
CSliderLinker::AddSlider(Slider *aSlider) {
	if (mNumSliders < MAX_LINKED_SLIDERS) {
		mSliderList[mNumSliders].theSlider = aSlider;
		mNumSliders++;
	}
}

Boolean
CSliderLinker::BeginLinking() {
	if (mNumSliders) {
		StopListening();
		int valuePer = 1000 / mNumSliders;
		for (int i = 0; i < mNumSliders; i++) {
			ThrowIfNil_(mSliderList[i].theSlider);
			mSliderList[i].lastValue = 0;//valuePer;
			mSliderList[i].theSlider->SetMinMax(0, 1000);
			mSliderList[i].theSlider->SetSliderValue(valuePer);
			mSliderList[i].theSlider->SetValueMessage(msg_SliderMoved);
			mSliderList[i].theSlider->AddListener(this);
		}
		StartListening();
		return true;
	} else {
		delete this;	// no sliders, delete the object
		return false;
	}
}

void
CSliderLinker::ListenToMessage(MessageT inMessage, void *ioParam) {
	int i;
	int firstUnlocked = -1;
	Slider *it;
	switch (inMessage) {

		case msg_BroadcasterDied:	// sent by a slider when it is being deleted
			if (mBroadcasters.GetCount() == 1)
				delete this;	// Last Broadcaster is dying. Nothing left in group, so
			break;				//   delete this Slider Linker
			
		case msg_SliderMoved:	// Slider's value changed, so we must update the linked
		{							// sliders to match
			StopListening();			
			Slider *theSlider = (Slider*)ioParam;
			long newValue = theSlider->CalcSliderValue();
			long oldValue = -1;
			int at = 0;
			int numSliders = 0;
			for (i = 0; i < mNumSliders; i++) {
				it = mSliderList[i].theSlider;
//				ThrowIfNil_(it);
				if (it == theSlider) {
					at = i;
					oldValue = mSliderList[i].lastValue;
					mSliderList[i].lastValue = newValue;
				} else if (!it->IsLocked()) {
					numSliders++;	//found a usable slider other than the one changed
				}
			}
			ThrowIf_(oldValue == -1);	// this means the slider that sent the message wasn't ours
			long toDistribute = newValue - oldValue;
			long diffPer = 0;
			if (numSliders) {
				diffPer = toDistribute / numSliders;
			}
			do {
				numSliders = 0;		// recount number of usable sliders in case it changes
				for (i = 0; i < mNumSliders; i++) {
					if (i == at) {
						continue;	// don't distribute to the one we are dragging
					}
					it = mSliderList[i].theSlider;
					if ( it->IsLocked() ) {
						continue;	// skip locked sliders
					}
					long min = it->GetMin();
					long max = it->GetMax();
					long lastValue = mSliderList[i].lastValue;
					if ((lastValue == min) && (diffPer > 0)) {
						continue;	//don't use the pegged values
					}
					if ((lastValue == max) && (diffPer < 0)) {
						continue;
					}
					if (firstUnlocked == -1) {
						firstUnlocked = i;
					}
					newValue = lastValue - diffPer;	// new value for the slider
					toDistribute -= diffPer;		// subtract the amount we just distributed
					numSliders++;					// presume this is still a movable slider
					if (newValue <= min) {			// did we try to go past the minimum?
						toDistribute += (min - newValue);	// add back the part we didn't distrib
						numSliders--;				// this slider can't be moved any further
						newValue = min;
						if (i == firstUnlocked) {	// couldn't distribute enough to this,
							firstUnlocked = -1;		// don't add back to it.
						}
					}
					mSliderList[i].lastValue = newValue;	// change the sliders
				}
				if (numSliders) {
					diffPer = toDistribute / numSliders;
				}
			} while (numSliders && diffPer);
			if (toDistribute) {	// was there a little bit left over??
				if (firstUnlocked != -1) {	// add to topmost unlocked slider, if available
					mSliderList[firstUnlocked].lastValue += toDistribute;
				} else {
					mSliderList[at].lastValue -= toDistribute;	// couldn't distrib this, take it back
				}
			}
			for (i = 0; i < mNumSliders; i++) {
				it = mSliderList[i].theSlider;
				it->SetSliderValue(mSliderList[i].lastValue);	// update all the sliders
			}
			if (mRecipient) {
				mRecipient->ListenToMessage(msg_SliderMoved, nil);	// inform of changes
			}
			StartListening();
			break;
		}
        	
		case msg_SetSlidersEven:
		{
        	StopListening();
//			ThrowIf_(mNumSliders == 0);
			int numSliders = mNumSliders;
			long toDistribute = 1000;
			for (i = 0; i < mNumSliders; i++) {
				it = mSliderList[i].theSlider;
//				ThrowIfNil_(it);
				if (it->IsLocked()) {
					numSliders--;// don't even out locked sliders
					toDistribute -= mSliderList[i].theSlider->GetSliderValue();
				} else if (firstUnlocked == -1) {
					firstUnlocked = i;	// record the first unlocked item
				}
			}
			if (numSliders) {
				int valuePer = toDistribute / numSliders;
				for (i = 0; i < mNumSliders; i++) {
					it = mSliderList[i].theSlider;
					if (it->IsLocked() ) {
						continue; // don't even out locked sliders
					}
					mSliderList[i].lastValue = valuePer;
					toDistribute -= valuePer;
				}
				if (toDistribute) {	// was there a little bit left over??
					mSliderList[firstUnlocked].lastValue += toDistribute;
				}
			}
			for (i = 0; i < mNumSliders; i++) {
				it = mSliderList[i].theSlider;
				it->SetSliderValue(mSliderList[i].lastValue);	// update all the sliders
			}
			if (mRecipient) {
				mRecipient->ListenToMessage(msg_SliderMoved, nil);	// inform of first change
			}
			StartListening();
			break;
		}
        	
		case msg_SlidersReset:
			for (i = 0; i < mNumSliders; i++) {	// and reload the last values, this is sent by the control panel when
				it = mSliderList[i].theSlider;	// a new object is being displayed and so the sliders are all changing
//				ThrowIfNil_(it);
				mSliderList[i].lastValue = it->GetSliderValue();
			}
	}
}
