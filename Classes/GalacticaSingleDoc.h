//	GalacticaSingleDoc.h		
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#pragma once

#include "GalacticaDoc.h"
#include "GalacticaProcessor.h"

#include <LFileStream.h>

class GalacticaSingleDoc : public GalacticaDoc, public GalacticaProcessor {
public:
    
    GalacticaSingleDoc(LCommander *inSuper);		// constructor
    virtual ~GalacticaSingleDoc();	// stub destructor

// UI methods
    virtual void        FindCommandStatus(CommandT inCommand, Boolean &outEnabled, 
                                Boolean &outUsesMark, UInt16 &outMark, Str255 outName);
    virtual Boolean     ObeyCommand(CommandT inCommand, void *ioParam);
	virtual void		UserChangedSettings();
  
// Initialization Methods
    OSErr               Open(FSSpec &inFileSpec);
	OSErr               DoNewGame(NewGameInfoT &inGameInfo, std::string& inPlayerName);
	void                InitNewGameData(NewGameInfoT &inGameInfo);
	virtual void        CreateNew(NewGameInfoT &inGameInfo);

// Informational Methods
    virtual bool        IsSinglePlayer() const {return true;} // single player means no separate host, not just one human
    virtual int         GetMyPlayerNum() const {return 1;}

// File Management Methods
	void                SpecifyFile(FSSpec &inTurnSpec);
    virtual void        DoSave();
	void                ReadSavedTurnFile();
	virtual void        DoRevert();
    void                WritePlayerInfoToFile(LFileStream& inStream);

// Turn Processing Methods
	virtual	void        StartIdling();
	virtual	void        StopIdling();
	virtual	void        SpendTime(const EventRecord &inMacEvent);
	virtual void        DoEndTurn();


protected:

public:
	UInt32				mLastThingyID;
};

