// Galactica Panels.b
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/23/96 ERZ

#ifndef GALACTICA_PANELS_H_INCLUDED
#define GALACTICA_PANELS_H_INCLUDED

#include <LView.h>
#include <LListener.h>
#include <LCaption.h>

#include "GalacticaPanes.h"
#include "CShip.h"

class GalacticaDoc;
class CGalacticSlider;
class CSliderLinker;
class CNewTable;
class CWaypointTable;
class CDividedView;


class CBoxFrame : public LPane  {
public:
	enum { class_ID = 'BxFr' };
	
			CBoxFrame(LStream *inStream);
			
	virtual void 	DrawSelf();
};


class CBoxView : public LView, public LListener  {
public:
	enum { class_ID = 'Boxv' };
	
			CBoxView(LStream *inStream);

	void 			InitBoxView(void);
	virtual short	GetPanelStrResIndex() {return 1;}
	virtual void	FinishCreateSelf();
	virtual void	ListenToMessage(MessageT inMessage, void *ioParam);
	virtual void	ObeyPanelCommand(MessageT inCommand);
	
	void			EnableItem(PaneIDT inItemID);
	void			DisableItem(PaneIDT inItemID);
	void			SetItemEnable(PaneIDT inItemID, bool inEnabled);
	void			SetItemValue(PaneIDT inItemID, SInt32 inValue);	// use SetControlValue() for controls
	void			SetControlValue(PaneIDT inItemID, SInt32 inValue);	//turns off broadcasting
	SInt32			GetItemValue(PaneIDT inItemID);
	bool			SimulateButtonClick(PaneIDT inItemID, bool inForceAction = false);
	virtual void 	DoPanelButton(MessageT inMessage, long inValue);
	virtual void	Show();
	virtual void	Hide();
	virtual void	UpdatePanel(AGalacticThingy* inThingy);

protected:
	GalacticaDoc*	mGame;
	CSliderLinker*	mLinker;
	bool			mSendUserConOnShow;
	PaneIDT			mFirstSliderID;
};

#define kForceButtonAction true


//**** Ship Navigation Panel (bxNa) ****

class CbxNaPanel : public CBoxView {
friend class CShip;
public:
	enum { class_ID = 'bxNa' };
	
			CbxNaPanel(LStream *inStream);		
			
	virtual short	GetPanelStrResIndex() {return (mPaneID == 2031) ? 6 : 2;}
	virtual void	Show();
	virtual void	Hide();
	virtual void	FinishCreateSelf();
	virtual void 	DoPanelButton(MessageT inMessage, long inValue);
	virtual void	ListenToMessage(MessageT inMessage, void *ioParam);
	virtual void	UpdatePanel(AGalacticThingy* inThingy);
protected:
	void			UpdateButtons();
	CShip 			*mShip;
	CStarMapView	*mStarMap;
	CCoursePlotter	*mPlotter;
	LButton			*mWaitingButton;
	CWaypointTable	*mWaypoints;
//	int				mCurrWPNum;
};

/*
// **** Ship Info Panel (bxSI) ****

typedef class CbxSIPanel : public CBoxView {
public:
	enum { class_ID = 'bxSI' };
	static CbxSIPanel*		CreateCbxSIPanelStream(LStream *inStream);
	
			CbxSIPanel(LStream *inStream);		
			
	virtual void 	DoPanelButton(MessageT inMessage, long inValue);
	virtual short	GetPanelStrResIndex() {return 3;}
	virtual void	UpdatePanel(AGalacticThingy* inThingy);
} CbxSIPanel, *CbxSIPanelPtr, **CbxSIPanelHnd, **CbxSIPanelHdl;
*/

//**** System Info Panel (bxIn) ****

class CbxInPanel : public CBoxView {
public:
	enum { class_ID = 'bxIn' };
	
			CbxInPanel(LStream *inStream);		
			
	virtual short	GetPanelStrResIndex() {return 4;}
	virtual void	FinishCreateSelf();
	virtual void 	DoPanelButton(MessageT inMessage, long inValue);
	virtual void	ListenToMessage(MessageT inMessage, void *ioParam);
	virtual void	UpdatePanel(AGalacticThingy* inThingy);
	void			UpdateShipCompletion();
protected:
	CGalacticSlider* 	mSliders[10];
	int					mNumSliders;
	CStarMapView		*mStarMap;
	CStar				*mStar;
	CCoursePlotter		*mPlotter;
	LButton				*mNewShipDestButton;
	LCaption			*mShipCompletionCaption;
	int					mPrevTTNS;		// last value for Turns To Next Ship
private:
	virtual int		GetFirstSettingNum() {return 0;}
};

//**** System Ships Panel (bxSh) ****

class CbxShPanel : public CBoxView {
public:
	enum { class_ID = 'bxSh' };
	
			CbxShPanel(LStream *inStream);		
			
	virtual short	GetPanelStrResIndex() {return (mPaneID == 2032) ? 7 : 5;}
	virtual void	Hide();
	virtual void	FinishCreateSelf();
	virtual void	ObeyPanelCommand(MessageT inCommand);
	virtual void	ListenToMessage(MessageT inMessage, void *ioParam);
	virtual void 	DoPanelButton(MessageT inMessage, long inValue);
	virtual void	UpdatePanel(AGalacticThingy* inThingy);
	
	static AGalacticThingy* sPrevThingy;

protected:
	void	UpdateButtons(CShip* inShip);

	AGalacticThingy* mAtThingy;
	CNewTable*		mShipsTable;
	CNewTable*		mFleetTable;
};


#endif // GALACTICA_PANELS_H_INCLUDED

