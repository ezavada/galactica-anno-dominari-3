//   GalacticaConsts.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#ifndef GALACTICA_CONSTS_H_INCLUDED
#define GALACTICA_CONSTS_H_INCLUDED

#include <DebugMacros.h>
#include <PP_Messages.h>
#include <PP_Resources.h>


// comment out line below to remove registration checks
#define CHECK_REGISTRATION
#define REGISTRATION_NOT_YET_DELAY 420
//#define CHECK_FOR_HACKING

// comment out line below to remove code specific to version for Monkey Byte Publishing
#define MONKEY_BYTE_VERSION

// if SINGLE_PLAYER_UNHOSTED is set, then a single player game will not create
// a database or a hidden host.
#define SINGLE_PLAYER_UNHOSTED

// **** NOTE: Some debugging related compiler defines are in "GalacticaUtils.h" *****
// some debugging compiler variables have been moved to "Galactica_DebugHeaders.pch++" and
// "Galactica_Headers.pch++"

// ADDITIONAL DEBUG MACROS

#define DEBUG_DATABASE     (1L<<2)   // values 1 and 2 are used to define the debug level
#define DEBUG_COMBAT       (1L<<3)
#define DEBUG_MESSAGE      (1L<<4)
#define DEBUG_EOT          (1L<<5)
#define DEBUG_MOVEMENT     (1L<<6)
#define DEBUG_CONTAINMENT  (1L<<7)
#define DEBUG_USER         (1L<<8)
#define DEBUG_OBJECTS      (1L<<9)
#define DEBUG_TUTORIAL     (1L<<10)
#define DEBUG_AI           (1L<<11)
#define DEBUG_PACKETS      (1L<<12)

// GAME CONSTANTS =====================================================================

#define kNumUniquePlayerColors   15
#define kAdminPlayerNum          -2    // the player number for the administrator
#define kNoSuchPlayer            -1

#define DEFAULT_MATCHMAKER_URL   "http://mm.annodominari.com:8080/GalacticaMM/GalacticaMM"
#define ADMINISTRATOR_NAME       "Administrator"
#define MAX_PLAYERS_CONNECTED    99    // max number we can enter in number of players
#define MAX_OBSERVERS_CONNECTED  20    // max number of non-participants who can be connected
#define MAX_BANNED_USERS         20    // max number of players who can be in the ban list
#define MAX_HOSTNAME_LEN         80
#define MAX_GAME_NAME_LEN        27
#define MAX_TEXT_MESSAGE_LEN     256
#define MAX_PLAYER_NAME_LEN      32
#define MAX_PASSWORD_LEN         8     // if you change this you will break legacy game file compatibility
#define MAX_STAR_LANES           5
#define MAX_STAR_SETTINGS        3
#define MAX_LINKED_SLIDERS       10
#define MAX_OWNER_BACKDROPS      7
#define NUM_SHIP_HULL_TYPES      4     // number of basic ship types as shown in "Build" menu
#define MAX_BATTLESHIP_PICTS     8     // number of battle ship types for which we have special images
#define PIXELS_PER_SECTOR        1024L // at 1:1 zoom   ¥¥¥get rid of this when coord are fixed

#define TICKS_PER_SELECTION_MARKER_UPDATE    8UL
#define INITIAL_THINGY_BUFFER_SIZE           2048L   // start with 2k read/write buffer

