// Galactica Tutorial.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 11/29/97 ERZ
// completely rewritten, 9/27/99, ERZ


#include "GalacticaTutorial.h"

#if TUTORIAL_SUPPORT

#include "GalacticaConsts.h"
#include "USound.h"

#include <UModalDialogs.h>
#include <LListener.h>
#include <LString.h>
#include <LWindow.h>
#include <UWindows.h>
#include <UEnvironment.h>
#include <UDesktop.h>

#include <Files.h>
#include <Resources.h>

#define DEFAULT_PAGE_HEIGHT	120
#define DEFAULT_PAGE_WIDTH	200

#define MIN_DISTANCE_FROM_EDGES	4	// for Window placements




Tutorial*	Tutorial::sTutorial = NULL;
bool		Tutorial::sActive = false;
FSSpec		Tutorial::sFSSpec;

TutorialWaiter*		TutorialWaiter::sTutorialWaiter = NULL;



Tutorial*
Tutorial::GetTutorial() {
	if (!sTutorial) {
		sTutorial = new Tutorial();
	}
	return sTutorial;
}

// constructor -- protected
Tutorial::Tutorial() {
	mRefNum = -1;
	mSuper = NULL;
	mWindow = NULL;
	mButton = NULL;
	mPicture = NULL;
	mTitle = NULL;
	mTitleBackground = NULL;
	mText = NULL;
	mCurrPageData = NULL;
	mWindStructExtraHeight = 0;
	mWindStructExtraWidth = 0;
}

void
Tutorial::StartTutorial(LCommander* inSuperCommander, SInt16 inAtPageNum) {
	mSuper = inSuperCommander;
	if (mRefNum == -1) {
		FSSpec fileSpec;
		LStr255 s((short)STR_AGTutorialName, (short)0);
		if (::FSMakeFSSpec(0,0, s, &fileSpec) == noErr) {
			mRefNum = ::FSpOpenResFile(&fileSpec, fsRdPerm);	// open tutorial file
			sFSSpec = fileSpec;
			mWindow = LWindow::CreateWindow(window_Tutorial, inSuperCommander);
			if (mWindow) {
				mText = dynamic_cast<LTextEditView*>(mWindow->FindPaneByID(1));
				mButton = dynamic_cast<LControl*>(mWindow->FindPaneByID(2));	// find the button
				mTitle = mWindow->FindPaneByID(3);	// the title caption
				mTitleBackground = mWindow->FindPaneByID(4);	// background for the title caption
				mPicture = dynamic_cast<LPicture*>(mWindow->FindPaneByID(5));	// find the picture
				if (mButton) {	// we will listen to the button, and pass messages on to our supercommander
					mButton->AddListener(this);
					DEBUG_OUT("Button now has listener", DEBUG_TRIVIA | DEBUG_TUTORIAL);
				}
				Rect contentRect = UWindows::GetWindowContentRect(mWindow->GetMacWindow());
				Rect structRect = UWindows::GetWindowStructureRect(mWindow->GetMacWindow());
				// extraHeight is the difference between struct rect and content rect heights, which
				// will remain constant as long as the window type doesn't change
				mWindStructExtraHeight = (contentRect.top - structRect.top) + (structRect.bottom - contentRect.bottom);
				mWindStructExtraWidth = (contentRect.left - structRect.left) + (structRect.right - contentRect.right);
				DEBUG_OUT("Started Tutorial", DEBUG_IMPORTANT | DEBUG_TUTORIAL);
			} else {
				DEBUG_OUT("Couldn't create window id [" << window_Tutorial << "]", DEBUG_ERROR);
			}
		} else {
			DEBUG_OUT("Couldn't open tutorial file ["<<s.TextPtr()<<"]", DEBUG_ERROR);
			::ParamText("\pCouldn't find tutorial file.","\p","\p","\p");
			UModalAlerts::Alert(alert_GenericErrorAlert);
		}
	}
	LoadPage(inAtPageNum);
	sActive = true;
}

