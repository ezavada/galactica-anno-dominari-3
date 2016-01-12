//	OrderFormAttach.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Changes:
//  12/10/01, separated from Galactica.cp


#include "OrderFormAttach.h"

#include <LDialogBox.h>
#include <UWindows.h>
#include <LEditField.h>
#include <UModalDialogs.h>
#include <UPrinting.h>
#include <LPrintout.h>
#include <LPlaceHolder.h>
#include <LControl.h>

#ifdef MONKEY_BYTE_VERSION

COrderFormAttach::COrderFormAttach()
:CDialogAttachment(msg_AnyMessage, true, false) {	// don't beep
	mLastTarget = NULL;
}

void
COrderFormAttach::ScrollToEntryField(PaneIDT inPaneID) {
	LView* view = dynamic_cast<LView*>(mDialog->FindPaneByID(viewID_OrderForm));
	LPane* pane = mDialog->FindPaneByID(inPaneID);
	if (view && pane) {
		Rect r;
		Point pt;
		SPoint32 p;
		pane->CalcPortFrameRect(r);
		long height = r.bottom - r.top;
		pt.h = 0;
		pt.v = r.top;
		view->PortToLocalPoint(pt);
		view->LocalToImagePoint(pt, p);

		bool scrollTo = false;
		if ( !view->ImagePointIsInFrame(0,p.v)) {
			scrollTo = true;
		}
		if ( !view->ImagePointIsInFrame(0, p.v + height) ) {
			scrollTo = true;
		}
		if (scrollTo) {
			// it wasn't, scroll to a point 300 pixels above the
			// item, thus leaving the item near the top
			long vpos = p.v - 300;
			if (vpos < 0) {	// make sure we don't scroll above the top
				vpos = 0;
			}
			view->ScrollImageTo(0, vpos, true);
		}
	}
}

bool
COrderFormAttach::PrepareDialog() {
// set the window frame size
	Rect windBounds;
	Rect stdBounds;
	mDialog->GetGlobalBounds(windBounds);
	GrafPtr windP = mDialog->GetMacPort();	
//	Rect strucRect = UWindows::GetWindowStructureRect(windP);
	GDHandle dominantDevice = UWindows::FindDominantDevice(windBounds);
	Rect screenRect = (**dominantDevice).gdRect;
	if (dominantDevice == ::GetMainDevice()) {
		screenRect.top += ::GetMBarHeight(); // don't overwrite menu bar on main device
	}
	mDialog->CalcStandardBoundsForScreen(screenRect, stdBounds);// mode
	windBounds.top = stdBounds.top;
	windBounds.bottom = stdBounds.bottom;
	mDialog->DoSetBounds(windBounds);
// prepare the contents
	costPerItem = REG_CODE_COST;
	quantity = 1;
	shipCD = false;
	sameAs = true;
	shipping = SHIP_US_COST;
	caSalesTaxPercent = CA_SALES_TAX_PERCENT;
	Recalculate();
	LEditField* ef = dynamic_cast<LEditField*>(mDialog->FindPaneByID(paneID_CardNumber));
	if (ef) {
		ef->SetKeyFilter(CreditCardField);
	}
	ef = dynamic_cast<LEditField*>(mDialog->FindPaneByID(paneID_Phone));
	if (ef) {
		ef->SetKeyFilter(PhoneNumberField);
	}
	ef = dynamic_cast<LEditField*>(mDialog->FindPaneByID(paneID_Fax));
	if (ef) {
		ef->SetKeyFilter(PhoneNumberField);
	}
	mQuantityField = mDialog->FindPaneByID(paneID_Quantity);
	if (mQuantityField == NULL) {
		::SysBeep(1);
		return false;
	} else {
		return true;
	}
}

void 
COrderFormAttach::ExecuteSelf(MessageT, void*) { //inMessage, ioParam
	LCommander* target = LCommander::GetTarget();
	if (mLastTarget != target) {
		// target changed
		LPane* pane = dynamic_cast<LPane*>(mLastTarget);
		mLastTarget = target;	// do this right away so Perform can safely process events
		if (pane) {
			PaneIDT id = pane->GetPaneID();
			Perform(id);	// handle the message
		}
		LEditField* ef = dynamic_cast<LEditField*>(target);
		// make sure the target entry field is visible to the user
		if (ef) {
			ScrollToEntryField(ef->GetPaneID());
		}
	} else {
		// same target, was it the quantity field?
		if (dynamic_cast<LEditField*>(target) == mQuantityField) {
				// change to quantity, instant recalc
		// same target, so see if quantity field changed
			long newQuant = mQuantityField->GetValue();
			if (newQuant != quantity) {
				Perform(paneID_Quantity);	// recalc the info based on the change
			}
		}
	}
}

