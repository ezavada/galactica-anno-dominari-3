// AGalacticThingyUI.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ 4/18/96
// 9/21/96, 1.0.8 fixes incorporated
// 1/11/98, 1.2, converted to CWPro1
// 9/14/98, 1.2b8, Changed to ThingyRefs, CWPro3
// 4/3/99,	1.2b9, Changed to support Single Player Unhosted Games, CWPro4
// 3/14/02, 2.1b5, Separated from AGalacticThingy


#include "AGalacticThingyUI.h"
#include "CStar.h"
#include "CMessage.h"
#include "CShip.h"
#include "CFleet.h"
#include "CRendezvous.h"
#include "LAnimateCursor.h"
#include "TArrayIterator.h"
#include "GalacticaDoc.h"
#include "GalacticaHostDoc.h"
#include "GalacticaSingleDoc.h"
#include "GalacticaPackets.h"
#include "GalacticaClient.h"
#include "GalacticaPanes.h"
#include "GalacticaSharedUI.h"

#include <UDrawingUtils.h>

#include <string>

#define DRAG_SLOP_PIXELS 4
// define as 1 to use a single handle as a reusable buffer, 0 to create new handle each time
#define SHARED_THINGY_IO_BUFFER	1

// lowest brightness of thingies other than stars
#define LOWEST_BRIGHTNESS 0.1


// these are defined in Galactica.cp
typedef long **classNumPtr;
extern UInt32	gAGalacticThingyUIClassNum;
extern UInt32	gANamedThingyClassNum;
extern LAnimateCursor* gTargetCursor;

// uncomment the following line to have all thingy's pane frames drawn
//#define Draw_Thingy_Frames

/*#ifdef DEBUG
ostream& operator << (ostream& cout, AGalacticThingyUI *it) {
    if (!ValidateThingy(it)) {
        cout << " INVALID "<<(void*)it;
        return cout;
    }
	long type = it->GetThingySubClassType();
	cout << LongTo4CharStr(type) << " " << it->GetPaneID() 
		 << " [" << it->GetOwner() << "] {" << (void*)it << "}";
	if ((type == thingyType_Ship) || (type == thingyType_Star) || (type == thingyType_Fleet) || (type == thingyType_Rendezvous)) {
		LStr255 s;
		it->GetDescriptor(s);
		s[s.Length()+1] = '\0';
		cout << " \"" << (char*)&s[1] << "\"";
	}
	if (it->IsDead()) {
		cout << " DEAD";
	}
	return cout;
}
#endif
*/

#if defined(GAME_CONFIG_HOST)
static bool				gAllowDragAnything = true;
#elif defined(DEBUG)
  static bool      gAllowDragAnything = true;
#else
  #define          gAllowDragAnything false
#endif

bool				AGalacticThingyUI::sCursorLoaded = false;
Cursor				AGalacticThingyUI::sTargetCursor;
AGalacticThingyUI*	AGalacticThingyUI::sHilitedThingy = nil;
SInt32				AGalacticThingyUI::sOldHilitedPos = 0;
bool				AGalacticThingyUI::sDragging = false;
bool				AGalacticThingyUI::sAutoscrolling = false; 
bool				AGalacticThingyUI::sChangingHilite = false; 
UInt32				AGalacticThingyUI::sSelectionMarkerChangeTick = 0;


