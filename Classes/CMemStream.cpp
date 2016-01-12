// ===========================================================================
// CMemStream.cpp
//
// malloc based implementation of LStream.h from PowerPlant 2.2
//
// (c) 2005, Sacred Tree Software & Ed Zavada
// ===========================================================================
//
//	A Stream whose bytes are in a malloc'd block in memory, but is still 
//  resizable the way an LHandleStream is

#include <CMemStream.h>

#include <cstring>
#include <cstdlib>


// ---------------------------------------------------------------------------
//	¥ LHandleStream							Default Constructor		  [public]
// ---------------------------------------------------------------------------

CMemStream::CMemStream()
{
	mDataP = 0;						// No data yet
	mDataSize = 0;
}


// ---------------------------------------------------------------------------
//	¥ LHandleStream							Copy Constructor		  [public]
// ---------------------------------------------------------------------------
//	Copies data Handle of the original

CMemStream::CMemStream(
	const CMemStream&	inOriginal)

	: LStream(inOriginal)
{
    mDataSize = inOriginal.mDataSize;
    if (inOriginal.mDataP) {
        mDataP = (char*)std::malloc(mDataSize);
        ThrowIfNil_( mDataP );
        std::memcpy(mDataP, inOriginal.mDataP, mDataSize);
    }
}


// ---------------------------------------------------------------------------
//	¥ LHandleStream							Parameterized Constructor [public]
// ---------------------------------------------------------------------------
//	Construct from an existing Handle
//
//	The LHandleStream object assumes ownership of the Handle

CMemStream::CMemStream(char* inPtr, size_t inDataSize)
{
    mDataSize = inDataSize;
	mDataP = inPtr;
	if (mDataP == 0) {
	    mDataSize = 0;
	}
	LStream::SetLength(mDataSize);
}


// ---------------------------------------------------------------------------
//	¥ operator =							Assignment Operator		  [public]
// ---------------------------------------------------------------------------
//	Disposes of existing data Handle and copies data Handle of original

CMemStream&
CMemStream::operator = (
	const CMemStream&	inOriginal)
{
	if (this != &inOriginal) {

		LStream::operator=(inOriginal);

        if (mDataP) {
            std::free(mDataP);
            mDataP = 0;
        }
        mDataSize = inOriginal.mDataSize;
        if (inOriginal.mDataP) {
            mDataP = (char*)std::malloc(mDataSize);
            ThrowIfNil_( mDataP );
            std::memcpy(mDataP, inOriginal.mDataP, mDataSize);
        }
	}

	return *this;
}


// ---------------------------------------------------------------------------
//	¥ ~LHandleStream						Destructor				  [public]
// ---------------------------------------------------------------------------

CMemStream::~CMemStream()
{
    if (mDataP) {
        std::free(mDataP);
        mDataP = 0;
    }
}


// ---------------------------------------------------------------------------
//	¥ SetLength														  [public]
// ---------------------------------------------------------------------------
//	Set the length, in bytes, of the HandleStream

void
CMemStream::SetLength(
	SInt32	inLength)
{
	if (mDataP == 0) {				// Allocate new Handle for data
		mDataP = (char*)std::malloc(inLength);
	} else {							// Resize existing Handle
		mDataP = (char*)std::realloc(mDataP, inLength);
	}
	if (mDataP == 0) {
        throw (ExceptionCode) memFullErr;
    }
    mDataSize = inLength;
	LStream::SetLength(inLength);		// Adjust length count
}


// ---------------------------------------------------------------------------
//	¥ PutBytes
// ---------------------------------------------------------------------------
//	Write bytes from a buffer to a HandleStream
//
//	Returns an error code and passes back the number of bytes actually
//	written, which may be less than the number requested if an error occurred.
//
//	Grows data Handle if necessary.
//
//	Errors:
//		memFullErr		Growing Handle failed when trying to write past
//							the current end of the Stream

ExceptionCode
CMemStream::PutBytes(
	const void*		inBuffer,
	SInt32&			ioByteCount)
{
	ExceptionCode	err = noErr;
	SInt32			endOfWrite = GetMarker() + ioByteCount;

	if (endOfWrite > GetLength()) {		// Need to grow Handle

		try {
			SetLength(endOfWrite);
		}

		catch (ExceptionCode inErr) {	// Grow failed. Write only what fits.
			ioByteCount = GetLength() - GetMarker();
			err = inErr;
		}

		catch (const LException& inException) {
			ioByteCount = GetLength() - GetMarker();
			err = inException.GetErrorCode();
		}
	}
										// Copy bytes into Handle
	if (ioByteCount > 0) {				//   Byte count will be zero if
										//   mDataH is nil
		std::memcpy(mDataP + GetMarker(), inBuffer, ioByteCount);
		SetMarker(ioByteCount, streamFrom_Marker);
	}

	return err;
}


// ---------------------------------------------------------------------------
//	¥ GetBytes
// ---------------------------------------------------------------------------
//	Read bytes from a HandleStream to a buffer
//
//	Returns an error code and passes back the number of bytes actually
//	read, which may be less than the number requested if an error occurred.
//
//	Errors:
//		readErr		Attempt to read past the end of the HandleStream

ExceptionCode
CMemStream::GetBytes(
	void*		outBuffer,
	SInt32&		ioByteCount)
{
	ExceptionCode	err = noErr;
									// Upper bound is number of bytes from
									//   marker to end
	if ((GetMarker() + ioByteCount) > GetLength()) {
		ioByteCount = GetLength() - GetMarker();
		err = readErr;
	}
									// Copy bytes from Handle into buffer
	if (mDataP != 0) {
		std::memcpy(outBuffer, mDataP + GetMarker(), ioByteCount);
		SetMarker(ioByteCount, streamFrom_Marker);
	}

	return err;
}


// ---------------------------------------------------------------------------
//	¥ SetDataHandle
// ---------------------------------------------------------------------------
//	Specify a Handle to use as the basis for a HandleStream
//
//	Class assumes ownership of the input Handle and destroys the
//	existing data Handle. Call DetachDataHandle() beforehand if
//	you wish to preserve the existing data Handle.

void
CMemStream::SetDataPtr(char* inPtr, size_t inDataSize)
{
	if (mDataP) {				// Free existing Handle
		std::free(mDataP);
		mDataP = 0;
	}

	mDataP = inPtr;

	if (mDataP) {
		mDataSize = inDataSize;
	} else {
	    mDataSize = 0;
	}
	LStream::SetLength(mDataSize);

	SetMarker(0, streamFrom_Start);
}


// ---------------------------------------------------------------------------
//	¥ DetachDataHandle
// ---------------------------------------------------------------------------
//	Dissociate the data Handle from a HandleStream.
//
//	Creates a new, empty data Handle and passes back the existing Handle.
//	Caller assumes ownership of the Handle.

char*
CMemStream::DetachDataPtr()
{
	char*	dataP = mDataP;		// Save current data Handle

	SetMarker(0, streamFrom_Start);
	LStream::SetLength(0);
	mDataP = 0;					// Reset to nil Handle
	mDataSize = 0;

	return dataP;
}


