//	GalacticaRegistration.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include <stdlib.h>

#include "GalacticaRegistration.h"
#include "Galactica.h"
#include "GalacticaGlobals.h"

#include "pdg/sys/config.h"

#ifndef GALACTICA_SERVER
  #include "LTextEditView.h"
  #include "LDialogBox.h"

  #include "GalacticaUtilsUI.h"
  #include "USound.h"

  #if BALLOON_SUPPORT
    #include <Balloons.h>
  #endif //BALLOON_SUPPORT

  extern bool gDoStartup;

#endif // GALACTICA_SERVER

#if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
    #include <UMemoryMgr.h>
#endif


#if COMPILER_MWERKS
// don't include any symbol information in the executable for the following 
// blocks of code
//#pragma sym off
#if TARGET_CPU_PPC
#pragma traceback off
#elif TARGET_CPU_68K
#pragma macsbug off
#endif
#endif // COMPILER_MWERKS

// Yes, Jamie, all this registration code is incredibly convoluted and messy. It's that
// way on purpose to make it harder to hack the game.


// ----------------- MAKE IT HARD TO HACK REGISTRATION ------------------

bool		CLoadPrefsAttach::sPrefsAttach = false;
bool		CLoadPrefsAttach::sPrefsFileWasntThere = false;
bool		DisplayRegistrationWindow::sInstalledRegistrationWind = false;
bool		DisplayRegistrationWindow::sWindBeingDisplayed = false;
bool        DisplayRegistrationWindow::sHoldStartup = true;
bool		DisplayUnregisteredWindow::sInstalledUnregisteredWind;
bool		DisplayUnregisteredWindow::sOldVersionRegistered = false;
Str27		DisplayUnregisteredWindow::sOldRegistration;
#ifndef GALACTICA_SERVER
LWindow*	DisplayRegistrationWindow::sRegistrationWind = NULL;
LPane*			CUpdateRegWin::sRegCodePane = NULL;
LControl*		CUpdateRegWin::sNotYetButton = NULL;
LControl*		CUpdateRegWin::sRegisterButton = NULL;
LControl*		CUpdateRegWin::sEnterCodeButton = NULL;
LView*			CUpdateRegWin::sButtonHolder = NULL;
#endif // GALACTICA_SERVER
unsigned long	CUpdateRegWin::sEnableTicks = (unsigned)-1;

#ifdef CHECK_REGISTRATION
bool		CCheckRegAttach::sRegistered = false;
long 		CCheckRegAttach::sVersionCode = 0;
long		CCheckTimeUpAttach::sTurnToExpireOn = 0;

#ifdef CHECK_FOR_HACKING
bool		CDoShutdownAttach::sInstalledShutdown = false;
bool		CDoShutdownAttach::sCopyRegistered = false;
#endif // CHECK_FOR_HACKING

char gConfigCvt[] = "eWdafNo.n3";  // conversion string for codes stored in the config
char gConfigCv2[] = "Mfi1#FL2aE";  // conversion string for backup codes stored in the config
static char sConverter[] = "ASDFGHJKL:";  // conversion string for blacklisted codes

// to add blacklisted codes, strip out the dashes, drop the GLEN, then use
// the conversion string to convert each digit to it's letter equivalent
// and add it to the list
static RegCoreT sinvlist[] = { 
    "JFSDLKSSHK::DJHDDA",   // GLEN-6312-8711-5799-2652-20
    "JLJDFDLHJHF::HHDDA",   // GLEN-6862-3285-6539-9552-20
    ""     // add new blacklisted reg codes above this line
};

RegCoreT gBlob;
RegCoreT gBlob2;

#endif // CHECK_REGISTRATION

#ifndef GALACTICA_SERVER

class ReRegDialogAttach : public CDialogAttachment {
public:
	ReRegDialogAttach(LStr255 regStr) : mRegStr(regStr) {}
	virtual bool PrepareDialog();
	virtual bool AutoDelete() {return true;}	// true to have the MovableModal delete attachment
protected:
	LStr255 mRegStr;
};

