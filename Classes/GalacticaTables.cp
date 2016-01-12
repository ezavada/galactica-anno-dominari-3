// GalacticaTables.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// classes to implement specific kinds of tables needed for the game Galacticaª
//
// written 5/9/96, Ed Zavada

#include "GalacticaTables.h"
#include "GalacticaUtils.h"
#include "Galactica.h"
#include "CHelpAttach.h"

//#include "CShip.h"
#include "CFleet.h"
#include "CMessage.h"
#include "GalacticaDoc.h"

#include <UDrawingUtils.h>

// *********** GALACTICA TABLE ************


CGalacticaTable::CGalacticaTable(LStream *inStream) 
: CNewTable(inStream) {
	mGame = GalacticaDoc::GetStreamingDoc();
 	short helpIndex = mUserCon;
 	if (helpIndex < 0) {
 		helpIndex = -helpIndex;
 	}
	CHelpAttach* theBalloonHelp = new CHelpAttach(STRx_StarmapHelp, helpIndex);
	AddAttachment(theBalloonHelp);
	mBackColor.red = mBackColor.green = mBackColor.blue = 0x4444;
}

void
CGalacticaTable::HiliteCell(const TableCellT&) {
	Rect	cellFrame;
	if (IsValidCell(mSelectedCell) &&
		FetchLocalCellFrame(mSelectedCell, cellFrame)) {
		if (CNewTable::sActiveTable == this) {
			SetRGBForeColor(0, 0xffff, 0);	// bright green if this is the active table
		} else {
			SetRGBForeColor(0, 0x7777, 0);	// medium green if inactive	
		}
		::PenMode(srcCopy);
		::MacFrameRect(&cellFrame);
	}
}

void
CGalacticaTable::UnhiliteCell(const TableCellT&) {
	Rect	cellFrame;
	if (IsValidCell(mSelectedCell) &&
		FetchLocalCellFrame(mSelectedCell, cellFrame)) {
		::RGBForeColor(&mBackColor);
		::PenMode(srcCopy);
		::MacFrameRect(&cellFrame);	// erase the green frame
	}
}

void
CGalacticaTable::ActivateSelf() {	// this keeps the standard activate/deactivate
}									// highlighting from being drawn

void
CGalacticaTable::DeactivateSelf() {	// this keeps the standard activate/deactivate
}									// highlighting from being drawn


// *********** WAYPOINT TABLE ************
#pragma mark -


CWaypointTable::CWaypointTable(LStream *inStream) 
: CGalacticaTable(inStream) {
	mActiveWPNum = 0;
}

void
CWaypointTable::SetActiveWaypointNum(int inRowNum) {
	if (mActiveWPNum != inRowNum) {
		// ¥¥¥ replace the Refresh() call with something that redraws the old and new active 
		//      waypoints' cells
		Refresh();
		mActiveWPNum = inRowNum;
	}
}

void
CWaypointTable::DrawCell(const TableCellT &inCell) {
	Rect	cellFrame;
	if (FetchLocalCellFrame(inCell, cellFrame)) {
		long	remainingTurnDist = 0;
		LStr255 distStr;
		::TextMode(srcCopy);
		::PenMode(srcCopy);
		Waypoint wp;
		GetCellData(inCell, &wp);
		AGalacticThingy *it = wp;
		if (it) {
			::TextFont(0);
			::TextFace(0);
			::TextSize(12);
			::MoveTo(cellFrame.left + 2, cellFrame.bottom - 3);
			if (it->IsVisibleTo(mGame->GetMyPlayerNum())) {  // Fog of War
			    ::RGBForeColor(mGame->GetColor(it->GetOwner(), false));
			} else {
			    ::RGBForeColor(mGame->GetColor(0, false));
			}
			::DrawString("\p¥");
		}
		if (inCell.row == mActiveWPNum) {
			remainingTurnDist = wp.GetRemainingTurnDistance();
			if (remainingTurnDist) {
				distStr.Assign(remainingTurnDist);
				distStr.Append('/');
			}
			::TextFace(bold);
		} else {
			::TextFace(0);
		}
		::TextFont(gGalacticaFontNum);
		::TextSize(10);
		::ForeColor(whiteColor);
		std::string descStr;
		LStr255 s = wp.GetDescription(descStr);
		::MoveTo(cellFrame.left + 10, cellFrame.bottom -4);
		::DrawString(s);
		if (inCell.row < mActiveWPNum) {
		    s.Assign("--");
		} else {
		    s.Assign(wp.GetTurnDistance());	// draw the turns for this leg of the course
		}
		distStr.Append(s);
		int width = ::StringWidth(distStr) + 5;
		::MoveTo(cellFrame.right - width, cellFrame.bottom - 4);
		::DrawString(distStr);
	}
}