// used as keys for preferences
#define GALACTICA_PREF_PLAY_SOUND				"PlaySound"
#define GALACTICA_PREF_PLAY_MUSIC				"PlayMusic"
#define GALACTICA_PREF_DEFAULT_NAME				"DefaultName"
#define GALACTICA_PREF_FULL_SCREEN				"Fullscreen"
#define GALACTICA_PREF_OLD_REGISTRATION			"Registration"
#define GALACTICA_PREF_REGISTRATION				"Blob"
#define GALACTICA_PREF_REG_BACKUP				"Blob2"
#define GALACTICA_PREF_PREFS_VERS				"PrefsVers"
#define GALACTICA_PREF_MAP_SIZE					"MapSize"
#define GALACTICA_PREF_MAP_DENSITY				"MapDensity"
#define GALACTICA_PREF_AI_SKILL					"AiSkill"
#define GALACTICA_PREF_AI_ADVANTAGE				"AiAdvantage"
#define GALACTICA_PREF_FAST_GAME				"FastGame"
#define GALACTICA_PREF_FOG_OF_WAR				"FogOfWar"
#define GALACTICA_PREF_START_ACTION				"StartAction"
#define GALACTICA_PREF_SHOW_SPLASH				"ShowSplash"
#define GALACTICA_PREF_SHOW_COURSES				"ShowCourses"
#define GALACTICA_PREF_SHOW_SHIPS				"ShowShips"
#define GALACTICA_PREF_SHOW_NAMES				"ShowNames"
#define GALACTICA_PREF_SHOW_GRIDLINES			"ShowGridlines"
#define GALACTICA_PREF_SHOW_NEBULA				"ShowNebula"
#define GALACTICA_PREF_SHOW_RANGES				"ShowRangeCircles"
#define GALACTICA_PREF_SHOW_COURIERS			"ShowCourierCourses"
#define GALACTICA_PREF_DEFAULT_BUILD			"DefaultShipBuildType"
#define GALACTICA_PREF_DEFAULT_GROWTH			"DefaultGrowth"
#define GALACTICA_PREF_DEFAULT_TECH				"DefaultTech"
#define GALACTICA_PREF_DEFAULT_SHIPS			"DefaultShips"
#define GALACTICA_PREF_FLEETS_WIND_HEIGHT		"FleetsWindHeight"
#define GALACTICA_PREF_FLEETS_WIND_XPOS			"FleetsWindXPos"
#define GALACTICA_PREF_FLEETS_WIND_YPOS			"FleetsWindYPos"
#define GALACTICA_PREF_FLEETS_WIND_SORTBY		"FleetsWindSortBy"
#define GALACTICA_PREF_SYS_WIND_HEIGHT			"SysWindHeight"
#define GALACTICA_PREF_SYS_WIND_XPOS			"SysWindXPos"
#define GALACTICA_PREF_SYS_WIND_YPOS			"SysWindYPos"
#define GALACTICA_PREF_SYS_WIND_SORTBY			"SysWindSortBy"



// used to determine ID#s for special records in the database
#define GAME_INFO_REC_ID         1
#define PLAYER_INFO_REC_ID       GAME_INFO_REC_ID + 1
#define NUM_PLAYER_INFO_DB_RECS  2

// these sizes are actually 1/2 the number of pixels for the width or height
#define FRAME_SIZE_DEFAULT       10
#define FRAME_SIZE_RENDEZVOUS    12
#define HOTSPOT_SIZE_DEFAULT     7
#define HOTSPOT_SIZE_SHIP        4
#define HOTSPOT_SIZE_FLEET       5
#define HOTSPOT_SIZE_RENDEZVOUS  10

#define BASE_REC_ID(playerNum)      (playerNum-1)*NUM_PLAYER_INFO_DB_RECS + PLAYER_INFO_REC_ID
#define PUBLIC_REC_ID(playerNum)    BASE_REC_ID(playerNum)
#define PRIVATE_REC_ID(playerNum)   BASE_REC_ID(playerNum)+1

#define kDBNeverTimeout          -1       // will try to connect forever
#define kDBDefaultTimeout        3600     // one minute default timeout (ticks)
#define kDBNoRetry               0        // will try only once to connect

#define GALACTICA_CLIENT_PING_INTERVAL	60000UL  // client pings host once per minute

#define kPingMatchMakerInterval  10       // ping matchmaker every 10 seconds
#define kCheckGameStateInterval  5        // wait 5 seconds between checks

#define version_IndexFile              0x0201d001  // version 2.1d1
#define version_GameFile               0x0201d001  // version 2.1d1
#define version_SavedTurnFile          0x0203a002  // version 2.3a2

// supported legacy saved turn file
#define version_v12b10_SavedTurnFile   0x0102b00A  // version 1.2b10
#define version_v21d1_SavedTurnFile    0x0201d001  // version 2.1d1
#define version_v21b9_SavedTurnFile    0x0201eb9d  // version 2.1b9r13
//#define version_v23a2_SavedTurnFile    0x0203a002  // version 2.3a2

#define kRefresh     true
#define kDontRefresh false

const UInt8    char_NonBreakingSpace   = 0xCA;

const ResType  type_EventsRes       = 'Evnt';   // resource that describes event sounds, messages and pictures

const OSType   type_Creator         = 'TrŽ5';
#define TYPE_CREATOR                  "TrŽ5"

const OSType   type_PrefsFile       = 'pref';
const OSType   type_ArtFile         = 'rsrc';
const OSType   type_TutorialFile    = 'tutl';
const OSType   type_MIDIMusicFile   = 'MooV';
const OSType   type_SavedTurnFile   = 'sgam';   // this is just a single turn in a multi-player game
const OSType   type_SavedGameFile   = 'game';   // this is the entire game in a single player game
const OSType   type_GameDataFile    = 'DatF';
const OSType   type_GameIndexFile   = 'IdxM';
const OSType   type_GameOtherIndexFile    = 'IdxF';
const OSType   type_UnhostedGameDataFile  = 'data';   //used to indicate that the host has closed
const OSType   type_UnhostedGameIndexFile = 'indx';

const long  thingyType_Star         = 'star';
const long  thingyType_Ship         = 'ship';
const long  thingyType_Fleet        = 'flet';
const long  thingyType_Wormhole     = 'worm';
const long  thingyType_StarLane     = 'lane';
const long  thingyType_Rendezvous   = 'meet';
const long  thingyType_Message      = 'msg ';
const long  thingyType_DestructionMessage = 'msgD';

