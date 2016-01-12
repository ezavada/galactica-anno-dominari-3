//	Author:		James Whitehouse
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
//	modified: 3/26/96, ERZ
//  modified: 5/9/96, ERZ	Added CNewTable class and ability to install a new array

#pragma once

// INCLUDES
#include <LTable.h>
#include <LBroadcaster.h>

// DEFINES

#define message_CellSelected		2000
#define message_CellDoubleClicked	2001
#define message_TableActivated		2002
#define message_TableDeactivated	2003

// TYPEDEFS

typedef struct CellInfoT {
	TableCellT	cell;
	PaneIDT		tableID;
} CellInfoT, *CellInfoPtr;


class CNewTable : public LTable, public LBroadcaster {
public:
	enum { class_ID = 'NTab' };
	
	CNewTable(LStream *inStream);
	~CNewTable();
	
	virtual void	InstallArray(LArray *inArray, Boolean inOwnIt = true);
	virtual void	ArrayChanged();		// used to inform table that you changed the array
	virtual void	SelectCell(const TableCellT &inCell);
	virtual void	ClickCell(const TableCellT &inCell, const SMouseDownEvent& inMouseDown);
	virtual void	ClickSelf(const SMouseDownEvent &inMouseDown);
	virtual void	ResizeImageBy(SInt32 inWidthDelta, SInt32 inHeightDelta, Boolean inRefresh);
	virtual void	ApplyForeAndBackColors() const;
	LArray*	GetArray() {return mCellData;}
	static CNewTable*	sActiveTable;
protected:
	Boolean	mOwnArray;
}; 


class CTextTable : public CNewTable {
public:
	enum { class_ID = 'TTab' };
	
	CTextTable(LStream *inStream);
	~CTextTable();

	virtual void DrawCell(const TableCellT &inCell);
};


// GLOBALS
