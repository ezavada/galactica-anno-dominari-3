//	Galactica.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Changes:
//	9/21/96, ERZ, 1.0.8 changes integrated
//	1/4/97, ERZ, 1.1 changes integrated

#include "Galactica.h"
#include "GalacticaGlobals.h"
#include "GalacticaPackets.h"
#include "AGalacticThingy.h"

#include "CMessage.h"
#include "CShip.h"
#include "CStar.h"
#include "CFleet.h"
#include "CRendezvous.h"
#include "ProductInformation.h"
#include "GalacticaRegistration.h"
#include "GalacticaHost.h"

#include <UMemoryMgr.h>
#include <UEnvironment.h>
#include <LFileStream.h>

#include "pdg/sys/config.h"
#include "pdg/sys/os.h"

#if !defined(POSIX_BUILD) && ( defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ) )
#include <UDrawingState.h>  // for UQDGlobals::SetRandomSeed()
#include <Folders.h>        // for FindFolder()
#endif // PLATFORM_MACOS

// ========= GUI ONLY =========
#ifndef GALACTICA_SERVER

#include "GalacticaConsts.h"
#include "GalacticaPanels.h"
#include "CGalacticSlider.h"
#include "GalacticaTables.h"
#include "GalacticaClient.h"
#include "GalacticaHostDoc.h"
#include "GalacticaSingleDoc.h"
#include "CGameSetupAttach.h"

#include "LPPobView.h"
#include "CDividedView.h"

#ifdef MONKEY_BYTE_VERSION
#include "OrderFormAttach.h"
#endif

#include <UStandardDialogs.h>
#include <LGrowZone.h>
#include <LWindow.h>
#include <PP_Messages.h>
#include <PP_Resources.h>
#include <UMemoryMgr.h>
#include <URegistrar.h>
#include <LEditField.h>
#include <LActiveScroller.h>
#include <LStdControl.h>
#include <LTextButton.h>
#include <LToggleButton.h>
#include <LPicture.h>
#include <LRadioGroup.h>
#include <LCicnButton.h>
#include <LIconPane.h>
#include <LGroupBox.h>
#include <UModalDialogs.h>
#include <LDialogBox.h>
#include <LTabGroup.h>
#include <UAttachments.h>
#include <LMenuBar.h>
#include <UDesktop.h>
#include <UScreenPort.h>

#include <LBevelButton.h>
#include <LAMBevelButtonImp.h>
#include <LGABevelButtonImp.h>

#include "GalacticaTutorial.h"

#include "Slider.h"
#include "CResCaption.h"
#include "CTextTable.h"
#include "CVarDataFile.h"
#include "CMasterIndexFile.h"
#include "CStyledText.h"
#include "CWindowMenu.h"
#include "USound.h"
#include "LAnimateCursor.h"
#include "CHelpAttach.h"
#include "CScrollingPict.h"
#include "CKeyCmdAttach.h"
#include "CLayeredOffscreenView.h"
#include "LThermometerPane.h"
#include "CScrollWheelHandler.h"

#ifdef DEBUG
  #include <string.h>
  #include <TextUtils.h>
  extern int		gDebugBreakOnErrors;	// in DebugMacros.cp
#endif

#if TARGET_OS_MAC	
  #include "InternetInfo.h"
#endif

#endif // GALACTICA_SERVER
// ========= end GUI ONLY =========


#ifndef NO_GAME_SMITH_SUPPORT
    #include "GameSmithAPI.h"
#endif // ! NO_GAME_SMITH_SUPPORT

#ifndef NO_GAME_RANGER_SUPPORT
  #include "OpenPlay.h"
  #define __NETSPROCKET__ // so GameRanger.h doesn't include it
  #include "GameRanger.h"
#endif // not defined NO_GAME_RANGER_SUPPORT

#if ( defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX )) && !defined( POSIX_BUILD ) && !GALACTICA_SERVER	
  #include "LangModule.h"
  #if !TARGET_API_MAC_CARBON
    #include "CDisplay.h"
  #endif
#endif

#include <ctime>

#ifndef COMPILER_MSVC
   #if COMPILER_GCC
      #define strnicmp strncasecmp
      #define stricmp  strcasecmp
   #else
     // externs for non-standard C library calls
     extern int strnicmp(const char*, const char*, int);
     extern int stricmp(const char*, const char*);
   #endif
#endif // !COMPILER_MSVC

LAnimateCursor*	        gTargetCursor;
UInt32					gFGNextEventCheck = 0;
UInt32					gFGBusyWorkTicks = 30;		// only check for user event 2x per sec when
bool					gHasNewGameWind;
bool					gDoStartup;
bool					gUseDebugMenu = false;
short					gSoundResFileRefNum = -1;
short					gArtResFileRefNum = -1;
short					gStartupLanguageCode = -1;

UInt32					gAGalacticThingyClassNum = 0;
UInt32                  gAGalacticThingyUIClassNum = 0;
UInt32					gANamedThingyClassNum = 0;
UInt32					gCShipClassNum = 0;
UInt32					gCStarClassNum = 0;
UInt32					gCMessageClassNum = 0;
UInt32					gCDestructionMsgClassNum = 0;
UInt32					gCStarLaneClassNum = 0;
UInt32					gCFleetClassNum = 0;
UInt32					gCWormholeClassNum = 0;
UInt32					gCRendezvousClassNum = 0;
UInt32					gTopMem = 0x00000;
std::string				gVersionString;

bool                    GalacticaApp::sGalacticaAppDestroyed = false; // 2.1b9r5, so we can handle OpenPlay messages during app shutdown
#ifndef GALACTICA_SERVER
CMidiMovieFilePlayer*   GalacticaApp::sMusicMov;
CNewGWorld*		        GalacticaApp::sStarGrid;	// Added 970630 JRW  Holds the grid of stars
bool                    GalacticaApp::sIsRightClick;
#endif // GALACTICA_SERVER

#ifdef GALACTICA_SERVER
// globals used by Galactica Standalone server build
int gNumPlayers;
int gNumAIs;
int gPortNum;
int gSkillLevel;
int gTurnTime; // minutes
int gDensity;
int gMapSize;
bool gCreateNewGame;
bool gLoadGame;
bool gFoundParam;
bool gFogOfWar;
bool gFastGame;
bool gEndTurnEarly;
std::string gGameName;
#endif // GALACTICA_SERVER

#ifdef COMPILER_GCC
template <> Galactica::Globals  Galactica::Singleton<Galactica::Globals>::sInstance = Galactica::Globals();
#else
Galactica::Globals      Galactica::Singleton<Galactica::Globals>::sInstance;
#endif

#ifdef CHECK_REGISTRATION
// these are defined in GalacticaRegistration.cpp
extern RegCoreT gBlob;
extern RegCoreT gBlob2;
extern char gConfigCvt[];  // conversion string for codes stored in the config
extern char gConfigCv2[];  // conversion string for backup codes stored in the config
#endif // CHECK_REGISTRATION


#define DELAY(ticks) {register unsigned long z_NextTicks = ::TickCount() + ticks;	\
					  register unsigned long z_CurrTicks; 							\
					  do { 															\
					  	z_CurrTicks = ::TickCount();								\
					  } while (z_CurrTicks < z_NextTicks); }


// -----------------------------------------------------------------------

#if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))

void LaunchRegisterApp();

#if TARGET_API_MAC_CARBON
const MacAPI::EventTypeSpec	kCarbonEventsWeWant[] = {
    { MacAPI::kEventClassMouse, MacAPI::kEventMouseDown },
    { MacAPI::kEventClassMouse, MacAPI::kEventMouseUp }
};
pascal MacAPI::OSStatus    CarbonEventHandler(MacAPI::EventHandlerCallRef handler, MacAPI::EventRef event, void* userData);
#endif // TARGET_API_MAC_CARBON

#if !TARGET_API_MAC_CARBON
pascal void GalacticaLanguageRedraw(void);

pascal void 
GalacticaLanguageRedraw(void) {
	LMd_StandardRedraw();
	LCommander* theTarget = LCommander::GetTarget();
	if (theTarget) {
		theTarget->ObeyCommand(cmd_SwitchLanguages, nil);
	}
}
#endif //!TARGET_API_MAC_CARBON


#endif // TARGET_OS_MAC	

#ifdef __MC68K__
#pragma code68020 off
#endif

#ifdef DEBUG
  #if PLATFORM_MACOS && COMPILER_MWERKS
    #include "MetroNubUtils.h"
  #endif
  extern int gDebugBreakOnErrors;
#endif

// for a test of the Spotlight debugger
#ifdef USE_SL_STUFF
	#include "SpotlightAPI.h"
/*	extern pascal OSErr __initialize(const CFragInitBlock *theInitBlock);
	void SpotlightInitialize(CFragInitBlock *theInitBlock) {
		SLInit();
		__initialize(theInitBlock);
	}*/
	pascal void __start(void);
	pascal void __SLStart(void);
	pascal void __SLStart(void) {
		SLInit();
		__start();
	}
#endif