#define IS_ANY_MESSAGE_TYPE(type) ( ((type) & 'msg\0') == 'msg\0')

// RESOURCE IDs =====================================================================


// interactive document windows from PPob resources
const ResIDT   window_StarMap          = 1;   // main game window
#ifdef GAME_CONFIG_HOST
const ResIDT   window_GameConfigHost   = 2;   // used only in the config game host application
#endif
const ResIDT   window_HostWindow       = 3;   // host window

// non-interactive windows from PPob resources, these open and close automatically
const ResIDT   window_WaitingDB        = 5;   // while waiting for database access
const ResIDT   window_BuildingGalaxy   = 6;   // while galaxy is being built
const ResIDT   window_ReadingGalaxy    = 7;   // while galaxy is being read
const ResIDT   window_PostingGalaxy    = 8;   // while charges are being posted
const ResIDT   window_BeginNewTurn     = 16;   // flashed at begining of NEW turn

// movable modal alerts from PPob resources
const ResIDT   window_EnterName        = 4;   // asking for your name in a single player game
const ResIDT   window_GameJoin         = 9;    // joining multiplayer game
const ResIDT   window_Message          = 10;   // messages
const ResIDT   window_ChangePlayer     = 11;   // admin changing player info in multiplayer game
const ResIDT   window_MissedTurns      = 12;   // missed turn in multi-player game
const ResIDT   window_AskSkipPlayer    = 13;   // admin end turn now and skip this player?
const ResIDT   window_EndTurnEarly     = 14;   // moves remaining, end turn anyway?
const ResIDT   window_Chat             = 15;   // chat window for in game player to player chat
const ResIDT   window_DuplicateName    = 17;   // player already logged in, login again?
// removed: const ResIDT   window_OverrideLogin      = 18;   // player already logged in, login again?
const ResIDT   window_GameNotSaved     = 19;   // save before closing/quitting?
const ResIDT   window_ConfirmRevert    = 20;   // revert to last saved version?
// removed:: const ResIDT   window_GameIsAllFull      = 21;   // game cannot accept more players
const ResIDT   window_AboutBox         = 22;   // about box
const ResIDT   window_SendMessage      = 24;   // enter message to send to another player
const ResIDT   window_StarSystems      = 23;   // list for managing all star systems you own
const ResIDT   window_ComparePlayers   = 25;   // player comparision window
const ResIDT   window_RenameItem       = 26;   // rename a fleet, ship or star
const ResIDT   window_CouldntDelete    = 27;   // tried to replace game that is in use
const ResIDT   window_PlayerDisconnected  = 28;   // another player was disconnect from multiplayer game
const ResIDT   window_PlayerConnected  = 29;   // another player joined the multiplayer game
const ResIDT   window_Fleets           = 30;   // list of fleets and their info
const ResIDT   window_EventReport      = 31;   // floating window with events for turn 
const ResIDT   window_IntroBox         = 32;   // intro splash screen
const ResIDT   window_HostNewGame      = 33;   // New multiplayer game setup window
const ResIDT   window_NewGame          = 34;   // New single player game setup window
const ResIDT   window_NameNewItem      = 35;   // Give a name to a newly aquired item
const ResIDT   window_Preferences      = 36;   // user configurable game options
const ResIDT   window_ConnectToGame    = 37;   // Join a game by connecting to a running host
const ResIDT   window_EnterIPAddr      = 38;   // manually enter an IP address or hostname
const ResIDT   window_GameHostReplaced = 39;   // old game is not being hosted, delete the reference?
const ResIDT   window_Register         = 1000;   // shareware plea and registration info
const ResIDT   window_Unregistered     = 1001;   // crippleware notice
const ResIDT   window_RegisterMethod   = 1002;   // shareware plea and registration info
#ifdef MONKEY_BYTE_VERSION
const ResIDT   window_OrderForm        = 1003;   // order form to mail in for Monkey Byte
#endif
const ResIDT   window_ReRegisterOld    = 1004;   // upgrade notice

const ResIDT   window_SetPopulation    = 1300;   // used only in the config game host application
const ResIDT   window_SetTechLevel     = 1301;   // used only in the config game host application
const ResIDT   window_SetOwner         = 1302;   // used only in the config game host application
const ResIDT   window_ChooseNewShipType   = 1303;   // used only in the config game host application

// PPob resources that are used for panels and button bars
const ResIDT   panel_Empty             = 2000;   // panel with nothing in it, for when no item selected
const ResIDT   panel_StarInfo          = 2010;   // star information, first of the panels for when star selected
const ResIDT   panel_ShipInfo          = 2020;   // ship info, first of panels for when ship selected
const ResIDT   panel_FleetInfo         = 2030;   // fleet info, first of panels for when fleet selected

