// CMessage.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ 4/18/96
// 4/4/99 ERZ Added Support for Single Player Unhosted


/* 
Here's roughly how messages work in v1.2

The host calls the static method CMessage::CreateEvent(), passing in info about the type of event that occured,
what and who were affected, and who is the recipient of the event. CreateEvent() then creates a CMessage object
with all the relevant info. It uses the Evnt resource Event_EventSetup (128) to load the info about what sounds,
text and event report strings are to be used, as well as what special PPob resource should be used to display
the message. CreateEvent() then finishes by saving the new CMessage object into the game's database.

When the client loads the changes from the last End of Turn, all the new CMessage objects are included. As they
are streamed in, CMessage::FinishCreateSelf() checks to see if any of the display info is unfinalized (ie, not
placed in mSndResID, mTEXTResID, etc...) and then fills them out based on whether that player is the winner, 
loser, or someone else. The actual report string is then built from the mSTRxResID and mStrIndex once all this 
info is in place. 

The messages are added to the Event Report list. If mStrIndex = 0, then item is not added to the 
event report. This is for events that will never be shown to other players, even when show enemy
moves is on.

CMessage::DoMessage() is called when the user double-clicks on the Event Report item or when the automatic display
of messages is taking place at the beginning of the turn. It checks that the message hasn't already expired, plays
the sound, and checks that the requested PPob resource is available. It will use window_Message as the PPob if the 
requested one isn't in the resource path. DoMessage() calls DoMessageSelf() to handle the actual display of the 
PPob.

CMessage::DoMessageSelf() defaults to just calling MovableAlert() with the PPob passed in.

CMessage objects are automatically deleted by both the host and the client after they expire, and the host never
informs clients of the deletion of a CMessage.
NOTE: As of v2.1, Events are always one shot, so they expire on the turn they were created.

IMPORTANT! Messages never get added to the view hierchy, so their mSuperView member variable should
be presumed always NIL

*/

#include "CMessage.h"
#include "GalacticaProcessor.h"
#include <Endian.h>

#ifndef GALACTICA_SERVER
#include "CNewGWorld.h"
#include "GalacticaDoc.h"
#include <UDrawingUtils.h>
#include "USound.h"
#include "CLayeredOffscreenView.h"
#include "GalacticaTutorial.h"
#include "GalacticaPanes.h"
#endif // GALACTICA_SERVER

#include <cctype>


#ifndef GALACTICA_SERVER
CNewGWorld*	CDestructionMsg::sExplosion = NULL;
RgnHandle   CDestructionMsg::sRefreshRgn = NULL;
#endif // GALACTICA_SERVER


EventDefT	gNullEvent = { {0,0,0}, 0, {0,0,0}, {0,0,0}, 0 };

#define DELAY(ticks) {register unsigned long z_NextTicks = ::TickCount() + ticks;	\
					  register unsigned long z_CurrTicks; 							\
					  do { 															\
					  	z_CurrTicks = ::TickCount();								\
					  } while (z_CurrTicks < z_NextTicks); }

bool IsDestructionEvent(UInt16 inCode);


EventDefListT*  CMessage::sEventsRes = 0;

#warning TODO: byte swapping ??
LStream& 
operator >> (LStream& inStream, EventDefT& outEventDef) {
	inStream.ReadBlock(&outEventDef, sizeof(EventDefT));
	return inStream;
}

LStream& 
operator << (LStream& inStream, EventDefT& inEventDef) {
	inStream.WriteBlock(&inEventDef, sizeof(EventDefT));
	return inStream;
}

// used by CreateEvent() to determine if the requested event is a destruction
// event or not. Make sure you keep this in sync with the EEventCode definitions
// in "GalacticaConsts.h"
bool 
IsDestructionEvent(UInt16 inCode) {
	if ( (inCode >= event_DefendersLost) && (inCode <= event_PlanetDamaged) ) {
		return true;
	} else {
		return false;
	}
}

// **** MESSAGE ****

#ifndef GALACTICA_SERVER


class CShowMessageAttach : public CDialogAttachment {
public:
	CShowMessageAttach(CMessage* inMessageObj) {mMessageObj = inMessageObj;}
	virtual bool	PrepareDialog();
	virtual bool	AutoDelete() {return true;}		// MovableAlert() will delete this for us
	virtual void	ExecuteSelf(MessageT inMessage, void* ioParam);
protected:
	CMessage* mMessageObj;
};