#ifdef GALACTICA_SERVER
int main(int argc, const char** argv)
#else
int main(void)
#endif
{

	gDoStartup = true;  // assume from the beginning that we will display the startup screen


	// ---------------------------
	// Set Debugging options
	// ---------------------------

	DEBUG_INITIALIZE();

#ifndef GALACTICA_SERVER
    #ifdef DEBUG

      #ifndef COMPILER_MWERKS
        #define AmIBeingMWDebugged() false
      #endif
      
      #if TARGET_OS_MAC	
    	if (AmIBeingMWDebugged() == true) {
    		SetDebugThrow_(debugAction_Nothing);//debugAction_LowLevelDebugger);
    		SetDebugSignal_(debugAction_Nothing);//debugAction_LowLevelDebugger);
    		DEBUG_SET_LEVEL(DEBUG_SHOW_ALL);
    //		DEBUG_ENABLE_ERROR(DEBUG_TUTORIAL);
    		gDebugBreakOnErrors = true;			// metronub should catch debugger traps
    		gUseDebugMenu = true;
    	} else {
    		SetDebugSignal_(debugAction_Nothing);
    		SetDebugThrow_(debugAction_Nothing);
    		DEBUG_SET_LEVEL(DEBUG_SHOW_ERRORS);
    		DEBUG_SET_LEVEL(DEBUG_SHOW_MOST);
    		DEBUG_ENABLE_ERROR(DEBUG_DATABASE); // get max info about database
    //		DEBUG_ENABLE_ERROR(DEBUG_PACKETS);
    		gDebugBreakOnErrors = false;
    		gUseDebugMenu = true;
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
    	DEBUG_ENABLE_ERROR(DEBUG_DATABASE);
    #endif

#else  // Server version
	SetDebugSignal_(debugAction_Nothing);
	SetDebugThrow_(debugAction_Nothing);
    #ifdef DEBUG
	    DEBUG_SET_LEVEL(DEBUG_SHOW_ALL);    // for debug server builds show everything
	#else
	    DEBUG_SET_LEVEL(DEBUG_SHOW_MOST);
	#endif
  #ifdef DEBUG
	gDebugBreakOnErrors = false;
	gUseDebugMenu = false;
  #endif
//	DEBUG_ENABLE_ERROR(DEBUG_DATABASE);
#endif // GALACTICA_SERVER

	// ---------------------------
	// Win32 Initialization
	// ---------------------------
  #if TARGET_OS_WIN32
	// do windows specific initialization
  #endif


	// ---------------------------
	// MacOS Initialization
	// ---------------------------
  #if TARGET_OS_MAC
//	InitializeHeap(14);	// Init Memory Manager: Param is num Master Ptr blocks to allocate
   #if !TARGET_API_MAC_CARBON
	UQDGlobals::InitializeToolbox(&qd);
   #endif // ! TARGET_API_MAC_CARBON
  #endif // TARGET_OS_MAC

  UEnvironment::InitEnvironment();

	// ---------------------------
	// Check for 68000 Processor
	// ---------------------------
#ifdef PLATFORM_68K
	// check to make sure we have 020 or better processor
	long processorType;
	Boolean abort = false;
	if (MacAPI::Gestalt(gestaltProcessorType, &processorType) ) {
		abort = true;
	} else if (processorType < gestalt68020) {
		abort = true;
	}
	if (abort) {
		MacAPI::Alert(alert_DeathBy68000, nil);
		MacAPI::ExitToShell();
	}
#endif

	// ---------------------------
	// General Initialization
	// ---------------------------

	// get the current application version string for later use
	GetVersionNumberString(kFileVersionID, gVersionString);

#ifdef DEBUG
	std::time_t lclTime;
	struct std::tm *now;
	char tempstr[256];
	char debugoutfilename[256];
	char debugoutinfostr[256];
  #if PLATFORM_68K
  	char *tsp = "68k";
  #elif PLATFORM_X86
  	char *tsp = "x86";
  #elif PLATFORM_POSIX
   char *tsp = PLATFORM_STR;
  #elif TARGET_API_MAC_CARBON
    char *tsp;
    if (UEnvironment::GetOSVersion() >= 0x1000) {
        tsp = "OSX";
    } else {
        tsp = "OS9";
    }
  #else
  	char *tsp = "PPC";
  #endif
  	std::sprintf(tempstr, "Galactica-%s_%%y%%m%%d_%%H%%M.out", tsp);
	lclTime = std::time(NULL);
	now = std::localtime(&lclTime);
	std::strftime(debugoutfilename, 256, tempstr, now);
	DEBUG_SET_FILENAME(debugoutfilename);

  	std::sprintf(tempstr, "Galactica/%s v%s %%Y/%%m/%%d %%H:%%M:%%S%c", tsp, gVersionString.c_str(), DEBUG_EOL);
	std::strftime(debugoutinfostr, 256, tempstr, now);
	DEBUG_SET_INFO_STR(debugoutinfostr);
#endif

    #if PLATFORM_BIG_ENDIAN
      #define ENDIANNESS "big endian"
    #else
      #define ENDIANNESS "little endian"
    #endif
    DEBUG_OUT("platform: " PLATFORM_STR " - " ENDIANNESS, DEBUG_IMPORTANT);
    DEBUG_OUT("compiler: " COMPILER_STR, DEBUG_IMPORTANT);

#if ( defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX )) && !defined( POSIX_BUILD )
	// Setup the application to run in the appropriate language based on what is found in
	// the preferences file
	std::string s;
	LoadStringResource(s, STR_PrefsFileName);
	FSSpec prefs;
	Galactica::Globals::GetInstance().sSysVRef = kOnSystemDisk;	// get the ref num for system pref folder
	Galactica::Globals::GetInstance().sPrfsFldrID = 0;
	MacAPI::FindFolder(kOnSystemDisk, kPreferencesFolderType, true, 
	                &Galactica::Globals::GetInstance().sSysVRef, 
	                &Galactica::Globals::GetInstance().sPrfsFldrID);
	Str255 tmpstr;
	c2pstrcpy(tmpstr, s.c_str());
	MacAPI::FSMakeFSSpec(Galactica::Globals::GetInstance().sSysVRef, Galactica::Globals::GetInstance().sPrfsFldrID, tmpstr, &prefs);
	short fRefNum = ::FSpOpenResFile(&prefs, fsRdWrShPerm);	// open prefs file
	if (fRefNum != -1) {
		// don't check legacy pref file's language code on Intel Macs
	  #if !defined( PLATFORM_X86 )
		Handle langCodeHnd = MacAPI::GetResource('pref', 2);
		if (langCodeHnd) {
			gStartupLanguageCode = **((short**)langCodeHnd);
			MacAPI::ReleaseResource(langCodeHnd);
		}
	  #endif // !PLATFORM_X86
		// check for DBUG resource #1 and enable debug menu if present
		Handle junkHandle = MacAPI::GetResource('DBUG', 1);
		if (junkHandle) {
			gUseDebugMenu = true;
			MacAPI::ReleaseResource(junkHandle);
		}
		MacAPI::CloseResFile(fRefNum);
	}

  #if !GALACTICA_SERVER
	LMd_SetLangMenuID(1);							// Galactica uses MENU 1 for languages
	LMdInitialize(gStartupLanguageCode);			// use system default
//    #if !PLATFORM_MAC_CARBON
//      LMdInstallRedrawProc(GalacticaLanguageRedraw);	// use a custom redraw proc
//    #endif
  #endif // ! GALACTICA_SERVER
#endif // PLATFORM_MACOS
  
    std::time_t seed;	// initialize random number generator
	std::time(&seed);
 	std::srand(seed);

#ifndef GALACTICA_SERVER
  #if TARGET_API_MAC_CARBON
    // under OS X, fonts aren't read from the resource chain anymore
    if (UEnvironment::IsRunningOSX()) {
		LStr255 s((short)STR_GalacticaFontFileName, (short)0);
		FSSpec fileSpec;
		if (MacAPI::FSMakeFSSpec(0,0, s, &fileSpec) == noErr) {
            MacAPI::FMActivateFonts(&fileSpec, NULL, NULL, 0); //kFMLocalActivationContext
        }
    }
    MacAPI::InstallApplicationEventHandler(MacAPI::NewEventHandlerUPP(CarbonEventHandler), 
                                            MacAPI GetEventTypeCount(kCarbonEventsWeWant), 
                                            kCarbonEventsWeWant, 0, NULL);
  #endif
   LStr255 fontNameStr((short)STR_GalacticaFontName, (short)0);
   if (fontNameStr.Length() == 0) {
      fontNameStr = "\pGalactica";
   }
	MacAPI::GetFNum(fontNameStr, &gGalacticaFontNum);	// find font num of Galactica font
	if (gGalacticaFontNum == 0) {
	  #if TARGET_API_MAC_CARBON
	    MacAPI::GetFNum("\pArial Narrow", &gGalacticaFontNum);	// find font num of Galactica font
	  #else
		gGalacticaFontNum = kFontIDHelvetica;	// use helvetica if Galactica font not in system
      #endif
	}
    DEBUG_OUT("Gestalt OS version [" << (void*) UEnvironment::GetOSVersion() << "]", DEBUG_IMPORTANT);
	DEBUG_OUT("Galactica using font: " << gGalacticaFontNum, DEBUG_IMPORTANT);

  #if BALLOON_SUPPORT
	if (UEnvironment::HasGestaltAttribute(gestaltHelpMgrAttr, gestaltHelpMgrPresent)) {
		gHaveBalloonHelp = true;
		MacAPI::HMGetFont(&gBalloonHelpFont);
		MacAPI::HMGetFontSize(&gBalloonHelpFontSize);
		gBalloonHelpOnInSystem = ::HMGetBalloons();	// remember balloon help state
		gBalloonHelpOnInGalactica = false;
		MacAPI::HMSetFont(gGalacticaFontNum);
		MacAPI::HMSetFontSize(12);
	}
  #endif //BALLOON_SUPPORT
#endif // GALACTICA_SERVER

	bool readHostConfig = true;

#ifdef GALACTICA_SERVER
// look for key command line parameters
	int numPlayers = 0;
	int numAIs = 0;
	int portNum = 0;
	int skillLevel = 0;
	int turnTime = 0; // minutes
	int density = 0;
	int mapSize = 0;
	bool createNewGame = false;
	bool loadGame = false;
	bool foundParam = false;
	bool endTurnEarly = false;
	bool fastGame = true;
	bool fogOfWar = true;
	std::string gameName;
    for (int i = 1; i<argc; i++) {
        DEBUG_ONLY( pdg::OS::_DOUT("Params: parsing param [%d] [%s]", i, argv[i]); )
        if (std::strncmp(argv[i], "--help", 6) == 0) {
        	using std::printf;
			printf("galactica-serv\n");
			printf("\n");
			printf("Command Line Server for Galactica: Anno Dominari v3.0\n");
			printf("\n");
			printf("Usage:\n");
			printf("-port=n             host game on a particular port\n");
			printf("-newgame=name       create a new game entitled 'name'\n");
			printf("-loadgame=name      load an existing game entitled 'name'\n");
			printf("-ignorehostconfig   don't read the hostconfig.txt file\n");
			printf("new game options:\n");
			printf("-numplayers=n   how many players are in a new game\n");
			printf("-numais=n       how many of the players are AIs\n");
			printf("-skill=n       	skill level of AIs (0..4, 0-novice, 4-expert)\n");
			printf("-density=n      density of the star map, 0-lowest, 9-highest\n");
			printf("-size=n       	size of star map, n x n, 1...5\n");
			printf("-turntime=n     minutes per turn\n");
			printf("-slowgame       don't use fast game option\n");
			printf("-nofog     		don't use fog of war\n");
			printf("-endturnearly   allow turn to end early if all players have posted");
			return 0;
        }
        if (std::strncmp(argv[i], "-slowgame", 9) == 0) {
            foundParam = true;
            // they are telling us not to use the fast game option
            fastGame = false;
			DEBUG_ONLY( pdg::OS::_DOUT("Server: will not do fast game as specified by -slowgame"); )
        }
        if (std::strncmp(argv[i], "-nofog", 6) == 0) {
            foundParam = true;
            // they are telling us not to use the fog of war option
            fogOfWar = false;
			DEBUG_ONLY( pdg::OS::_DOUT("Server: will not do fog of war game as specified by -nofog"); )
        }
        if (std::strncmp(argv[i], "-endturnearly", 13) == 0) {
            foundParam = true;
            // they are telling us to end turn early if everyone posts
            endTurnEarly = true;
			DEBUG_ONLY( pdg::OS::_DOUT("Server: will end turn early if everyone posts as specified by -endturnearly"); )
        }
        if (std::strncmp(argv[i], "-port=", 6) == 0) {
            foundParam = true;
            // they are giving us a port number on the command line
            portNum = std::atoi(&argv[i][6]);
            DEBUG_ONLY( pdg::OS::_DOUT("Server: using port number specified by -port=%d", portNum); )
        }
        if (std::strncmp(argv[i], "-numplayers=", 12) == 0) {
            foundParam = true;
            // they are giving us the total player count on the command line
            numPlayers = std::atoi(&argv[i][12]);
            if ( (numPlayers > MAX_PLAYERS_CONNECTED) || (numPlayers < 1) ) {
                DEBUG_ONLY( pdg::OS::_DOUT("Server: Illegal number of players specified by -numplayers=%d -- SHUTTING DOWN", numPlayers); )
                return -1;
            } else {
                DEBUG_ONLY( pdg::OS::_DOUT("Server: using number of players specified by -numplayers=%d", numPlayers); )
            }
        }
        if (std::strncmp(argv[i], "-numais=", 8) == 0) {
            foundParam = true;
            // they are giving us the player count on the command line
            numAIs = std::atoi(&argv[i][8]);
            if ( (numAIs > MAX_PLAYERS_CONNECTED) || (numAIs < 0) ) {
                DEBUG_ONLY( pdg::OS::_DOUT("Server: Illegal number of ai players specified by -numais=%d -- SHUTTING DOWN", numAIs); )
                return -1;
            } else {
                DEBUG_ONLY( pdg::OS::_DOUT("Server: using number of ai players specified by -numais=%d", numAIs); )
            }
        }
		if (std::strncmp(argv[i], "-skill=", 7) == 0) {
            foundParam = true;
			// they are giving us AI skill level
            // format is -skill=n, n is 0...4, 0 - novice, 4 - expert
 			skillLevel = argv[i][7] - '0';
			if ((skillLevel < 0) || (skillLevel > 4)) {
				DEBUG_ONLY( pdg::OS::_DOUT("Server: Illegal skill level specified by %s -- SHUTTING_DOWN", argv[i]); )
                return -1;
			} else {
                DEBUG_ONLY( pdg::OS::_DOUT("Server: set ai skill level as specified by -skill=%d", skillLevel); )
			}
		}
		if (std::strncmp(argv[i], "-density=", 9) == 0) {
            foundParam = true;
			// they are giving us map density
            // format is -density=n, n is 0...9, 0 - least dense, 9 - most dense
 			density = argv[i][9] - '0';
			if ((density < 0) || (density > 9)) {
				DEBUG_ONLY( pdg::OS::_DOUT("Server: Illegal map density specified by %s -- SHUTTING_DOWN", argv[i]); )
                return -1;
			} else {
                DEBUG_ONLY( pdg::OS::_DOUT("Server: set map density as specified by -density=%d", density); )
			}
		}
		if (std::strncmp(argv[i], "-size=", 9) == 0) {
            foundParam = true;
			// they are giving us map size
            // format is -size=n, n x n map, n is 1...5
 			mapSize = argv[i][9] - '0';
			if ((density < 1) || (density > 5)) {
				DEBUG_ONLY( pdg::OS::_DOUT("Server: Illegal map size specified by %s -- SHUTTING_DOWN", argv[i]); )
                return -1;
			} else {
                DEBUG_ONLY( pdg::OS::_DOUT("Server: set map size as specified by -size=%d", mapSize); )
			}
		}
        if (std::strncmp(argv[i], "-turntime=", 10) == 0) {
            foundParam = true;
            // they are giving us the turn time limit on the command line
            turnTime = std::atoi(&argv[i][10]);
            if ( turnTime < 1 ) {
                turnTime = 5;
            }
			DEBUG_ONLY( pdg::OS::_DOUT("Server: using turn time limit of %d minutes as specified by -turntime=%d", turnTime, turnTime); )
        }
        if (std::strncmp(argv[i], "-ignorehostconfig", 17) == 0) {
            foundParam = true;
            // they are telling us to ignore the hostconfig file
            readHostConfig = false;
			DEBUG_ONLY( pdg::OS::_DOUT("Server: will not read hostconfig.txt as specified by -ignorehostconfig"); )
        }
        if (std::strncmp(argv[i], "-newgame=", 9) == 0) {
            foundParam = true;
            // they are telling us to create a new game
            createNewGame = true;
            loadGame = false;
            const char* p = &argv[i][9];
            gameName.assign(p);
			DEBUG_ONLY( pdg::OS::_DOUT("Server: will create a new game as specified by -newgame=%s", gameName.c_str()); )
        }
        if (std::strncmp(argv[i], "-loadgame=", 10) == 0) {
            foundParam = true;
            // they are telling us to create a new game
            createNewGame = false;
            loadGame = true;
            const char* p = &argv[i][10];
            gameName.assign(p);
			DEBUG_ONLY( pdg::OS::_DOUT("Server: will load an existing game as specified by -loadgame=%s", gameName.c_str()); )
        }
    }
    gNumPlayers = numPlayers;
    gNumAIs = numAIs;
	gPortNum = portNum;
	gSkillLevel = skillLevel;
	gTurnTime = turnTime; // minutes
	gDensity = density;
	gMapSize = mapSize;
	gCreateNewGame = createNewGame;
	gLoadGame = loadGame;
	gFoundParam = foundParam;
	gGameName = gameName;
	gEndTurnEarly = endTurnEarly;
	gFogOfWar = fogOfWar;
	gFastGame = fastGame;
#endif

	if (readHostConfig) {
		GalacticaApp::ReadHostConfigFile();
	}

   // create, intialize and startup the application object
   GalacticaApp* theApp = NULL;
   try {
    	theApp = new GalacticaApp();
      #ifdef DEBUG
       #ifdef CHECK_FOR_MEMORY_LEAKS
         DebugMemTurnOptionsOn(dgbmemOptDontFreeBlocks);
         DebugMemForgetLeaks();
    	   DebugMemReadLeaks();
       #endif
      #endif
      theApp->Run();
    }
	catch (std::exception &e) {
	   DEBUG_OUT("main() caught exception "<< e.what() <<" during init or GalacticaApp::Run()", DEBUG_ERROR);
	}
	catch(...) {
	   DEBUG_OUT("main() caught unknown exception during init or GalacticaApp::Run()", DEBUG_ERROR);
	}

    // final reports
  #ifdef DEBUG
   ReportPacketStatistics();
   ThingyRef::ReportThingyRefLookupStatistics();
   #ifdef CHECK_FOR_MEMORY_LEAKS
  	   DebugMemReportLeaks();
  	   DebugMemForgetLeaks();
   #endif
  #endif

  #if BALLOON_SUPPORT
	if (gHaveBalloonHelp) {
		MacAPI::HMSetFont(gBalloonHelpFont);
		MacAPI::HMSetFontSize(gBalloonHelpFontSize);
		MacAPI::HMSetBalloons(gBalloonHelpOnInSystem);		// restore balloon help state
	}
  #endif
	delete theApp;

	DEBUG_TERMINATE();
	
}
#ifdef __MC68K__
#pragma code68020 reset
#endif



enum PrefType {
	prefType_Bool,
	prefType_Long,
	prefType_Float,
	prefType_String,
	prefType_SKIP,
	prefType_END_LIST
};

struct PrefItem {
	const char* configName;
	PrefType	prefType;
};

#define SLIDER_START_IDX 19 // this is index in array, not PaneID

static PrefItem sPrefItems[] = {					// Pane ID  (idx+1)
	{ GALACTICA_PREF_PLAY_SOUND, 	prefType_Bool }, 	// 1
	{ GALACTICA_PREF_PLAY_MUSIC, 	prefType_Bool }, 	// 2
	{ GALACTICA_PREF_FULL_SCREEN, 	prefType_Bool }, 	// 3
	{ GALACTICA_PREF_AI_SKILL, 		prefType_Long }, 	// 4
	{ GALACTICA_PREF_AI_ADVANTAGE, 	prefType_Long },	// 5
	{ GALACTICA_PREF_FAST_GAME, 	prefType_Bool },	// 6
	{ GALACTICA_PREF_FOG_OF_WAR, 	prefType_Bool },	// 7
	{ GALACTICA_PREF_MAP_DENSITY, 	prefType_Long },	// 8
	{ GALACTICA_PREF_MAP_SIZE, 		prefType_Long },	// 9
	{ GALACTICA_PREF_SHOW_SPLASH, 	prefType_Bool },	// 10
	{ GALACTICA_PREF_SHOW_SHIPS, 	prefType_Bool },	// 11
	{ GALACTICA_PREF_SHOW_COURSES, 	prefType_Bool },	// 12
	{ GALACTICA_PREF_SHOW_NAMES, 	prefType_Bool },  	// 13
	{ GALACTICA_PREF_SHOW_RANGES, 	prefType_Bool },  	// 14
	{ GALACTICA_PREF_SHOW_COURIERS, prefType_Bool },	// 15	
	{ GALACTICA_PREF_SHOW_GRIDLINES,prefType_Bool },	// 16
	{ GALACTICA_PREF_SHOW_NEBULA,   prefType_Bool },	// 17
	{ 0, prefType_SKIP },
	{ GALACTICA_PREF_DEFAULT_BUILD, prefType_Long },	// 19
	{ GALACTICA_PREF_DEFAULT_GROWTH,prefType_Long },	// 20	
	{ GALACTICA_PREF_DEFAULT_SHIPS, prefType_Long },	// 21	
	{ GALACTICA_PREF_DEFAULT_TECH,  prefType_Long },	// 22	
	{ 0, prefType_END_LIST }		// end marker
};

#ifndef GALACTICA_SERVER

class PrefsDialogAttach : public CDialogAttachment {
public:
	PrefsDialogAttach() : CDialogAttachment(msg_AnyMessage, true, false) {}
	virtual bool	PrepareDialog();				// return false to abort display of dialog
	virtual bool	CloseDialog(MessageT inMsg);	// false to prohibit closing	
protected:
	CSliderLinker*	mLinker;
};

bool PrefsDialogAttach::PrepareDialog() {
	mLinker = new CSliderLinker();
	int idx = 0;
	CGalacticSlider *aSlider = NULL;
	for (int i = 0; i < MAX_LINKED_SLIDERS; i++) {
		aSlider = dynamic_cast<CGalacticSlider*>( mDialog->FindPaneByID(i + SLIDER_START_IDX + 1) );
		if (!aSlider) {
			break;	// if no slider found, quit checking
		}
		mLinker->AddSlider(aSlider);
	}
	mLinker->BeginLinking();
	mLinker->ListenToMessage(msg_SetSlidersEven, NULL);
	pdg::ConfigManager* config = Galactica::Globals::GetInstance().getConfigManager();
	while (sPrefItems[idx].prefType != prefType_END_LIST) {
		if (sPrefItems[idx].prefType != prefType_SKIP) {
			LPane* pane = mDialog->FindPaneByID(idx + 1);
		 	if (pane) {
			 	SInt32 val = 0;
			 	bool exists = false;
			 	if (sPrefItems[idx].prefType == prefType_Bool) {
			 		bool tempBool;
			 		exists = config->getConfigBool(sPrefItems[idx].configName, tempBool);
			 		val = tempBool;
			 	} else if (sPrefItems[idx].prefType == prefType_Long) {
			 		exists = config->getConfigLong(sPrefItems[idx].configName, val);
			 	}
			 	if (exists) {
		 			LControl* ctrl = dynamic_cast<LControl*>(pane);	// get each control on the prefs screen
		 			if (ctrl) {
					 	ctrl->StopBroadcasting();
					 	ctrl->SetValue(val);
					 	ctrl->StartBroadcasting();
				 	} else if (idx >= SLIDER_START_IDX) {
				 		aSlider = dynamic_cast<CGalacticSlider*>(pane); // get the slider
				 		if (aSlider) {
				 			aSlider->StopBroadcasting();
				 			aSlider->SetSliderValue(val);
				 			aSlider->StartBroadcasting();
				 		}
			 		}
			 	}
		 	}
	 	}
		++idx;
	}
	return true;
}

bool PrefsDialogAttach::CloseDialog(MessageT inMsg) {
	if (inMsg == button_NewGameBegin) {
		GalacticaApp* app = static_cast<GalacticaApp*>(LCommander::GetTopCommander());
		GalacticaDoc* doc = app->GetFrontmostGame();
		int idx = 0;
	 	pdg::ConfigManager* config = Galactica::Globals::GetInstance().getConfigManager();
		while (sPrefItems[idx].prefType != prefType_END_LIST) {
			if (sPrefItems[idx].prefType != prefType_SKIP) {
				LPane* pane = mDialog->FindPaneByID(idx + 1);
			 	LControl* ctrl = dynamic_cast<LControl*>(pane);	// get each control on the prefs screen
			 	if (ctrl) {
				 	SInt32 val = ctrl->GetValue();
				 	if (sPrefItems[idx].prefType == prefType_Bool) {
				 		bool tempBool = (val != 0);
				 		config->setConfigBool(sPrefItems[idx].configName, tempBool);
				 	} else if (sPrefItems[idx].prefType == prefType_Long) {
				 		config->setConfigLong(sPrefItems[idx].configName, val);
				 	}
				} else if (idx >= SLIDER_START_IDX) {
				 	CGalacticSlider* aSlider = dynamic_cast<CGalacticSlider*>(pane); // get the slider
				 	if (aSlider) {
						long val = aSlider->GetSliderValue();
						config->setConfigLong(sPrefItems[idx].configName, val);
					}
				}
			}
			++idx;
		}
		if (doc) {
			// adapt to new settings
			doc->UserChangedSettings();
		}
		return true;
	} else if (inMsg == button_NewGameCancel) {
		return true;
	} else {
		return false;
	}
}
#endif // !GALACTICA_SERVER


typedef long **classNumPtr;

GalacticaApp::GalacticaApp() {
    mGameSmithInited = false;
	sGalacticaAppDestroyed = false;
 #ifndef GALACTICA_SERVER
	sMusicMov = nil;
	mAbout = false;
	gHasNewGameWind = false;
	UScreenPort::Initialize();
  #if TARGET_OS_MAC && !TARGET_API_MAC_CARBON
	CDisplay::Init();	// make us Display Manager aware
  #endif // TARGET_OS_MAC
	// Register functions to create core PowerPlant classes
	
//	RegisterAllPPClasses();
	RegisterClass_(LButton);
	RegisterClass_(LCaption);
	RegisterClass_(LDialogBox);
	RegisterClass_(LEditField);
	RegisterClass_(LPane);
	RegisterClass_(LPicture);
	RegisterClass_(LScroller);
	RegisterClass_(LStdControl);
	RegisterClass_(LStdButton);
	RegisterClass_(LStdCheckBox);
	RegisterClass_(LStdRadioButton);
	RegisterClass_(LStdPopupMenu);
	RegisterClass_(LTextEditView);
	RegisterClass_(LView);
	RegisterClass_(LWindow);
	RegisterClass_(LRadioGroup);
	RegisterClass_(LTabGroup);
	RegisterClass_(LCicnButton);
	RegisterClass_(LActiveScroller);
	RegisterClass_(LTable);
	RegisterClass_(LPaintAttachment);
	RegisterClass_(LGroupBox);
	RegisterClass_(LTextButton);
	RegisterClass_(LToggleButton);
	RegisterClass_(CDividedView);
	RegisterClass_(LToggleButton);
	RegisterClass_(CDividedView);
	
	RegisterClass_(LBevelButton);
    if (UEnvironment::IsRunningOSX()) {
	    DEBUG_OUT("Running on OSX: registering Appearance Manager Classes", DEBUG_IMPORTANT);
	    // Let MacOS know that we are appearance-savy
	    MacAPI::RegisterAppearanceClient();
	    RegisterClassID_(LAMBevelButtonImp, LBevelButton::imp_class_ID);
	} else {
	    DEBUG_OUT("Running on Classic: registering Grayscale Appearance Classes", DEBUG_IMPORTANT);
    	RegisterClassID_(LGABevelButtonImp, LBevelButton::imp_class_ID);
    }
    
	// register our custom classes
	RegisterClass_(GalacticaWindow);
	RegisterClass_(CStarMapView);
	RegisterClass_(CBoxView);
	RegisterClass_(CBoxFrame);
	RegisterClass_(CNewButton);
	RegisterClass_(CNewToggleButton);
	RegisterClass_(CNewTable);
	RegisterClass_(CLayeredOffscreenView);
	RegisterClass_(LPPobView);
	RegisterClass_(LThermometerPane);
	
	RegisterClass_(CResCaption);
	RegisterClass_(CStyledText);
	RegisterClass_(CScrollingPict);

	RegisterClass_(CbxNaPanel);
	RegisterClass_(CbxShPanel);
//	RegisterClass_(CbxSIPanel);
	RegisterClass_(CbxInPanel);
	
	RegisterClass_(HorzSlider);
	RegisterClass_(VertSlider);
	RegisterClass_(CTextTable);
	RegisterClass_(CWaypointTable);
	RegisterClass_(CShipTable);
	RegisterClass_(CFleetTable);
	RegisterClass_(CEventTable);
	RegisterClass_(CGalacticSlider);
 #endif // GALACTICA_SERVER

	// find some internal reference numbers we can use to distinguish valid Thingy subclasses
	// from garbage data
  #if TARGET_API_MAC_CARBON || !TARGET_OS_MAC
    gTopMem = 0xffffffff;
  #else
	gTopMem = (UInt32) MacAPI::TempTopMem();
  #endif
	DEBUG_OUT("TopMem is: " << (void*)gTopMem, DEBUG_IMPORTANT);

  #ifdef DEBUG_MACROS_ENABLED
	int oldDebugLevel = gDebugLevel;
	DEBUG_SET_LEVEL(DEBUG_SHOW_NONE);
  #endif
	AGalacticThingy* it;
	it = new CStar(NULL, NULL);
	gCStarClassNum = **((classNumPtr)it);
	delete it;
	it = new CShip(NULL, NULL);
	gCShipClassNum = **((classNumPtr)it);
	delete it;
	it = new CFleet(NULL, NULL);
	gCFleetClassNum = **((classNumPtr)it);
	delete it;
	it = new CMessage(NULL);
	gCMessageClassNum = **((classNumPtr)it);
	delete it;
	it = new CDestructionMsg(NULL);
	gCDestructionMsgClassNum = **((classNumPtr)it);
	delete it;
	it = new CRendezvous(NULL, NULL);
	gCRendezvousClassNum = **((classNumPtr)it);
	delete it;
  #ifdef DEBUG_MACROS_ENABLED
	DEBUG_SET_LEVEL(oldDebugLevel);
  #endif

#if ( defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX )) && !defined( POSIX_BUILD )
	OSErr	err;
	
	// get the ref num for system pref folder
	Galactica::Globals::GetInstance().sSysVRef = kOnSystemDisk;
	Galactica::Globals::GetInstance().sPrfsFldrID = 0;
	err = MacAPI::FindFolder(kOnSystemDisk, kPreferencesFolderType, true, 
	                    &Galactica::Globals::GetInstance().sSysVRef, 
	                    &Galactica::Globals::GetInstance().sPrfsFldrID);	
#endif // PLATFORM_MACOS

#ifndef GALACTICA_SERVER
	// Make the busy cursor
	Galactica::Globals::GetInstance().setBusyCursor( new LAnimateCursor(CURS_AnimatedBusy, 8) );
	ThrowIfNil_( Galactica::Globals::GetInstance().getBusyCursor() );

	// Make the animated target cursor
	gTargetCursor = new LAnimateCursor(CURS_AnimatedTarget, 8);
	ThrowIfNil_( gTargetCursor );

	// Make the window menu.
	Galactica::Globals::GetInstance().setWindowMenu( new CWindowMenu( MENU_Window ) );
	ThrowIfNil_( Galactica::Globals::GetInstance().getWindowMenu() );
#endif // GALACTICA_SERVER

	AddAttachment( new CMakePrefsVarAttach());	// build the prefs variable and load the prefs

#if !TARGET_OS_WIN32 && !GALACTICA_SERVER
	LStr255 s;
	FSSpec fsSpec;
// Open the Galactica Music File's resource fork so the game music will be available
	s.Assign((short)STR_MusicFileName, 0);
	if (MacAPI::FSMakeFSSpec(0, 0, s, &fsSpec) == noErr) {
		gSoundResFileRefNum = MacAPI::FSpOpenResFile(&fsSpec, fsRdPerm);
	}

// Open the Galactica Art File's resource fork so the game art will be available
	s.Assign((short)STR_ArtHiResFileName, 0);	// 1st try to find the hi-res version
	gArtResFileRefNum = -1;
	if (MacAPI::FSMakeFSSpec(0, 0, s, &fsSpec) == noErr) {
		gArtResFileRefNum = MacAPI::FSpOpenResFile(&fsSpec, fsRdPerm);
		DEBUG_OUT("FSpOpenResFile returned "<< gArtResFileRefNum << " for large art", DEBUG_IMPORTANT);
	}
	if (gArtResFileRefNum == -1) {
		s.Assign((short)STR_ArtFileName, 0);	// then try the lo-res version if no hi-res available 
		if (MacAPI::FSMakeFSSpec(0, 0, s, &fsSpec) == noErr) {
			gArtResFileRefNum = MacAPI::FSpOpenResFile(&fsSpec, fsRdPerm);
    		DEBUG_OUT("FSpOpenResFile returned "<< gArtResFileRefNum << " for small art", DEBUG_IMPORTANT);
		}
	}
#endif // !TARGET_OS_WIN32 && !GALACTICA_SERVER

#ifdef CHECK_REGISTRATION
	long* lp = (long*) LoadResource(type_Creator, 0);
	if (lp) {
	    long n = *lp;
    	CCheckRegAttach::sVersionCode = BigEndian32_ToNative(n);
    	CCheckRegAttach::sVersionCode &= 0x7f7f7f7f;	// strip off high bits
    	UnloadResource(lp);
	}
#endif

#ifndef GALACTICA_SERVER
	CKeyCmdAttach *anAttach = new CKeyCmdAttach(0x05000000, cmd_GalacticaHelp);	// HELP key for Documentation
	AddAttachment(anAttach);

// Added   970703 JRW
	// read Star images from resource fork.
	sStarGrid = new CNewGWorld(PICT_StarGrid, 16);	// 16 bits per pixel, cannot use 0 here since
											// there may be no valid active port to match with
	if (sStarGrid->GetPICTid() == 0) {		// no star art was available
		delete sStarGrid;
		sStarGrid = nil;
	} else {
		sStarGrid->SetSubImageSize(30, 30);
// use the following with the high quality art
//		sStarGrid->SetSubImageSize(60, 60);
	}
// End add

#endif // GALACTICA_SERVER
}


// ---------------------------------------------------------------------------
//		* ~GalacticaApp			// replace this with your App type
// ---------------------------------------------------------------------------
//	Destructor
//

GalacticaApp::~GalacticaApp() {
  #ifndef NO_GAME_SMITH_SUPPORT
    if (mGameSmithInited) {
        GS_Terminate();
        mGameSmithInited = false;
    }
  #endif // NO_GAME_SMITH_SUPPORT
	sGalacticaAppDestroyed = true;
#ifndef GALACTICA_SERVER
  #if TARGET_OS_MAC && !TARGET_API_MAC_CARBON
	CDisplay::Stop();	// no further need for Display manager
  #endif // TARGET_OS_MAC
	if (sMusicMov) {
		delete sMusicMov;
	}
	if (gSoundResFileRefNum != -1) {
		::CloseResFile(gSoundResFileRefNum);
		gSoundResFileRefNum = -1;
	}
	if (gArtResFileRefNum) {
	   ::CloseResFile(gArtResFileRefNum);
	   gArtResFileRefNum = -1;
	}
	delete Galactica::Globals::GetInstance().getBusyCursor();
	if (sStarGrid) {		// delete star art
		delete sStarGrid;
		sStarGrid = nil;
	}
//	delete gWindowMenu;	// 1.1d8, does nothing but free memory, and CW10 PP makes it do a Signal_
#endif // GALACTICA_SERVER
}

void
GalacticaApp::Initialize() {
// no menu bar stuff for dedicated server
#ifndef GALACTICA_SERVER
// Get the menu bar
	LMenuBar	*theMBar = LMenuBar::GetCurrentMenuBar();
	ThrowIfNil_( theMBar );

   // setup admin menu
	LMenu *adminMenu = new LMenu( MENU_GameConfig );
	Galactica::Globals::GetInstance().setAdminMenu( adminMenu );

  // special case for when we have Tutorial Support but no Balloon Support
  #if !BALLOON_SUPPORT && TUTORIAL_SUPPORT
    LStr255 menuItemStr(STRx_MenuHelp, str_GalacticaTutorial);
    Galactica::Globals::GetInstance().getWindowMenu()->InsertCommand(menuItemStr, cmd_GalacticaTutorial, 2);
  #endif //!BALLOON_SUPPORT && TUTORIAL_SUPPORT
 
    // install the GameSmith menus
  #ifndef NO_GAME_SMITH_SUPPORT
    LMenu* gameSmithMenu = new LMenu( 9001, "\pGameSmith" );
    if (gameSmithMenu) {
        GS_InstallMenuItems(gameSmithMenu->GetMacMenuH(), 0);
        theMBar->InstallMenu( gameSmithMenu, kInsertHierarchicalMenu );
        LMenu* fileMenu = theMBar->FetchMenu( MENU_File );
        if (!fileMenu) {
            fileMenu = theMBar->FetchMenu( MENU_FileOSX );
        }
        if (fileMenu) {
            fileMenu->InsertCommand("\pGameSmith", 0, 3);
            fileMenu->InsertCommand("\p-", 0, 4);
            SetMenuItemHierarchicalMenu(fileMenu->GetMacMenuH(), 4, gameSmithMenu->GetMacMenuH());
        }
    }
    
  #endif // !NO_GAME_SMITH_SUPPORT
  
// Install the window menu.
	theMBar->InstallMenu( Galactica::Globals::GetInstance().getWindowMenu(), 0 );

// Install the debug menu, if debug reports are enabled
  #ifdef DEBUG
  	if (IsOptionKeyDown() || gUseDebugMenu) {
        try {
            LMenu *debugMenu = new LMenu( MENU_Debug );
            if (debugMenu) {
                theMBar->InstallMenu( debugMenu, 0);
            } else {
                SysBeep(1);
            }
        }
        catch(...) {
            DEBUG_OUT("Exception initializing debug menu. Missing Resource?", DEBUG_ERROR);
        }
  	}
  #endif

// set up the help menu	
  #if BALLOON_SUPPORT
	MenuHandle	mh;
	OSErr err = MacAPI::HMGetHelpMenuHandle(&mh);
	if ((err == noErr) && mh) {
		LStr255 menuItemStr1(STRx_MenuHelp, str_DisableFlybyHelp);
		MacAPI::MacInsertMenuItem (mh, menuItemStr1, kHMShowBalloonsItem);
		LStr255 menuItemStr2(STRx_MenuHelp, str_GalacticHelp);
		MacAPI::MacInsertMenuItem (mh, menuItemStr2, kHMShowBalloonsItem+2);
		LStr255 menuItemStr3(STRx_MenuHelp, str_GalacticaTutorial);
		MacAPI::MacInsertMenuItem (mh, menuItemStr3, kHMShowBalloonsItem+3);
	}
  #endif // BALLOON_SUPPORT

  #if TARGET_OS_MAC && !TARGET_API_MAC_CARBON
	MenuHandle langMenu = nil;
	langMenu = LMdGetLanguageMenu();	// now we grab our language menu
	if (langMenu != nil && (LMdGetNumModules()>1)) {
		MacAPI::MacInsertMenu(langMenu, 0);	// add the language menu to our fine menu bar
	}
  #endif // TARGET_OS_MAC

	// setup handler for scroll wheel
  	mScrollWheelHandlerOwner.Adopt(new CScrollWheelHandler);
#endif // GALACTICA_SERVER
}

OSType
GalacticaApp::GetInternalFileType(FSSpec *inMacFSSpec) {
	OSType theType;
	bool readInternal = true;
#if ( defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX )) && !defined( POSIX_BUILD )
	FInfo fileInfo;
	OSErr err;
	err = MacAPI::FSpGetFInfo(inMacFSSpec, &fileInfo);
	if (err != noErr) {
		theType = '\?\?\?\?';
		readInternal = false;
	} else {
		theType = fileInfo.fdType;
		if ( (theType == type_SavedTurnFile) || (theType == type_GameDataFile) ||
			 (theType == type_GameIndexFile) || (theType == type_GameOtherIndexFile) ) {
			readInternal = true;
		} else {
			readInternal = false;
        }		    
    }
#endif // PLATFORM_MACOS
    if (readInternal) {
		LFileStream file(*inMacFSSpec);
		try {
			file.OpenDataFork(fsRdWrShPerm);
			file >> theType;
			file.CloseDataFork();
		}
		catch(std::exception& e) {
			DEBUG_OUT("Exception "<<e.what()<<" trying to get file type", DEBUG_ERROR | DEBUG_USER | DEBUG_DATABASE);
			theType = '\?\?\?\?';
		}
	}
	return theType;
}

void
GalacticaApp::SetInternalFileType(FSSpec *inMacFSSpec, OSType inFType) {
	LFileStream file(*inMacFSSpec);
	file.OpenDataFork(fsRdWrShPerm);
	file << inFType;
	file.CloseDataFork();
}

/* GetHostFSSpecs
		Convert a single FSSpec, such as one returned from a call to StandardPutFile()
		into the two file specs needed for the data and index files.
	*/
void
GalacticaApp::GetHostFSSpecs(FSSpec &inSpec, FSSpec &outDataSpec, FSSpec &outIndexSpec) {
  #if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
	LStr255 filename = inSpec.name;
	LStr255 s;
	if (filename.EndsWith(".idx", 4)) {	// if our filename ends with .idx, remove the ending
		filename.Remove(filename.Length()-3,4);
	}
	if (filename.EndsWith(".dat", 4)) {	// if our filename ends with .dat, remove the ending
		filename.Remove(filename.Length()-3,4);
	}
	s.Assign(filename);
	s.Append(".dat");
	MacAPI::FSMakeFSSpec(inSpec.vRefNum, inSpec.parID, s, &outDataSpec);
	s.Assign(filename);
	s.Append(".idx");
	MacAPI::FSMakeFSSpec(inSpec.vRefNum, inSpec.parID, s, &outIndexSpec);
  #else
    std::string filename = inSpec.name;
    std::string s;
    if ((filename.length()>4) && (filename.compare(filename.size()-4, 4, ".idx") == 0)) { 	
       // if our filename ends with .idx, remove the ending
        filename.erase(filename.size()-4, 4);
    }
    if ((filename.length()>4) && (filename.compare(filename.size()-4, 4, ".dat") == 0)) { 	
         // if our filename ends with .idx, remove the ending
        filename.erase(filename.size()-4, 4);
    }
    s = filename + ".dat";
    MakeRelativeFSSpec(s.c_str(), &outDataSpec);
    s = filename + ".idx";
    MakeRelativeFSSpec(s.c_str(), &outIndexSpec);
  #endif // PLATFORM_MACOS
}

// we use the YieldTimeToForegoundTask() proc to make sure we remain cooperative while doing 
// heavy processing in the background. You can put calls to this proc in your time consuming 
// loops and it will slow your loop very little while the app is in the foreground, but will 
// make your app very cooperative with other apps while in the background.
void
GalacticaApp::YieldTimeToForegoundTask() {
  #ifndef GALACTICA_SERVER
	try {
		GalacticaApp* app = (GalacticaApp*) LCommander::GetTopCommander();
		if (!app->IsOnDuty()) {		// don't do anything if we are the foreground app
			EventRecord macEvent;
			Boolean gotEvent = MacAPI::WaitNextEvent(updateMask | osMask, &macEvent, 1, nil);	// yield one tick
		  #if SEND_DEBUGGING_TEXT_TO_SCREEN
			if (!SIOUXHandleOneEvent(&macEvent)) {	// can SIOUX handle the event?
				if (gotEvent) // only accepting update events to keep screen looking pretty
					app->DispatchEvent(macEvent);	// dispatch the update event
			}
		  #else
			if (gotEvent) { // only accepting update events to keep screen looking pretty
				app->DispatchEvent(macEvent);	// dispatch the update event
			}
		  #endif
		}
	}
	catch (...) {
		DEBUG_OUT("C++ exception caught in YieldTimeToForegroundTask()", DEBUG_IMPORTANT);
	}
  #endif // GALACTICA_SERVER
}


void
GalacticaApp::SavePrefs() {
	DEBUG_OUT("GalacticaApp::SavePrefs", DEBUG_IMPORTANT);
	pdg::ConfigManager* config = Galactica::Globals::GetInstance().getConfigManager();
	ThrowIfNil_(config);
	config->setConfigBool(GALACTICA_PREF_FULL_SCREEN, Galactica::Globals::GetInstance().getFullScreenMode());
  #ifndef GALACTICA_SERVER
	config->setConfigBool(GALACTICA_PREF_PLAY_SOUND, CSoundResourcePlayer::sPlaySound);
	config->setConfigBool(GALACTICA_PREF_PLAY_MUSIC, CMidiMovieFilePlayer::sPlayMusic);
  #endif // GALACTICA_SERVER
    std::string s( Galactica::Globals::GetInstance().getDefaultPlayerName() );
    config->setConfigString(GALACTICA_PREF_DEFAULT_NAME, s);
  	s = (char*)gBlob;
	config->setConfigString(GALACTICA_PREF_REGISTRATION, s);
  	s = (char*)gBlob2;
	config->setConfigString(GALACTICA_PREF_REG_BACKUP, s);
	if ((strlen(gBlob) > 0) && (strlen(gBlob2) > 0) && (DisplayUnregisteredWindow::sOldRegistration[0] > 0)) {
		// if there is a valid new registration code and the old registration is still set, then clear it
		s = "";
		config->setConfigString(GALACTICA_PREF_OLD_REGISTRATION, s);
	}
}

void
GalacticaApp::TakeOffDuty() {
	// restore system wide balloon help state for a suspend
  #if BALLOON_SUPPORT
	if (gHaveBalloonHelp) {
		MacAPI::HMSetFont(gBalloonHelpFont);
		MacAPI::HMSetFontSize(gBalloonHelpFontSize);
		gBalloonHelpOnInGalactica = MacAPI::HMGetBalloons();
		MacAPI::HMSetBalloons(gBalloonHelpOnInSystem);
	}
  #endif // !BALLOON_SUPPORT
}

void
GalacticaApp::PutOnDuty(LCommander* ) {	//inNewTarget
	// restore balloon help state Galactica's Balloon Help state for a resume
  #if BALLOON_SUPPORT
	if (gHaveBalloonHelp) {
		MacAPI::HMSetFont(gGalacticaFontNum);
		MacAPI::HMSetFontSize(12);
		gBalloonHelpOnInSystem = MacAPI::HMGetBalloons();
		MacAPI::HMSetBalloons(gBalloonHelpOnInGalactica);
	}
  #endif // !BALLOON_SUPPORT
  #ifndef NO_GAME_RANGER_SUPPORT
    // see if GameRanger sent us a command
    DEBUG_OUT("Checking for GameRanger command", DEBUG_IMPORTANT);
    if (GRCheckFileForCmd()) {
        DEBUG_OUT("GameRanger command detected", DEBUG_IMPORTANT);
        gDoStartup = false;
        GRGetWaitingCmd();
        DoGameRangerCommand();
        return; // don't do anything further
    }
  #endif // not defined NO_GAME_RANGER_SUPPORT
}

void
GalacticaApp::DisplayError(OSErr err) {
	if (err != noErr) {
        // inform of failure
		DEBUG_OUT("GalacticaApp displaying "<<err<<" to user", DEBUG_ERROR | DEBUG_USER);
	  #ifndef GALACTICA_SERVER
		if (err == dbDataCorrupt) {
			UModalAlerts::StopAlert(alert_FileDamaged);
		} else if (err == envBadVers) {
			UModalAlerts::StopAlert(alert_FileWrongVers);
		} else if (err != permErr) {
			LStr255 errStr(STRx_Errors, str_ErrorOccured);
			LStr255 errNumStr = (SInt32)err;
			errStr.Append(errNumStr);
			MacAPI::ParamText(errStr, "\p", "\p", "\p");
			UModalAlerts::StopAlert(alert_Error);
		}
	  #endif // GALACTICA_SERVER
    }
}

void 
GalacticaApp::DoGameRangerCommand() {
  #ifndef NO_GAME_RANGER_SUPPORT
    // see if GameRanger started us up
    DEBUG_OUT("DoGameRangerCommand", DEBUG_IMPORTANT);
    if (GRIsHostCmd()) {
        // temporarily switch the default port for creating games to the one we
        // specified using Game Ranger. That way when we host the game it
        // will be at the correct port
        UInt16 defaultPort = Galactica::Globals::GetInstance().getDefaultPort();
        Galactica::Globals::GetInstance().setDefaultPort(GRGetPortNumber());
        ObeyCommand(cmd_HostNewGame, NULL);
        SetUpdateCommandStatus(true);
        // restore default port
        Galactica::Globals::GetInstance().setDefaultPort(defaultPort);
        // see if we need to join the game we just started hosting
   #ifndef GALACTICA_SERVER   
        if (std::strlen(GRGetPlayerName()) > 0) {
            GalacticaClient* game = new GalacticaClient(this);
            // connect to the game we are hosting
            OSErr err = game->JoinGame(GRGetPlayerName(), 
                                       Galactica::Globals::GetInstance().getDefaultHostname(), 
                                       gameType_GameRanger, 
                                       GRGetPortNumber());
            if (err) {
                DisplayError(err);
            }
        }
    } else {
        GalacticaClient* game = new GalacticaClient(this);
       #warning FIXME: not handling rejoin case for Game Ranger games!
       // we would need to have the rejoin key
        OSErr err = game->JoinGame(GRGetPlayerName(), GRGetJoinAddressStr(), 
                                    gameType_GameRanger, 
                                    GRGetPortNumber());
        if (err) {
            DisplayError(err);
        }
   #endif // GALACTICA_SERVER
    }
  #endif // not defined NO_GAME_RANGER_SUPPORT
}

void
GalacticaApp::ReadHostConfigFile() {
   // attempt to read hostconfig file, primarily used for Galactica Server app
   // supports:
   // hostname={ip or name where ports are opened}
   // port={port number to host games}
   // autoload={name of file to load on startup}
   // see hostconfig-docs.txt for more
   Galactica::Globals::GetInstance().ClearAllowedAndBannedLists();
   std::FILE* fp = std::fopen("hostconfig.txt", "r");
   if (fp != NULL) {
	   DEBUG_OUT("Reading hostconfig.txt file", DEBUG_IMPORTANT);
      while (!std::feof(fp)) {
         char theLine[1024];
         if (std::fscanf(fp, "%[^\n\r]", theLine) > 0) {
            char* token;
            token = std::strtok(theLine, "=");
            token = std::strtok(NULL, "\n\r");
            if (theLine[0] == '#') {
               continue;   // skip comment lines
            }
            if (token) {
               if (stricmp(theLine, "hostname") == 0) {
                   Galactica::Globals::GetInstance().setDefaultHostname(token);  
            	   DEBUG_OUT(" hostname="<<token, DEBUG_IMPORTANT);
               } else if (stricmp(theLine, "port") == 0) {
                   Galactica::Globals::GetInstance().setDefaultPort(std::atoi(token));
            	   DEBUG_OUT(" port="<<token, DEBUG_IMPORTANT);
               } else if (stricmp(theLine, "status-hostname") == 0) {
                   Galactica::Globals::GetInstance().setStatusHostname(token);  
            	   DEBUG_OUT(" status-hostname="<<token, DEBUG_IMPORTANT);
               } else if (stricmp(theLine, "status-port") == 0) {
                   Galactica::Globals::GetInstance().setStatusPort(std::atoi(token));
            	   DEBUG_OUT(" status-port="<<token, DEBUG_IMPORTANT);
               } else if (stricmp(theLine, "autoload") == 0) {
                   Galactica::Globals::GetInstance().setAutoloadFile(token);
            	   DEBUG_OUT(" autoload="<<token, DEBUG_IMPORTANT);
               } else if (stricmp(theLine, "admin-pwd") == 0) {
                   char tmpPwd[MAX_PASSWORD_LEN+1];
                   for (int i = 0; i < MAX_PASSWORD_LEN; i++) {
                     if (token[i]) {
                        tmpPwd[i] = token[i] ^ 0xff;  // copy and encode the password
                     } else {
                        tmpPwd[i] = 0; // don't encode the nul terminator
                        break;
                     }
                   }
                   tmpPwd[MAX_PASSWORD_LEN] = 0;     // always nul terminate
                   Galactica::Globals::GetInstance().setAdminPassword(tmpPwd);
            	   DEBUG_OUT(" admin-pwd="<<token, DEBUG_IMPORTANT);
               } else if (stricmp(theLine, "admit-unreg") == 0) {
                  char c = std::tolower(token[0]);
                  if ( (c == 'y') || (c == 't') ) {   // "yes" or "true"
                     token = "yes";
                     Galactica::Globals::GetInstance().setAdmitUnreg(true);
                  } else {
                     token = "no";
                     Galactica::Globals::GetInstance().setAdmitUnreg(false);
                  }
            	   DEBUG_OUT(" admit-unreg="<<token, DEBUG_IMPORTANT);
               } else if (stricmp(theLine, "admit") == 0) {
                   Galactica::Globals::GetInstance().AddAllowedUser(token);
            	   DEBUG_OUT(" admitting regcode "<<token, DEBUG_IMPORTANT);
               } else if (stricmp(theLine, "ban") == 0) {
                   Galactica::Globals::GetInstance().AddBannedUser(token);
            	   DEBUG_OUT(" banning regcode "<<token, DEBUG_IMPORTANT);
               }
            }
         } else {
            char c;
            std::fscanf(fp, "%c", &c);
         }
      }
      std::fclose(fp);
      fp = NULL;
   }
}

void
GalacticaApp::InitializeGameSmith() {
  #ifndef NO_GAME_SMITH_SUPPORT
    long creatorType = 'SmCd';  // type_Creator
    if (!mGameSmithInited) {
    	std::time_t seed;	// initialize random number generator
    	std::time(&seed);
    	MacAPI::SetQDGlobalsRandomSeed((long)seed);
        GS_InitNetworking( creatorType );
        bool gameRegistered = Galactica::Globals::GetInstance().sRegistered;
        GS_Init("Galactica: Anno Dominari", creatorType, 16, false, gameRegistered, 0, 0, 0);
        GS_CreateNSpGames(false);   // don't create Net Sprocket games for us
        mGameSmithInited = true;
    }
  #endif // NO_GAME_SMITH_SUPPORT
}

#ifndef GALACTICA_SERVER

void
GalacticaApp::ReadLoginInfo(FSSpec* inMacFSSpec, LStr255 &outName, SInt8 &outPlayerNum) {
	OSType theType;
	UInt32 theVersion;
	LFileStream file(*inMacFSSpec);
	LStr255 password;   // not currently used
	file.OpenDataFork(fsRdPerm);
	file >> theType >> theVersion;
	
	if (theVersion == version_v12b10_SavedTurnFile) {
	    // support for saved game files saved from v1.2b10 through v2.0.x
	    DBPlayerInfoT info;
	    file.ReadBlock(&info, sizeof(DBPlayerInfoT) ); // no byte swapping necesary because we only get the name string
	    outName.Assign((StringPtr)info.name);
	    outPlayerNum = 1;   // for the moment
	} else {
	    // support for current saved game files
	    file >> outPlayerNum >> outName >> password;
	}
	file.CloseDataFork();
}


bool
GalacticaApp::GetLogin(std::string &outName, bool updatePrefs) {
   LStr255 nameStr(outName.c_str());
	bool entryOK = false;
	StDialogHandler	theHandler(window_EnterName, LCommander::GetTopCommander());
	LWindow *theDialog = theHandler.GetDialog();
	LEditField *theField = (LEditField*) theDialog->FindPaneByID(editText_LoginName);
	ThrowIfNil_(theField);
	theField->SetDescriptor(nameStr);
	theField->SelectAll();
	theDialog->SetLatentSub(theField);
	theDialog->Show();
	// v1.2fc4, tutorial, go ahead to next page
  #if TUTORIAL_SUPPORT
	if (Tutorial::TutorialIsActive()) {
		Tutorial* t = Tutorial::GetTutorial();
		if (t->GetPageNum() == tutorialPage_GameSetup) {
			t->NextPage();
		}
	}
  #endif // TUTORIAL_SUPPORT
	// end tutorial
	while (true) {
		MessageT hitMessage = theHandler.DoDialog();
		if (hitMessage == msg_Cancel) {
			break;
		} else if (hitMessage == msg_OK) {
			theField->GetDescriptor(nameStr);
			if (nameStr.Length() > MAX_PLAYER_NAME_LEN) {
			    nameStr.Remove(MAX_PLAYER_NAME_LEN+1, 255);
			}
			if (updatePrefs) {
		      if (nameStr != (ConstStringPtr)Galactica::Globals::GetInstance().getDefaultPlayerNamePStr()) {
				char playerName[256];
				p2cstrcpy(playerName, nameStr);
				Galactica::Globals::GetInstance().setDefaultPlayerName(playerName);
		        SavePrefs();
		      }
		    }
			entryOK = true;
			break;
		}
	}
	char tmpstr[256];
   p2cstrcpy(tmpstr, nameStr);
	outName = tmpstr;
	return entryOK;
}

void
GalacticaApp::ClickMenuBar(const EventRecord &inMacEvent) {
  #if TARGET_OS_MAC && !TARGET_API_MAC_CARBON
	LMdCheckForNewModules();	// we will always show the user exactly what's available **** ERZ
  #endif // TARGET_OS_MAC
	CBalloonApp::ClickMenuBar(inMacEvent);
}

#if SEND_DEBUGGING_TEXT_TO_SCREEN
// ---------------------------------------------------------------------------
//		* ProcessNextEvent
// ---------------------------------------------------------------------------
//	Retrieve and handle the next event in the event queue 
// Mofified from LApplication::ProcessNextEvent() by addition of SIOUX event support
// see LApplication::ProcessNextEvent() for comments
void
GalacticaApp::ProcessNextEvent() {
	EventRecord		macEvent;
	if (IsOnDuty()) {
		MacAPI::OSEventAvail(0, &macEvent);
		AdjustCursor(macEvent);
	}
	SetUpdateCommandStatus(false);
	Boolean	gotEvent = MacAPI::WaitNextEvent(everyEvent, &macEvent, mSleepTime, mMouseRgnH);
	if (!SIOUXHandleOneEvent(&macEvent)) {	// can SIOUX handle the event?
		if (LAttachable::ExecuteAttachments(msg_Event, &macEvent)) {
			if (gotEvent) {
				DispatchEvent(macEvent);
			} else {
				UseIdleTime(macEvent);
			}
		}
	}
	LPeriodical::DevoteTimeToRepeaters(macEvent);
	if (IsOnDuty() && GetUpdateCommandStatus()) {
		UpdateMenus();
	}
}
#endif



// ---------------------------------------------------------------------------
//		* StartUp
// ---------------------------------------------------------------------------
//	This function lets you do something when the application starts up. 
//	For example, you could issue your own new command, or respond to a system
//  oDoc (open document) event.
void
GalacticaApp::StartUp() {
	pdg::ConfigManager* config = Galactica::Globals::GetInstance().getConfigManager();
	bool showSplashScreen = true;
	if (!config->getConfigBool(GALACTICA_PREF_SHOW_SPLASH, showSplashScreen)) {
		showSplashScreen = true; // default to true if not set
	}
	if (!mAbout && !DisplayRegistrationWindow::sRegistrationWind && !DisplayRegistrationWindow::sHoldStartup) {	// delay startup event if registering
		if (gDoStartup) {
		    gDoStartup = false; // we have now done the startup
            DEBUG_OUT("Doing normal startup", DEBUG_IMPORTANT);

			if (showSplashScreen) {
				SetSleepTime(1);	// check for events more frequently now to smooth scrolling
				// init the QT Midi Music Player
				sMusicMov = new CMidiMovieFilePlayer(STR_MusicFileName, player_Loop, player_DeleteSelf);
				sMusicMov->SetFadeTicks(5*60);	// duration of fade
				mAbout = true;
				std::string s;
				GetVersionNumberString(kFileVersionID, s);
				MovableParamText(0, s.c_str());
				MovableAlert(window_IntroBox, nil, 0, false);	// don't beep
				mAbout = false;
				sMusicMov->FadeAway();	// the MusicPlayer will delete itself at volume level 0
				sMusicMov = nil;		// so we don't need to track it anymore
				SetSleepTime(6);
			}

			if (CLoadPrefsAttach::sPrefsFileWasntThere) {	// no prefs file, must be new install
//				Boolean useAppleGuide = false;				// so give user extra help
              #if TUTORIAL_SUPPORT
				ObeyCommand(cmd_GalacticaTutorial, nil);	// try to start the tutorial
				if (!Tutorial::TutorialIsActive()) {	// failed to start tutorial,
				  	ObeyCommand(cmd_GalacticaHelp, nil);	// try to open the docs
				}
			  #else  // on when there's no Tutorial support, just open the Galactica Help
			    ObeyCommand(cmd_GalacticaHelp, nil);
			  #endif //TUTORIAL_SUPPORT
			}
			long startupAction;
			if (config->getConfigLong(GALACTICA_PREF_START_ACTION, startupAction)) {
				ObeyCommand(startupAction, nil);
			} else {
				ObeyCommand(cmd_New, nil);		// issue a "new" command
//				ObeyCommand(cmd_HostNewGame, nil);		// issue a "host new" command
			}
		}
	}
}

Boolean
GalacticaApp::AllowSubRemoval(LCommander* inSub) {
  #if TUTORIAL_SUPPORT
	if (Tutorial::TutorialIsActive()) {
		Tutorial* t = Tutorial::GetTutorial();
		if (t && (t->GetTutorialWindow() == inSub)) {
			t->StopTutorial();	// must return true below so the window is deleted
					// since StopTutorial() doesn't delete the window
			sMouseLingerEnabled = true;
		}
	  #warning TODO: make tutorial warn user if they close it early that it must be started over
	}
  #endif //TUTORIAL_SUPPORT
	return true;
}



void
GalacticaApp::ShowHelpDocs() {
  #if TARGET_OS_MAC
	FSSpec spec;
	LStr255 docsName((short)STR_DocsFileName, 0);
	if (noErr == FSMakeFSSpec(0,0, docsName, &spec) ) {
		OSErr err = OpenFileWithBrowser(spec);
		if (err != noErr) {
			LStr255 numStr = (SInt32) err;
			MacAPI::ParamText(numStr, docsName, "\p", "\p");
			UModalAlerts::Alert(alert_CouldntLaunchBrowser);	// browser not found
		}
	} else {
		MacAPI::ParamText(docsName, "\p","\p","\p");
		UModalAlerts::Alert(alert_CouldntFindDocs);	// app not found
	}	
  #endif // TARGET_OS_MAC
}

void			
GalacticaApp::CheckForNewVersion() {
    std::string url;
    url = "http://www.annodominari.com/v";
	std::string s;
    url += gVersionString;
    url += "/";
    OSErr err = OpenHttpWithBrowser(url);
    if (err != noErr) {
	    LStr255 numStr = (SInt32) err;
		LStr255 urlStr = url.c_str();
		MacAPI::ParamText(numStr, urlStr, "\p", "\p");
		UModalAlerts::Alert(alert_CouldntLaunchBrowser);	// browser not found
    }
}

void			
GalacticaApp::GoToCommunitySite() {
    std::string url;
    url = "http://www.annodominari.com/community/";
    OSErr err = OpenHttpWithBrowser(url);
    if (err != noErr) {
	    LStr255 numStr = (SInt32) err;
		LStr255 urlStr = url.c_str();
		MacAPI::ParamText(numStr, urlStr, "\p", "\p");
		UModalAlerts::Alert(alert_CouldntLaunchBrowser);	// browser not found
    }
}

void
GalacticaApp::ShowAboutBox() {
	if (!mAbout) {	// prevent opening window twice
		SetSleepTime(0);	// check for events more frequently now to smooth scrolling
		// init the QT Midi Music Player
		sMusicMov = new CMidiMovieFilePlayer(STR_MusicFileName, player_Loop, player_DeleteSelf);
		sMusicMov->SetFadeTicks(5*60);	// duration of fade
		mAbout = true;
	  #if TARGET_OS_MAC && !TARGET_API_MAC_CARBON
		LMdMakeCurrentModuleTop();	// make module our curr res file
	  #endif
		std::string moduleVersionStr;	// since module is curr res file, vers comes from there
		GetVersionNumberString(kFileVersionID, moduleVersionStr);
		MovableParamText(0, gVersionString.c_str());
		MovableParamText(1, moduleVersionStr.c_str());	// got the app vers in main()
		MovableAlert(window_AboutBox, nil, 0, false);	// don't beep
		mAbout = false;
		sMusicMov->FadeAway();	// the MusicPlayer will delete itself at volume level 0
		sMusicMov = nil;		// so we don't need to track it anymore
		SetSleepTime(6);
	}
}

void
GalacticaApp::DoPreferences() {
	PrefsDialogAttach thePrefsAttach;
	MovableAlert(window_Preferences, nil, 0, false, &thePrefsAttach); // don't beep
}

void
GalacticaApp::FindCommandStatus(CommandT inCommand, Boolean &outEnabled, Boolean &outUsesMark,
								UInt16 &outMark, Str255 outName) {
	ResIDT		theMenuID;
	SInt16		theMenuItem;
	if (DisplayRegistrationWindow::sRegistrationWind) {	// disable all menus while registering
		outEnabled = false;								// except Quit and AboutÉ
		if ( (inCommand == cmd_About) || (inCommand == cmd_Quit) ) {
			outEnabled = true;
		}
		return;
	}
  #if !BALLOON_SUPPORT && TUTORIAL_SUPPORT
    if (inCommand == cmd_GalacticaTutorial) {
        outEnabled = true;
	    outMark = noMark;
	    return;
    }
  #endif //!BALLOON_SUPPORT && TUTORIAL_SUPPORT
	if (IsSyntheticCommand(inCommand, theMenuID, theMenuItem) ) {
		if (theMenuID == Galactica::Globals::GetInstance().getWindowMenu()->GetMenuID() ) {	// If command is from the Window menu.
			// Find the window corresponding to the menu item.
			LWindow	*theWindow = Galactica::Globals::GetInstance().getWindowMenu()->MenuItemToWindow( theMenuItem );
			if ( theWindow != nil ) { // All window menu items enabled and use a mark.
				outEnabled = true;
				outUsesMark = true;
				outMark = noMark;
				if ( theWindow == UDesktop::FetchTopRegular() ) { // mark top regular wind menu item
					outMark = checkMark;
				}
			}
		} 
		else {// Synthetic command not in Window menu. Call inherited.
			LApplication::FindCommandStatus( inCommand, outEnabled, outUsesMark, outMark, outName );
		}
	} else {
		switch (inCommand) {
		  #ifdef DEBUG
			case 9000:
			case 9001:
			case 9002:
			case 9003:
			case 9004:	// show DebugLevel
				outEnabled = true;
				outUsesMark = true;
				if (gDebugLevel == (inCommand-9000)) {
					outMark = diamondMark;
				} else {
					outMark = noMark;
				}
				break;
			case 9100:	// show status of DebugBreakOnErrors
              #if TARGET_OS_MAC
				outEnabled = UEnvironment::HasGestaltAttribute(gestaltOSAttr,gestaltSysDebuggerSupport);
				outEnabled = (outEnabled || AmIBeingMWDebugged());
			  #else
			    outEnabled = false;
              #endif // TARGET_OS_MAC
				outUsesMark = true;
				if (gDebugBreakOnErrors) {
					outMark = checkMark;
				} else {
					outMark = noMark;
				}
				break;
			case 9200:	// show errors from the gDebugErrorMask
			case 9201:
			case 9202:
			case 9203:
			case 9204:
			case 9205:
			case 9206:
			case 9207:
			case 9208:
			case 9209: {
                    outEnabled = true;
                    outUsesMark = true;
                    int shiftamount = 2 + inCommand - 9200;
                    if (gDebugErrorMask & (1<<shiftamount)) {
                        outMark = checkMark;
                    } else {
                        outMark = noMark;
                    }
                }
				break;
		  #endif
			case cmd_HostNewGame:
			case cmd_OpenAsHost:
			case cmd_New:
			case cmd_JoinGame:
			case cmd_Open:
				outEnabled = !CStarMapView::IsAwaitingClick(); // enable the New command
				break;
			case cmd_GalacticaHelp:
			case cmd_GalacticaCommunity:
			case cmd_CheckForNewVersion:
			case cmd_Preferences:
				outEnabled = true;
				break;
			case cmd_About:
				outEnabled = !mAbout;
				break;
			case cmd_Sound:
				outEnabled = true;
				outUsesMark = true;
				if (CSoundResourcePlayer::sPlaySound) {
					outMark = checkMark;
				} else {
					outMark = noMark;
				}
				break;
			case cmd_FullScreen:
				outEnabled = true;
				outUsesMark = true;
				if (Galactica::Globals::GetInstance().getFullScreenMode()) {
					outMark = checkMark;
				} else {
					outMark = noMark;
				}
				break;
			default:
				LDocApplication::FindCommandStatus(inCommand, outEnabled, outUsesMark, outMark, outName);
		}
	}
}

void
GalacticaApp::OpenDocument(FSSpec* inMacFSSpec) {
	OSType fType = GetInternalFileType(inMacFSSpec);
	OSErr theErr = noErr;
	switch (fType) {
		case type_TutorialFile:
			// double-clicked on tutorial file, open tutorial
			ObeyCommand(cmd_GalacticaTutorial, nil);
			break;
		case type_PrefsFile:
		case type_ArtFile:
			break;	// don't do anything with these files
		case type_MIDIMusicFile:
			ObeyCommand(cmd_About, nil);	// show about box if music file double-clicked
			break;	
		case type_GameDataFile:		        // multiplayer galaxy file (currently hosted)
		case type_GameIndexFile:	        // multiplayer galaxy index (currently hosted)
		case type_UnhostedGameDataFile:
		case type_UnhostedGameIndexFile:	// find out if we need to create a game doc too
    		{
            	FSSpec dataSpec, indexSpec;
        		GetHostFSSpecs(*inMacFSSpec, dataSpec, indexSpec);
    			GalacticaHostDoc* theHost = new GalacticaHostDoc(this);
    			theErr = theHost->Open(dataSpec, indexSpec);
    		}
			gDoStartup = false;
    		break;
		case type_SavedGameFile:	// single player game 
            {
                GalacticaSingleDoc* theSingleGame = new GalacticaSingleDoc(this);
                theErr = theSingleGame->Open(*inMacFSSpec);
                gDoStartup = false;
            }
		    break;
		default:
			theErr = dbDataCorrupt;
			break;
	}
	if (theErr) {
		DEBUG_OUT("GalacticaApp::OpenDocument failed", DEBUG_ERROR | DEBUG_USER);
		DisplayError(theErr);
	}
}


// ---------------------------------------------------------------------------
//		* PrintDocument
// ---------------------------------------------------------------------------
//	Print a Document specified by an FSSpec

void
GalacticaApp::PrintDocument(FSSpec*) { // inMacFSSpec
	DEBUG_OUT("In PrintDocument(), should never happen", DEBUG_ERROR | DEBUG_USER);
	gDoStartup = false;
}
	

void
GalacticaApp::ChooseDocument() {
	FSSpec	replyFile;
	OSType types[1] = {type_SavedGameFile};
	LFileTypeList fileTypes(1, types);
	PP_StandardDialogs::LFileChooser chooser;
	NavDialogOptions* dlgOptions = chooser.GetDialogOptions();
	if (dlgOptions) {	// only nav services will actually return a value here
		dlgOptions->dialogOptionFlags = (kNavDefaultNavDlogOptions & ~kNavAllowPreviews);
	}
	Boolean replyGood = chooser.AskChooseOneFile(fileTypes, replyFile);
	if (replyGood) {
		SendAEOpenDoc(replyFile);
	}
}

bool
GalacticaApp::ChooseHostDocument(FSSpec &outFileSpec) {
	FSSpec	replyFile;
    Boolean replyGood;
	if (IsOptionKeyDown()) {
	    // hold down option key to open a saved game file as into a host file
    	replyGood = PP_StandardDialogs::AskChooseOneFile(type_SavedGameFile, replyFile,
    			(kNavDefaultNavDlogOptions & ~kNavAllowPreviews));
	} else {
    	replyGood = PP_StandardDialogs::AskChooseOneFile(type_GameDataFile, replyFile,
    			(kNavDefaultNavDlogOptions & ~kNavAllowPreviews));
    }
	outFileSpec = replyFile;
	return replyGood;
}

bool
GalacticaApp::DoNewGame(NewGameInfoT &gameInfo) {
    OSErr   err = noErr;
    bool singlePlayer = !gameInfo.hosting; //(gameInfo.numHumans == 1);
    if (singlePlayer) {
        std::string theName = Galactica::Globals::GetInstance().getDefaultPlayerName();
        if ( GetLogin(theName) ) {	// let the user enter the login name
            GalacticaSingleDoc* game = new GalacticaSingleDoc(this);
    	    err = game->DoNewGame(gameInfo, theName);
    	} else {
          #if TUTORIAL_SUPPORT
    		if (Tutorial::TutorialIsActive()) {
    			Tutorial::GetTutorial()->NextPage();
    		}
          #endif // TUTORIAL_SUPPORT
		    return false;   // don't close the new game window
    	}
    } else {
        GalacticaHostDoc* host = new GalacticaHostDoc(this);
        err = host->DoNewGame(gameInfo);
        if (err == noErr) {
          #ifndef NO_GAME_RANGER_SUPPORT
            // don't do the join if this was a Game Ranger host command
            if (GRIsCmd() && GRIsHostCmd()) {
                return true;
            }
          #endif
        	// follow creation of a new host with attempt to join that game unless it is a robot game
        	if (gameInfo.numHumans > 0) {
        	    ObeyCommand(cmd_JoinGame, NULL);
        	}
        } else
        if (err == userCanceledErr) {
            return false; // user canceled, don't close game window
        }
    }
    if (err != noErr) {
        // inform of failure
		DEBUG_OUT("GalacticaApp::DoNewGame failed", DEBUG_ERROR | DEBUG_USER);
        DisplayError(err);
        return false;
    }
    return true;
}

void
GalacticaApp::DoJoinGame() {
	GalacticaClient* theGame = new GalacticaClient(this);
    OSErr err = theGame->BrowseAndJoinGame();
	if (err != noErr) {
        // inform of failure
		DEBUG_OUT("GalacticaApp::DoJoin failed", DEBUG_ERROR | DEBUG_USER);
        DisplayError(err);
    }
}

Boolean
GalacticaApp::ObeyCommand(CommandT inCommand, void *ioParam) {
	ResIDT			theMenuID;
	SInt16			theMenuItem;
#ifdef CHECK_FOR_HACKING
	if (!sInstalledShutdown) {
		AddAttachment(new CDoShutdownAttach);
	}
#endif
	Boolean handled = true;
	// for some reason cmd_Preferences is coming across as a synthetic cmd but not negative
	if (inCommand == 1324679167) {
		inCommand = cmd_Preferences;
	}
	switch (inCommand) {
	  #ifdef DEBUG
		case 9000:
		case 9001:
		case 9002:
		case 9003:
		case 9004:
			DEBUG_SET_LEVEL(inCommand - 9000);
			break;
		case 9100:	// set DebugBreakOnErrors
			gDebugBreakOnErrors = !gDebugBreakOnErrors;
			break;
		case 9200:	// set errors in the gDebugErrorMask
		case 9201:
		case 9202:
		case 9203:
		case 9204:
		case 9205:
		case 9206:
		case 9207:
		case 9208:
		case 9209: {
                long whicherr = 1L<<(2L + inCommand - 9200L);
                if (gDebugErrorMask & whicherr) {
                    DEBUG_DISABLE_ERROR(whicherr);
                } else {
                    DEBUG_ENABLE_ERROR(whicherr);
                }
            }
			break;
	  #endif
		case cmd_SharewareInfo:			// user got crippleware limit notice and
			new DisplayRegistrationWindow();			// pushed the "Shareware info" button
			break;
		case cmd_UpdateRegistration: // update now button in the Update (ReReg window)
			{
		       std::string s;
		       LoadStringResource(s, STR_UpdateNowURL);
			   OSErr err = OpenHttpWithBrowser(s, false); // not secure
			   if (err != noErr) {
				   LStr255 numStr = (SInt32) err;
				   LStr255 urlStr = s.c_str();
				   MacAPI::ParamText(numStr, urlStr, "\p", "\p");
				   UModalAlerts::Alert(alert_CouldntLaunchBrowser);	// browser not found
			   }
			}
			break;
		case cmd_EnableFlybyHelp: {
			CBalloonApp::sMouseLingerEnabled = !CBalloonApp::sMouseLingerEnabled;
		  #if BALLOON_SUPPORT
			SInt16 strIndex;
			if (CBalloonApp::sMouseLingerEnabled) {
				strIndex = str_DisableFlybyHelp;
			} else {
				strIndex = str_EnableFlybyHelp;
			}
			LStr255 menuItemStr(STRx_MenuHelp, strIndex);
			MenuHandle mh;
			MacAPI::HMGetHelpMenuHandle(&mh);
			MacAPI::SetMenuItemText(mh, kEnableFlybyHelpItem, menuItemStr);
		  #endif
			SavePrefs();
			break;
		}
		case cmd_GalacticaHelp:
			ShowHelpDocs();
			break;
		case cmd_GalacticaCommunity:
			GoToCommunitySite();
			break;
		case cmd_CheckForNewVersion:
			CheckForNewVersion();
			break;
		case cmd_GalacticaTutorial:
		  #if TUTORIAL_SUPPORT
            {
                Tutorial* t = Tutorial::GetTutorial();
                SInt16 startingPage = tutorialPage_Welcome;
                SInt16 nextPage = tutorialPage_ChooseNewGame;	// assume we will start tutorial on page one and continue
                // to page 2, but check front window, modal or regular, to see if we need to
                // go to a different page as the next page
                LWindow* top = UDesktop::FetchTopModal();
                PaneIDT id;
                if (top) {
                    id = top->GetPaneID();
                    if ( (id == window_IntroBox) || (id == window_AboutBox) ) {
                        // about or splash windows, close them immediately, but
                        // we still start with page one
                        top->ObeyCommand(cmd_Close, nil);
                        if (id == window_AboutBox) {
                            top = NULL;	// so we can check for other windows under the about box
                        }
                    } else if ( (id != window_NewGame) && (id != window_EnterName) ) {
                        // some other modal window
                        // ask them to deal with it first
                        startingPage = tutorialPage_CloseModal;
                    }
                }
                if (!top) {
                    top = UDesktop::FetchTopRegular();
                    if (top) {
                        id = top->GetPaneID();
                        if (id == window_StarMap) {
                            // already have a game open, let them know that we
                            // are going to hide it
                            startingPage = tutorialPage_HidingGame;
                            nextPage = tutorialPage_Welcome;
                        }
                    }
                }
                t->StartTutorial(this, startingPage);
                t->ForceNextPageToBe(nextPage);
                sMouseLingerEnabled = false;
            }
		  #endif // TUTORIAL_SUPPORT
			break;
		case cmd_TutorialOK:
		  #if TUTORIAL_SUPPORT
            {
                Tutorial* t = Tutorial::GetTutorial();
                if (t->GetPageNum() == tutorialPage_HidingGame) {
                    // special case, hide a previously open game
                    LWindow* top = UDesktop::FetchTopRegular();
                    if (top) {
                        top->Hide();
                    }
                    t->NextPage();
                } else if (t->GetPageNum() == tutorialPage_CloseModal) {
                    // special case, need to restart tutorial after they closed a
                    // window that was causing problems.
                    t->GetTutorialWindow()->ObeyCommand(cmd_Close, nil);	// close the tutorial
                    ObeyCommand(cmd_GalacticaTutorial, nil);	// restart the tutorial
                } else {
                    t->NextPage();	// normally we just go to the next page
                }
                break;
            }
		  #endif // TUTORIAL_SUPPORT
		case cmd_New:
		case cmd_HostNewGame: {
		    if (!mAbout) {
                DEBUG_OUT("New or HostNew cmd", DEBUG_USER | DEBUG_IMPORTANT);
                bool isHosting = (inCommand == cmd_HostNewGame);
    			CGameSetupAttach theSetupAttach(isHosting);
    			ResIDT windID = (inCommand == cmd_New) ? window_NewGame: window_HostNewGame;
    			long response = MovableAlert(windID, this, 0, false, &theSetupAttach);	// display new game setup wind
    		  #if TUTORIAL_SUPPORT
    			if (Tutorial::TutorialIsActive()) {
    				Tutorial* t = Tutorial::GetTutorial();
    				t->NextPage();
    			}
    		  #endif // TUTORIAL_SUPPORT
		    }
			break;
		}
		case cmd_DoNewGame:
            DEBUG_OUT("DoNewGame cmd", DEBUG_USER | DEBUG_IMPORTANT);
			SetUpdateCommandStatus(true);
			handled = DoNewGame(*((NewGameInfoT*)ioParam));
			break;
		case cmd_JoinGame:
			DoJoinGame();
			break;
		case cmd_OpenAsHost: {
			FSSpec aHostFile;
			SetUpdateCommandStatus(true);
			if (ChooseHostDocument(aHostFile) ) {
			    // support creating a host from a saved single player game file
				if (GetInternalFileType(&aHostFile) == type_SavedGameFile) {
        			GalacticaHostDoc* theHost = new GalacticaHostDoc(this);
				    theHost->Open(aHostFile);
				} else {
				    SendAEOpenDoc(aHostFile);
				}
			}
			break;
		}
	  #if BALLOON_SUPPORT
		case cmd_ToggleBalloons:
			MacAPI::HMSetBalloons(!HMGetBalloons());
			break;
	  #endif
		case cmd_Sound: {
			CSoundResourcePlayer::sPlaySound = !CSoundResourcePlayer::sPlaySound;
			CMidiMovieFilePlayer::sPlayMusic = CSoundResourcePlayer::sPlaySound;
			SavePrefs();
			SetUpdateCommandStatus(true);
			break;
		}
		case cmd_FullScreen: {
			Galactica::Globals::GetInstance().setFullScreenMode( !Galactica::Globals::GetInstance().getFullScreenMode() );
			SavePrefs();
			AdjustAllGameWindows();
			SetUpdateCommandStatus(true);
			break;
		}
		case cmd_Preferences:
			DoPreferences();
			break;
		default: {	// special cases
			if (IsSyntheticCommand(inCommand, theMenuID, theMenuItem) ) {
				if (theMenuID == Galactica::Globals::GetInstance().getWindowMenu()->GetMenuID() ) {// If command is from the Window menu.
					// Find the window corresponding to the menu item.
					LWindow	*theWindow = Galactica::Globals::GetInstance().getWindowMenu()->MenuItemToWindow( theMenuItem );
					if ( theWindow != nil ) {
						theWindow->Show();	// make sure it's visible
						UDesktop::SelectDeskWindow( theWindow );	// Bring the window to the front.
					}
				}
			  #if TARGET_OS_MAC && !TARGET_API_MAC_CARBON
				else if (theMenuID == 1) {			// was it a selection from the language menu?
					// v1.2b11d6, fix problem with rebuilding Apple Menu
					LMenuBar* mbar = LMenuBar::GetCurrentMenuBar();
					LMenu* menu = mbar->FetchMenu(MENU_Apple);
					mbar->RemoveMenu(menu);
					delete menu;
					menu = nil;
					// end v1.2b11d6 changes
					LMdHandleMenuSelect(theMenuItem);	// tell LangMod of selection
					gStartupLanguageCode = LMdGetCurrentModuleInfo(nil);	// get our new language
					SavePrefs();						// make game start up in that language
					// v1.2b11d6, rebuild Apple Menu
					mbar->InstallMenu(new LMenu(MENU_Apple), MENU_File);
													// Populate the Apple Menu
					MenuHandle	macAppleMenuH = ::GetMenuHandle(MENU_Apple);
					if (macAppleMenuH != nil) {
						::AppendResMenu(macAppleMenuH, ResType_Driver);
					}
					// end v1.2b11d6 changes
				} 
			  #endif // TARGET_OS_MAC
				else { // Synthetic command not in Window menu. Call inherited.
					handled = LApplication::ObeyCommand( inCommand, ioParam );
				}
			} else if (DisplayRegistrationWindow::sRegistrationWind) { // case of reg wind open
				Boolean closeWind = false;	// close the registration window?

			  #if TARGET_OS_MAC
				if (inCommand == cmd_ChooseRegMethod) {		// "Register" button in Shareware wind
					CommandT theCmd;
					if (HasWebBrowser()) {	// check for existance of web browser on user's system
						theCmd = MovableAlert(window_RegisterMethod, this, 0, false); // no beepÉ
					} else {		//Équery user: Web, secure Web, or FAX/email/postal registration
						theCmd = cmd_LaunchRegister;	// no online reg available, don't offer
					}
					switch (theCmd) {
						case cmd_RegisterOnline:
						case cmd_RegisterSecure: {			// try to do SSL online registration
						   {
						      std::string s;
						      LoadStringResource(s, STR_RegisterOnlineURL);
							   OSErr err = OpenHttpWithBrowser(s, (theCmd == cmd_RegisterSecure));
							   if (err != noErr) {
								   LStr255 numStr = (SInt32) err;
								   LStr255 urlStr = s.c_str();
								   MacAPI::ParamText(numStr, urlStr, "\p", "\p");
								   UModalAlerts::Alert(alert_CouldntLaunchBrowser);	// browser not found
							   }
							}
							break;
						}
					  #ifdef MONKEY_BYTE_VERSION
						case cmd_RegisterByMail:	// FAX or postal registration
							MovableAlert(window_OrderForm, this, 0, false, new COrderFormAttach);
							break;
					  #else
						case cmd_LaunchRegister:	// FAX, email or postal registration
							LaunchRegisterApp();
							break;
					  #endif
					}	
				}	// now continue on since we changed the command above
			  #endif // TARGET_OS_MAC
			  
				switch (inCommand) {	// all these commands are only generated by the reg window
				    case cmd_BoxRegistration:
					   {
					       std::string s;
					       LoadStringResource(s, STR_RegisterOnlineURL);
						   OSErr err = OpenHttpWithBrowser(s, false); // not secure
						   if (err != noErr) {
							   LStr255 numStr = (SInt32) err;
							   LStr255 urlStr = s.c_str();
							   MacAPI::ParamText(numStr, urlStr, "\p", "\p");
							   UModalAlerts::Alert(alert_CouldntLaunchBrowser);	// browser not found
						   }
						}
						break;
					case cmd_NotYet:	// not yet button in registration window
						closeWind = true;			// go ahead and close the window
						SavePrefs();				// record that we have no registration
						break;
					case cmd_EnterCode: {	// "enter code" button in registration window
						Boolean goodCode = true;
						LPane* regNumPane = DisplayRegistrationWindow::sRegistrationWind->FindPaneByID(editText_RegCode);
						if (regNumPane) {
							LStr255 s;
							regNumPane->GetDescriptor(s);
							goodCode = (s.Length() == 27);	// registration fails if: length not 27
							char lc = CCheckRegAttach::sVersionCode >> 24;
							if (lc!='*') {
								if (s[3] != lc)	{			// hi 2 bytes of owner resource != chars 3 & 4 of
									goodCode = false;		// registration code: basically a check to
								}
							}								//  make sure the language version matches
							lc = (CCheckRegAttach::sVersionCode >> 16) & 0xff;	// registration code so one 
							if (lc!='*') {					// can't use english vers reg code in german vers
								if (s[4] != lc)	{			// "**" is accepted as a wildcard in vers
									goodCode = false;
								}
							}
							char vers = (CCheckRegAttach::sVersionCode >> 8) & 0xff;
							if (s[26] != vers) {				// code from different major version release
								goodCode = false;
							}
							if (s[1] != 'G') {				// code is from different application
								goodCode = false;
							}
							if (s[2] != 'L') {
								goodCode = false;
							}
							int checksum = 0;					// compute checksum for this code
							int i = 23;
							char* p3 = &gBlob[15];
							char* p4 = &gBlob2[15];
							while (i>6) {
								if (((i-5)%5) == 4)	{			// skip over the dashes
									i--;
								}
								int ri = s[i+1] -'0';
								checksum += ri;					// adjust checksum
								*p3-- = gConfigCvt[ri];
								*p4-- = gConfigCv2[ri];
								i--;
							}
							checksum -= (s[26] - '0');			// include version number in checksum
							checksum -= ((s[27] - '0') * 10);
							
							gBlob[16] = gConfigCvt[(s[26] - '0')];
							gBlob2[16] = gConfigCv2[(s[26] - '0')];

							gBlob[17] = gConfigCvt[(s[27] - '0')];
							gBlob2[17] = gConfigCv2[(s[27] - '0')];
							
							gBlob[0] = gConfigCvt[(s[6] - '0')];
							gBlob2[0] = gConfigCv2[(s[6] - '0')];
							
							gBlob[1] = gConfigCvt[(s[7] - '0')];
							gBlob2[1] = gConfigCv2[(s[7] - '0')];
							
							if (s[7] != ((checksum % 10) + '0')) {	// fail if checksum incorrect
								goodCode = false;
							}
							if (s[6] != (((checksum / 10) % 10) + '0')) {
								goodCode = false;
							}
							if (goodCode == false) {
								s = "\p";
								gBlob[0] = 0;
								gBlob2[0] = 0;
							}
							LString::CopyPStr(s, Galactica::Globals::GetInstance().sRegistration, sizeof(Str27));
							SavePrefs();
						}
						closeWind = goodCode;
						CCheckRegAttach::sRegistered = goodCode;
						if (!goodCode) {	// invalid reg code entered
							DEBUG_OUT("Invalid Registration Code", DEBUG_ERROR | DEBUG_USER);
							SysBeep(1);
						}
					}
				}
				if (closeWind) {	// we can close the registration window
					DisplayRegistrationWindow::sRegistrationWind->DoClose();
					DisplayRegistrationWindow::sRegistrationWind = nil;
					DisplayRegistrationWindow::sWindBeingDisplayed = false;
					SetUpdateCommandStatus(true);
					StartUp();	// finish delayed startup event
				}
			} else { // end special case of registration window open
				handled = LDocApplication::ObeyCommand(inCommand, ioParam);
			}
		} // end special cases
	}	// end main switch(inCommand)
	return handled;
}

bool
GalacticaApp::AskNewGameFile(FSSpec &outDataSpec, FSSpec &outIndexSpec, LStr255 &inDefaultName) {
	Boolean replyGood;
    FSSpec replyFile;
	if (inDefaultName.EndsWith(".idx", 4)) {	// if our filename ends with .idx, remove the ending
		inDefaultName.Remove(inDefaultName.Length()-3,4);
	}
	if (!inDefaultName.EndsWith(".dat", 4)) {	// if filename doesn't end with .dat, add it.
		inDefaultName.Append("\p.dat");
	}
	bool replacing;
	replyGood = PP_StandardDialogs::AskSaveFile(inDefaultName, type_GameDataFile, 
							replyFile, replacing, kNavDefaultNavDlogOptions);

	if (replyGood) {
		GetHostFSSpecs(replyFile, outDataSpec, outIndexSpec);
		if (replacing) {	// Delete existing files
			OSErr err = noErr;
			err = ::FSpDelete(&replyFile);
			if (err == fBsyErr) {
				DEBUG_OUT("Exception "<<err<<". File Busy, could not delete", DEBUG_ERROR | DEBUG_USER | DEBUG_DATABASE);
				LStr255 s = replyFile.name;
				MovableParamText(0, s);
				MovableAlert(window_CouldntDelete, nil);
				replyGood = false;
			} else {
				DEBUG_OUT("Exception "<<err<<" trying to delete file", DEBUG_ERROR | DEBUG_USER | DEBUG_DATABASE);
				ThrowIfOSErr_(err);		// bad if other error occurs
			}
			if (err == noErr) {
				::FSpDelete(&outDataSpec);	// other names may or may not exist, don't worry if
				::FSpDelete(&outIndexSpec);	// they don't
			}
		}
	}
	return replyGood;
}


void
GalacticaApp::AdjustAllGameWindows() {
	LArrayIterator iterator(mSubCommanders, LArrayIterator::from_Start);
	LCommander	*theSub;
	GalacticaDoc *theGameDoc;
	while (iterator.Next(&theSub)) {		// resize all the document windows
		theGameDoc = dynamic_cast<GalacticaDoc*> (theSub);	// only do this with game windows
		if (theGameDoc) {
			theGameDoc->AdjustWindowSize();
		}
	}
}

GalacticaDoc*	
GalacticaApp::GetFrontmostGame() {
	GalacticaDoc* resultDoc = 0;
	LWindow* wind = UDesktop::FetchTopRegular();
	GalacticaWindow* gwin = dynamic_cast<GalacticaWindow*>(wind);
	if (gwin) {
		LCommander* cmdr = gwin->GetSuperCommander();
		resultDoc = dynamic_cast<GalacticaDoc*>(cmdr);
	}
	return resultDoc;
}


#if ( defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ) ) && !defined( POSIX_BUILD )

pascal MacAPI::OSStatus    
CarbonEventHandler(MacAPI::EventHandlerCallRef handler, MacAPI::EventRef event, void* userData) {
    MacAPI::OSStatus err = MacAPI::eventNotHandledErr;
//    MacAPI::EventRecord macEvent;
    
    switch ( MacAPI::GetEventClass(event) ) { 
        case kEventClassMouse:
            {
                // this is a quick hack to support right clicking. We don't actually handle the clicks
                // from this, we just record whether it was a right click or not then pass back that it
                // wasn't handled. The standard PowerPlant event loop will pick up the click normally
                // and can query the GalacticaApp::IsRightClick() static method if it cares about right clicks
                UInt32 eventKind = MacAPI::GetEventKind(event);
                EventMouseButton buttonPressed = 0;
                MacAPI::GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL, sizeof(EventMouseButton), NULL, &buttonPressed);
		        GalacticaApp::SetIsRightClick(buttonPressed == kEventMouseButtonSecondary);
            }
            break;
        default:
            break;
    }
    return err;
}

