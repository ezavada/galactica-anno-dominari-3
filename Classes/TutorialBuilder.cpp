#include "TutorialBuilder.h"
#include <MetroNubUtils.h>
#include "DebugMacros.h"
#include "GalacticaConsts.h"

extern int gDebugBreakOnErrors;	// from debugmacros.cp

#define TUTORIAL_BASE_RES_ID	7000

void main(void) {

	// ---------------------------
	// Set Debugging options
	// ---------------------------

	DEBUG_INITIALIZE();


#ifdef DEBUG

  #ifdef CHECK_FOR_MEMORY_LEAKS
	gDebugNewFlags |= dnDontFreeBlocks;
  #endif

  #if TARGET_OS_MAC	
	if (AmIBeingMWDebugged() == true) {
		SetDebugThrow_(debugAction_Nothing);//debugAction_LowLevelDebugger);
		SetDebugSignal_(debugAction_Nothing);//debugAction_LowLevelDebugger);
		DEBUG_SET_LEVEL(DEBUG_SHOW_MOST);
		DEBUG_ENABLE_ERROR(DEBUG_TUTORIAL);
		gDebugBreakOnErrors = true;			// metronub should catch debugger traps
	} else {
		SetDebugSignal_(debugAction_Nothing);
		SetDebugThrow_(debugAction_Nothing);
		DEBUG_SET_LEVEL(DEBUG_SHOW_ERRORS);
		gDebugBreakOnErrors = false;
	}
  #else	// TARGET_OS_WIN32
	SetDebugThrow_(debugAction_Nothing);//debugAction_LowLevelDebugger);
	SetDebugSignal_(debugAction_Nothing);//debugAction_LowLevelDebugger);
	DEBUG_SET_LEVEL(DEBUG_SHOW_SOME);
	gDebugBreakOnErrors = true;		// metronub should catch debugger traps
	gUseDebugMenu = true;
  #endif	// TARGET_OS_MAC
  
#else	// non debug version
	SetDebugSignal_(debugAction_Nothing);
	SetDebugThrow_(debugAction_Nothing);
	DEBUG_SET_LEVEL(DEBUG_SHOW_ERRORS);
#endif


	// ---------------------------
	// MacOS Initialization
	// ---------------------------
  #if TARGET_OS_MAC
	InitializeHeap(14);	// Init Memory Manager: Param is num Master Ptr blocks to allocate
  #if !TARGET_API_MAC_CARBON
	UQDGlobals::InitializeToolbox(&qd);
  #endif // ! TARGET_CARBON
  #endif // TARGET_OS_MAC


	TutorialBuilderApp*	theApp = new TutorialBuilderApp();			// replace this with your App type
	theApp->Run();

	delete theApp;

	DEBUG_TERMINATE();
}



TutorialBuilderApp::TutorialBuilderApp() {
	mPrevPageHadButton = false;
//	RegisterClass_(LButton);
	RegisterClass_(LCaption);
	RegisterClass_(LDialogBox);
//	RegisterClass_(LEditField);
	RegisterClass_(LPane);
	RegisterClass_(LPicture);
//	RegisterClass_(LScroller);
	RegisterClass_(LStdControl);
	RegisterClass_(LStdButton);
	RegisterClass_(LStdCheckBox);
//	RegisterClass_(LStdRadioButton);
//	RegisterClass_(LStdPopupMenu);
	RegisterClass_(LTextEditView);
	RegisterClass_(LView);
	RegisterClass_(LWindow);
//	RegisterClass_(LRadioGroup);
//	RegisterClass_(LTabGroup);
//	RegisterClass_(LCicnButton);
//	RegisterClass_(LOffscreenView);
//	RegisterClass_(LActiveScroller);
//	RegisterClass_(LTable);
//	RegisterClass_(LIconPane);
	RegisterClass_(LPaintAttachment);
//	RegisterClass_(LGroupBox);
//	RegisterClass_(LTextButton);
//	RegisterClass_(LToggleButton);
}