bool
CShowMessageAttach::PrepareDialog() {
	mDoSysBeep = false;	// don't beep
	bool bUseFullName = false;
	// make sure the correct picture is displayed
	CLayeredOffscreenView* lov = 
	    static_cast<CLayeredOffscreenView*>(mDialog->FindPaneByID(layeredView_MessagePict));
	ResIDT aLayrID = mMessageObj->GetLayrResID();
	if (lov && aLayrID)
		lov->SetLayrResID(aLayrID);
	// find the text item in the mDialog and replace any special characters with appropriate names
	LTextEditView* lte = static_cast<LTextEditView*>(mDialog->FindPaneByID(textEdit_MessageText));
	if (lte) {
		GalacticaDoc* game = mMessageObj->GetGameDoc();
		TEHandle te = lte->GetMacTEH();
	    DEBUG_ONLY(
    	  std::string s = TEGetTextString(te);
    	  DEBUG_OUT(mMessageObj<<" using base string \""<<s.c_str()<<"\"", DEBUG_TRIVIA | DEBUG_MESSAGE);
        )
		short playerTypeIndex = mMessageObj->GetMyPlayerTypeIndex();
		if (mMessageObj->GetEventCode() == event_PlayerDied) {	// don't use "You" as player's name
			bUseFullName = true;								// in death event
		}
		std::string name;
		int pos;
		UInt16 affectedPlayer;
		SInt32 otherAffectedPlayer;
		bool solo = game->IsSinglePlayer();
		mMessageObj->GetAffectedPlayers(&affectedPlayer, &otherAffectedPlayer);
		do {
			pos = TEFindString(te, "*");	// replace * with affected player's name
			if (pos != std::string::npos) {
				if ((playerTypeIndex == kWinner) && !bUseFullName) { // if I am winner in this event
					LoadStringResource(name, STRx_Names, str_You);				// use "you" for the player's name
				} else if (affectedPlayer == kAllPlayers) {
					LoadStringResource(name, STRx_Messages, str_AllPlayers);	// use "all players" for player's name
				} else if (solo && game->PlayerIsComputer(affectedPlayer)) {
					std::string actualName;
					game->GetPlayerName(affectedPlayer, actualName);	// call other players by their names
					LoadStringResource(name, STRx_Names, str_ReportSyntax);	// call computer players in single
					int pos2 = name.find('*');
					if (pos2 != std::string::npos) {
						name.replace(pos2, 1, actualName);	// make name like "the Forest Pact"
					}
				} else {
					game->GetPlayerName(affectedPlayer, name);	// call other players by their names
				}
				TEReplace(te, pos, 1, name.c_str(), name.size());
			}
		} while (pos != std::string::npos);
		do {
			pos = TEFindString(te, "#");	// replace # with other affected player's name
			if (pos != std::string::npos) {
				if (playerTypeIndex == kLoser) {			// if I am loser in this event
					LoadStringResource(name, STRx_Names, str_You);		// use "you" for the player's name
				} else if (otherAffectedPlayer == kAllPlayers) {
					LoadStringResource(name, STRx_Messages, str_AllPlayers);	// use "all players" for player's name
				} else if (solo && game->PlayerIsComputer(otherAffectedPlayer)) {
					std::string actualName;
					game->GetPlayerName(otherAffectedPlayer, actualName);	// call other players by their names
					LoadStringResource(name, STRx_Names, str_ReportSyntax);	// call computer players in single
					int pos2 = name.find('*');
					if (pos2 != std::string::npos) {
						name.replace(pos2, 1, actualName);	// make name like "the Forest Pact"
					}
				} else {
					game->GetPlayerName(otherAffectedPlayer, name);	// call other players by their names
				}
				TEReplace(te, pos, 1, name.c_str(), name.size());
			}
		} while (pos != std::string::npos);
		AGalacticThingy* focus = mMessageObj->GetPosition();
		do {
			pos = TEFindString(te, "$");	// replace $ with numeric value
			if (pos != std::string::npos) {
    		    char ts[256];
    		    std::sprintf(ts, "%ld", mMessageObj->GetNumber());
				TEReplace(te, pos, 1, ts, std::strlen(ts));
			}
		} while (pos != std::string::npos);
		AGalacticThingy* subject = mMessageObj->GetSubject();
		do {
			pos = TEFindString(te, "%");	// replace % with Subject item's name
			if (pos != std::string::npos) {
				if (mMessageObj->GetSubjectNamePtr()->size()>0) {
					name = mMessageObj->GetSubjectNamePtr()->c_str();
				} else if (subject) {
					name = subject->GetShortName();
				} else {					
					DEBUG_OUT("Event Message "<<mMessageObj<<" has NIL Subject but has % character", DEBUG_ERROR | DEBUG_MESSAGE);
					name = "?";
				}
				TEReplace(te, pos, 1, name.c_str(), name.size());
			}
		} while (pos != std::string::npos);
		do {
			pos = TEFindString(te, "@");	// replace @ with Focus position
			if (pos != std::string::npos) {
				Waypoint wp = mMessageObj->GetPosition();
				if (wp.IsNowhere()) {	// having a Nowhere position as focus means that we should
					if (subject) {		// use the subject's position as the focus
						wp = focus->GetPosition();
					} else {							// opps! no subject, let's hope the subject
						wp = mMessageObj->GetSubjectWP();	// waypoint has a correct position at least
					}
				}
				wp.GetDescription(name);
				TEReplace(te, pos, 1, name.c_str(), name.size());
				ASSERT(!wp.IsNull());
			}
		} while (pos != std::string::npos);
		// report the final result
	    DEBUG_ONLY(
          s = TEGetTextString(te);
          DEBUG_OUT(mMessageObj<<" string = \""<<s.c_str()<<"\"", DEBUG_TRIVIA | DEBUG_MESSAGE);
        )
   	lte->AlignTextEditRects();
   	lte->AdjustImageToText();
	}
	// tutorial stuff
	// these are here because the tutorial window placement is relative to the message window,
	// which means the message window must have been created
  #if TUTORIAL_SUPPORT
	if (Tutorial::TutorialIsActive()) {
		Tutorial* t = Tutorial::GetTutorial();
		SInt16 pageNum = t->GetPageNum();
		if ( (pageNum == tutorialPage_EndTurnOne) ||	// EndTurnTwo handled in ShowMessages()
			 (pageNum == tutorialPage_EndTurnThree) || 
			 (pageNum == tutorialPage_EndTurnEight) || 
			 (pageNum == tutorialPage_NoticeRing) ) {
			// we were waiting for them to end the turn. They did, and now they are getting
			// message playback
			t->NextPage();
		}
	}
  #endif // TUTORIAL_SUPPORT
	return true;
}

