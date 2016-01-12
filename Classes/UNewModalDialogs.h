// ===========================================================================
//	UNewModalDialogs.h		   ©1995-1996 Metrowerks Inc. All rights reserved.
// portions copyright ©1997 Sacred Tree Software, inc.
// ===========================================================================

#ifndef _H_UNewModalDialogs
#define _H_UNewModalDialogs
#pragma once

#if defined(__CFM68K__) && !defined(__USING_STATIC_LIBS__)
	#pragma import on
#endif

#include <UModalDialogs.h>

class	StNewDialogHandler : public StDialogHandler {
public:
						StNewDialogHandler(ResIDT inDialogResID, LCommander *inSuper);
	virtual				~StNewDialogHandler();

	virtual void	AdjustCursor(const EventRecord &inMacEvent); // overridden from LEventDispatcher
protected:
//	static LPane*	sLastHelpLine;			// Cached pane for help line display
//	static UInt32	sLastMouseMoveTick;		// when did the mouse last move?
//	static bool		sMouseLingerActive;		// is a balloon being shown because of a lingering mouse?
	Boolean			mHasBalloonHelp;		// Flag for Gestalt
};


#if defined(__CFM68K__) && !defined(__USING_STATIC_LIBS__)
	#pragma import reset
#endif

#endif _H_UNewModalDialogs


