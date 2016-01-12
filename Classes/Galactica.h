//	Galactica.h		
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#ifndef GALACTICA_H_INCLUDED
#define GALACTICA_H_INCLUDED

//#include <LListener.h>
#include "pdg/sys/platform.h"
#include <LArray.h>
#include <LPeriodical.h>
#include <LCommander.h>

#if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
    #include <UMemoryMgr.h>
#endif

#include "GalacticaTypes.h"
#include "GalacticaConsts.h"
#include "GalacticaUtils.h"

#ifndef GALACTICA_SERVER
  #include "CBalloonApp.h"

  class CMidiMovieFilePlayer;	
  class CNewGWorld;
  class CScrollWheelHandler;
#endif // GALACTICA_SERVER



class GalacticaApp 
    #ifndef GALACTICA_SERVER
        : public CBalloonApp 
    #else
        : public LCommander
    #endif // GALACTICA_SERVER
{
public:
						GalacticaApp();		// constructor registers all PPobs
	virtual 			~GalacticaApp();		// stub destructor
	
	static bool     WasAppDestoyed() { return sGalacticaAppDestroyed;}  // 2.1b9r5, so we can handle OpenPlay messages during app shutdown
    
	virtual void	Initialize();

	virtual void	TakeOffDuty();
	virtual void	PutOnDuty(LCommander* inNewTarget);
	static OSType	GetInternalFileType(FSSpec *inMacFSSpec);
	static void		SetInternalFileType(FSSpec *inMacFSSpec, OSType inFType);
	static void		GetHostFSSpecs(FSSpec &inSpec, FSSpec &outDataSpec, FSSpec &outIndexSpec);	
	static void		YieldTimeToForegoundTask();
	static void		SavePrefs();
    void            DisplayError(OSErr err);
    void            DoGameRangerCommand();

    static void     ReadHostConfigFile();
    void            InitializeGameSmith();

#ifndef GALACTICA_SERVER
// move these to document?

	static void	   ReadLoginInfo(FSSpec* inMacFSSpec, LStr255 &outName, SInt8 &outPlayerNum);
    static bool    GetLogin(std::string &outLoginInfo, bool updatePrefs = true);

	virtual Boolean	AllowSubRemoval(LCommander*	inSub);

	virtual void	ClickMenuBar(const EventRecord &inMacEvent);

	virtual void	ShowAboutBox();
	void			ShowHelpDocs();
	void			CheckForNewVersion();
	void			GoToCommunitySite();
	
#if SEND_DEBUGGING_TEXT_TO_SCREEN
	virtual void	ProcessNextEvent();
#endif
	
	virtual Boolean	ObeyCommand(CommandT inCommand, void* ioParam);	
	virtual void	FindCommandStatus(CommandT inCommand,
							Boolean &outEnabled, Boolean &outUsesMark,
							UInt16 &outMark, Str255 outName);
	virtual void	OpenDocument(FSSpec *inMacFSSpec);
	virtual void	PrintDocument(FSSpec *inMacFSSpec);
	virtual void	DoPreferences();
	static bool 	AskNewGameFile(FSSpec &outDataSpec, FSSpec &outIndexSpec, LStr255 &inDefaultName);
	bool			DoNewGame(NewGameInfoT &gameInfo);
	void            DoJoinGame();   // attempt to join a multiplayer game
	static CNewGWorld*  GetStarGrid() {return sStarGrid;}
	
	virtual void	ChooseDocument();
	bool			ChooseHostDocument(FSSpec &outFileSpec);
	virtual void	StartUp();			// overriding startup functions
	bool            IsAboutBoxActive() {return mAbout;}
	static bool     IsRightClick() {return sIsRightClick;}
	static void     SetIsRightClick(bool isRightClick) { sIsRightClick = isRightClick; }
	void			AdjustAllGameWindows();
	GalacticaDoc*	GetFrontmostGame();
#else
    // this section duplicates the key part of the run loop from the PowerPlant LApplication class
    // for use with the GALACTICA_SERVER build option
    enum	EProgramState {
    	programState_StartingUp,
    	programState_ProcessingEvents,
    	programState_Quitting
    };
    EProgramState		mState;
    void           Quit();
    void           Run();  // do our own run loop

#endif // GALACTICA_SERVER

protected:
	static bool                   sGalacticaAppDestroyed;
#ifndef GALACTICA_SERVER
	static CMidiMovieFilePlayer*  sMusicMov;
	static CNewGWorld*		      sStarGrid;	// Added 970630 JRW  Holds the grid of stars
	static bool                   sIsRightClick;  // was the last click a right click?
	bool				mAbout;
	StDeleter<CScrollWheelHandler>  mScrollWheelHandlerOwner;
#endif // GALACTICA_SERVER
    bool    mGameSmithInited;
};

#endif // GALACTICA_H_INCLUDED