const ResIDT   buttonbar_Empty         = 2500;   // button bar used when no item selected
const ResIDT   buttonbar_Star          = 2501;   // button bar used when a star is selected
const ResIDT   buttonbar_Ship          = 2502;   // button bar used when a ship selected
const ResIDT   buttonbar_Fleet         = 2503;   // button bar used when a ship selected

// model alerts from PPob resources that are used for messages
// these appear in the Galactica Art file
const ResIDT   window_BuiltNewClass    = 4000;   // built new starship class notice
const ResIDT   window_InventedNewClass = 4001;   // invented new starship drive notice
const ResIDT   window_YouWin           = 4002;   // player wins notification
const ResIDT   window_YouLose          = 4003;   // player loses notification

// PPob resources for tutorial, found in Galactica Tutorial file
const ResIDT   window_Tutorial         = 7000;   // main window used in tutorial


// non-movable modal alerts defined by ALRT resources
const ResIDT   alert_DeathBy68000         = 128;   // can't run on a 68000 machine
const ResIDT   alert_FileDamaged          = 129;   // file is damaged
const ResIDT   alert_Error                = 130;   // generic error occured message
const ResIDT   alert_FileWrongVers        = 131;   // can't open this file version
const ResIDT   alert_CouldntFindRegister  = 132;   // Register application not in Galactica Folder
const ResIDT   alert_CouldntStartRegister = 133;   // Register app refused to launch (insuf. mem?)
const ResIDT   alert_CouldntLaunchBrowser = 134;   // web browser refused to launch (insuf. mem?)
const ResIDT   alert_CouldntFindDocs      = 135;   // missing docs.html file
const ResIDT   alert_GenericErrorAlert    = 136;   // use ParamText to provide the message

// menu ids
const ResIDT   MENU_Window       = 134;
const ResIDT   MENU_File         = 129;
const ResIDT   MENU_FileOSX      = 142;
const ResIDT   MENU_Goto         = 133;
const ResIDT   MENU_Game         = 137;
const ResIDT   MENU_Message      = 135;
const ResIDT   MENU_GameConfig   = 1300;
const ResIDT   MENU_Debug        = 9000;

// additional resource types
#define CURS_CrossHair           128
#define CURS_Target              129
#define CURS_AnimatedBusy        130
#define CURS_Hand                140
#define CURS_X                   141
#define CURS_MenuHand            142
#define CURS_AnimatedTarget      150      // CURS 150 ... 157

#define Evnt_EventSetup          128      // defines how all events are displayed

#define Layr_StardockBase        300      // for items displayed at a stardock
#define Layr_LabBase             400      // for items displayed in the laboratory
#define Layr_ViewerBase          1000   // for items to go in detail view at top left
#define LayrID_PlayerOffset      100      // multiply by (playerNum -1) and add to base id
#define LayrID_ButtonBarOffset   1000   // multiply by button bar num and add to base id
#define LayrID_UnknownTypeOffset 99     // added in for unknown item type

#define pltt_DefaultColors       128

#define PICT_StarmapBackground	 129
#define PICT_Explosions          1000
#define PICT_StarGrid            1118

#define snd_FIRST                1000
#define snd_Explosion            1000
#define snd_Attention            1001
#define snd_Warning              1002
#define snd_Message              1003
#define snd_YouLose              1004
#define snd_YouWin               1005
#define snd_PlayerDied           1006
#define snd_WonBattle            1007
#define snd_LostBattle           1008
#define snd_WonPlanet            1009
#define snd_LostPlanet           1010
#define snd_NewColony            1011
#define snd_NoMore               1012
#define snd_LAST                 1012

#define snd_TutorialAttention    7000   // only appears in the tutorial plug-in


#define STR_SaveNewGalaxy           4
#define STR_MusicFileName           128
#define STR_PrefsFileName           129
//#define STR_RegisterAppName         130
#define STR_AGTutorialName          131   // may vary with Module
//#define STR_AGHelpName              132   // may vary with Module
#define STR_ArtFileName             133
#define STR_DocsFileName            134
#define STR_ArtHiResFileName        135
#define STR_GalacticaFontFileName   136
#define STR_MatchMakerURL           137
#define STR_RegisterOnlineURL       138
#define STR_GalacticaFontName       139
#define STR_UpdateNowURL			140


#define STRx_Status        128
#define str_Idle              1
#define str_ProcessingComp    2
#define str_ProcessingEOT     3
#define str_CheckingEOT       4
#define str_ReadingData       5

#define STRx_Errors        129
#define str_HostDisconnect    10
#define str_ErrorOccured      11