bool ReRegDialogAttach::PrepareDialog() {
	LPane* thePane = mDialog->FindPaneByID(100);
	if (thePane) {
		thePane->SetDescriptor(mRegStr);
		LTextEditView* tev = dynamic_cast<LTextEditView*>(thePane);
		if (tev) {
			tev->SetSelectionRange(0,0);
		}
	}
	thePane = mDialog->FindPaneByID(3);
	if (thePane) {
		LControl* ctrl = dynamic_cast<LControl*>(thePane);
		LDialogBox* dlg = dynamic_cast<LDialogBox*>(mDialog);
		if (ctrl) {
			ctrl->AddListener(dlg);
		}
	}
	return true;
}
#endif // !GALACTICA_SERVER


CMakePrefsVarAttach::CMakePrefsVarAttach()
: LAttachment(msg_CommandStatus, true) {
#ifdef CHECK_REGISTRATION
	DisplayRegistrationWindow::sInstalledRegistrationWind = false;
	DisplayRegistrationWindow::sWindBeingDisplayed = false;
  #ifndef GALACTICA_SERVER
	DisplayRegistrationWindow::sRegistrationWind = nil;
  #endif // GALACTICA_SERVER
  #ifdef CHECK_FOR_HACKING
	CDoShutdownAttach::sInstalledShutdown = false;
	CDoShutdownAttach::sCopyRegistered = false;
  #endif
#endif
}

void 
CMakePrefsVarAttach::ExecuteSelf(MessageT, void *) {
#if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
	Galactica::Globals::GetInstance().sPref = Galactica::Globals::GetInstance().sPref >> 1;
#endif
	LCommander* app = LCommander::GetTopCommander();
	app->AddAttachment(new CLoadPrefsAttach);
	delete this;
}

CLoadPrefsAttach::CLoadPrefsAttach()
: LAttachment(msg_CommandStatus, true) {
	sPrefsAttach = true;
}

