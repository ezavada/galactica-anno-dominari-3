// Galactica Panes.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ
// 9/21/96 added 1.0.8 changes

#include "pdg/sys/platform.h"

#include "GalacticaPanes.h"
#include "CStar.h"
#include "Galactica.h" // for YieldTimeToForegoundTask()
#include "GalacticaPanels.h"
#include "GalacticaTutorial.h"
#include "GalacticaGlobals.h"
#include "GalacticaDoc.h"
#include "GalacticaProcessor.h"
#include "GalacticaHostDoc.h"
#include "CShip.h"
#include "CFleet.h"

#include "CHelpAttach.h"
#include "LAnimateCursor.h"
#include "CTextTable.h"

#include <TArrayIterator.h>

#if defined( COMPILER_GCC ) && defined ( PLATFORM_MACOSX )
    #include <cmath>
    // OS X with XCode 1.5 doesn't have hypot in std namespace
    namespace std {
        using ::hypot;
    }
#endif

#ifdef __MWERKS__
#include "Profiler.h"
#endif 

//#pragma mark === globals ===

const long	simulateClick_Duration	= 8;	// Keep hot spot hilited for
											// this many ticks when simulating
											// a click (HIG p205)

bool				CStarMapView::sCursorLoaded = false;
bool				CStarMapView::sAwaitingClick = false;
EClickTypes			CStarMapView::sClickType = clickType_None;
Cursor				CStarMapView::sCrossHairCursor;
Cursor				CStarMapView::sArrowCursor;
Cursor				CStarMapView::sHandCursor;
Cursor				CStarMapView::sXCursor;
Cursor				CStarMapView::sMenuHandCursor;
bool				CStarMapView::sBackgroundLoaded = false;
PicHandle			CStarMapView::sBackgroundImage = 0;

//#pragma mark -
// **** STARMAP VIEW ****

void
CStarMapView::ClickReceived() {
	sAwaitingClick = false; 
	sClickType = clickType_None;
	AGalacticThingyUI *last = AGalacticThingyUI::GetHilitedThingy();
	if (last) {
		last->DrawHilite(false);
	}
}

CStarMapView::CStarMapView() 
: CZoomView(), LListener() {
	InitStarMap();
}

CStarMapView::CStarMapView(const SPaneInfo &inPaneInfo, const SViewInfo &inViewInfo)
: CZoomView(inPaneInfo, inViewInfo), LListener() {
	InitStarMap();
}

CStarMapView::CStarMapView(LStream *inStream)
: CZoomView(inStream), LListener() {
	InitStarMap();
}


void
CStarMapView::InitStarMap(void) {
	RGBColor white = {0xffff,0xffff,0xffff};
//	RGBColor black = {0,0,0};
	RGBColor veryDarkGrey = {0x4444,0x4444,0x4444};
	SetForeAndBackColors(&white, &veryDarkGrey);
	mNumStars = -1;
	mSectorSize = 1;
	if (!sCursorLoaded) {
		CursHandle cursH;
		cursH = ::MacGetCursor(CURS_CrossHair);
		HLock((Handle) cursH);
		sCrossHairCursor = **cursH;		
		MacAPI::ReleaseResource((Handle)cursH);
		cursH = ::MacGetCursor(CURS_Hand);
		HLock((Handle) cursH);
		sHandCursor = **cursH;
		MacAPI::ReleaseResource((Handle)cursH);
		cursH = ::MacGetCursor(CURS_X);
		HLock((Handle) cursH);
		sXCursor = **cursH;
		MacAPI::ReleaseResource((Handle)cursH);
		cursH = ::MacGetCursor(CURS_MenuHand);
		HLock((Handle) cursH);
		sMenuHandCursor = **cursH;
		MacAPI::ReleaseResource((Handle)cursH);
        UQDGlobals::GetArrow(&sArrowCursor);
		sCursorLoaded = true;
	}
	if (!sBackgroundLoaded) {
		sBackgroundLoaded = true;
		sBackgroundImage = MacAPI::GetPicture(PICT_StarmapBackground);
	}

	mGame = NULL;
	GalacticaShared* game = GalacticaDoc::GetStreamingDoc();
	if (game == NULL) {
	    game = GalacticaShared::GetStreamingGame();
	}
	if (game) {
	    mGame = game->AsSharedUI();
	}
	ASSERT_STR(mGame != NULL, "StarMapView initialized with mGame=NULL");
	ThrowIfNil_(mGame);
	mDisplayMode = displayMode_Owner;
}


void
CStarMapView::SetSectorSize(SInt32 inSize) {
	if (mSectorSize != inSize) {
		mSectorSize = inSize;
		float oldZoom = GetZoom();	// get old zoom as float
		ZoomTo(1.0, false);			// adjust to natural size, no refresh
		mImageSizeNorm.width = PIXELS_PER_SECTOR * mSectorSize;	// calc the zoomed size based on sector size
		mImageSizeNorm.height = PIXELS_PER_SECTOR * mSectorSize;
		ZoomTo(2.0, false);			// we zoom twice to make sure everything is recalculated
		ZoomTo(oldZoom, false);		// and restore our old zoom level
	}
}

// the data written is different than what is used for initialization from a 'PPob' 
// resource stream.
// This method writes the data needed to restore the state of a StarMap AFTER it has already
// been initialized some other way. It complements the ReadStream() method.
void
CStarMapView::WriteStream(LStream *inStream) {
	AGalacticThingy	*aThingy;
	UInt32 numPanes = mSubPanes.GetCount();
	ThrowIfNil_(mGame);
	*inStream << numPanes << mNumStars << mSectorSize;
	for (UInt32 i = 1; i <= numPanes; i++) {
		LPane* it;
		Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
		mSubPanes.FetchItemAt(i, it);
		aThingy = static_cast<AGalacticThingyUI*>(it);
		ThrowIfNil_(aThingy);
		*inStream << aThingy->GetThingySubClassType();
		aThingy->WriteStream(inStream);
	}
}

