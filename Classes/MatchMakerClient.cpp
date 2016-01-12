//
// MatchMakerClient.cp
//
//

#include "pdg/sys/platform.h"
#include "pdg/sys/os.h"

#include <stdlib.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <iostream>

#include "MatchMakerClient.h"
#include "GenericUtils.h"

#if defined( PLATFORM_MACOS )
	// It needs to use URL Access if it's going to run on OS 9
	#include <URLAccess.h>
	#define MM_USING_URLACCESS
#elif defined( PLATFORM_MACOSX ) && !defined( POSIX_BUILD )
	// We will use new CFNetwork stuff for OS X only builds
	#define MM_USING_CFNETWORK
	#include <Carbon/Carbon.h>
#elif
	// We will use CURL for anything else
	#define MM_USING_CURL	
	#include "curl/curl.h"
#endif


//
// MMGameList()
//
// Default constructor. Sets member variables to their default values.
//
MMGameList::MMGameList()
{
	mNumGames = 0;
	mGameList = 0;
}

//
// ~MMGameList()
//
// Iterates the game list disposing the game objects, and then frees the game list handle.
//
MMGameList::~MMGameList()
{
	freeGames();
}

//
// getGame
//
// Retrieves the game record at the index specified. If the index is out of scope, or if the
// game list is nil, this method will return nil.
//
MMGameRec* MMGameList::getGame(int index)
{
   MMGameRec*	gameRec;

	gameRec = 0;
	if ((mGameList != 0) && (index >= 0) && (index < mNumGames)) {
		gameRec = mGameList[index];
	}

	return gameRec;
}

//
// setGame
//
// index	-> position to insert the game rec
// gameRec	-> game to be set
//
// Stores the game record at the index specified.  If this call succeeds, the MMGameList object
// takes responsibility for destroying this object.
//
// throw - exception if the game list is nil or the index is out of bounds.
//
void MMGameList::setGame(int index, MMGameRec* gameRec)
{
	if ((mGameList != 0) && (index >= 0) && (index < mNumGames)) {
		mGameList[index] = gameRec;
	}
}

//
// setNumGames
//
// gameCount	-> number of games to be stored in the game list
//
// Calls freeGames to dispose of the current games and game list. It then sets the member variable
// for the game count, and allocates a game list to store the games.
//
// throw - exception if the memory to store the list can not be allocated.
//
void MMGameList::setNumGames(int gameCount)
{
	freeGames();
	
	mGameList = (MMGameRec**) malloc(sizeof(MMGameRec*) * gameCount);
	if (mGameList == 0) {
        DEBUG_OUT("malloc failed in MMGameList::setNumGames, no games will be stored", DEBUG_ERROR);
		gameCount = 0; // we couldn't allocate any games
	}
	mNumGames = gameCount;
}


//
// getNumGames
//
// return	<- number of games in list
//
// Returns the number of games stored in the list
//
long MMGameList::getNumGames()
{
	return mNumGames;
}


//
// freeGames
//
// Iterates the game list destroying the game objects and frees the game list. It then sets
// the game list member variable to nil.
//
void MMGameList::freeGames()
{
    MMGameRec*	gameRec;

	if (mGameList != 0) {
		for (int i=0; i<mNumGames ; i++) {
			gameRec = mGameList[i];
			if (gameRec != nil) {
				delete gameRec;
			}
		}
		free(mGameList);
		mGameList = nil;
	}
	mNumGames = 0;
}


//
// MatchMakerClient()
//
// Default constructor 
//
MatchMakerClient::MatchMakerClient()
{
}

//
// ~MatchMakerClient()
//
// Destructor 
//
MatchMakerClient::~MatchMakerClient()
{
}

//
// accessAvailable()
//
// return	<- true if the library is available.
//
// Used to determine if the URLAccess library is available. This object will not function
// without access to the URLAccess library.
//
bool MatchMakerClient::matchmakerAvailable()
{
#ifdef MM_USING_URLACCESS
	if ( MacAPI::URLAccessAvailable()) {
		return true;
	}

	return false;
#else
	return true;
#endif
}