void		
CShowMessageAttach::ExecuteSelf(MessageT inMessage, void* ioParam) {
	if (inMessage == msg_KeyPress) {
		EventRecord* eventP = (EventRecord*)ioParam;
		char key = eventP->message & '\t';
		LControl* ok = (LControl*)mDialog->FindPaneByID(1);
		if (ok) {
			ok->SimulateHotSpotClick(1);
		}
	} else {
		CDialogAttachment::ExecuteSelf(inMessage, ioParam);
	}
}


#endif // GALACTICA_SERVER

#warning FIXME: concentrate Mac specific code as per comment below
// Make the CMessage object just have a std::string for the message,
// and have the display portion analyze the text and figure out which
// part of it needs the header font/size, then use the Mac display
// calls to set that up. This way we only need to convert the display
// code to move to a new platform.
// This will also allow us to remove a ton of #ifdef GALACTICA_SERVER stuff
// from CMessage and at least one place in GalacticaHost.cpp:1940 (HandlePlayerToPlayerMessage)

CMessage::CMessage(GalacticaShared* inGame, long inThingyType)
#ifndef GALACTICA_SERVER
: AGalacticThingyUI(NULL, inGame, inThingyType)
#else
: AGalacticThingy(inGame, inThingyType)
#endif 
{
	mLiveUntil = 0;			// assume 1 turn lifespan
	mLastTurnChecked = 0;			// never shown
	mChanged = true;
	mOtherOwner = kNoOtherPlayers;
	mAffectedPlayer = kAllPlayers;
	mOtherAffectedPlayer = kNoOtherPlayers;
	mEventCode = 0;
	mEventDisplayInfo = gNullEvent;
	mNum = 0;
	mSndResID = mPPobResID = mTEXTResID = mPPobResID = mSTRxResID = mLayrResID = 0;
	mStrIndex = 0;
#ifndef GALACTICA_SERVER
	mMessageText = mMessageStyle = nil;
	mSndPlayer = nil;
#endif // GALACTICA_SERVER
}


CMessage::~CMessage() {
	mCurrPos = gNullPt3D;	// make sure we don't try to remove the message from whatever it was in
}

static const SInt32 length_NilBlock = -1;

//#warning FIXME: Win32 version does not read mMessageText or mMessageStyle
void
CMessage::ReadStream(LStream *inStream, UInt32 streamVersion) {
	AGalacticThingy::ReadStream(inStream, streamVersion);
	*inStream >> mLiveUntil >> mLastTurnChecked >> mEventDisplayInfo >> mAffectedPlayer 
				>> mOtherAffectedPlayer >> mOtherOwner >> mEventCode >> mNum >> mSndResID 
				>> mPPobResID >> mTEXTResID >> mSTRxResID >> mLayrResID >> mStrIndex
				>> mSubjectWP;

#ifndef GALACTICA_SERVER
	*inStream >> mMessageText;
	*inStream >> mMessageStyle;
#else
   // galactica server specific implementation
   SInt32 msgTextLen;
 	*inStream >> msgTextLen;
 	if (msgTextLen != length_NilBlock) {
 	   char* buf = (char*) std::malloc(msgTextLen + 1);
 	   inStream->ReadBlock(buf, msgTextLen);
 	   buf[msgTextLen] = 0; // nul terminator just in case
 	   mMessageStr = buf;
 	} else {
 	   mMessageStr = "";
 	}
 	// now read the style, which should never be sent
   SInt32 unusedStyleLen;
	*inStream >> unusedStyleLen;
	ASSERT(unusedStyleLen == length_NilBlock);
#endif // GALACTICA_SERVER

   // our data format stores these as Pascal strings, with a one byte length
   // so we have to read them as Pascal strings then convert them to C strings
   Str255 reportStr;
   Str255 subjectName;
	*inStream >> reportStr >> subjectName;
   char tmpstr[256];
   p2cstrcpy(tmpstr, reportStr);
   mReportStr.assign(tmpstr);
   p2cstrcpy(tmpstr, subjectName);
   mSubjectName.assign(tmpstr);
	mChanged = false;	// don't let game doc write it back out again if we got it from the host
 #ifndef GALACTICA_SERVER
   if (mGame->IsClient()) {
	    BuildClientEvent();
   }
 #endif // !GALACTICA_SERVER
}

//#warning FIXME: Win32 version does not write mMessageText or mMessageStyle
void
CMessage::WriteStream(LStream *inStream, SInt8 forPlayerNum) {
	AGalacticThingy::WriteStream(inStream);
	*inStream << mLiveUntil << mLastTurnChecked << mEventDisplayInfo << mAffectedPlayer
				<< mOtherAffectedPlayer << mOtherOwner << mEventCode << mNum << mSndResID 
				<< mPPobResID << mTEXTResID << mSTRxResID << mLayrResID << mStrIndex
				<< mSubjectWP;
    // write the message string
 #ifndef GALACTICA_SERVER
	*inStream << mMessageText;
	*inStream << mMessageStyle;
 #else
   // galactica server specific implementation
   SInt32 msgTextLen = mMessageStr.length();
   SInt32 unusedStyleLen = length_NilBlock;
   if (msgTextLen <= 0) {
      msgTextLen = length_NilBlock;
   }
   *inStream << msgTextLen;
   if (msgTextLen != length_NilBlock) {
      inStream->WriteBlock(mMessageStr.data(), msgTextLen);
   }
   *inStream << unusedStyleLen;
 #endif // GALACTICA_SERVER
 
    // our data format stores these as Pascal strings, with a one byte length
    // so we have to convert them to Pascal strings before writing them out
    Str255 reportStr;
    Str255 subjectName;
    c2pstrcpy(reportStr, mReportStr.c_str());
    c2pstrcpy(subjectName, mSubjectName.c_str());
	*inStream << reportStr <<  subjectName;
 #ifndef GALACTICA_SERVER
   if (mGame->IsHost()) {
	   ASSERT(mMessageStyle == NULL);   // we should never be writing a message style from the server
	}
 #endif // !GALACTICA_SERVER
}

