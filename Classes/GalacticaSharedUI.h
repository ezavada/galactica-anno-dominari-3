//	GalacticaSharedUI.h		
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#ifndef GALACTICA_SHARED_UI_H_INCLUDED
#define GALACTICA_SHARED_UI_H_INCLUDED

#include "GalacticaShared.h"

class CStarMapView;
class AGalacticThingyUI;

class GalacticaSharedUI : virtual public GalacticaShared {
public:
	GalacticaSharedUI() : mSelectedThingy(NULL), mStarMap(NULL) { mSharedUI = this; } // constructor
    
	CStarMapView*               GetStarMap() const {return mStarMap;}
	virtual void		        SetSelectedThingy(AGalacticThingyUI* inThingy) { mSelectedThingy = inThingy; }
	AGalacticThingyUI*	        GetSelectedThingy() const {return mSelectedThingy;}
	virtual AGalacticThingy*    MakeThingy(long inThingyType);
    virtual void                NotifyBusy(long inCurrUnit = 0, long inTotalUnits = 0);

protected:
	AGalacticThingyUI*      mSelectedThingy;
	CStarMapView*           mStarMap;
};

#endif // GALACTICA_SHARED_UI_H_INCLUDED