void 
CLoadPrefsAttach::ExecuteSelf(MessageT, void *) {
	bool hasPrefs;
	bool convertedVersion2Prefs = false;
	bool doRegWindow = false;
	bool doReRegWindow = false;
	std::string s;
	Str27 regCodeSave;
	regCodeSave[0] = 0;
	
	sPrefsFileWasntThere = false;
	pdg::ConfigManager* config = pdg::createConfigManager();
	ThrowIfNil_(config);
	Galactica::Globals::GetInstance().setConfigManager(config);
	hasPrefs = config->useConfig("GalacticaAD3");
	
	if (!hasPrefs) {
		// do whatever we need to for case of no preferences
		sPrefsFileWasntThere = true;
      #ifdef GALACTICA_BOX_VERSION
        // for box version, we do want to do registration window very first time they launch
		doRegWindow = true;
      #endif // GALACTICA_BOX_VERSION
	} else {
		// if the preferences exist, always check the reg code
		doRegWindow = true;
	}
	
#if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX )) && !defined( PLATFORM_X86 )

	DisplayUnregisteredWindow::sOldRegistration[0] = 0;

	GalacticaPrefsT* thePrefsPtr = 0;
  	// read in old version prefs from 2.2 and earlier
	if (!hasPrefs) {
		LoadStringResource(s, STR_PrefsFileName);
		short fRefNum = -1;
		Str255 tmpstr;
		c2pstrcpy(tmpstr, s.c_str());
		DEBUG_OUT("Opening Prefs file \""<<s.c_str()<<"\"", DEBUG_IMPORTANT);
		::FSMakeFSSpec(Galactica::Globals::GetInstance().sSysVRef, Galactica::Globals::GetInstance().sPrfsFldrID, 
		                tmpstr, &Galactica::Globals::GetInstance().sPrefsSpec);
		// open or create prefs file
		fRefNum = ::FSpOpenResFile(&Galactica::Globals::GetInstance().sPrefsSpec, fsRdWrShPerm);
		if (fRefNum != -1) {
			GalacticaPrefsHnd prefs;
			prefs = (GalacticaPrefsHnd) ::GetResource(Galactica::Globals::GetInstance().sPref, 1);
			UInt32 prefSize = ::GetHandleSize((Handle)prefs);
			if (prefSize < sizeof(Galactica12PrefsT)) {
				::ReleaseResource((Handle)prefs);
				prefs = NULL;
			}
			if (prefs) {
				hasPrefs = true; // we are getting prefs from an old version
				convertedVersion2Prefs = true;
				StHandleLocker lock((Handle)prefs);
				// the prefs are from an older (and smaller) version. Lose them and start over.
				LString::CopyPStr((**prefs).registration, regCodeSave, sizeof(Str27));
				// save the old registration code
				LString::CopyPStr((**prefs).registration, 
						DisplayUnregisteredWindow::sOldRegistration, sizeof(Str27));
				// an old registration code was in the prefs file, so assume it was valid and
				// let them know that they are eligable for an upgrade
				if (DisplayUnregisteredWindow::sOldRegistration[0] != 0) {
					doReRegWindow = true;
					doRegWindow = false;
				}
				thePrefsPtr = *prefs;
				if (prefSize < sizeof(GalacticaPrefsT) ) {
					// add in the new prefs item, defaultPlayerName, as an empy value
			    	::SetHandleSize((Handle)prefs, sizeof(GalacticaPrefsT));
			    	(**prefs).defaultPlayerName[0] = 0;
			    	::ChangedResource((Handle)prefs);
			    	::WriteResource((Handle)prefs);
			    }
				Galactica::Globals::GetInstance().setFullScreenMode((**prefs).fullScreen);
				LString::CopyPStr((**prefs).registration, Galactica::Globals::GetInstance().sRegistration, sizeof(Str27));
				char playerName[256];
				p2cstrcpy(playerName, (**prefs).defaultPlayerName);
				s = playerName;
      			config->setConfigString(GALACTICA_PREF_DEFAULT_NAME, s);
				Galactica::Globals::GetInstance().setDefaultPlayerName(playerName);
				// save the old registration code in the config
				s = Galactica::Globals::GetInstance().getRegistration();
				config->setConfigString(GALACTICA_PREF_OLD_REGISTRATION, s);
		      #ifndef GALACTICA_SERVER
				CSoundResourcePlayer::sPlaySound = (**prefs).wantSound;
				CMidiMovieFilePlayer::sPlayMusic = (**prefs).wantSound;
				config->setConfigBool(GALACTICA_PREF_PLAY_SOUND, CSoundResourcePlayer::sPlaySound);
				config->setConfigBool(GALACTICA_PREF_PLAY_MUSIC, CMidiMovieFilePlayer::sPlayMusic);
		      #endif // GALACTICA_SERVER
			}
			::CloseResFile(fRefNum);
		}
	}
  #endif // PLATFORM_MACOS
  	// this section is prefs that are in old versions, so we only need to set them to defaults if
  	// there were no prefs at all.
    if (!hasPrefs) {
    	// no preferences, so set the defaults
		Galactica::Globals::GetInstance().setFullScreenMode(true);
      	config->setConfigBool(GALACTICA_PREF_PLAY_SOUND, true);
      	config->setConfigBool(GALACTICA_PREF_PLAY_MUSIC, true);
      	s = "";
      	config->setConfigString(GALACTICA_PREF_DEFAULT_NAME, s);
	} else {
		// preferences exist, so load them into the globals where needed
		config->getConfigString(GALACTICA_PREF_OLD_REGISTRATION, s);
		if (s.length() > 0) {
			// we have an old registration code saved in a current file, so they must
			// have declined to upgrade. Set the Old reg code and flag for the upgrade
			// window
			doReRegWindow = true;
			doRegWindow = false;
			c2pstrcpy(DisplayUnregisteredWindow::sOldRegistration, s.c_str());
		}
		bool b;
		config->getConfigBool(GALACTICA_PREF_FULL_SCREEN, b);
		Galactica::Globals::GetInstance().setFullScreenMode(b);
      #ifndef GALACTICA_SERVER
      	config->getConfigBool(GALACTICA_PREF_PLAY_SOUND, b);
		CSoundResourcePlayer::sPlaySound = b;
      	config->getConfigBool(GALACTICA_PREF_PLAY_MUSIC, b);
		CMidiMovieFilePlayer::sPlayMusic = b;
	  #endif // GALACTICA_SERVER      	
      	config->getConfigBool(GALACTICA_PREF_PLAY_MUSIC, b);
      	s = "";
		config->getConfigString(GALACTICA_PREF_DEFAULT_NAME, s);
		Galactica::Globals::GetInstance().setDefaultPlayerName(s.c_str());
		// now get the registration code, which is stored twice encoded
		// two different ways
      	s = "";
		config->getConfigString(GALACTICA_PREF_REGISTRATION, s);
		strncpy((char*)gBlob, s.c_str(), sizeof(RegCoreT));
      	s = "";
		config->getConfigString(GALACTICA_PREF_REG_BACKUP, s);
		strncpy((char*)gBlob2, s.c_str(), sizeof(RegCoreT));
	}
	// this section is prefs that were not in old prefs versions, so we need
	// to set them to defaults if we had no prefs or if we had old prefs
	if (!hasPrefs || convertedVersion2Prefs) {
      	config->setConfigBool(GALACTICA_PREF_SHOW_SPLASH, true);
      	config->setConfigBool(GALACTICA_PREF_SHOW_SHIPS, true);
      	config->setConfigBool(GALACTICA_PREF_SHOW_COURSES, true);
      	config->setConfigBool(GALACTICA_PREF_SHOW_NAMES, true);
      	config->setConfigBool(GALACTICA_PREF_SHOW_RANGES, true);
      	config->setConfigBool(GALACTICA_PREF_SHOW_COURIERS, true);
      	config->setConfigBool(GALACTICA_PREF_SHOW_GRIDLINES, true);
      	config->setConfigBool(GALACTICA_PREF_SHOW_NEBULA, true);
      	config->setConfigBool(GALACTICA_PREF_FAST_GAME, true);
      	config->setConfigBool(GALACTICA_PREF_FOG_OF_WAR, true);
	}

  // stuff not done for command line server
  #ifndef GALACTICA_SERVER
//	CBalloonApp::sMouseLingerEnabled = true;
  #endif // GALACTICA_SERVER
//		Galactica::Globals::GetInstance().sTurnsBetweenCallins = 5;
	Galactica::Globals::GetInstance().sPrefsLoaded = true;
	if (doRegWindow) {
		new DisplayRegistrationWindow();
	}
	if (doReRegWindow) {
		DisplayUnregisteredWindow::sOldVersionRegistered = true;
		new DisplayUnregisteredWindow();
	}
	if (!doRegWindow && !doReRegWindow) {
	    DisplayRegistrationWindow::sHoldStartup = false;
	}
	delete this;
}

