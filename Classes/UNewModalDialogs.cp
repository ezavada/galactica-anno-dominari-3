// ===========================================================================
//	UNewModalDialogs.cp		   ©1995-1996 Metrowerks Inc. All rights reserved.
// portions copyright ©1997 Sacred Tree Software, inc.
// ===========================================================================
//
//	Utilities for handling (moveable) modal dialog boxes


// ===========================================================================
// StNewDialogHandler
// ===========================================================================
//
//	Same as standard Metrowerks powerplant class StDialogHandler, but adds
//	support for balloon help.

#include "UNewModalDialogs.h"
#include "CBalloonApp.h"
#include "CHelpAttach.h"
#include <UEnvironment.h>
#include <LWindow.h>
#include <UWindows.h>
#include <UCursor.h>

#define MOUSE_LINGER_TICKS	120

StNewDialogHandler::StNewDialogHandler(ResIDT inDialogResID, LCommander *inSuper)
	: StDialogHandler(inDialogResID, inSuper) {
	mHasBalloonHelp = UEnvironment::HasGestaltAttribute(gestaltHelpMgrAttr,gestaltHelpMgrPresent);
}


StNewDialogHandler::~StNewDialogHandler() {
}


// Handle cursor motion for help balloons
void StNewDialogHandler::AdjustCursor(
	const EventRecord	&inMacEvent)
{
	bool		useArrow = true;	// Assume cursor will be the Arrow
	

	WindowPtr	macWindowP;
	Point		globalMouse = inMacEvent.where;
	WindowPartCode	part = ::MacFindWindow(globalMouse, &macWindowP);

	mMouseRgn.Clear();				// Start with an empty mouse region

	if (macWindowP != nil) {		// Mouse is inside a Window

		LWindow	*theWindow = LWindow::FetchWindowObject(macWindowP);

		if ( (theWindow != nil)  &&			// Mouse is inside an active
			 theWindow->IsActive()  &&		//   and enabled PowerPlant
			 theWindow->IsEnabled() ) {		//   window
			 
			useArrow = false;
			Point	portMouse = globalMouse;
			theWindow->GlobalToPortPoint(portMouse);
			
			if (part == inContent) {
				theWindow->AdjustContentMouse(portMouse, inMacEvent, mMouseRgn);

			} else {
				theWindow->AdjustStructureMouse(part, inMacEvent, mMouseRgn);
			}

// -------------------- Start of help line code ---------------------

			UInt32 tick = ::TickCount();

			// Find the top pane under the mouse
			LPane* hitPane = theWindow->FindDeepSubPaneContaining(portMouse.h, portMouse.v);
			if (hitPane!=CBalloonApp::sLastHelpLine) {		// Check if it's different from the last time
/*				if (CBalloonApp::sHelpLineDisplay) {	// clear old display
					CBalloonApp::sHelpLineDisplay->SetDescriptor("\p");
					CBalloonApp::sHelpLineDisplay->UpdatePort();
				} */
				CBalloonApp::sLastHelpLine = hitPane;				// Cache it
				CBalloonApp::sLastMouseMoveTick = tick;	// save the time we last moved
				// when balloon help is off, we need to check for a balloon drawn
				// because of a lingering mouse and remove it.
              #if BALLOON_SUPPORT
				if (CBalloonApp::sMouseLingerActive) {
					CBalloonApp::sMouseLingerActive = false;
					::HMRemoveBalloon();	// turn off balloon help, which was only active to show
					::HMSetBalloons(false);	// this one item.
				}
			  #endif //BALLOON_SUPPORT
/*				if (hitPane) {
					hitPane->ExecuteAttachments(msg_ShowHelpLine, (void*) hitPane);
				} */
			}

// --------------- Start of balloon help support code ---------------
#if BALLOON_SUPPORT		// ERZ Win32 Mod -- Balloon help not available in Windows

			if (mHasBalloonHelp) {
				bool balloonHelpActive = ::HMGetBalloons();
				// ERZ Mod, v1.2fc1, check for lingering mouse and show balloon
				// ERZ Mod, v1.2fc4, only permit mouse linger in windows with UserCon set to 'help'
				if (CBalloonApp::sMouseLingerEnabled && !CBalloonApp::sMouseLingerActive
						&& !balloonHelpActive && (theWindow->GetUserCon() == 'help')) {
					// mouse is not already lingering, and balloon help is not active, so see if
					// we need to activate balloon help because of a lingering mouse
					if (hitPane && ((CBalloonApp::sLastMouseMoveTick + MOUSE_LINGER_TICKS) < tick)) {
						CBalloonApp::sMouseLingerActive = true;	// mouse lingered in same pane long enough to show the balloon
						::HMSetBalloons(true);	// turn on balloon help so we can display this balloon
						balloonHelpActive = true;
					}
				}
				if (balloonHelpActive) {	// Only do if balloons currently being shown
					if (hitPane!=CBalloonApp::sHelpPane) {		// Check if it's different from the last time
						CBalloonApp::sHelpPane = hitPane;		// Cache it
						if (hitPane) {	// Execute the CHelpAttach if the pane exists
							hitPane->ExecuteAttachments(msg_ShowHelp, (void*) hitPane);
						}
					}
				}
			}

#endif // BALLOON_SUPPORT
// --------------- End of balloon help support code ---------------
		}
	}
	
	if (mMouseRgn.IsEmpty()) {		// No Pane set the mouse region

		mMouseRgn = ::GetGrayRgn();	// Gray region is desktop minus menu bar
		
									// Add bounds of main device so mouse
									//   region includes the menu bar
		GDHandle	mainGD = ::GetMainDevice();
		mMouseRgn += (**mainGD).gdRect;
		
									// Exclude structure regions of all
									//   active windows
		UWindows::ExcludeActiveStructures(mMouseRgn);
	}
	
	
	if (useArrow) {					// Window didn't set the cursor
		UCursor::SetArrow();		// Default cursor is the arrow	
	}	
}
