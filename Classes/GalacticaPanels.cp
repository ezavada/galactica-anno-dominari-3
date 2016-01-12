// Galactica Panes.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/23/96 ERZ

/*
Overview of Main Button Bar and Control Panels:

A CBoxView implements the core framework for a visual panel that contains a group of controls.
BoxViews are designed to be loaded from a PPob. It will automatically link controls to itself
and link sliders to one another. It can send its own UserCon value upon initial display,
as if one of its buttons had be pressed -- useful for button bars that want to start with a
particular control-panel selected.

The various Button Bars are implemented directly with the CBoxView. The Control Panels
are all subclasses of CBoxView that are specifically adapted to load the data they need to 
display from the currently selected object.

As of version 1.2d4, each button bar and control panel is stored in it's own CBoxView PPob 
resource. The Starmap Window contains two LPPobViews, which load in the appropriate CBoxView
(as specified by a PPob ResID) as the different panels are needed. These panels are cached
in their fully created state by the LPPobView, so this is very efficient.

I. Clicking on a button within the main button bar does the following:
  (1) Sends the button's message to CBoxView::ListenToMessage()
  (2) calls GameDoc::ObeyCommand() with that message code
  (3) GameDoc tells the LPPobView to display the correct panel, by calling mPanel->SetPPob()
  	(a) 
  NOTE: if the message sent by the button is not in the range cmd_SetPanelFirst...cmd_SetPanelLast,
    the message will instead be sent through to its own ObeyPanelCommand() method.

II. Clicking on a button within a control panel:
  (1) Sends button's message to CBoxView::ListenToMessage()
  (2) calls own DoPanelButton() method
  NOTE: for this to work, the button must have a UserCon less than zero and the message
    must have a message of "b???", where ? is any character. Otherwise, the message will
    instead be sent through to its own ObeyPanelCommand() method.

III. Clicking on an item in the starmap:
  (1) AGalacticThingy::ClickSelf() calls AGalacticThingy::Select()
  (2) AGalacticThingy::Select() calls mGameDoc->SetSelectedThingy()
  (3) within SetSelectedThingy(), SwitchToButtonBar() is called, which:
  	(a) calls mButtonBar->SetPPob() with the appropriate PPob resource
  	(b) when the new button bar is shown, the default panel button is "pushed",
  		and the events described in (I) take place.
*/


#include "GalacticaPanels.h"
#include "CStar.h"
#include "Galactica.h"
#include "CGalacticSlider.h"
#include "GalacticaTables.h"
#include "CShip.h"
#include "CFleet.h"
#include "CResCaption.h"

#include <LStdControl.h>

#include "GalacticaTutorial.h"
#include "GalacticaDoc.h"
#include "GalacticaClient.h"

AGalacticThingy* CbxShPanel::sPrevThingy = NULL;	//tracks the previous thing shown so it can be
				// autoselected in ships lists

#ifdef DEBUG
static bool gAllowChangeAnything = false;
#endif


CBoxFrame::CBoxFrame(LStream *inStream)
: LPane(inStream) {
}

void
CBoxFrame::DrawSelf() {
	Rect	frame;
	CalcLocalFrameRect(frame);
	::PenNormal();
	SetRGBForeColor(0xCCCC,0xCCCC,0xCCCC);
//	SetRGBForeColor(0xBBBB,0xBBBB,0xBBBB);
	::MoveTo(frame.left, frame.bottom);
	::MacLineTo(frame.left, frame.top);
	::MacLineTo(frame.right, frame.top);
	::ForeColor(blackColor);
//	SetRGBForeColor(0x0000,0x0000,0x0000);
	::MacLineTo(frame.right, frame.bottom);
	::MacLineTo(frame.left, frame.bottom);
	::ForeColor(whiteColor);
	::MoveTo(frame.left,frame.top);
	::Line(0,0);
//	::FrameRect(&frame);
}

#pragma mark-


// **** Box VIEW ****

CBoxView::CBoxView(LStream *inStream)
: LView(inStream), LListener() {
    Bool8 sendUserConOnShow;
	*inStream >> sendUserConOnShow;
	*inStream >> mFirstSliderID;
	mSendUserConOnShow = sendUserConOnShow;
	InitBoxView();
}

void
CBoxView::InitBoxView(void) {
	mGame = GalacticaDoc::GetStreamingDoc();
	mLinker = NULL;
}

void
CBoxView::FinishCreateSelf() {
	// find all subpanes with a negative usercon and make them broadcast to the box
	// make toggle buttons broadcast to the box whether or not they have an negative user con
	LArrayIterator iterator(mSubPanes, LArrayIterator::from_Start);
	LControl	*theSub;
	LToggleButton *theTB;
	while (iterator.Next(&theSub)) {
		if (theSub->GetUserCon() < 0) {
			theSub->AddListener(this);
		} else {
			theTB = dynamic_cast<LToggleButton*>(theSub);
			if (theTB) {
				theTB->AddListener(this);
			}
		}
	}
	if (mSendUserConOnShow) { // generally this means we are a button bar
		if (mVisible != triState_Off) {		// simulate a click on default button (userCon) to select panel, forcing
			SimulateButtonClick(mUserCon, kForceButtonAction);	// the button to sent the ValueChanged message
		}
	}
	if (mFirstSliderID) {	// we have sliders and need to link them
		CSliderLinker *aLinker = NULL;
		CGalacticSlider *aSlider = NULL;
		for (int i = 0; i < MAX_LINKED_SLIDERS; i++) {
			aSlider = (CGalacticSlider*) FindPaneByID(i + mFirstSliderID);
			if (!aSlider) {
				break;	// if no slider found, quit checking
			}
			if (!aLinker) {
				aLinker = new CSliderLinker(this);
			}
			LControl *theLock = (LControl*) FindPaneByID(i + mFirstSliderID + kSliderLockIDOffset);
			if (theLock) {	// did we find a lock toggle button for this slider?
				theLock->AddListener(aSlider);			// make the lock broadcast to the slider
				aSlider->SetLockToggleButton(theLock);	// and let the slider know who locks it
			}
			aLinker->AddSlider(aSlider);
		}
		if (aLinker) {
			if (aLinker->BeginLinking() ) {
				mLinker = aLinker;	// make sure the linker listens to us to
			}
		}
	}
	if (mVisible == triState_Off) {
		MoveBy(-10000, 0, false);	// move out of the window's viewable region
	}
}

void
CBoxView::ListenToMessage(MessageT inMessage, void *ioParam) {
	SInt32 value = *(static_cast<long*>(ioParam));
	if (inMessage == cmd_SwitchLanguages) {		// we switched languages, tell all of our sub-strings to reload
		LArrayIterator iterator(mSubPanes, LArrayIterator::from_Start);
		LPane	*theSub;
		CResCaption *theResCaption;
		while (iterator.Next(&theSub)) {
			theResCaption = dynamic_cast<CResCaption*>(theSub);
			if (theResCaption) {
				theResCaption->ReloadStrRes();
			}
		}
	} else if ((inMessage >= cmd_SetPanelFirst) && (inMessage <= cmd_SetPanelLast)) { // check for message was a button pressed
		if (value == 1)	{					// only pass on set panel command when button selected,
			mGame->ObeyCommand(inMessage, NULL);	// ignore any button deselected messages
		}
	} else if ((inMessage & 'b\0\0\0') == 'b\0\0\0') {	// any buttons that want to have DoPanelButton called for them
		DoPanelButton(inMessage, value);				// should have a message of "b???", where ? is any character
	} else {
		ObeyPanelCommand(inMessage);
	}
}

void
CBoxView::ObeyPanelCommand(MessageT) {	// inCommand) {
	// default method does nothing
}

void
CBoxView::EnableItem(PaneIDT inItemID) {
	LPane *theItem = FindPaneByID(inItemID);
	ThrowIfNil_(theItem);
	if (!theItem->IsEnabled()) {
		theItem->Enable();
		theItem->Refresh();
	}
}

void
CBoxView::DisableItem(PaneIDT inItemID) {
	LPane *theItem = FindPaneByID(inItemID);
	ThrowIfNil_(theItem);
	if (theItem->IsEnabled()) {
		theItem->Disable();
		theItem->Refresh();
	}
}


