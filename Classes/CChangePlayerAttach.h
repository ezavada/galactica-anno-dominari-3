//   CChangePlayerAttach.h
// ===========================================================================
// Copyright (c) 1996-2003, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Changes:
//   6/7/03, ERZ, Initial version

#ifndef CCHANGEPLAYER_ATTACH_H_INCLUDED
#define CCHANGEPLAYER_ATTACH_H_INCLUDED

#include "GalacticaTypes.h"
#include "GalacticaUtilsUI.h"
#include <LListener.h>
#include <LStdControl.h>

// private attachment class sent to MovableAlert() for use with Game Setup windows
class CChangePlayerAttach : public CDialogAttachment, public LListener {
public:
            CChangePlayerAttach(GalacticaShared* inGame);
   virtual bool   PrepareDialog();            // return false to abort display of dialog
   virtual bool   CloseDialog(MessageT inMsg);   // false to prohibit closing
   virtual void   ListenToMessage(MessageT inMessage, void* ioParam);
   
protected:
   GalacticaShared* mGame;
   LStdPopupMenu  *mPlayer;
   LStdCheckBox   *mHuman;
   LStdCheckBox   *mAssigned;
   LStdCheckBox   *mAlive;
   LStdButton     *mMakeHuman;
   LStdButton     *mMakeComputer;
   LStdButton     *mKillPlayer;
   LStdButton     *mRemovePlayer;
   LStdButton     *mRenamePlayer;

private: 
   CChangePlayerAttach() {}  // don't construct this way
};

#endif // CCHANGEPLAYER_ATTACH_H_INCLUDED