// this is different than initialization from a 'PPob' resource stream
// it reads the data needed to restore the state of a StarMap AFTER it has already
// been initialized some other way. It complements the WriteStream() method.
bool
CStarMapView::ReadStream(LStream *inStream, UInt32 checkVersion) {
	AGalacticThingy	*aThingy;
	UInt32 numPanes;
	long aThingyCode;
	bool bResult = true;  // v2.1b9r7, return a result code so we know if we got a complete read
	*inStream  >> numPanes >> mNumStars >> mSectorSize;
    DEBUG_OUT("Reading " << numPanes << " thingys ("<<mNumStars<<" stars) from (" << (void*)inStream->GetMarker() << ").", DEBUG_DETAIL);
    UInt32 i;
    try {
    	for (i = 1; i <= numPanes; i++) {
    		Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
    		*inStream >> aThingyCode;
    		aThingy = mGame->MakeThingy(aThingyCode);
    		if (aThingy) {
    			aThingy->ReadStream(inStream, checkVersion);
    			DEBUG_OUT("Read (stream) " << aThingy << " from (" << (void*)inStream->GetMarker() << ")", DEBUG_DETAIL);
        		// 1.2b5, 1/21/98, ignore objects which were created and intended for 
        		// deletion when the game was saved.
        		if (aThingy->GetID() == PaneIDT_Unspecified) {
        			DEBUG_OUT("Removing incomplete object "<<aThingy<<" found in saved turn", DEBUG_IMPORTANT);
        			delete aThingy;	// delete this 
        		}
    		}
    	}
	}
	catch (...) {
	    // v2.1b9r7, failures give a partial read
	    DEBUG_OUT("Failed to read all "<< numPanes << " thingys; only read " << i-1, DEBUG_ERROR);
        #warning FIXME: Need to add alert to inform user that they only got a partial game load
        bResult = false;
	}
    DEBUG_OUT("Done reading thingys.", DEBUG_DETAIL);
	FinishStreaming();
	return bResult;
}

void
CStarMapView::FinishStreaming() {
    DEBUG_OUT("CStarMapView::FinishStreaming", DEBUG_IMPORTANT);
	AGalacticThingyUI	*aThingy;
	UInt32 numPanes = mSubPanes.GetCount();
	for (UInt32 i = 1; i <= numPanes; i++) {
		register LPane* it;
		Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
		mSubPanes.FetchItemAt(i, it);
		aThingy = (AGalacticThingyUI*)it;
		ThrowIfNil_(aThingy);
		aThingy->CalcPosition(false);	// v1.2b8, removed from FinishStreaming()
	}
}


void
CStarMapView::GalacticToImageCoord(Point3D &ioPt) {
	float zoom = GetZoom();
	float a = ioPt.x * zoom;
	ioPt.x = a;
	a = ioPt.y * zoom;
	ioPt.y = a;
	a = ioPt.z * zoom;
	ioPt.z = a;	// no need to adjust for outside QD coordinates here, cause we have long coords
}

void
CStarMapView::GalacticToImagePt(Point &ioPt) {
	float zoom = GetZoom();
	SPoint32 imagePt32;
	float a = ioPt.h * zoom;
	imagePt32.h = a;
	a = ioPt.v * zoom;
	imagePt32.v = a;
	ImageToLocalPoint(imagePt32, ioPt);	// v1.2d8, this should adjust for outside QD coordinates
}

void
CStarMapView::PortToGalacticaCoord(Point inPortPt, Point3D &outPt) {
	Point localPt;
	localPt.h = inPortPt.h + mPortOrigin.h;	// Port to Local coordinates
	localPt.v = inPortPt.v + mPortOrigin.v;
	outPt.x = (float)localPt.h/(float)mSectorSize * (float)mImageSizeNorm.width / GetZoom();	// ZOOMING
	outPt.y = (float)localPt.v/(float)mSectorSize * (float)mImageSizeNorm.height / GetZoom();
	outPt.z = 0;
}

void
CStarMapView::ImageToGalacticaCoord(SPoint32 inImagePt, Point3D &outPt) {
#pragma unused(inImagePt, outPt)
}

void
CStarMapView::ScrollToThingy(AGalacticThingy *inThingy) {
//	GalacticaApp::RefreshScreen();
	UpdatePort();
	Point p = inThingy->GetPosition().GetLocalPoint();	// center object in local/image coordinates
	Point3D pos;
	pos.x = p.h;
	pos.y = p.v;
	pos.z = 0;
	GalacticToImageCoord(pos);
	if ( ! ImagePointIsInFrame(pos.x, pos.y) ) {
		long dh = mFrameSize.width>>1;	// this will adjust so we put the object in the center
		long dv = mFrameSize.height>>1;	// of the scrolling frame rather than the top left corner
		ScrollImageTo(pos.x - dh, pos.y - dv, true);
//		ReconcileFrameAndImage(true);	// doesn't adjust for past top.
        AGalacticThingyUI* ui = inThingy->AsThingyUI();
        ThrowIfNil_(ui);
		ui->Refresh();
		UpdatePort();
//		GalacticaApp::RefreshScreen();
	}
}


void
CStarMapView::ScrollBits(SInt32 inLeftDelta, SInt32 inTopDelta) {
	AGalacticThingyUI *it = AGalacticThingyUI::GetHilitedThingy();
	if (it) {
		it->DrawHilite(false);	// this should keep things from being drawn
	}							// half-hilited while autoscrolling
	it = mGame->GetSelectedThingy();	
    if (it) {
    	it->EraseSelectionMarker();
    }
	GalacticaDoc* game = dynamic_cast<GalacticaDoc*>(mGame);
	if (game && game->DrawStarmapNebulaBackground() ) {
		Refresh();	// make sure the whole thing is invalidated
	} else {
		::BackColor(blackColor);		// this way starmap is erased to black during autoscroll
		LView::ScrollBits(inLeftDelta, inTopDelta);
	}
}



void
CStarMapView::ZoomTo(float inZoom, Boolean inRefresh) {
//    ProfilerInit(collectDetailed, bestTimeBase, 1000, 100);
	Boolean centerOnThingy = false;
	AGalacticThingyUI* focus = nil;
	if (inZoom > 16.0) {
		inZoom = 16.0;	// limit zoom in to 16:1
	}
	AGalacticThingyUI *last = AGalacticThingyUI::GetHilitedThingy();
	if (last) {
		last->DrawHilite(false);
		focus = last;	// focus on the highlighted item if there is one
		// еее rather than centering on the highlighted item, we really should make it be
		// еее in the same place on the screen, and everything else should zoom around it
	}
	if (!focus) {
		focus = mGame->GetSelectedThingy();
	}
	if (focus) {
		focus->EraseSelectionMarker();	// 1.2b8d5, eliminate graphic artifacts
		Point p = focus->GetPosition().GetLocalPoint();	// center object in local/image coordinates
		Point3D pos;
		pos.x = p.h;
		pos.y = p.v;
		pos.z = 0;
		GalacticToImageCoord(pos);
		if ( ImagePointIsInFrame(pos.x, pos.y) ) { // only center on the thingy if it was in view
			centerOnThingy = true;				   // to begin with
		}
	}
	CZoomView::ZoomTo(inZoom, inRefresh);
	if (centerOnThingy) {
		ScrollToThingy(focus);
	}
//	ProfilerDump("\pprofile dump");
//	ProfilerClear();
}


