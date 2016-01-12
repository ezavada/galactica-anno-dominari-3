//	GalacticaRegistration.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#ifndef NO_REGISTRATION_OBFUSCATION
	#define CMakePrefsVarAttach  		XLd56DT
	#define CLoadPrefsAttach			G5Dkjhn
	#define DisplayRegistrationWindow	dsjkadf
	#define DisplayUnregisteredWindow	af9823d
	#define CUpdateRegWin				zx349vm
	#define CCheckRegAttach				mz4mv8a
	#define CCheckTimeUpAttach			ponc381
	#define CDoShutdownAttach			zcmnj20
	#define ReRegDialogAttach			foernc4
	#define sPrefsAttach				adnoweoew
	#define sPrefsFileWasntThere		dnkjzcuif
	#define sInstalledRegistrationWind	adfjbniew
	#define sWindBeingDisplayed			adfjbziub
	#define sHoldStartup				nwernxccn
	#define sRegistrationWind			nvmretlao
	#define sInstalledUnregisteredWind	irnvzllms
	#define sOldVersionRegistered		oiweroqq
	#define sOldRegistration			zoiwopqv
	#define sRegCodePane				tyeropqx
	#define sNotYetButton				mbnzbaoq
	#define sRegisterButton				qweoeojfm
	#define sEnterCodeButton			lpfdoina
	#define sButtonHolder				xcnhgyufwl
	#define sEnableTicks				kkdafojqq
	#define sVersionCode				oiuweruzc
	#define sRegistered					cxzvmnvbs
	#define sTurnToExpireOn				hfgdjshfeq
	#define sInstalledShutdown			auafdbberr
	#define sCopyRegistered				ppafnwerhdf
#endif // NO_REGISTRATION_OBFUSCATION

#include "GalacticaConsts.h"
#include "GalacticaShared.h"

#include <LAttachment.h>
#include <LPeriodical.h>

#ifndef GALACTICA_SERVER
	#include <LWindow.h>
	#include <LControl.h>
#endif // GALACTICA_SERVER

#if COMPILER_MWERKS
// don't include any symbol information in the executable for the following 
// blocks of code
#if TARGET_CPU_PPC
#pragma traceback off
#elif TARGET_CPU_68K
#pragma macsbug off
#endif
#endif // COMPILER_MWERKS


// Yes, Jamie, all this registration code is incredibly convoluted and messy. It's that
// way on purpose to make it harder to hack the game.

typedef char RegCoreT[19];


// ----------------- MAKE IT HARD TO HACK REGISTRATION ------------------
class CMakePrefsVarAttach : public LAttachment {
public:
			CMakePrefsVarAttach();				
	virtual void	ExecuteSelf(MessageT inMessage,void *ioParam);		// Show help balloon
};

class CLoadPrefsAttach : public LAttachment {
public:
			CLoadPrefsAttach();				
	virtual void	ExecuteSelf(MessageT inMessage,void *ioParam);		// Show help balloon
	static bool sPrefsAttach;
	static bool sPrefsFileWasntThere;
};

class DisplayRegistrationWindow : public LPeriodical {
public:
			DisplayRegistrationWindow();
	virtual	void	SpendTime(const EventRecord &inMacEvent);
	static bool	sInstalledRegistrationWind;
	static bool	sWindBeingDisplayed;
	static bool sHoldStartup;
  #ifndef GALACTICA_SERVER
	static LWindow*	sRegistrationWind;
  #endif // GALACTiCA_SERVER
};

class DisplayUnregisteredWindow : public LPeriodical {
public:
			DisplayUnregisteredWindow();
	virtual	void	SpendTime(const EventRecord &inMacEvent);
	static bool	    sInstalledUnregisteredWind;
	static bool	    sOldVersionRegistered;
	static Str27	sOldRegistration;
};

class CUpdateRegWin : public LPeriodical {
public:
			CUpdateRegWin();				
	virtual	void	    SpendTime(const EventRecord &inMacEvent);
  #ifndef GALACTICA_SERVER
	static LPane*		sRegCodePane;
	static LControl*	sNotYetButton;
	static LControl*	sRegisterButton;
	static LControl*	sEnterCodeButton;
	static LView*		sButtonHolder;
  #endif // GALACTiCA_SERVER
	static unsigned long	sEnableTicks;
	bool			mCheckField;
};

#ifdef CHECK_REGISTRATION

class CCheckRegAttach : public LAttachment {
public:
			CCheckRegAttach();				
	virtual void	ExecuteSelf(MessageT inMessage,void *ioParam);
	static long		sVersionCode;
	static bool	    sRegistered;
};

class CCheckTimeUpAttach : public LAttachment {
public:
			CCheckTimeUpAttach(GalacticaShared* inGame);				
	virtual void    ExecuteSelf(MessageT inMessage,void *ioParam);
	static long     sTurnToExpireOn;
	GalacticaShared *mGame;
};

#ifdef CHECK_FOR_HACKING
class CDoShutdownAttach : public LAttachment {
public:
			CDoShutdownAttach();
	virtual void	ExecuteSelf(MessageT inMessage,void *ioParam);		// Show help balloon
	static bool	sInstalledShutdown;
	static bool	sCopyRegistered;
};

#endif // CHECK_FOR_HACKING

#endif // CHECK_REGISTRATION


#if COMPILER_MWERKS
#if TARGET_CPU_PPC
#pragma traceback reset
#elif TARGET_CPU_68K
#pragma macsbug reset
#endif
#endif // COMPILER_MWERKS


