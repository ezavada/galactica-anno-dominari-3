// CSystemInfoLine.h
// ===========================================================================
// Copyright (c) 1996-2003, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include <LView.h>
#include <CStar.h>

class CStar;

class CSystemInfoLine : public LView {
public:

	CSystemInfoLine();
	CSystemInfoLine(const LView &inOriginal);
	CSystemInfoLine(const SPaneInfo &inPaneInfo, const SViewInfo &inViewInfo);
	CSystemInfoLine(LStream *inStream);
	~CSystemInfoLine();
	
	void SetStar(CStar* inStar);
	void SetBackColor(RGBColor Color);
	virtual void DrawSelf();
   virtual void Click(SMouseDownEvent& inMouseDown);
   virtual void ClickSelf(const SMouseDownEvent& inMouseDown);

protected:
	RGBColor	fBackColor;
	CStar* mStar;
private:
	void	InitSystemInfoLine();
};