/*
AGalacticThingyUI*
AGalacticThingyUI::MakeUIThingyFromSubClassType(LView* inSuper, GalacticaShared* inGame, long inThingyType) {
	AGalacticThingyUI* aThingy = NULL;
	DEBUG_OUT("Make "<< LongTo4CharStr(inThingyType), DEBUG_DETAIL | DEBUG_OBJECTS);
	switch (inThingyType) {
		case thingyType_Star:
			aThingy = new CStar(inSuper, inGame);
			break;
		case thingyType_Ship:
			aThingy = new CShip(inSuper, inGame);
			break;
		case thingyType_Fleet:
			aThingy = new CFleet(inSuper, inGame);
			break;
		case thingyType_Wormhole:
//			aThingy = new CWormhole(inSuper, inGame);  // wormholes aren't implemented yet
			break;
		case thingyType_StarLane:
			aThingy = new CStarLane(inSuper, inGame);
			break;
		case thingyType_Rendezvous:
			aThingy = new CRendezvous(inSuper, inGame);
			break;
		case thingyType_Message:
//			ASSERT_STR(inSuper == nil, "Messages shouldn't have a superview");
			aThingy = new CMessage(inGame); 	//(inSuper);
			break;
		case thingyType_DestructionMessage:
//			ASSERT_STR(inSuper == nil, "Destruction Messages shouldn't have a superview");
			aThingy = new CDestructionMsg(inGame);	//(inSuper);
			break;
		default:
			DEBUG_OUT("Unknown Thingy Type "<< LongTo4CharStr(inThingyType), DEBUG_ERROR | DEBUG_OBJECTS);
			break;
	}
	return aThingy;
}
*/

// **** GALACTIC THINGY ****


AGalacticThingyUI::AGalacticThingyUI(LView *inSuperView, GalacticaShared* inGame, long inThingyType)
 : AGalacticThingy(inGame, inThingyType),
   LPane()
  
{
	InitThingyUI();
	PutInside(inSuperView, true);	// NULL safe, do call OrientSubPane() since we won't call FinishCreate()
}

/*
AGalacticThingyUI::AGalacticThingyUI(LView *inSuperView, GalacticaShared* inGame, LStream *inStream, long inThingyType) 
 : AGalacticThingy(inGame, inStream, inThingyType),
   LPane()
{
	SetPaneID(GetID());
	InitThingyUI();
	PutInside(inSuperView, false);	// don't do ThingMoved until after everything is created
}
*/

AGalacticThingyUI::~AGalacticThingyUI() {
	if (gAGalacticThingyUIClassNum == 0) {		// this is how we get the class number of a
		gAGalacticThingyUIClassNum = **((classNumPtr)this);	// partially constructed thingy
	}
}

void
AGalacticThingyUI::InitThingyUI() {
	DEBUG_OUT("InitThingyUI " << this, DEBUG_DETAIL | DEBUG_OBJECTS);
	mSelectionMarkerShown = false;
	mSelectionMarkerPos = 0;
	if (mGame) {
    	ASSERT_STR( mGame->AsSharedUI() != NULL, "Illegal creation of UI thingy without UI game");
    	ThrowIf_(mGame->AsSharedUI() == NULL); // would crash otherwise
	}
	if (!sCursorLoaded) {
		CursHandle	cursH;
		cursH = ::MacGetCursor(CURS_Target);
		HLock ((Handle) cursH);
		sTargetCursor = **cursH;		
		::ReleaseResource((Handle)cursH);
		sCursorLoaded = true;
	}
}

void	
AGalacticThingyUI::FinishInitSelf() {
}

void
AGalacticThingyUI::ReadStream(LStream *inStream, UInt32 streamVersion) {
    DEBUG_OUT("UI::ReadStream "<<this, DEBUG_TRIVIA | DEBUG_OBJECTS);
    AGalacticThingy::ReadStream(inStream, streamVersion);
	SetPaneID(GetID());
}

void
AGalacticThingyUI::Click(SMouseDownEvent &inMouseDown) {
	LPane::Click(inMouseDown);
	if (GetClickCount() == 1) {
		DragSelf(inMouseDown);		// give ourselves a chance to be dragged around
	}
}

