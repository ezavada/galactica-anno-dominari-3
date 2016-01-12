//	Author:		James Whitehouse
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
//	modified: 3/26/96, ERZ	Subclassed from Broadcaster
//  modified: 5/9/96, ERZ	Added CNewTable class and ability to install a new array
//  modified: 9/19/97, ERZ	Added active table tracking


// INCLUDES
#include "GenericUtils.h"

#include "CTextTable.h"

#ifndef __TOOLUTILS__
#include <ToolUtils.h>
#endif
// DEFINES


// TYPEDEFS


// GLOBALS

CNewTable*	CNewTable::sActiveTable = nil;

// IMPLEMENTATION

#pragma mark -


CNewTable::CNewTable(LStream *inStream) 
:LTable(inStream), LBroadcaster() {
	mOwnArray = true;
}

CNewTable::~CNewTable() {
	if (!mOwnArray)
		mCellData = nil;	// delete operator called by ~LTable will do nothing with a nil object
	if (this == sActiveTable)
		sActiveTable = nil;
}

// installing a new array presumes the same data arrangement (number of columns) as the original
// the number of rows will be adjusted to suit the new data.
// the table will be redrawn;
void
CNewTable::InstallArray(LArray *inArray, Boolean inOwnIt) {
	if (mOwnArray) {
		delete mCellData;
	}
	mOwnArray = inOwnIt;
	mCellData = inArray;
	mSelectedCell.row = mSelectedCell.col = 0;
	if (inArray) {
		mRows = inArray->GetCount() / mCols;	// find out how many rows are in the new array
	} else {
		mRows = 0;
	}
	ScrollImageTo(0, 0, false);
	ResizeImageBy(0, -mImageSize.height + (mRows * mRowHeight), false);	// only change num rows
	Refresh();
}


// used to inform table that you changed the array. Generally it's better to use
// the RemoveRows(), RemoveCols(), InsertRows(), and InsertCols methods of the table,
// but if you MUST change the array manually, use ArrayChanged() so the table can clean
// up the mess.
// NOTE: you cannot delete columns indirectly and expect ArrayChanged() to fix it.
void
CNewTable::ArrayChanged() {
	long oldRows = mRows;
	if (mCellData) {
		mRows = mCellData->GetCount() / mCols;	// find out how many rows are now in the array
	} else {
		mRows = 0;
	}
	if (oldRows != (long)mRows) {		// something actually changed
		ResizeImageBy(0, -mImageSize.height + (mRows * mRowHeight), false);	// only change num rows
		DEBUG_OUT("CNewTable::ArrayChanged() detected new row cout = "<<mRows, DEBUG_IMPORTANT);
	}
}


void
CNewTable::SelectCell(const TableCellT &inCell) {
	if (!EqualCell(inCell, mSelectedCell)) {
		Boolean valid = IsValidCell(inCell);
		if ( (inCell.row > 0) && (inCell.row <= mRows) ) {
			SInt32 cellBottom = inCell.row * mRowHeight + mImageLocation.v;	// Cell in port coordinates
			SInt32 cellTop = cellBottom - mRowHeight;
			SInt32 frTop  = mFrameLocation.v;				// Frame in Port Coords
			SInt32 frBottom = frTop + mFrameSize.height;
			if (cellTop < frTop) {							// scroll minimal amount necessary to put
				ScrollImageBy(0, cellTop - frTop, true);	// entire cell in frame. Use instant refresh
			} else if (cellBottom > frBottom) {				// if cell too big for frame, cell top is
				ScrollImageBy(0, cellBottom - frBottom, true);	// shown at frame top
			}
		}
		LTable::SelectCell(inCell);
		CellInfoT cellInfo;
		if (valid) 
			cellInfo.cell = inCell;
		else {
			cellInfo.cell.row = 0;	// not a valid cell, broadcast that we selected
			cellInfo.cell.col = 0;	// cell (0,0)
		}
		cellInfo.tableID = mPaneID;
		BroadcastMessage(message_CellSelected, &cellInfo);
	}
}

void
CNewTable::ClickCell(const TableCellT &inCell, const SMouseDownEvent&) { // inMouseDown
	SelectCell(inCell);
	if (GetClickCount() == 2) {
		CellInfoT cellInfo;
		cellInfo.cell = inCell;
		cellInfo.tableID = mPaneID;
		BroadcastMessage(message_CellDoubleClicked, &cellInfo);
	}
}


void
CNewTable::ClickSelf(const SMouseDownEvent &inMouseDown) {
	TableCellT	hitCell;
	SPoint32	imagePt;
	
	LocalToImagePoint(inMouseDown.whereLocal, imagePt);
	FetchCellHitBy(imagePt, hitCell);

	if (sActiveTable != this) {	// еее ERZ addition, make this the active list.
		CNewTable* oldActive = sActiveTable;
		TableCellT selectedCell;
		sActiveTable = this;	
		if (oldActive) {
			oldActive->GetSelectedCell(selectedCell);
			oldActive->FocusDraw();
			oldActive->UnhiliteCell(selectedCell);		// this redraws correctly for new active state
			oldActive->HiliteCell(selectedCell);
			BroadcastMessage(message_TableDeactivated, oldActive);
		}
		GetSelectedCell(selectedCell);
		if ((selectedCell.row == hitCell.row) && (selectedCell.col == hitCell.col)) {
			FocusDraw();					// since cell click on is already selected, ClickCell()
			UnhiliteCell(selectedCell);		// won't re-highlight it. We need the highlighting refreshed
			HiliteCell(selectedCell);		// since we changed active lists, so force the redraw.
		}
	}
	
	if (IsValidCell(hitCell)) {
		ClickCell(hitCell, inMouseDown);
	} else {
		hitCell.row = 0;		// еее ERZ additions to standard LTable::ClickSelf()
		hitCell.col = 0;		// еее deselect all if clicked in an invalid cell
		ClickCell(hitCell, inMouseDown);
	}
	BroadcastMessage(message_TableActivated, this);
}

void
CNewTable::ResizeImageBy(SInt32 inWidthDelta, SInt32 inHeightDelta, Boolean inRefresh) {
	LTable::ResizeImageBy(inWidthDelta, inHeightDelta, inRefresh);
	// еее put code here to make the image size big enough to always fill the frame
}

void
CNewTable::ApplyForeAndBackColors() const
{
	LPane::ApplyForeAndBackColors();
}


#pragma mark -

CTextTable::CTextTable(LStream *inStream)
:CNewTable(inStream) {
}

CTextTable::~CTextTable() {
}

void 
CTextTable::DrawCell(const TableCellT	&inCell) {
	Rect	cellFrame;
	Str255	theData;
	if (FetchLocalCellFrame(inCell, cellFrame)) {
		::TextFont(kFontIDHelvetica);
		::TextSize(10);
		::TextMode(srcCopy);
		::TextFace(0);
		MoveTo(cellFrame.left + 4, cellFrame.bottom - 4);
		GetCellData(inCell, &theData);
		DrawString(theData);
	}
}
