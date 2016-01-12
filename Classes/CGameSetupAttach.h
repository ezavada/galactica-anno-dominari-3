//   CGameSetupAttach.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Changes:
//   4/15/02, ERZ, 2.1b7, separated from Galactica.cpp

#ifndef CGAMESETUP_ATTACH_H_INCLUDED
#define CGAMESETUP_ATTACH_H_INCLUDED

#include "GalacticaTypes.h"
#include "GalacticaUtilsUI.h"
#include <LListener.h>
#include <LEditField.h>
#include <LStdControl.h>

// private attachment class sent to MovableAlert() for use with Game Setup windows
class CGameSetupAttach : public CDialogAttachment, public LListener {
public:
            CGameSetupAttach(bool isHosting);
            CGameSetupAttach(GameInfoT* ioGameSettings);
   virtual bool   PrepareDialog();            // return false to abort display of dialog
   virtual bool   CloseDialog(MessageT inMsg);   // false to prohibit closing
   virtual void   ExecuteSelf(MessageT inMessage, void* ioParam);
   virtual void   ListenToMessage(MessageT inMessage, void* ioParam);
   
protected:
   void           GetInfo(NewGameInfoT *outInfo);
   LEditField     *mGameTitle;
   LEditField     *mNumHumans;
   LEditField     *mNumComputers;
   LEditField     *mMaxTime;
   LStdPopupMenu  *mCompSkill;
   LStdPopupMenu  *mCompAdvantage;
   LStdPopupMenu  *mDensity;
   LStdPopupMenu  *mSectorSize;
   LStdPopupMenu  *mTimeUnits;
   LStdCheckBox   *mTimeLimit;
   LStdCheckBox   *mFastGame;
   LStdCheckBox   *mFogOfWar;
   LStdCheckBox   *mEndEarly;
   LView          *mTimeLimitView;
   GameInfoT      *mCurrGameInfoP;
   bool           mIsHosting; // do we need to create a multiplayer host or not?
};

#endif // CGAMESETUP_ATTACH_H_INCLUDED