/* CStarMapView::ResolveOverlappingPanes

This makes sure the correct pane is selected when a collection of panes that are overlapping.

It does this by:
	1. Checking to see if there are any other Galactic objects with centerpoints that are
		closer than a certain distance
	2. Building a menu of those overlapping items and displaying it to the user, with the
		item that was actually clicked on selected.
	3. Recording the user's choice, or NIL if no item selected, in the ioClickedPaneP 
		parameter

*/
#define MIN_DIST 10
#define TEMP_POPUP_ID 2500
#define MENU_TEXT_TRAITS_ID 130

void
CStarMapView::ResolveOverlappingPanesFromPoint(Point &clickPt, StdThingyUIList* inPopupList, LPane** ioClickedPaneP) {
	StdThingyUIList* foundList;
	int numFound = 0;
	int initialItem = 0;
	int myPlayerNum = 0;
    GalacticaDoc* gamedoc = dynamic_cast<GalacticaDoc*>(mGame);
    if (gamedoc) {
        myPlayerNum = gamedoc->GetMyPlayerNum();
    }
	if (inPopupList) {	// list passed in, so just display it
		foundList = inPopupList;
	} else {
		foundList = new StdThingyUIList();
		int numItems = mSubPanes.GetCount();
		for (int i = 1; i <= numItems; i++) {
			LPane* it;
			mSubPanes.FetchItemAt(i, it);
			AGalacticThingyUI *aNeighbor = static_cast<AGalacticThingyUI*>(it);
			Point p = aNeighbor->GetDrawingPosition();
			// if (PointsAreClose(p, clickPt)) еее use this instead of doing own calculations??
			p.h -= clickPt.h;
			p.v -= clickPt.v;
			if ( (p.h > MIN_DIST) || (p.h < -MIN_DIST) || (p.v > MIN_DIST) || (p.v < -MIN_DIST) )
				continue; 	// skip this item, it's too far away 
			if (aNeighbor->IsDead())
				continue;	// skip this one, it's dead
			if (aNeighbor->GetThingySubClassType() == thingyType_Fleet) {
				if (static_cast<CFleet*>(aNeighbor)->IsSatelliteFleet()) {
					continue;	// skip satellite fleets
				}
			}
			if (aNeighbor->GetThingySubClassType() == thingyType_Ship) {
				if (static_cast<CShip*>(aNeighbor)->GetShipClass() == class_Satellite) {
					continue;	// skip this one, it's a satellite
				}
				if (static_cast<CShip*>(aNeighbor)->IsOnPatrol()) {
					continue;	// not interested in patrolling ships, either
				}
			}
			if (IS_ANY_MESSAGE_TYPE(aNeighbor->GetThingySubClassType()) ) {
			    DEBUG_OUT("Why is there a message contained in the starmap?", DEBUG_ERROR);
				continue;	// skip this one, it's a message
			}
	        AGalacticThingy *aContainer = aNeighbor->GetContainer();
			if (aContainer != nil) {
				if (aContainer->GetThingySubClassType() == thingyType_Fleet)
					continue;	// skip this one too, it's inside a fleet
			}
			// fog of war
			if (aNeighbor->GetThingySubClassType() != thingyType_Star) {
    			if (!(aNeighbor->IsAllyOf(myPlayerNum) || aNeighbor->IsVisibleTo(myPlayerNum))) {
    			    continue; // we can't see this item
    			}
			}
			numFound++;
			if (aNeighbor == *(reinterpret_cast<AGalacticThingyUI**>(ioClickedPaneP))) {
				initialItem = numFound;	// which one did we initially click on?
			}
			foundList->insert(foundList->end(), aNeighbor);	// this item is overlapping
		}
	}
	numFound = foundList->size();
	if (numFound < 1) {
		return;		// nothing found that was close enough to be overlapping
	}
 // ERZ, v1.2b8, 8/14/98 we actually want to show the menu even when there is only one thing
 	if (sAwaitingClick && (numFound < 2)) {
 		return;		// unless of course we are waiting for a click.
 	}

// Build a menu of those overlapping items and displaying it to the user, with the
//		item that was actually clicked on selected.
	MenuHandle theMenu = ::NewMenu(TEMP_POPUP_ID, "\p");
	MacAPI::MacInsertMenu(theMenu, hierMenu);
	
	int i = 1;
	for (StdThingyUIList::iterator p = foundList->begin(); p < foundList->end(); p++, i++) {
		LStr255 name;
		AGalacticThingyUI* it = *p;
		Boolean doBold = false;
		Boolean doItalics = false;
		Boolean doRedUnderline = false;
		name.Assign(it->GetName());			    // get the name of the Nth item in the list
		if (name.Length() == 0) {				// did we find something without a name?
			name.Assign(STRx_General, str_Unknown_);	// give them the name "unknown"
			doItalics = true;							// and put it in italics
		}
		MacAPI::MacAppendMenu(theMenu, name);			// add it to the end of the menu
		long itemType = it->GetThingySubClassType();
		if ((itemType == thingyType_Ship) || (itemType == thingyType_Fleet)) {
			if (static_cast<CShip*>(it)->CoursePlotted()) {
				doBold = true;	// items with courses plotted are done in bold
			}
			if (static_cast<CShip*>(it)->IsToBeScrapped()) {
				doRedUnderline = true;
			}
		}
		short style = normal;
		if (doItalics) {
			style |= italic;
		}
		if (doBold) {
			style |= bold;
		}
		if (doRedUnderline) {
			style |= underline;
		}
		MacAPI::SetItemStyle(theMenu, i, style);
		char markChar = ' ';
		switch (itemType) {		// we put particular marks beside certain things
			case thingyType_Rendezvous:
				markChar = 'x';	// rendezvous points
				break;
			case thingyType_Fleet:
				markChar = '╞';	// fleets
				break;
			case thingyType_Star:
				markChar = '*';	// stars
				break;
			case thingyType_Ship:
				markChar = '^'; // ships
			default:
				break;
		}
		::SetItemMark(theMenu, i, markChar);
		
		// v1.2b11d3, show items that are scrapped in red underline
		if (doRedUnderline) {
			MCEntry mce;
			mce.mctID = TEMP_POPUP_ID;
			mce.mctItem = i;
			mce.mctRGB1.red = 0;
			mce.mctRGB1.green = 0;
			mce.mctRGB1.blue = 0;
			mce.mctRGB2.red = 0xffff;
			mce.mctRGB2.green = 0;
			mce.mctRGB2.blue = 0;
			mce.mctRGB3.red = 0;
			mce.mctRGB3.green = 0;
			mce.mctRGB3.blue = 0;
			mce.mctRGB4.red = 0;
			mce.mctRGB4.green = 0;
			mce.mctRGB4.blue = 0;
			mce.mctReserved = 0;
			SetMCEntries(1, &mce);
		}
	}
	LocalToPortPoint(clickPt);
	PortToGlobalPoint(clickPt);
	
	
/*	SInt16 sysFont = LMGetSysFontFam();			// save the old system font and size
	SInt16 sysFontSize = LMGetSysFontSize();	// and use the Galactica Font
	LMSetSysFontFam(gGalacticaFontNum);			// for the Popup Menu
	LMSetSysFontSize(10);
this didn't work consistantly, so I have disabled it */	

	long menuAndItem = MacAPI::PopUpMenuSelect(theMenu, clickPt.v, clickPt.h, initialItem);
	
/*	LMSetSysFontFam(sysFont);			// restore the old system font and size
	LMSetSysFontSize(sysFontSize);
*/
	::MacDeleteMenu(TEMP_POPUP_ID);
	::DisposeMenu(theMenu);

// Record the user's choice, or NIL if no item selected, in the ioClickedPaneP 
//		parameter
	if (menuAndItem) {
		menuAndItem &= 0xffff; // mask to get the menu item
		*ioClickedPaneP = foundList->at(menuAndItem-1);
	} else {
		*ioClickedPaneP = NULL;
	}
	if (!inPopupList) {
		delete foundList;	// there was no list passed in, so we created the list
	}
}