#endif // PLATFORM_MACOS



#else



#if ( defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX )) && !defined( POSIX_BUILD )
// This section contains a custom Run loop, along with a handler for the quit Apple Event
// it is only used when GALACTICA_SERVER is defined, because we don't want to drag in the
// entire PowerPlant Apple Event Model just to handle quitting

pascal OSErr QuitAppleEventHandler(const AppleEvent*, AppleEvent*, long);

pascal OSErr QuitAppleEventHandler(const AppleEvent*, AppleEvent*, long) {
    static_cast<GalacticaApp*>(LCommander::GetTopCommander())->Quit();
	return noErr;
}

#endif // PLATFORM_MACOS

#ifdef PLATFORM_UNIX
extern "C" {
   #include <signal.h>
  	
   void signal_handler(int signo);

	void signal_handler(int signo) {
		// do graceful shutdown so we close the database properly
      static_cast<GalacticaApp*>(LCommander::GetTopCommander())->Quit();
	   DEBUG_OUT("Quitting because we got signal ["<<signo<<"]", DEBUG_ERROR);
	}
}
#endif

void
GalacticaApp::Quit() {
   mState = programState_Quitting;
	DEBUG_OUT("Got Quit Command.", DEBUG_IMPORTANT);
}

#ifdef PLATFORM_UNIX
   #include <unistd.h>