void
CBoxView::SetItemEnable(PaneIDT inItemID, bool inEnabled) {
	LPane *theItem = FindPaneByID(inItemID);
	ThrowIfNil_(theItem);
	bool wasEnabled = theItem->IsEnabled();
	if (!wasEnabled && inEnabled) {
		theItem->Enable();
		theItem->Refresh();
	} else if (wasEnabled && !inEnabled) {
		theItem->Disable();
		theItem->Refresh();
	}
}

void
CBoxView::SetControlValue(PaneIDT inItemID, SInt32 inValue) {
	LControl *theItem = (LControl*) FindPaneByID(inItemID);
	ThrowIfNil_(theItem);
	SInt32 oldValue = theItem->GetValue();
	if (oldValue != inValue) {
		theItem->StopBroadcasting();	// don't be telling us you changed, we already know it
		theItem->SetValue(inValue);
		theItem->Refresh();
		theItem->StartBroadcasting();
	}
}

void
CBoxView::SetItemValue(PaneIDT inItemID, SInt32 inValue) {
	LPane *theItem = FindPaneByID(inItemID);
	ThrowIfNil_(theItem);
	SInt32 oldValue = theItem->GetValue();
	if (oldValue != inValue) {
		theItem->SetValue(inValue);
		theItem->Refresh();
	}
}

SInt32
CBoxView::GetItemValue(PaneIDT inItemID) {
	LPane *theItem = FindPaneByID(inItemID);
	ThrowIfNil_(theItem);
	return theItem->GetValue();
}

// passing true for inForceAction will first set the control value to zero so that the "click" will
// be sure to trigger the entire effects of a normal click. For example, if a radio button is already
// selected, and SimulateButtonClick() with inForceAction = true is called it will behave just as if
// the button was not selected, and re-select it. Otherwise, if inForce action had been false, the
// SetValue() call would not have broadcast the ValueChanged message. Use this with caution, since it
// normally shouldn't need the "true" behavior.
bool
CBoxView::SimulateButtonClick(PaneIDT inItemID, bool inForceAction) {
	LPane *foundPane = FindPaneByID(inItemID);
	LControl *theControl = dynamic_cast<LControl*>(foundPane);	// double check that we have a valid item
	if (theControl) {
		if (inForceAction) {
			theControl->StopBroadcasting();		// force the control to sent the ValueChanged message
			theControl->SetValue(0);			// when the SimulateHotSpotClick() method is called
			theControl->StartBroadcasting();
		}
		theControl->SimulateHotSpotClick(1);
		return true;
	} else {
		return false;
	}
}

void
CBoxView::DoPanelButton(MessageT, long) {
	DEBUG_OUT("CBoxView::DoPanelButton() was called", DEBUG_ERROR);
	SysBeep(1);
}


void
CBoxView::Show() {
	if (mSendUserConOnShow) {
		SimulateButtonClick(mUserCon, kForceButtonAction);	// select our default button's panel
	}
	MoveBy(10000, 0, false);	// move back into the window's viewable region
	LView::Show();
}

void
CBoxView::Hide() {
	LView::Hide();
	MoveBy(-10000, 0, false);	// move out of the window's viewable region
}


void
CBoxView::UpdatePanel(AGalacticThingy *) {	// inThingy
	// default method does nothing, override to do panel specific stuff
}


#pragma mark -

// **** Ship Navigation Panel (bxNa) ****

CbxNaPanel::CbxNaPanel(LStream *inStream)
: CBoxView(inStream) {
	mShip = NULL;
	mStarMap = NULL;
	mPlotter = NULL;
	mWaitingButton = NULL;
	mWaypoints = NULL;	// this table will be assigned in FinishCreateSelf()
}

void
CbxNaPanel::Show() {
	CBoxView::Show();
	if (mShip) {
		DEBUG_OUT("CbxNaPanel::Show() is executing code that I didn't think it ever would", DEBUG_ERROR);
		if (mWaypoints) {
			TableCellT cell;
			mWaypoints->GetSelectedCell(cell);
			CShip* aShip = (CShip*) ValidateThingy(mShip);
			if (aShip != mShip) {
				DEBUG_OUT("CbxNaPanel::Show() - Invalid Ship", DEBUG_ERROR);
			}
			mShip = aShip;
			if (mShip) {
				mShip->SetSelectedWP(cell.row);
				mShip->Refresh();
			}
		}
	}
}

void
CbxNaPanel::Hide() {
	CBoxView::Hide();
	mShip = ValidateShip(mShip);	// ¥¥¥ debugging
	ASSERT(mShip != NULL);
	if (mShip) {
		mShip->SetSelectedWP(0);
		mShip->Refresh();
	}				// whenever this item is shown again, we will get an
	mShip = NULL;	// UpdatePanel() call, and we can set mShip there.
	if (CNewTable::sActiveTable == mWaypoints) {
		CNewTable::sActiveTable = NULL;
	}
}

void
CbxNaPanel::FinishCreateSelf() {
	mWaypoints = static_cast<CWaypointTable*>( FindPaneByID('crsL') );
	ThrowIfNil_(mWaypoints);
	mWaypoints->AddListener(this);	// listen to its messages
	CBoxView::FinishCreateSelf();
}


void 
CbxNaPanel::DoPanelButton(MessageT inMessage, long inValue) {
	bool reloadList = false; 
	bool cancelScrap = false;
	bool killPatrol = false;
	bool updateButtons = true;
	bool updatePatrol = false;
	TableCellT cell;
	switch (inMessage) {
		case 'bAdd':
          #if TUTORIAL_SUPPORT
			if (Tutorial::TutorialIsActive()) {
				Tutorial* t = Tutorial::GetTutorial();
				if ((t->GetPageNum() == tutorialPage_MovingShips2) || 
					(t->GetPageNum() == tutorialPage_MovingShips3)) {
					t->NextPage();
				}
			}
          #endif //TUTORIAL_SUPPORT
		case 'bSet':
			mGame->HideSelectionMarker();
			if (mPlotter != NULL) { 			// don't let us get two plotters going at once
				mPlotter->StopRepeating();	// also, turn off plotting course if button clicked	
				delete mPlotter;			// again while set
				mPlotter = NULL;		
			}
			if (inValue == 1) {
			  #ifdef DEBUG
				AGalacticThingy* selected = mGame->GetSelectedThingy();
				if (selected != mShip) {
					ASSERT(selected != NULL);
					DEBUG_OUT("Plotting course for "<<mShip<<" but pane "<<selected->GetID()<<" was selected", DEBUG_ERROR);
				}
			  #endif
				mWaitingButton = static_cast<LButton*>( FindPaneByID(inMessage) );
				CStarMapView::ClickPending(clickType_Waypoint);	// let starmap know to send us 
				int index = mShip->GetSelectedWP() - 1;
				if ( ((index == 0) && mShip->CourseLoops()) || (index < 0) ) {
				    index = mShip->GetWaypointCount();
				}
				Waypoint wp = mShip->GetWaypoint(index);			// the next click
				mPlotter = new CCoursePlotter(mStarMap, wp, mShip->GetSpeed());
				mPlotter->StartRepeating();		// now plotter will track mouse until clicked and
				// then the StarMap will send a ClickedInMap message back to us, we will handle the click
				// completion in our ListenToMessage() method
			} else {	// value = 0, clicked on button second time (or click simulated)
				CStarMapView::ClickReceived();	// deactivating course plotting
				mWaitingButton = NULL;
			}
			LCommander::SetUpdateCommandStatus(true);
			break;
		case 'bClr':
			mShip->ClearCourse(true, kRefresh);	// clear course deletes the list, so we need to reload it
			mShip->UserChangedCourse();
			reloadList = true;
			break;
		case 'bMem':
			mShip->SwapCourseWithCourseMem();	// swapping changes from one list to another,
			mShip->UserChangedCourse();			// so we need to reload it
			reloadList = true;
			killPatrol = true;
			cancelScrap = true;
			break;
		case 'bCal':
			mShip->SetCallIn(inValue);
			mShip->Changed();
			break;
		case 'bLop':
			mShip->SetLooping(inValue);
			mShip->UserChangedCourse();
			mShip->Refresh();
			break;
		case 'bHun':
			mShip->SetHunting(inValue);
			mShip->Changed();
			break;
		case 'bPat':
			// begin tutorial stuff
		  #if TUTORIAL_SUPPORT
			if (Tutorial::TutorialIsActive()) {
				Tutorial* t = Tutorial::GetTutorial();
				if (t->GetPageNum() == tutorialPage_AwaitingOrders) {
					t->NextPage();
				}
			}
		  #endif //TUTORIAL_SUPPORT
			// end tutorial stuff
			mShip->SetPatrol(inValue);
			mShip->Changed();
			if (inValue) {
				cancelScrap = true;		// if the ship is being put on patrol, clear the scrap flag
				mShip->ClearCourse(true, kRefresh);	// also clear the entire course
				mShip->UserChangedCourse();
			}	
			updatePatrol = true;
			reloadList = true;
			break;
		case 'bDel':
			mWaypoints->GetSelectedCell(cell);
			mShip->DeleteWaypoint(cell.row, kRefresh);
			if (mShip->GetWaypointCount() == 1) {
			    // only the point of origin is left, delete it
			    mShip->DeleteWaypoint(1, kRefresh); 
			}
			mShip->UserChangedCourse();
			reloadList = true;
			break;
		default:
			updateButtons = false;
			DEBUG_OUT("Nav Panel got button press from an unknown button", DEBUG_ERROR);
	} // end switch
	if (reloadList) {
		mWaypoints->GetSelectedCell(cell);  // save the selected item for later recall
		LArray *aList = MakeArrayFromWaypointVector(mShip->GetWaypointsList());	// get the new list
		mWaypoints->InstallArray(aList, true);				                // display the ship's waypoints
		mWaypoints->SetActiveWaypointNum( mShip->GetDestWaypointNum() ); // indicate the active wp
		mWaypoints->SelectCell(cell);   // this will generate a message_CellSelected
		updateButtons = true;
	}
	// killPatrol is set whenever the ship does something that should take it off
	// patrol
	if (killPatrol && mShip->IsOnPatrol() && mShip->CoursePlotted()) {
		mShip->SetPatrol(false);
		updatePatrol = true;
	}
	// cancelScrap is set if the ship has been told to do something
	// that should cause it to cancel scraping if needed.
	if (cancelScrap) {
		mShip->SetScrap(false);
		LCaption *theScrappedCaption = (LCaption*) FindPaneByID(666);	// update scrapped status
		theScrappedCaption->Hide();
	}
	// updatePatrol is set whenever the ship changed its patrol state, so that
	// the patrol strength of the owner can be recalculated.
	if (updatePatrol) {
		Waypoint currPos = mShip->GetPosition();
		if (currPos.GetType() == wp_Star) {
			CStar* atStar = (CStar*)currPos.GetThingy();
			if (atStar) {
				atStar->RecalcPatrolStrength();
				atStar->Refresh();
			}
		}
		mShip->Refresh();
		updateButtons = true;
	}
	if (updateButtons) {
		UpdateButtons();
	}
}