// Takes a raw message, one that only the EventDisplayInfo set, and figures out
// exactly what to display based on who the player is in relation to the event
// (ie: winner, loser, other). Then it calls MakeReportString() to actually
// construct the text that will be shown to the user
void 
CMessage::BuildClientEvent() {
	if (mGame->IsClient()) {		// only do for client
    	DEBUG_OUT("Building client event "<< this, DEBUG_DETAIL | DEBUG_MESSAGE | DEBUG_EOT);
    	short playerTypeIndex = GetMyPlayerTypeIndex();	// are we winner, loser, or other?
    	if (mSndResID == 0) {				// fill out details of message, if not already provided
    		mSndResID = mEventDisplayInfo.sndResID[playerTypeIndex];
    		if (mSndResID == 0) {
    			mSndResID = resID_Undefined;	// permit zeroes as undefined snd Res IDs
    		}
    	}
    	if (mPPobResID == 0) {
    		mPPobResID = mEventDisplayInfo.PPobResID[playerTypeIndex];
    	}
    	if (mTEXTResID == 0) {
    		mTEXTResID = mEventDisplayInfo.TEXTResID;
    	}
    	if (mSTRxResID == 0) {
    		mSTRxResID = mEventDisplayInfo.STRxResID;
    	}
    	if (mStrIndex == 0) {
    		mStrIndex = mEventDisplayInfo.strIndex[playerTypeIndex];
    	}
    	if (mStrIndex) {		// Events with no mStrIndex are not intended for display in the event report window
    		MakeReportString();
    		mWantsAttention = true;
    	} else {
    		mWantsAttention = false;
    	}
	} else {
	    DEBUG_OUT("Called CMessage::BuildClientEvent() from Host for "<< this, DEBUG_ERROR | DEBUG_MESSAGE | DEBUG_EOT);
    }
}


void
CMessage::EndOfTurn(long inTurnNum) {
  #ifdef SINGLE_PLAYER_UNHOSTED
	if (mGame->IsSinglePlayer()) {
		// in Single-player unhosted, we can tell the item to die at the end of its turn
		// and it will be automatically cleaned up at the end of the next turn.
		if (mLiveUntil != 0) {
			// has a specific time to live
			if (mLiveUntil < inTurnNum) {	// we've lived longer than we need to
				if (mLastTurnChecked == 0) {
					DEBUG_OUT("Killing "<< this<< ", never checked", DEBUG_DETAIL | DEBUG_EOT | DEBUG_MESSAGE);
				}
				Die();
			}
		} else {
			// when mLiveUntil == 0, the event only lives until the turn after it is shown
			if (mLastTurnChecked != 0) {
				DEBUG_OUT("Killing "<< this<< ", never checked", DEBUG_DETAIL | DEBUG_EOT | DEBUG_MESSAGE);
				Die();
			} else {
				DEBUG_OUT("Leaving alive "<< this<< ", not shown yet", DEBUG_TRIVIA | DEBUG_EOT | DEBUG_MESSAGE);
			}
		}
	} else
  #endif	// SINGLE_PLAYER_UNHOSTED	
	{
		if (mLiveUntil <= inTurnNum) {
			// we want to give the system an extra turn before messages are deleted from the host
			// database in a multiplayer game (or in single-player hosted games)
			if (mLastTurnChecked != -1) {
				DEBUG_OUT("Flagging "<< this<< " for death next turn", DEBUG_DETAIL | DEBUG_EOT | DEBUG_MESSAGE);
				mLastTurnChecked = -1;		// ready for deletion
				mChanged = true;			// make sure system knows that item has been altered
			} else {
				DeleteFromDataStore();		// delete the message now, it's been ready for deletion
				delete this;				// long enough
			}
		}
	}
}

void
CMessage::SetRecipients(UInt16 inPlayer, SInt32 inOtherPlayer) {
	mOwnedBy = inPlayer;
	mOtherOwner = inOtherPlayer;
}

void
CMessage::GetRecipients(UInt16 *outPlayer, SInt32 *outOtherPlayer) {
	*outPlayer = mOwnedBy;
	if (outOtherPlayer) {
		*outOtherPlayer = mOtherOwner;
	}
}

void
CMessage::SetAffectedPlayers(UInt16 inPlayer, SInt32 inOtherPlayer) {
	mAffectedPlayer = inPlayer;
	mOtherAffectedPlayer = inOtherPlayer;
}

void
CMessage::GetAffectedPlayers(UInt16 *outPlayer, SInt32 *outOtherPlayer) {
	*outPlayer = mAffectedPlayer;
	if (outOtherPlayer) {
		*outOtherPlayer = mOtherAffectedPlayer;
	}
}


void
CMessage::SetSubject(AGalacticThingy* inThingy) {
	ASSERT(ValidateThingy(inThingy) == inThingy);
	mSubjectWP = inThingy;
	mCurrPos = gNowherePt3D;		// assume the message has no physical location
	if (inThingy) {
		if (inThingy->IsDead()) {	// there is nothing
		    mSubjectName.assign(inThingy->GetName()); // for dead things, the physical location
			mCurrPos = inThingy->GetPosition();	      // needs to be saved for after the thing
		}											  // is gone.
	}
}