// if inPopupList is not NIL, don't search for nearby items, just display the list passed in
void
CStarMapView::ResolveOverlappingPanes(LPane** ioClickedPaneP, StdThingyUIList* inPopupList) {
// Check to see if there are any other Galactic objects with centerpoints that are
//		closer than a minimum distance

	ASSERT(ioClickedPaneP != NULL);
	if (*ioClickedPaneP == NULL) {
		return;		// didn't click on a pane
	}
	if (sAwaitingClick && (GetClickType() == clickType_Rendezvous)) {
		*ioClickedPaneP = NULL;		// placing a Rendezvous point
		return;						// we never want to put it into an item
	}
	LPane* saveClickedPane = *ioClickedPaneP;	// v1.2fc1, try to default to a pane if nothing selected
	Point clickPt = static_cast<AGalacticThingyUI*>(*ioClickedPaneP)->GetDrawingPosition();
	ResolveOverlappingPanesFromPoint(clickPt, inPopupList, ioClickedPaneP);
	if (*ioClickedPaneP == NULL) {
		*ioClickedPaneP = saveClickedPane;
	}
}


void
CStarMapView::ListenToMessage(MessageT inMessage, void *ioParam) {
    if (inMessage == msg_BroadcasterDied) {
        return; // XCode Build -- this keeps us from doing a fatal dynamic_cast<> below
    }
	SInt32 value = 0;	// v1.2fc3, check for null ioParam before dereferencing it
	if (ioParam != NULL) {
		value = *((long*)ioParam);
	}
  #if TUTORIAL_SUPPORT
	if (Tutorial::TutorialIsActive()) {
		if ( (inMessage == cmd_ZoomIn) || (inMessage == cmd_ZoomOut) ) {
			Tutorial* t = Tutorial::GetTutorial();
			if (t->GetPageNum() == tutorialPage_StarmapMove) {
				// we are on page 7, waiting for them to try zooming in and out
				if (TutorialWaiter::GetTutorialWaiter()) {
					// they have already started zooming, so now we give them a 
					// chance to finish
					TutorialWaiter::ResetTutorialWaiter();
				} else {
					new TutorialWaiter(3);
				}
			}
		}
	}
  #endif //TUTORIAL_SUPPORT
	GalacticaDoc* game = dynamic_cast<GalacticaDoc*>(mGame);
	switch (inMessage) {
		case msg_ControlClicked:	// sent when any control clicked on, ignore
			break;
		case cmd_ToggleBalloons:
		case cmd_RenameItem:
		case cmd_EndTurn:
		    if (game) {
			    game->ObeyCommand(inMessage, ioParam);
			}
			Refresh();
			break;
		case cmd_ZoomIn:
			ZoomIn();
			break;
		case cmd_ZoomOut:
			ZoomOut();
			break;
		case cmd_ZoomFill:
			ZoomFill();
			break;
		case cmd_DisplayNone:		// these handle the display mode for items in the starmap
		case cmd_DisplayProduct:	// depending on which mode is used, the brightness of the
		case cmd_DisplayTech:		// items is represents various attributes of the items.
		case cmd_DisplayDefense:
		case cmd_DisplayDanger:
		case cmd_DisplayGrowth:
		case cmd_DisplayShips:
		case cmd_DisplayResearch:
			if (value) {
				mDisplayMode = inMessage - cmd_DisplayNone;
				Refresh();
			}
			break;
		case message_CellSelected:
		case message_CellDoubleClicked:
		    if (game) {
			    game->ObeyCommand(inMessage, ioParam);
			}
			break;
        case msg_BroadcasterDied:
            break;
		default:
			DEBUG_OUT("StarMap View received unknown message "<<inMessage, DEBUG_TRIVIA | DEBUG_USER);
//			SignalPStr_("\pStarMap View received unknown message");
			break;
	}
}

