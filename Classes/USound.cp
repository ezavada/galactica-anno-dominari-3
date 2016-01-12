//  USound.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Plays a sound resource asynchronously
//
// created	6/7/96, ERZ
// modified 10/20/96, ERZ, added StopPlaying() to CSoundResPlayer and fixed bug in midi fade-out

#include "GenericUtils.h"

#include "USound.h"
#include <LEventDispatcher.h>
#include <PP_KeyCodes.h>

#define MAX_SOUNDS_IN_PLAY 20

Boolean	CSoundResourcePlayer::sPlaySound = true;
Boolean	CMidiMovieFilePlayer::sPlayMusic = true;

typedef struct SoundInPlayT {
	Handle	sndH;
	short	usage;
} SoundInPlayT;

SoundInPlayT	gSoundsInPlay[MAX_SOUNDS_IN_PLAY];
Boolean			gSoundsArrayInited = false;

void InitSoundsArray();
void PlayingSoundRes(Handle inSndH);
void DonePlayingSoundRes(Handle inSndH);


void 
InitSoundsArray() {
	for (int i = 0; i<MAX_SOUNDS_IN_PLAY; i++) {
		gSoundsInPlay[i].sndH = nil;
		gSoundsInPlay[i].usage = 0;
	}
	gSoundsArrayInited = true;
}

void 
PlayingSoundRes(Handle inSndH) {
	if (!gSoundsArrayInited)
		InitSoundsArray();
	int emptySlot = -1;
	for (int i = 0; i<MAX_SOUNDS_IN_PLAY; i++) {
		if (gSoundsInPlay[i].sndH == inSndH) {
			gSoundsInPlay[i].usage++;
			return;
		} else if ( (emptySlot == -1) && (gSoundsInPlay[i].sndH == nil) )
			emptySlot = i;
	}
	::HLock(inSndH);	// will be locked the first time it's used
	if (emptySlot != -1) {	// if we had a space for it, add it to the list
		gSoundsInPlay[emptySlot].sndH = inSndH;
		gSoundsInPlay[emptySlot].usage = 1;
	}	// if there wasn't any space in the list, it will simply not be unlocked or released
}		// this is bad because it will cause heap fragmentation and memory leaks, but it is
		// better than having crashes from releasing the resource twice.

void 
DonePlayingSoundRes(Handle inSndH) {
	if (!gSoundsArrayInited)
		InitSoundsArray();
	for (int i = 0; i<MAX_SOUNDS_IN_PLAY; i++) {
		if (gSoundsInPlay[i].sndH == inSndH) {
			gSoundsInPlay[i].usage--;
			if (gSoundsInPlay[i].usage <= 0) {
				::HUnlock(inSndH);			// now that no one is using the resource, it can
				::ReleaseResource(inSndH);	// be unlocked and released.
				gSoundsInPlay[i].sndH = nil;	// and it's entry in the list cleared
			}
			return;
		}
	}
}

#pragma mark-
// the sound starts playing immediately upon creation of the object, and plays asynchronously
// until the destructor is called or the sound finishes.

// inCanCut = true means destructor can close the sound channel immediately, cutting off the
// sound in the middle if it is not complete. leaving it as false forces the destructor to wait
// until the sound is complete, tying up the CPU in the meantime (except for Update events)

// the general idea here is you create this object on the stack, at the beginning of a procedure
// that does animation or some such, and the sound plays while the procedure is executing. The
// destructor is called automatically at the end of the proc, ending the sound (or letting it
// finish if inCanCutShort is false)

StSoundResourcePlayer::StSoundResourcePlayer(ResIDT inSndID, long inChannelInit,
											 Boolean inCanCutShort) {
	mCutShort = inCanCutShort;
	mSndChannel = nil;							// no channel yet
	if (CSoundResourcePlayer::sPlaySound)
		mSoundH = ::GetResource(soundListRsrc, inSndID);	// get the sound
	else 
		mSoundH = nil;
	if (mSoundH) {
		PlayingSoundRes(mSoundH);
		if (::SndNewChannel(&mSndChannel, 0, inChannelInit, nil) == noErr)
			::SndPlay(mSndChannel, (SndListHandle)mSoundH, true);
	}
}