void
AGalacticThingyUI::ClickSelf(const SMouseDownEvent&) {
    if (GetClickCount() == 1) {
    	Select();
    } else if (GetClickCount() == 2) {
  		AGalacticThingyUI* selected = mGame->AsSharedUI()->GetSelectedThingy();
        if (selected != this) {
        	ASSERT(selected != nil);
        	DEBUG_OUT("Double-clicked item "<<this<<" but pane "<<selected->GetPaneID()<<" was selected", DEBUG_ERROR | DEBUG_USER);
        	Select();
        }
        DoubleClickSelf();
    }
}

void
AGalacticThingyUI::DoubleClickSelf() {
	// default method ignores a double click
}

Boolean
AGalacticThingyUI::DragSelf(SMouseDownEvent &) {//inMouseDown) {
	if (mGame->GetTurnNum() != 1) {
	    // we can only drag things around on the first turn
	    return false;
	}
	if (mGame->IsClient() && !(mGame->GetNumHumans()==1)) { 
	    // we can only drag stuff on client in a single player game
		return false;
    }
	if (mGame->IsClient() && !gAllowDragAnything) {
	    // we can only drag stuff on client if gAllowDragAnything is set
		return false;
	}
	if (CStarMapView::IsAwaitingClick()) {	// if we are plotting a course, we don't
		return false;					// want to allow them to drag the thingy
    }
	Point currentPoint, firstPoint, lastPoint;
	Point clickOffset;  // pixels by which the click point was offset from the centerpoint
	bool dragging = false;
	FocusDraw();
	::GetMouse(&firstPoint);
	clickOffset.h = firstPoint.h - mDrawOffset.h;
	clickOffset.v = firstPoint.v - mDrawOffset.v;
	bool drawn = false;
	int deltaH, deltaV;
	while (::StillDown() && !dragging) {
		::GetMouse(&currentPoint);
		deltaH = currentPoint.h-firstPoint.h;
		deltaV = currentPoint.v-firstPoint.v;
		if ( (deltaH > DRAG_SLOP_PIXELS) || (deltaH < -DRAG_SLOP_PIXELS) || 
			 (deltaV > DRAG_SLOP_PIXELS) || (deltaV < -DRAG_SLOP_PIXELS) ) {
			dragging = true;
			lastPoint = firstPoint;
		}
	}
	if (dragging) {
		FocusDraw();
		SPoint32 lp;
		CStarMapView* map = static_cast<CStarMapView*>(mSuperView);
		ASSERT(map != nil);
		::PenMode(patXor);
		sDragging = true;
		while (::StillDown()) {
			::GetMouse(&currentPoint);
			if (*((long*)&lastPoint) != *((long*)&currentPoint)) {	// current point is mouse pos,
				if (drawn) {										// not drawing position.
					DrawSelf();		// erase
				}
				mDrawOffset.h = currentPoint.h - clickOffset.h;	// this makes it so the exact pixel
				mDrawOffset.v = currentPoint.v - clickOffset.v;	// that was clicked on in the target
				map->LocalToImagePoint(currentPoint, lp);		// remains under the mouse as we drag
				Point deltaPt = {0,0};
				sAutoscrolling = true;					// Autoscroll may do redraw, so setting
				map->AutoScrollDragging(lp, deltaPt);	// this flag tells CRendezvous::DrawSelf()
				sAutoscrolling = false;					// not to draw and thus avoid leaving tracks
				if (*((long*)&deltaPt) != 0) {	// if it was scrolled, then the Within view is
					FocusDraw();				// no longer in focus, so we need to set
					::PenMode(patXor);			// it up again
				}
				DrawSelf();
				drawn = true;
				lastPoint = currentPoint;
			}
		}
		if (drawn) {	// do final erase. Only needed because at high magnifications, the galactic
			DrawSelf();	// coordinates will not translate exactly to screen coords, so the item
		}				// we dragged may shift slightly.
		Point3D coord;
		float zoom = map->GetZoom();
//		map->PortToLocalPoint(lastPoint);
		lp.h = (long)mDrawOffset.h << 10L;	// draw offset always represents the exact center of the item,
		lp.v = (long)mDrawOffset.v << 10L;	// so we can use it to get the actual position in the starmap
		coord.x = (float)lp.h / zoom;	//еее incorrect assumptions about size
		coord.y = (float)lp.v / zoom;	// of sector, valid only for v1.2 and before еее
		coord.z = 0;
		// check for the item in a container, and remove them
		AGalacticThingy* container = mCurrPos.GetThingy();
		if (container) {
		    container->RemoveContent(this, contents_Any, kDontRefresh);
		}
		mCurrPos = coord;
		sDragging = false;
		Refresh();	// redraw original position
		// update Fog of War info
		UpdateWhatIsVisibleToMe();
		CalculateVisibilityToAllOpponents();
		ThingMoved(kDontRefresh);
		// now adjust anything that refers to this, telling it to recalc its position, dst, etc..
		UMoveReferencedThingysAction recalcAction(this);
		mGame->DoForEverything(recalcAction);
		if (mGame->IsHost() && !mGame->IsSinglePlayer()) {	// host must save changes now or they will be lost
			WriteToDataStore();
		}
		map->Refresh();
		map->UpdatePort();
	}
	return dragging;
}