void
CStarMapView::Click(SMouseDownEvent &inMouseDown) { // Check if a SubPane of this View is hit
    GalacticaDoc* game = dynamic_cast<GalacticaDoc*>(mGame);
	if (IsOptionKeyDown()) {	// track mouse and move pane along with it
	    AGalacticThingy* it = NULL;
    	it = mGame->GetSelectedThingy();
		bool wasDrawn = false;
		if (mCurrTracker) {
			wasDrawn = mCurrTracker->IsDrawn();
			if (wasDrawn) {
				mCurrTracker->Hide();
			}
		}
		FocusDraw();
		MacAPI::SetOrigin(0,0);
   	    OutOfFocus(this); // make sure we refocus, cause we changed the origin
		Point currentPoint, lastPoint, moveDelta;
		MacAPI::GetMouse(&lastPoint);
		while (MacAPI::StillDown()) {
			if (it) {
				it->AsThingyUI()->UpdateSelectionMarker(::TickCount());
			}
			FocusDraw();
			MacAPI::SetOrigin(0,0);
			OutOfFocus(this); // make sure we refocus, cause we changed the origin
			MacAPI::GetMouse(&currentPoint);
			if (MacAPI::EqualPt(currentPoint, lastPoint)) {	// scroll image if the mouse moved.
				continue;
			}
			moveDelta.h = lastPoint.h - currentPoint.h;
			moveDelta.v = lastPoint.v - currentPoint.v;
			lastPoint = currentPoint;
			if (it) {
				it->AsThingyUI()->EraseSelectionMarker();
			}
			ScrollImageBy(moveDelta.h, moveDelta.v, true);
			if (it) {
				it->AsThingyUI()->DrawSelectionMarker();
			}
	    #if TARGET_API_MAC_CARBON
         MacAPI::QDFlushPortBuffer(GetMacPort(), NULL);
       #endif
			OutOfFocus(this);
		}
		if (wasDrawn) {
			mCurrTracker->Show();
		}
	} else {
		LPane *clickedPane = FindSubPaneHitBy(inMouseDown.wherePort.h, inMouseDown.wherePort.v);
		bool doResolveOverlapping = false;
		UStarPopupItemCountAction* popupItemsAction = nil;
		StdThingyUIList* popupArray = NULL;
		if (sAwaitingClick || IsControlKeyDown() || GalacticaApp::IsRightClick() ) {
			doResolveOverlapping = true;	// v1.2b8, only do popup if control is down
		} else if (clickedPane) {			// or awaiting a click
			AGalacticThingyUI* theThingy = ValidateThingyUI(static_cast<AGalacticThingyUI*>(clickedPane));
			if (!theThingy) {
				DEBUG_OUT("Clicked on invalid thingy {"<<(void*)clickedPane<<"}", DEBUG_ERROR | DEBUG_USER);
				clickedPane = nil;
			} else if (theThingy->IsAtStar()) {	// it was actually a thingy, check to see if at star
				AGalacticThingy* theStar = theThingy->GetContainer();
				theStar = ValidateThingy(theStar);
				ASSERT_STR(theStar != nil, theThingy->GetContainer() << " is invalid, but should be a star");
                if (theStar) {
    				ASSERT_STR(theStar->GetThingySubClassType() == thingyType_Star, theStar << " is not a star, but "<<theThingy<<" thinks it is");
    				CThingyList* shipsList = (CThingyList*) theStar->GetContentsListForType(contents_Ships);
    				if (!shipsList) { // v2.0.9, 11/24/01, check for nil contents list returned
    				    DEBUG_OUT(theStar << " (should be star) returned nil contentsListForType(contents_Ships)", DEBUG_ERROR | DEBUG_CONTAINMENT);
                    } else {
        				popupItemsAction = new UStarPopupItemCountAction();
        				shipsList->DoForEntireList(*popupItemsAction);
        				popupArray = popupItemsAction->GetStarPopupItems();
        				if (popupArray) {
            				// v2.0.9d2 debugging for problem with popup array
            				DEBUG_OUT("pre-GetCount() for popup array "<<(void*)popupArray << " from "<<theStar, DEBUG_DETAIL | DEBUG_USER);
            				if (popupArray->size() > 1) {
            					doResolveOverlapping = true;
            				}
            				DEBUG_OUT("post-GetCount()", DEBUG_DETAIL | DEBUG_USER);
        				}
    				}
				}
			}
		} else {	// if clicked in empty space, try to find nearby items
			// еее this is almost certianly the wrong coordinate system
	//		ResolveOverlappingPanesFromPoint(inMouseDown, popupArray, &clickedPane);
		}
		if (doResolveOverlapping) {
			ResolveOverlappingPanes(&clickedPane, popupArray);
		}
		if (popupItemsAction) {
			delete popupItemsAction;	// cleanup the popup items action, which cleans up the popup array, too
		}
		if (sAwaitingClick) {
			MapClickT theClickInfo;
			theClickInfo.mouseEvent = &inMouseDown;	//send mousedown event with the waypoint clicked
			if (clickedPane) {
				theClickInfo.clickedWP = static_cast<AGalacticThingyUI*>(clickedPane);
			} else {	// nil for clickedPane means we clicked in open space
				Point3D coord;
				PortToLocalPoint(inMouseDown.wherePort);
				coord.x = (float)inMouseDown.wherePort.h/(float)mSectorSize * (float)mImageSizeNorm.width / GetZoom();	// ZOOMING
				coord.y = (float)inMouseDown.wherePort.v/(float)mSectorSize * (float)mImageSizeNorm.height / GetZoom();
				coord.z = 0;
				theClickInfo.clickedWP = coord;
			}
			if (sClickType == clickType_Waypoint) {
            	if (game) {
    				CBoxView* thePanel = game->GetActivePanel();	// selected a waypoint
    				if (thePanel) {
    					thePanel->ListenToMessage(msg_ClickedInMap, &theClickInfo);
    				}
				}
				sAwaitingClick = false;
				sClickType = clickType_None;
			} else if (sClickType == clickType_Rendezvous) {
			    if (game) {
    				if (game->ObeyCommand(cmd_NewRendezvous, &theClickInfo)) {	// placed a rendezvous point
    					sAwaitingClick = false;
    					sClickType = clickType_None;
    				}
				}
			}
		} else if (clickedPane != nil) {		// SubPane is hit, let it respond to the Click
			clickedPane->Click(inMouseDown);
		} else {	// clicked in background of starmap
			LPane::Click(inMouseDown);			// let LPane process click on this View
			mGame->SetSelectedThingy(nil);		// go back to galaxy map and deselect thingy
		}
	}
}