//
// setMatchMakerURL()
//
// URL	-> URL of the Match Maker Server
//
// Sets the URL at which the Match Maker Server can be accessed.
//
void MatchMakerClient::setMatchMakerURL(const char* URL)
{
	if (strlen(URL) < MM_MAX_URL_LEN) {
		strcpy(mURL, URL);
	}
	else {
		throw;
	}
}

//
// DoPost()
//
// post	-> Text to be posted
// err	<- err code 
//
// Utility method for posting the string provided to the URL specified by setMatchMakerURL.  Any
// errors are returned in err.
//
// returns 0 if error occured, or a malloc'd C string (NUL terminated) if no error
//
#ifdef MM_USING_URLACCESS
char* MatchMakerClient::doPost(char* post, long &err, long timeoutMs)
{
    MacAPI::URLReference urlRef;
//    DEBUG_OUT("MMClient::DoPost ["<<post<<"]", DEBUG_TRIVIA);
    err = MacAPI::URLNewReference( mURL, &urlRef );
    if ( err != noErr) {
        DEBUG_OUT("URLNewReference failed, err ["<<err<<"]", DEBUG_ERROR);
    	return 0;
    }
    long	length = strlen(post);

	MacAPI::URLSetProperty (urlRef, (const char *)kURLHTTPRequestMethod, (void *)"POST", 4);
	MacAPI::URLSetProperty (urlRef, kURLHTTPRequestBody, (void *)post, length);

// the old way, using URLDownload to do all the work for us, but with no ability to timeout
#ifdef USE_URL_DOWNLOAD_FOR_POST
    MacAPI::Handle dataHandle = NULL;
	dataHandle = MacAPI::NewHandle(0);
	if (dataHandle == nil) {
		MacAPI::URLDisposeReference(urlRef);
		return 0;
	}
	err = MacAPI::URLDownload( urlRef, nil, dataHandle, 0, nil, nil);
	// convert handle to cstring even if there was an error so we can display an empty reply string
    length = MacAPI::GetHandleSize(dataHandle);
    ++length;
    char* p = (char*) malloc(length);
    MacAPI::HLock(dataHandle);
    memcpy(p, *dataHandle, length-1);
    p[length-1] = 0; // nul terminate the string in the data handle
    MacAPI::HUnlock(dataHandle);
	MacAPI::DisposeHandle(dataHandle);
    // check for an error and return nil if there was one
	if (err != noErr) {
        DEBUG_OUT("URLDownload failed, err ["<<err<<"] from POST ["<<post<<"] reply ["<<p<<"]", DEBUG_ERROR);
		free(p);
		return nil;
    }
#else
	// the new way, using URLOpen to start the transfer so we can manage the memory and timeout ourselves
	err = MacAPI::URLOpen(urlRef, 0, 0, 0, 0, 0);
	if (err != noErr) {
		DEBUG_OUT("URLOpen failed, err ["<<err<<"] from POST ["<<post<<"]", DEBUG_ERROR);
		MacAPI::URLDisposeReference(urlRef);
		return 0;
	}
	UInt32 startMs = pdg::OS::getMilliseconds();
	bool done = false;
	Size bufferAllocated = 64*1024;
	Size bufferRemaining = bufferAllocated;
	char* p = (char*) malloc(bufferAllocated);	// malloc a 64k block to start
	if (p == 0) {
		DEBUG_OUT("malloc failed, for POST ["<<post<<"]", DEBUG_ERROR);
		MacAPI::URLDisposeReference(urlRef);
		return 0;
	}		
	p[0] = 0;
	while (!done) {
		if ((pdg::OS::getMilliseconds() - startMs) > timeoutMs) {
        	DEBUG_OUT("timeout waiting for response to POST ["<<post<<"] reply ["<<p<<"]", DEBUG_ERROR);
			done = true;	// timeout
		} else {
			// haven't timed out yet
			// fetch any data we have
			void* bufferPtr;
			Size bufferSize;
			err = MacAPI::URLGetBuffer(urlRef, &bufferPtr, &bufferSize); // get a buffer full of data
			if ((err == noErr) && ((bufferSize + 1) > bufferRemaining)) {
				bufferAllocated += (bufferSize + 64*1024); // allocate another 64k block plus the overflow
				p = (char*) realloc( (void*)p, bufferAllocated);
				if (p == 0) {
					DEBUG_OUT("realloc failed, trying to allocated ["<<bufferAllocated<<" bytes for response to POST ["<<post<<"]", DEBUG_ERROR);
        			MacAPI::URLReleaseBuffer(urlRef, bufferPtr);
					MacAPI::URLDisposeReference(urlRef);
					return 0;
				}
			}
			if (bufferPtr) {
				strncat(p, (char*)bufferPtr, bufferAllocated); // append the new data to the existing buffer
	        	MacAPI::URLReleaseBuffer(urlRef, bufferPtr);
        	}
			MacAPI::URLIdle();
    		URLState currentState;
        	MacAPI::URLGetCurrentState(urlRef, &currentState);
        	if (currentState == kURLErrorOccurredState) {
        		MacAPI::URLGetError(urlRef, &err);
        		done = true;
        	}
	    	if ((currentState == kURLTransactionCompleteState) ||
	    	    (currentState == kURLAbortingState) ||
	    	    (currentState == kURLCompletedState) ) {
	    	    err = noErr;
	    		done = true;
	    	}
		}
	}
#endif // USE_URL_DOWNLOAD_FOR_POST	
	MacAPI::URLDisposeReference(urlRef);
	
	return p; // return whatever we got back, even if we timed out
}
#endif // MM_USING_URLACCESS


