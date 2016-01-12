// CSummaryLine.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include <LView.h>
#include <GalacticaConsts.h>

typedef struct {
	char    Name[MAX_PLAYER_NAME_LEN+1];
	long	Stars;
	long long	Population;		// 1.2fc7, try to avoid population going negative in large games
	long	Production;
	long	StarLow;
	long	StarHigh;
	long	StarAvg;
	long	Ships;
	long	Power;
	long	ShipLow;
	long	ShipHigh;
	long	ShipAvg;
	bool    italicizeName;	// added line ERZ, Fri 7/12/96
	bool    dead;			// added line ERZ, Fri 7/19/96
	bool    loggedIn;       // added line ERZ, Tue 4/9/02
	}	tSummaryRec;
	
typedef tSummaryRec	*tSummaryP, **tSummaryH;

class CSummaryLine : public LView {
public:


	CSummaryLine();
	CSummaryLine(const LView &inOriginal);
	CSummaryLine(const SPaneInfo &inPaneInfo, const SViewInfo &inViewInfo);
	CSummaryLine(LStream *inStream);
	~CSummaryLine();
	
	void SetSummary(tSummaryRec theRec, tSummaryRec Max, tSummaryRec Min, bool ShowMax);
	void SetBackColor(RGBColor Color);
	void SetDotColor(RGBColor Color);
	virtual void DrawSelf();
	
protected:
	RGBColor	fBackColor;
	RGBColor	fDotColor;
private:
	void	InitSummaryLine();
};
