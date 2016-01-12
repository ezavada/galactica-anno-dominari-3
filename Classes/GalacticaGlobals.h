// GalacticaGlobals.h      
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#ifndef GALACTICA_GLOBALS_H_INCLUDED
#define GALACTICA_GLOBALS_H_INCLUDED

#include <cstring>
#include <string.h>
#include "pdg/sys/platform.h"

#if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
  #include <MacTypes.h>
  #include <Files.h>
#else
  #include <PortableMacTypes.h>
#endif

#include "GalacticaConsts.h"

class CWindowMenu;
class LAnimateCursor;
class LMenu;

namespace pdg {
	class ConfigManager;
}

namespace Galactica {

template <class T>
class Singleton {
public:
    static T& GetInstance() { return sInstance; }
private:
    static T sInstance;
};

class Globals;

class Globals : public Singleton<Globals> { 
public:
	// these are public for direct access to keep names having to do with registration from showing up
	// in debuggers
   bool           sRegistered;
   bool           sPrefsLoaded;
   Str27          sRegistration;
   
#if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
   // following are MacOS Specific, and should eventually be eliminated
   short          sSysVRef;
   long           sPrfsFldrID;
   OSType         sPref;
   FSSpec         sPrefsSpec;
#endif // PLATFORM_MACOS
  
protected:
   bool           sFullScreenMode;   // publicly accessable Application variables
   bool           sAdmitUnreg;
   short          sTurnsBetweenCallins;
   Str32          sDefaultPlayerName;
   CWindowMenu*   sWindowMenu;
   LMenu*         sAdminMenu;
   LAnimateCursor*   sBusyCursor;
   char           sDefaultHostname[256];
   int            sDefaultPort;
   char           sStatusHostname[256];
   int            sStatusPort;
   char           sAutoloadFile[1024];
   char           sAdminPassword[MAX_PASSWORD_LEN+1];
   Str27          sAllowedUsers[MAX_PLAYERS_CONNECTED];
   Str27          sBannedUsers[MAX_BANNED_USERS];
   int            sClientID;        // randomly generated each time a game is hosted, used to tell if
                                    // we are playing a hotseat game
    char		sTempRegCode[256];   
    char		sTempPlayerName[256];
    
    pdg::ConfigManager*	sConfigMgr;

public:
	bool		getFullScreenMode() { return sFullScreenMode; }
	void		setFullScreenMode(bool mode) { sFullScreenMode = mode; }
	bool		getPrefsLoaded() { return sPrefsLoaded; }
	bool		getAdmitUnreg() { return sAdmitUnreg; }
	void		setAdmitUnreg(bool admit) { sAdmitUnreg = admit; }
	LAnimateCursor* getBusyCursor() { return sBusyCursor; }
	void		setBusyCursor(LAnimateCursor* curs) { sBusyCursor = curs; }
	CWindowMenu*	getWindowMenu() { return sWindowMenu; }
	void		setWindowMenu(CWindowMenu* menu) { sWindowMenu = menu; }
	LMenu*		getAdminMenu() { return sAdminMenu; }
	void		setAdminMenu(LMenu* menu) { sAdminMenu = menu; }

	
	const char* getDefaultHostname() { return (const char*)sDefaultHostname; }
	void		setDefaultHostname(const char* name) { strncpy(sDefaultHostname, name, 256); sDefaultHostname[255] = 0;}
	const char* getStatusHostname() { return (const char*)sStatusHostname; }
	void		setStatusHostname(const char* name) { strncpy(sStatusHostname, name, 256); sStatusHostname[255] = 0;}
	const char* getDefaultPlayerName() { p2cstrcpy(sTempPlayerName, sDefaultPlayerName); return (const char*)sTempPlayerName; }
	StringPtr	getDefaultPlayerNamePStr() { return (StringPtr)sDefaultPlayerName; }
	void		setDefaultPlayerName(const char* name) { c2pstrcpy(sDefaultPlayerName, name); }
	const char* getAdminPassword() { return (const char*)sAdminPassword; }
	void		setAdminPassword(const char* name) { strncpy(sAdminPassword, name, MAX_PASSWORD_LEN); sAdminPassword[MAX_PASSWORD_LEN] = 0;}
	const char* getRegistration() { p2cstrcpy(sTempRegCode, sRegistration); return (const char*)sTempRegCode; }
	const char* getAutoloadFile() { return (const char*)sAutoloadFile; }
	void		setAutoloadFile(const char* name) { strncpy(sAutoloadFile, name, 1024); sAutoloadFile[1023] = 0;}

