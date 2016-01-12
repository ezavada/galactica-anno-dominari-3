// AGalacticThingyUI.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ, 4/18/96
// 3/14/02 ERZ, separated from AGalacticThingy


#ifndef AGALACTIC_THINGY_UI_H_INCLUDED
#define AGALACTIC_THINGY_UI_H_INCLUDED

#define kSelectionFrame false
#define kEntireFrame true

#include "AGalacticThingy.h"

#ifndef GALACTICA_SERVER

#include <LPane.h>
#include "GalacticaUtilsUI.h"

class GalacticaClient;

class AGalacticThingyUI : public AGalacticThingy, public LPane {
public:
	static AGalacticThingyUI*	MakeUIThingyFromSubClassType(LView* inSuper, GalacticaShared* inGame, long inThingyType);

	enum { class_ID = 'thgy' };

			AGalacticThingyUI(LView *inSuperView, GalacticaShared* inGame, long inThingyType);
			AGalacticThingyUI(LView *inSuperView, GalacticaShared* inGame, LStream *inStream, long inThingyType);
			virtual ~AGalacticThingyUI();
			
	void	InitThingyUI();

// ---- identity methods ----
    virtual void    SetID(SInt32 inID) { mPaneID = inID; AGalacticThingy::SetID(inID); }
    virtual AGalacticThingyUI* AsThingyUI() { return this; }


// ---- persistence/streaming methods ----
	virtual void	FinishInitSelf();		// do anything that was waiting til pane id was assigned
	virtual void	ReadStream(LStream *inStream, UInt32 streamVersion = version_SavedTurnFile);

// ---- mouse handling methods ----
	virtual void	Click(SMouseDownEvent &inMouseDown);
	virtual void	ClickSelf(const SMouseDownEvent& inMouseDown);
	virtual void	DoubleClickSelf();
	virtual Boolean	DragSelf(SMouseDownEvent &inMouseDown);	// called by click, return false if no dragging occurred.
	virtual void	AdjustMouseSelf(Point inPortPt, const EventRecord& inMacEvent, RgnHandle outMouseRgn);
	virtual Boolean	Contains(SInt32 inHorizPort, SInt32 inVertPort) const;
	virtual Boolean	IsHitBy(SInt32 inHorizPort, SInt32 inVertPort);
	Rect			GetHotSpotRect() const;	// returns info based on ThingyClassInfo::GetHotSpotSize()
	void			CalcPortHotSpotRect(Rect &outRect) const;

// ---- selection methods ----
	virtual void 	Select();
	bool			IsSelected() const;

// ---- drawing methods ----
	void 			DrawHilite(bool inHilite);
	inline bool 	Hiliting() const {return sChangingHilite;}
	bool			IsHilited() const {return sHilitedThingy == this;}
	static AGalacticThingyUI* GetHilitedThingy() {return sHilitedThingy;}
	virtual bool	ShouldDraw() const;
	virtual float	GetBrightness(short inDisplayMode);
	virtual Point	GetDrawOffsetFromCenter() const {Point p = {0,0}; return p;}
	virtual Rect	GetCenterRelativeFrame(bool full = false) const;	// override to return frame relative to center pt
	virtual float	GetScaleFactorForZoom() const;
	void			UpdateSelectionMarker(UInt32 inTick);	// does everything needed to draw the selection marker
	void			DrawSelectionMarker();
	void			EraseSelectionMarker();
	virtual bool    DrawSphereOfInfluence(int level = -1);
	virtual void 	DrawSelf();
	virtual void	Refresh();
	Point			GetDrawingPosition() const {return mDrawOffset;}

// ---- object info methods ----
	Waypoint		GetPosition() const {return mCurrPos;}
	virtual ResIDT	GetViewLayrResIDOffset();
	virtual void	Changed(); 

// ---- miscellaneous methods ----
	virtual void	ThingMoved(bool inRefresh);
	void			CalcPosition(bool inRefresh);
	virtual void    RemoveSelfFromGame(); // take something that has already been marked as dead completely out of the game

protected:
	Point				mDrawOffset;	// center of thingy relative to the topleft of frame
	static AGalacticThingyUI*	sHilitedThingy;	// last thingy to receive a hilite command
	static SInt32		sOldHilitedPos;	// order in list of last thingy that was hilited
	static Cursor		sTargetCursor;
	static bool 		sCursorLoaded;
	static bool 		sDragging;		// item is being dragged
	static bool 		sAutoscrolling;	// autoscrolling during a drag
private:	
	bool 			    mSelectionMarkerShown;		// private: use DrawSelectionMarker()
	int					mSelectionMarkerPos;
	static UInt32		sSelectionMarkerChangeTick;
	static bool 		sChangingHilite;	// true = Drawing should change state of hiliting; Private: use DrawHilite(), Hiliting() and IsHilited()
};

/*#ifdef DEBUG
	ostream& operator << (ostream& cout, AGalacticThingyUI *outThingy);
#endif
*/

#else
   // for a Galactica Server, we just declare AGalacticaThingyUI to be the same as a 
   // non-UI thingy
    class AGalacticThingyUI : public AGalacticThingy {
    public:
		AGalacticThingyUI(LView*, GalacticaShared* inGame, long inThingyType)
		    : AGalacticThingy(inGame, inThingyType) {}
//		AGalacticThingyUI(LView*, GalacticaShared* inGame, LStream *inStream, long inThingyType)
//		    : AGalacticThingy(inGame, inStream, inThingyType) {}
    };
#endif // GALACTICA_SERVER

typedef std::vector<AGalacticThingyUI*> StdThingyUIList;

#endif // AGALACTIC_THINGY_UI_H_INCLUDED


