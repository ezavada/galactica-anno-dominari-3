// CSystemInfoLine.cpp
// ===========================================================================
// Copyright (c) 1996-2003, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include "CFleetInfoLine.h"
#include "GalacticaUtils.h"
#include "CFleet.h"
#include "GalacticaDoc.h"
#include <Quickdraw.h>
#include <UDrawingState.h>

#define kNameWidth			(74+46)
#define	kNameCol			   14
#define kTechWidth			45
#define	kTechCol			   (87+46)
#define kSpeedWidth	      70
#define	kSpeedCol		   (134+46)
#define kPowerWidth	      64
#define	kPowerCol		   (212+46)
#define kNumShipsWidth		64
#define	kNumShipsCol		(293+46)
#define kDamagePercentWidth  30
#define	kDamagePercentCol   (378+46)
#define kColonyWidth       45
#define	kColonyCol        (420+46)
#define kDestinationWidth  90
#define	kDestinationCol   (472+46)


CFleetInfoLine::CFleetInfoLine() : LView()
{
	InitFleetInfoLine();
}


CFleetInfoLine::CFleetInfoLine(const LView &inOriginal) : LView(inOriginal)
{
	InitFleetInfoLine();
}

CFleetInfoLine::CFleetInfoLine(const SPaneInfo &inPaneInfo, const SViewInfo &inViewInfo) : LView(inPaneInfo, inViewInfo)
{
	InitFleetInfoLine();
}

CFleetInfoLine::CFleetInfoLine(LStream *inStream) : LView(inStream)
{
	InitFleetInfoLine();
}

void CFleetInfoLine::InitFleetInfoLine() {
	fBackColor.red = 0xFFFF;
	fBackColor.blue = 0xFFFF;
	fBackColor.green = 0xFFFF;
	mShip = NULL;
}

CFleetInfoLine::~CFleetInfoLine()
{	
}

void CFleetInfoLine::SetShip(CShip* inShip)
{
   mShip = inShip;   // save the star for when we double click

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
	LStr255 s = inShip->GetName();
	aCaption = new LCaption(Info, s, traitsID);
	aCaption->FinishCreate();

   // Create Tech Level Caption
   Info.width = kTechWidth;
   Info.left = kTechCol;
   traitsID = 146;
   aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
   aCaption->FinishCreate();
   aCaption->SetValue(inShip->GetTechLevel());
	
	// Create Speed Caption
	Info.width = kSpeedWidth;
	Info.left = kSpeedCol;
   traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
	aCaption->FinishCreate();	
	std::string speedStr;
	GetSpeedString(mShip->GetSpeed(), speedStr);	// Speed
	s = speedStr.c_str();
	aCaption->SetDescriptor(s);
	
   // Create Power Caption
   Info.width = kPowerWidth;
   Info.left = kPowerCol;
   traitsID = 146;
   aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
   aCaption->FinishCreate();
   aCaption->SetValue(inShip->GetPower());
	
	// Create NumShips Caption
	Info.width = kNumShipsWidth;
	Info.left = kNumShipsCol;
    traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
	aCaption->FinishCreate();
	if (inShip->GetThingySubClassType() == thingyType_Fleet) {
      aCaption->SetValue( inShip->GetContentsCountForType(contents_Ships) );
    } else {
      aCaption->SetDescriptor("\p-");
    }
	
	// Create Damage Percent Caption
	Info.width = kDamagePercentWidth;
	Info.left = kDamagePercentCol;
    traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
	aCaption->FinishCreate();
	s = inShip->GetDamagePercent();
	s += "\p %";
    aCaption->SetDescriptor(s);
	
	// Create Colony Y/N Caption
	Info.width = kColonyWidth;
	Info.left = kColonyCol;
    traitsID = 156;      // centered
	aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
	aCaption->FinishCreate();
	bool isColony = false;
	if (inShip->GetShipClass() == class_Colony) {
	   isColony = true;
	} else if (inShip->GetThingySubClassType() == thingyType_Fleet) {
	   CFleet* fleet = static_cast<CFleet*>(inShip);
	   isColony = fleet->HasShipClass(class_Colony);
	} 
	if (isColony) {
	   aCaption->SetDescriptor("\pY");
	} else {
	   aCaption->SetDescriptor("\pN");
	}
	
	// Create Destination Caption
	Info.width = kDestinationWidth;
	Info.left = kDestinationCol;
    traitsID = 146;
	aCaption = new LCaption(Info, (unsigned char *)"", traitsID);
	aCaption->FinishCreate();
	std::string destStr;
	if (inShip->IsOnPatrol()) {
	   LoadStringResource(destStr, STRx_General, str__on_patrol);
	} else if (inShip->CoursePlotted()) {
	   Waypoint wp = inShip->GetDestination();
	   wp.GetDescription(destStr);
	} else {
	   destStr = '-';
	}
    s = destStr.c_str();
	aCaption->SetDescriptor(s);
	
}

void CFleetInfoLine::SetBackColor(RGBColor Color)
{
	fBackColor = Color;
}

void CFleetInfoLine::DrawSelf()
{
RGBColor	Color = {0,0,0};
Rect	r;

	CalcLocalFrameRect(r);	
	::RGBForeColor(&fBackColor);
	
	Pattern black;
	UQDGlobals::GetBlackPat(&black);
	::MacFillRect(&r, &black);
}

void CFleetInfoLine::Click(SMouseDownEvent &inMouseDown) {
   // don't bother to look at 
   LPane::Click(inMouseDown);	//   will process click on this View
}

void CFleetInfoLine::ClickSelf(const SMouseDownEvent &) {
   if (GetClickCount() == 2) {
      // we double-clicked a line, jump to that star
      if (mShip) {
         mShip->Select();
         GalacticaDoc* doc = mShip->GetGameDoc();
         if (doc) {
            doc->CloseFleetsWindow();
         }
      }
   }
}