StringPtr
CMessage::GetDescriptor(Str255 outDescriptor) const {
	if (mStrIndex && (mReportStr.size() == 0)) {
		(const_cast<CMessage*>(this))->MakeReportString();
	}
	c2pstrcpy(outDescriptor, mReportStr.c_str());
	return outDescriptor;
}

void
CMessage::MakeReportString() {
	short playerTypeIndex = GetMyPlayerTypeIndex();
	std::string s;
//	s.SetCompareFunc(LString::CompareBytes);	// v1.2b11d3, toolbox compare was too slow
	if (mReportStr.size()>0) {
		s = mReportStr;
		DEBUG_OUT(this<<" report string exists, reusing", DEBUG_TRIVIA | DEBUG_MESSAGE);
	} else {
	    LoadStringResource(s, mSTRxResID, mStrIndex);
		DEBUG_OUT(this<<" using base string \""<<s.c_str()<<"\"", DEBUG_TRIVIA | DEBUG_MESSAGE);
	}
	std::string name;
	int pos;
	bool solo = mGame->IsSinglePlayer();
	do {
		pos = s.find('*');	// replace * with affected player's name
		if (pos != std::string::npos) {
			if (playerTypeIndex == kWinner)	{			// if I am winner in this event
				LoadStringResource(name, STRx_Names, str_You);		// use "you" for the player's name
			} else if (mAffectedPlayer == kAllPlayers) {
				LoadStringResource(name, STRx_Messages, str_AllPlayers);	// use "all players" for player's name
			} else if (solo && mGame->PlayerIsComputer(mAffectedPlayer)) {
				std::string actualName;
				GetGame()->GetPlayerName(mAffectedPlayer, actualName);	// call other players by their names
				LoadStringResource(name, STRx_Names, str_ReportSyntax);	// call computer players in single
				int pos2 = name.find('*');
				if (pos2 != std::string::npos) {
					name.replace(pos2, 1, actualName);	// make name like "the Forest Pact"
				}
			} else {
				GetGame()->GetPlayerName(mAffectedPlayer, name);	// call other players by their names
			}
			s.replace(pos, 1, name);
		}
	} while (pos != std::string::npos);		// do with all stars
	do {
		pos = s.find('#');	// replace # with other affected player's name
		if (pos != std::string::npos) {
			if (playerTypeIndex == kLoser) {			// if I am loser in this event
				LoadStringResource(name, STRx_Names, str_You);		// use "you" for the player's name
			} else if (mOtherAffectedPlayer == kAllPlayers) {
				LoadStringResource(name, STRx_Messages, str_AllPlayers);	// use "all players" for player's name
			} else if (solo && mGame->PlayerIsComputer(mOtherAffectedPlayer)) {
				std::string actualName;
				GetGame()->GetPlayerName(mOtherAffectedPlayer, actualName);	// call other players by their names
				LoadStringResource(name, STRx_Names, str_ReportSyntax);	// call computer players in single
				int pos2 = name.find('*');
				if (pos2 != std::string::npos) {
					name.replace(pos2, 1, actualName);	// make name like "the Forest Pact"
				}
			} else {
				GetGame()->GetPlayerName(mOtherAffectedPlayer, name);	// call other players by their names
			}
			s.replace(pos, 1, name);
		}
	} while (pos != std::string::npos);		// do with all stars
	do {
		pos = s.find('$');	// replace $ with numeric value
		if (pos != std::string::npos) {
		    char ts[256];
		    std::sprintf(ts, "%ld", mNum);
			s.replace(pos, 1, ts);
		}
	} while (pos != std::string::npos);		// do with all stars
	AGalacticThingy* subject = GetSubject();
	do {
		pos = s.find('%');	// replace % with Subject item's name
		if (pos != std::string::npos) {
			if (mSubjectName.size()>0) {
				name.assign(mSubjectName);
			} else if (subject) {
			    name.assign(subject->GetName());
			} else {
				DEBUG_OUT("Message "<<this<<" has NIL Subject but has % character", DEBUG_ERROR | DEBUG_MESSAGE);
				name.assign("?");
			}
			s.replace(pos, 1, name);
		}
	} while (pos != std::string::npos);		// do with all stars
	do {
		pos = s.find('@');	// replace @ with Focus position
		if (pos != std::string::npos) {
			Waypoint wp = mCurrPos;
			if (wp.IsNowhere()) {	// having a Nowhere position as focus means that we should
				if (subject) {		// use the subject's position as the focus
					wp = subject->GetPosition();
				} else {					// opps! no subject, let's hope the subject
					wp = mSubjectWP;		// waypoint has a correct position at least
				}
			}
			std::string descStr;
			s.replace(pos, 1, wp.GetDescription(descStr));
			ASSERT(!wp.IsNull());
			if (wp.IsNull()) {
				DEBUG_OUT(this<<" has bad focus; text = " << s.c_str(), DEBUG_ERROR | DEBUG_MESSAGE);
			}
		}
	} while (pos != std::string::npos);		// do with all stars
	// uppercase the first character of the string
	s[0] = std::toupper(s[0]);
	mReportStr = s;
	DEBUG_OUT(this<<" string = \"" << mReportStr.c_str()<<"\"", DEBUG_TRIVIA | DEBUG_MESSAGE);
}