#warning TODO: change hardcoded strings in order form to resources

#define SET_PANE_VALUE(paneid, valuename)	\
	pane = mDialog->FindPaneByID(paneid);	\
	if (pane) {								\
		s.Assign(currencySign.c_str());							\
		s.Append(valuename);				\
		s.Insert(currencyDecimal.c_str(), 1, s.Length()-1);		\
		pane->SetDescriptor(s);				\
	}

#define SET_PANE_TEXT(paneid, text)			\
	pane = mDialog->FindPaneByID(paneid);	\
	if (pane) {								\
	   Str255 s;                     \
	   c2pstrcpy(s, text.c_str());   \
		pane->SetDescriptor(s);			\
	}

#define PARAM_TEXT(text)  { Str255 s; c2pstrcpy(s, text.c_str()); ::ParamText(s, "\p", "\p", "\p"); }

void
COrderFormAttach::Recalculate() {
	totalOrderCost = costPerItem * quantity;
	// calc sales tax with rounding
	caSalesTax = totalOrderCost * caSalesTaxPercent;
	if ((caSalesTax % 100) >= 50) {
		caSalesTax /= 100;
		caSalesTax += 1;
	} else {
		caSalesTax /= 100;
	}
	subtotal = totalOrderCost + caSalesTax;
	// add in shipping if we are actually sending something
	if (shipCD) {
		if (quantity > SHIP_MAX_COUNT_BEFORE_ADDITIONAL_COST) {
			shipping += ((quantity - SHIP_MAX_COUNT_BEFORE_ADDITIONAL_COST) * SHIP_ADDITIONAL_COST);
		}
		totalPrice = subtotal + shipping;
	} else {
		totalPrice = subtotal;
	}
	// values calculated, now update display
	LStr255 s;
	LPane* pane;
	std::string currencySign;
	std::string currencyDecimal;
	std::string notApplicable;
	LoadStringResource(currencySign, STRx_OrderForm, str_CurrencySign);
	LoadStringResource(currencyDecimal, STRx_OrderForm, str_CurrencyDecimal);
	LoadStringResource(notApplicable, STRx_OrderForm, str_NotApplicable);
	SET_PANE_VALUE(paneID_CostPerItem, costPerItem);
	SET_PANE_VALUE(paneID_TotalOrderCost, totalOrderCost);
//#define str_NotApplicable     1
	if (caSalesTax != 0) {
		SET_PANE_VALUE(paneID_SalesTax, caSalesTax);
	} else {
		SET_PANE_TEXT(paneID_SalesTax, notApplicable);
	}
	SET_PANE_VALUE(paneID_Subtotal, subtotal);
	if (shipCD) {
		SET_PANE_VALUE(paneID_Shipping, shipping);
	} else {
		SET_PANE_TEXT(paneID_Shipping, notApplicable);
	}
	SET_PANE_VALUE(paneID_TotalPrice, totalPrice);
	LStr255 description;
	if (quantity == 0) {
		description.Assign(STRx_OrderForm, str_EnterQuantity);   // Please enter the quantity in the box to the left
	} else {
		description = quantity;
		if (shipCD) {
		   std::string copies;
			if (quantity == 1) {
			   LoadStringResource(copies, STRx_OrderForm, str_CopyOf);  // copy of
			} else {
			   LoadStringResource(copies, STRx_OrderForm, str_CopiesOf); // copies of
			}
			description += copies.c_str();
		} else {
		   std::string codes;
			if (quantity == 1) {
			   LoadStringResource(codes, STRx_OrderForm, str_RegCode);  // registration code
			} else {
			   LoadStringResource(codes, STRx_OrderForm, str_RegCodes); // registration codes
			}
			description += codes.c_str();
         std::string forStr;
         LoadStringResource(forStr, STRx_OrderForm, str_For);  // for
			description += forStr.c_str();
		}
      std::string productWill;
      LoadStringResource(productWill, STRx_OrderForm, str_ProductWillBe);  // Galacticaª: anno Dominari will be
		description += productWill.c_str();
		if (shipCD) {
		   std::string shippedOnCD;
			if (quantity == 1) {
			   LoadStringResource(shippedOnCD, STRx_OrderForm, str_ShippedOnCD);  // shipped to you on CD-ROM
			} else {
			   LoadStringResource(shippedOnCD, STRx_OrderForm, str_ShippedOnCDs); // shipped to you on CD-ROMs
			}
			description += shippedOnCD.c_str();
		} else {
		   std::string emailed;
         LoadStringResource(emailed, STRx_OrderForm, str_EmailedNoCD);  // emailed to you. You will not receive a CD-ROM
			description += emailed.c_str();
		}
	}
	char buffer[256];
	p2cstrcpy(buffer, description);
	std::string descriptStr(buffer);
	SET_PANE_TEXT(paneID_OrderSummary, descriptStr);
}

