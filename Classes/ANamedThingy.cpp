// AGalacticThingyUI.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ 4/18/96
// 9/21/96, 1.0.8 fixes incorporated
// 1/11/98, 1.2, converted to CWPro1
// 9/14/98, 1.2b8, Changed to ThingyRefs, CWPro3
// 4/3/99,	1.2b9, Changed to support Single Player Unhosted Games, CWPro4
// 3/14/02, 2.1b5, Separated from AGalacticThingy

#include "ANamedThingy.h"

#ifndef GALACTICA_SERVER
#include "GalacticaDoc.h"
#include "GalacticaPanes.h"
#endif // GALACTICA_SERVER


extern short gGalacticaFontNum; // defined in GalacticaUtils.cpp


#define DRAG_SLOP_PIXELS 4
// define as 1 to use a single handle as a reusable buffer, 0 to create new handle each time
#define SHARED_THINGY_IO_BUFFER	1

// lowest brightness of thingies other than stars
#define LOWEST_BRIGHTNESS 0.1


ANamedThingy::ANamedThingy(LView *inSuperView, GalacticaShared* inGame, long inThingyType) 
#ifndef GALACTICA_SERVER
  : AGalacticThingyUI(inSuperView, inGame, inThingyType) {
#else
  : AGalacticThingy(inGame, inThingyType) {
#endif // GALACTICA_SERVER
	mNameWidth = 0;
}

/*
ANamedThingy::ANamedThingy(LView *inSuperView, GalacticaShared* inGame, LStream *inStream, long inThingyType)
: AGalacticThingyUI(inSuperView, inGame, inStream, inThingyType) {
	ReadNameStream(*inStream);
}
*/

void 
ANamedThingy::ReadStream(LStream *inStream, UInt32 streamVersion) {
#ifndef GALACTICA_SERVER
	AGalacticThingyUI::ReadStream(inStream, streamVersion);
#else
	AGalacticThingy::ReadStream(inStream, streamVersion);
#endif // GALACTICA_SERVER
	ReadNameStream(*inStream);
}

void 
ANamedThingy::WriteStream(LStream *inStream, SInt8 forPlayerNum) {
	AGalacticThingy::WriteStream(inStream);
	*inStream << (unsigned char) mName.length();
	inStream->WriteBlock(mName.c_str(), mName.length());
	*inStream << mNameWidth;
}

#ifndef GALACTICA_SERVER
void
ANamedThingy::UpdateClientFromStream(LStream *inStream) {
	AGalacticThingy::UpdateClientFromStream(inStream);
	SInt16 nameWidth = mNameWidth;		// preserve the name in case we own this
	std::string s = mName;
	ReadNameStream(*inStream);		// read in the name
	GalacticaDoc* doc = GetGameDoc();
	if (doc) {
	    if ( GetOwner() == doc->GetMyPlayerNum() ) {
        	mNameWidth = nameWidth;			// restore the interface data
        	mName = s;
        }
    }
}
#endif // GALACTICA_SERVER

void
ANamedThingy::UpdateHostFromStream(LStream *inStream) {
	AGalacticThingy::UpdateHostFromStream(inStream);
	ReadNameStream(*inStream);		// read in the name
}

const char*
ANamedThingy::GetName(bool considerVisibility) {
    GalacticaDoc* game = GetGameDoc();
  #ifndef GALACTICA_SERVER
    if (considerVisibility && game) {
        int myPlayerNum = game->GetMyPlayerNum();
        if (!IsAllyOf(myPlayerNum) && (GetVisibilityTo(myPlayerNum) < visibility_Class)) {
            return "?";  // Fog of War
        }
    }
  #endif // GALACTICA_SERVER
	return mName.c_str();
}

#warning NOTE: GetShortName() is not thread safe, must pass in buffer
const char*
ANamedThingy::GetShortName(bool considerVisibility) {
    GalacticaDoc* game = GetGameDoc();
  #ifndef GALACTICA_SERVER
    if (considerVisibility && game) {
        int myPlayerNum = game->GetMyPlayerNum();
        if (!IsAllyOf(myPlayerNum) && (GetVisibilityTo(myPlayerNum) < visibility_Class)) {
            return "?";  // Fog of War
        }
    }
  #endif // GALACTICA_SERVER
    long pos = mName.find('\t');    // find a tab if one exists
    if (pos == (long)std::string::npos) {
        // no tab, return entire name
	    return mName.c_str();
	} else {
	    // tab found, just return name, truncating at tab to leave off the serial number
        static std::string s; // static makes this not thread safe, need to pass in buffer, mutexing won't work
        s = mName.substr(0, pos);
        return s.c_str();
    }
}

#ifndef GALACTICA_SERVER
StringPtr 
ANamedThingy::GetDescriptor(Str255 outDescriptor) const {
    GalacticaDoc* game = GetGameDoc();
    if (game) {
        int myPlayerNum = game->GetMyPlayerNum();
        if (!IsAllyOf(myPlayerNum) && (GetVisibilityTo(myPlayerNum) < visibility_Class)) {
            c2pstrcpy(outDescriptor, "?");  // Fog of War
            return outDescriptor;
        }
    }
    long pos = mName.find('\t');    // find a tab if one exists
    if (pos == std::string::npos) {
        // no tab, return entire name
	    c2pstrcpy(outDescriptor, mName.c_str());
	} else {
	    // tab found, just return name, truncating at tab to leave off the serial number
        std::string s;
        s = mName.substr(0, pos);
        c2pstrcpy(outDescriptor, s.c_str());
    }
	return outDescriptor;
}

void 
ANamedThingy::SetDescriptor(ConstStr255Param inDescriptor) {
	SetName(inDescriptor);
	ThingMoved(kRefresh);	// we changed the name width, so we have to update our frame size
}

// low level use only, doesn't recalc frame size
void
ANamedThingy::SetName(ConstStr255Param inName) {
	mName.assign((const char*)&inName[1], inName[0]);
	SetName(mName.c_str());
}
#endif // GALACTICA_SERVER

void
ANamedThingy::SetName(const char* inName) {
	mName = inName;
  #ifndef GALACTICA_SERVER
	LStr255 tempPascalStr(mName.c_str());
	StTextState saveTextState;
	MacAPI::TextFont(gGalacticaFontNum);
	MacAPI::TextFace(0);
	MacAPI::TextSize(10);
	mNameWidth = MacAPI::StringWidth(tempPascalStr);
  #else
    #warning NOTE: assuming 10 pixels per character for galactica server
    mNameWidth = 10 * std::strlen(inName);   // assume 10 pixels per character
  #endif // GALACTICA_SERVER
}


void
ANamedThingy::ReadNameStream(LStream &inStream) {
    SInt32 strLen;
    char strBuf[256];
    unsigned char tempLen;
	inStream >> tempLen;
	strLen = tempLen;
	inStream.ReadBlock(strBuf, strLen);
	strBuf[strLen] = 0;   // add NUL terminator
	mName = strBuf;         // assign to 
	inStream >> mNameWidth;
}

#ifndef GALACTICA_SERVER

Rect 
ANamedThingy::GetCenterRelativeFrame(bool full) const {	// presumes name goes underneath thingy
	Rect r = AGalacticThingyUI::GetCenterRelativeFrame(full);
	if (full) {
		r.left = Min(-(mNameWidth>>1), r.left);
		r.right = Max(mNameWidth>>1, r.right);
		r.bottom = r.bottom + 12;
	}
	return r;
}

void
ANamedThingy::DrawSelf() {
    InternalDrawSelf(kConsiderVisibility);
}

void
ANamedThingy::InternalDrawSelf(bool considerVisibility) {
	static Str255 s;
//	::PenNormal();
	ASSERT(mSuperView != nil);
	CStarMapView* theStarMap = static_cast<CStarMapView*>(mSuperView);
	if (!theStarMap) {
		return;
	}
	float zoom = theStarMap->GetZoom();
	if (zoom < 0.5) {	// don't draw names when at less than 1/2 zoom (1:2 scale)
		return;
	}
    GalacticaDoc* game = GetGameDoc();
    bool drawNames = true;
    if (game) {
        drawNames = game->DrawNames();
    }
    bool isAlly = false;
    int myPlayerNum = 0;
    if (game) {
        myPlayerNum = game->GetMyPlayerNum();
	    isAlly = IsAllyOf(myPlayerNum) || game->FogOfWarOverride();
    }
	bool canSeeOwner = isAlly || (GetVisibilityTo(myPlayerNum) > visibility_None);
	if ( drawNames && mName.length() ) {
		Rect frame = GetCenterRelativeFrame(kSelectionFrame);
		MacAPI::MacOffsetRect(&frame, mDrawOffset.h, mDrawOffset.v);
		bool selected = (this == mGame->AsSharedUI()->GetSelectedThingy());
		StTextState saveTextState;
		MacAPI::TextSize(10);	//10 pt
		MacAPI::TextFont(gGalacticaFontNum);
		MacAPI::TextFace(0);	//plain
		if (selected || IsHilited()) {
			MacAPI::ForeColor(whiteColor);
			MacAPI::BackColor(blackColor);
			MacAPI::PenMode(srcCopy);
			MacAPI::TextMode(srcCopy); // v1.2b11d6, draw selected object's name over solid background
		}
#ifdef NO_COLORED_THINGY_NAMES
		else if ( isAlly && canSeeOwner ) {
			SetRGBForeColor(0xAAAA,0xAAAA,0xAAAA);
			MacAPI::TextMode(srcOr);
		} else {
			SetRGBForeColor(0x6666,0x6666,0x6666);
			MacAPI::TextMode(srcOr);
		}
#else
		else {
		    RGBColor c;
		    if (game) {
		        int owner;
		        if (isAlly && canSeeOwner) {
		            owner = mOwnedBy;
		        } else {
		            owner = 0;
		        }
			    c = *(game->GetColor(owner, false ));
    			int displayMode = theStarMap->GetDisplayMode();
    			BrightenRGBColor(c, GetBrightness(displayMode));
    			MacAPI::RGBForeColor(&c);
			} else {
			    // host draw names in gray
    			SetRGBForeColor(0x6666,0x6666,0x6666);			    
			}
			MacAPI::TextMode(srcOr);
		}
#endif
		MoveToNamePos(frame);
	 #warning TODO: make this a preferences option whether we do GetName() or GetShortName()
		c2pstrcpy(s, GetShortName(considerVisibility));
		MacAPI::DrawString(s);
		MacAPI::TextMode(srcCopy);
	}
	AGalacticThingyUI::DrawSelf();
}

void
ANamedThingy::MoveToNamePos(const Rect &frame) {
	MacAPI::MoveTo(frame.left + (frame.right - frame.left)/2 - mNameWidth/2, frame.bottom + 8);
}

#endif // GALACTICA_SERVER