#ifdef MM_USING_CFNETWORK
char* MatchMakerClient::doPost(char* post, long &err, long timeoutMs)
{
	err = noErr;
    char* buffer = 0;
    CFHTTPMessageRef messageRef = 0;
    CFURLRef urlRef = 0;
    CFStringRef postRef = 0;
    CFDataRef postDataRef = 0;
    CFReadStreamRef readStreamRef = 0;
	Size bufferAllocated = 64*1024;
	Size bufferRemaining = bufferAllocated;
    char* p = 0;
	bool done = false;
    UInt32 startMs;
	urlRef = MacAPI::CFURLCreateWithBytes( kCFAllocatorDefault, (UInt8*)mURL, std::strlen(mURL), kCFStringEncodingASCII, NULL );
	if (urlRef == 0) {
		DEBUG_OUT("CFURLCreateWithBytes failed, for POST ["<<post<<"]", DEBUG_ERROR);
		err = -1;
		goto BAIL;
	}
	messageRef = MacAPI::CFHTTPMessageCreateRequest( kCFAllocatorDefault, CFSTR("POST"), urlRef, kCFHTTPVersion1_1 );
	if (messageRef == 0) {
		DEBUG_OUT("CFHTTPMessageCreateRequest failed, for POST ["<<post<<"]", DEBUG_ERROR);
		err = -2;
		goto BAIL;
	}
    postRef = MacAPI::CFStringCreateWithCString( kCFAllocatorDefault, post, kCFStringEncodingUTF8 );
	if (postRef == 0) {
		DEBUG_OUT("CFStringCreateWithCString failed, for POST ["<<post<<"]", DEBUG_ERROR);
		err = -3;
		goto BAIL;
	}
    postDataRef = MacAPI::CFStringCreateExternalRepresentation( kCFAllocatorDefault, postRef, kCFStringEncodingUTF8, 0);
    if (postDataRef == 0) {
		DEBUG_OUT("CFStringCreateExternalRepresentation failed, for POST ["<<post<<"]", DEBUG_ERROR);
		err = -4;
		goto BAIL;
    }
	MacAPI::CFHTTPMessageSetBody(messageRef, postDataRef);
	readStreamRef = MacAPI::CFReadStreamCreateForHTTPRequest( kCFAllocatorDefault, messageRef );
	if (readStreamRef == 0) {
		DEBUG_OUT("CFReadStreamCreateForHTTPRequest failed, for POST ["<<post<<"]", DEBUG_ERROR);
		err = -5;
		goto BAIL;
	}
	startMs = pdg::OS::getMilliseconds();
	buffer = (char*) malloc(bufferAllocated);	// malloc a 64k block to start
	p = buffer;
	if (p == 0) {
		DEBUG_OUT("malloc failed, for POST ["<<post<<"]", DEBUG_ERROR);
		err = -6;
		goto BAIL;
	}		
	p[0] = 0;
	// Start the HTTP connection
	if ( MacAPI::CFReadStreamOpen( readStreamRef ) == false ) {
        err = -8;
	    goto BAIL;
    }
	while (!done) {
		if ((long)(pdg::OS::getMilliseconds() - startMs) > timeoutMs) {
        	DEBUG_OUT("timeout waiting for response to POST ["<<post<<"] reply ["<<p<<"]", DEBUG_ERROR);
			done = true;	// timeout
		} else {

		    CFIndex numBytesRead = MacAPI::CFReadStreamRead(readStreamRef, (UInt8*)p, bufferRemaining-1);
		    if (numBytesRead < 0) {
		        CFStreamError error = MacAPI::CFReadStreamGetError(readStreamRef);
		        err = error.error;
				DEBUG_OUT("numBytesRead < 0 ["<<numBytesRead<<"] and error code ["<<err<<"] in response to POST ["<<post<<"]", DEBUG_ERROR);
                done = true;
		    } else if (numBytesRead == 0) {
		        done = true;
		    } else {
		    	bufferRemaining -= numBytesRead;
		    	p += numBytesRead;
                *p = 0; // make sure we are nul terminated
		    	if (bufferRemaining <= 1) { // we have a nul terminator at the end we want to preserve
					bufferAllocated += 64*1024; // allocate another 64k block
					buffer = (char*) realloc( (void*)buffer, bufferAllocated);
					if (buffer == 0) {
						DEBUG_OUT("realloc failed, trying to allocated ["<<bufferAllocated<<" bytes for response to POST ["<<post<<"]", DEBUG_ERROR);
						err = -10;
						goto BAIL;
					}
		    	}
		    }
		}
	}
	if (err == noErr) {	
		// get the status code
		CFHTTPMessageRef myResponse = (CFHTTPMessageRef) MacAPI::CFReadStreamCopyProperty(readStreamRef, kCFStreamPropertyHTTPResponseHeader);
        if (myResponse != 0) {
            err = MacAPI::CFHTTPMessageGetResponseStatusCode(myResponse);
            MacAPI::CFRelease(myResponse);
            if (err == 200) {	// this is actually success
                err = noErr;
            }
        }
	}
BAIL:
    if (urlRef) {
        MacAPI::CFRelease(urlRef);
    }
    if (messageRef) {
        MacAPI::CFRelease(messageRef);
    }
    if (readStreamRef) {
        MacAPI::CFRelease(readStreamRef);
    }
    if (postRef) {
        MacAPI::CFRelease(postRef);
    }
    if (postDataRef) {
        MacAPI::CFRelease(postDataRef);
    }
	return buffer;
}
#endif // MM_USING_CFNETWORK