#define STRx_General       130
#define str_Turn              1
#define str_Host              2
#define str_Days              3
#define str_TimeLeft          4
#define str_At_               5
#define str_DebrisAt_         6
#define str_Star_             7
#define str_Ship_             8
#define str_Fleet_            9
#define str_Wormhole_         10
#define str_Unknown_          11
#define str_viaStarlane       12
#define str_Waiting           13
#define str_Paused            14
#define str_Posted            15
#define str_Quitting          16
#define str_Closing           17
#define str__on_patrol        18
#define str_Rendezvous_       19
#define str_Group             20
#define str_New               21
#define str_Full              22
#define str_Open              23
#define str_Join              24
#define str_Rejoin            25
#define str_checking          26
#define str_NotFound          27
#define str_Yes               28
#define str_No                29
#define str_Defeated          30
#define str_Online            31
#define str_Offline           32
#define str_Update            33

#define STRx_Messages      131
#define str_MsgRepeatsUntil      1
#define str_AllPlayers           2
#define str_ChattingWith         3
#define str_To                   4
//#define str_PlayerDeathNotice    5
//#define str_PlayerVictoryNotice  6

#define STRx_Names         132
#define str_Computer          1
#define str_Uncontrolled      2
#define str_You               3   // used for Events with the reader as the direct object
#define str_ReportSyntax      4   // syntax description for comp name in report, where 
#define str_TitleSyntax       5   // % is a word from STR# 136, and * is the homeworld name
#define str_Satellites        6   // the name of the fleet that holds your satellites

#define STRx_IntroAbout    133      // strings used in the intro and about boxes
#define str_By                1
#define str_Version           2

#define STRx_NameYourNew   135      // used in the "Please name your new ..." dialog.
#define str_Starship          1
#define str_Fleet             2
#define str_Colony            3
#define str_RendezvousPt      4

#define STRx_CompNames     136      // used for computer names in single player games
// has names such as Faction, Alliance, Pact, etc..

#define STRx_SkillLevels   137      // strings for mapping skill level to human readable form
#define str_Expert            1
#define str_Good              2
#define str_Average           3
#define str_Passable          4
#define str_Novice            5

#define STRx_OrderForm     138
#define str_NotApplicable     1
#define str_CurrencySign      2
#define str_CurrencyDecimal   3
#define str_EnterQuantity     4
#define str_CopyOf            5
#define str_CopiesOf          6
#define str_RegCode           7
#define str_RegCodes          8
#define str_For               9
#define str_ProductWillBe     10
#define str_ShippedOnCD       11
#define str_ShippedOnCDs      12
#define str_EmailedNoCD       13
#define str_NeedEmail         14
#define str_NeedShipTo        15
#define str_NeedCardHolder    16
#define str_NeedCardNumber    17
#define str_NeedCardExpDate   18
#define str_CAStateCode       19
#define str_CAResident        20
#define str_CANonResident     21
#define str_ShipMethodError   22


#define STRx_RedoGame      154
#define STRx_UndoGame      155
#define str_CourseChange      1
#define str_ResourceAlloc     2

//      STRx_Standards      200      // already defined in PP_Resources.h

//      STRx_Panels         300      // used in resources, but not refered to by code

#define STRx_StarNames     400
#define STRx_ShipNames     401
#define STRx_StarNumbers   402

#define STRx_Events        500

#define STRx_PanelNames    2000
#define STRx_ViewNames     2001

#define STRx_StarmapHelp      5000
#define str_ShipTypeMenuHelp     65
#if BALLOON_SUPPORT
   #define STRx_GameSetupHelp    5010
   #define STRx_MenuHelp         5020
   #define str_GalacticHelp         1
   #define str_EnableFlybyHelp      53
   #define str_DisableFlybyHelp     54
   #define str_GalacticaTutorial    55
   #define STRx_MenuHelpMore     5021
#else
   #define STRx_MenuHelp         5020
   #define str_GalacticaTutorial    1
#endif // BALLOON SUPPORT 

#define STRx_ShipHelp         6000   // all the Thingy Help STR# resources have the same
#define STRx_StarHelp         6001   // substrings. The substrings are listed below
#define STRx_FleetHelp        6002
#define STRx_StarlaneHelp     6003
#define STRx_WormholeHelp     6004
#define STRx_RendezvousHelp   6005

// these strings appear in the Galactica Tutorial file
#define STRx_TutorialStrs     7000
#define str_TutorialTitle        1   // just says "Galactica Tutorial"
#define STRx_TutorialBtns     7001   // names of buttons on tutorial page, blank means no button
#define str_unowned              1   // displays this string if unowned
#define str_otherOwned           2   // displays this the thingy is owned but not by the player
#define str_playerOwned          3   // displays this when the player owns the thingy
#define str_normal               4   // added to ownership str when star is not hilited or selected
#define str_selected             5   // adds this to the ownership string when selected
#define str_hilited              6   // adds this to the ownership string when hilited



// PANE IDs =====================================================================