// gets the boundaries of the playing area in Galactic coordinates
// еее this assumes 16x16 ly per 1x1 sector, which is not correct, should be 64x64 ly 
/*void
CStarMapView::CalcSectorBounds(Point3D &topLeftFront, Point3D &botRightRear) {
	topLeftFront.x = 0;						// 1 = 140,000,000 km or 90,000,000 miles
	topLeftFront.y = 0;						// 0x10000 = 1 light year
	topLeftFront.z = 0;
	botRightRear.x = PIXELS_PER_SECTOR * 1024L * mSectorSize; // 1048576 = 0x10.0000
	botRightRear.y = PIXELS_PER_SECTOR * 1024L * mSectorSize; //             ^^ ^^^^---- 65 thousandths of light year
	botRightRear.z = PIXELS_PER_SECTOR * 1024L * mSectorSize; //              |___ light years (hi word)
}
*/

/*void
CStarMapView::CreateStars(SInt32 inNumStars) {
	const Boolean kNoRecalc = false;
	CStar *myStar = nil;
	Point3D p;
	mNumStars = inNumStars * (mSectorSize * mSectorSize);
	for (int i = 0; i < mNumStars; i++) {
		Galactica::Globals::GetInstance().sBusyCursor->Set();		// put up our busy cursor.
		p.x = Rnd(PIXELS_PER_SECTOR * mSectorSize) * 1024L;	// еее this assumes 16x16 ly per 1x1 sector
		p.y = Rnd(PIXELS_PER_SECTOR * mSectorSize) * 1024L;	// еее which is not correct
		p.z = 0;
		myStar = new CStar(this, mGame);
		myStar->SetCoordinates(p, kNoRecalc);	// the recalc will be done in FinishInitSelf()
		myStar->Persist();	// star saves itself into database
	}
}
*/

void
CStarMapView::AdjustMouse(Point inPortPt, const EventRecord &inMacEvent, RgnHandle ioMouseRgn) {
	if (IsOptionKeyDown() == true) {	// these are global to entire map
		MacAPI::MacSetCursor(&sHandCursor);
	} else if (sClickType == clickType_Rendezvous) {
		MacAPI::MacSetCursor(&sXCursor);
	} else if (IsControlKeyDown() == true) {
		MacAPI::MacSetCursor(&sMenuHandCursor);
	} else {
		CZoomView::AdjustMouse(inPortPt, inMacEvent, ioMouseRgn);
	}
}


void
CStarMapView::AdjustMouseSelf(Point, const EventRecord&, RgnHandle) {
	if (!sAwaitingClick) {
		::MacSetCursor(&sArrowCursor);
	} else {
		AGalacticThingyUI *last = AGalacticThingyUI::GetHilitedThingy();
		if (last) {
			last->DrawHilite(false);
		}
		MacAPI::MacSetCursor(&sCrossHairCursor);
	}
}

void
CStarMapView::Draw(RgnHandle inSuperDrawRgnH) {
//	mGame->HideSelectionMarker();
	LView::Draw(inSuperDrawRgnH);
}