void
TutorialBuilderApp::OpenDocument(FSSpec* inMacFSSpec) {
	short srcRefNum = ::FSpOpenResFile(inMacFSSpec, fsRdPerm);
	LStr255 dstName = "Galactica Tutorial"; //inMacFSSpec->name;
//	dstName += ".rsrc";
	FSSpec dstSpec;
	::FSMakeFSSpec(inMacFSSpec->vRefNum, inMacFSSpec->parID, dstName, &dstSpec);
	::FSpCreateResFile(&dstSpec, 'Tré5', 'rsrc', 0);
	short dstRefNum = ::FSpOpenResFile(&dstSpec, fsWrPerm);
	::UseResFile(srcRefNum);
	short ppobCount = ::Count1Resources('PPob');
	mPrevPageHadButton = false;
	for (int i = 1; i <= ppobCount; i++) {
		Handle resH = ::Get1IndResource('PPob', i);
		ResIDT theID;
		ResType theType;
		Str255 theResName;
		::GetResInfo(resH, &theID, &theType, theResName);
		if (theID > 0) {	// wouldn't work with many negative numbers anyway
			LWindow* wind = LWindow::CreateWindow(theID, this);
			wind->SetDescriptor(theResName);
			wind->Show();
			wind->UpdatePort();
			SavePageToResFile(wind, dstRefNum);
			delete wind;
		}
		::ReleaseResource(resH);
	}
	
	::CloseResFile(srcRefNum);
	::CloseResFile(dstRefNum);
}