DisplayRegistrationWindow::DisplayRegistrationWindow() {
	StartRepeating();
//	GalacticaApp::sRegistration[0] = 0;
	sInstalledRegistrationWind = true;
}
void	
DisplayRegistrationWindow::SpendTime(const EventRecord &) {
	GalacticaApp* app = (GalacticaApp*)LCommander::GetTopCommander();
	sHoldStartup = false;               // allow startup to happen when other conditions allow
  #ifndef GALACTICA_SERVER
	if (app->IsAboutBoxActive() == true) { // do nothing while the about box is active
	    return;
	}
	if (strlen(gBlob) == 0) {	// display the registration window
		ResIDT windID = window_Register - 345;	// don't let them search for the window ids
	  #ifdef GALACTICA_BOX_VERSION
	    windID += 5;
	  #endif // GALACTICA_BOX_VERSION
		sRegistrationWind = LWindow::CreateWindow(windID + 345, app);
		if (sRegistrationWind) {
			sWindBeingDisplayed = true;
			sRegistrationWind->UpdatePort();	// force immediate drawing
			CUpdateRegWin::sRegCodePane = sRegistrationWind->FindPaneByID(editText_RegCode);
			CUpdateRegWin::sNotYetButton = (LControl*)sRegistrationWind->FindPaneByID(button_NotYet);
			CUpdateRegWin::sRegisterButton = (LControl*)sRegistrationWind->FindPaneByID(button_Register);
			CUpdateRegWin::sEnterCodeButton = (LControl*)sRegistrationWind->FindPaneByID(button_EnterCode);
			CUpdateRegWin::sButtonHolder = (LView*)sRegistrationWind->FindPaneByID(view_ButtonHolder);

			if (CUpdateRegWin::sNotYetButton) {
				CUpdateRegWin::sNotYetButton->Disable();
			}
			if (CUpdateRegWin::sRegisterButton) {
				CUpdateRegWin::sRegisterButton->Enable();
			}
			if (CUpdateRegWin::sEnterCodeButton) {
				CUpdateRegWin::sEnterCodeButton->Disable();
				CUpdateRegWin::sEnterCodeButton->AddListener((LDialogBox*)sRegistrationWind);
			}
			CUpdateRegWin::sEnableTicks = ::TickCount() + REGISTRATION_NOT_YET_DELAY;
		}
		if (sWindBeingDisplayed) {
			new CUpdateRegWin;
			delete this;
		}
	} else {
		delete this;	// appears to be registered
		// convert the reg code from the blob representation to the actual in-memory version
		Str255 tmpStr;
		int pos = 6;
		int len = 0;
		for (int i = 0; i<(int)sizeof(RegCoreT); i++) {
			char c = gBlob[i];
			tmpStr[pos] = 'X';
			for (int j = 0; j < 10; j++) {
				if (gConfigCvt[j] == c) {
					tmpStr[pos++] = j + '0';
					len++;
					break; // stop the for loop
				}
			}
			if ((i % 4) == 3) {
				tmpStr[pos++] = '-';
				len++;
			}
		}
		char c2 = 'g';
		tmpStr[4] = 'N';
		tmpStr[5] = '-';
		tmpStr[2] = ('l'- 0x20);
		tmpStr[0] = len + 5;
		tmpStr[1] = (c2 - 0x20);
		c2 = 'e' + 1;
		tmpStr[3] = (c2 - 0x21);
		LString::CopyPStr(tmpStr, Galactica::Globals::GetInstance().sRegistration, sizeof(Str27));		
	}
  #else
    delete this; // on server we never show the registration window
  #endif // GALACTICA_SERVER
  #ifdef CHECK_REGISTRATION
	app->AddAttachment(new CCheckRegAttach);
  #endif
  #ifndef GALACTICA_SERVER
    if (!sWindBeingDisplayed) {
        app->StartUp();
    }
  #endif // !GALACTICA_SERVER
}