// ids for main game window (some used in host game window too)
#define view_StarMap          'smap'   // view in which star map appears
//#define button_StarLane       100      // button for toggling on and off the starlanes
//#define button_Ship           101      // button for toggling on and off the ships
//#define button_Names          102      // button for toggling on and off the names
//#define button_Grid           103      // button for toggling on and off the grid
//#define button_ToggleBalloons 104      // button for toggling on and off the balloon help
#define button_EndTurn        105      // button for ending turn
#define caption_ViewName      3        // caption pane used for names of little view
#define layerView_Viewer      'vwof'   // little viewer in which all ship picts shown
#define caption_EventText     'evnt'   // shows text over top of pictures
#define pict_Dimmer           'dim '   // used to dim pictures of ships
#define view_BaseButtonBar    2500     // button bar base number
#define ppobview_ButtonBar    'Butn'   // LPPobView for main button bar
#define caption_PanelName     4        // caption pane used for names of panels
#define panel_BasePanel       2000     // CBoxView with nothing in it
#define ppobview_Panel        'Panl'   // LPPobView for control panels
#define caption_TimeDisplay   8        // displays time remaining in turn
#define caption_HelpLine      9        // shows a single line of text as help
#define caption_TurnNum       5        // used in both host and game windows
#define popup_ShipType        4200     // ship type being build at a star

// ids for items that appear only in host game window
#define caption_NumHumans     7        // used in host doc & Join Game
#define caption_NumComps      4        // used in host doc & Join Game
#define caption_NumThings     2        // used in host doc
#define caption_HostMessage   6        // used in host doc for messages

// pane ids for panels
#define pane_Scroller         100      // any scroller in a panel should have pane id 100
#define kSliderLockIDOffset   500      // for any given slider, the CBoxView will attempt to look for
                           // a ToggleButton with the slider's id + kSliderLockIDOffset
                           // and used that button (if found) to toggle the slider locking

//pane ids for New Game Windows (single and multiplayer)
#define button_NewGameBegin            90
#define button_NewGameCancel           91
#define editText_GameTitle             1
#define editText_NumHumans             2
#define editText_NumComputers          3
#define popupMenu_CompSkill            4
#define popupMenu_CompAdvantage        5
#define checkbox_FastGame              6
#define checkbox_FogOfWar              7
#define popupMenu_Density              8
#define popupMenu_SectorSize           9
#define popupMenu_TimeUnits            10
#define checkbox_LimitTurns            11
#define editText_MaxTurnLength         12
#define checkbox_EndEarlyIfAllPosted   13
#define view_NumPlayers                20
#define view_CompOppponents            21
#define view_GameOptions               22
#define view_TimeLimit                 23

// pane ids for Change Player Settings window
#define popupMenu_Player               1
#define checkbox_Human                 2
#define checkbox_Assigned              3
#define checkbox_Alive                 4
#define button_MakeHuman               5
#define button_MakeComputer            6
#define button_KillPlayer              7
#define button_RemovePlayer            8
#define button_RenamePlayer            9
#define button_Cancel                  10

// pane ids for Message Windows
#define textEdit_MessageText           90   // main text of messages
#define button_Reply                   2   // reply button in Message window

#define layeredView_MessagePict        10   // CLayeredOffscreenView for main image

// pane ids for login window
#define editText_LoginName             1

// pane ids for Join Game window
#define caption_GameName               1
#define caption_GameStatus             2
#define caption_SectorInfo             3
//#define caption_NumComps               4  // already defined above
#define caption_CompSkill              5
#define caption_CompAdvantage          6
//#define caption_NumHumans              7  // already defined above
#define caption_TurnLimit              8
#define caption_CurrTurn               9
#define caption_TurnEnds               10
#define caption_FastGame               11
#define caption_FogOfWar               12
#define listbox_GameServers            50
#define button_EnterIPAddress          100
#define button_DeleteEntry             101
#define button_SearchForInternetGames  2000

// pane ids for choose player window
//#define editText_LoginName             1  // already defined above
#define editText_Password              2
#define checkbox_SavePassword          3
#define listbox_Players                50

// pane ids for Event Report floating window
#define table_Events      100         // table that shows the events in the needy list

// pane ids for Registration window
#define editText_RegCode   100
#define button_Register    1
#define button_NotYet      2
#define button_EnterCode   3
#define view_ButtonHolder  10         // view that contains the buttons for refresh purposes

// pane ids for Download progress window
#define progressBar_ThingyProgress     10

// pane ids for chat window
//#define popupMenu_Player       1     // already defined
#define textEditView_ChatHistory 100   // the scrolling history
#define editField_ChatText       101   // the edit field you enter the chat message to send
#define button_Send              102   // the hidden button for sending the message

// COMMANDS & MESSAGES =====================================================================