#ifdef MM_USING_CURL
char* MatchMakerClient::doPost(char* post, long &err, long timeoutMs)
{
	err = -1;
	return 0;
}
#endif // MM_USING_CURL

//
// parseGame()
//
// game		-> string containing the XML that describes the game
// gameRec	<- gameRec object
//
// Creates an MMGameRec object and parses the XML provided to fill in the fields.
//
void MatchMakerClient::parseGame(char* game, MMGameRec* gameRec)
{
	
long	startPos, endPos;
std::string	gameStr(game);
std::string	subS;
long	len;

	// Game Name
	startPos = gameStr.find("gamename='");	
	if(startPos != -1) {
		startPos += 10;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		if(len >= (long)sizeof(gameRec->mGameName)) {
			len = sizeof(gameRec->mGameName);
		}
		strcpy(gameRec->mGameName, gameStr.substr(startPos, len).c_str());
	}
		
	// Status
	startPos = gameStr.find("status='");	
	if(startPos != -1) {
		startPos += 8;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		if(len >= (long)sizeof(gameRec->mStatus)) {
			len = sizeof(gameRec->mStatus);
		}
		strcpy(gameRec->mStatus, gameStr.substr(startPos, len).c_str());
	}
		
	// primary IP
	startPos = gameStr.find("primaryip='");	
	if(startPos != -1) {
		startPos += 11;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		if(len >= (long)sizeof(gameRec->mPrimaryIP)) {
			len = sizeof(gameRec->mPrimaryIP);
		}
		strcpy(gameRec->mPrimaryIP, gameStr.substr(startPos, len).c_str());
	}
		
	// Primary Port
	startPos = gameStr.find("primaryport='");	
	if(startPos != -1) {
		startPos += 13;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		if(len >= (long)sizeof(gameRec->mPrimaryPort)) {
			len = sizeof(gameRec->mPrimaryPort);
		}
		strcpy(gameRec->mPrimaryPort, gameStr.substr(startPos, len).c_str());
	}
		
	// Secondary IP
	startPos = gameStr.find("secondaryip='");	
	if(startPos != -1) {
		startPos += 13;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		if(len >= (long)sizeof(gameRec->mSecondaryIP)) {
			len = sizeof(gameRec->mSecondaryIP);
		}
		strcpy(gameRec->mSecondaryIP, gameStr.substr(startPos, len).c_str());
	}
		
	// Secondary Port
	startPos = gameStr.find("secondaryport='");	
	if(startPos != -1) {
		startPos += 15;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		if(len >= (long)sizeof(gameRec->mSecondaryPort)) {
			len = sizeof(gameRec->mSecondaryPort);
		}
		strcpy(gameRec->mSecondaryPort, gameStr.substr(startPos, len).c_str());
	}
		
	// Players max
	startPos = gameStr.find("playersmax='");	
	if(startPos != -1) {
		startPos += 12;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		gameRec->mPlayersMax = atol(gameStr.substr(startPos, len).c_str());
	}
		

	// Players max
	startPos = gameStr.find("playersmax='");	
	if(startPos != -1) {
		startPos += 12;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		gameRec->mPlayersMax = atol(gameStr.substr(startPos, len).c_str());
	}
	
	// Players Current
	startPos = gameStr.find("playerscurrent='");	
	if(startPos != -1) {
		startPos += 16;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		gameRec->mPlayersCurrent = atol(gameStr.substr(startPos, len).c_str());
	}
	
	// Computers
	startPos = gameStr.find("computers='");	
	if(startPos != -1) {
		startPos += 11;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		gameRec->mComputers = atol(gameStr.substr(startPos, len).c_str());
	}
	
	// Computer Skill
	startPos = gameStr.find("compskill='");	
	if(startPos != -1) {
		startPos += 11;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		gameRec->mComputerSkill = atol(gameStr.substr(startPos, len).c_str());
	}
	
	// Computer Advantage
	startPos = gameStr.find("compadvantage='");	
	if(startPos != -1) {
		startPos += 15;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		gameRec->mComputerAdvantage = atol(gameStr.substr(startPos, len).c_str());
	}
	
	// Size
	startPos = gameStr.find("size='");	
	if(startPos != -1) {
		startPos += 6;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		gameRec->mSize = atol(gameStr.substr(startPos, len).c_str());
	}
	
	// Density
	startPos = gameStr.find("density='");	
	if(startPos != -1) {
		startPos += 9;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		gameRec->mDensity = atol(gameStr.substr(startPos, len).c_str());
	}
	
	// Current Turn
	startPos = gameStr.find("currentturn='");	
	if(startPos != -1) {
		startPos += 13;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		gameRec->mCurrentTurn = atol(gameStr.substr(startPos, len).c_str());
	}

	// Fast
	startPos = gameStr.find("fast='");	
	if(startPos != -1) {
		gameRec->mFast = false;
		if(gameStr[startPos + 6] == 't') {
			gameRec->mFast = true;
		}
	}
	
	// Fog Of War
	startPos = gameStr.find("fogofwar='");	
	if(startPos != -1) {
		gameRec->mFogOfWar = false;
		if(gameStr[startPos + 10] == 't') {
			gameRec->mFogOfWar = true;
		}
	}
		
	// Time Limit In Seconds
	startPos = gameStr.find("timelimitsecs='");	
	if(startPos != -1) {
		startPos += 15;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		gameRec->mTimeLimitSecs = atol(gameStr.substr(startPos, len).c_str());
	}

	// Time Limit In Seconds
	startPos = gameStr.find("gameid='");	
	if(startPos != -1) {
		startPos += 8;
		endPos = gameStr.find("'", startPos);
		len = endPos - startPos;
		gameRec->mGameID = atol(gameStr.substr(startPos, len).c_str());
	}

	// Public game
	startPos = gameStr.find("public='");	
	if(startPos != -1) {
		gameRec->mPublic = false;
		if(gameStr[startPos + 8] == 't') {
			gameRec->mPublic = true;
		}
	}
}