bool
CMessage::MessageIsToMe() const {
  #ifdef GALACTICA_SERVER
    return false;
  #else
    ASSERT(mGame->IsClient());   // call from client only
	int me = GetGameDoc()->GetMyPlayerNum();
  #warning FIXME: mOwnedBy is an UInt16, but treat player number as a SInt8 in most places
	if (me == (SInt8)mOwnedBy) {
	    // v2.1b3, don't request rename of a colony that has been recaptured by enemy
	    if (mEventCode == event_RenameColony) {
	        if (GetSubject() && (GetSubject()->GetOwner() == me)) {
	            return true;
	        } else {
	            return false;
	        }
	    }
		return true;		// sent directly to me
	} else if (mOtherOwner == kAllPlayers) {
		return true;		// sent to everyone
	} else if (mOwnedBy == kAllPlayersExcept) {
		if (me != mOtherOwner) {	// this also covers case of (kAllPlayersExcept, kNoOtherPlayers)
			return true;		// excluded someone other than me
		} else {
			return false;		// I have been excluded from this message
		}
	} else if (me == mOtherOwner) {
		return true;		// sent specifically to me as secondary recipient
	} else {
		return false;		// I'm not supposed to get this message
	}
  #endif // GALACTICA_SERVER
}

bool
CMessage::MessageIsAboutMe() const {
    return (GetMyPlayerTypeIndex() < kOther);
}

bool
CMessage::MessageConcernsEveryone() const {
	if ( (mEventCode == event_PlayerDied) || (mEventCode == event_PlayerWins) ) {
		return true;
	} else {
		return false;
	}
}


short
CMessage::GetMyPlayerTypeIndex() const {
  #ifdef GALACTICA_SERVER
    return kHost;
  #else
    ASSERT(mGame->IsClient());   // call from client only
	UInt16 me = GetGameDoc()->GetMyPlayerNum();
	if (me == mAffectedPlayer) {
		return kWinner;
	} else if (me == mOtherAffectedPlayer) {
		return kLoser;
	} else {
		return kOther;
	}
  #endif // GALACTICA_SERVER
}


bool
CMessage::RefersTo(AGalacticThingy *inThingy) {
	if (mGame->IsSinglePlayer()) {
		// 4/11/99, no way to check for this properly under single player unhosted because
		// the item gets deleted first before the message even dies
		return false;
	}
  	if (mGame->IsHost()) {
  		// 6/8/99, v1.2b11d3, on the host, it doesn't matter if a Message refers to 
  		// something that is dead since the message is never displayed on the host
  		return false;

  	}
	ASSERT(inThingy != nil);
	if (mNeedsDeletion) {	// 3/13/98, fix crashes caused by dangling reference to deleted object
		return false;		// don't check for references when this is already marked for deletion
	}
	bool refersToIt = (inThingy == mSubjectWP);
	if (!refersToIt) {
		refersToIt = AGalacticThingy::RefersTo(inThingy);
	}
	return refersToIt;
}



/* CMessage::CreateEvent() - static method

	should only be called by host from within calling chain of GalacticHostDoc::DoEndTurn()
	can pass in -1 for inOtherPlayer to indicate "not applicable"
 PARAMETERS:
    inGame      - a pointer to a GalacticaProcessor that is the game host
	inCode		- an Event Code as defined in GalacticaConsts.h. Index into the EventDefs
					("Evnt" resource #128) which have info about which Event Strings are
					used and what sounds are played.
	inRecipient	- the player # of the primary recipient
					Special Values:
						kAllPlayers - inOtherRecipient must be unspecified (or kAllPlayers)
						kAllPlayersExcept - inOtherRecipient must be player# of player to exclude
	inSubject	- Thingy that was affected by the event (optional)
					Event String:
						% is replaced with name of this item
						@ is replace with position of this item, unless SetFocus() is called
							to explicitly set a different position.
	inOtherRecipient - the player number of the additional recipient (optional)
					Special Values:
						kAllPlayers - all other players
						kNoOtherPlayers - don't send it to anyone else
	inAffectedPlayer - the player # of the player primarily affected by this event (optional)
					Special:
						If unspecified, the value for inRecipient is used instead.
						In a combat event, this player was the Winner.
					Event String:
						* is replaced with this player's name
	inOtherAffectedPlayer - the player # of the other player affected by this event (optional)
					Special:
						If unspecified, the value for inOtherRecipient is used instead.
						In a combat event, this player was the Loser.
					Event String:
						# is replaced with this player's name
	inNum - a numeric value (optional)
					Event String:
						$ is replaced with a string representation of this number
	inLayrResID - a resource ID to a Layr resource (optional)
	inWriteToDB - defaults to true. false means you must write it yourself, mainly useful if
					you need to set additional info in the message, such as changing its
					focus using SetFocus()
RETURNS:
	a pointer to the newly created CMessage object. Unless you set inWriteToDB to false,
	you shouldn't need to pay any attention to this return result.
*/
CMessage*
CMessage::CreateEvent(GalacticaProcessor* inGame, UInt16 inCode, UInt16 inRecipient, 
                        AGalacticThingy* inSubject,
						SInt32 inOtherRecipient, UInt16 inAffectedPlayer,
						SInt32 inOtherAffectedPlayer, SInt32 inNum, ResIDT inLayrResID, 
						bool inWriteToDB) {
	if (inCode == event_None) {
		return NULL;
	}
	if (sEventsRes == NULL) {
		sEventsRes = static_cast<EventDefListT*>( LoadResource(type_EventsRes, Evnt_EventSetup) );
		ThrowIfNil_(sEventsRes);
        // endian swapping here, we only load this once
        sEventsRes->numEventCodes = EndianS16_BtoN(sEventsRes->numEventCodes);
        for (int i = 0; i < sEventsRes->numEventCodes; i++) {
			sEventsRes->eventDef[i].STRxResID = EndianS16_BtoN(sEventsRes->eventDef[i].STRxResID);
			sEventsRes->eventDef[i].TEXTResID = EndianS16_BtoN(sEventsRes->eventDef[i].TEXTResID);
            for (int j = 0; j < EVENT_TO; j++) {
				sEventsRes->eventDef[i].sndResID[j] = EndianS16_BtoN(sEventsRes->eventDef[i].sndResID[j]);
				sEventsRes->eventDef[i].strIndex[j] = EndianS16_BtoN(sEventsRes->eventDef[i].strIndex[j]);
				sEventsRes->eventDef[i].PPobResID[j] = EndianS16_BtoN(sEventsRes->eventDef[i].PPobResID[j]);
            }
        }
	}
	EventDefListT* eventDefs = sEventsRes;

	if (inAffectedPlayer == 0) {				// if no affected player specified, assume
		inAffectedPlayer = inRecipient;			// the recipients were affected. Anything other
	}											// than a single specific player as the other
	if (inOtherAffectedPlayer == 0) {
		inOtherAffectedPlayer = inOtherRecipient; // recipient means no other affected player
	}
	CMessage* aMessage = nil;
	if ( IsDestructionEvent(inCode) ) {			// check for destruction events
		aMessage = new CDestructionMsg(inGame);	// and create a destruction Message instead 
	} else {									// of a conventional message
		aMessage = new CMessage(inGame);
	}
	aMessage->AssignUniqueID();					// v1.2b8, get the id right away.
	aMessage->SetEventCode(inCode);				// what event are we sending?
	aMessage->SetSubject(inSubject);			// what thingy is the event about?
	aMessage->SetRecipients(inRecipient, inOtherRecipient);	 // who is the event intended for?
	aMessage->SetAffectedPlayers(inAffectedPlayer, inOtherAffectedPlayer);	// who was affected by the event
	aMessage->SetNumber(inNum);
	aMessage->SetLayrResID(inLayrResID);
	if (inCode <= eventDefs->numEventCodes) {	// use the event code to fill out the sound and display info
		aMessage->SetEventDisplayInfo(&(eventDefs->eventDef[inCode-1]));
	}
	if (inWriteToDB) {
		aMessage->Persist();
		aMessage->NotChanged();	// keep host from persisting it twice
	}
	return aMessage;
}