// *********** SHIP TABLE ************
#pragma mark-

CShipTable::CShipTable(LStream *inStream) 
: CGalacticaTable(inStream) {
	mActiveFleetNum = -1;
	mBackColor.red = mBackColor.green = mBackColor.blue = 0x1111;
}

void
CShipTable::DrawSelf() {
	Rect	frame;
	CalcLocalFrameRect(frame);
	SetRGBForeColor(0x1111,0x1111,0x1111);
	::PaintRect(&frame);
//	::InsetRect(&frame, 1, 1);
//	SetRGBForeColor(0x9999,0xCCCC,0xFFFF);
//	FrameRect(&frame);
	CGalacticaTable::DrawSelf();
}


void
CShipTable::DrawCell(const TableCellT &inCell) {
	Rect	cellFrame;
	Boolean	drawNameInBlack = false;
	Boolean isFleet = false;
	Boolean isSatelliteFleet = false;
	CShip* aShip;
	LStr255 s;
	if (FetchLocalCellFrame(inCell, cellFrame)) {
		::PenMode(srcCopy);
		GetCellData(inCell, &aShip);
		if (!aShip) {	// reference couldn't be resolved
			::TextMode(srcOr);
			::TextFont(gGalacticaFontNum);
			::TextSize(10);
			::ForeColor(redColor);
			s = "**INVALID REF ";
//			s += aRef.GetThingyID();
			::MoveTo(cellFrame.left + 12, cellFrame.bottom - 4);
			::DrawString(s);
			DEBUG_OUT("User: ship table has an invalid ref ", DEBUG_ERROR);
			return;
		}
		if (aShip->SupportsContentsOfType(contents_Ships)) {
			isFleet = true;
			CFleet* fleet = dynamic_cast<CFleet*>(aShip);
			if (fleet && fleet->IsSatelliteFleet()) {
				isSatelliteFleet = true;
			}
		}
		if ((mActiveFleetNum > 0) && (inCell.row == mActiveFleetNum)) {
			if (isFleet) {
				SetRGBForeColor(0x9999, 0xCCCC, 0xFFFF);
				::PaintRect(&cellFrame);
				drawNameInBlack = true;
			} else {		// if the active fleet has been deleted we can clear
				mActiveFleetNum = 0;	 // the mActiveFleetNum variable
			}
		} else if (isFleet) {
			::ForeColor(blackColor); // non-active fleet, draw black background
			::PaintRect(&cellFrame);
		}
		::TextMode(srcOr);
		::TextFont(0);
		::TextFace(0);
		::RGBForeColor(mGame->GetColor(aShip->GetOwner(), false));
		if (isFleet) {
			::MoveTo(cellFrame.left + 1, cellFrame.bottom - 3);
			::TextSize(14);
			::DrawString("\p");
		} else {
			::MoveTo(cellFrame.left + 3, cellFrame.bottom - 3);
			::TextSize(12);
			::DrawString("\p¥");
		}
		::TextFont(gGalacticaFontNum);
		::TextSize(10);
		if (isSatelliteFleet) {
			::TextFace(italic);	// draw satellite fleets in italics
		} else if (aShip->CoursePlotted()) {	// draw ships that have courses laid in with bold
			::TextFace(bold);
		} else {
			::TextFace(0);
		}
		::MoveTo(cellFrame.left + 12, cellFrame.bottom - 4);
		if (aShip->WillCallIn()) {	// draw ships that are set to call in with yellow
			::ForeColor(yellowColor);
		} else if (aShip->IsToBeScrapped() ) {	// draw ships to be scraped in underlined bold red
			::ForeColor(redColor);
			::TextFace(underline | bold);
		} else if (isFleet) {
			if (drawNameInBlack) {
				::ForeColor(blackColor);	// if it was the selected fleet, name is black on a
			} else {						// light blue background, otherwise the name is drawn
				SetRGBForeColor(0x9999, 0xCCCC, 0xFFFF);	// light blue on a black backgroung
			}
		} else {
			::ForeColor(whiteColor);
		}
		aShip->GetDescriptor(s);
		if (isFleet && !isSatelliteFleet) {
			LStr255 s1;
			EWaypointType where = aShip->GetPosition();
			if (where == wp_Fleet) {
				s1.Assign(STRx_General, str_Group);
			} else {
				s1.Assign(STRx_General, str_Fleet_);
			}
			s += ' ';
			s += s1;
		}
		if (aShip->IsOnPatrol()) {
			LStr255 s2(STRx_General, str__on_patrol);
			s += s2;
		}
		::DrawString(s);
		char dataStr[256];
		std::string speedStr;
		GetSpeedString(aShip->GetSpeed(), speedStr);	// Speed
		std::sprintf(dataStr, "%ld/%s/%lu", 
		    aShip->GetTechLevel(), speedStr.c_str(), aShip->GetPower() );
		c2pstrcpy(s, dataStr);
		::TextFace(normal);		// v1.2b11d3, use most readable typestyle for numbers
		short width = ::StringWidth(s) + 5;
		::MoveTo(cellFrame.right - width, cellFrame.bottom - 4);
		::DrawString(s);
		Point p;
		p.h = cellFrame.left + 110;
		p.v = cellFrame.top + 6;
		DrawHealthBar(p, 30, 100 - aShip->GetDamagePercent() );
	}
}