// used only by the Game Config Host
const MessageT cmd_SetPopulation    = 1300;
const MessageT cmd_SetTechLevel     = 1301;
const MessageT cmd_SetOwner         = 1302;
const MessageT cmd_NewShipAtStar    = 1303;
const MessageT cmd_ChangeSettings   = 1304;
const MessageT cmd_ChangePlayer     = 1305;
const MessageT cmd_EndTurnNow       = 1306;
const MessageT cmd_PauseResume      = 1307;

const MessageT cmd_EndTurn          = 4000;  //'EndT'   // cmd-T   // I changed these to numbers
const MessageT cmd_LoginAs          = 4001;  //'Logn'   // cmd-L   // just for you, Jamie  ;-)
const MessageT cmd_OpenAsHost       = 4002;  //'Open';
const MessageT cmd_DoNewGame        = 4003;  //'NwGm'   // cmd-N
const MessageT cmd_GotoNext         = 4004;  //'Next'   // cmd-G
const MessageT cmd_GotoCurr         = 4005;  //'Jump'   // cmd-J
const MessageT cmd_GotoHome         = 4006;  //'Home'   // cmd-H
const MessageT cmd_GotoEnemyMoves   = 4007;  //'Enem';
const MessageT cmd_ShowMessages     = 4008;  //'SMsg'
const MessageT cmd_HoldMessages     = 4009;  //'HMsg'
const MessageT cmd_SendMessage      = 4010;  //'Msg '   // cmd-M
const MessageT cmd_ComparePlayers   = 4011;            // cmd-P
const MessageT cmd_RenameItem       = 4012;            // cmd-R
const MessageT cmd_ZoomIn           = 4013;
const MessageT cmd_ZoomOut          = 4014;
const MessageT cmd_ZoomFill         = 4015;
const MessageT cmd_FullScreen       = 4016;            // cmd-B
const MessageT cmd_Sound            = 4017;
const MessageT cmd_ShowHideEvents   = 4018;            // cmd-E
const MessageT cmd_SwitchLanguages  = 4019;
const MessageT cmd_HostNewGame      = 4020;
const MessageT cmd_JoinGame         = 4021;
const MessageT cmd_NewRendezvous    = 4022;            // cmd-U
const MessageT cmd_ShowAllCourses   = 4023;
const MessageT cmd_Delete           = 4024;      // set when delete key hit
const MessageT cmd_ShowSystems      = 4025;
const MessageT cmd_ShowFleets       = 4026;

const MessageT cmd_DisplayNone      = 4100;
const MessageT cmd_DisplayOwner     = cmd_DisplayNone;
const MessageT cmd_DisplayProduct   = 4101;
const MessageT cmd_DisplayTech      = 4102;
const MessageT cmd_DisplayDefense   = 4103;
const MessageT cmd_DisplayDanger    = 4104;
const MessageT cmd_DisplayGrowth    = 4105;
const MessageT cmd_DisplayShips     = 4106;
const MessageT cmd_DisplayResearch  = 4107;

const MessageT cmd_ChangeShipType   = 4200;         // new ship type to build at a star


enum {   // commands 5000 - 5010 are sent when user clicks on button in main bar
   cmd_SetPanelFirst   = 5000,
   cmd_SetPanelSecond,
   cmd_SetPanelThird,
   cmd_SetPanelForth,
   cmd_SetPanelFifth,
   cmd_SetPanelSixth,
   cmd_SetPanelSeventh,
   cmd_SetPanelEighth,
   cmd_SetPanelNinth,
   cmd_SetPanelTenth,
   cmd_SetPanelInfo        = cmd_SetPanelFirst,
   cmd_SetPanelNav         = cmd_SetPanelSecond,
   cmd_SetPanelShips       = cmd_SetPanelSecond,
   cmd_SetPanelFleetShips  = cmd_SetPanelThird,
   cmd_SetPanelLast        = cmd_SetPanelTenth
};

enum {  // commands 5200 - 5299 are sent as control messages when a user clicks 
        // on a control in a comparison window, and are translated by UControlCommandListener
        // into commands that can be passed into ObeyCommand()
    base_SortButtonPaneId = 200,
    cmd_BaseSortCommand = 5200,
    cmd_LastSortCommand = 5299,
    num_SortCommands = cmd_LastSortCommand - cmd_BaseSortCommand
};

const MessageT cmd_TutorialOK          = 7000;         // clicked "OK" in tutorial window

const MessageT cmd_EnableFlybyHelp     = 0x4069FFFC;   // special synthetic commands that come from
const MessageT cmd_GalacticaHelp       = 0x4069FFFA;   // system's help menu
const MessageT cmd_GalacticaTutorial   = 0x4069FFF9;

const MessageT cmd_GalacticaCommunity  = 8000;      // from Apple menu
const MessageT cmd_CheckForNewVersion  = 8001;      // from Apple menu

