//	OrderFormAttach.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Changes:
//  12/10/01, separated from Galactica.cp


#include "GalacticaUtilsUI.h"

#ifdef MONKEY_BYTE_VERSION

#define CA_SALES_TAX_PERCENT	8
#define REG_CODE_COST			2999
#define CDROM_COST				2999
#define SHIP_US_COST			   595
#define SHIP_MAX_COUNT_BEFORE_ADDITIONAL_COST	2
#define SHIP_ADDITIONAL_COST	250
#define SHIP_OTHER_COST			995
#define ALWAYS_PAY_SALES_TAX  true



// private attachment used with MovableAlert() for the Monkey Byte order form
class COrderFormAttach : public CDialogAttachment {
public:
	COrderFormAttach();
	virtual bool	PrepareDialog();				// return false to abort display of dialog
	virtual void	ExecuteSelf(MessageT inMessage, void* ioParam);
	virtual bool	CloseDialog(MessageT inMsg);	// false to prohibit closing
	void			Perform(MessageT inMessage);
	void			ScrollToEntryField(PaneIDT inPaneID);
	void			CalcBaseShippingCost();
	
	enum {
		// order info panel
		paneID_CDROMCheckbox	= 1,
		paneID_Quantity			= 2,
		paneID_CostPerItem		= 3,
		paneID_TotalOrderCost	= 4,
		paneID_SalesTax			= 5,
		paneID_SalesTaxCheckbox	= 12,
		paneID_Subtotal			= 6,
		paneID_Shipping			= 7,
		paneID_TotalPrice		= 8,
		paneID_RadioShipUS		= 9,
		paneID_RadioShipCanada	= 10,
		paneID_RadioShipOverseas	= 11,
		paneID_OrderSummary		= 37,
		// order info panel
		paneID_VISA				= 13,
		paneID_MasterCard		= 14,
		paneID_AmEx				= 15,
		paneID_CheckMoneyOrder	= 16,
		paneID_CardholdersName	= 17,
		paneID_CardNumber		= 18,
		paneID_ExpirationDate	= 19,
		// billing address panel
		paneID_BillingName		= 20,
		paneID_BillingAddress	= 21,
		paneID_BillingCity		= 22,
		paneID_BillingState		= 23,
		paneID_BillingZip		= 24,
		paneID_BillingCountry	= 25,
		// shipping address panel
		paneID_SameAsBilling	= 26,
		paneID_ShippingName		= 27,
		paneID_ShippingAddress	= 28,
		paneID_ShippingCity		= 29,
		paneID_ShippingState	= 30,
		paneID_ShippingZip		= 31,
		paneID_ShippingCountry	= 32,
		// shipping address panel
		paneID_SendNoUpdates	= 33,
		paneID_EmailAddress		= 34,
		paneID_Phone			= 35,
		paneID_Fax				= 36,
		// the master view in which all this stuff sits
		viewID_OrderForm		= 100
	};
	
	long	quantity;
	long	costPerItem;
	long	totalOrderCost;
	long	caSalesTaxPercent;
	long	caSalesTax;
	long	subtotal;
	long	shipping;
	long	totalPrice;
	bool	shipCD;
	bool	sameAs;
	
	LCommander* mLastTarget;
	LPane*	mQuantityField;
	void Recalculate();
};


#endif	// MONKEY_BYTE_VERSION