void
AGalacticThingyUI::AdjustMouseSelf(Point, const EventRecord&, RgnHandle ) {
	if (!CStarMapView::IsAwaitingClick()) {
		::MacSetCursor(&CStarMapView::sArrowCursor);
	} else {
		DrawHilite(true);
//		::SetCursor(&sTargetCursor);
		gTargetCursor->Set();
	}
}

Boolean
AGalacticThingyUI::Contains(SInt32 inHorizPort, SInt32 inVertPort) const {
	if ( ShouldDraw() ) {
		Point p = mDrawOffset;
		ASSERT(mSuperView != nil);
		mSuperView->LocalToPortPoint(p);	// offset center point to match port coordinates
		Rect r = GetHotSpotRect();			// then calc the hotspot rect in port coordinates
		::MacOffsetRect(&r, p.h, p.v);
		if ( (inHorizPort >= r.left) &&
			 (inHorizPort < r.right) &&
			 (inVertPort >= r.top) &&
			 (inVertPort < r.bottom) ) {
			return PointIsInFrame(inHorizPort, inVertPort);
		} else {
			return false;	// only count points inside hot rect;
		}
	} else {
		return false;	// doesn't contain any points while not shown
	}
}

Boolean
AGalacticThingyUI::IsHitBy(SInt32 inHorizPort, SInt32 inVertPort) {
	if ( ShouldDraw() ) {
		Point p = mDrawOffset;
		ASSERT(mSuperView != nil);
		mSuperView->LocalToPortPoint(p);	// offset center point to match port coordinates
		Rect r = GetHotSpotRect();			// then calc the hotspot rect in port coordinates
		::MacOffsetRect(&r, p.h, p.v);
		if ( (inHorizPort >= r.left) &&
			 (inHorizPort < r.right) &&
			 (inVertPort >= r.top) &&
			 (inVertPort < r.bottom) ) {
			return LPane::IsHitBy(inHorizPort, inVertPort);
		} else {
			return false;	// only count clicks inside hot rect;
		}
	} else {
		return false;	// can't be hit by any clicks while not shown
	}
}


void
AGalacticThingyUI::DrawHilite(bool inHilite) {
	if (inHilite && (sHilitedThingy == this)) {	// don't draw again if we are already hilited
		return;
	}
	LView* super = GetSuperView();
	if (super == nil) {
		return;
	}
	if (inHilite) {
		if (sHilitedThingy) {
			sHilitedThingy->DrawHilite(false);
		}
		sHilitedThingy = this;
		sOldHilitedPos = super->GetSubPanes().FetchIndexOf(this);	// find ourself in super's list
		super->GetSubPanes().SwapItems(LArray::index_Last, sOldHilitedPos);	// put self in front
	} else if (sHilitedThingy == this) {
		if (sOldHilitedPos > 0) {
			super->GetSubPanes().SwapItems(LArray::index_Last, sOldHilitedPos);	// restore position
		}
		sHilitedThingy = nil;
	}
	sChangingHilite = true;		// this serves to flag FocusDraw() and DrawSelf() that we are doing a hilite or unhilite
	FocusDraw();
	Draw(nil);
	sChangingHilite = false;
}