void
CStarMapView::DrawSelf() {
	Rect	frame;
	CalcLocalFrameRect(frame);
	RgnHandle saveClipRgn = MacAPI::NewRgn();	// make an empty region in which to save the clip region
	MacAPI::GetClip(saveClipRgn);				// save current clip region
	RgnHandle tempRgn = MacAPI::NewRgn();		// make a temporary working region
	MacAPI::MacCopyRgn(mUpdateRgn, tempRgn);	// and fill it with a copy of mUpdateRgnH member variable
	MacAPI::MacOffsetRgn(tempRgn, mPortOrigin.h, mPortOrigin.v);	// that was created in LView::Draw(), then
	MacAPI::SetClip(tempRgn);					// convert it to LocalCoordinates and install it as clipRgn 
//	ForeColor(yellowColor);	// еее╩debugging
//	MacAPI::PaintRgn(tempRgn);
	MacAPI::PenNormal();
    GalacticaDoc* game = dynamic_cast<GalacticaDoc*>(mGame);
	
	// Put code to draw a background image here:
	if (sBackgroundImage && (game && game->DrawStarmapNebulaBackground())) {
		MacAPI::DrawPicture(sBackgroundImage, &frame);
	} else {
		ForeColor(blackColor);
		MacAPI::PaintRect(&frame);
	}
	
    // draw the speres of influence
	if (game && game->DrawRangeCircles()) {
		for (int level = 0; level < 3; level++) {
			for (int playerNum = 1; playerNum <= game->GetNumPlayers(); playerNum++) {
				TArrayIterator<LPane*> iterator(mSubPanes);
				LPane	*theSub;
				RgnHandle influenceRgn = MacAPI::NewRgn();	// make an empty region in which to save the sphere of influence
				while (iterator.Next(theSub)) {
				    AGalacticThingyUI* aThingy = dynamic_cast<AGalacticThingyUI*>(theSub);
				    if (aThingy && (aThingy->GetOwner() == playerNum) && aThingy->ShouldDraw()) {
						RgnHandle tempRgn = MacAPI::NewRgn();		// holds region of a single star for union operations
						MacAPI::OpenRgn();
					    bool didDrawing = aThingy->DrawSphereOfInfluence(level);
						MacAPI::CloseRgn(tempRgn);	// save all the outlines drawn since the OpenRgn call into the temporary region
						if (didDrawing) {
							MacAPI::MacUnionRgn(tempRgn, influenceRgn, influenceRgn); // aggregate them into the influence region
						}
						MacAPI::DisposeRgn(tempRgn);
					}
				}
			    // map the region, which was drawn at zoom = 0.25, to the current zoom
			    Rect srcRect;
			    srcRect.top = 0;
			    srcRect.left = 0;
			    srcRect.bottom = mImageSizeNorm.height/4L;
			    srcRect.right = mImageSizeNorm.width/4L;
			    Rect dstRect;
			    dstRect.top = 0;
			    dstRect.left = 0;
			    dstRect.bottom = (short)((float)mImageSizeNorm.height * GetZoom());
			    dstRect.right = (short)((float)mImageSizeNorm.width * GetZoom());
			    MacAPI::MapRgn(influenceRgn, &srcRect, &dstRect);
				// choose the drawing color
			    RGBColor scanColor = *(game->GetColor(playerNum, false));
			    float brightness = (game->DrawStarmapNebulaBackground()) ? 0.1f : 0.0625f;
			    if (level > 0) {
			    	brightness *= (1.5f * level);
			    }
			    BrightenRGBColor(scanColor, brightness);
			    short penMode = addMax; // (game->DrawStarmapNebulaBackground()) ? addMax : srcCopy;
				MacAPI::PenMode(penMode);
			    MacAPI::RGBForeColor(&scanColor);
			    // draw the region
				MacAPI::MacPaintRgn(influenceRgn);
				MacAPI::PenMode(srcCopy);
				MacAPI::DisposeRgn(influenceRgn);
			}
		}
	}
	

	// draw the gridlines
    bool drawgrid = true;
    if (game && !game->DrawGrid()) {
        drawgrid = false;
    }
	if (drawgrid) {
	    MacAPI::PenMode(addMax);
		float zoom = GetZoom();
		float lineOffset[3];					// pixels between lines for given zoom level
		lineOffset[2] = PIXELS_PER_SECTOR * zoom;	// sector boundaries
		lineOffset[1] = (PIXELS_PER_SECTOR * zoom)/10;	// grid line every 10th sector
		lineOffset[0] = lineOffset[1] / 10;				// 10 sub-lines for every grid line
		long imageOffsetH = mFrameLocation.h - mImageLocation.h;	// figure out how far the image
		long imageOffsetV = mFrameLocation.v - mImageLocation.v;	// is offset from the frame
		for (int level = 0; level < 3; level++) {
//			if (lineOffset[level] < 20)			// if lines would be closer than 20 pixels apart,
			if (lineOffset[level] < 10)			// if lines would be closer than 10 pixels apart,
				continue;						// then they are too closely spaced, so skip them
/*			if (level == 2) {
				SetRGBForeColor(0x6666, 0x6666, 0x8888);	// color varies according to grid level
			} else if (level == 1) {
				SetRGBForeColor(0x4444, 0x4444, 0x5555);
			} else {
				SetRGBForeColor(0x2222, 0x2222, 0x3333);
			}
*/
			long colorVal = ((lineOffset[level] - 10) * 0x0033L); // v2.0.8, improved gridline visuals
			if (colorVal > 0x6666) {
			    colorVal = 0x6666;
			}
			SetRGBForeColor(colorVal, colorVal, colorVal);
			long lineHCount = 3 + mFrameSize.width / lineOffset[level];	// rough number of lines to
			long lineVCount = 3 + mFrameSize.height / lineOffset[level];// draw for the visible area
			long numSkipped = imageOffsetH / lineOffset[level];
			for (int i = 0; i < lineHCount; i++) {
				long lineHPos = (float)(numSkipped+i) * lineOffset[level];
				short h = lineHPos + mPortOrigin.h + mImageLocation.h;
				MacAPI::MoveTo(h, frame.top);
				MacAPI::MacLineTo(h, frame.bottom);
				// еее put code to draw horizontal grid numbers here
//				MacAPI::MoveTo(h+2, frame.top + 10);
//				MacAPI::TextSize(10);
//				MacAPI::DrawChar('0');
			}
			numSkipped = imageOffsetV / lineOffset[level];
			for (int i = 0; i < lineVCount; i++) {
				long lineVPos = (float)(numSkipped+i) * lineOffset[level];
				short v = lineVPos + mPortOrigin.v + mImageLocation.v;
				MacAPI::MoveTo(frame.left, v);
				MacAPI::MacLineTo(frame.right, v);
				// еее put code to draw vertical grid numbers here
			}
		}
	}
	MacAPI::PenMode(srcCopy);
	MacAPI::SetClip(saveClipRgn);			// restore previous value
	MacAPI::DisposeRgn(saveClipRgn);		// not needed any more
	MacAPI::DisposeRgn(tempRgn);
}

void
CStarMapView::EndOfTurn(long inTurnNum) {
	LArrayIterator iterator(mSubPanes, LArrayIterator::from_Start);	// all stars are at beginning of list
	AGalacticThingy	*aThingy;
	sAwaitingClick = false;
	int i = 0;
	while ( iterator.Next(&aThingy) ) {
		Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
		ThrowIfNil_(aThingy);
		aThingy->EndOfTurn(inTurnNum);
		i++;
		if ((i % 10)==0)
			GalacticaApp::YieldTimeToForegoundTask();		// give up time to forground process
	}
	iterator.ResetTo(LArrayIterator::from_Start);	// go back and make sure all the ships are aimed right
	while (iterator.Next(&aThingy)) {
		Galactica::Globals::GetInstance().getBusyCursor()->Set();		// put up our busy cursor.
		ThrowIfNil_(aThingy);
		aThingy->FinishEndOfTurn(inTurnNum);				// clean up any loose ends 
		i++;
		if ((i % 10)==0)
			GalacticaApp::YieldTimeToForegoundTask();		// give up time to forground process
	}
}

void
CStarMapView::ZoomSubPane(LPane *inSub, float) { // deltaZoom) {
	((AGalacticThingyUI*)inSub)->CalcPosition(false);
}


#pragma mark -

bool
UStarPopupItemCountAction::DoAction(LArray &inAffectedArray, ArrayIndexT inItemIndex, void* inItemData) {
#pragma unused (inAffectedArray, inItemIndex)
	ThingyRef* aRef = (ThingyRef*)inItemData;
	if (!mItems) {
		mItems = new StdThingyUIList();
	}
	if (aRef->GetThingyID() != PaneIDT_Undefined) {
		CShip* aShip = (CShip*) aRef->GetThingy();
		if (!aShip) {
			return false;
		}
		if (aShip->GetThingySubClassType() == thingyType_Fleet) {	// fleets
		    CFleet* fleet = dynamic_cast<CFleet*>(aShip);
			if (fleet && fleet->IsSatelliteFleet()) {
				return false;	// skip satellite fleets
			}
		}
		if (aShip->GetThingySubClassType() == thingyType_Ship) {
			if (aShip->GetShipClass() == class_Satellite) {
				return false;	// skip this one, it's a satellite
			}
			if (aShip->IsOnPatrol()) {
				return false;	// not interested in patrolling ships, either
			}
			GalacticaDoc* game = aShip->GetGameDoc();
			if (game && (aShip->GetVisibilityTo(game->GetMyPlayerNum()) < visibility_Class)) {
			    return false;
			}
		}
		mItems->insert(mItems->end(), aShip); // add item to a list
	}
	return false;
}

#pragma mark -

#define DIST_FONT_SIZE 10

