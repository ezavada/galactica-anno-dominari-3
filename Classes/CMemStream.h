// ===========================================================================
// CMemStream.h
//
// malloc based implementation of LStream.h from PowerPlant 2.2
//
// (c) 2005, Sacred Tree Software & Ed Zavada
// ===========================================================================
//
//	A Stream whose bytes are in a malloc'd block in memory, but is still 
//  resizable the way an LHandleStream is


#ifndef MEM_STREAM_H_INCLUDED
#define MEM_STREAM_H_INCLUDED

#include <LStream.h>
#include <cstdlib>


// ---------------------------------------------------------------------------

class	CMemStream : public LStream {
public:

    enum {
        memFullErr = -1,
        readErr    = -2
    };
							CMemStream();

							CMemStream( const CMemStream& inOriginal );

							CMemStream( char* inPtr, size_t inDataSize );

	CMemStream&			    operator = ( const CMemStream& inOriginal );

	virtual					~CMemStream();

	virtual void			SetLength( SInt32 inLength );

	virtual ExceptionCode	PutBytes(
									const void*		inBuffer,
									SInt32&			ioByteCount);

	virtual ExceptionCode	GetBytes(
									void*			outBuffer,
									SInt32&			ioByteCount);

	void					SetDataPtr( char* inPtr, size_t inDataSize );

	char*					GetDataPtr()		{ return mDataP; }

	char*					DetachDataPtr();

protected:
    char*           mDataP;
    size_t          mDataSize;
};


#endif // MEM_STREAM_H_INCLUDED