ResIDT
AGalacticThingyUI::GetViewLayrResIDOffset() {
	return 0;	// default returns no offset
}

bool
AGalacticThingyUI::IsSelected() const {
    return (this == mGame->AsSharedUI()->GetSelectedThingy());
}

void
AGalacticThingyUI::Refresh() {
    GalacticaDoc* game = GetGameDoc();
    if (game) {
        game->HideSelectionMarker();
    } 
    LPane::Refresh();
}

void
AGalacticThingyUI::Changed() {
    mChanged = true;
    GalacticaDoc* game = GetGameDoc();
    if (game) {
        game->SetModified(true);
    }
} 

float
AGalacticThingyUI::GetBrightness(short inDisplayMode) {	//
	float currLevel, highestLevel, lowestLevel;
	GalacticaDoc* game = GetGameDoc();
	if (!game) {
	    return 1.0;
	}
	if (inDisplayMode == 0) {
		return 1.0;
	}
	highestLevel = game->GetHighest(inDisplayMode);	// the GameDoc pre-calcs these for each turn
	lowestLevel = game->GetLowest(inDisplayMode);		// in GalacticaDoc::RecalcHighestValues()
	if (inDisplayMode == displayMode_Product) {
		currLevel = GetProduction();
	}
	else if (inDisplayMode == displayMode_Tech) {
		currLevel = GetTechLevel();
	}
	else if (inDisplayMode == displayMode_Defense) {
		currLevel = GetDefenseStrength();
	}
	else if (inDisplayMode == displayMode_Danger) {
		currLevel = GetDangerLevel();
	} else {
		return LOWEST_BRIGHTNESS;	// default returns low brightness
	}
	return CalcBrightnessFromRange(currLevel, lowestLevel, highestLevel, LOWEST_BRIGHTNESS);
}

bool
AGalacticThingyUI::ShouldDraw() const {
    if ( (mCurrPos.GetType() == wp_Coordinates) || mCurrPos.IsDebris() ) {
        return true;
    } else {
        return IsSelected();
    }
}

void
AGalacticThingyUI::ThingMoved(bool inRefresh) {
    AGalacticThingy::ThingMoved(inRefresh);
	CalcPosition(inRefresh);
}

void
AGalacticThingyUI::CalcPosition(bool inRefresh) {
	if (!mSuperView) {  // v2.0.9, moved to top to eliminate
	    if (IS_ANY_MESSAGE_TYPE(GetThingySubClassType())) {
	        // messages don't have superviews, no big deal
	        DEBUG_OUT("Skipping CalcPosition for "<< this, DEBUG_TRIVIA | DEBUG_MESSAGE);
	    } else {
	        // anything else not having a superview is an error
      	    DEBUG_OUT("SKIPPING: CalcPosition() " << this<< " @ " << mCurrPos, DEBUG_ERROR | DEBUG_MOVEMENT);
      	}
		return;
	}
	Rect r = GetCenterRelativeFrame(kEntireFrame);
	SInt32 width = r.right - r.left;
	SInt32 height = r.bottom - r.top;
	if ( (width != mFrameSize.width) || (height != mFrameSize.height) ) {
		ResizeFrameTo(width, height, inRefresh);
	}
	Point p = mCurrPos.GetLocalPoint();
  	DEBUG_OUT("CalcPosition() " << this<< " @ " << mCurrPos, DEBUG_TRIVIA | DEBUG_MOVEMENT);
	Point3D pos;		// we must calc mDrawOffset separately from frame location for
	pos.x = p.h;		// times when we are outside Quickdraw coordinates
	pos.y = p.v;
	pos.z = 0;
	static_cast<CStarMapView*>(mSuperView)->GalacticToImagePt(p);
	static_cast<CStarMapView*>(mSuperView)->GalacticToImageCoord(pos);
	Point cOffset = GetDrawOffsetFromCenter();
	p.h += cOffset.h;
	p.v += cOffset.v;
	mDrawOffset = p;	 
	pos.x += cOffset.h + r.left;
	pos.y += cOffset.v + r.top;
	PlaceInSuperImageAt(pos.x, pos.y, inRefresh);
}

