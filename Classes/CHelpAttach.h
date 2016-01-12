// Header for CHelpAttach class

#ifndef HELP_ATTACH_H_INCLUDED
#define HELP_ATTACH_H_INCLUDED

#pragma once

#include <LAttachment.h>
#include <LPane.h>

#if BALLOON_SUPPORT
  #include <Balloons.h>
#endif

const MessageT	msg_ShowHelp = 0x4000;
const MessageT	msg_ShowHelpLine = 0x4001;

enum EPaneState {
	pane_Inactive = 0,	// inactive, whether enabled or disabled
	pane_Disabled = 1,	// active && disabled
	pane_Enabled = 2	// active && enabled
};


class CHelpAttach : public LAttachment {
public:
			CHelpAttach(short strId, short index);
			
	static void 	SetHelpLinePane(LPane* inPane);
					
protected:
	virtual void	ExecuteSelf(MessageT inMessage,void *ioParam);		// Show help balloon
	virtual void	CalcPaneHotRect(LPane* inPane, Rect &outRect);		// where to show the balloon
  #if BALLOON_SUPPORT
	virtual	void	FillHMRecord(HMMessageRecord &theHelpMsg);			// Fill in the HMMessageRecord
  #endif // BALLOON_SUPPORT
	static LPane*	GetHelpLinePane() {return sHelpLinePane;}

	short			mStrId;						// Id for STR# rsrc
	short			mIndex;						// Index for balloon help text
	EPaneState		mPaneState;			// enabled, active, etc..
	static LPane*	sHelpLinePane;
};

#endif // HELP_ATTACH_H_INCLUDED


