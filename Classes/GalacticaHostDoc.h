//	GalacticaHostDoc.h		
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved.
// ===========================================================================

#ifndef GALACTICA_HOST_DOC_H_INCLUDED
#define GALACTICA_HOST_DOC_H_INCLUDED

#include "GalacticaHost.h"
#include "GalacticaSharedUI.h"
#include <LSingleDoc.h>

class GalacticaHostDoc : virtual public GalacticaSharedUI, public GalacticaHost, public LSingleDoc {
public:
 						GalacticaHostDoc(LCommander *inSuper);		// constructor
						~GalacticaHostDoc();	// stub destructor

// Periodical methods
	virtual	void		SpendTime(const EventRecord &inMacEvent);

// Initialization methods
	virtual OSErr		Open(FSSpec &inDataSpec, FSSpec &inIndexSpec);
	OSErr               DoNewGame(NewGameInfoT &inGameInfo);
    OSErr               Open(FSSpec &inFileSpec);  // hack to read saved game files into host

// UI methods
	void				ShowHostWindow();
    void                AdjustWindowSize();	// adjust the window to match the state of the Full Screen item
	virtual void		MakeWindow();
	void				UpdateDisplay();
	RGBColor*           GetColor(int owner);
    SInt32              GetColorIndex(int inOwner);
    void                RecalcAndRedrawEverything();
    
	virtual void		SetStatusMessage(int inMessageNum, const char* inExtra);

// Command handling methods
	virtual Boolean		AttemptQuitSelf(SInt32 inSaveOption);
	virtual void		FindCommandStatus(CommandT inCommand,
							Boolean &outEnabled, Boolean &outUsesMark,
							UInt16 &outMark, Str255 outName);
    virtual Boolean     ObeyCommand(CommandT inCommand, void *ioParam);
	virtual void		DoEndTurn();


protected:
    void    LoadFromSavedTurnFile();

	LPane*		        mTurnNumDisplay;
	LPane*		        mNumHumansDisplay;
	LPane*		        mNumCompsDisplay;
	LPane*		        mNumThingysDisplay;
	LPane*		        mMessageDisplay;
	RGBColor		    mColorArray[kNumUniquePlayerColors];
};

#endif // GALACTICA_HOST_DOC_H_INCLUDED