// outData is an AGalacticThingy*;
void
CShipTable::GetCellData(const TableCellT &inCell, void *outData) {
	AGalacticThingy** resultP = (AGalacticThingy**)outData;
	if (mCellData != nil) {
		CThingyList* theList = (CThingyList*)mCellData;
		*resultP = theList->FetchThingyAt(FetchCellDataIndex(inCell));
	}
}

// inData is an AGalacticThingy;
void
CShipTable::SetCellData(const TableCellT &inCell, void *inData) {
	ThingyRef aRef = (AGalacticThingy*)inData;
	if (mCellData != nil) {
		CThingyList* theList = (CThingyList*)mCellData;
		((CThingyList*)mCellData)->AssignItemsAt(1, FetchCellDataIndex(inCell), aRef);
	}
}


// *********** FLEET TABLE ************

/*	This class assumes it has a sister list, of type CShipTable
	within the view heirachy. It thus will ask it's supervisor's
	supervisor (which must be have an ID of 1000) to find the
	that sister list when it is first created.
*/

#pragma mark-

CFleetTable::CFleetTable(LStream *inStream) 
: CShipTable(inStream) {
	*inStream >> mShipsTableID;
	mShipsTable = nil;
	mActiveFleetNum = 0;
	mBackColor.red = mBackColor.green = mBackColor.blue = 0x4444;
}

void
CFleetTable::FinishCreateSelf() {
	if (mShipsTableID) {	// scroller -> divider views -> CDividedView id 1000
		LView* superSuper = mSuperView->GetSuperView()->GetSuperView();
		ThrowIf_(superSuper->GetPaneID() != 1000);	// make sure everything is set up as we expect, see note above
		mShipsTable = (CShipTable*) superSuper->FindPaneByID(mShipsTableID);
	}
}

