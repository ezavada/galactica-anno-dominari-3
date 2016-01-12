/**********************************************************************	CDividedView.h	Written by Rick Eames	�1994 Rick Eames.  All Rights Reserved.	You may use this class in any non-commercial program without	permission.  The only restriction is that you must credit the	author in your about box.  For commercial use, please contact	the author at the email address below.		DESCRIPTION:	This class splits a view into two sub-views that					can be dynamically sized by dragging around a 					dividing line.					Send bug-reports and comments to athos@pendragon.com	**********************************************************************/#pragma once#include <LView.h>#include <LStream.h>#include <LPane.h>const SInt32	kDividerSize = 4;const SInt32 kOutsideSlop = 0x80008000;#define	kHCursor 1001#define kVCursor 1000class CDividedView : public LView{public:	enum {class_ID = 'divV'};							CDividedView();							CDividedView(const CDividedView &inOriginal);							CDividedView(const SPaneInfo &inPaneInfo,											const SViewInfo &inViewInfo,											SInt16 inDividerPos,											SInt16 inMinFirstSize,											SInt16 inMinSecondSize,											Boolean inIsHorizontal);							CDividedView(LStream *inStream);	virtual					~CDividedView();		//virtual			void	InstallViews(LView *inFirstView, LView *inSecondView);		void					InitDividedView();	virtual			void	FinishCreateSelf();// ��� ERZ Addition		virtual			void	DrawSelf();	virtual			void	ClickSelf(const SMouseDownEvent &inMouseDown);						void	InstallFirstView(LView *inFirstView, Boolean autoExpand);					void	InstallSecondView(LView *inSecondView, Boolean autoExpand);		virtual			void	CalcDividerRect(Rect & outRect);	virtual			void	AdjustMouseSelf(Point inPortPt, const EventRecord &inMacEvent, RgnHandle outMouseRgn);		SInt16					GetDividerPos() {return mDividerPos;}	void					SetDividerPos(SInt16 inPos) {mDividerPos = inPos;}			protected:	LView			*mFirstView;	// top or left view	LView			*mSecondView;	// bottom or right view		Cursor			mHCursor;	Cursor			mVCursor;		SInt16			mDividerPos;	// where is the divider?	SInt16			mMinFirstSize;	// mimimum size of top (or left) pane	SInt16			mMinSecondSize;	// minimum size of bottom (or right) pane	Boolean			isHorizontal;	// top/bottom or left/right configuration?	PaneIDT			mTopLeftPaneID; // ��� ERZ Addition	PaneIDT			mBotRightPaneID;// ��� ERZ Addition};