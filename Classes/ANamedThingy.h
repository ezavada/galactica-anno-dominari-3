// ANamedThingy.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ, 4/18/96
// 3/14/02 ERZ, separated from AGalacticThingy
// 12/27/02 ERZ, separated from AGalacticThingyUI

#ifndef ANAMED_THINGY_H_INCLUDED
#define ANAMED_THINGY_H_INCLUDED

#include "AGalacticThingy.h"

#ifndef GALACTICA_SERVER
#include "AGalacticThingyUI.h"
#endif

class ANamedThingy
#ifndef GALACTICA_SERVER
 : public AGalacticThingyUI
#else
 : public AGalacticThingy
#endif
{

public:
			ANamedThingy(LView *inSuperView, GalacticaShared* inGame, long inThingyType);
//			ANamedThingy(LView *inSuperView, GalacticaShared* inGame, LStream *inStream, long inThingyType);

	virtual void		ReadStream(LStream *inStream, UInt32 streamVersion = version_SavedTurnFile);
	virtual void		WriteStream(LStream *inStream, SInt8 forPlayerNum = kNoSuchPlayer);
	virtual void	    UpdateHostFromStream(LStream *inStream);	// doesn't alter anything controlled by host
    virtual const char* GetName(bool considerVisibility = kConsiderVisibility);
    virtual const char* GetShortName(bool considerVisibility = kConsiderVisibility);
	void                SetName(const char* inName);
  #ifndef GALACTICA_SERVER
	virtual void	    UpdateClientFromStream(LStream *inStream);	// doesn't alter user interface items
	virtual StringPtr	GetDescriptor(Str255 outDescriptor) const;
	virtual void		SetDescriptor(ConstStr255Param inDescriptor);
	void				SetName(ConstStr255Param inName);	// low level, doesn't recalc frame size
	virtual Rect		GetCenterRelativeFrame(bool full = false) const;
	virtual void 		DrawSelf(void);
  #endif // GALACTICA_SERVER
protected:
	std::string mName;
	SInt16		mNameWidth;
	void                InternalDrawSelf(bool considerVisibility);
private:
	void			ReadNameStream(LStream &inStream);
  #ifndef GALACTICA_SERVER
	virtual void	MoveToNamePos(const Rect &frame);	// only call from ANamedThingy::DrawSelf()
  #endif // GALACTICA_SERVER
};

#endif // ANAMED_THINGY_H_INCLUDED