bool
COrderFormAttach::CloseDialog(MessageT inMsg) {
	if (inMsg == msg_Cancel) {
		return true;
	} else if (inMsg == msg_OK) {
		Str255 ds;
		LStr255 s;
		std::string msg;
		LPane* pane;
		pane = mDialog->FindPaneByID(paneID_EmailAddress);
		// need to email registration code, make sure we have email address
		if (pane) {
			s = pane->GetDescriptor(ds);
			if (s.Length() < 7) {
			   LoadStringResource(msg, STRx_OrderForm, str_NeedEmail);
			   PARAM_TEXT(msg); // We need your email address to send you the registration code
				UModalAlerts::Alert(alert_GenericErrorAlert);
				LCommander::SwitchTarget(dynamic_cast<LEditField*>(pane));
				return false;
			}
		}
		if (shipCD) {	// definately need shipping address to send CDROMs
			pane = mDialog->FindPaneByID(paneID_ShippingAddress);
			if (pane) {
				s = pane->GetDescriptor(ds);
				if (s.Length() < 5) {
			      LoadStringResource(msg, STRx_OrderForm, str_NeedShipTo);
			      PARAM_TEXT(msg);  // We need your shipping address to send the CD-ROM
					UModalAlerts::Alert(alert_GenericErrorAlert);
					LCommander::SwitchTarget(dynamic_cast<LEditField*>(pane));
					return false;
				}
			}
		}
		// enable/disable credit card related panes
		pane = mDialog->FindPaneByID(paneID_CheckMoneyOrder);
		if (pane && (pane->GetValue() != 1)) {
			// paying by credit card
			pane = mDialog->FindPaneByID(paneID_CardholdersName);
			if (pane) {
				s = pane->GetDescriptor(ds);
				if (s.Length() < 3) {
			      LoadStringResource(msg, STRx_OrderForm, str_NeedCardHolder);
			      PARAM_TEXT(msg); // We need the name of the credit card holder if you want to pay by credit card
					UModalAlerts::Alert(alert_GenericErrorAlert);
					LCommander::SwitchTarget(dynamic_cast<LEditField*>(pane));
					return false;
				}
			}
			pane = mDialog->FindPaneByID(paneID_CardNumber);
			if (pane) {
				s = pane->GetDescriptor(ds);
				if (s.Length() < 12) {
			      LoadStringResource(msg, STRx_OrderForm, str_NeedCardNumber);
			      PARAM_TEXT(msg); // We need the card number if you want to pay by credit card
					UModalAlerts::Alert(alert_GenericErrorAlert);
					LCommander::SwitchTarget(dynamic_cast<LEditField*>(pane));
					return false;
				}
			}
			pane = mDialog->FindPaneByID(paneID_ExpirationDate);
			if (pane) {
				s = pane->GetDescriptor(ds);
				if (s.Length() < 5) {
			      LoadStringResource(msg, STRx_OrderForm, str_NeedCardExpDate);
			      PARAM_TEXT(msg); // We need the credit card's expiration date if you want to pay by credit card
					UModalAlerts::Alert(alert_GenericErrorAlert);
					LCommander::SwitchTarget(dynamic_cast<LEditField*>(pane));
					return false;
				}
			}
		}
		// everything has been "validated", so now print out the form
		// ERZ, 10/13/01, updated for Carbon
    	LPrintSpec printRec	= UPrinting::GetAppPrintSpec();
		if (!UPrinting::AskPageSetup(printRec)) {
			return false;	// user canceled
		}
		LPrintout* form = new LPrintout();
		LPlaceHolder* ph = new LPlaceHolder();
		ph->PutInside(form, true);
		form->ExpandSubPane(ph, true, true);
		ph->InstallOccupant(mDialog->FindPaneByID(viewID_OrderForm), kAlignHorizontalCenter);
		if (!UPrinting::AskPrintJob(printRec)) {
			delete form;
			return false;	// user canceled
		} else {
//			form->EstablishPort();
			form->DoPrintJob();		// print it
		}
		delete form;
		return false;	// we don't want them to have to re-enter everything if their
						// printout didn't work.
	} else {
		Perform(inMsg);
		return false;
	}
}