CCoursePlotter::CCoursePlotter(CStarMapView* inStarMap, Waypoint inFromWP, long inSpeed) 
	: CMouseTracker(inStarMap) {
	float zoomFactor = (float) PIXELS_PER_SECTOR / inStarMap->GetZoom();
	mFromWP = inFromWP;
	mSpeed = inSpeed;
	Point3D from = inFromWP;	// waypoint gives us image coordinates
	mAdjustedSpeed = (float)mSpeed / zoomFactor;
	mFromPt.h = (float)from.x / zoomFactor;
	mFromPt.v = (float)from.y / zoomFactor;
//	((CStarMapView*)mWithinView)->SetPlotter(this);
}

CCoursePlotter::~CCoursePlotter() {
//	((CStarMapView*)mWithinView)->SetPlotter(nil);
}

void
CCoursePlotter::AdjustZoomed() {	// recalc from point to compensate for zooming
	float zoomFactor = (float) PIXELS_PER_SECTOR / mWithinView->GetZoom();
	*((long*)&mLastMousePt) = -1;
	Point3D from = mFromWP;	// waypoint gives us image coordinates
	mAdjustedSpeed = (float)mSpeed / zoomFactor;
	mFromPt.h = (float)from.x / zoomFactor;
	mFromPt.v = (float)from.y / zoomFactor;
}

void
CCoursePlotter::PrepareToDraw() {
	MacAPI::PenNormal();
	MacAPI::PenMode(patXor);
	MacAPI::ForeColor(whiteColor);
	if (mSpeed) {
		MacAPI::TextFont(gGalacticaFontNum);
		MacAPI::TextSize(DIST_FONT_SIZE);
		MacAPI::TextMode(srcXor);
	}	
}

void
CCoursePlotter::DrawSelf(Point inWherePt) {
	Point fromPt;
	mWithinView->ImageToLocalPoint(mFromPt, fromPt);
	MacAPI::MoveTo(fromPt.h, fromPt.v);
	MacAPI::MacLineTo(inWherePt.h, inWherePt.v);
	if (mAdjustedSpeed) {	// now calculate the number of turns to the destination
		long dist = std::hypot( (double) inWherePt.h - fromPt.h, (double) inWherePt.v - fromPt.v );
		long turns = (dist/mAdjustedSpeed) + 1;
		turns = Max(turns, 1);
		if (turns != mLastTurns) {
			mLastTurnsStr.Assign(turns);
			mLastStrWidth = ::StringWidth(mLastTurnsStr);
		}
		if (inWherePt.h < fromPt.h)	{ // draw number of turns in new location
			MacAPI::Move(-4 - mLastStrWidth, -2);
		} else {
			MacAPI::Move(4, -2);
		}
		MacAPI::DrawString(mLastTurnsStr);
	}
}

void	
CCoursePlotter::EraseSelf(Point inWherePt) {
	DrawSelf(inWherePt);	// erasing is same as drawing
}


#pragma mark -
// **** new TOGGLE BUTTON ****

CNewToggleButton::CNewToggleButton(LStream *inStream)
 : LToggleButton(inStream) {
 	short helpIndex = mUserCon;
 	if (helpIndex < 0) {
 		helpIndex = -helpIndex;
 	}
	CHelpAttach* theBalloonHelp = new CHelpAttach(STRx_StarmapHelp, helpIndex);
	AddAttachment(theBalloonHelp);
}

void
CNewToggleButton::SetValue(SInt32 inValue) {
	LToggleButton::SetValue(inValue);
	if (inValue == Button_On) {
		BroadcastMessage(msg_ControlClicked, (void*) this);
	}
}

// differs slightly from LControl version
void
CNewToggleButton::SimulateHotSpotClick(SInt16 inHotSpot) {
	if (mEnabled == triState_On) {
		if (IsVisible()) {
			unsigned long	ticks;
			HotSpotAction(inHotSpot, true, false);	// Do action for click inside
			::Delay(simulateClick_Duration, &ticks);// Wait so user can see effect
			HotSpotAction(inHotSpot, false, true);	// Undo visual effect
		}
		HotSpotResult(inHotSpot);				// Perform result of click
	}
}

void
CNewToggleButton::DrawGraphic(ResIDT inGraphicID) {
	Rect	frame;
	CalcLocalFrameRect(frame);
	StColorPenState::Normalize();
	
	if (mGraphicsType == 'ICN#') {
		if (mEnabled != triState_Off) {
//			::PlotIconID(&frame, atNone, ttNone, inGraphicID);
		} else {
			if (mValue == 1)
				inGraphicID = mOnID;
			else
				inGraphicID = mOnID + 3;
//			::PlotIconID(&frame, atNone, ttSelected, inGraphicID); //do disabled drawing
		}
		MacAPI::PlotIconID(&frame, atNone, ttNone, inGraphicID);
	} else {
		LToggleButton::DrawGraphic(inGraphicID);
	}
}


void
CNewToggleButton::HotSpotResult(SInt16) {
	if (mUserCon < 0) {				// negative means use as radio button
		FocusDraw();
		DrawGraphic(mOnID);
		SetValue(Button_On);		// Clicking sets it, like radio button
	} else {
		SetValue(1 - GetValue());	// Toggle between on/off
	}
}

#pragma mark -

// **** new BUTTON ****

CNewButton::CNewButton(LStream *inStream)
 : LButton(inStream) {
 	short helpIndex = mUserCon;
 	if (helpIndex < 0) {
 		helpIndex = -helpIndex;
 	}
	CHelpAttach* theBalloonHelp = new CHelpAttach(STRx_StarmapHelp, helpIndex);
	AddAttachment(theBalloonHelp);
}

void
CNewButton::DrawGraphic(ResIDT inGraphicID) {
	Rect	frame;
	CalcLocalFrameRect(frame);
	StColorPenState::Normalize();
	
	if (mGraphicsType == 'ICN#') {
		if (mEnabled != triState_Off) {
			MacAPI::PlotIconID(&frame, atNone, ttNone, inGraphicID);
		} else {
			MacAPI::PlotIconID(&frame, atNone, ttNone, mNormalID + 2); //do disabled drawing
//			MacAPI::PlotIconID(&frame, atNone, ttSelected, mPushedID); //do disabled drawing
		}			// we use selected, because on a black background it looks disabled
	} else {
		LButton::DrawGraphic(inGraphicID);
	}
}