void
Tutorial::LoadPage(SInt16 inPageNum) {
	if (inPageNum == 0) {
		mPageNum++;
	} else if (inPageNum == -1) {	// end of tutorial
		DEBUG_OUT("LoadPage(-1): Stopping tutorial.", DEBUG_IMPORTANT | DEBUG_TUTORIAL);
		mWindow->DoClose();
		StopTutorial();
		return;
	} else {
		mPageNum = inPageNum;
	}
	mCurrPageData = NULL;
	DEBUG_OUT("Loading tutorial page ["<<mPageNum<<"]", DEBUG_IMPORTANT | DEBUG_TUTORIAL);
	if (UEnvironment::IsRunningOSX()) {
	   mCurrPageData = ::GetResource('page', mPageNum + 1000); // try to load the OSX alt page data
	}
	if (mCurrPageData == NULL) {
	   mCurrPageData = ::GetResource('page', mPageNum);	// get the standard page data
	}
	if (mCurrPageData == NULL) {
		DEBUG_OUT("Failed to get page resource "<<mPageNum<< " for tutorial", DEBUG_ERROR);
		return;
	}
	#ifdef DEBUG
	{	// in debug mode set the window title to the page number in the tutorial
		LStr255 s((SInt32)mPageNum);
		mWindow->SetDescriptor(s);
	}
	#endif
	
	pageResT page;
	pageResHnd pageRes = (pageResHnd)mCurrPageData;
	ResIDT resID;
	
	// do endian swapping for the tutorial resource, which is stored in Big Endian format
	page.variant = EndianS16_BtoN((**pageRes).variant);
	page.pageHeight = EndianS16_BtoN((**pageRes).pageHeight);
	page.pageWidth = EndianS16_BtoN((**pageRes).pageWidth);
	page.titleSTRxID = EndianS16_BtoN((**pageRes).titleSTRxID);
	page.titleSTRxIndex = EndianS16_BtoN((**pageRes).titleSTRxIndex);
	page.mainTextID = EndianS16_BtoN((**pageRes).mainTextID);
	page.buttonSTRxID = EndianS16_BtoN((**pageRes).buttonSTRxID);
	page.buttonSTRxIndex = EndianS16_BtoN((**pageRes).buttonSTRxIndex);
	page.pictID = EndianS16_BtoN((**pageRes).pictID);
	page.pictLocation.top = EndianS16_BtoN((**pageRes).pictLocation.top);
	page.pictLocation.left = EndianS16_BtoN((**pageRes).pictLocation.left);
	page.pictLocation.bottom = EndianS16_BtoN((**pageRes).pictLocation.bottom);
	page.pictLocation.right = EndianS16_BtoN((**pageRes).pictLocation.right);
	page.sndID = EndianS16_BtoN((**pageRes).sndID);
	page.bindToWindowID = EndianS32_BtoN((**pageRes).bindToWindowID);
	page.nextPage = EndianS16_BtoN((**pageRes).nextPage);
	page.exceptionCount = EndianS16_BtoN((**pageRes).exceptionCount);
	for (int i = 0; i<page.exceptionCount; i++) {
		page.exceptions[i].frontWindID = EndianS32_BtoN((**pageRes).exceptions[i].frontWindID);
		page.exceptions[i].gotoPage = EndianS16_BtoN((**pageRes).exceptions[i].gotoPage);
	}
	// boolean flags don't need to be byte swapped, just copied
	page.positionAtTop = (**pageRes).positionAtTop;
	page.positionAtLeft = (**pageRes).positionAtLeft;
	page.positionAtBottom = (**pageRes).positionAtBottom;
	page.positionAtRight = (**pageRes).positionAtRight;
	
	
	// set window size if needed
	SDimension16 windSize;
	mWindow->GetFrameSize(windSize);
	if (page.pageHeight != 0) {
		windSize.height = page.pageHeight;
		if (windSize.height == -1) {
			windSize.height = DEFAULT_PAGE_HEIGHT;
		}
//		windSize.height += 4;  // ERZ, 2.0.6 add some extra space
	}
	if (page.pageWidth != 0) {
		windSize.width = page.pageWidth;
		if (windSize.width == -1) {
			windSize.width = DEFAULT_PAGE_WIDTH;
		}
        if ( (mPageNum != 1) && (UEnvironment::IsRunningOSX()) ) {
            windSize.width += 16;  // ERZ, 2.1b9, add extra width to compensate for different fonts under OSX
        }                          // except for on page 1, where a fixed width graphic is used
	}
	mWindow->ResizeWindowTo(windSize.width, windSize.height);
	
	// now set window position
	Rect windRect;
	mWindow->GetGlobalBounds(windRect);
	bool positionAtTop = page.positionAtTop;
	bool positionAtBottom = page.positionAtBottom;
	bool positionAtLeft = page.positionAtLeft;
	bool positionAtRight = page.positionAtRight;
	bool bound = false;
	Rect screenRect;
	SInt16 screenHeight, screenWidth;
	PaneIDT	targetID = page.bindToWindowID;
	if (targetID != 0) {
		LWindow* targetWindow;
		bool targetWindowMoved = false;
		bool canMoveTarget = false;
		Rect targetRect;
		long diff;
		long targetExtraHeight = 0;
		long targetExtraWidth = 0;
		if (page.variant == 0) {
			DEBUG_OUT("Tutorial page "<<mPageNum<<" binding to window id ["<<
						targetID<<"]", DEBUG_DETAIL | DEBUG_TUTORIAL);
			// variant 0 is binding to a window
			if (targetID == -1) {
				// -1 means whatever window is available. So grab the top modal, or if none, the top regular 
				targetWindow = UDesktop::FetchTopModal();
				if (targetWindow == NULL) {
					targetWindow = UDesktop::FetchTopRegular();
				}
			} else {
				targetWindow = LWindow::FindWindowByID(targetID);
			}
			if (targetWindow) {
				bound = true;
				canMoveTarget = true;
				Rect contentRect = UWindows::GetWindowContentRect(targetWindow->GetMacWindow());
				Rect structRect = UWindows::GetWindowStructureRect(targetWindow->GetMacWindow());
				// extraHeight is the difference between struct rect and content rect heights, which
				// will remain constant as long as the window type doesn't change
				targetExtraHeight = (contentRect.top - structRect.top) + (structRect.bottom - contentRect.bottom);
				targetExtraWidth = (contentRect.left - structRect.left) + (structRect.right - contentRect.right);
				targetWindow->GetGlobalBounds(targetRect);
			} else {
				DEBUG_OUT("Binding to window id ["<<targetID<<"], but window not found"
						" (tutorial page "<<mPageNum<<")", DEBUG_ERROR | DEBUG_TUTORIAL);
			}
		} else {
			LPane* targetPane = NULL;
			targetWindow = UDesktop::FetchTopRegular();
			if (targetWindow) {
				if (page.variant == 1) {
					// variant 1 is binding to a pane
					DEBUG_OUT("Tutorial page "<<mPageNum<<" binding to pane id ["<<
								targetID<<"]", DEBUG_DETAIL | DEBUG_TUTORIAL);
					targetPane = targetWindow->FindPaneByID(targetID);
				} else if (page.variant == 2) {
				  #ifndef NO_GALACTICA_DEPENDENCIES
					// variant 2 is binding to a particular Galactic Thingy
					DEBUG_OUT("Tutorial page "<<mPageNum<<" binding to thingy id ["<<
								targetID<<"]", DEBUG_DETAIL | DEBUG_TUTORIAL);
					targetPane = targetWindow->FindPaneByID(view_StarMap);
					if (targetPane) {
						targetPane = targetPane->FindPaneByID(targetID);
					} else {
						DEBUG_OUT("Tutorial page "<<mPageNum<<" binding to thingy id ["<<
									targetID<<"] couldn't find starmap in window id ["<<
									targetWindow->GetPaneID()<<"]", DEBUG_ERROR| DEBUG_TUTORIAL);
					}
				  #else
					DEBUG_OUT("Used Galactica specific variant 2 for page "<<mPageNum, DEBUG_ERROR);
				  #endif // NO_GALACTICA_DEPENDENCIES
				}
				if (targetPane) {
					// we found the target pane, find out where it is in global coordinates
					bound = true;
					targetPane->CalcPortFrameRect(targetRect);
					targetPane->PortToGlobalPoint(topLeft(targetRect));
					targetPane->PortToGlobalPoint(botRight(targetRect));					
				}
			} else {
				DEBUG_OUT("Tutorial page "<<mPageNum<<" binding to pane in top non-modal window, but no "
						"non-modal window found", DEBUG_ERROR | DEBUG_TUTORIAL);
			}
		}
		if (bound) {
			GDHandle dominantDevice = UWindows::FindDominantDevice(targetRect);
			if (dominantDevice == nil) { // Window is offscreen, so use main screen
				dominantDevice = ::GetMainDevice();
				DEBUG_OUT("Target id ["<<targetID<<"] is offscreen"
						" (tutorial page "<<mPageNum<<")", DEBUG_ERROR | DEBUG_TUTORIAL);
			}
			screenRect = (**dominantDevice).gdRect;
			if (dominantDevice == ::GetMainDevice()) {
				screenRect.top += ::GetMBarHeight(); // don't overwrite menu bar on main device
			}
			// place it relative to a specific window
			// adjust vertical
			if (positionAtTop && !positionAtBottom) {			// put above target window
				windRect.top = targetRect.top - (windSize.height + targetExtraHeight + MIN_DISTANCE_FROM_EDGES);
				diff = windRect.top - (screenRect.top + MIN_DISTANCE_FROM_EDGES + mWindStructExtraHeight);
				if (diff < 0) {	// above top of screen
					// move the windows down to fit everything in
					targetRect.top -= diff;
					windRect.top -= diff;
					targetWindowMoved = true;
				}
			} else if (positionAtBottom && !positionAtTop) {	// put underneath target window
				windRect.top = targetRect.bottom + mWindStructExtraHeight + MIN_DISTANCE_FROM_EDGES;
				diff = screenRect.bottom - (windRect.top + windSize.height + MIN_DISTANCE_FROM_EDGES);
				if (diff < 0) {	// below bottom of screen
					// move the windows up to fit everything in
					targetRect.top += diff;
					windRect.top += diff;
					targetWindowMoved = true;
				}
			} else if (positionAtBottom && positionAtTop) {		// center vertically relative to target window
				diff = (targetRect.bottom + targetRect.top)/2;	// find vertical center of target window
				windRect.top = diff - (windSize.height/2);
			}
			// adjust horizontal
			if (positionAtLeft && !positionAtRight) {			// put to left of target window
				windRect.left = targetRect.left - (windSize.width + mWindStructExtraWidth + MIN_DISTANCE_FROM_EDGES);
				diff = windRect.left - (screenRect.left + MIN_DISTANCE_FROM_EDGES + mWindStructExtraWidth);
				if (diff < 0) {	// goes off the left side of screen
					// move the windows right to fit everything in
					targetRect.left -= diff;
					windRect.left -= diff;
					targetWindowMoved = true;
				}
			} else if (positionAtRight && !positionAtLeft) {	// put to right of target window
				windRect.left = targetRect.right + mWindStructExtraWidth + MIN_DISTANCE_FROM_EDGES;
				diff = screenRect.right - (windRect.left + windSize.width + MIN_DISTANCE_FROM_EDGES);
				if (diff < 0) {	// goes off the right side of screen
					// move the windows left to fit everything in
					targetRect.left += diff;
					windRect.left += diff;
					targetWindowMoved = true;
				}
			} else if (positionAtRight && positionAtLeft) {		// center horizontally relative to window
				diff = (targetRect.right + targetRect.left)/2;
				windRect.left = diff - (windSize.width/2);
			}
			if (canMoveTarget && targetWindowMoved) {	// we may not be allowed to move the target, since
				// it may be a pane rather than a window
				if (targetRect.left < 0) {
					targetRect.left = 0;
				}
				if (targetRect.top < 0) {
					targetRect.top = 0;
				}
				targetWindow->MoveWindowTo(targetRect.left, targetRect.top);
				DEBUG_OUT("Tutorial page "<<mPageNum<<" moved bind-to window", DEBUG_DETAIL | DEBUG_TUTORIAL);
			}
		}
	}
	if (!bound) {
		GDHandle mainDevice = ::GetMainDevice();
		screenRect = (**mainDevice).gdRect;
		screenRect.top += ::GetMBarHeight(); // don't overwrite menu bar on main device
		// not binding to a window, just to screen
		if (positionAtTop && !positionAtBottom) {			// put at top of screen
			windRect.top = screenRect.top + mWindStructExtraHeight + MIN_DISTANCE_FROM_EDGES;
		} else if (positionAtBottom && !positionAtTop) {	// put at bottom of screen
			windRect.top = screenRect.bottom - windSize.height 
							- mWindStructExtraHeight - MIN_DISTANCE_FROM_EDGES;
		} else if (positionAtBottom && positionAtTop) {		// center vertically
			screenHeight = screenRect.bottom - screenRect.top;
			windRect.top = screenRect.top + (screenHeight - windSize.height 
							- mWindStructExtraHeight)/2;
		}
		if (positionAtLeft && !positionAtRight) {			// put at left side of screen
			windRect.left = mWindStructExtraWidth + MIN_DISTANCE_FROM_EDGES;
		} else if (positionAtRight && !positionAtLeft) {	// put at right side of screen
			windRect.left = screenRect.right - windSize.width 
							- mWindStructExtraWidth - MIN_DISTANCE_FROM_EDGES;
		} else if (positionAtRight && positionAtLeft) {		// center horizontally
			screenWidth = screenRect.right - screenRect.left;
			windRect.left = screenRect.left + (screenWidth - windSize.width 
							- mWindStructExtraWidth)/2;
		}
	}
	mWindow->MoveWindowTo(windRect.left, windRect.top);

	// set next page
	mNextPageNum = page.nextPage;

	// set the title text, or hide it and the grey background if none
	if (mTitle && mTitleBackground) {
		resID = page.titleSTRxID;
		if (resID != 0) {
			LStr255 title(resID, page.titleSTRxIndex);
			if (title.Length() == 0) {	// blank name means hide the title
				mTitle->Hide();
				mTitleBackground->Hide();
			} else {
				mTitle->SetDescriptor(title);
				mTitle->Show();
				mTitleBackground->Show();
			}
		} else {
			mTitle->Hide();	// title STR# id = 0 means hide title and background
			mTitleBackground->Hide();
		}
	}

	// set the button text, or hide the button if none set
	if (mButton) {
		resID = page.buttonSTRxID;
		if (resID != 0) {
			LStr255 buttonName(resID, page.buttonSTRxIndex);
			if (buttonName.Length() == 0) {	// blank name means hide the button
				mButton->Hide();
			} else {
				mButton->SetDescriptor(buttonName);
				mButton->Show();
			}
		} else {
			mButton->Hide();	// button STR# id = 0 means hide button
		}
	}

	// set the picture, or hide it if none set
	if (mPicture) {
		resID = page.pictID;
		if (resID != 0) {
			Rect r = page.pictLocation;
			mPicture->PlaceInSuperFrameAt(r.left, r.top, false);
			mPicture->ResizeFrameTo(r.right - r.left, r.bottom - r.top, false);
			mPicture->SetPictureID(resID);
			mPicture->Show();
		} else {
			mPicture->Hide();	// pict id = 0 means hide picture
		}
	}

	// set the main text
	if (mText) {
		resID = page.mainTextID;
		if (resID == -1) {	// default
			resID = inPageNum + window_Tutorial - 1;
		}
		Handle theTextH = ::GetResource('TEXT', resID);
		Handle theStyleH = ::GetResource('styl', resID);
		mText->SetTextHandle(theTextH, (StScrpHandle)theStyleH);
		if (theTextH) {
			::ReleaseResource(theTextH);
		}
		if (theStyleH) {
			::ReleaseResource(theStyleH);
		}
	}

	// play the sound if there was one
	resID = page.sndID;
	if (resID != 0) {	// zero means none.
		if (resID == -1)	{	// use default
			resID = snd_TutorialAttention;
		}
		new CSoundResourcePlayer(resID);
	}

	// make sure everything gets redrawn
	mWindow->Refresh();
	mWindow->UpdatePort();
}


