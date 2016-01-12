// ===========================================================================
//	CStyledText.cp					
// ===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include "CStyledText.h"
#include <LStream.h>

// ---------------------------------------------------------------------------
//		¥ CStyledText(LStream*)
// ---------------------------------------------------------------------------
//	Contruct an CStyledText from the data in a Stream

CStyledText::CStyledText(LStream *inStream)
 :LTextEditView(inStream) {
 	*inStream >> mUseTextColor;
 	*inStream >> mTextColor.red;
 	*inStream >> mTextColor.green;
 	*inStream >> mTextColor.blue;
	if (mUseTextColor) {	// update the color if we have a color override
	    UseTextColor(mTextColor);
	}
}


void
CStyledText::UseTextColor( RGBColor color ) {
    mTextColor = color;
    mUseTextColor = true;
	TextStyle tStyle;
	tStyle.tsColor = mTextColor;
	::TESetSelect(0, 32767, mTextEditH);
	::TESetStyle(doColor, &tStyle, false, mTextEditH);
}

void
CStyledText::SetTextPtr(
	const void*		inTextP,
	SInt32			inTextLen,
	StScrpHandle	inStyleH )
{
    LTextEditView::SetTextPtr(inTextP, inTextLen, inStyleH);
	if (mUseTextColor) {	// update the color if we have a color override
	    UseTextColor(mTextColor);
	}
}

void
CStyledText::DrawSelf()
{
	RGBColor	backColor;
	GetForeAndBackColors(NULL, &backColor);
    ::RGBBackColor(&backColor);
    LTextEditView::DrawSelf();
}

