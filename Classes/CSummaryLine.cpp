// CSummaryLine.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include "CSummaryLine.h"
#include "GalacticaUtils.h"
#include <Quickdraw.h>
#include <LCaption.h>
#include <UDrawingState.h>

#define kNameWidth			(74+46)
#define	kNameCol			14
#define kStarsWidth			45
#define	kStarsCol			(86+46)
#define kPopulationWidth	70
#define	kPopulationCol		(132+46)
#define kProductionWidth	60
#define	kProductionCol		(202+46)
#define kStarLowWidth		30
#define	kStarLowCol			(262+46)
#define kStarHighWidth		30
#define	kStarHighCol		(296+46)
#define kStarAvgWidth		30
#define	kStarAvgCol			(328+46)
#define kShipsWidth			45
#define	kShipsCol			(360+46)
#define kPowerWidth			70
#define	kPowerCol			(402+46)
#define kShipLowWidth		30
#define	kShipLowCol			(474+46)
#define kShipHighWidth		30
#define	kShipHighCol		(506+46)
#define kShipAvgWidth		30
#define	kShipAvgCol			(538+46)


CSummaryLine::CSummaryLine() : LView()
{
	InitSummaryLine();
}


CSummaryLine::CSummaryLine(const LView &inOriginal) : LView(inOriginal)
{
	InitSummaryLine();
}

CSummaryLine::CSummaryLine(const SPaneInfo &inPaneInfo, const SViewInfo &inViewInfo) : LView(inPaneInfo, inViewInfo)
{
	InitSummaryLine();
}

CSummaryLine::CSummaryLine(LStream *inStream) : LView(inStream)
{
	InitSummaryLine();
}

void CSummaryLine::InitSummaryLine() {
	fBackColor.red = 0xFFFF;
	fBackColor.blue = 0xFFFF;
	fBackColor.green = 0xFFFF;
	fDotColor.red = 0xFFFF;
	fDotColor.blue = 0xFFFF;
	fDotColor.green = 0xFFFF;
}

CSummaryLine::~CSummaryLine()
{	
}

