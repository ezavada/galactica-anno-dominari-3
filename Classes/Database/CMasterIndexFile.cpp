// =============================================================================
// CMasterIndexFile.cp                     ©1995-96, Sacred Tree Software, inc.
// 
// MasterIndexFile.cp contains routines needed for a LFileStream based
// index file object for a database management system.
//
// depends upon AMasterIndex, LFileStream.h & UException.h
// LFileStream is OS Dependant
// only OS dependancy in this file is use of Aliases and FSSpecs
//
// General Purpose Access Methods:
//	SInt32		FindEntry(RecIDT inRecID, SInt32 *outRecSize = NULL)
//							returns the position and size of specified record
//	SInt32		AddEntry(RecIDT inRecID, SInt32 inRecSize = 0)
//							adds an entry for the specified rec (pos & size)
//							returns final record position
//	SInt32		UpdateEntry(RecIDT inRecID, SInt32 inNewRecSize)
//							updates entry size info for the specified rec
//							returns final record position
//	SInt32		DeleteEntry(RecIDT inRecID)
//							marks the entry for the specified rec as deleted
//							returns position of deleted slot
//
// Other Useful Methods:
//	Boolean		SetBatchMode(Boolean inBatchMode)
//							prevents resorting of index during batch operations
//	SInt32		GetEntryCount() const
//							returns the number of valid entries in the index
//	Boolean		FindItemAt(SInt32 inAtIndex, IndexEntryT &outEntry)
//							returns the Nth index entry
//
// version 1.5.4
//
// created:   7/25/95, ERZ
// modified:  3/29/96, ERZ 	changed initialization process: OpenNew() & OpenExisting()
//								rewrote the entire object really, since all it does
//								now is define the constructor and the Open methods
// modified:  4/14/96, ERZ	added file permissions as optional params to Open methods
// modified:  4/15/96, ERZ	added Close() override
// modified:  7/27/96, ERZ	v1.5, added ConvertDataFileSpecToIndexFileSpec() to
//								simplify initialization process
// modified:  8/12/96, ERZ	changed OpenExisting() param order to match ::ResolveAlias()
// modified:  2/21/98, ERZ	v1.5.2, fixed write to Read Only files bug
// modified:  9/26/98, ERZ	v1.5.3, removed file permission from OpenNew(), which must
//								have Exclusive Read/Write privileges
// modified: 10/23/05, ERZ  v1.5.4, changed to use platform neutral CFileStream instead of LFileStream
//
// =============================================================================

#include "CMasterIndexFile.h"

#include <UException.h>


#if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
#include "FullPath.h"
#endif // mac

#ifndef MAX_FILENAME
#define MAX_FILENAME 2048
#endif


CMasterIndexFile::CMasterIndexFile() 
:  AMasterIndex(), CFileStream() {
  	itsStream = this;	// CW8 -- save this in itsStream so object refers to self
}


void	
CMasterIndexFile::OpenNew(const char* inPath, const char* inName, const char* inCreator) {
    char fname[MAX_FILENAME];
    std::strncpy(fname, inName, MAX_FILENAME);
    fname[MAX_FILENAME-1] = 0;
  #if DB_V15
    ConvertDataFileNameToIndexFileName(fname);
  #endif
    SetPath(inPath);
    SetName(fname);
    mReadOnly = false;
	DB_LOG("Create Index file \"" << inPath << "/" << fname << "\"");
	ExceptionCode err = CreateFile();		//create file
	ThrowIfOSErr_(err);
	SetFileAttribute(attrib_Type, "IdxM");
	SetFileAttribute(attrib_Creator, inCreator);
	err = OpenFile(CFileStream::privileges_ReadWrite);
	ThrowIfOSErr_(err);
	SetTypeAndVersion(kOSType_indx, 0x10000000);		// defaults for new file
	ThrowIf_( Open() );			// Open() should always return false for a new file
}


void	// ** NOT THREAD SAFE **
CMasterIndexFile::OpenExisting(const char* inPath, const char* inName, int inPrivileges) {
    char fname[MAX_FILENAME];
    std::strncpy(fname, inName, MAX_FILENAME);
    fname[MAX_FILENAME-1] = 0;
  #if DB_V15
    ConvertDataFileNameToIndexFileName(fname);
  #endif
    SetPath(inPath);
    SetName(fname);
	mReadOnly = (inPrivileges == CFileStream::privileges_ReadOnly);		// v1.5.5 bug fix, don't write to read-only files
	DB_LOG("Open Index file \"" << inPath << "/" << fname << "\"");
	ExceptionCode err = OpenFile(CFileStream::privileges_ReadWrite);
	ThrowIfOSErr_(err);
	Open();
	if ( mItemCount > mAllocatedSlots ) {
		Throw_( dbIndexCorrupt );
	}
	if ( mAllocatedSlots < 0 ) {
		Throw_( dbIndexCorrupt );
    }
}