//
// requestGames()
//
// gameList	<- game list of all available games
// regCode	-> registration code of this Galactica client
//
// return	<- error code
//
// Requests the list of active games from the Match Maker Server
//
long MatchMakerClient::requestGames(MMGameList* &gameList, char* regCode)
{
std::string	post("");
long	err;

	// <?XML >
	post += kXML;

	// <protocol>
	post += kProtocol;
	
	// <client>
	post += "<client>";
	
	// <registration />
	post += std::string("<registration code='") + std::string(regCode) + std::string("' />");


	// </client>
	post += "</client>";
	
	// </protocol>
	post += "</protocol>";
	
	// Post Game List Request
	err = noErr;
	char* p = doPost((char*)post.c_str(), err);
	
	if ((err != noErr) || !p) {
	    DEBUG_OUT("MatchMaker requestGames DoPost() returned error code: ["<< err<<"]", DEBUG_ERROR);
		return err;
	}

	
	std::string	str(p);
	std::string segment;
	std::string	value;
	long		startPos, endPos, cnt;
	MMGameRec*	game;
	
	
	long index;
	index = str.find("error code=");
	if (index != -1 ) {
		index += 14;
		value.assign(index,str.find(">",index)-(index-1));
		err = atol(value.c_str()) + kGalacticaMM_errorBase;
	    DEBUG_OUT("MatchMaker requestGames found error response: ["<< err<<"] from POST ["<<post.c_str()
	                <<"] with reply ["<<str.c_str()<<"]", DEBUG_ERROR);
	} else {
		cnt = 0;
		startPos = str.find("<game>");
		while(startPos != -1) {
			++cnt;
			startPos = str.find("<game>", startPos + 6);
		}

		gameList = new MMGameList();
		gameList->setNumGames(cnt);

		startPos = 0;
		endPos = 0;
		cnt = 0;
		startPos = str.find("<game>");
		while(startPos != -1) {
			endPos = str.find("</game>",startPos);
			endPos += 7;
			segment = str.substr(startPos, endPos - startPos);
			game = new MMGameRec();
			parseGame((char*)segment.c_str(), game);
			gameList->setGame(cnt, game);
			++cnt;
			startPos = str.find("<game>",endPos);
		}
	}
	
	free(p); // free the memory allocated by doPost()
	return err;

}

