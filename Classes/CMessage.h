// AGalacticThingy.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ, 4/18/96 ERZ

#ifndef CMESSAGE_H_INCLUDED
#define CMESSAGE_H_INCLUDED

#include <LStream.h>

#include "AGalacticThingy.h"

#ifndef GALACTICA_SERVER
#include "AGalacticThingyUI.h"

#include "USound.h"
class CNewGWorld;
#endif // GALACTICA_SERVER

#define kAllPlayers 0
#define kAllPlayersExcept kAllPlayers
#define kNoOtherPlayers -1

#define EVENT_TO 3		// types of people an event can affect
#define kWinner 0		// code for person who is winner
#define kLoser  1		// code for person who is loser
#define kOther  2		// code for some other person who is neither winner nor loser
#define kHost   3       // code for a "person" who is the host

#ifdef __MWERKS__
#pragma options align=mac68k
#endif
typedef struct {
	ResIDT	sndResID[EVENT_TO];
	ResIDT	STRxResID;
	SInt16	strIndex[EVENT_TO];
	ResIDT	PPobResID[EVENT_TO];
	ResIDT	TEXTResID;
	UInt8	autoShow;	// bool
	UInt8	UNUSED_PAD;
} EventDefT, *EventDefPtr;

// following defines structure of an 'Evnt' resource
typedef struct {
	SInt16		numEventCodes;		// how many events were defined
	EventDefT	eventDef[];			// number of events defs as specified in the resource
} EventDefListT, *EventDefListPtr;	
#ifdef __MWERKS__
#pragma options align=reset
#endif

LStream& operator >> (LStream& inStream, EventDefT& outEventDef);
LStream& operator << (LStream& inStream, EventDefT& inEventDef);