void
Tutorial::NextPage() {
	DEBUG_OUT("Tutorial leaving page "<<mPageNum<<", picking next page", DEBUG_IMPORTANT | DEBUG_TUTORIAL);
	if (mCurrPageData != NULL) {
		pageResHnd pageRes = (pageResHnd)mCurrPageData;
		int exceptionCount = EndianS16_BtoN((**pageRes).exceptionCount);
		DEBUG_OUT("Page "<<mPageNum<<" has "<<exceptionCount<<" exceptions", DEBUG_DETAIL | DEBUG_TUTORIAL);
		if (exceptionCount > 0) {
			// evaluate the exceptions
			LWindow* top = UDesktop::FetchTopModal();
			if (!top) {	// no modal on top, try getting a regular window
				top = UDesktop::FetchTopRegular();
			}
			if (top) {	// use the normal nextPage if there is no window open
				PaneIDT id = top->GetPaneID();
				DEBUG_OUT("Top window is ID ["<<id<<"]", DEBUG_DETAIL | DEBUG_TUTORIAL);
				for (int i = 0; i < exceptionCount; i++) {
					// find the exception that matches our topmost window,
					// of the -1 exception to handle all cases where there is a window
					// not previously listed
					PaneIDT targetID = EndianS32_BtoN((**pageRes).exceptions[i].frontWindID);
					if ( (targetID == id) || (targetID == -1) ) {
						mNextPageNum = EndianS16_BtoN((**pageRes).exceptions[i].gotoPage);
						DEBUG_OUT("Found exception for ID ["<<targetID<<"], going to page ["<<
									mNextPageNum<<"]", DEBUG_DETAIL | DEBUG_TUTORIAL);
						break;	// stop looking
					}
				}
			} else {
				DEBUG_OUT("No top window, ignoring exceptions", DEBUG_DETAIL | DEBUG_TUTORIAL);
			}
		} 
		LoadPage(mNextPageNum);
	} else {
		DEBUG_OUT("NextPage failed: No data for page ["<<mPageNum<<"]", DEBUG_ERROR | DEBUG_TUTORIAL);
	}
}
 