//
// postGame()
//
// game		-> game information to be posted
// regCode	-> registration code of this Galactica client
//
// return	<- error code
//
// Posts a game created by this game client to the Match Maker Servlet.
//
long MatchMakerClient::postGame(MMGameRec* game, char* regCode)
{
std::string	post("");
char	s[36];
long	err;


    ReplaceIllegalCharacters(game->mGameName, "\'\"\\<>", '_', kRestrictTo7BitAscii);

	// <?XML >
	post += kXML;

	// <protocol>
	post += kProtocol;
	
	// <game>
	post += "<game>";
	post += "<info gamename='";		// Game Name
	post += game->mGameName;
	post += "' status='";			// Status
	post += game->mStatus;
	post += "' primaryip='";		// Primary IP
	post += game->mPrimaryIP;
	post += "' primaryport='";		// Primary Port
	post += game->mPrimaryPort;
	post += "'/>";

	// <details>
	post += "<details playersmax='";		// Num. Players Max
    snprintf(s, 10, "%d", game->mPlayersMax);
//	_ltoa(game->mPlayersMax, s, 10);
	post += s;
	post += "' playerscurrent='";			// Num. Current Players
    snprintf(s, 10, "%d", game->mPlayersCurrent);
//	_ltoa(game->mPlayersCurrent, s, 10);
	post += s;
	post += "' computers='";				// Num. Computer Opponents
    snprintf(s, 10, "%d", game->mComputers);
//	_ltoa(game->mComputers, s, 10);
	post += s;
	post += "' compskill='";				// Computer Skill
    snprintf(s, 10, "%d", game->mComputerSkill);
//	_ltoa(game->mComputerSkill, s, 10);
	post += s;
	post += "' compadvantage='";			// Computer Advantage
    snprintf(s, 10, "%d", game->mComputerAdvantage);
//	_ltoa(game->mComputerAdvantage, s, 10);
	post += s;
	post += "' size='";						// Size
    snprintf(s, 10, "%d", game->mSize);
//	_ltoa(game->mSize, s, 10);
	post += s;
	post += "' density='";					// Density
    snprintf(s, 10, "%d", game->mDensity);
//	_ltoa(game->mDensity, s, 10);
	post += s;
	post += "' currentturn='";				// Current Turn
    snprintf(s, 10, "%d", game->mCurrentTurn);
//	_ltoa(game->mCurrentTurn, s, 10);
	post += s;
	if (game->mFast) {						// Fast
		post += "' fast='true";
	}
	else {
		post += "' fast='false";
	}
	if (game->mFogOfWar) {					// Fog of War
		post += "' fogofwar='true";
	}
	else {
		post += "' fogofwar='false";
	}
	post += "' timelimitsecs='";			// time limit seconds
    snprintf(s, 10, "%d", game->mTimeLimitSecs);
//	_ltoa(game->mTimeLimitSecs, s, 10);
	post += s;
	post += "' gameid='";			// time limit seconds
    snprintf(s, 10, "%d", game->mGameID);
//	_ltoa(game->mGameID, s, 10);
	post += s;
	if(game->mPublic) {						// Public
		post += "' public='true";
	}
	else {
		post += "' public='false";
	}
	post += "' />";
	
	// <registration>
	post += "<registration regcode='";		// registration code
	post += regCode;
	post += "' />";
	
	// </game>
	post += "</game>";
	
	// </protocol>
	post += "</protocol>";
	
	// Post Game
	err = noErr;
	char* p = doPost((char*)post.c_str(), err);
	
	if ((err != noErr) || !p) {
	    DEBUG_OUT("MatchMaker postGame DoPost() returned error code: ["<< err<<"]", DEBUG_ERROR);
		return err;
	}
	
	std::string	str(p);
	std::string value;
	
//	DEBUG_OUT("MatchMaker postGame POST reply: ["<< str.c_str()<<"]", DEBUG_TRIVIA);

	long index = str.find("success guid='");
	if(index != -1) {
		index += 14;
		value = str.substr(index,str.find("'",index)-index);
		strcpy(game->mGUID, value.c_str());
		err = noErr;
	}
	else {
		index = str.find("error code=");
		if (index != -1) {
			index += 14;
			value = str.substr(index,str.find(">",index)-index);
			err = atol(value.c_str()) + kGalacticaMM_errorBase;
		    DEBUG_OUT("MatchMaker postGame found error response: ["<< err<<"] from POST ["<<post.c_str()
		                <<"] with reply ["<<str.c_str()<<"]", DEBUG_ERROR);
	    } else {
		    DEBUG_OUT("MatchMaker postGame couldn't understand reply ["<< str.c_str()<<"] from POST ["<<post.c_str()
		                <<"]", DEBUG_ERROR);
		    err = kNoResult;
	    }
	}
	
	free(p); // free the memory allocated by doPost()
	return err;
}

