//	CKeyCmdAttach.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
//	Accepts a list of up to four characters, and a single command that will
//	be generated when any of those keys are pressed. Attach it to any 
//	LCommander subclass 

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include "GalacticaKeys.h"
#include "GalacticaPanels.h"
#include "GalacticaPanes.h"
#include <LAttachable.h>


// ---------------------------------------------------------------------------
//		¥ CGalacticaKeyAttach
// ---------------------------------------------------------------------------
//	Constructor

CGalacticaKeyAttach::CGalacticaKeyAttach(OSType inKeyChars, PaneIDT inID, GalacticaDoc* inGameDoc)
: CKeyCmdAttach(inKeyChars, (SInt32)inID) {
	mGameDoc = inGameDoc;
	ThrowIfNil_(mGameDoc);
}


// ---------------------------------------------------------------------------
//		¥ HandleKeyPress
// ---------------------------------------------------------------------------
//	Simulates a click on the appropriate button of its GameDoc's current panel.

void
CGalacticaKeyAttach::HandleKeyPress() { // inMessage
	CBoxView* panel = mGameDoc->GetActivePanel();
	if (panel)
		panel->SimulateButtonClick(mCommand);
}


#pragma mark-

// ---------------------------------------------------------------------------
//		¥ CStarMapKeyAttach
// ---------------------------------------------------------------------------
//	Constructor

CStarMapKeyAttach::CStarMapKeyAttach(OSType inKeyChars, SInt32 inCmd, GalacticaDoc* inGameDoc)
: CKeyCmdAttach(inKeyChars, inCmd) {
	mGameDoc = inGameDoc;
	ThrowIfNil_(mGameDoc);
}


// ---------------------------------------------------------------------------
//		¥ HandleKeyPress
// ---------------------------------------------------------------------------
//	Sends its command to as a message to its GameDoc's StarMap via ListenToMessage()

void		
CStarMapKeyAttach::HandleKeyPress() { // inMessage
	CStarMapView* theMap = mGameDoc->GetStarMap();
	theMap->ListenToMessage(mCommand, nil);
}

#pragma mark-

// ---------------------------------------------------------------------------
//		¥ CPanelKeyAttach
// ---------------------------------------------------------------------------
//	Constructor

CPanelKeyAttach::CPanelKeyAttach(OSType inKeyChars, SInt32 inCmd, GalacticaDoc* inGameDoc, PaneIDT inPanelID)
: CKeyCmdAttach(inKeyChars, inCmd) {
	mGameDoc = inGameDoc;
	mPanelID = inPanelID;
	ThrowIfNil_(mGameDoc);
}


// ---------------------------------------------------------------------------
//		¥ HandleKeyPress
// ---------------------------------------------------------------------------
//	Simulates a click on the appropriate button of its GameDoc's current panel.

void
CPanelKeyAttach::HandleKeyPress() { // inMessage
	CBoxView* panel = mGameDoc->GetActivePanel();
	if (panel) {
		if ((mPanelID == 0) || (panel->GetPaneID() == mPanelID)) {
			panel->ObeyPanelCommand(mCommand);
		}
	}
}


#pragma mark-

CButtonKeyAttach::CButtonKeyAttach(OSType inKeyChars, PaneIDT inID, GalacticaDoc* inGameDoc, PaneIDT inPanelID)
: CKeyCmdAttach(inKeyChars, 0) {
	mGameDoc = inGameDoc;
	mID = inID;
	mPanelID = inPanelID;
	ThrowIfNil_(mGameDoc);
}

void
CButtonKeyAttach::HandleKeyPress() { // inMessage
	LControl* button = nil;
	LView* bar = mGameDoc->GetCurrButtonBar();
	if (bar) {	// check current button bar 1st
		if ((mPanelID == 0) || (bar->GetPaneID() == mPanelID)) {
			button = (LControl*)bar->FindPaneByID(mID);
		}
	}
	if (!button) {
		LView* panel = mGameDoc->GetActivePanel();
		if (panel) {	// then check active control panel
			if ((mPanelID == 0) || (panel->GetPaneID() == mPanelID)) {
				button = (LControl*)panel->FindPaneByID(mID);
			}
		}
	}
	if (!button && !mPanelID)	// finally, check the entire window contents
		button = (LControl*)mGameDoc->GetWindow()->FindPaneByID(mID);
	if (button)
		button->SimulateHotSpotClick(1);
}


#pragma mark-

CListNavKeyAttach::CListNavKeyAttach() 
: LAttachment(msg_KeyPress, true) {
}

void
CListNavKeyAttach::ExecuteSelf(MessageT, void* ioParam) { //inMessage
	if (!CKeyCmdAttach::sKeyCmdsEnabled) {	// do nothing when keyboard shortcuts disabled
		return;
	}
	CNewTable* table = CNewTable::sActiveTable;
	if (!table)
		return;
	EventRecord* eventP = (EventRecord*)ioParam;
	char key = eventP->message & charCodeMask;
	int cellVOffset = 0, cellHOffset = 0;
	switch (key) {
		case 0x1e:
			cellVOffset = -1;
			break;
		case 0x1f:
			cellVOffset = 1;
			break;
		case 0x1d:
			cellHOffset = 1;
			break;
		case 0x1c:
			cellHOffset = -1;
			break;
	}
	if (cellVOffset || cellHOffset) {
		TableCellT cell = {0,0};
		TableIndexT rows, cols;
		table->GetTableSize(rows, cols);
		table->GetSelectedCell(cell);
		cell.row += cellVOffset;
		if (cell.row < 1)
			cell.row = rows;
		else if (cell.row > rows)
			cell.row = 1;
		if (cols == 1)			// just one column?
			cell.col = 1;
		else {
			cell.col += cellHOffset;
			if (cell.col < 1)
				cell.col = cols;
			else if (cell.col > cols)
				cell.col = 1;
		}
		table->SelectCell(cell);
	}
//	SetExecuteHost(false);
}