StSoundResourcePlayer::~StSoundResourcePlayer() {
    EventRecord macEvent;
	LEventDispatcher* theEventer = LEventDispatcher::GetCurrentEventDispatcher();
    bool dispatchEvent = false;
	if (mSoundH && !mCutShort) {
		SCStatus sndStatus;
		if (theEventer) {
			do {
				::SndChannelStatus(mSndChannel, sizeof(SCStatus), &sndStatus);
				if (sndStatus.scChannelBusy) {
					::WaitNextEvent(updateMask + keyDownMask + keyUpMask + mDownMask + mUpMask, &macEvent, 1, nil);
					if (macEvent.what == updateEvt) {
						theEventer->DispatchEvent(macEvent);
					} else if (macEvent.what == keyDown) {
					    // Galactica specific, pass on tabs
                    	UInt8 theChar = (UInt8) (macEvent.message & charCodeMask);
                    	if (theChar == char_Tab) {
                    	    // pass on tabs
                    	    dispatchEvent = true;
                    	    break;
                    	} else {
                    	    SysBeep(1);
                    	}
					} else if (macEvent.what == mouseDown) {
					    SysBeep(1);
					}
				}
			} while (sndStatus.scChannelBusy);
			mCutShort = true;	// just to make sure it actually kills the sound
		}
	}
	if (mSndChannel) {
		::SndDisposeChannel(mSndChannel, mCutShort);
	}
	if (mSoundH) {
		DonePlayingSoundRes(mSoundH);	// only disposes of the sound when no one else is using it
    }
    if (dispatchEvent) {
        theEventer->DispatchEvent(macEvent);
    }
}


#pragma mark-

// this is similar in usage to the StSoundResourcePlayer, but it will function even if you
// want to leave the procedure from which it was called. It MUST be allocated using the new
// operator and it will delete itself at idle-time, when tying up the CPU waiting for a sound
// to complete is not so horrible. This means the sound will NEVER be cut short (other than
// by system errors or calling StopPlaying() of course)


CSoundResourcePlayer::CSoundResourcePlayer(ResIDT inSndID, long inChannelInit,
											Boolean inAutoDelete, Boolean inWaitToStart) {
	OSErr err = noErr;
	mSndChannel = nil;							// no channel yet
	mAutoDelete = inAutoDelete;
	mWaitToStart = inWaitToStart;
	if (CSoundResourcePlayer::sPlaySound)
		mSoundH = ::GetResource('snd ', inSndID);	// get the sound
	else
		mSoundH = nil;
	if (mSoundH) {
		PlayingSoundRes(mSoundH);
		if (::SndNewChannel(&mSndChannel, 0, inChannelInit, nil) == noErr)
			if (!mWaitToStart)
				err = ::SndPlay(mSndChannel, (SndListHandle)mSoundH, true);
		StartIdling();								// put yourself in the idle queue
	} else {
		mWaitToStart = false;
		if (mAutoDelete)
			delete this;	// no sound available, delete ourselves immediately
	}
}


void
CSoundResourcePlayer::SpendTime(const EventRecord &) {	//inMacEvent
	Boolean done = true;
	SCStatus theStatus;
	if (mSndChannel) {	// check channel status to make sure we don't cut it short
		if (mWaitToStart) {
			OSErr err = ::SndPlay(mSndChannel, (SndListHandle)mSoundH, true);
			mWaitToStart = false;	// no longer waiting, have actually started playing
			done = false;			// can't be done yet, we just started
		} else if (::SndChannelStatus(mSndChannel, sizeof(SCStatus), &theStatus) == noErr)
			done = !theStatus.scChannelBusy;
	}
	if (done) {
		StopPlaying();
		if (mAutoDelete)
			delete this;
	}
}