void
TutorialBuilderApp::SavePageToResFile(LWindow *inPageWind, short inRefNum) {
	ResIDT id = inPageWind->GetPaneID();
	Str255 resName;
	
	inPageWind->GetDescriptor(resName);
	short oldRefNum = ::CurResFile();
	::UseResFile(inRefNum);
	pageResHnd pageH = (pageResHnd) ::Get1Resource('page', id);
	if (!pageH) {
		pageH = (pageResHnd)::NewHandle(sizeof(pageResT));
		// make this a resource handle
		::AddResource((Handle)pageH, 'page', id, resName);
		// initialize the stuff that doesn't get edited through our tool
		// no way to do bindings at this point
		(**pageH).bindToWindowID = 0;
		(**pageH).positionAtTop = false;
		(**pageH)._unused1 = 0;
		(**pageH).positionAtLeft = false;
		(**pageH)._unused2 = 0;
		(**pageH).positionAtBottom = false;
		(**pageH)._unused3 = 0;
		(**pageH).positionAtRight = false;
		(**pageH)._unused4 = 0;
		(**pageH).nextPage = 0;
		(**pageH).exceptionCount = 0;
		(**pageH).sndID = -1;	// we will set this below, but only if it's -1
	}

	// take the varient from the user con.
	(**pageH).variant = inPageWind->GetUserCon();
	
	// set page size
	SDimension16 windSize;
	inPageWind->GetFrameSize(windSize);
	(**pageH).pageHeight = windSize.height;
	(**pageH).pageWidth = windSize.width;
	
	// set the title
	LPane* title = inPageWind->FindPaneByID(-3);	// the title caption
	Str255 titleStr;
	title->GetDescriptor(titleStr);
	ResIDT	titleSTRxID;
	Handle titleH;
	if (titleStr[0] > 0) {
		titleH = ::Get1NamedResource('STR ', titleStr);
		if (!titleH) {
			// no such string found, add it to the resource file
			titleSTRxID = FindUnusedResourceID('STR ', TUTORIAL_BASE_RES_ID);
			titleH = ::NewHandle(titleStr[0]+1);
			::BlockMoveData(titleStr, *titleH, titleStr[0]+1);	//pascal string
			::AddResource(titleH, 'STR ', titleSTRxID, titleStr);
			::ReleaseResource(titleH);
		} else {
			// found a matching string, find out its ID
			ResType junk;
			Str255 junkStr;
			::GetResInfo(titleH, &titleSTRxID, &junk, junkStr);
		}
	} else {
		titleSTRxID = 0;
	}
	(**pageH).titleSTRxID = titleSTRxID;
	(**pageH).titleSTRxIndex = 0;
	
	// set the text
	ResIDT	mainTextID = id + TUTORIAL_BASE_RES_ID;
	(**pageH).mainTextID = mainTextID;
	// delete the exiting TEXT resource
	Handle textH = ::Get1Resource('TEXT', mainTextID);
	if (textH) {
		::RemoveResource(textH);
		::DisposeHandle(textH);
	}
	textH = ::Get1Resource('styl', mainTextID);
	if (!textH) {	// no existing style resource, this must be new
		textH = ::GetResource('styl', 0);	// add a default style resource
		if (textH) {
			::DetachResource(textH);	// detach it from the application and add it
			::AddResource(textH, 'styl', mainTextID, resName);	// to the tutorial file
			::ReleaseResource(textH);
		}
	} else {
		::ReleaseResource(textH);	// already have a styl resource, leave it alone
	}
	// now get and copy the new contents
	LPane* text = inPageWind->FindPaneByID(-1);
	Str255 textStr;
	text->GetDescriptor(textStr);
	textH = ::NewHandle(textStr[0]);
	::BlockMoveData(&textStr[1], *textH, textStr[0]);	// text block, no nul terminator
	// write out a new resource for the text
	::AddResource(textH, 'TEXT', mainTextID, resName);
	::ReleaseResource(textH);

	// set up the sound, assuming it hasn't been set ever before
	if ((**pageH).sndID == -1) {
		if (mPrevPageHadButton) {
			(**pageH).sndID = 0;		// generally don't play a sound when coming from a 
		} else {				// page with a button, since the user sees the transition.
			(**pageH).sndID = TUTORIAL_BASE_RES_ID;	// otherwise, set it to the default sound
		}					// of 7000 which is a pleasant ping that will draw their attention.
	}

	// set the button name
	LControl* button = dynamic_cast<LControl*>(inPageWind->FindPaneByID(-2));	// find the button
	ResIDT	buttonSTRxID;
	if (button->IsVisible()) {
		mPrevPageHadButton = true;	// so the next page knows that we had a button
		button->GetDescriptor(titleStr);
		titleH = ::Get1NamedResource('STR ', titleStr);
		if (!titleH) {
			// no such string found, add it to the resource file
			buttonSTRxID = FindUnusedResourceID('STR ', TUTORIAL_BASE_RES_ID);
			titleH = ::NewHandle(titleStr[0]+1);
			::BlockMoveData(titleStr, *titleH, titleStr[0]+1);
			::AddResource(titleH, 'STR ', buttonSTRxID, titleStr);
			::ReleaseResource(titleH);
		} else {
			// found a matching string, find out its ID
			ResType junk;
			Str255 junkStr;
			::GetResInfo(titleH, &buttonSTRxID, &junk, junkStr);
		}
	} else {
		mPrevPageHadButton = false;	// so the next page knows that we had no button
		buttonSTRxID = 0;
	}
	(**pageH).buttonSTRxID = buttonSTRxID;
	(**pageH).buttonSTRxIndex = 0;
	

	// set the picture information
	LPicture* pict = dynamic_cast<LPicture*>(inPageWind->FindPaneByID(-5));	// find the picture
	ResIDT pictID = 0;
	Rect pictLocation = {0,0,0,0};
	if (pict && (pict->IsVisible())) {
		pictID = pict->GetPictureID();
		pict->CalcPortFrameRect(pictLocation);
		// now get and copy the new PICT resource (if there is one)
		::UseResFile(oldRefNum);
		Handle pictH = ::Get1Resource('PICT', pictID);
		::UseResFile(inRefNum);
		if (pictH) {
			::DetachResource(pictH);
			// delete any old resource
			Handle pict2H = ::Get1Resource('PICT', pictID);
			if (pict2H) {
				::RemoveResource(pict2H);
				::DisposeHandle(pict2H);
			}
			// write out a new resource for the text
			::AddResource(pictH, 'PICT', pictID, resName);
			::ReleaseResource(pictH);
		}
	}
	(**pageH).pictID = pictID;
	(**pageH).pictLocation = pictLocation;

	// update the items in the resource file
	::ChangedResource((Handle)pageH);
	::UpdateResFile(inRefNum);
	
	::UseResFile(oldRefNum);
}


short
TutorialBuilderApp::FindUnusedResourceID(ResType inType, short startWith) {
	::SetResLoad(false);
	Handle resH = (Handle)-1;
	while (resH) {
		resH = ::Get1Resource(inType, startWith);
		::ReleaseResource(resH);
		startWith++;
	}
	::SetResLoad(true);
	return startWith - 1;
}