CUpdateRegWin::CUpdateRegWin() {
	StartRepeating();
	mCheckField = false;
}

void
CUpdateRegWin::SpendTime(const EventRecord &inMacEvent) {
#ifndef GALACTICA_SERVER
	if (!DisplayRegistrationWindow::sWindBeingDisplayed) {
		delete this;	// get rid of this key handler once the window is gone
	} else {
		if (mCheckField) {
			if (sRegCodePane) {
				Str255 s;
				sRegCodePane->GetDescriptor(s);
				if (s[0] > 0) {	// check the reg code entry to see if it has anything in it
					((LDialogBox*)DisplayRegistrationWindow::sRegistrationWind)->SetDefaultButton(button_EnterCode);	// it does, default button is Enter Code
					sEnterCodeButton->Enable();
				} else {
					((LDialogBox*)DisplayRegistrationWindow::sRegistrationWind)->SetDefaultButton(button_Register);	// default button is Register 
					sEnterCodeButton->Disable();	
				}
//				if (sButtonHolder) {
//					sButtonHolder->Refresh();
//					sButtonHolder->UpdatePort();
//				}
			}
			mCheckField = false;
		}
		if ((inMacEvent.what == keyDown) || (inMacEvent.what == autoKey)) {
			mCheckField = true;
		}
		if ( sEnableTicks && (::TickCount() > sEnableTicks)) {
			if (CUpdateRegWin::sNotYetButton) {
				CUpdateRegWin::sNotYetButton->Enable();
			}
			sEnableTicks = 0;
		}
	}
#else
    delete this;
#endif // GALACTICA_SERVER
}

#ifdef CHECK_REGISTRATION

// Registration code is in format:
//
// GLEN-5413-0979-3226-6323-20
// ppll-cc##-####-####-####-vv
//
// 27 chars long:
// pp = 2 chars representing product, GL for Galactica
// ll = 2 chars representing language, capitalized ISO language codes 
//               (EN = english, DE = german, etc..) 
// -  = 1 char dash separator
// cc = 2 chars giving checksum
// ##-####-####-####- = 18 chars gives reg number (with separator dashes)
// vv = 2 chars giving major and minor version (20 is version 2.0, 12 is version 1.2)

