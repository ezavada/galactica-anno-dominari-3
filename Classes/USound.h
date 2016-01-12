//  USound.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Plays a sound resource asynchronously
//
// created	6/7/96, ERZ

#ifndef USOUND_H_INCLUDED
#define USOUND_H_INCLUDED

#include <Sound.h>
#include <Movies.h>
#include <LPeriodical.h>


class StSoundResourcePlayer {
public:
		StSoundResourcePlayer(ResIDT inSndID, long inChannelInit = initMono, 
								Boolean inCanCutShort = false);
		~StSoundResourcePlayer();
protected:
	Handle 			mSoundH;
	SndChannelPtr	mSndChannel;
	Boolean			mCutShort;
};



// begins playing the sound, then queues itself to delete itself at the next idle event after
// sound completes.

class CSoundResourcePlayer : public LPeriodical {
public:
		CSoundResourcePlayer(ResIDT inSndID, long inChannelInit = initMono,
								Boolean inAutoDelete = true, Boolean inWaitToStart = false);
	virtual	void	SpendTime(const EventRecord &inMacEvent);
	void			StopPlaying();
	static Boolean	sPlaySound;
protected:
	Handle 			mSoundH;
	SndChannelPtr	mSndChannel;
	Boolean			mAutoDelete;
	Boolean			mWaitToStart;
};

// loads the midi movie, then queues itself to play during event handling,
// deletes itself when finished;

class CMidiMovieFilePlayer : public LPeriodical {
public:
		CMidiMovieFilePlayer(ResIDT inFileNameSTRid, Boolean inLoop = false, 
								Boolean mCanDeleteSelf = false);
		~CMidiMovieFilePlayer();
	virtual	void	SpendTime(const EventRecord &inMacEvent);
	void			FinishNow() {mDone = true;}
	void			DeleteSelfWhenDone() {mCanDelete = true;}
	void			DontLoop() {mLoop = false;}
	void			SetFadeTicks(UInt32 inFadeTicks) {mFadeTicks = inFadeTicks;}
	void			FadeAway() {mFading = true;}
	static Boolean	sPlayMusic;
protected:
	Movie 			mMovie;
	Boolean 		mDone;
	Boolean			mCanDelete;
	Boolean			mLoop;
	Boolean			mStarted;
	Boolean			mFading;
	UInt32			mFadeTicks;
	UInt32			mStartedFadingTickCount;
	short			mStartedFadingVolume;
};


#define player_DeleteSelf		true
#define player_DontDeleteSelf	false
#define player_Loop				true
#define player_DontLoop			false

#endif // USOUND_H_INCLUDED

