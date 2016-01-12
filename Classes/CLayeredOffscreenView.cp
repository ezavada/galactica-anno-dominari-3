//	CLayeredOffscreenView.cp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// implements an offscreen view that supports the use of a Layr resource
// to describe the order and manner that a series of PICTs are to be overlayed
// to produce a final view that is drawn onscreen.

#include "CLayeredOffscreenView.h"
#include <LStream.h>
#include "DebugMacros.h"
#include <UMemoryMgr.h>
#include "GenericUtils.h"


//	Default Constructor
CLayeredOffscreenView::CLayeredOffscreenView() {
	mLayerArray = nil;
	mLayrResID = resID_Undefined;
}


//	Constructor from input parameters
CLayeredOffscreenView::CLayeredOffscreenView(const LayerListT *inLayers, 
		const SPaneInfo &inPaneInfo, const SViewInfo &inViewInfo)
	: LView(inPaneInfo, inViewInfo) {
//	: LOffscreenView(inPaneInfo, inViewInfo) {
	mLayerArray = nil;
	mLayrResID = resID_Undefined;
	InitLayers(inLayers);
}

//	Another constructor from input parameters
CLayeredOffscreenView::CLayeredOffscreenView(const ResIDT inLayrID,
		const SPaneInfo &inPaneInfo, const SViewInfo &inViewInfo)
	: LView(inPaneInfo, inViewInfo) {
//	: LOffscreenView(inPaneInfo, inViewInfo) {
	mLayrResID = resID_Undefined;
	mLayerArray = nil;
	SetLayrResID(inLayrID);
}

//	Constructor from a Stream
CLayeredOffscreenView::CLayeredOffscreenView(LStream *inStream)
	: LView(inStream) {
//	: LOffscreenView(inStream) {
	ResIDT layrResID;
	mLayerArray = nil;
	mLayrResID = resID_Undefined;
	*inStream >> layrResID;
	SetLayrResID(layrResID);
}


//	Destructor
CLayeredOffscreenView::~CLayeredOffscreenView() {
	DeleteLayers();
}

void
CLayeredOffscreenView::DeleteLayers() {
	if (mLayerArray) {
		LayerDefT layerInfo;
		for (int i = 1; i <= mLayerArray->GetCount(); i++) {	// dispose of the layers
			layerInfo.gWorld = nil;
			mLayerArray->FetchItemAt(i, &layerInfo);
			if (layerInfo.gWorld)
				delete layerInfo.gWorld;
		}
		delete mLayerArray; 	// dispose of array
		mLayerArray = nil;
	}
}

void
CLayeredOffscreenView::SetLayrResID(ResIDT inLayrID) {
	int n = 1;
	if (inLayrID == mLayrResID) {
		return; // the one requested is already loaded
	}
	DeleteLayers();
	LayrResT* layers = 0;
	while (!layers) {
	   	DEBUG_OUT("Load 'Layr' res "<<inLayrID, DEBUG_DETAIL);
		layers = static_cast<LayrResT*>( LoadResource('Layr', inLayrID) );
		if (!layers) {	// if it couldn't be found, get for the the 10s, 100s, 1000s, etc.
			if (inLayrID == 0) {
				break;
		   	}
		   	if (inLayrID == 2000) { // check for failure of stars to load
		      	OSErr err = ::ResError();
   	      		DEBUG_OUT("Load of 'Layr' res "<<inLayrID<<" failed err = "
   	               << err << ", retry...", DEBUG_ERROR);
		   	}
			n *= 10;	// so if we asked for 5121, we would check 5121, 5120, 5100, 5000 and 0
			inLayrID -= (inLayrID % n);
		}
	}
	mLayrResID = inLayrID;
	if (layers) {
 		// endian conversion here
		layers->numLayers = EndianS16_BtoN(layers->numLayers);
		for (int i = 0; i < layers->numLayers; i++) {
			layers->layer[i].PICTid = EndianS16_BtoN(layers->layer[i].PICTid);
			layers->layer[i].pixelDepth = EndianS16_BtoN(layers->layer[i].pixelDepth);
			layers->layer[i].mode = EndianS16_BtoN(layers->layer[i].mode);
			layers->layer[i].flags = EndianU32_BtoN(layers->layer[i].flags);
			layers->layer[i].fgColor.red = EndianS16_BtoN(layers->layer[i].fgColor.red);
			layers->layer[i].fgColor.green = EndianS16_BtoN(layers->layer[i].fgColor.green);
			layers->layer[i].fgColor.blue = EndianS16_BtoN(layers->layer[i].fgColor.blue);
			layers->layer[i].bgColor.red = EndianS16_BtoN(layers->layer[i].bgColor.red);
			layers->layer[i].bgColor.green = EndianS16_BtoN(layers->layer[i].bgColor.green);
			layers->layer[i].bgColor.blue = EndianS16_BtoN(layers->layer[i].bgColor.blue);
			layers->layer[i].opColor.red = EndianS16_BtoN(layers->layer[i].opColor.red);
			layers->layer[i].opColor.green = EndianS16_BtoN(layers->layer[i].opColor.green);
			layers->layer[i].opColor.blue = EndianS16_BtoN(layers->layer[i].opColor.blue);
		}
		InitLayers(layers);
		UnloadResource(layers);
	}
	Refresh();
}

