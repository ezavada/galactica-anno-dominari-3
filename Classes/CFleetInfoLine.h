// CFleetInfoLine.h
// ===========================================================================
// Copyright (c) 1996-2003, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include <LView.h>
#include <CShip.h>

class CFleetInfoLine : public LView {
public:

	CFleetInfoLine();
	CFleetInfoLine(const LView &inOriginal);
	CFleetInfoLine(const SPaneInfo &inPaneInfo, const SViewInfo &inViewInfo);
	CFleetInfoLine(LStream *inStream);
	~CFleetInfoLine();
	
	void SetShip(CShip* inShip);
	void SetBackColor(RGBColor Color);
	virtual void DrawSelf();
   virtual void Click(SMouseDownEvent& inMouseDown);
   virtual void ClickSelf(const SMouseDownEvent& inMouseDown);

protected:
	RGBColor	fBackColor;
	CShip* mShip;
private:
	void	InitFleetInfoLine();
};
