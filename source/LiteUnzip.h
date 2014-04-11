#ifndef _LITEUNZIP_H
#define _LITEUNZIP_H

/*
 * LiteUnzip.h 
 *
 * For decompressing the contents of zip archives using LITEUNZIP.DLL.
 *
 * This file is a repackaged form of extracts from the zlib code available
 * at www.gzip.org/zlib, by Jean-Loup Gailly and Mark Adler. The original
 * copyright notice may be found in unzip.cpp. The repackaging was done
 * by Lucian Wischik to simplify and extend its use in Windows/C++. Also
 * encryption and unicode filenames have been added. Code was further
 * revamped and turned into a DLL by Jeff Glatt.
 *
 * HotKeyIt 2003 (removed not required functions)
 * See http://www.codeproject.com/Articles/13370/LiteZip-and-LiteUnzip
 * The article, along with any associated source code and files, is licensed under The GNU Lesser General Public License (LGPLv3)
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

// An HUNZIP identifies a zip archive that has been opened
#define HUNZIP	void *

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#define IDS_OK        20
#define IDS_UNKNOWN   21
#define DIRSLASH_CHAR	'\\'
#ifndef CP_UTF8
#define CP_UTF8		65001
#endif
// =========================== Defines ======================
// Basic data types
typedef unsigned int	uInt;		// 16 bits or more
typedef unsigned char	UCH;
typedef unsigned long	ULG;
typedef unsigned long	lutime_t;	// define it ourselves since we don't include time.h

#define ZIP_MEMORY		0x01
#define ZIP_FILENAME	0x02
#define ZIP_HANDLE		0x04
#define ZIP_RAW			0x08
#define ZIP_ALREADYINIT	0x40000000
#define ZIP_UNICODE		0x80000000

// Allowed flush values; see deflate() for details
#define Z_NO_FLUSH		0
#define Z_SYNC_FLUSH	2
#define Z_FULL_FLUSH	3
#define Z_FINISH		4

// The deflate compression method (the only one supported in this version)
#define Z_DEFLATED		8

// Return codes for the DEFLATE low-level functions. Negative
// values are errors, positive values are used for special but normal events.
#define Z_OK			0
#define Z_STREAM_END	1
#define Z_NEED_DICT		2
#define Z_STREAM_ERROR	(-2)
#define Z_DATA_ERROR	(-3)
#define Z_MEM_ERROR		(-4)
#define Z_BUF_ERROR		(-5)

// preset dictionary flag in zlib header 
#define PRESET_DICT		0x20

// Maximum size of dynamic tree. The maximum found in a long but non-
// exhaustive search was 1004 huft structures (850 for length/literals
// and 154 for distances, the latter actually the result of an
// exhaustive search). The actual maximum is not known, but the
// value below is more than safe.
#define MANY		1440





// =========================== Structs ======================

typedef struct
{
	unsigned int tm_sec;	// seconds after the minute - [0,59]
	unsigned int tm_min;	// minutes after the hour - [0,59]
	unsigned int tm_hour;	// hours since midnight - [0,23]
	unsigned int tm_mday;	// day of the month - [1,31]
	unsigned int tm_mon;	// months since January - [0,11]
	unsigned int tm_year;	// years - [1980..2044]
} TM_UNZ;

// ZIPENTRYINFO holds information about one entry in the zip archive.
// This header appears in the ZIP archive.

#define UNZ_BUFSIZE (16384)
#define UNZ_MAXFILENAMEINZIP (256)
#define SIZECENTRALDIRITEM (0x2e)
#define SIZEZIPLOCALHEADER (0x1e)

#define ZIP_FIELDS_REFORMAT 0x000060F0
#define NUM_FIELDS_REFORMAT 15

#pragma pack(1)
typedef struct
{
	unsigned short	version;			// version made by					2 bytes
	unsigned short	version_needed;		// version needed to extract		2 bytes
	unsigned short	flag;				// general purpose bit flag			2 bytes
	unsigned short	compression_method;	// compression method				2 bytes
	unsigned long	dosDate;			// last mod file date in Dos fmt	4 bytes
	unsigned long	crc;				// crc-32							4 bytes
	unsigned long	compressed_size;	// compressed size					4 bytes
	unsigned long	uncompressed_size;	// uncompressed size				4 bytes
	unsigned short	size_filename;		// filename length					2 bytes
	unsigned short	size_file_extra;	// extra field length				2 bytes
	unsigned short	size_file_comment;	// file comment length				2 bytes
	unsigned short	disk_num_start;		// disk number start				2 bytes
	unsigned short	internal_fa;		// internal file attributes			2 bytes
	unsigned long	external_fa;		// external file attributes			4 bytes
	unsigned long	offset;				// Byte offset of local header		4 bytes
} ZIPENTRYINFO;
#pragma pack()


// Used for DEFLATE decompression
typedef struct {
	union {
		struct {
			UCH Exop;		// number of extra bits or operation
			UCH Bits;		// number of bits in this code or subcode
		} what;
		uInt pad;			// pad structure to a power of 2 (4 bytes for
	} word;					//  16-bit, 8 bytes for 32-bit int's)
	uInt base;				// literal, length base, distance base, or table offset
} INFLATE_HUFT;


// INFLATE_CODES_STATE->mode
// waiting for "i:"=input, "o:"=output, "x:"=nothing 
#define START	0	// x: set up for LEN 
#define LEN		1	// i: get length/literal/eob next 
#define LENEXT	2	// i: getting length extra (have base) 
#define DIST	3	// i: get distance next 
#define DISTEXT	4	// i: getting distance extra 
#define COPY	5	// o: copying bytes in window, waiting for space
#define LIT		6	// o: got literal, waiting for output space 
#define WASH	7	// o: got eob, possibly still output waiting 
#define END		8	// x: got eob and all data flushed 
#define BADCODE	9	// x: got error 

// inflate codes private state
typedef struct {
  uInt						len;

  // mode dependent information 
  union {
    struct {
      const INFLATE_HUFT	*tree;	// pointer into tree 
      uInt					need;	// bits needed 
    } code;							// if LEN or DIST, where in tree 
    uInt					lit;	// if LIT, literal 
    struct {
      uInt					get;	// bits to get for extra 
      uInt					dist;	// distance back to copy from 
    } copy;							// if EXT or COPY, where and how much 
  } sub;							// submode

  // mode independent information 
  const INFLATE_HUFT		*ltree;	// literal/length/eob tree
  const INFLATE_HUFT		*dtree;	// distance tree
  UCH						lbits;	// ltree bits decoded per branch 
  UCH						dbits;	// dtree bits decoder per branch 
  unsigned char				mode;	// current inflate_codes mode 
} INFLATE_CODES_STATE;

// INFLATE_BLOCKS_STATE->mode
#define IBM_TYPE	0	// get type bits (3, including end bit)
#define IBM_LENS	1	// get lengths for stored
#define IBM_STORED	2	// processing stored block
#define IBM_TABLE	3	// get table lengths
#define IBM_BTREE	4	// get bit lengths tree for a dynamic block
#define IBM_DTREE	5	// get length, distance trees for a dynamic block
#define IBM_CODES	6	// processing fixed or dynamic block
#define IBM_DRY		7	// output remaining window bytes
#define IBM_DONE	8	// finished last block, done 
#define IBM_BAD		9	// got a data error--stuck here 

// inflate blocks semi-private state 
typedef struct {
	// mode dependent information 
	union {
		uInt			 left;		// if STORED, bytes left to copy 
		struct {
			uInt		 table;		// table lengths (14 bits) 
			uInt		 index;		// index into blens (or border)
			uInt		 *blens;	// bit lengths of codes
			uInt		 bb;		// bit length tree depth 
			INFLATE_HUFT *tb;		// bit length decoding tree 
		} trees;					// if DTREE, decoding info for trees 

		struct {
			INFLATE_CODES_STATE *codes;
		} decode;				// if CODES, current state 
	} sub;						// submode

	// mode independent information 
	uInt			last;		// TRUE if this block is the last block 
	uInt			bitk;		// bits in bit buffer 
	ULG				bitb;		// bit buffer 
	INFLATE_HUFT	*hufts;		// single malloc for tree space 
	UCH				*window;	// sliding window 
	UCH				*end;		// one byte after sliding window 
	UCH				*read;		// window read pointer 
	UCH				*write;		// window write pointer 
	ULG				check;		// check on output 
	unsigned char	 mode;		// current inflate_block mode 
} INFLATE_BLOCKS_STATE;

// INTERNAL_STATE->mode
#define IM_METHOD	0	// waiting for method byte
#define IM_FLAG		1	// waiting for flag byte
#define IM_DICT4	2	// four dictionary check bytes to go
#define IM_DICT3	3	// three dictionary check bytes to go
#define IM_DICT2	4	// two dictionary check bytes to go
#define IM_DICT1	5	// one dictionary check byte to go
#define IM_DICT0	6	// waiting for inflateSetDictionary
#define IM_BLOCKS	7	// decompressing blocks
#define IM_CHECK4	8	// four check bytes to go
#define IM_CHECK3	9	// three check bytes to go
#define IM_CHECK2	10	// two check bytes to go
#define IM_CHECK1	11	// one check byte to go
#define IM_DONE		12	// finished check, done
#define IM_BAD		13	// got an error--stay here

// inflate private state
typedef struct {

	// mode dependent information
	union {
		uInt method;		// if IM_FLAGS, method byte
		struct {
			ULG	was;		// computed check value
			ULG	need;		// stream check value
		} check;			// if CHECK, check values to compare
		uInt marker;		// if IM_BAD, inflateSync's marker bytes count
	} sub;					// submode

	// mode independent information
	uInt					wbits;		// log2(window size)  (8..15, defaults to 15)
	INFLATE_BLOCKS_STATE	blocks;		// current inflate_blocks state
	unsigned char			mode;		// current inflate mode
//	unsigned char			nowrap;		// flag for no wrapper
} INTERNAL_STATE;


// readEntry() updates next_in and avail_in when avail_in has
// dropped to zero. It updates next_out and avail_out when avail_out
// has dropped to zero. All other fields are set by low level
// DEFLATE routines and must not be updated by the higher level.
//
// The fields total_in and total_out can be used for statistics or
// progress reports. After compression, total_in holds the total size of
// the uncompressed data and may be saved for use in the decompressor
// (particularly if the decompressor wants to decompress everything in
// a single step)
typedef struct {
	UCH	*		next_in;	// next input byte
	DWORD		avail_in;	// # of bytes available at next_in
	DWORD		total_in;	// total # of input bytes read so far
	UCH	*		next_out;	// next output byte should be put there
	DWORD		avail_out;	// remaining free space at next_out
	DWORD		total_out;	// total # of bytes output so far
#ifndef NDEBUG
	char *		msg;		// last error message, NULL if no error
#endif
	INTERNAL_STATE *state;
//	int			data_type;	// best guess about the data type: ascii or binary
//	ULG			adler;		// adler32 value of the uncompressed data
} Z_STREAM;


// ENTRYREADVARS holds tables/variables used when reading and decompressing an entry
typedef struct
{
	UCH			*InputBuffer;				// Buffer for reading in compressed data of the current entry
	Z_STREAM	stream;						// structure for inflate()
//	DWORD		PosInArchive;				// Current "file position" within the archive
	ULG			RunningCrc;					// crc32 of all data uncompressed
	DWORD		RemainingCompressed;		// Remaining number of bytes to be decompressed
	DWORD		RemainingUncompressed;		// Remaining number of bytes to be obtained after decomp
	unsigned long Keys[3];					// Decryption keys, initialized by initEntry()
	DWORD		RemainingEncrypt;			// The first call(s) to readEntry will read this many encryption-header bytes first
	char		CrcEncTest;					// If encrypted, we'll check the encryption buffer against this
} ENTRYREADVARS;

// For TUNZIP->flags
#define TZIP_ARCMEMORY			0x0000001	// Set if TZIP->archive is memory, instead of a file, handle.
#define TZIP_ARCCLOSEFH			0x0000002	// Set if we open the file handle in archiveOpen() and must close it later.
#define TZIP_GZIP				0x0000004	// Set if a GZIP archive.
#define TZIP_RAW				0x0000008	// Set if "raw" mode


// TUNZIP holds information about the ZIP archive itself.
typedef struct
{
	DWORD			Flags;
	HANDLE			ArchivePtr;					// Points to a handle, or a buffer if TZIP_ARCMEMORY
	DWORD			LastErr;					// Holds the last TUNZIP error code
	DWORD			InitialArchiveOffset;		// Initial offset within a file where the ZIP archive begins. This allows reading a ZIP archive contained within another file
	DWORD			ArchiveBufLen;				// Size of memory buffer
	DWORD			ArchiveBufPos;				// Current position within "ArchivePtr" if TZIP_ARCMEMORY
	DWORD			TotalEntries;				// Total number of entries in the current disk of this archive
	DWORD			CommentSize;				// Size of the global comment of the archive
	DWORD			ByteBeforeZipArchive;		// Byte before the archive, (>0 for sfx)
	DWORD			CurrentEntryNum;			// Number of the entry (in the archive) that is currently selected for
												// unzipping. -1 if none.
	DWORD			CurrEntryPosInCentralDir;	// Position of the current entry's header within the central dir
//	DWORD			CentralDirPos;				// Byte offset to the beginning of the central dir
	DWORD			CentralDirOffset;			// Offset of start of central directory with respect to the starting disk number
	unsigned char	*Password;					// Password, or 0 if none.
	unsigned char	*OutBuffer;					// Output buffer (where we decompress the current entry when unzipping it).
	ZIPENTRYINFO	CurrentEntryInfo;			// Info about the currently selected entry (gotten from the Central Dir)
	ENTRYREADVARS	EntryReadVars;				// Variables/buffers for decompressing the current entry
	unsigned char	Rootdir[MAX_PATH];			// Root dir for unzipping entries. Includes a trailing slash. Must be the last field!!!
} TUNZIP;



// Struct used to retrieve info about an entry in an archive
#ifdef UNICODE
typedef struct
{
	DWORD			Index;					// index of this entry within the zip archive
	DWORD			Attributes;				// attributes, as in GetFileAttributes.
	FILETIME		AccessTime, CreateTime, ModifyTime;	// access, create, modify filetimes
	unsigned long	CompressedSize;			// sizes of entry, compressed and uncompressed. These
	unsigned long	UncompressedSize;		// may be -1 if not yet known (e.g. being streamed in)
	WCHAR			Name[MAX_PATH];			// entry name
} ZIPENTRY;
#else
typedef struct
{
	DWORD			Index;
	DWORD			Attributes;
	FILETIME		AccessTime, CreateTime, ModifyTime;	// access, create, modify filetimes
	unsigned long	CompressedSize;
	unsigned long	UncompressedSize;
	char			Name[MAX_PATH];
} ZIPENTRY;
#endif

// Functions for opening a ZIP archive
DWORD openArchive(HANDLE *ptr, void *z, DWORD len, DWORD flags, const char *pwd);
DWORD unzipEntry(register TUNZIP *tunzip, void *dst, ZIPENTRY *ze, DWORD flags);
void closeArchive(register TUNZIP *tunzip);
#if !defined(ZR_OK)
// These are the return codes from Unzip functions
#define ZR_OK			0		// Success
// The following come from general system stuff (e.g. files not openable)
#define ZR_NOFILE		1		// Can't create/open the file
#define ZR_NOALLOC		2		// Failed to allocate memory
#define ZR_WRITE		3		// A general error writing to the file
#define ZR_NOTFOUND		4		// Can't find the specified file in the zip
#define ZR_MORE			5		// There's still more data to be unzipped
#define ZR_CORRUPT		6		// The zipfile is corrupt or not a zipfile
#define ZR_READ			7		// An error reading the file
#define ZR_NOTSUPPORTED	8		// The entry is in a format that can't be decompressed by this Unzip add-on
// The following come from mistakes on the part of the caller
#define ZR_ARGS			9		// Bad arguments passed
#define ZR_NOTMMAP		10		// Tried to ZipGetMemory, but that only works on mmap zipfiles, which yours wasn't
#define ZR_MEMSIZE		11		// The memory-buffer size is too small
#define ZR_FAILED		12		// Already failed when you called this function
#define ZR_ENDED		13		// The zip creation has already been closed
#define ZR_MISSIZE		14		// The source file size turned out mistaken
#define ZR_ZMODE		15		// Tried to mix creating/opening a zip 
// The following come from bugs within the zip library itself
#define ZR_SEEK			16		// trying to seek in an unseekable file
#define ZR_NOCHANGE		17		// changed its mind on storage, but not allowed
#define ZR_FLATE		18		// An error in the de/inflation code
#define ZR_PASSWORD		19		// Password is incorrect
#endif

#ifdef __cplusplus
}
#endif

#endif // _LITEUNZIP_H
