//   CChangePlayerAttach.h
// ===========================================================================
// Copyright (c) 1996-2003, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Changes:
//   6/7/03, ERZ, Initial version

#include "GalacticaHostDoc.h"
#include "GalacticaClient.h"
#include "CChangePlayerAttach.h"
#include "Galactica.h"

CChangePlayerAttach::CChangePlayerAttach(GalacticaShared* inGame)
:CDialogAttachment(msg_AnyMessage, true, false), mGame(inGame) {
}

bool
CChangePlayerAttach::PrepareDialog() {
   // setup variables
	mPlayer = (LStdPopupMenu*)mDialog->FindPaneByID(popupMenu_Player);
	ThrowIfNil_(mPlayer);
	mPlayer->AddListener(this);
	mHuman = (LStdCheckBox*)mDialog->FindPaneByID(checkbox_Human);
	ThrowIfNil_(mHuman);
	mAssigned = (LStdCheckBox*)mDialog->FindPaneByID(checkbox_Assigned);
	ThrowIfNil_(mAssigned);
	mAlive = (LStdCheckBox*)mDialog->FindPaneByID(checkbox_Alive);
	ThrowIfNil_(mAlive);
	mMakeHuman = (LStdButton*)mDialog->FindPaneByID(button_MakeHuman);
	ThrowIfNil_(mMakeHuman);
	mMakeHuman->SetEnabled(false);
	mMakeComputer = (LStdButton*)mDialog->FindPaneByID(button_MakeComputer);
	ThrowIfNil_(mMakeComputer);
	mMakeComputer->SetEnabled(false);
	mKillPlayer = (LStdButton*)mDialog->FindPaneByID(button_KillPlayer);
	ThrowIfNil_(mKillPlayer);
	mKillPlayer->SetEnabled(false);
	mRemovePlayer = (LStdButton*)mDialog->FindPaneByID(button_RemovePlayer);
	ThrowIfNil_(mRemovePlayer);
	mRemovePlayer->SetEnabled(false);
	mRenamePlayer = (LStdButton*)mDialog->FindPaneByID(button_RenamePlayer);
	ThrowIfNil_(mRenamePlayer);
	mRenamePlayer->SetEnabled(false);
   // populate player popup menu with player names
	MenuHandle theMenu = mPlayer->GetMacMenuH();	// Get the menu handle
	for (int i = 1; i <= mGame->GetNumPlayers(); i++) {
		std::string name;
		bool knownPlayer = mGame->GetPlayerNameAndStatus(i, name);	// get the name of the Nth player
		Str255 tmpstr;
		c2pstrcpy(tmpstr, name.c_str());
		::AppendMenuItemText(theMenu, tmpstr);    // add it to the end of the menu
		if (!knownPlayer) {				         // did we find someone who is not a known player?
			::SetItemStyle(theMenu, i, italic);
		}
	}
	mPlayer->SetMaxValue(mGame->GetNumPlayers()); // since we manually added items, make sure the control knows this

   return true;
}

bool
CChangePlayerAttach::CloseDialog(MessageT inMsg) {
	bool canClose = true;		// this is called each time a control within the dialog
	bool sendInfo = true;
	PublicPlayerInfoT info;
	mGame->GetPlayerInfo(mPlayer->GetValue(), info);
	switch (inMsg) {				// broadcasts a message
		case msg_Cancel:
			sendInfo = false;
			break;
		case button_MakeHuman:
		   info.isComputer = false;
			break;
		case button_KillPlayer:
		   info.isAlive = false;
			break;
		case button_RemovePlayer:
		   info.isAssigned = false;
			break;
		case button_MakeComputer:
		   info.isComputer = true;
		   if (info.isAssigned) {
			   break;   // fall through and get the computer name if this is an unassigned human slot
			}           // because there will be no name associated
		case button_RenamePlayer:
   		{
   		   std::string theName(info.name);
   		   if (GalacticaApp::GetLogin(theName, false) ) {  // false so we don't update preferences with this name
   		      std::strcpy(info.name, theName.c_str());
   		   } else {
      		   canClose = false;
      		   sendInfo = false;
      		}
   		}
			break;
		default:
		   return false;
	}
	if (sendInfo) {
	   AdminSetPlayerInfoPacket* p = CreatePacket<AdminSetPlayerInfoPacket>();
	   p->playerNum = mPlayer->GetValue();
	   p->info = info;
	   if (mGame->IsHost()) {
	      GalacticaHostDoc* host = dynamic_cast<GalacticaHostDoc*>(mGame);
	      host->HandleAdminSetPlayerInfo(p);
	   } else {
	      GalacticaClient* client = dynamic_cast<GalacticaClient*>(mGame);
	      client->SendPacket(p);
	   }
	   ReleasePacket(p);
	}
	return canClose;
}

void
CChangePlayerAttach::ListenToMessage(MessageT inMessage, void* ioParam) {
   if (inMessage == popupMenu_Player) {
      SInt32 value = *((SInt32*)(ioParam));
      // choose a player, get their info
      PublicPlayerInfoT info;
      mGame->GetPlayerInfo(value, info);
      if (info.isComputer) {
         mHuman->SetValue(0);
         mMakeComputer->SetEnabled(false);
         mMakeHuman->SetEnabled(true);
         mRemovePlayer->SetEnabled(false);
         info.isAssigned = true;
      } else {
         mHuman->SetValue(1);
         mMakeComputer->SetEnabled(true);
         mMakeHuman->SetEnabled(false);
         mRemovePlayer->SetEnabled(info.isAssigned);
      }
      mAssigned->SetValue(info.isAssigned);
      mKillPlayer->SetEnabled(info.isAlive);
      mAlive->SetValue(info.isAlive);
      mRenamePlayer->SetEnabled(info.isAssigned);
   }
}