// product, and major version must match
// language must match hi two bytes of owner resource unless that is owner resource has **
// checksum is sum of reg number digits, skipping dashes, then minus flipped version number

CCheckRegAttach::CCheckRegAttach()
: LAttachment(msg_CommandStatus, true) {
}

void 
CCheckRegAttach::ExecuteSelf(MessageT, void *) {
	sRegistered = true;		// registration fails if:
	RegCoreT core;
	char* p = reinterpret_cast<char*>(Galactica::Globals::GetInstance().sRegistration);
	if (p[0] == 0) {	// no registration code entered
		sRegistered = false;
	} else {
		char lc = sVersionCode >> 24;
		if (lc != '*') {
			if (p[3] != lc) { // hi 2 bytes of owner resource != 
				sRegistered = false;				  // chars 3 & 4 of registration code
			}
		}										// (basically a check to make sure the language
		lc = (sVersionCode >> 16) & 0xff;		// version matches registration code so someone
		if (lc != '*') {						// can't use english vers reg code in german vers)
			if (p[4] != lc) {
				sRegistered = false;
			}	
		}
		char vers = (sVersionCode >> 8) & 0xff;
		if (p[26] != vers) {	// code from different major version release
			sRegistered = false;
		}
		if (p[1] != 'G') {	// code is from different application
			sRegistered = false;
		}
		if (p[2] != 'L') {
			sRegistered = false;
		}
		int checksum = 0;		// compute checksum for this code
		int i = 23;
		char* p2 = &core[15]; // start at the end of the string and work backwards
		char* p3 = &gBlob[15];
		char* p4 = &gBlob2[15];
		while (i>6) {
			if (((i-5)%5) == 4) {	// don't count dashes toward checksum
			   if (p[i+1] != '-') {
			      sRegistered = false; // make sure the dashes are really dashes
			      break;
			   }
				i--;
			}
			int ri = p[i+1] -'0';
			*p2-- = sConverter[ri];
			*p3-- = gConfigCvt[ri];
			*p4-- = gConfigCv2[ri];
			checksum += ri;		// adjust checksum
			i--;
		}
		checksum -= (p[26] - '0');	// include version number in checksum
		checksum -= ((p[27] - '0') * 10);
		
		core[16] = sConverter[(p[26] - '0')];
		gBlob[16] = gConfigCvt[(p[26] - '0')];
		gBlob2[16] = gConfigCv2[(p[26] - '0')];
		
		core[17] = sConverter[(p[27] - '0')];
		gBlob[17] = gConfigCvt[(p[27] - '0')];
		gBlob2[17] = gConfigCv2[(p[27] - '0')];
		
		core[0] = sConverter[(p[6] - '0')];
		gBlob[0] = gConfigCvt[(p[6] - '0')];
		gBlob2[0] = gConfigCv2[(p[6] - '0')];
		
		core[1] = sConverter[(p[7] - '0')];
		gBlob[1] = gConfigCvt[(p[7] - '0')];
		gBlob2[1] = gConfigCv2[(p[7] - '0')];
		
		if (p[7] != ((checksum % 10) + '0')) {	// fail if checksum incorrect
			sRegistered = false;
		}
		if (p[6] != (((checksum / 10) % 10) + '0')) {
			sRegistered = false;
		}
		// new in version 2.0, check for old registration code
		if (sRegistered && (p[26] < '3') ) {
			DisplayUnregisteredWindow::sOldVersionRegistered = true;
			sRegistered = false;
		}
		if (sRegistered) {
		    // check to see if the core of the registration code matches any denied codes
		    int i = 0;
		    int matches = 43;
		    while (sinvlist[i][0]) {
		        if (std::strncmp(core, sinvlist[i], 18) == 0) {
		            // matches a denied code, block registration
		            sRegistered = false;
		            --matches;
		        }
		        i++;
		    }
		    if ((matches+1) != 44) {
		        sRegistered = false;
		    }
		}
		if (sRegistered == false) {
			gBlob[0] = 0;
			gBlob2[0] = 0;
			p[0] = 0;
			new DisplayRegistrationWindow();	// make the registration window display again
		}
	}
	Galactica::Globals::GetInstance().sRegistered = sRegistered;
	delete this;	// don't need to keep checking, we've decided one way or another
  #ifdef CHECK_FOR_HACKING
	LCommander* app = LCommander::GetTopCommander();
	app->AddAttachment(new CDoShutdownAttach);
  #endif
}