#ifndef GALACTICA_SERVER
Rect
CMessage::GetHotSpotRect() const {	// messages aren't clickable
	Rect r = {0, 0, 0, 0};
	return r;
}


void
CMessage::Select() {
	DEBUG_OUT("CMessage::Select() is being called: "<< this, DEBUG_DETAIL | DEBUG_EOT | DEBUG_MESSAGE);
    if (!mGame->IsClient()) {
        return;
    }
    AGalacticThingyUI* subject = NULL;
    AGalacticThingy* it = mSubjectWP.GetThingy();
    if (it) {
	    subject = it->AsThingyUI();
	}
	if (subject && !subject->IsDead()) {	// don't select dead subjects
	    DEBUG_OUT("CMessage::Select() calling "<<subject<<" Select()", DEBUG_DETAIL | DEBUG_EOT | DEBUG_MESSAGE);
		subject->Select();					// move to the subject thingy, if there is one
	} else if ( !mCurrPos.IsNowhere() ) {	// otherwise, try to move to the focused position
	    DEBUG_OUT("CMessage::Select() calling AGalacticThingyUI::Select()", DEBUG_DETAIL | DEBUG_EOT | DEBUG_MESSAGE);
		AGalacticThingyUI::Select();			// assuming that is not Nowhere. If it is nowhere,
	} else {								    // then this message has no physical location.
		GetGameDoc()->ThingyAttendedTo(this);	// It also means we have to mark the thingy as
	}										    // attended to ourself, rather than via Select()
	GetGameDoc()->ShowMessageEventText(this);	// Put up the appropriate text in the viewer
}


void
CMessage::PlayMessage() {
	DEBUG_OUT("PlayMessage: "<< this, DEBUG_DETAIL | DEBUG_EOT | DEBUG_MESSAGE);
	long turnNum = mGame->GetTurnNum();	
   Select();
	DoMessage(turnNum);				// ignoring the return result, which is meaningless
	MessageHandled();
}

void
CMessage::MessageHandled() {
	mLastTurnChecked = mGame->GetTurnNum();	
}

bool
CMessage::DoMessage(long inTurnNum) {
	bool result = true;
	mSndPlayer = nil;
	if (mPPobResID <= 0) {							// no PPob, may be a sound only
		if (mSndResID != resID_Undefined) {
			if (mSubjectWP.IsNull()) {
				StSoundResourcePlayer(mSndResID, initMono);	// let it play completely before moving on to next message
			} else {
				new CSoundResourcePlayer(mSndResID);		// v1.2b10, go immediately on to next message
			}
		}
		result = true;	
	} else {										// this is a more normal message, so 
		if (mSndResID != resID_Undefined) {			// call DoMessage() and play the message's
			mSndPlayer = new CSoundResourcePlayer(mSndResID, initMono, false, true); // sound, if there is one	
		}
		SetResLoad(false);							// make sure we actually have the PPob resource requested
		Handle temp = ::GetResource('PPob', mPPobResID);
		SetResLoad(true);
		if (temp == nil) {					// no PPob resource: Galactica Art file probably not present
			mPPobResID = window_Message;	// so use the message window instead
		}
		result = DoMessageSelf(inTurnNum);
		if (mSndPlayer) {
			mSndPlayer->StopPlaying();
			delete mSndPlayer;
		}
	}
	return result;
}