	short		getTurnsBetweenCallins() { return sTurnsBetweenCallins; }
	int 		getDefaultPort() { return sDefaultPort; }
	void		setDefaultPort(int port) { sDefaultPort = port; }
	int			getStatusPort() { return sStatusPort; }
	void		setStatusPort(int port) { sStatusPort = port; }
	int 		getClientID() { return sClientID; }
	void		setClientID(int id) { sClientID = id; }
	
	bool 		haveAllowedUsersList() { return (sAllowedUsers[0][0] != 0); }
	
	pdg::ConfigManager* getConfigManager() { return sConfigMgr; }
	void		setConfigManager(pdg::ConfigManager* configMgr) { sConfigMgr = configMgr; }
	
public:
   Globals() : sRegistered(false),
			   sPrefsLoaded(false), 
               sRegistration(),
#if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
               sSysVRef(-1), 
               sPrfsFldrID(0), 
               sPref('pref' << 1L), 
 #endif // MacOS
               sFullScreenMode(true), 
               sAdmitUnreg(true),      
               sTurnsBetweenCallins(5), 
               sDefaultPlayerName(), 
               sWindowMenu(NULL), 
               sAdminMenu(NULL),
               sBusyCursor(NULL),
               sDefaultPort(4429),
               sStatusPort(0),
               sClientID(0),
               sConfigMgr(0)
               { 
                  std::strcpy(sDefaultHostname, "127.0.0.1");
                  std::strcpy(sAutoloadFile, "Test");
                  std::strcpy(sAdminPassword, "");
                  std::strcpy(sStatusHostname, "127.0.0.1");
                  ClearAllowedAndBannedLists();
                #ifdef GALACTICA_SERVER
                  sAdmitUnreg = false;    // the Galactica server defaults to only allowing registered players
                #endif
               }

   void     ClearAllowedAndBannedLists() {
                  for (int i = 0; i < MAX_PLAYERS_CONNECTED; i++) {
                     sAllowedUsers[i][0] = 0;
                  }
                  for (int i = 0; i < MAX_BANNED_USERS; i++) {
                     sBannedUsers[i][0] = 0;
                  }
               }

   void     AddAllowedUser(const char* inUserRegCode) {
                  if (std::strlen(inUserRegCode) != 27) {
                     DEBUG_OUT("Attempt to allow invalid reg code "<<inUserRegCode, DEBUG_ERROR);
                     return;
                  }
                  for (int i = 0; i < MAX_PLAYERS_CONNECTED; i++) {
                     if (sAllowedUsers[i][0] == 0) {
                        c2pstrcpy(sAllowedUsers[i], inUserRegCode);
                        return;
                     }
                  }
                  DEBUG_OUT("No more room in allow list; Cannot allow user "<<inUserRegCode, DEBUG_ERROR);
               }

   void     AddBannedUser(const char* inUserRegCode) {
                  if (std::strlen(inUserRegCode) != 27) {
                     DEBUG_OUT("Attempt to ban invalid reg code "<<inUserRegCode, DEBUG_ERROR);
                     return;
                  }
                  for (int i = 0; i < MAX_BANNED_USERS; i++) {
                     if (sBannedUsers[i][0] == 0) {
                        c2pstrcpy(sBannedUsers[i], inUserRegCode);
                        return;
                     }
                  }
                  DEBUG_OUT("No more room in ban list; Cannot ban user "<<inUserRegCode, DEBUG_ERROR);
               }

   bool     IsUserAllowed(const char* inUserRegCode) const {
				  int i;
                  if (sAllowedUsers[0][0] != 0) {
                     // there is a list of allowed users
                     for (i = 0; i < MAX_PLAYERS_CONNECTED; i++) {
                        if (sAllowedUsers[i][0] == 0) {
                           return false;  // we reached the end of the list
                        } else {
                           char buffer[28];
                           p2cstrcpy(buffer, sAllowedUsers[i]);
                           if (std::strcmp(buffer, inUserRegCode) == 0) {
                              return true;
                           }
                        }
                     }
                     return false;
                  }
                  for (i = 0; i < MAX_BANNED_USERS; i++) {
                     if (sBannedUsers[i][0] == 0) {
                        return true;  // we reached the end of the list
                     } else {
                        char buffer[28];
                        p2cstrcpy(buffer, sBannedUsers[i]);
                        if (std::strcmp(buffer, inUserRegCode) == 0) {
                           return false;
                        }
                     }
                  }
                  return true;
               }
};

} // end namespace Galactica

#endif // GALACTICA_GLOBALS_H_INCLUDED

