// ===========================================================================
//	CStyledText.h
// ===========================================================================

#ifndef CSTYLED_TEXT_H_INCLUDED
#define CSTYLED_TEXT_H_INCLUDED

#include <LTextEditView.h>

class CStyledText : public LTextEditView {
public:
	enum { class_ID = 'STxt' };

						CStyledText(LStream *inStream);

    void            UseTextColor( RGBColor color );
    virtual void    SetTextPtr(const void*inTextP, SInt32 inTextLen, StScrpHandle inStyleH);
	virtual void	DrawSelf();
	
/*	virtual Boolean		ObeyCommand(CommandT inCommand, void *ioParam);
	void				SetStyledText(Handle inTextH, Handle inStyleH);
	Handle				GetTextStyleHandle();

private:
	void				InitStyledText(ResIDT inTEXTstylID);
*/
protected:
	Boolean		mUseTextColor;	// true means mTextColor is valid
	RGBColor	mTextColor;		// color to which all text will be set
};

#endif // CSTYLED_TEXT_H_INCLUDED


