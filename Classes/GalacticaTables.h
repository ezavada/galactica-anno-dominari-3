// Galactica Tables.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// classes to implement specific kinds of tables needed for the game Galacticaª
//
// written 5/9/96, Ed Zavada
// modified 9/19/97, ERZ, added CFleetTable class, made CShipTable interact with it

#pragma once

#include "CTextTable.h"

class GalacticaDoc;

class CGalacticaTable : public CNewTable {
public:
	CGalacticaTable(LStream *inStream);

	virtual void	HiliteCell(const TableCellT &inCell);
	virtual void	UnhiliteCell(const TableCellT &inCell);
	virtual void	ActivateSelf();
	virtual void	DeactivateSelf();
protected:
	long			mCellDataSize;
	GalacticaDoc*	mGame;
	RGBColor		mBackColor;
}; 



class CWaypointTable : public CGalacticaTable {
public:
	enum { class_ID = 'WTab' };

	CWaypointTable(LStream *inStream);

	void			SetActiveWaypointNum(int inRowNum);
	int				GetActiveWaypointNum() {return mActiveWPNum;}
	virtual void 	DrawCell(const TableCellT &inCell);	
protected:
	int	mActiveWPNum;
};



class CShipTable : public CGalacticaTable {
public:
	enum { class_ID = 'STab' };

	CShipTable(LStream *inStream);

	virtual void	DrawSelf();
	virtual void 	DrawCell(const TableCellT &inCell);
	virtual void	GetCellData(const TableCellT &inCell, void *outData);
	virtual void	SetCellData(const TableCellT &inCell, void *inData);
	void			SetActiveFleetNum(int inNum) {mActiveFleetNum = inNum;}
	int				GetActiveFleetNum() {return mActiveFleetNum;}
protected:
	int mActiveFleetNum;
}; 


class CFleetTable : public CShipTable {
public:
	enum { class_ID = 'FTab' };

	CFleetTable(LStream *inStream);

	virtual void	DrawSelf();
	virtual void	SelectCell(const TableCellT &inCell);
protected:
	PaneIDT			mShipsTableID;
	CShipTable*		mShipsTable;
	virtual void	FinishCreateSelf();
}; 


class CEventTable : public CGalacticaTable {
public:
	enum { class_ID = 'ETab' };

	CEventTable(LStream *inStream);

	virtual void 	DrawCell(const TableCellT &inCell);
}; 

