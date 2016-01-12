//
// MatchMakerClient.h
//
//
#ifndef MATCH_MAKER_CLIENT_H_INCLUDED
#define MATCH_MAKER_CLIENT_H_INCLUDED

class MatchMakerClient;

#define kXML		"<?xml version='1.0' encoding='UTF-8'?>"
#define kProtocol	"<protocol copyright='copyright (c) 2002, Sacred Tree Software'>"

#define kGalacticaMM_errorBase	32000


#define kNoResult				0  + kGalacticaMM_errorBase
#define kNoProtocol				1  + kGalacticaMM_errorBase
#define kGameRecSuccess			2  + kGalacticaMM_errorBase
#define kClientSuccess			3  + kGalacticaMM_errorBase
#define kUnrecognizedPost		4  + kGalacticaMM_errorBase
#define kInvalidGame			5  + kGalacticaMM_errorBase
#define kPingSuccess			6  + kGalacticaMM_errorBase


// ERZ, no magic numbers
#define MM_MAX_STATUS_LEN    10
#define MM_MAX_IP_LEN        16
#define MM_MAX_PORT_LEN      6
#define MM_MAX_REG_CODE_LEN  40
#define MM_MAX_GAME_NAME_LEN 50
#define MM_MAX_GUID_LEN      30
#define MM_MAX_URL_LEN       200

// --------------------
//
// MMGameRec
//
// Wrapper for the data associated with a game.  All member variables are publicly accessible, and should accessed directly.
//
// --------------------
class MMGameRec {
public:
	char	mGameName[MM_MAX_GAME_NAME_LEN];
	char	mStatus[MM_MAX_STATUS_LEN];
	char	mPrimaryIP[MM_MAX_IP_LEN];
	char	mPrimaryPort[MM_MAX_PORT_LEN];
	char	mSecondaryIP[MM_MAX_IP_LEN];
	char	mSecondaryPort[MM_MAX_PORT_LEN];
	int		mPlayersMax;
	int		mPlayersCurrent;
	int		mComputers;
	int		mComputerSkill;
	int		mComputerAdvantage;
	int		mCurrentTurn;
	int		mSize;
	int		mDensity;
	bool	mFast;
	bool	mFogOfWar;
	int		mTimeLimitSecs;
	char	mRegCode[MM_MAX_REG_CODE_LEN];
	char	mGUID[MM_MAX_GUID_LEN];
	int     mGameID;  // in v2.3, helps recognize when games with same gamename and/or IP/hostname are different games.
	bool	mPublic;
	
	MMGameRec() : mGameID(0), mPublic(true) {}
};

// --------------------
//
// MMGameList
//
// This class is used to access the list of games returned by a requestGames call.
//
// --------------------
//
// MMGameList()
//
// Default constructor. Sets member variables to their default values.
//
// ----------
//
// ~MMGameList()
//
// Iterates the game list disposing the game objects, and then frees the game list handle.
//
// ----------
//
// getGame()
//
// index	-> position from which to retrieve the game
//
// return	<- ptr to the game record
//
// Retrieves the game record at the index specified. If the index is out of scope, or if the
// game list is nil, this method will return nil.  The game list retains responsibility for 
// managing the destruction of the object.
//
// ----------
//
// setGame()
//
// index	-> position to insert the game rec
// gameRec	-> game to be set
//
// Stores the game record at the index specified.  If this call succeeds, the MMGameList object
// takes responsibility for destroying this object.  The list assumes responsibility for managing
// the destruction of the object.
//
// throw - exception if the game list is nil or the index is out of bounds.
//
// ----------
//
// setNumGames()
//
// gameCount	-> number of games to be stored in the game list
//
// Calls freeGames to dispose of the current games and game list. It then sets the member variable
// for the game count, and allocates a game list to store the games.
//
// throw - exception if the memory to store the list can not be allocated.
//
// ----------
//
// getNumGames()
//
// return	<- number of games in list
//
// Returns the number of games stored in the list
//
// ----------
//
// freeGames()
//
// Provide method to cleanup the memory and objects used by the game list.  Iterates the game list
// destroying the game objects and frees the game list. It then sets the game list member variable to nil.
//
// --------------------
class MMGameList {
	int         mNumGames;
	MMGameRec**	mGameList;
	
public:
	MMGameList();
	~MMGameList();
	
	MMGameRec* getGame(int index);
	void setGame(int index, MMGameRec* game);
	void setNumGames(int gameCount);
	long getNumGames();
	
protected:
	void freeGames();
};


// --------------------
//
// MatchMakerClient
//
// This object provides methods for accessing the Match Maker Server.  It will not function if the URLAccess 
// library is unavailable.  The presence of this library can be tested with the static method accessAvailable.
//
// --------------------
//
// MatchMakerClient()
//
// Default constructor 
//
// ----------
//
// ~MatchMakerClient()
//
// Destructor 
//
// ----------
//
// static
// accessAvailable()
//
// return	<- true if the library is available.
//
// Used to determine if the URLAccess library is available. This object will not function
// without access to the URLAccess library.
//
// ----------
//
// setMatchMakerURL()
//
// URL	-> URL of the Match Maker Server
//
// Sets the URL at which the Match Maker Server can be accessed.
//
// ----------
//
// DoPost()
//
// post	-> Text to be posted
// err	<- err code 
//
// Utility method for posting the string provided to the URL specified by setMatchMakerURL.  Any
// errors are returned in err.
//
// ----------
//
// parseGame()
//
// game		-> string containing the XML that describes the game
// gameRec	<- gameRec object
//
// Creates an MMGameRec object and parses the XML provided to fill in the fields.
//
// ----------
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
// ----------
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
// ----------
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
// --------------------
class MatchMakerClient {
	char	mURL[MM_MAX_URL_LEN];
	
public:
	MatchMakerClient();
	virtual ~MatchMakerClient();
	
	static bool matchmakerAvailable();
	void setMatchMakerURL(const char* URL);
	long requestGames(MMGameList* &gameList, char* regCode);
	long postGame(MMGameRec* game, char* regCode);
	long ping(MMGameRec* game);
	
protected:
	void parseGame(char* game, MMGameRec* gameRec);
	char* doPost(char* post, long &err, long timeoutMs = 10000);  // returns malloc'd block, must be freed
};

#endif // MATCH_MAKER_CLIENT_H_INCLUDED