class CMessage 
#ifndef GALACTICA_SERVER
: public AGalacticThingyUI 
#else
: public AGalacticThingy
#endif
{
public:

	enum { class_ID = thingyType_Message };

			CMessage(GalacticaShared* inGame, long inThingyType = thingyType_Message);
			virtual ~CMessage();
			
// ---- database/streaming methods ----
	virtual void		ReadStream(LStream *inStream, UInt32 streamVersion = version_SavedTurnFile);
	virtual void		WriteStream(LStream *inStream, SInt8 forPlayerNum = kNoSuchPlayer);

// ---- container methods ----

// ---- object info methods ----
	bool				   HasModalWindow() const {return (mPPobResID != 0);}	//only meaningful after BuildClientEvent() has been called
	bool				   HasAutoShow() const {return mEventDisplayInfo.autoShow;}
	bool				   MessageIsToMe() const;
	bool				   MessageIsAboutMe() const;
	bool				   MessageConcernsEveryone() const;
	bool				   IsSubject(AGalacticThingy* inThingy) {return GetSubject() == inThingy;}
	void				   SetSubject(AGalacticThingy* inThingy);
	AGalacticThingy*	   GetSubject() const {return mSubjectWP.GetThingy();}
	Waypoint			   GetSubjectWP() const {return mSubjectWP;}
	std::string*	       GetSubjectNamePtr() {return &mSubjectName;}
	void				   SetRecipients(UInt16 inPlayer, SInt32 inOtherPlayer = kNoOtherPlayers);
	void				   GetRecipients(UInt16 *outPlayer, SInt32 *outOtherPlayer);
	void				   SetAffectedPlayers(UInt16 inPlayer, SInt32 inOtherPlayer = kNoOtherPlayers);
	void				   GetAffectedPlayers(UInt16 *outPlayer, SInt32 *outOtherPlayer);
	void				   LiveUntil(const long inTurnNum) {mLiveUntil = inTurnNum;}
#ifdef GALACTICA_SERVER
	void              SetMessageText(const char* inMessageText) {mMessageStr.assign(inMessageText);}
#endif // GALACTICA_SERVER
	void				   SetEventDisplayInfo(const EventDefPtr inEventDefP) {mEventDisplayInfo = *inEventDefP;}
	void			       SetEventCode(const UInt16 inCode) {mEventCode = inCode;}
	UInt16                 GetEventCode() const {return mEventCode;}
	void				   SetLayrResID(const ResIDT inID) {mLayrResID = inID;}
	ResIDT			       GetLayrResID() const {return mLayrResID;}
	virtual StringPtr	   GetDescriptor(Str255 outDescriptor) const;
	void				   BuildClientEvent();
	void				   RebuildReportString() {mReportStr.clear(); MakeReportString();}
	void				   MakeReportString();
	void				   SetNumber(const SInt32 inNum) {mNum = inNum;}
	SInt32				   GetNumber() const {return mNum;}
	void				   SetFocus(Waypoint inFocus) {mCurrPos = inFocus;}
	short				   GetMyPlayerTypeIndex() const;
	SInt32				   GetLiveUntil() const {return mLiveUntil;}

// ---- host only turn processing methods ----
	virtual void 		   EndOfTurn(long inTurnNum);	//only called from host - will delete itself

// ---- miscellaneous methods ----
	virtual bool	    RefersTo(AGalacticThingy *inThingy);
//	virtual void	    RemoveReferencesTo(AGalacticThingy *inThingy);
	static CMessage*	CreateEvent(GalacticaProcessor* inGame,     // game host that is doing the creating
	                                UInt16 inCode, 			        // event being sent
									UInt16 inRecipient, 	        // to whom
									AGalacticThingy* inSubject = nil,	// about what
									SInt32 inOtherRecipient = kNoOtherPlayers, 	// optional
									UInt16 inAffectedPlayer = 0,		// defaults to recipient
									SInt32 inOtherAffectedPlayer = 0,	// defaults to other recipient
									SInt32 inNum = 0, 				// some extra data
									ResIDT inLayrResID = 0,			// Layr resource to use
									bool inWriteToDB = true);	    // false to write it yourself
													                // after call returns 

// ======= GUI ONLY ==========				
#ifndef GALACTICA_SERVER
   void				   SetMessage(Handle inText, Handle inStyle) {mMessageText = inText; mMessageStyle = inStyle;}

// ---- mouse handling methods ----
	virtual Rect		GetHotSpotRect() const;	// override to return clickable area relative to center pt

// ---- selection methods ----
	virtual void 		Select();

// ---- drawing methods ----
	virtual void		PlayMessage();		// returns true if we should immediately display next message
	virtual void		MessageHandled();
	virtual bool		DoMessage(long inTurnNum);		// shouldn't generally have to override this
	virtual bool		DoMessageSelf(long inTurnNum);	// this is the prefered method to override
#endif // GALACTICA_SERVER

protected:

	SInt32	mLiveUntil;
	SInt32	mLastTurnChecked;		// last turn in which message was played or skipped
	UInt16	mEventCode;
	UInt16	mAffectedPlayer;
	SInt32	mOtherAffectedPlayer;
	SInt32	mOtherOwner;
	SInt32	mNum;
	ResIDT	mSndResID;
	ResIDT	mPPobResID;
	ResIDT	mTEXTResID;
	ResIDT	mSTRxResID;
	ResIDT	mStrIndex;
	ResIDT	mLayrResID;
	Waypoint	mSubjectWP;
	std::string	mSubjectName;
	std::string	mReportStr;
#ifdef GALACTICA_SERVER
	std::string mMessageStr;
#endif // GALACTICA_SERVER
//	PaneIDT		mPreviousMessageID;
	EventDefT	mEventDisplayInfo;
	static EventDefListT* sEventsRes;
#ifndef GALACTICA_SERVER
	Handle		mMessageText;
	Handle		mMessageStyle;
	CSoundResourcePlayer* mSndPlayer;
#endif // GALACTICA_SERVER
};


class CDestructionMsg : public CMessage {
public:
	CDestructionMsg(GalacticaShared* inGame);

#ifndef GALACTICA_SERVER
	virtual bool		DoMessage(long inTurnNum);	// show explosion
protected:
	static CNewGWorld*	sExplosion;
	static RgnHandle    sRefreshRgn;
#endif // GALACTICA_SERVER

};

#endif // CMESSAGE_H_INCLUDED


