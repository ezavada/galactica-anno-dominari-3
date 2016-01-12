// CRendezvous
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 9/22/97 ERZ

#include "CRendezvous.h"

#ifndef GALACTICA_SERVER

#include "GalacticaPanels.h"
#include "GalacticaDoc.h"
#if BALLOON_SUPPORT
#include "CHelpAttach.h"
#endif // BALLOON_SUPPORT

#include <UDrawingUtils.h>

#define DRAG_SLOP_PIXELS 4

#endif // GALACTICA_SERVER

CRendezvous::CRendezvous(LView *inSuperView, GalacticaShared* inGame)
:ANamedThingy(inSuperView, inGame, thingyType_Rendezvous), mFleetRef() {
	InitRendezvous();
}


/*
CRendezvous::CRendezvous(LView *inSuperView, GalacticaShared* inGame, LStream *inStream) 
:ANamedThingy(inSuperView, inGame, inStream, thingyType_Rendezvous), mFleetRef() {
	InitRendezvous();
	ReadRendezvousStream(*inStream);
}
*/

CRendezvous::~CRendezvous() {
}

void
CRendezvous::InitRendezvous() {
  #if BALLOON_SUPPORT
	CThingyHelpAttach* theBalloonHelp = new CThingyHelpAttach(this);
	AddAttachment(theBalloonHelp);
  #endif // BALLOON_SUPPORT
}


void
CRendezvous::ReadStream(LStream *inStream, UInt32 streamVersion) {
	ANamedThingy::ReadStream(inStream, streamVersion);
	ReadRendezvousStream(*inStream);
}

void
CRendezvous::WriteStream(LStream *inStream, SInt8 forPlayerNum) {
	ANamedThingy::WriteStream(inStream);
	*inStream << mFleetRef;
}

void
CRendezvous::UpdateClientFromStream(LStream *inStream) {
	ANamedThingy::UpdateClientFromStream(inStream);
	ReadRendezvousStream(*inStream);		// read in everything
}

void
CRendezvous::UpdateHostFromStream(LStream *inStream) {
	ANamedThingy::UpdateHostFromStream(inStream);
	ReadRendezvousStream(*inStream);		// read in everything
}

void
CRendezvous::ReadRendezvousStream(LStream &inStream) {
	inStream >> mFleetRef;
}


ResIDT
CRendezvous::GetViewLayrResIDOffset() {
	return 0;
}


void
CRendezvous::FinishEndOfTurn(long inTurnNum) {
	ANamedThingy::FinishEndOfTurn(inTurnNum);
	CFleet* aFleet = (CFleet*)mFleetRef;
	if (aFleet) {
		if (aFleet->IsDead()) {	// no point in having ships rendezvous into a dead fleet
			mFleetRef.ClearRef();	// = nil;
			mChanged = true;
		} else if (!(aFleet->GetPosition() == mCurrPos)) {	// the fleet has left the rendezvous pt,
			mFleetRef.ClearRef();	// = nil;							// so create a new fleet upon next arrival
			mChanged = true;
		}
	}
}

#ifndef GALACTICA_SERVER

Boolean
CRendezvous::DragSelf(SMouseDownEvent &) {//inMouseDown) {
	if (CStarMapView::IsAwaitingClick())	// if we are plotting a course, we don't
		return false;					// want to allow them to drag the rendezvous point
    if (!mGame->IsClient())
        return false;
	if (mOwnedBy != GetGameDoc()->GetMyPlayerNum())
		return false;
	if (mNeedsDeletion != 0)
		return false;
	Point currentPoint, firstPoint, lastPoint;
	Point clickOffset;  // pixels by which the click point was offset from the centerpoint
	Boolean dragging = false;
	FocusDraw();
	::GetMouse(&firstPoint);
	clickOffset.h = firstPoint.h - mDrawOffset.h;
	clickOffset.v = firstPoint.v - mDrawOffset.v;
	Boolean drawn = false;
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
		ASSERT(mSuperView != nil);
		CStarMapView* map = (CStarMapView*)mSuperView;
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
		mCurrPos = coord;
		sDragging = false;
		Refresh();	// redraw original position
		ThingMoved(kRefresh);
		// now adjust anything that refers to this, telling it to recalc its position, dst, etc..
		UMoveReferencedThingysAction recalcAction(this);
		mGame->DoForEverything(recalcAction);
	}
	return dragging;
}

Boolean
CRendezvous::IsHitBy(SInt32 inHorizPort, SInt32 inVertPort) {
	if (mNeedsDeletion != 0) {
		return false;	// can't click on or otherwise interact with deleted Rendezvous
	} else {
		return ANamedThingy::IsHitBy(inHorizPort, inVertPort);
	}
}


void
CRendezvous::DrawSelf() {
    if (!mGame->IsClient()) {
        return;     // don't draw on host
    }
	if (sAutoscrolling)
		return;		// don't draw ourselves while autoscrolling or we will leave nasty tracks
	if (!sDragging) {
		if ( !ShouldDraw() )
			return;
	    if (mGame->IsClient()) {
    		if (mOwnedBy != GetGameDoc()->GetMyPlayerNum()) {
    			return;
    	    }
		}
		if (mNeedsDeletion != 0)
			return;
	}
	EraseSelectionMarker();
	Rect frame;
	int pensize;
	if (mSuperView) {
		frame = GetCenterRelativeFrame(kSelectionFrame);
		float zoom = ((CStarMapView*)mSuperView)->GetZoom();
		if (zoom < 0.25)
			return;
		if (zoom < 0.5)	// change frame when less than 1:1 zoom
			pensize = 1;
		else if (zoom < 1)
			pensize = 2;
		else
			pensize = 3;
	}
	::PenSize(pensize,pensize);
	::MacInsetRect(&frame, 2, 2);
	frame.bottom -= (pensize - 1);
	frame.right -= (pensize - 1);
	::MacOffsetRect(&frame, mDrawOffset.h, mDrawOffset.v);
	if (sDragging) {
		::ForeColor(whiteColor);
	} else {
	    RGBColor c = *(GetGameDoc()->GetColor(mOwnedBy, false));
	    ::RGBForeColor(&c);
	}
	::MoveTo(frame.left, frame.top);
	::MacLineTo(frame.right, frame.bottom);
	::MoveTo(frame.right, frame.top);
	::MacLineTo(frame.left, frame.bottom);
	::PenSize(1, 1);	// restore pen size
	if (!sDragging) {	// don't draw name while dragging
		ANamedThingy::DrawSelf();
	}
}

void
CRendezvous::RemoveSelfFromClient() {
	DEBUG_OUT("CRendezvous::RemoveSelfFromClient() "<<this, DEBUG_DETAIL | DEBUG_USER);
	Refresh();
	mNeedsDeletion = -1;	// set to be scrapped
	Changed();
	// immediately remove all reference to it, since 
  	URemoveReferencesAction removeRefsAction(this);
  	mGame->DoForEverything(removeRefsAction);
}

#endif // GALACTICA_SERVER