#endif

void
GalacticaApp::Run() {

   GalacticaHost* theHost = 0;
    
	mState = programState_StartingUp;
	sTopCommander = this;
	try {

#if ( defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX )) && !defined( POSIX_BUILD )
	    // install a handler for the quit event
		OSErr err = MacAPI::AEInstallEventHandler(MacAPI::kCoreEventClass, MacAPI::kAEQuitApplication, 
		              MacAPI::NewAEEventHandlerUPP(QuitAppleEventHandler), 0, false);
		ThrowIfOSErr_(err);
#endif // PLATFORM_MACOS

#ifdef PLATFORM_UNIX
      // install a signal handler for all signals
      //      signal(SIGABRT, signal_handler);
      //      signal(SIGFPE, signal_handler);
      //      signal(SIGLL, signal_handler);
      signal(SIGINT, signal_handler);  // SIGINT (ctrl-c)
      //      signal(SIGSEGV, signal_handler);
      //      signal(SIGTERM, signal_handler);
      //      signal(SIGBREAK, signal_handler);
      // setup the OpenPlay library environment variable
      #define MY_ARBITRARY_PATH_LEN 2048
      char currdir[MY_ARBITRARY_PATH_LEN];
      getcwd(currdir, MY_ARBITRARY_PATH_LEN);
      char path[MY_ARBITRARY_PATH_LEN];
      sprintf(path, "%s/OpenPlay Modules", currdir);
      setenv("OPENPLAY_LIB", path, 0); // can be overridden in environment
#endif // PLATFORM_UNIX

		Initialize();
		ForceTargetSwitch(this);

      #warning TODO: replace following with function to read or create a file based on command line params
        // create a base filespec
    	FSSpec fsSpec, dataSpec, indexSpec;
    	// if a name was given on the command line, override what was in the config file
    	if (gGameName.length() != 0) {
			Galactica::Globals::GetInstance().setAutoloadFile(gGameName.c_str());
		}
    #if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
        Str255 fname;
        c2pstrcpy(fname, Galactica::Globals::GetInstance().getAutoloadFile());
    	MacAPI::FSMakeFSSpec(0, 0, fname, &fsSpec);
    #else
        MakeRelativeFSSpec(Galactica::Globals::GetInstance().getAutoloadFile(), &fsSpec);
    #endif
    	// create data and index file specs
    	GetHostFSSpecs(fsSpec, dataSpec, indexSpec);
    	// create the host
    	theHost = new GalacticaHost();
    	
    	if (gCreateNewGame) {
			NewGameInfoT newGameInfo;
			c2pstrcpy(newGameInfo.gameTitle, gGameName.c_str());
			newGameInfo.numHumans = gNumPlayers - gNumAIs;
			newGameInfo.numComputers = gNumAIs;
			newGameInfo.density = gDensity;
			newGameInfo.sectorSize = gMapSize;
			newGameInfo.compSkill = gSkillLevel;
			newGameInfo.compAdvantage = 0;
			newGameInfo.maxTimePerTurn = gTurnTime * 60;
			newGameInfo.hosting = true;
			newGameInfo.fastGame = gFastGame;
			newGameInfo.omniscient = !gFogOfWar;
			newGameInfo.endEarly = gEndTurnEarly;
			theHost->DoCreateNew(dataSpec, indexSpec, newGameInfo);   // create the host files & init server
			theHost->InitNewGameData(newGameInfo);
			theHost->StartIdling();
		} else if (gLoadGame) {
			theHost->SpecifyHost(dataSpec, indexSpec);	// create the database file objects
			theHost->OpenHost();	// get exclusive write and mark ourselves as host
			theHost->LoadGameInfo();
			theHost->SaveGameInfo(true);	// we have a host now
			theHost->InitializeServer();
			theHost->ReadGameData();
			theHost->ResumeTurnClock();
			theHost->StartIdling();
    	}

		mState = programState_ProcessingEvents;
	}

	catch (...) {
		// Initialization failed. After signalling, the program
		// will terminate since mState will not have been
		// set to programState_ProcessingEvents.
		DEBUG_OUT("App Initialization failed.", DEBUG_ERROR);
	}

	while (mState == programState_ProcessingEvents) {
		try {
        	EventRecord		macEvent;
        #if ( defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX )) && !defined( POSIX_BUILD )
        	bool gotEvent = MacAPI::WaitNextEvent(everyEvent, &macEvent, 6, NULL);
        #else
            bool gotEvent = false;
        #endif
    		// Let Attachments process the event. Continue with normal
    		// event dispatching unless suppressed by an Attachment.
	        if (LAttachable::ExecuteAttachments(msg_Event, &macEvent)) {
			    if (gotEvent) {
                  #if ( defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX )) && !defined( POSIX_BUILD )
			        if (macEvent.what == MacAPI::kHighLevelEvent) {
                        MacAPI::AEProcessAppleEvent(&macEvent);
			        }
			      #endif // PLATFORM_MACOS
			    } else {
	                LPeriodical::DevoteTimeToIdlers(macEvent);
			    }
	        }
			// Repeaters get time after every event
	        LPeriodical::DevoteTimeToRepeaters(macEvent);
		}

			// You should catch all exceptions in your code.
			// If an exception reaches here, we'll signal
			// and continue running.

		catch (std::bad_alloc) {
			DEBUG_OUT("bad_alloc (out of memory) exception caught in GalacticaApp::Run", DEBUG_ERROR);
		}
		catch (const LException& inException) {
			DEBUG_OUT("LException "<<inException.GetErrorCode()<<": "<<inException.what()<<" caught in GalacticaApp::Run", DEBUG_ERROR);
		}
		catch (std::exception& inExcept) {
			DEBUG_OUT("standard "<< inExcept.what() << " exception caught in GalacticaApp::Run", DEBUG_ERROR);
		}
		catch (ExceptionCode inError) {
			DEBUG_OUT("PowerPlant Exception Code "<<inError<<" caught in GalacticaApp::Run", DEBUG_ERROR);
		}
		catch (...) {
			DEBUG_OUT("Exception caught in GalacticaApp::Run", DEBUG_ERROR);
		}
	}
#ifdef PLATFORM_UNIX
      // restore default signal handler for all signals
      //      signal(SIGABRT, SIG_DFL);
      //      signal(SIGFPE, SIG_DFL);
      //      signal(SIGLL, SIG_DFL);
      signal(SIGINT, SIG_DFL);  // SIGINT (ctrl-c)
      //      signal(SIGSEGV, SIG_DFL);
      //      signal(SIGTERM, SIG_DFL);
      //      signal(SIGBREAK, SIG_DFL);
#endif // PLATFORM_UNIX
	if (theHost) {
		theHost->CloseHostFiles();
	}
	delete theHost;
	theHost = NULL;
}

#endif // GALACTICA_SERVER