void
CLayeredOffscreenView::InitLayers(const LayerListT *inLayers) {
	mLayerArray = new LArray(sizeof(LayerDefT));
	mLayerArray->AdjustAllocation(inLayers->numLayers);
	for (int i = 0; i < inLayers->numLayers; i++) {		// build the array
		LayerDefT layer = inLayers->layer[i];	// allocate each layer's offscreen GWorld
		InitLayer(&layer);
		mLayerArray->InsertItemsAt(1, LArray::index_Last, &layer);
	}
}

void
CLayeredOffscreenView::InitLayer(LayerDefT *inLayer) {
	if (inLayer->gWorld)
		delete inLayer->gWorld;
	inLayer->gWorld = new CNewGWorld(inLayer->PICTid, inLayer->pixelDepth, inLayer->flags);
}

void
CLayeredOffscreenView::GetNthLayer(ArrayIndexT inWhichLayer, LayerDefT *outLayerInfo) {
	mLayerArray->FetchItemAt(inWhichLayer, outLayerInfo);
}

void
CLayeredOffscreenView::SetNthLayer(ArrayIndexT inWhichLayer, LayerDefT *inLayerInfo) {
	if (inWhichLayer > mLayerArray->GetCount()) {	// adding new entry
		InitLayer(inLayerInfo);
		mLayerArray->InsertItemsAt(1, LArray::index_Last, inLayerInfo);
	} else {	// changing existing entry
		LayerDefT layer = *inLayerInfo;
		mLayerArray->FetchItemAt(inWhichLayer, &layer);	// InitLayer() may want to do something
		inLayerInfo->gWorld = layer.gWorld;				// with the old GWorld, so make sure it
		InitLayer(inLayerInfo);							// knows about it
		mLayerArray->AssignItemsAt(1, inWhichLayer, inLayerInfo);
	}
}

void
CLayeredOffscreenView::DrawSelf() {
	if (!mLayerArray)	// no array at all
		return;
	CGrafPtr	dstPort;
	GDHandle	dstDevice;
	Rect	frame;
	CalcLocalFrameRect(frame);
	LayerDefT	layerInfo;
	EstablishPort();
	::GetGWorld(&dstPort, &dstDevice);
		
	for (int i = 1; i <= mLayerArray->GetCount(); i++) {	// dispose of the layers
		mLayerArray->FetchItemAt(i, &layerInfo);
		CNewGWorld *gWorld = layerInfo.gWorld;
		if (!gWorld)
			continue;
		if (!gWorld->PrepareDrawing())
			continue;
		::SetGWorld(dstPort, dstDevice);
		
		::RGBForeColor(&layerInfo.fgColor);
		::RGBBackColor(&layerInfo.bgColor);
		::OpColor(&layerInfo.opColor);

//		::ForeColor(blackColor);
//		::BackColor(whiteColor);

		gWorld->CopyImage((GrafPtr)dstPort, frame, layerInfo.mode);
		gWorld->EndDrawing();
	}
	LView::OutOfFocus(nil);
}



