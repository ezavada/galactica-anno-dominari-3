//	CThingyList.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#ifndef CTHINGYLIST_H_INCLUDED
#define CTHINGYLIST_H_INCLUDED

#include <TArray.h>
#include "GalacticaUtils.h"

#define kAllowShipsOnly true
#define kAllowAnything  false

class CThingyList : public TArray<ThingyRef> {
public:
					CThingyList(
							bool            inShipsOnly,
							LComparator		*inComparator = CThingyRefComparator::GetComparator(),
							bool			inKeepSorted = false)
								: TArray<ThingyRef>(inComparator,
									inKeepSorted), 
								  mIsShipsOnly(inShipsOnly) 
								  { mOwnsComparator = false;}

	virtual			~CThingyList() { }

	void 		ReadStream(LStream& inInputStream, UInt32 streamVersion = version_SavedTurnFile);
	void		WriteStream(LStream& inOutputStream);
	void		DoForEntireList(UArrayAction &inAction) {
					UArrayAction::DoForEachElement(*this, inAction);}
					
	AGalacticThingy*	FetchThingyAt(ArrayIndexT inAtIndex) const;
	ThingyRef&			FetchThingyRefAt(ArrayIndexT inAtIndex) const;
	void        ValidateList();
protected:
    bool        mIsShipsOnly;
};

#endif // CTHINGYLIST_H_INCLUDED

