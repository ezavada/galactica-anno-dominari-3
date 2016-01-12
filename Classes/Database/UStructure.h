// =============================================================================// UStructure.h								�1996, Sacred Tree Software, inc.// // UStructure is a class for managing structure info for a view of a database//// version 1.3, compiled and tested//// created:   7/23/96, ERZ// modified:  //// =============================================================================// See UStructure.cp for documentation#pragma once#include "DatabaseTypes.h"class LComparator;// FIELD TYPESenum EFieldType {	fieldType_Undefined = 0xffffffff,	fieldType_Flag		= 'flag',	// boolean bit, stored compressed in a bit field	fieldType_Bool		= 'bool',	// C style boolean, 8 bytes, forced to be 0 or 1	fieldType_LongLong	= 'llng',	// 64bit number	fieldType_Long		= 'long',	// 32bit number	fieldType_Short		= 'shrt',	// 16bit number	fieldType_Char		= 'char',	// 8bit number	fieldType_Float		= 'fp  ',	// float	fieldType_Double	= 'dblf',	// double float	fieldType_LongDouble = 'ldbf',	// long double float	fieldType_Fixed		= 'fixd',	// Fixed (long) hiword has int, loword has fract part	fieldType_RecID		= 'id  ',	// id number of another record//	fieldType_Point		= 'pt  ',	// Mac Point structure	//��� should have platform independant//	fieldType_DateTime	= 'dtim',	// Mac DateTime structure	// types for these	fieldType_Data		= 'data',	// variable length block of untyped data, 2GB max size	fieldType_SmallData	= 'sdat',	// variable length block of untyped data, 64k max size	fieldType_TinyData	= 'tdat',	// varaible length block of untyped data, 255 bytes max size	fieldType_Text		= 'text',	// variable length block of text, no length byte, 32k max size	fieldType_PString	= 'pstr'	// pascal string, with length byte, 255 chars max size};#define fieldSize_Flag		 0#define fieldSize_Variable	-1#define fieldSize_Undefined	-2#pragma options align=mac68ktypedef struct FieldInfoT {	EFieldType	fieldType;	// defined as OSType in 'Strc' res	Int32		fieldSize;	// this should be -1 (or 0xFFFFFFFF unsigned) if variable length} FieldInfoT;						// or can be left at 0 to use default size// FLAG type is a boolean bit, need special routines to check for a flag and to extract it from// a bit field.typedef struct StrcResT {	OSType	fileType;		// 4 char code for which ADataStore subclass should be used	Int16	nameSTR_ResID;	// id of STR resource that has logical name of file/view	Int16	namesSTRxResID;	// id of STR# res that contains names of all fields	UInt16	numFields;		// how many fields are there in the view?	FieldInfoT	fields[];	// info on each field} StrcResT, *StrcResPtr, **StrcResHnd, **StrcResHdl;#pragma options align=resettypedef struct FieldXInfoT {	EFieldType	fieldType;	Int32		fieldSize;	Int32		fieldOffset;	// offset from start of record to start of field	UInt8		flagMask;		// used only with flags, all other cases is 0	UInt8		lengthSize;		// used only with var fields, all other cases is 0} FieldXInfoT, *FieldXInfoPtr;	// or to field position ptr if var length fieldclass UStructure;// ===================== CLASS DEFINITIONS =======================typedef class UFieldTypeInfo {friend class UStructure;public:	static void	Init();	virtual void			SetFieldDefaultSize(EFieldType inFieldType, Int32 &ioFieldSize) const;	virtual void			SetFieldLengthSize(EFieldType inFieldType, Int8 &ioLengthSize) const;	virtual LComparator*	GetFieldComparator(EFieldType inFieldType) const;protected:	UFieldTypeInfo();	// never create this directly, UStructure calls UFieldTypeInfo::Init()	static UFieldTypeInfo* sFieldTypeInfo;} UFieldTypeInfo;typedef class UStructure {public:		UStructure(StrcResPtr inStrcRecP);		~UStructure();	Int32		GetFieldOffset(Int16 inFieldNum) const;	Int32		GetFieldSize(Int16 inFieldNum) const;	EFieldType	GetFieldType(Int16 inFieldNum) const;	UInt8		GetFlagMask(Int16 inFieldNum) const;	Int8		GetFieldLengthSize(Int16 inFieldNum) const;	Int32		GetFieldMaxSize(Int16 inFieldNum) const;	LComparator* GetFieldComparator(Int16 inFieldNum) const;	void		GetViewName(Str255 &outName) const;	void		GetFieldName(Int16 inFieldNum, Str255 &outName) const;	Int16		GetNamedFieldNum(Str255 &inName) const;	bool		ValidFieldNum(Int16 &ioFieldNum) const;	Int16		GetFieldCount() const {return mNumFields;}	Int16		GetFixedFieldCount() const {return mNumFixedFields;}	Int16		GetVarFieldCount() const {return mNumVarFields;}	Int16		GetFlagCount() const {return mNumFlags;}	Int32		GetNewRecordSize() const {return mNewRecSize;}protected:	Int16			mNumVarFields;	Int16			mNumFlags;	Int16			mNumFixedFields;	Int16			mNumFields;	Int32			mNewRecSize;	ResIDT			mNameSTR_ID;	ResIDT			mNamesSTRxID;	UFieldTypeInfo*	mFieldTypeInfo;	FieldXInfoT*	mFieldInfo;	// info on the various fields of the viewprivate:	void	InitStructure();} UStructure, *UStructurePtr, **UStructureHnd;