void
CbxNaPanel::ListenToMessage(MessageT inMessage, void *ioParam) {
	if (inMessage == message_CellDoubleClicked) {
		if (mShip->GetOwner() == mGame->GetMyPlayerNum()) {
			LButton* aButton = (LButton*) FindPaneByID('bSet');
			ThrowIfNil_(aButton);
			aButton->SimulateHotSpotClick(0);
		}
	} else if (inMessage == message_CellSelected) {
		CellInfoT *cellInfo = (CellInfoT*) ioParam;
		UpdateButtons();
		// tell the ship to highlight that leg of its course
		ASSERT_STR(mShip!=NULL, "Nav Panel got select cell but no ship active!");
		if (!mShip)
			return;
		if (cellInfo->cell.row == 1) {
		    // disallow selecting first row (point of origin)
		    cellInfo->cell.row = 0;
		    mWaypoints->SelectCell(cellInfo->cell);
		} else {
    		mShip->SetSelectedWP(cellInfo->cell.row);
    		mShip->FocusDraw();	// redraw the selected item immediately
    		mShip->EraseSelectionMarker();
    		mShip->DrawSelf();
		}
	} else if (inMessage == msg_ClickedInMap) {
	    bool reloadList = false;
	    // the user clicked in the starmap to choose the destination for
	    // a particular waypoint they were plotting
		if (mPlotter != NULL) {
		    mShip->Refresh();
			MapClickT *theClickInfo = static_cast<MapClickT*>(ioParam);
			TableCellT cell;
			mWaypoints->GetSelectedCell(cell);
			// begin tutorial stuff
			bool tutorialSkipAdd = false;
		  #if TUTORIAL_SUPPORT
			if (Tutorial::TutorialIsActive()) {
				Tutorial* t = Tutorial::GetTutorial();
				Waypoint wp = theClickInfo->clickedWP;
				AGalacticThingy* it = wp.GetThingy();
				if (t->GetPageNum() == tutorialPage_MovingShips3) {
					// waiting for them to target star S-20 (later called New Terra)
					if (it && it->GetID() == tutorial_NewTerraStarID) {
						t->ForceNextPageToBe(tutorialPage_BuildSatellite);
					} else {
						// didn't pick the right course, don't add it
						tutorialSkipAdd = true;
					}
					t->NextPage();
				} else if (t->GetPageNum() == tutorialPage_ShipsOrders) {
					// waiting for them to target star S-7
					if (it && it->GetID() == tutorial_S7StarID) {
						t->NextPage();
					}
				} else if (t->GetPageNum() == tutorialPage_CourierOrders) {
					if (it && it->GetID() == tutorial_NewTerraStarID) {
						t->NextPage();
					}
				}
			}
		  #endif //TUTORIAL_SUPPORT
			if (!tutorialSkipAdd) {
				if (mWaitingButton->GetPaneID() == 'bAdd') {
					if (!mWaypoints->IsValidCell(cell)) {	// beyond end of list
                        // always put starting position into courses
            			if ( mShip->GetWaypointCount() == 0) {	// is the list empty?
            			    Waypoint wp = mShip->GetPosition();
            			    // we need to add this to both the ships course and the UI waypoint table
    						mShip->AddWaypoint(kEndOfWaypointList, wp, kDontRefresh);  
    						mShip->SetFromWaypointNum(1);
    						mShip->SetDestWaypointNum(2);
                    		mWaypoints->SetActiveWaypointNum(2); // indicate the active wp
            			} else if (mShip->GetWaypointCount() == 2) {
            			    // check for adding 2nd waypoint to courier
            			    if ( (mShip->GetShipClass() == class_Scout) && !mShip->CourseLoops()) {
            			        mShip->SetLooping(true);
            			    }
            			}
           			    // we need to add this to both the ships course and the UI waypoint table
						mShip->AddWaypoint(kEndOfWaypointList, theClickInfo->clickedWP, kRefresh);
//						SPoint32 wasAtPos;
//						mWaypoints->GetScrollPosition(wasAtPos);	// re-installing the array changes our position
//						mWaypoints->InstallArray(MakeArrayFromWaypointVector(mShip->GetWaypointsList()), true);
//						mWaypoints->ScrollImageTo(wasAtPos.h, wasAtPos.v, false);
						reloadList = true;
					} else {	// within list
					    DEBUG_OUT("Adding waypoint "<<theClickInfo->clickedWP<<" into course at "<<cell.row - 1, DEBUG_TRIVIA | DEBUG_USER);
						mShip->AddWaypoint(cell.row - 1, theClickInfo->clickedWP, kRefresh);
						cell.row++;
						mWaypoints->SelectCell(cell);
						mShip->SetSelectedWP(cell.row);
						mShip->WaypointsChanged(kRefresh);	// we manually changed the course, recalc movement
						reloadList = true;
					}
				} else {  // changing a waypoint, not adding
					mShip->SetWaypoint(cell.row, theClickInfo->clickedWP, kRefresh);
					mShip->WaypointsChanged(kRefresh);	// let the ship recalc it's current move
					reloadList = true;
				}
			}	// ! tutorialSkipAdd
			// end tutorial stuff
			mWaitingButton->SimulateHotSpotClick(0);
			if (mShip->IsOnPatrol()) {
				mShip->SetPatrol(false);
				Waypoint currPos = mShip->GetPosition();
				if (currPos.GetType() == wp_Star) {
					CStar* atStar = static_cast<CStar*>(currPos.GetThingy());
					if (atStar) {
						atStar->RecalcPatrolStrength();
						atStar->Refresh();
					}
				}
			}
			if (mShip->IsToBeScrapped()) {	// if ship was to be scrapped, and now
				mShip->SetScrap(false);		// a course has been assigned, cancel scrapping
				LCaption *theScrappedCaption = static_cast<LCaption*>( FindPaneByID(666) );	// update scrapped status
				theScrappedCaption->Hide();
			}
			mShip->UserChangedCourse();
			mShip->Refresh();
			UpdateButtons();
		}
    	if (reloadList) {
    	    TableCellT cell;
    		mWaypoints->GetSelectedCell(cell);  // save the selected item for later recall
    		LArray *aList = MakeArrayFromWaypointVector(mShip->GetWaypointsList());	// get the new list
    		mWaypoints->InstallArray(aList, true);				                // display the ship's waypoints
    		mWaypoints->SetActiveWaypointNum( mShip->GetDestWaypointNum() ); // indicate the active wp
    		mWaypoints->SelectCell(cell);   // this will generate a message_CellSelected
    	}
	} else {
		CBoxView::ListenToMessage(inMessage, ioParam);
	}
}