enum {
   kEnableFlybyHelpItem    = 4,         /* help menu item number of Enable/Disable Flyby Balloons */
   kGalacticaDocsItem      = 6           /* help menu item number of Galactica Docs... */
};

//const MessageT cmd_ToggleStarLanes     = 100;
//const MessageT cmd_ToggleShips         = 101;
//const MessageT cmd_ToggleNames         = 102;
//const MessageT cmd_ToggleGrid          = 103;
const MessageT cmd_ToggleBalloons      = 104;

// these commands are all handled by the GalacticaApp and manage the registration process.
const MessageT cmd_NotYet              = -100;
const MessageT cmd_LaunchRegister      = -101;
const MessageT cmd_RegisterByMail      = -101;
const MessageT cmd_EnterCode           = -102;
const MessageT cmd_SharewareInfo       = -103;
const MessageT cmd_ChooseRegMethod     = -104;
const MessageT cmd_RegisterSecure      = -105;
const MessageT cmd_RegisterOnline      = -106;
const MessageT cmd_BoxRegistration     = -107;
const MessageT cmd_UpdateRegistration  = -108;

const MessageT msg_ClickedInMap     = 3000;
const MessageT msg_TurnEnded        = 3001;

//#define msg_CellSelected      2000   these are reserved
//#define msg_CellDoubleClicked   2001


#define msg_SliderMoved          1000
#define msg_SetSlidersEven       1001
#define msg_AdjustSliderLock     1002
#define msg_SlidersReset         1003

#define compSkill_Expert         1L
#define compSkill_Good           2L
#define compSkill_Average        3L
#define compSkill_Passable       4L
#define compSkill_Novice         5L

// OTHER =====================================================================

enum ESpending {
   spending_Growth = 0,
   spending_Ships,
   spending_Research,
   
   spending_Last = spending_Research
};

enum EDisplayMode {
   displayMode_Owner = 0,
   displayMode_Product = 1,
   displayMode_Tech,
   displayMode_Defense,
   displayMode_Danger,
   
   displayMode_Spendings,
      displayMode_Growth = displayMode_Spendings + spending_Growth,
      displayMode_Ships = displayMode_Spendings + spending_Ships,
      displayMode_Research = displayMode_Spendings + spending_Research,
      
   displayMode_NumOfModes,
   displayMode_Last = displayMode_Research
};


enum EAction {
   action_Added   = 0,
   action_Deleted = 1,
   action_Updated = 2,
   action_Message = 3
};

enum EGameState {
   state_NormalPlay = 0,
   state_GameOver = 1,   // this appears in the turn the game ended, then is cleared
   state_PostGame = 2,   // the game ended, but someone is continuing to play
   state_FullFlag = 0x08       // the game has all human players assigned
};

enum EShipClass {
   class_Satellite = 0,
   class_Scout,
   class_Colony,
   class_Fighter,
   class_Fighter2,
   class_Fighter3,
   class_Fighter4,
   class_Fighter5,
   class_Fighter6,
   class_Fighter7,
   class_Fighter8,
   class_Fighter9,
   class_Fighter10,
   class_Fighter11,
   class_Fighter12,
   class_Fighter13,
   class_Last = class_Fighter13
};

// if you add new destruction events, make sure you 
// update the IsDestructionEvent() function in CMessage.cp
enum EEventCode {
   event_None = 0,
   event_ColonyCaptured,   // 1
   event_DefendersLost,    // 2   - destruction (see comment above)
   event_ShipLost,         // 3   - destruction
   event_FleetLost,        // 4   - destruction
   event_FleetDamaged,     // 5   - destruction
   event_PlanetDamaged,    // 6   - destruction   // new to v1.2b10
   event_Message,          // 7
   event_ShipCallin,       // 8
   event_FleetCallin,      // 9
   event_BuiltShip,        // 10
   event_BuiltNewClass,    // 11
   event_InventNewClass,   // 12
   event_NewTechLevel,     // 13
   event_NewColony,        // 14
   event_NewFleet,         // 15
   event_FleetFormed,      // 16
   event_PlayerDied,       // 17
   event_PlayerWins,       // 18
   event_PlayerSurrenders, // 19   // events 19 to 22 are new to v1.2b10
   event_ShipDamageFixed,  // 20
   event_FleetDamageFixed, // 21
   event_RenameColony,     // 22
   event_Last = event_RenameColony
};

enum EContentTypes {
   contents_Any = 0,       // holds any sublists at all
   contents_Ships,         // has a sublist of ships at/in it
   contents_Stars,         // has a sublist of stars (currently only used to identify stars by type)
   contents_Course,        // has a sublist of waypoints along a course for movement
   contents_SatelliteFleet // a special type of Ship that is always at the top of the list. This
                           // value is only meaningful to CStar::AddContent().
};


#endif // GALACTICA_CONSTS_H_INCLUDED

