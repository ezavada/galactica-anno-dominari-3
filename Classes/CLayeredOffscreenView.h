//	CLayeredOffscreenView.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// implements an offscreen view that supports the use of a Layr resource
// to describe the order and manner that a series of PICTs are to be overlayed
// to produce a final view that is drawn onscreen.

#pragma once

#include <LOffscreenView.h>
#include <LArray.h>
#include "CNewGWorld.h"

#pragma options align=mac68k
typedef struct {
	ResIDT		PICTid;
	SInt16		pixelDepth;
	SInt16		mode;
	GWorldFlags	flags;
	RGBColor	fgColor;
	RGBColor	bgColor;
	RGBColor	opColor;
	CNewGWorld	*gWorld;	// this field for internal use only
} LayerDefT;

typedef struct {
	SInt16		numLayers;
	LayerDefT	layer[];
} LayerListT, LayrResT;
#pragma options align=reset


class	CLayeredOffscreenView : public LView { //LOffscreenView {
public:
	enum { class_ID = 'lofV' };

			CLayeredOffscreenView();
			CLayeredOffscreenView(const LayerListT *inLayers, const SPaneInfo &inPaneInfo, 
									const SViewInfo	&inViewInfo);
			CLayeredOffscreenView(ResIDT inLayrID, const SPaneInfo &inPaneInfo, 
									const SViewInfo	&inViewInfo);
			CLayeredOffscreenView(LStream *inStream);
	virtual	~CLayeredOffscreenView();

	void			GetNthLayer(ArrayIndexT inWhichLayer, LayerDefT *outLayerInfo);
	void			SetNthLayer(ArrayIndexT inWhichLayer, LayerDefT *inLayerInfo);
	void			SetLayrResID(ResIDT inLayrID);
	virtual void 	DrawSelf();

protected:
	void			InitLayers(const LayerListT *inLayers);
	void			InitLayer(LayerDefT *inLayer);
	void			DeleteLayers();
	ResIDT		mLayrResID;
	LArray*		mLayerArray;
};