void
CbxNaPanel::UpdateButtons() {
	bool disableAll = (mShip->GetOwner() != mGame->GetMyPlayerNum());
  #ifdef DEBUG
	if (gAllowChangeAnything) {
		disableAll = false;
	}
  #endif
	if (mWaitingButton || disableAll) {	// if a click is pending, disable all the other buttons
		mWaypoints->Disable();
		SetControlValue('bLop', 0 );	// v1.2b11d6, make sure disable items aren't drawn set
		SetControlValue('bCal', 0 );
		SetControlValue('bHun', 0 );
		SetControlValue('bPat', 0 );
		DisableItem('bClr');
		DisableItem('bMem');
		DisableItem('bLop');
		DisableItem('bSav');
		DisableItem('bDel');
		DisableItem('bCal');
		DisableItem('bLod');
		DisableItem('bHun');
		DisableItem('bPat');
		if (disableAll) {
			DisableItem('bSet');	// disable set, since we clicked the Add button
			DisableItem('bAdd');	// disable Add, cause we clicked the Set button
		} else {
			if (mWaitingButton->GetPaneID() == 'bAdd') {
				DisableItem('bSet');	// disable set, since we clicked the Add button
			} else {
				DisableItem('bAdd');	// disable Add, cause we clicked the Set button
			}
		}
	} else {	// no click pending, update buttons as normal
		mWaypoints->Enable();
		bool canMove = (mShip->GetSpeed() != 0);
		bool mustPatrol = (mShip->GetShipClass() == class_Satellite);
		bool hasCourse = mShip->CoursePlotted();
		bool hasMemCourse = mShip->CourseInMem();
		bool hasLoopable = (mShip->GetWaypointCount() > 1);
		TableCellT cell;
		mWaypoints->GetSelectedCell(cell);
		bool valid = mWaypoints->IsValidCell(cell);
		SetItemEnable('bClr', hasCourse);
		SetItemEnable('bMem', (hasCourse || hasMemCourse) );
		SetItemEnable('bLop', hasLoopable && canMove);
		SetItemEnable('bHun', hasCourse && canMove);
		SetItemEnable('bPat', mShip->IsDocked() && !mustPatrol);
//		SetItemEnable('bSav', hasCourse);	¥¥¥ commented out till load and save implemented
		SetItemEnable('bDel', valid);	// del & set are only available when a valid list item is selected
		SetItemEnable('bSet', valid && canMove);
		SetItemEnable('bCal', canMove);
		SetItemEnable('bAdd', canMove);
//		EnableItem('bLod');	// Load is always available ¥¥¥ commented out till load and save implemented
		if (!hasLoopable) {
			mShip->SetLooping(false);	// make sure the loop button is never set while disabled
		}
		SetControlValue('bLop', mShip->CourseLoops() );
		SetControlValue('bHun', mShip->IsHunting() );
		SetControlValue('bPat', mShip->IsOnPatrol() );
		SetControlValue('bSet', 0);
		SetControlValue('bAdd', 0);
		LCaption *theMemCaption = (LCaption*) FindPaneByID(2);	// find 
		ThrowIfNil_(theMemCaption);
		if (hasMemCourse) {
			theMemCaption->SetDescriptor("\pMem");	// us mem indicator if course in memory
		} else {
			theMemCaption->SetDescriptor("\p");	// leave blank if no course in memory
		}
	}
//	DisableItem('bLod');	// ¥¥¥ TEMP CODE till load and save are implemented
//	DisableItem('bSav');	// ¥¥¥ then code marked '¥¥¥' above can be uncommented
}

void
CbxNaPanel::UpdatePanel(AGalacticThingy* inThingy) {
	ASSERT_STR(inThingy == ValidateThingy(inThingy), "CbxNaPanel::UpdatePanel() called for invalid thingy.");
	inThingy = ValidateThingy(inThingy);
	if (inThingy) {
	  #ifdef DEBUG
		AGalacticThingyUI* selected = mGame->GetSelectedThingy();
		if (selected != inThingy) {
			ASSERT(selected != NULL);
			DEBUG_OUT("Updating Nav Panel for "<<inThingy<<" but pane "<<selected->GetID()<<" selected", DEBUG_ERROR);
		}
	  #endif
		mPlotter = NULL;	// 1.2fc9 bug fix: never hold course plotter between updates
		bool newShip = (inThingy != mShip);
		if (newShip) {
			mShip = (CShip*) inThingy;
			mStarMap = (CStarMapView*) mShip->GetSuperView();
		}
		int myPlayerNum = mGame->GetMyPlayerNum();
		bool isAlly = mShip->IsAllyOf(myPlayerNum);
		UInt8 vis = mShip->GetVisibilityTo(myPlayerNum);
		LCaption *theScrappedCaption = (LCaption*) FindPaneByID(666);	// show scrapped status
		ThrowIfNil_(theScrappedCaption);
		if (mShip->IsToBeScrapped() && isAlly) {
			theScrappedCaption->Show();
		} else {
			theScrappedCaption->Hide();
		}
		if (isAlly) {
    		LArray *aList = MakeArrayFromWaypointVector(mShip->GetWaypointsList());
    		mWaypoints->InstallArray(aList, true);					// display the ship's waypoints
    		mWaypoints->SetActiveWaypointNum( mShip->GetDestWaypointNum() ); // indicate the active wp
    		CNewTable::sActiveTable = mWaypoints;
    		SetControlValue('bCal', mShip->WillCallIn() );			// initialize the callin button
		} else {
		    mWaypoints->InstallArray(NULL, false);
    		SetControlValue('bCal', false );			// initialize the callin button
		}
		LCaption *thePos = (LCaption*) FindPaneByID(1);			// initialize the ship position display
		std::string descStr;
		LStr255 s = mShip->GetPosition().GetDescription(descStr);
		thePos->SetDescriptor(s);
		if (mShip->GetThingySubClassType() == thingyType_Fleet) {
		   // v2.1.2, recalc fleet info before displaying any info to make sure it is correct
		   CFleet* fleet = static_cast<CFleet*>(mShip);
		   fleet->CalcFleetInfo();
		}
		LCaption *theTechCaption = (LCaption*) FindPaneByID(3);	// find tech level
		ThrowIfNil_(theTechCaption);
		if (isAlly || (vis >= visibility_Tech)) {
		    s = (SInt32)mShip->GetTechLevel();
	    } else {
	        s = "?";
	    }
		theTechCaption->SetDescriptor(s);
		LCaption *thePowerCaption = (LCaption*) FindPaneByID(6); 
		ThrowIfNil_(thePowerCaption);
		if (isAlly || (vis >= visibility_Class)) {
		    s = (SInt32) mShip->GetPower();
	    } else {
	        s = "?";
	    }
		thePowerCaption->SetDescriptor(s);
		LCaption *theSpeedCaption = (LCaption*) FindPaneByID(5);
		ThrowIfNil_(theSpeedCaption);
		if (isAlly || (vis >= visibility_Speed)) {
    		std::string speedStr;
    		GetSpeedString(mShip->GetSpeed(), speedStr);	// Speed
    		s = speedStr.c_str();
	    } else {
	        s = "?";
	    }
		theSpeedCaption->SetDescriptor(s);
		LCaption *theDamageCaption = (LCaption*) FindPaneByID(4);
		ThrowIfNil_(theDamageCaption);
		if (isAlly || (vis >= visibility_Class)) {
    		long damagePercent = mShip->GetDamagePercent();
    		ASSERT_STR((damagePercent < 100), mShip << " has " << damagePercent << " percent damage!");
    		s = damagePercent;
    		s += "\p %";
	    } else {
	        s = "?";
	    }
		theDamageCaption->SetDescriptor(s);
		StopListening();	//we can't guarrantee that SelectCell() will always broadcast, so
		TableCellT cell = {0, 1};	// UpdateButtons() might not get called. So instead we ignore
		mWaypoints->SelectCell(cell);	// the message and call UpdateButtons() manually.
		mShip->SetSelectedWP(0);
		StartListening();
		LPane *scroller = FindPaneByID(pane_Scroller);
		if (scroller) {	// panel has scroller, enable it so we can use it
			scroller->Enable();	// ¥¥¥ if we don't do this, the panel will not accept clicks, ¥¥¥
		} else {				// ¥¥¥ even if we set it to "Enabled" in the PPob. Why??? ¥¥¥
			DEBUG_OUT("No scroller found", DEBUG_ERROR);
		}
		UpdateButtons();
	} else {
		mShip = NULL;
	}
}