bool
CMessage::DoMessageSelf(long inTurnNum) {
	bool result = true;
	LStr255 s;
	if (mLiveUntil > inTurnNum) {
		s.Assign(STRx_Messages, str_MsgRepeatsUntil);
		s += mLiveUntil;
	}
	LStr255 name;
	if (mOwnedBy == 0) {
		name.Assign(STRx_Messages, str_AllPlayers);
	} else {
		name = GetGameDoc()->GetMyPlayerName();
	}
	LStr255 messageText(mMessageText);
	MovableParamText(0, name);
	MovableParamText(1, messageText);	
	MovableParamText(2, s);
	long autoCloseDelay = GetGameDoc()->GetSecondsRemainingInTurn();
	if (autoCloseDelay == -1) {
		autoCloseDelay = 0;		// no auto close, the game is not timed
	} else {
		autoCloseDelay *= 2;	// convert to half-seconds, used by MovableAlert
		if (autoCloseDelay < 20) {	// window must stay open at least 10 seconds
			autoCloseDelay = 20;
		}
	}
	long response = MovableAlert(mPPobResID, nil, autoCloseDelay, false, new CShowMessageAttach(this));
	if (response == 1) {		// OK button
		result = (mLiveUntil <= inTurnNum);	// message expired
	} else {
		result = false;		// keep message around
	}
	return result;	// wasn't for us. delete it immediately
}

#endif // GALACTICA_SERVER


// **** DESTRUCTION MESSAGE ****

CDestructionMsg::CDestructionMsg(GalacticaShared* inGame) 
: CMessage(inGame) {
	mClassInfo = AGalacticThingy::GetClassInfoFromType(thingyType_DestructionMessage);
  #ifndef GALACTICA_SERVER
	if (sRefreshRgn == NULL) {
	    // setup a static region handle so we don't have to do constant reallocations
	    sRefreshRgn = ::NewRgn();
	}
  #endif // GALACTICA_SERVER
}


#ifndef GALACTICA_SERVER
bool	
CDestructionMsg::DoMessage(long) {	// inTurnNum
	CStarMapView* starmap = GetGameDoc()->GetStarMap();
	ASSERT(starmap != nil);
	if (sExplosion == nil) {
		sExplosion = new CNewGWorld(PICT_Explosions);
		sExplosion->SetNumSubImages(8);
	}
	StSoundResourcePlayer sound(snd_Explosion);	// begin playing the explosion sound
	Rect r = sExplosion->GetSubImageSizeRect();
	starmap->FocusDraw();
	GrafPtr dstPort = starmap->GetMacPort();
	Point p;
	if (mCurrPos.IsNowhere()) {
		DEBUG_OUT(this << " exploding at subject position " << mSubjectWP, DEBUG_TRIVIA | DEBUG_MESSAGE);		
		p = mSubjectWP.GetLocalPoint();	// gives us the center point of the subject's position
	} else {
		DEBUG_OUT(this << " exploding at " << mCurrPos, DEBUG_TRIVIA | DEBUG_MESSAGE);		
		p = mCurrPos.GetLocalPoint();	// gives us the center point of the message's position
	}
	starmap->GalacticToImagePt(p);
	::MacOffsetRect(&r, p.h - (r.right>>1), p.v - (r.bottom>>1));
	::ForeColor(blackColor);
	::BackColor(whiteColor);

#ifdef USE_OLD_EXPLOSIONS
	// this is the old code for drawing explosions. It doesn't erase between
	// frames.
	for (int i = 1; i <= 8; i++) {
		sExplosion->CopySubImage(dstPort, r, i, 1, transparent);
		DELAY(3);
	}
	DELAY(12);
	::BackColor(blackColor);
	::EraseRect(&r);
	RgnHandle refreshRgn = ::NewRgn();
	if (refreshRgn != nil) {
		starmap->LocalToPortPoint(topLeft(r));
		starmap->LocalToPortPoint(botRight(r));
		::RectRgn(refreshRgn, &r);
		starmap->Draw(refreshRgn);	// erase the explosion
		::DisposeRgn(refreshRgn);
	}
#else
	// here's the new code for drawing explosions. It erases between
	// frames where the explosion is getting smaller
	Rect refreshRect = r;	
	starmap->LocalToPortPoint(topLeft(refreshRect));
	starmap->LocalToPortPoint(botRight(refreshRect));
	::RectRgn(sRefreshRgn, &refreshRect);
	for (int i = 1; i <= 8; i++) {
		sExplosion->CopySubImage(dstPort, r, i, 1, transparent);
	  #if TARGET_API_MAC_CARBON
        ::QDFlushPortBuffer(dstPort, NULL);
      #endif
		if (i < 5) {
			DELAY(i + 1);
		} else {
			DELAY(11-i);
			::BackColor(blackColor);
			::EraseRect(&r);	
			starmap->Draw(sRefreshRgn);	// erase the explosion
			::BackColor(whiteColor);
		  #if TARGET_API_MAC_CARBON
         ::QDFlushPortBuffer(dstPort, NULL);
        #endif
		}
	}
#endif

	if (mSndResID != resID_Undefined) {
		DELAY(20);
		StSoundResourcePlayer sound(mSndResID);	// begin playing the extra sound
	}
	return true; // CMessage::DoMessage(inTurnNum);
}

#endif // GALACTICA_SERVER
