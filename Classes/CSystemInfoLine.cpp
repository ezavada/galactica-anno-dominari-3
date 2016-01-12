// CSystemInfoLine.cpp
// ===========================================================================
// Copyright (c) 1996-2003, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include "CSystemInfoLine.h"
#include "GalacticaUtils.h"
#include "CFleet.h"
#include "GalacticaDoc.h"
#include <Quickdraw.h>

#define kNameWidth			(74+46)
#define	kNameCol			   14
#define kTechWidth			45
#define	kTechCol			   (87+46)
#define kPopulationWidth	70
#define	kPopulationCol		(134+46)
#define kProductionWidth	64
#define	kProductionCol		(212+46)
#define kShipBuildWidth		64
#define	kShipBuildCol		(293+46)
#define kShipPercentWidth  30
#define	kShipPercentCol   (378+46)
#define kPowerTotalWidth   45
#define	kPowerTotalCol    (420+46)
#define kPowerSatWidth     45
#define	kPowerSatCol      (472+46)
#define kPowerShipsWidth   45
#define	kPowerShipsCol    (523+46)


CSystemInfoLine::CSystemInfoLine() : LView()
{
	InitSystemInfoLine();
}


CSystemInfoLine::CSystemInfoLine(const LView &inOriginal) : LView(inOriginal)
{
	InitSystemInfoLine();
}

CSystemInfoLine::CSystemInfoLine(const SPaneInfo &inPaneInfo, const SViewInfo &inViewInfo) : LView(inPaneInfo, inViewInfo)
{
	InitSystemInfoLine();
}

CSystemInfoLine::CSystemInfoLine(LStream *inStream) : LView(inStream)
{
	InitSystemInfoLine();
}

void CSystemInfoLine::InitSystemInfoLine() {
	fBackColor.red = 0xFFFF;
	fBackColor.blue = 0xFFFF;
	fBackColor.green = 0xFFFF;
	mStar = NULL;
}

CSystemInfoLine::~CSystemInfoLine()
{	
}

void CSystemInfoLine::SetStar(CStar* inStar)
{
   mStar = inStar;   // save the star for when we double click

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
	
	ResIDT traitsID = 0;
	// Create Name Caption
	Info.width = kNameWidth;
	Info.left = kNameCol;
	traitsID = 145;
	LStr255 s = inStar->GetName();
	aCaption = new LCaption(Info, s, traitsID);
	aCaption->FinishCreate();

   // Create Tech Level Caption
   Info.width = kTechWidth;
   Info.left = kTechCol;
   traitsID = 146;
   aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
   aCaption->FinishCreate();
   aCaption->SetValue(inStar->GetTechLevel());
	
	// Create Population Caption
	Info.width = kPopulationWidth;
	Info.left = kPopulationCol;
   traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
	aCaption->FinishCreate();	
   aCaption->SetValue(inStar->GetPopulation());
	
   // Create Production Caption
   Info.width = kProductionWidth;
   Info.left = kProductionCol;
   traitsID = 146;
   aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
   aCaption->FinishCreate();
   aCaption->SetValue(inStar->GetProduction()/100);
	
	// Create Ship Building Type Caption
	Info.width = kShipBuildWidth;
	Info.left = kShipBuildCol;
   traitsID = 156;   // centered 10 galactica srcOr
	aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
	aCaption->FinishCreate();
	std::string buildTypeStr;
	int hullTypeIndex = inStar->GetBuildHullType();
	if (hullTypeIndex == class_Fighter) {
	   hullTypeIndex += inStar->GetTechLevel()/5;
	}
	LoadStringResource(buildTypeStr, STRx_ShipNames, hullTypeIndex+1);
	s.Assign(buildTypeStr.c_str());
   aCaption->SetDescriptor(s);
	
	// Create Ship Percent Complete Caption
	Info.width = kShipPercentWidth;
	Info.left = kShipPercentCol;
   traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
	aCaption->FinishCreate();
   aCaption->SetValue(inStar->GetShipPercentComplete());
	
	// Create Total Power Caption
	Info.width = kPowerTotalWidth;
	Info.left = kPowerTotalCol;
   traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
	aCaption->FinishCreate();
	long power = inStar->GetTotalDefenseStrength() / 100;
   aCaption->SetValue(power);
	
	// Create Satellites Power Caption
	Info.width = kPowerSatWidth;
	Info.left = kPowerSatCol;
   traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
	aCaption->FinishCreate();
	CFleet* satFleet = inStar->GetSatelliteFleet();
   power = 0;
	if (satFleet) {
	   power = ((satFleet->GetPower()*100) - satFleet->GetDamage()) / 100;
	}
	aCaption->SetValue(power);
	
	// Create Ships Power Caption
	Info.width = kPowerShipsWidth;
	Info.left = kPowerShipsCol;
   traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
	aCaption->FinishCreate();
	power = inStar->GetNonSatelliteShipStrength() / 100;
   aCaption->SetValue(power);
}

void CSystemInfoLine::SetBackColor(RGBColor Color)
{
	fBackColor = Color;
}

void CSystemInfoLine::DrawSelf()
{
//RGBColor	Color = {0,0,0};
Rect	r;

	CalcLocalFrameRect(r);	
	::RGBForeColor(&fBackColor);
	
	Pattern black;
	UQDGlobals::GetBlackPat(&black);
	::MacFillRect(&r, &black);
}

void CSystemInfoLine::Click(SMouseDownEvent &inMouseDown) {
   // don't bother to look at 
   LPane::Click(inMouseDown);	//   will process click on this View
}

void CSystemInfoLine::ClickSelf(const SMouseDownEvent &) {
   if (GetClickCount() == 2) {
      // we double-clicked a line, jump to that star
      if (mStar) {
         mStar->Select();
         GalacticaDoc* doc = mStar->GetGameDoc();
         if (doc) {
            doc->CloseSystemsWindow();
         }
      }
   }
}