#pragma mark -

/*
// **** Ship Info Panel (bxSI) ****

CbxSIPanel*
CbxSIPanel::CreateCbxSIPanelStream(LStream *inStream) {
	return (new CbxSIPanel(inStream));
}

CbxSIPanel::CbxSIPanel(LStream *inStream)
: CBoxView(inStream) {
}

void 
CbxSIPanel::DoPanelButton(MessageT, long) {	// inMessage, inValue
	DEBUG_OUT("Ship Info Panel got button press from an unknown button", DEBUG_ERROR);
}

void
CbxSIPanel::UpdatePanel(AGalacticThingy*) { // inThingy
	// do custom stuff here
}


#pragma mark -
*/

// **** System Info Panel (bxSI) ****

CbxInPanel::CbxInPanel(LStream *inStream)
: CBoxView(inStream) {
	mPlotter = NULL;
	mStar = NULL;
	mStarMap = NULL;
	mNewShipDestButton = NULL;
	mShipCompletionCaption = NULL;
	mPrevTTNS = 0;
}

void
CbxInPanel::FinishCreateSelf() {
	CBoxView::FinishCreateSelf();
	for (int i = 0; i < 10; i++) {
		mSliders[i] = (CGalacticSlider*)FindPaneByID(i + mFirstSliderID);
		if (mSliders[i] == NULL) {
			mNumSliders = i;
			break;
		}
	}
	for (int i = mNumSliders; i < 10; i++) {
		mSliders[i] = NULL;
	}
	mNewShipDestButton = (LButton*) FindPaneByID('bSet');
	ThrowIfNil_(mNewShipDestButton);
	mShipCompletionCaption = (LCaption*) FindPaneByID(3);
	ThrowIfNil_(mShipCompletionCaption);
	LControl* popup = (LControl*) FindPaneByID(popup_ShipType);
	ThrowIfNil_(popup);
	CHelpAttach* aHelpAttach = new CHelpAttach(STRx_StarmapHelp, str_ShipTypeMenuHelp);
	popup->AddAttachment(aHelpAttach);
}

void 
CbxInPanel::DoPanelButton(MessageT inMessage, long inValue) {
	if (inMessage == 'bEvn') {
		mLinker->ListenToMessage(msg_SetSlidersEven, NULL);
	  #if TUTORIAL_SUPPORT
		if (Tutorial::TutorialIsActive()) {	// v1.2fc7, deal with tutorial
			Tutorial* t = Tutorial::GetTutorial();
			if (t->GetPageNum() == tutorialPage_SetSliderEqual) {
				// we were waiting for them to set sliders equal
				t->NextPage();
			}
		}
	  #endif //TUTORIAL_SUPPORT
	} else if (inMessage == 'bSet') {
		mGame->HideSelectionMarker();
		if (mPlotter != NULL) { 			// don't let us get two plotters going at once
			mPlotter->StopRepeating();	// also, turn off plotting course if button clicked	
			delete mPlotter;			// again while set
			mPlotter = NULL;
		}
		if (inValue == 1) {
			CStarMapView::ClickPending(clickType_Waypoint);	// let starmap know to send us the next click
			Waypoint wp = mStar->GetPosition();
			mPlotter = new CCoursePlotter(mStarMap, wp, 0);
			mPlotter->StartRepeating();		// now plotter will track mouse until clicked and
			// then the StarMap will send a ClickedInMap message back to us, we will handle the click
			// completion in our ListenToMessage() method
		} else {	// value = 0, clicked on button second time (or click simulated)
			CStarMapView::ClickReceived();	// deactivating course plotting
			Waypoint wp;
			mStar->SetNewShipDestination(wp);	// set to null point
			LCaption *theNSDest = (LCaption*) FindPaneByID(5);
			theNSDest->SetDescriptor("\p");
			mStar->ThingMoved(kRefresh);
		}
	} else {
		DEBUG_OUT("System Info Panel got button press from an unknown button", DEBUG_ERROR);
	}
}

void
CbxInPanel::UpdateShipCompletion() {
	SInt32 ttns = mStar->GetEstimatedTurnsToNextShip();
	if (ttns != mPrevTTNS) {	// only do the update if the value has changed
		mPrevTTNS = ttns;
		LStr255 s = (SInt32)mStar->GetShipPercentComplete();
		s += "\p%";
		if (ttns != -1) {
			LStr255 s2 = ttns;
			s += "\p ("+s2+"\p)";
		}
		mShipCompletionCaption->SetDescriptor(s);
		mShipCompletionCaption->UpdatePort();
		mShipCompletionCaption->SetVisible(mStar->IsAllyOf(mGame->GetMyPlayerNum()));
	}
}

void
CbxInPanel::ListenToMessage(MessageT inMessage, void *ioParam) {
	if (inMessage == cmd_ChangeShipType) {
		SInt32 value = *(SInt32*)ioParam;
		SInt16 shiptype = value - 1;
		mStar->SetBuildHullType(shiptype);
		UpdateShipCompletion();
 		DEBUG_OUT("Set "<<mStar<<" to produce class "<<value<<" ships", DEBUG_TRIVIA | DEBUG_USER);
		mStar->Changed();
	  #if TUTORIAL_SUPPORT
		if (Tutorial::TutorialIsActive()) {	// v1.2fc7, deal with tutorial
			Tutorial* t = Tutorial::GetTutorial();
			if (mStar->GetPaneID() == tutorial_HomeStarID) {
				// we were waiting for a change to type of ship being built at Andromeda
				if ( (t->GetPageNum() == tutorialPage_BuildSatellite) && (shiptype == class_Satellite)) {
					t->NextPage();
				} else if ((t->GetPageNum() == tutorialPage_BuildCouriers) && (shiptype == class_Scout)) {
					t->NextPage();
				} else if ((t->GetPageNum() == tutorialPage_BuildBattlshp2) && (shiptype == class_Fighter)) {
					t->NextPage();
				}
			} else if (mStar->GetPaneID() == tutorial_NewTerraStarID) {
				if ((t->GetPageNum() == tutorialPage_BuildBattleshp) && (shiptype == class_Fighter)) {
					t->NextPage();
				}
				// we were waiting for a change to type of ship being built at New Terra
			}
		}
	  #endif //TUTORIAL_SUPPORT
	} else if (inMessage == msg_SliderMoved) {	// slider linker will send this first time slider moves
		DEBUG_OUT("Slider Moved, calling Changed() for "<<mStar, DEBUG_TRIVIA | DEBUG_USER);
		UpdateShipCompletion();	//dynamically update turns to completion
		mStar->Changed();						// so if it is ours, we can flag things
	  #if TUTORIAL_SUPPORT
		if (Tutorial::TutorialIsActive()) {	// v1.2fc4, deal with tutorial
			if (Tutorial::GetTutorial()->GetPageNum() == tutorialPage_ReduceShipbld) {
				// we were waiting for them to start reducing their shipbuilding
				if (TutorialWaiter::GetTutorialWaiter()) {
					TutorialWaiter::ResetTutorialWaiter();
				} else {
					new TutorialWaiter(2);	// wait two seconds after they stop moving the the bar
				}
			}
		}
	  #endif // TUTORIAL_SUPPORT
	} else if (inMessage == msg_ClickedInMap) {
		if (mPlotter != NULL) {
			mPlotter->StopRepeating();	// also, turn off plotting course if button clicked	
			delete mPlotter;			// again while set
			mPlotter = NULL;
			MapClickT *theClickInfo = (MapClickT*)ioParam;
			mStar->SetNewShipDestination(theClickInfo->clickedWP);
			// begin tutorial stuff
		  #if TUTORIAL_SUPPORT
			if (Tutorial::TutorialIsActive()) {
				Tutorial* t = Tutorial::GetTutorial();
				Waypoint wp = theClickInfo->clickedWP;
				AGalacticThingyUI* it = (AGalacticThingyUI*) wp.GetThingy();
				if (it && (it->GetPaneID() == tutorial_RendezvousAlphaID) 
				   && (it->GetThingySubClassType() == thingyType_Rendezvous) ) {
					if ( (t->GetPageNum() == tutorialPage_ClickSendTo) ||
						 (t->GetPageNum() == tutorialPage_NTerraAutoDst) ) {
						t->NextPage();
					}
				}
			}
		  #endif //TUTORIAL_SUPPORT
			// end tutorial stuff
			LCaption *theNSDest = (LCaption*) FindPaneByID(5);
			std::string descStr;
		    LStr255 s = theClickInfo->clickedWP.GetDescription(descStr);
			theNSDest->SetDescriptor(s);
			mStar->ThingMoved(kRefresh);
		}
		CStarMapView::ClickReceived();	// deactivating course plotting
	} else {
		CBoxView::ListenToMessage(inMessage, ioParam);
	}
}