// take something that has already been marked as dead completely out of the game,
// including removing all references to it from other items.
// This is usually just before deleting an item.
void
AGalacticThingyUI::RemoveSelfFromGame() {
    AGalacticThingy::RemoveSelfFromGame();
    // take it out of the starmap so it can't get clicks
    PutInside(NULL, false);
	// and make sure that we aren't the selected thingy
	if (mGame->AsSharedUI()->GetSelectedThingy() == this) {
	    mGame->AsSharedUI()->SetSelectedThingy(NULL);
	}
}

float
AGalacticThingyUI::GetScaleFactorForZoom() const {	// scaling is not always linear, 1.2d8
	float scale;
	if (mSuperView) {
		scale = static_cast<CStarMapView*>(mSuperView)->GetZoom();	// v1.2d8
	} else {
		scale = 1;
	}
	if (scale > 1) {
		scale = 1;		// stop scaling when at full size
	}
	return scale;
}

Rect
AGalacticThingyUI::GetCenterRelativeFrame(bool full) const {
	SInt16 halfSize = mClassInfo->GetFrameSize();
	Rect r = {-halfSize, -halfSize, halfSize, halfSize};
	float scale = GetScaleFactorForZoom();
	ScaleRect(&r, scale);
	// new for Fog Of War
	if (full) {
	    Rect scanRect;
	    // divide by 256 because we want 4x the scanner range, otherwise we would
	    // divide by 1024 to convert from range to unzoomed pixels
		float zoom = static_cast<CStarMapView*>(mSuperView)->GetZoom();
        long longscanradius = (long)((float)(GetScannerRange() / (256L)) * zoom);
        if (longscanradius < 32000) {
            scanRect.top = -longscanradius;
            scanRect.bottom = longscanradius;
            scanRect.left = -longscanradius;
            scanRect.right = longscanradius;
        }
        if (scanRect.top < r.top) r.top = scanRect.top;
        if (scanRect.left < r.left) r.left = scanRect.left;
        if (scanRect.bottom > r.bottom) r.bottom = scanRect.bottom;
        if (scanRect.right > r.right) r.right = scanRect.right;
	}
	return r;
}

void
AGalacticThingyUI::UpdateSelectionMarker(UInt32 inTick) {
	if (inTick >= sSelectionMarkerChangeTick) {	// is it time for an update?
		sSelectionMarkerChangeTick = inTick + TICKS_PER_SELECTION_MARKER_UPDATE;
		EraseSelectionMarker();				// erase the marker if it isn't already there
		mSelectionMarkerPos++;				// get the next position
		if (mSelectionMarkerPos >= 6) {
			mSelectionMarkerPos = 0;
		}
		DrawSelectionMarker();
	} else if (!mSelectionMarkerShown) {	// not time for an update:
		DrawSelectionMarker();				// make sure marker is visible
	}
}

