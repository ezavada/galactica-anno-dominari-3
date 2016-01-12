// ===========================================================================
//	CResCaption.h				   ©1993-1995 Metrowerks Inc. All rights reserved.
// ===========================================================================
//
//	Pane with a single line of text

#pragma once

#include <LCaption.h>


class	CResCaption : public LCaption {
public:
	enum { class_ID = 'RCap' };

						CResCaption();
						CResCaption(const SPaneInfo &inPaneInfo, ResIDT inTextTraitsID,
									ResIDT inSTRxID, short inStrIdx = 0);
						CResCaption(LStream *inStream);

	void			GetStrID(ResIDT& outSTRxID, short &outStrIdx) const
									{ outSTRxID = mSTRxID; outStrIdx = mStrIdx; }				
	void			SetStrID(ResIDT inSTRxID, short inStrIdx = 0);
	void			ReloadStrRes();
	
	void			SetDrawDisabled(Boolean inDrawDisabled) {mDrawDisabled = inDrawDisabled;}
						
	virtual void	DisableSelf();
	virtual void	EnableSelf();
protected:
	ResIDT		mSTRxID;
	short		mStrIdx;
	Boolean		mDrawDisabled;
	
	virtual void	DrawSelf();	
};