CCheckTimeUpAttach::CCheckTimeUpAttach(GalacticaShared *inGame)
: LAttachment(msg_TurnEnded, true) {
	mGame = inGame;
	sTurnToExpireOn = 87;	// a convoluted way to check for turn 100
}


void
CCheckTimeUpAttach::ExecuteSelf(MessageT inMessage,void *ioParam) {
#ifdef __MWERKS__
#pragma unused(inMessage, ioParam)
#endif
	if (CCheckRegAttach::sRegistered) {
		delete this;
	} else {
		bool bExpired = false;
		if ((mGame->GetTurnNum() - 13) >= sTurnToExpireOn) {
			bExpired = true;
		}
		if (!DisplayUnregisteredWindow::sInstalledUnregisteredWind && bExpired) {
			new DisplayUnregisteredWindow;
		} else if (DisplayUnregisteredWindow::sInstalledUnregisteredWind) {
			bExpired = true;
		}
		SetExecuteHost(!bExpired);
	}
}

DisplayUnregisteredWindow::DisplayUnregisteredWindow() {
	StartRepeating();
	sInstalledUnregisteredWind = true;
}
void	
DisplayUnregisteredWindow::SpendTime(const EventRecord &) {
	StopRepeating();
  #ifndef GALACTICA_SERVER
	LCommander* app = LCommander::GetTopCommander();
	ResIDT windID = window_Unregistered - 127;	// don't let them search for the window ids
	CDialogAttachment* dlgAttach = 0;
	if (sOldVersionRegistered) {
		LStr255 s = DisplayUnregisteredWindow::sOldRegistration;
		MovableParamText(0, s);
		windID = window_ReRegisterOld - 127;
		dlgAttach = new ReRegDialogAttach(s);
	}
	bool oldDoStartup = gDoStartup;	// save state of do startup to temporarily prevent opening
	gDoStartup = false;				// the about box and doing new game
	long response = MovableAlert(windID + 127, app, 0, false, dlgAttach);
	gDoStartup = oldDoStartup;		// restore old state of do startup
	delete this;
	sInstalledUnregisteredWind = false;
	if ((response != cmd_SharewareInfo) && gDoStartup) {
	    DisplayRegistrationWindow::sHoldStartup = false;
		static_cast<GalacticaApp*>(app)->StartUp();	// finish delayed startup event
	}
  #else
//    DEBUG_OUT("Unregistered Copy. Aborting.", DEBUG_ERROR);
    exit(1);    // exit with an error
  #endif // GALACTICA_SERVER
}

#ifdef CHECK_FOR_HACKING
CDoShutdownAttach::CDoShutdownAttach()
: LAttachment(msg_CommandStatus, true) {
	sInstalledShutdown = true;
	sCopyRegistered = CCheckRegAttach::sRegistered;	// this has already been set to false
}
void 
CDoShutdownAttach::ExecuteSelf(MessageT, void *) {
	Boolean hacked = false;
	// v1.2fc4, this won't be true for the first time they ever launch
	if (!DisplayRegistrationWindow::sInstalledRegistrationWind) {
		hacked = true;
	}
	if (!CLoadPrefsAttach::sPrefsAttach()) {
		hacked = true;
	}
	if (CCheckRegAttach::sRegistered != sCopyRegistered) {
		hacked = true;
	}
	if (hacked) {
		// еее someone tried to hack it
//		DEBUG_OUT("Hack Attempt Detected", DEBUG_ERROR | DEBUG_USER);
		::ExitToShell();
	}
	if (CCheckRegAttach::sRegistered) {
		delete this;
	}
}
#endif // CHECK_FOR_HACKING
#endif // CHECK_REGISTRATION


#if COMPILER_MWERKS
//#pragma sym reset
#if TARGET_CPU_PPC
#pragma traceback reset
#elif TARGET_CPU_68K
#pragma macsbug reset
#endif
#endif // COMPILER_MWERKS