void
CFleetTable::DrawSelf() {
	CGalacticaTable::DrawSelf();	// skip the inherited CShipTable::DrawSelf() method
}									// so we don't get the black background

void
CFleetTable::SelectCell(const TableCellT &inCell) {
	if (mShipsTable && !EqualCell(inCell, mSelectedCell) && IsValidCell(inCell)) {
		CShip* aShip;
		GetCellData(inCell, &aShip);
		if (aShip) {
			if (aShip->SupportsContentsOfType(contents_Ships) ) {	// selected a fleet
				long oldFleet = mActiveFleetNum;
				if (((CFleet*)aShip)->IsSatelliteFleet()) {
					mActiveFleetNum = 0;	// can't transfer to satellite fleet
				} else {
					mActiveFleetNum = inCell.row;
				}
				FocusDraw();
				DrawCell(inCell);
				if (oldFleet) {
					TableCellT cell;
					cell.col = 1;
					cell.row = oldFleet;
					DrawCell(cell);
				}
			  #warning FIXME: this needs to change to a std::vector<>
				LArray* tmpArray = static_cast<LArray*>(aShip->GetContentsListForType(contents_Ships));
				if (tmpArray) { // v2.0.9d2, list could be null
				    mShipsTable->InstallArray(tmpArray, false);
				} else {
				    DEBUG_OUT(aShip << " returned nil contentsListForType(contents_Ships)", DEBUG_ERROR | DEBUG_CONTAINMENT);
				}
			}
		}
	}
	CNewTable::SelectCell(inCell);
}



// *********** EVENT TABLE ************
#pragma mark-

CEventTable::CEventTable(LStream *inStream) 
: CGalacticaTable(inStream) {
}

void
CEventTable::DrawCell(const TableCellT &inCell) {
	Rect cellFrame;
	AGalacticThingy* it = nil;
 	ThingMemInfoT info;
	if (FetchLocalCellFrame(inCell, cellFrame)) {
		GetCellData(inCell, &info);
		::TextMode(srcCopy);
		::TextFace(0);
		::TextSize(12);
		::TextFont(gGalacticaFontNum);
//		SetRGBForeColor(0x7777,0x7777,0x7777);
		if ((inCell.row != mSelectedCell.row) && info.info.flags & thingInfoFlag_Handled) {
			SetRGBForeColor(0xBBBB,0xBBBB,0xBBBB);
		} else {
			::ForeColor(whiteColor);
		}
		LStr255 s;
		LStr255 s2;
/*		::MoveTo(cellFrame.left, cellFrame.bottom-2);
		::LineTo(cellFrame.right, cellFrame.bottom-2);
		s.Assign(info.thingID);
		::MoveTo(cellFrame.left + 4, cellFrame.top + 12);
		::DrawString(s);
		s.Assign((void*)&info.thingType, 4);
		::MoveTo(cellFrame.left + 44, cellFrame.top + 12);
		::DrawString(s);			*/
		switch (info.info.action) {
			case action_Message:
            {
//				s.Assign("\pMessage");
				CMessage* msg = ValidateMessage(info.thing);	// we should always have a message here
				ASSERT(msg != NULL);					                    // should never have nil message here
				if (msg) {
					msg->GetDescriptor(s2);
				}
				break;
            }
			case action_Added:
				s.Assign("\pAdded");
				break;
			case action_Deleted:
				s.Assign("\pDeleted");
				break;
			case action_Updated:
				s.Assign("\pUpdated");
				break;
			default:
				s.Assign((SInt32)info.info.action);
				s += "\p ERROR";
				break;
		}
		// ¥¥¥MESSAGE¥¥¥
//		::MoveTo(cellFrame.left + 84, cellFrame.top + 12);
//		::DrawString(s);
//		cellFrame.top += 16;
		
		cellFrame.left += 4;
		cellFrame.right -= 4;
		UTextDrawing::DrawWithJustification((char*)&s2[1], s2.Length(), cellFrame, teForceLeft);
	}
}