void
CbxInPanel::UpdatePanel(AGalacticThingy *inThingy) {
	ASSERT_STR(inThingy == ValidateThingy(inThingy), "CbxInPanel::UpdatePanel() called for invalid thingy.");
	inThingy = ValidateThingy(inThingy);
	if (inThingy) {
		int baseSettingNum = GetFirstSettingNum();
		for (int i = 0; i < mNumSliders; i++) {
			mSliders[i]->DetachSetting();
		}
		mPlotter = NULL;	//  1.2fc9 bug fix, never need to hang on to this pointer through an update
		CStar* aStar = (CStar*) inThingy;
		mStar = aStar;
		bool isOurs = aStar->IsMyPlayers();
		int myPlayerNum = mGame->GetMyPlayerNum();
		bool isAlly = aStar->IsAllyOf(myPlayerNum);
		UInt8 vis = aStar->GetVisibilityTo(myPlayerNum);
		bool detailVis = (vis >= visibility_Details);
	  #ifdef DEBUG
		if (gAllowChangeAnything) {
			isOurs = true;
			isAlly = true;
		}
	  #endif
	  #ifdef GAME_CONFIG_HOST
		isOurs = true;	// always can change stuff in game config host
	  #endif // GAME_CONFIG_HOSR
		mStarMap = (CStarMapView*) aStar->GetSuperView();
	//	if (isOurs)
	//		aStar->Changed();	// ¥¥¥ presume it was changed if we examined it
		for (int i = 0; i < mNumSliders; i++) {
			if (isOurs) {
				mSliders[i]->Enable();
			} else {
				mSliders[i]->Disable();
			}
			if (isAlly && (i < MAX_STAR_SETTINGS)) {
    			mSliders[i]->AttachToSetting(aStar->GetSettingPtr((ESpending)(i + baseSettingNum)));
    	    } else {
    	        SettingT emptySetting;
    	        emptySetting.value = 0;
    	        emptySetting.desired = 0;
    	        emptySetting.rate = 0;
    	        emptySetting.benefit = 0;
    	        emptySetting.economy = 0;
    	        emptySetting.locked = false;
    			mSliders[i]->UpdateFromSetting(emptySetting);
    	    }
		    mSliders[i]->SetVisible(isAlly || detailVis);
		}
		mNewShipDestButton->StopBroadcasting();	// this keeps the SetValue() call from activatingÉ
		if (isOurs) {							// Éthe course plotter.
			mNewShipDestButton->Enable();
		} else {
			mNewShipDestButton->Disable();
		}
		bool bHasNSDest = !(aStar->GetNewShipDestination().IsNull());
		if (!isAlly) {
		    bHasNSDest = false;     // we never have info on the dest for opponents
		}
		mNewShipDestButton->SetValue(bHasNSDest);
		LCaption *theNSDest = (LCaption*) FindPaneByID(5);
		std::string descStr;
		if (bHasNSDest) {	// set the new ship destination string, v1.2b7
    		LStr255 s = aStar->GetNewShipDestination().GetDescription(descStr);
			theNSDest->SetDescriptor(s);
		} else {
			theNSDest->SetDescriptor("\p");
		}
		mNewShipDestButton->StartBroadcasting();
		mNewShipDestButton->SetVisible(isAlly);
		mNewShipDestButton->Refresh();
		mLinker->ListenToMessage(msg_SlidersReset, NULL);
		LStdPopupMenu *hullTypeMenu = (LStdPopupMenu *) FindPaneByID(cmd_ChangeShipType);
		hullTypeMenu->StopBroadcasting();
		SInt32 value = aStar->GetBuildHullType();
		hullTypeMenu->SetValue(value+1);
		hullTypeMenu->StartBroadcasting();
		hullTypeMenu->SetVisible(isAlly || detailVis);
		LCaption *thePos = (LCaption*) FindPaneByID(1);
		LStr255 s = aStar->GetPosition().GetDescription(descStr);
		thePos->SetDescriptor(s);
		LCaption *theTechLevel = (LCaption*) FindPaneByID(2);
        if (isAlly || (vis >= visibility_Tech)) {
		    s = (long)aStar->GetTechLevel();
		} else {
		    s = "?";
		}
		theTechLevel->SetDescriptor(s);
		mPrevTTNS = 0;
		UpdateShipCompletion();
		LCaption *thePopulation = (LCaption*) FindPaneByID(4);
		long pop = aStar->GetPopulation();
		if (isAlly || (vis >= visibility_Class)) {
    		s = pop;
    		if (pop>0) {	// format the population number with commas
    			int len = s.Length();
    			if (len > 3) {
    				s.Insert(',', len-2);
    			}
    			if (len > 6) {
    				s.Insert(',', len-5);
    			}
    			if (len > 9) {
    				s.Insert(',', len-8);
    			}
    			s += "\p,000";
    		}
		} else {
		    s = "?";
		}
		thePopulation->SetDescriptor(s);
//		thePopulation->SetVisible(isAlly);
		SetItemEnable('bEvn', isOurs);
		SetItemEnable('bMan', isOurs);
		SetItemEnable(cmd_ChangeShipType, isOurs);
	}
}

#pragma mark -

// **** System Ships Panel (bxSh) ****

CbxShPanel::CbxShPanel(LStream *inStream)
: CBoxView(inStream) {
	mShipsTable = NULL;
	mFleetTable = NULL;
	mAtThingy = NULL;
}

void
CbxShPanel::Hide() {
	CBoxView::Hide();
			// whenever this item is shown again, we will get an
			// UpdatePanel() call, and we can set the active table there.
	if ((CNewTable::sActiveTable == mShipsTable)||(CNewTable::sActiveTable == mFleetTable)) {
		CNewTable::sActiveTable = NULL;
	}
}

void
CbxShPanel::FinishCreateSelf() {
	CBoxView::FinishCreateSelf();
	mFleetTable = (CNewTable*) FindPaneByID('fltL');	//get fleet list, which is the main list of ships and fleets/groups
	ThrowIfNil_(mFleetTable);							// within a particular star or fleet (or other ship container)
	mFleetTable->AddListener(this);	// listen to your table and you will learn many things...
	mShipsTable = (CNewTable*) FindPaneByID('shpL');	//get ships list, which is the sub-list of ships within a fleet
	ThrowIfNil_(mShipsTable);
	mShipsTable->AddListener(this);
}

void
CbxShPanel::ObeyPanelCommand(MessageT inCommand) {
	if (inCommand == 'rght') {
		DoPanelButton('bSet', 0);	// right arrow is same as look at button
	}
}

