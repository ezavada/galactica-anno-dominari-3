//	GalacticaKeys.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#pragma once

#include "CKeyCmdAttach.h"
#include "CTextTable.h"		// for CNavKeyListAttach
#include "GalacticaDoc.h"

class	CGalacticaKeyAttach : public CKeyCmdAttach {
public:
				CGalacticaKeyAttach(OSType inKeyChars, PaneIDT inID, GalacticaDoc* inGameDoc);
protected:
	GalacticaDoc* mGameDoc;

	virtual void	HandleKeyPress();
};


class	CStarMapKeyAttach : public CKeyCmdAttach {
public:
				CStarMapKeyAttach(OSType inKeyChars, SInt32 inCmd, GalacticaDoc* inGameDoc);
protected:
	GalacticaDoc* mGameDoc;

	virtual void	HandleKeyPress();
};

class	CPanelKeyAttach : public CKeyCmdAttach {
public:
				CPanelKeyAttach(OSType inKeyChars, SInt32 inCmd, GalacticaDoc* inGameDoc, PaneIDT inPanelID = 0);
protected:
	GalacticaDoc* mGameDoc;
	PaneIDT		mPanelID;

	virtual void	HandleKeyPress();
};

class	CButtonKeyAttach : public CKeyCmdAttach {
public:
				CButtonKeyAttach(OSType inKeyChars, PaneIDT inID, GalacticaDoc* inGameDoc, PaneIDT inPanelID = 0);
protected:
	GalacticaDoc* mGameDoc;
	PaneIDT		mID;
	PaneIDT		mPanelID;
	virtual void	HandleKeyPress();
};

class	CListNavKeyAttach : public LAttachment {	// attach to an event handler, not to list
public:
				CListNavKeyAttach();
protected:
	virtual void	ExecuteSelf(MessageT inMessage, void*ioParam);
};