void
Tutorial::StopTutorial() {
	if (mWindow) {
		// don't delete it here, this is called from GalacticaApp::AllowSubRemoval(), which
		// is only called when the window is about to delete itself
		mWindow = NULL;
		mButton = NULL;
		mText = NULL;
		mPicture = NULL;
		mTitle = NULL;
		mTitleBackground = NULL;
		mText = NULL;
		mCurrPageData = NULL;
	}
	if (mRefNum != -1) {
		::CloseResFile(mRefNum);
		mRefNum = -1;
	}
	mSuper = NULL;
	sActive = false;
}


void
Tutorial::ListenToMessage(MessageT inMessage, void* ioParam) {
	if (inMessage == msg_BroadcasterDied) {
		StopTutorial();
	}
	if (mSuper) {	// just pass the message on to the supercommander for this tutorial
		DEBUG_OUT("Sending message ["<<inMessage<<"] on to supervisor.", DEBUG_DETAIL | DEBUG_TUTORIAL);
		mSuper->ObeyCommand(inMessage, ioParam);
	}
}


// ---------------------------------------------------------------------------
//	¥ SpendTime
// ---------------------------------------------------------------------------

void
TutorialWaiter::SpendTime( const EventRecord&) {	//inMacEvent
	if (::TickCount() > mTimeUpTicks) {
		StopIdling(); 
		if (Tutorial::TutorialIsActive()) {
			Tutorial* t = Tutorial::GetTutorial();
			if (mPageNum == -1) { 
				t->ListenToMessage(cmd_TutorialOK, nil);
			} else {
				t->GotoPage(mPageNum);
			}
		}
		if (mAutoDelete) { 
			delete this; 
		}
	} 
}

#endif //TUTORIAL_SUPPORT