/*----------------------------------------------------------------------------------------
ListenToMessage() - handles special messages send to the Ships In System panel

INPUT			a message code, and a pointer to a block of data, whose structure
				depends on the message sent.

SIDE EFFECTS	Buttons on the panel may be updated, enabled or disabled.
				If the user double-clicked on an item in the list, the Look At button will
				be "pressed", which will cause a the item that was double-clicked on
				(taken from the active list) to be selected in the starmap.
				All this updating also generates a lot of calls to Refresh() -- queuing 
				screen redraw events.

OUTPUT			none
----------------------------------------------------------------------------------------*/
void
CbxShPanel::ListenToMessage(MessageT inMessage, void *ioParam) {
	CellInfoT *cellInfo = (CellInfoT*) ioParam;
	CShipTable *table = (CShipTable*)CNewTable::sActiveTable;
	CShip *theShip = NULL;
//	ThingyRef aRef;
	ASSERT(table != NULL);
	if ((table != mFleetTable) && (table != mShipsTable)) {
		table = (CShipTable*)mFleetTable;	// ¥¥¥ this assumes what in overall program structure?
	}
	if (inMessage == message_TableActivated) {
		TableCellT cell;
		table->GetSelectedCell(cell);
		table->GetCellData(cell, &theShip);
		UpdateButtons(theShip);
	} else if (inMessage == message_CellDoubleClicked) {	// press the 'bSet' button, which is
		DoPanelButton('bSet', 0);					// seen by the user as the Look At button
	} else if (inMessage == message_CellSelected) {
		ASSERT(ioParam != NULL);
		CShip *theShip = NULL;
		AGalacticThingy* it = mGame->GetSelectedThingy();
		it = ValidateThingy(it);
		if (it == NULL) {
			DEBUG_OUT("Ships List Panel got message but nothing selected in starmap", DEBUG_ERROR);
		} else {
			table->GetCellData(cellInfo->cell, &theShip);
			UpdateButtons(theShip);
		}
	} else {
		CBoxView::ListenToMessage(inMessage, ioParam);
	}
}

void 
CbxShPanel::DoPanelButton(MessageT inMessage, long inValue) {
	CShip *theShip = NULL;
	TableCellT cell;
	CShipTable *table = (CShipTable*) CNewTable::sActiveTable;
	if ((table != mFleetTable) && (table != mShipsTable)) {
		table = (CShipTable*)mFleetTable;
	}
	table->GetSelectedCell(cell);
	table->GetCellData(cell, &theShip);
	theShip = ValidateShip(theShip);
	if (!theShip) {
		return;
	}
	if (inMessage == 'bSet') { // goto ship button pressed
		theShip->Select();
		if (theShip->GetThingySubClassType() == thingyType_Fleet) {
			CBoxView* bar = mGame->GetCurrButtonBar();
			if (bar) {												// try to choose
				bar->SimulateButtonClick(cmd_SetPanelThird, true);	// the ships in fleet panel
			}
		}
	} else if (inMessage == 'bDel') {	// scrap ship button pressed
		theShip->ToggleScrap();	// this toggles the ship's dead flag
		if (theShip->IsToBeScrapped()) {		// if the ship is being scrapped, mark it
			mGame->ThingyAttendedTo(theShip);	// as attended to
		}
		theShip->Persist();  // make sure this is immediately updated on the host
		UpdateButtons(theShip);
		table->Refresh();
			// refresh ships in fleet list if was a fleet whose contents are in
			// that list that we just scrapped
		if ((theShip->GetThingySubClassType() == thingyType_Fleet) && (table == mFleetTable)) {
			int fltNum = ((CShipTable*)mFleetTable)->GetActiveFleetNum();
			int selNum = cell.row;
			// v1.2b11d3, check for fltNum == 0 since it appears to not always be set
			if ((fltNum == 0) || (fltNum == selNum)) {
				mShipsTable->Refresh();
			}
		}
	} else if (inMessage == 'bPat') {	// patrol button pressed
		theShip->SetPatrol(inValue);
		if (inValue == 1) {
			theShip->ClearCourse(true, kRefresh);	// clear the entire course
			theShip->UserChangedCourse();
			mGame->ThingyAttendedTo(theShip);
		}
		theShip->Refresh();
		theShip->Changed();
		UpdateButtons(theShip);
			// refresh ships in fleet list if was a fleet whose contents are in
			// that list that we just put changed patrol status for
		if ((theShip->GetThingySubClassType() == thingyType_Fleet) && (table == mFleetTable)) {
			int fltNum = ((CShipTable*)mFleetTable)->GetActiveFleetNum();
			int selNum = cell.row;
			// v1.2b11d3, check for fltNum == 0 since it appears to not always be set
			if ((fltNum == 0) || (fltNum == selNum)) {
				mShipsTable->Refresh();
			}
		}
		Waypoint currPos = theShip->GetPosition();
		if (currPos.GetType() == wp_Star) {
			CStar* atStar = (CStar*)currPos.GetThingy();
			if (atStar) {
				atStar->RecalcPatrolStrength();
				atStar->Refresh();
			}
		}
		table->Refresh();
	} else if (inMessage == 'bExg') {	// exhanging ships between fleets
		CShipTable *dstTable;
		AGalacticThingy *dstThingy;
		int fltNum = ((CShipTable*)mFleetTable)->GetActiveFleetNum();
		int selNum = cell.row;
		bool adjustFltNum = false;
		bool createdFleet = false;
		if (table == mFleetTable) {	// figure out where we are moving the ship to.
			dstTable = (CShipTable*)mShipsTable;
			if (fltNum) {
				if (selNum < fltNum) {		// moving from in front of active fleet,
					adjustFltNum = true;	// so active fleet will shift upward in list
				}
				cell.row = fltNum;
				mFleetTable->GetCellData(cell, &dstThingy);	// replace ship in list with new fleet
			} else {						// we need to create the destination fleet
				CStarMapView* theMapView = mGame->GetStarMap();
				CFleet *theFleet = new CFleet(theMapView, mGame);	// than for the ships table
				dstThingy = theFleet;
				dstThingy->SetPosition(theShip->GetPosition(), kRefresh);	// dst fleet has position ship being moved
				dstThingy->SetOwner(theShip->GetOwner());
                dstThingy->Persist();
                createdFleet = true;    // we created the fleet, which means we are getting a message back from the host
                                        // with the official ID number
				LStr255 s(STRx_General, str_New);				// Set name to New
				theFleet->SetName(s);
//				s.Assign(dstThingy->GetName());	// must set name after calling Persist()
				mFleetTable->SetCellData(cell, dstThingy);		// replace ship in list with new fleet
				((CShipTable*)mFleetTable)->SetActiveFleetNum(cell.row);	// make the new fleet active
				CThingyList *dstArray = (CThingyList*) dstThingy->GetContentsListForType(contents_Ships);
				if (dstArray) {  // v2.0.9d2, list could be NULL
    				mShipsTable->InstallArray(dstArray, false);		// put new array into dst table
    				mAtThingy->Changed();
				} else {
				    DEBUG_OUT(dstThingy << " GetContentsListForType(contents_Ships) returned NULL", DEBUG_ERROR | DEBUG_CONTAINMENT);
				}
			}
		} else {
			dstTable = (CShipTable*)mFleetTable;		// moving from subfleet into main fleet/star
			dstThingy = mAtThingy;						// so get selected thingy
		}
		if (dstThingy == theShip) {
			return;										// don't add anything to itself
		}
		AGalacticThingy* oldContainer = theShip->GetPosition().GetThingy(); // save the previous location
		if (dstThingy->AddContent(theShip, contents_Ships, kRefresh)) {	// add the ship to the fleet
		    if (mGame->IsClient() && !mGame->IsSinglePlayer()) {
                GalacticaClient* client = dynamic_cast<GalacticaClient*>(mGame);
                if (client) {
                    // inform the host that we think we should be able to relocate this ship into a different container
                    // the host may tell us we failed, and we will clean up that mess if we get a failure back in the response.
                    // until then, we just assume that everything worked fine
                    client->SendReassignThingyRequest(theShip, oldContainer);
                } else {
                    // this is bad, it means client and host now disagree on what is where
                    // it will be corrected when the user reenters the game or gets and update of those objects from the
                    // host, so it shouldn't destabilize the host, but it could cause problems for the client.
                    // on the other hand, this shouldn't be possible at all, since a multiplayer client should
                    // always be able to dynamically cast to a GalacticaClient object
                    DEBUG_OUT("Couldn't get game client pointer to send ReassignThingyRequest", DEBUG_ERROR);
                }
		    }
		} else {
			SysBeep(1);
		}
		mGame->ThingyAttendedTo(theShip);
		if (adjustFltNum) {
			((CShipTable*)mFleetTable)->SetActiveFleetNum(fltNum - 1);	// re-mark the fleet active
		}
		table->ArrayChanged();
		dstTable->ArrayChanged();
		table->Refresh();
		dstTable->Refresh();
		table->GetSelectedCell(cell);
		table->GetCellData(cell, &theShip);
		UpdateButtons(theShip);
	} else {
		DEBUG_OUT("Ship in System panel got button press from an unknown button", DEBUG_ERROR);
	}
}