//
// ping()
//
// game		-> Game information that should be updated.  
//
// return	<- error code
//
// Pings the game to keep it alive at the Match Maker Server. Games will expire after a server set
// timeout. The numeric and boolean data associated with a game can also be updated.  The GUID for
// the game must match the GUID returned by the Server.
// 
long MatchMakerClient::ping(MMGameRec* game)
{
std::string	post("");
char	s[36];  // _ltoa can return up to 33 bytes according to MS docs
long	err;


	// <?XML >
	post += kXML;

	// <protocol>
	post += kProtocol;
	
	// <ping>
	post += "<ping guid='";		// GUID
	post += game->mGUID;
	post += "'>";
	
	// <details>
	post += "<details playersmax='";		// Num. Players Max
    snprintf(s, 10, "%d", game->mPlayersMax);
//	_ltoa(game->mPlayersMax, s, 10);
	post += s;
	post += "' playerscurrent='";			// Num. Current Players
    snprintf(s, 10, "%d", game->mPlayersCurrent);
//	_ltoa(game->mPlayersCurrent, s, 10);
	post += s;
	post += "' computers='";				// Num. Computer Opponents
    snprintf(s, 10, "%d", game->mComputers);
//	_ltoa(game->mComputers, s, 10);
	post += s;
	post += "' compskill='";				// Computer Skill
    snprintf(s, 10, "%d", game->mComputerSkill);
//	_ltoa(game->mComputerSkill, s, 10);
	post += s;
	post += "' compadvantage='";			// Computer Advantage
    snprintf(s, 10, "%d", game->mComputerAdvantage);
//	_ltoa(game->mComputerAdvantage, s, 10);
	post += s;
	post += "' size='";						// Size
    snprintf(s, 10, "%d", game->mSize);
//	_ltoa(game->mSize, s, 10);
	post += s;
	post += "' density='";					// Density
    snprintf(s, 10, "%d", game->mDensity);
//	_ltoa(game->mDensity, s, 10);
	post += s;
	post += "' currentturn='";				// Current Turn
    snprintf(s, 10, "%d", game->mCurrentTurn);
//	_ltoa(game->mCurrentTurn, s, 10);
	post += s;
	if (game->mFast) {						// Fast
		post += "' fast='true";
	}
	else {
		post += "' fast='false";
	}
	if (game->mFogOfWar) {					// Fog of War
		post += "' fogofwar='true";
	}
	else {
		post += "' fogofwar='false";
	}
	post += "' timelimitsecs='";			// time limit seconds
    snprintf(s, 10, "%d", game->mTimeLimitSecs);
//	_ltoa(game->mTimeLimitSecs, s, 10);
	post += s;
	post += "' gameid='";			// time limit seconds
    snprintf(s, 10, "%d", game->mGameID);
//	_ltoa(game->mGameID, s, 10);
	post += s;
	if (game->mPublic) {						// Public
		post += "' public='true";
	}
	else {
		post += "' public='false";
	}
	post += "' />";
	
	// </game>
	post += "</ping>";
	
	// </protocol>
	post += "</protocol>";
	
	// Post Game
	err = noErr;
	char* p = doPost((char*)post.c_str(), err);
	
	if ((err != noErr) || !p) {
	    DEBUG_OUT("MatchMaker ping DoPost() returned error code: ["<< err<<"]", DEBUG_ERROR);
		return err;
	}
	
	std::string	str(p);
	std::string value;
	
	
//	DEBUG_OUT("MatchMaker ping POST reply: ["<< str.c_str()<<"]", DEBUG_TRIVIA);

	long index = str.find("error code=");
	if (index != -1) {
		index += 14;
		value.assign(index,str.find(">",index)-(index-1));
		err = atol(value.c_str()) + kGalacticaMM_errorBase;
	    DEBUG_OUT("MatchMaker ping found error response: ["<< err<<"] from POST ["<<post.c_str()
	                <<"] with reply ["<<str.c_str()<<"]", DEBUG_ERROR);
	}
	
	free(p); // free the memory allocated by doPost()
	return err;
}