void
CSoundResourcePlayer::StopPlaying() {
	StopIdling();
	if (mSndChannel)
		::SndDisposeChannel(mSndChannel, true);
	mSndChannel = nil;
	if (mSoundH)
		DonePlayingSoundRes(mSoundH);	// only releases snd res when no one else is using it
}

#pragma mark-

CMidiMovieFilePlayer::CMidiMovieFilePlayer(ResIDT inFileNameSTRid, Boolean inLoop,
											Boolean inCanDeleteSelf) {
	mDone = true;				// haven't started yet
	mCanDelete = inCanDeleteSelf;
	mLoop = inLoop; // && !inCanDeleteSelf;
	mMovie = nil;
	mStarted = false;
	mFading = false;
	mFadeTicks = 180;	// default takes 3 seconds to fade out
	mStartedFadingTickCount = 0;
	mStartedFadingVolume = 255;
	SInt32	qtVersion;
	Boolean bHaveQT = ::Gestalt(gestaltQuickTime, &qtVersion) == noErr;
	OSErr err = featureUnsupported;	// indicate that QuickTime was not available with an error
	if ((CSoundResourcePlayer::sPlaySound) && sPlayMusic && bHaveQT)
		err = ::EnterMovies();
	if (err == noErr) {
		FSSpec	fsSpec;
		LStr255 s(inFileNameSTRid, 0);
		err = ::FSMakeFSSpec(0, 0, s, &fsSpec);
		SInt16	movieRefNum;
		if (err == noErr)
			err = ::OpenMovieFile(&fsSpec, &movieRefNum, fsRdPerm);
		if (err == noErr) {
			SInt16	actualResID = DoTheRightThing;
			Boolean	wasChanged;
			err = ::NewMovieFromFile(&mMovie, movieRefNum, &actualResID,
										nil, newMovieActive, &wasChanged);
			if (err != noErr)
				mMovie = nil;
//			err := LoadMovieIntoRam(mMovie, 0, TimeValue duration, long flags);
			err = ::CloseMovieFile(movieRefNum);
		}
	} else if (bHaveQT)
		::ExitMovies();	// close quicktime immediately.
	if (mMovie) {
		StartIdling();	// don't start playing till we receive an idle event
		mDone = false;
	} else
		mDone = true;	// not available, can't play it
}

CMidiMovieFilePlayer::~CMidiMovieFilePlayer() {
	if (mMovie) {
		if (!mDone)
			::StopMovie(mMovie);
		::DisposeMovie(mMovie);
		::ExitMovies();
	}
}


void
CMidiMovieFilePlayer::SpendTime(const EventRecord &) {	//inMacEvent
	if (!mStarted & !mDone) {
		StopIdling();
		StartRepeating();
		mStarted = true;
		::StartMovie(mMovie);
		mDone = (::GetMoviesError() != noErr);
	}
	if (!CSoundResourcePlayer::sPlaySound || !sPlayMusic) {
		mDone = true;
		::StopMovie(mMovie);
	}
	if (mFading) {
		if (mStartedFadingTickCount == 0) {
			mStartedFadingTickCount = ::TickCount();
			mStartedFadingVolume = ::GetMovieVolume(mMovie);
		}
		UInt32 fadingTicksElapsed = ::TickCount() - mStartedFadingTickCount;
		if (fadingTicksElapsed >= mFadeTicks) {
			mDone = true;
			mLoop = false;
			::StopMovie(mMovie);
		} else {
			float fadeRate = (float)mStartedFadingVolume/(float)mFadeTicks;
			short fadeVolume = (short)((float)fadingTicksElapsed*fadeRate);
//			out << fadingTicksElapsed << " " << fadeVolume << "\n";
			::SetMovieVolume(mMovie, mStartedFadingVolume - fadeVolume);
		}
	}
	if (!mDone) {
		::MoviesTask (mMovie, DoTheRightThing);
		mDone = ::IsMovieDone(mMovie);
	}
	if (mDone) {
		if (mLoop) {
			::GoToBeginningOfMovie(mMovie);
			mDone = false;
		} else {
			StopRepeating();
			if (mCanDelete)
				delete this;
		}
	}
}