void CSummaryLine::SetSummary(tSummaryRec theRec, tSummaryRec Max, tSummaryRec Min, bool ShowMax)
{
SPaneInfo	Info;
LCaption	*aCaption;

	// Initialize fields of Info that do not change.
	Info.paneID = 0;
	Info.height = 16;
	Info.visible = TRUE;
	Info.enabled = TRUE;
	Info.bindings.left = FALSE;
	Info.bindings.right = FALSE;
	Info.bindings.top = FALSE;
	Info.bindings.bottom = FALSE;
	Info.top = 2;
	Info.userCon = 0;
	Info.superView = this;
	
	ResIDT traitsID = 0;	// ¥¥¥ ERZ
	bool realShowMax = ShowMax;
	if (theRec.dead)
		ShowMax = false;	// don't do colors for dead people ¥¥¥ ERZ
	// Create Name Caption
	Info.width = kNameWidth;
	Info.left = kNameCol;
	if (theRec.italicizeName) { // || theRec.dead || !theRec.loggedIn)
		traitsID = 148;
	} else {
		traitsID = 145;
	}
	LStr255 s = theRec.Name;
	aCaption = new LCaption(Info, s, traitsID);
	aCaption->FinishCreate();

	// Create Stars Caption
	Info.width = kStarsWidth;
	Info.left = kStarsCol;
	if(ShowMax && (theRec.Stars >= Max.Stars))
		traitsID = 147;
	else if (ShowMax && (theRec.Stars == Min.Stars))
		traitsID = 149;
	else
		traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"\pÑ", traitsID);
	aCaption->FinishCreate();
	if (!theRec.dead)
		aCaption->SetValue(theRec.Stars);
	
	// Create Population Caption
	Info.width = kPopulationWidth;
	Info.left = kPopulationCol;
	if(ShowMax && (theRec.Population >= Max.Population))
		traitsID = 147;
	else if (ShowMax && (theRec.Population == Min.Population))
		traitsID = 149;
	else
		traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"\pÑ", traitsID);
	aCaption->FinishCreate();	
	if (!theRec.dead) {
	   // because SetValue uses a long, rather than a long long like
	   // we need for population, we have to convert this ourselves
	   char buff[40];
	   std::sprintf(buff, "%lld", theRec.Population);
	   s.Assign(buff);
		aCaption->SetDescriptor(s);
	}
	
	// Create Production Caption
	Info.width = kProductionWidth;
	Info.left = kProductionCol;
	if(ShowMax && (theRec.Production >= Max.Production))
		traitsID = 147;
	else if (ShowMax && (theRec.Production == Min.Production))
		traitsID = 149;
	else
		traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"\pÑ", traitsID);
	aCaption->FinishCreate();
	if (!theRec.dead)
		aCaption->SetValue(theRec.Production/100);
	
	// Create StarLow Caption
	Info.width = kStarLowWidth;
	Info.left = kStarLowCol;
	if(ShowMax && (theRec.StarLow >= Max.StarLow))
		traitsID = 147;
	else if (ShowMax && (theRec.StarLow == Min.StarLow))
		traitsID = 149;
	else
		traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"\pÑ", traitsID);
	aCaption->FinishCreate();
	if (!theRec.dead)
		aCaption->SetValue(theRec.StarLow);
	
	// Create StarHigh Caption
	Info.width = kStarHighWidth;
	Info.left = kStarHighCol;
	if(ShowMax && (theRec.StarHigh >= Max.StarHigh))
		traitsID = 147;
	else if (ShowMax && (theRec.StarHigh == Min.StarHigh))
		traitsID = 149;
	else
		traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"\pÑ", traitsID);
	aCaption->FinishCreate();
	if (!theRec.dead)
		aCaption->SetValue(theRec.StarHigh);
	
	// Create StarAvg Caption
	Info.width = kStarAvgWidth;
	Info.left = kStarAvgCol;
	if(ShowMax && (theRec.StarAvg >= Max.StarAvg))
		traitsID = 147;
	else if (ShowMax && (theRec.StarAvg == Min.StarAvg))
		traitsID = 149;
	else
		traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"\pÑ", traitsID);
	aCaption->FinishCreate();
	if (!theRec.dead)
		aCaption->SetValue(theRec.StarAvg);
	
	// Create Ships Caption
	Info.width = kShipsWidth;
	Info.left = kShipsCol;
	if(ShowMax && (theRec.Ships >= Max.Ships))
		traitsID = 147;
	else if (ShowMax && (theRec.Ships == Min.Ships))
		traitsID = 149;
	else
		traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"\pÑ", traitsID);
	aCaption->FinishCreate();
	if (!theRec.dead && realShowMax)
		aCaption->SetValue(theRec.Ships);
	
	// Create Power Caption
	Info.width = kPowerWidth;
	Info.left = kPowerCol;
	if(ShowMax && (theRec.Power >= Max.Power))
		traitsID = 147;
	else if (ShowMax && (theRec.Power == Min.Power))
		traitsID = 149;
	else
		traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"\pÑ", traitsID);
	aCaption->FinishCreate();
	if (!theRec.dead && realShowMax)
		aCaption->SetValue(theRec.Power);
	
	// Create ShipLow Caption
	Info.width = kShipLowWidth;
	Info.left = kShipLowCol;
	if(ShowMax && (theRec.ShipLow >= Max.ShipLow))
		traitsID = 147;
	else if (ShowMax && (theRec.ShipLow == Min.ShipLow))
		traitsID = 149;
	else
		traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"\pÑ", traitsID);
	aCaption->FinishCreate();
	if (!theRec.dead && realShowMax)
		aCaption->SetValue(theRec.ShipLow);
	
	// Create ShipHigh Caption
	Info.width = kShipHighWidth;
	Info.left = kShipHighCol;
	if(ShowMax && (theRec.ShipHigh >= Max.ShipHigh))
		traitsID = 147;
	else if (ShowMax && (theRec.ShipHigh == Min.ShipHigh))
		traitsID = 149;
	else
		traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"\pÑ", traitsID);
	aCaption->FinishCreate();
	if (!theRec.dead && realShowMax) {
		aCaption->SetValue(theRec.ShipHigh);
	}
	
	// Create ShipAvg Caption
	Info.width = kShipAvgWidth;
	Info.left = kShipAvgCol;
	if(ShowMax && (theRec.ShipAvg >= Max.ShipAvg))
		traitsID = 147;
	else if (ShowMax && (theRec.ShipAvg == Min.ShipAvg))
		traitsID = 149;
	else
		traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"\pÑ", traitsID);
	aCaption->FinishCreate();
	if (!theRec.dead && realShowMax)
		aCaption->SetValue(theRec.ShipAvg);
	
}

void CSummaryLine::SetBackColor(RGBColor Color)
{
	fBackColor = Color;
}

void CSummaryLine::SetDotColor(RGBColor Color)
{
	fDotColor = Color;
}

void CSummaryLine::DrawSelf()
{
RGBColor	Color = {0,0,0};
Rect	r;

	CalcLocalFrameRect(r);	
	::RGBForeColor(&fBackColor);
	
	Pattern black;
	UQDGlobals::GetBlackPat(&black);
	::MacFillRect(&r, &black);
	
	::RGBForeColor(&fDotColor);
	::MoveTo(r.left+4, r.top+12);
	::TextFont(0);
	::TextFace(0);
	::TextSize(9);
	::DrawChar('¥');
	::RGBForeColor(&Color);
}