void
COrderFormAttach::CalcBaseShippingCost() {
	LControl* cb;
	// make sure we have the correct shipping cost, too.
	cb = (LControl*) mDialog->FindPaneByID(paneID_RadioShipUS);
	if (cb) {
		if (cb->GetValue() == 1) {
			shipping = SHIP_US_COST;
		} else {
			shipping = SHIP_OTHER_COST;
		}
	}
	// ship cost will be adjusted for quantity in the Recalculate() method
}

void
COrderFormAttach::Perform(MessageT inMsg) {
	LControl* cb;
	LPane* pane;
	LPane* src;
	LPane* dst;
	Str255 ds;
	switch (inMsg) {

		case paneID_Quantity:
			pane = mDialog->FindPaneByID(inMsg);
			if (pane) {
				quantity = pane->GetValue();
			}
			CalcBaseShippingCost();
			break;
			// deliberately fall through and re-evaluate the ship cost

		case paneID_CDROMCheckbox: 
			cb = (LControl*) mDialog->FindPaneByID(inMsg);
			if (cb) {
				if (cb->GetValue() == 1) {
					shipCD = true;
					costPerItem = CDROM_COST;
				} else {
					shipCD = false;
					costPerItem = REG_CODE_COST;
				}
			}
			CalcBaseShippingCost();
			break;
		
		case paneID_SalesTaxCheckbox:
			cb = (LControl*) mDialog->FindPaneByID(inMsg);
			if (cb) {
				if (cb->GetValue() == 1) {
					caSalesTaxPercent = CA_SALES_TAX_PERCENT;
				} else {
					caSalesTaxPercent = 0;
				}
			}
			break;
			
		case paneID_CardholdersName:
			src = mDialog->FindPaneByID(paneID_CardholdersName);
			dst = mDialog->FindPaneByID(paneID_BillingName);
			if (src && dst) {
				dst->SetDescriptor(src->GetDescriptor(ds));
			}
			// fall through to set shipping name, too.
		case paneID_BillingName:
			if (sameAs) {
				src = mDialog->FindPaneByID(paneID_BillingName);
				dst = mDialog->FindPaneByID(paneID_ShippingName);
				if (src && dst) {
					dst->SetDescriptor(src->GetDescriptor(ds));
				}
			}
			break;
	
		case paneID_BillingAddress:
			if (sameAs) {
				src = mDialog->FindPaneByID(paneID_BillingAddress);
				dst = mDialog->FindPaneByID(paneID_ShippingAddress);
				if (src && dst) {
					dst->SetDescriptor(src->GetDescriptor(ds));
				}
			}
			break;
	
		case paneID_BillingCity:
			if (sameAs) {
				src = mDialog->FindPaneByID(paneID_BillingCity);
				dst = mDialog->FindPaneByID(paneID_ShippingCity);
				if (src && dst) {
					dst->SetDescriptor(src->GetDescriptor(ds));
				}
			}
			break;
	
		case paneID_BillingState:
			pane = mDialog->FindPaneByID(inMsg);
			if (pane) {
			   std::string msg;
				LStr255 stateStr = pane->GetDescriptor(ds);
				stateStr.SetCompareFunc(LString::CompareIgnoringCase);
				if (stateStr.CompareTo("\pCA") == 0) {
					// billing to california, check for ca sales tax
					if (caSalesTaxPercent == 0) {
						cb = (LControl*)mDialog->FindPaneByID(paneID_SalesTaxCheckbox);	// double check that we have a valid item
						if (cb) {
							cb->SimulateHotSpotClick(1);
						}
   			      LoadStringResource(msg, STRx_OrderForm, str_CAResident);
   			      PARAM_TEXT(msg); // Since are a California resident, you must pay sales tax. I've checked the CA Sales Tax box for you
						UModalAlerts::Alert(alert_GenericErrorAlert);
					}
				} else {
					// not billing to california, check for ca sales tax
					if (caSalesTaxPercent != 0) {
						cb = (LControl*)mDialog->FindPaneByID(paneID_SalesTaxCheckbox);	// double check that we have a valid item
						if (cb) {
							cb->SimulateHotSpotClick(1);
						}
                  LoadStringResource(msg, STRx_OrderForm, str_CANonResident);
   			      PARAM_TEXT(msg); // Since are not a California resident, you don't need to pay sales tax. I've unchecked CA Sales Tax box for you
						UModalAlerts::Alert(alert_GenericErrorAlert);
					}
				}
				// is Shipping address same?
				if (sameAs) {
					dst = mDialog->FindPaneByID(paneID_ShippingState);
					if (dst) {
						dst->SetDescriptor(stateStr);
					}
				}
			}
			break;
		
		case paneID_BillingZip:
			if (sameAs) {
				src = mDialog->FindPaneByID(paneID_BillingZip);
				dst = mDialog->FindPaneByID(paneID_ShippingZip);
				if (src && dst) {
					dst->SetDescriptor(src->GetDescriptor(ds));
				}
			}
			break;
	
		case paneID_BillingCountry:
			if (!sameAs) {	// nothing to do here if shipping and billing address aren't same
				break;
			}
			src = mDialog->FindPaneByID(paneID_BillingCountry);
			dst = mDialog->FindPaneByID(paneID_ShippingCountry);
			if (src && dst) {
				dst->SetDescriptor(src->GetDescriptor(ds));
			}
			// fall through and evaluate the shipping country

		case paneID_ShippingCountry:
			pane = mDialog->FindPaneByID(paneID_RadioShipUS);
			if (pane && (pane->GetValue() != 0)) {
				pane = mDialog->FindPaneByID(paneID_ShippingCountry);
				if (pane) {
					LStr255 s = pane->GetDescriptor(ds);
					if (s.Length() > 0) {
					   std::string msg;
   			      LoadStringResource(msg, STRx_OrderForm, str_ShipMethodError);
   			      PARAM_TEXT(msg); // You've entered a Country to ship to, but Continental US is your shipping option. Please fix this
						UModalAlerts::Alert(alert_GenericErrorAlert);
					}
				}
			}
			break;
	
		case paneID_SameAsBilling:
			cb = (LControl*) mDialog->FindPaneByID(inMsg);
			if (cb) {
				if (cb->GetValue() == 1) {
					sameAs = true;
					// shipping address is same as billing address, copy it
					src = mDialog->FindPaneByID(paneID_BillingName);
					dst = mDialog->FindPaneByID(paneID_ShippingName);
					if (src && dst) {
						dst->SetDescriptor(src->GetDescriptor(ds));
					}
					src = mDialog->FindPaneByID(paneID_BillingAddress);
					dst = mDialog->FindPaneByID(paneID_ShippingAddress);
					if (src && dst) {
						dst->SetDescriptor(src->GetDescriptor(ds));
					}
					src = mDialog->FindPaneByID(paneID_BillingCity);
					dst = mDialog->FindPaneByID(paneID_ShippingCity);
					if (src && dst) {
						dst->SetDescriptor(src->GetDescriptor(ds));
					}
					src = mDialog->FindPaneByID(paneID_BillingState);
					dst = mDialog->FindPaneByID(paneID_ShippingState);
					if (src && dst) {
						dst->SetDescriptor(src->GetDescriptor(ds));
					}
					src = mDialog->FindPaneByID(paneID_BillingZip);
					dst = mDialog->FindPaneByID(paneID_ShippingZip);
					if (src && dst) {
						dst->SetDescriptor(src->GetDescriptor(ds));
					}
					src = mDialog->FindPaneByID(paneID_BillingCountry);
					dst = mDialog->FindPaneByID(paneID_ShippingCountry);
					if (src && dst) {
						dst->SetDescriptor(src->GetDescriptor(ds));
					}
				} else {
					sameAs = false;
				}
			}
			break;

		case msg_ControlClicked:	// all radio buttons seem to send this
			CalcBaseShippingCost();
			// enable/disable credit card related panes
			cb = (LControl*) mDialog->FindPaneByID(paneID_CheckMoneyOrder);
			if (cb) {
				if (cb->GetValue() == 1) {
					pane = mDialog->FindPaneByID(paneID_CardholdersName);
					if (pane) {
						pane->Disable();
					}
					pane = mDialog->FindPaneByID(paneID_CardNumber);
					if (pane) {
						pane->Disable();
					}
					pane = mDialog->FindPaneByID(paneID_ExpirationDate);
					if (pane) {
						pane->Disable();
					}
					// disable all credit card related panes
				} else {
					// enable all credit card related panes
					pane = mDialog->FindPaneByID(paneID_CardholdersName);
					if (pane) {
						pane->Enable();
					}
					pane = mDialog->FindPaneByID(paneID_CardNumber);
					if (pane) {
						pane->Enable();
					}
					pane = mDialog->FindPaneByID(paneID_ExpirationDate);
					if (pane) {
						pane->Enable();
					}
				}
			}
					
			break;

		default:
//			::SysBeep(1);
			break;
	}
	Recalculate();
}
#endif	// MONKEY_BYTE_VERSION