/*----------------------------------------------------------------------------------------
UpdateButtons() - Set the buttons in the panel to the appropriate state for the
				item passed in

INPUT			a ptr to a valid CShip (or decendant), or NULL

SIDE EFFECTS	Buttons on the panel are updated, which generates a lot of
				calls to Refresh() -- queuing screen redraw events.

OUTPUT			none.
----------------------------------------------------------------------------------------*/
void
CbxShPanel::UpdateButtons(CShip* inShip) {
	bool bDisableButtons = true;
	bool onPatrol = false;
	bool patEnabled = false;
	StopListening();		// ignore all messages while we mess with the buttons
	if (mAtThingy && inShip) {
		UInt32 shipOwner = inShip->GetOwner();
		UInt32 atOwner = mAtThingy->GetOwner();
		UInt32 player = mGame->GetMyPlayerNum();
	  #ifdef GAME_CONFIG_HOST
		player = shipOwner;	// in Game config host we always want to be able to change things
	  #endif // GAME_CONFIG_HOST
		bool exgEnabled = false;
		EnableItem('bSet');		// look at button is always enabled if anything is selected
	  #ifdef DEBUG
		if (gAllowChangeAnything) {
			player = shipOwner;
			atOwner = shipOwner;
		}
	  #endif
		if (shipOwner == player) {
			bDisableButtons = false;	// we will have some buttons enabled			
			// scrap button is the simplest
			if (atOwner == player) {	// your ship at someone else's planet
				EnableItem('bDel');
			} else {
				DisableItem('bDel');
			}
			// figure out if the exchange button can be enabled
			onPatrol = inShip->IsOnPatrol();
			if (!onPatrol) {	// don't allow items on patrol to be swapped into fleets
				CShipTable *table = (CShipTable*) CNewTable::sActiveTable;
				if (table == mShipsTable) {	// if the active table is the ships list, we can
					exgEnabled = true;		// always enable the exchange button
				} else { 
					int fltRow = ((CShipTable*)mFleetTable)->GetActiveFleetNum();
					if (fltRow < 1) {		// no fleet currently selected, always
						exgEnabled = true;	// enable exchange
					} else {
						TableCellT cell;
						table->GetSelectedCell(cell);
						if (fltRow != (int)cell.row) {
							exgEnabled = true;
						}		// disable exchange if active fleet is also selected item
					}
				}
			}
			SetItemEnable('bExg', exgEnabled);
			// now deal with patrol button
			bool mustPatrol = (inShip->GetShipClass() == class_Satellite);
			if (mustPatrol) {
				DisableItem('bPat');	// can't change patrol status of satellites
			} else if (inShip->GetPosition().GetType() == wp_Star) {
				EnableItem('bPat');	// only ships at a star can patrol
				patEnabled = true;
			} else {
				DisableItem('bPat');
			}
		}
	} else {
		DisableItem('bSet');	// nothing to even look at
	}
	SetItemValue('bPat', patEnabled && onPatrol);	// set patrol button's value
	if (bDisableButtons) {
		DisableItem('bPat');
		DisableItem('bExg');
		DisableItem('bDel');
	}
	StartListening();
}


static bool AllySearchFunc(const void* item, void* searchData) {
	const AGalacticThingy* it = static_cast<const AGalacticThingy*>(item);
	UInt8 playerNum = (int)searchData;
	if (it->IsAllyOf(playerNum)) {
		return true;
	}
	return false;
}

/*----------------------------------------------------------------------------------------
UpdatePanel() - Adjust the panel for a new selected thingy

INPUT			a ptr to a valid GalacticThingy, or NULL

SIDE EFFECTS	Buttons on the panel are updated, a new fleet table is installed, 
				and the ships table is cleared. The fleet table is made the active
				table (receives navigation keystrokes). Then its first item is selected,
				which causes the mFleetTable to send a message_CellSelected to
				this->ListenToMessage(). All this reloading also generates a lot of
				calls to Refresh() -- queuing screen redraw events.

OUTPUT			none.
----------------------------------------------------------------------------------------*/
void
CbxShPanel::UpdatePanel(AGalacticThingy* inThingy) {

	ASSERT_STR(inThingy == ValidateThingy(inThingy), "CbxShPanel::UpdatePanel() called for invalid thingy.");
	inThingy = ValidateThingy(inThingy);
	mAtThingy = inThingy;
	UpdateButtons(NULL);	// ERZ v1.2b8d4, make sure buttons are updated correctly
	if (inThingy) {
		mShipsTable->InstallArray(NULL, false);			// ships table should start empty
		bool isAlly = inThingy->IsAllyOf(mGame->GetMyPlayerNum());
		UInt8 vis = inThingy->GetVisibilityTo(mGame->GetMyPlayerNum());
		if ((inThingy->GetThingySubClassType() == thingyType_Star) && !inThingy->IsOwned()) { 
			// unowned star, check to see if we have a ship there
			if (inThingy->SearchContents(contents_Ships, AllySearchFunc, (void*)mGame->GetMyPlayerNum()) != 0) {
				vis = visibility_Details;	// make it visible if we have a ship there
			}
		} else if (inThingy->GetThingySubClassType() == thingyType_Fleet) {
			// this is a fleet, so if we can see the fleet class, then let us see the fleet details
			if (vis >= visibility_Class) {
				vis = visibility_Details;
			}
		}
		if (isAlly || (vis >= visibility_Details)) {
		  #warning TODO: details visibility, use the visibility level of individual ships at system
    		int shipCount;
    		CThingyList *theShipRefs = (CThingyList*) mAtThingy->GetContentsListForType(contents_Ships);
    		if (!theShipRefs) { // v2.0.9d2, could return null
    			DEBUG_OUT(mAtThingy << " GetContentsListForType(contents_Ships) returned NULL", DEBUG_ERROR | DEBUG_CONTAINMENT);
    		    return;
    		};
    		mFleetTable->InstallArray(theShipRefs, false);	// table can't dispose of array
    		((CFleetTable*)mFleetTable)->SetActiveFleetNum(0);
    		CNewTable::sActiveTable = mFleetTable;
    		TableCellT cell = {0, 0};
    		mFleetTable->SelectCell(cell);
    		shipCount = theShipRefs->GetCount();
    		if (shipCount > 0) {
    			int i;
    			TableCellT cell = {1, 1};
    			ASSERT(sPrevThingy == ValidateThingy(sPrevThingy));
    			sPrevThingy = ValidateThingy(sPrevThingy);
    			if (sPrevThingy) {		// find previously selected thingy in list and select it. This
    				CShip* it;			// makes use of left arrow key (select container) more intuitive.
    				DEBUG_OUT("Looking for previous thingy in list "<<sPrevThingy, DEBUG_TRIVIA | DEBUG_USER);
    				for (i = 1; i <= shipCount; i++) {
    					it = (CShip*)theShipRefs->FetchThingyAt(i);
    					if (it == sPrevThingy) {
    						cell.row = i;
    						DEBUG_OUT("Prev thingy found at item "<<i, DEBUG_TRIVIA | DEBUG_USER);
    						break;		// found it, stop searching
    					}
    				}
    			}
    			DEBUG_OUT("Selecting item "<<cell.row, DEBUG_TRIVIA | DEBUG_USER);
    			mFleetTable->SelectCell(cell);
    		}
		} else {
		    mFleetTable->InstallArray(NULL, false); // not ally, don't show
		}
		LPane *scroller = FindPaneByID(pane_Scroller);
		if (scroller) {	// panel has scroller, enable it so we can use it, must be disabled in PPob
			scroller->Enable();
		} else {
			DEBUG_OUT("No upper scroller found in panel", DEBUG_ERROR);
		}
		scroller = FindPaneByID(pane_Scroller+1);
		if (scroller) {	// panel has scroller, enable it so we can use it
			scroller->Enable();
		} else {
			DEBUG_OUT("No lower scroller found in panel", DEBUG_ERROR);
		}
	}
}