void
AGalacticThingyUI::DrawSelectionMarker() {
	if (!mSuperView) {
		return;	// move along, nothing to see here
	}
	FocusDraw();
	int inset = mSelectionMarkerPos;
	if (inset > 3) {
		inset = (6 - inset);
			// for mSelectionMarkerPos =     0   1   2   3   4   5 
			// the above gives the values    0   1   2   3   2   1
	}							//			 :   :   :   :   :   :
	inset -= 2;					// inset =  -2  -1   0   1   0  -1
	::PenMode(srcXor);
	Rect sframe = GetCenterRelativeFrame(kSelectionFrame);
	::MacOffsetRect(&sframe, mDrawOffset.h, mDrawOffset.v);
	::MacInsetRect(&sframe, inset, inset);	// adjust the selection marker
	::ForeColor(whiteColor);
//	::PenMode(srcCopy);
	int width = (sframe.right - sframe.left) / 3;
	int height = (sframe.bottom - sframe.top) / 3;
	if (width < 2) {
		width = 2;
	}
	if (height < 2) {
		height = 2;
	}
	int leftpos = sframe.left + width;
	::MoveTo(leftpos, sframe.top);
	::Line(-width, 0);
	::Line(0, height);
	::MoveTo(leftpos, sframe.bottom - 1);
	::Line(-width, 0);
	::Line(0, -height);
	int rightpos = sframe.right - width - 1;
	::MoveTo(rightpos, sframe.top);
	::Line(width, 0);
	::Line(0, height);
	::MoveTo(rightpos, sframe.bottom - 1);
	::Line(width, 0);
	::Line(0, -height);
	mSelectionMarkerShown = true;
  #if TARGET_API_MAC_CARBON
// #warning make this region a static variable to avoid constant reallocation, and/or use StRegion
//	RgnHandle refreshRgn = ::NewRgn();
	GrafPtr dstPort = mSuperView->GetMacPort();
//	::RectRgn(refreshRgn, &sframe);
    ::QDFlushPortBuffer(dstPort, NULL); //refreshRgn
//	::DisposeRgn(refreshRgn);
  #endif // TARGET_API_MAC_CARBON
}

void
AGalacticThingyUI::EraseSelectionMarker() {
	if (mSelectionMarkerShown) {
		DrawSelectionMarker();
	}
	mSelectionMarkerShown = false;
}

bool
AGalacticThingyUI::DrawSphereOfInfluence(int) {
    // override to draw a sphere of influence
    return false;	// we didn't draw anything
}

void
AGalacticThingyUI::DrawSelf() {
    bool selected = IsSelected();
#ifdef Draw_Thingy_Frames
	Rect	realframe;		//еее debugging
	CalcLocalFrameRect(realframe);
	if (selected) {
		ForeColor(yellowColor);
	} else {
		SetRGBForeColor(0xBBBB, 0xBBBB, 0);
	}
	::PenMode(srcCopy);
	::FrameRect(&realframe);
#endif
	if (selected && mSelectionMarkerShown && !Hiliting()) {	
		// we are the currently selected object, the selection marker
		// was shown, and we've received an DrawSelf() request that wasn't
		// from a call to DrawHilite() -- this means that we are responding
		// to a refresh event, which in all likelihood will have erased the
		// background.
		DrawSelectionMarker();
	}
}

Rect
AGalacticThingyUI::GetHotSpotRect() const {
	SInt16 halfSize = mClassInfo->GetHotSpotSize();
	Rect r = {-halfSize, -halfSize, halfSize, halfSize};
	float scale = GetScaleFactorForZoom();	// v1.2d8
	ScaleRect(&r, scale);
	if (r.right < 2) {	// minimum 4x4 click rect.
		r.right = r.bottom = 2;
		r.left = r.top = -2;
	}
	return r;
}

void
AGalacticThingyUI::CalcPortHotSpotRect(Rect &outRect) const {
	outRect = GetHotSpotRect();
	::MacOffsetRect(&outRect, mDrawOffset.h, mDrawOffset.v);
	LocalToPortPoint(topLeft(outRect));
	LocalToPortPoint(botRight(outRect));
}

void
AGalacticThingyUI::Select() {
    mGame->AsSharedUI()->SetSelectedThingy(this);
}