void
CMasterIndexFile::Backup(const char* appendToFilename) {
	char filename[kDBFilenameBufferSize];
	char ext[kDBFilenameBufferSize];
    char backupFolderName[kDBFilenameBufferSize];
	ext[0] = 0;
    std::strcpy(filename, GetName());
	int len = std::strlen(filename);
	char* p = std::strrchr(filename, '.');
	if (p) {
	    std::strcpy(ext, p);    // copy the extension for later use
	    *p = 0;    // drop the extension from the filename
	}
    std::strcpy(backupFolderName, filename); // save off the base filename as the backup folder name
	std::strncat(filename, appendToFilename, kDBFilenameBufferSize);
	filename[kDBFilenameBufferSize-1] = 0;
	std::strncat(filename, ext, kDBFilenameBufferSize);
	filename[kDBFilenameBufferSize-1] = 0;
	
    std::strncat(backupFolderName, " Backups", kDBFilenameBufferSize);
	backupFolderName[kDBFilenameBufferSize-1] = 0;
	
	std::string backupPath = GetPath();
	backupPath += '/';
	backupPath += backupFolderName;
	
	CFileStream* backupFile = new CFileStream(backupPath.c_str(), filename);

	ExceptionCode err;
	
  #if !defined( POSIX_BUILD ) && (defined( PLATFORM_MACOS ) || defined( PLATFORM_MACOSX ))
    // try to create the backup directory on the Mac
    FSSpec backupFolderSpec;
    err = FSpMakeFSSpecFromPosixFullPath(backupFile->GetPath(), backupFolderSpec);
	if (err == fnfErr) {
        FSSpec tempSpec;
        ExceptionCode err = FSpMakeFSSpecFromPosixFullPath(GetPathName(), tempSpec);
	    long dirID = tempSpec.parID;
	    // folder doesn't exist, we need to create it, and get the new folder's dir ID
	    err = MacAPI::FSpDirCreate(&backupFolderSpec, smSystemScript, &dirID);
	    if ((err != noErr) && (err != dupFNErr)) {
	        DEBUG_OUT("WARNING: CMasterIndexFile Backup problem, creating backup folder [" << backupFolderName
	                <<"] failed with OS error [" << err << "]", DEBUG_ERROR | DEBUG_DATABASE);
	        dirID = tempSpec.parID;
	    }
	}
  #endif // !PLATFORM_MACOS
    
    backupFile->CreateFile();		//create file
    // copy creator and type
	char ostype[5];
	char creator[5];
	GetFileAttribute(attrib_Type, ostype, 5);
	GetFileAttribute(attrib_Creator, creator, 5);
	backupFile->SetFileAttribute(attrib_Type, ostype);
	backupFile->SetFileAttribute(attrib_Creator, creator);
	// now open the file to copy contents
	backupFile->OpenFile(CFileStream::privileges_ReadWrite);
    SInt32 bytesToCopy = GetLength();
    SInt32 pos = 0;
    
  #define BLOCK_SIZE 65536L    // copy in 64k blocks
    void* buffer = std::malloc(BLOCK_SIZE);
    
	// duplicate the DataFile into the backup file
	while (bytesToCopy > 0) {
	    SInt32 bytes = bytesToCopy;
	    if (bytes > BLOCK_SIZE) {
	        bytes = BLOCK_SIZE;
	    }   
	    SetMarker(pos, streamFrom_Start);
	    backupFile->SetMarker(pos, streamFrom_Start);
	    err = GetBytes(buffer, bytes);
	    if (err != noErr) {
	        DEBUG_OUT("CMasterIndexFile Backup failed, getting ["<<bytes<<"] bytes from pos ["<<pos
	                    <<"] with error [" << err << "]", DEBUG_ERROR | DEBUG_DATABASE);
	        break;
	    }
	    err = backupFile->PutBytes(buffer, bytes);
	    if (err != noErr) {
	        DEBUG_OUT("CMasterIndexFile Backup failed, putting ["<<bytes<<"] bytes from pos ["<<pos
	                    <<"] with error [" << err << "]", DEBUG_ERROR | DEBUG_DATABASE);
	        break;
	    }
	    pos += bytes;
	    bytesToCopy -= bytes;	        
	}
	std::free(buffer);
	backupFile->CloseFile();
}

void
CMasterIndexFile::Close() {
	AMasterIndex::Close();
	CloseFile();
}

void
CMasterIndexFile::ConvertDataFileNameToIndexFileName(char* ioFileName) {
    // TODO: rewrite this, it looks very badly done
	char dat[5] = ".dat";
	char idx[5] = ".idx";
  	int maxlen = MAX_FILENAME - 4;
  	int len = std::strlen(ioFileName);
	int fpos = 0;
  	int i = len - 4;
  	int j = 0;
  	char c;
  	// look for .dat in the filename
	while ( i <= len) {
		c = ioFileName[i];
		if ( (c >= 'A') && (c <= 'Z')) {	// convert to lowercase
			c += ('a' - 'A');
		}
		if (c == dat[j]) {
			if (j == 0) {
				fpos = i;
			} else if (j == 3) {
				break; // found
			}
			j++;
		} else {
			j = 0;	// reset our position
			fpos = 0;
		}
		i++;
	}
	if (!fpos) {	// if we didn't find it, add it to the end
		fpos = len + 1;
	}
  	if (fpos > maxlen) {
  		fpos = maxlen;
    }
	for (int i = 0; i < 5; i++) { // do 5 chars to get the trailing nul too
		ioFileName[i+fpos] = idx[i];
    }
}


void
CMasterIndexFile::WriteHeader(bool inIndexOpen) {
    AMasterIndex::WriteHeader(inIndexOpen);
    FlushFile();
}
