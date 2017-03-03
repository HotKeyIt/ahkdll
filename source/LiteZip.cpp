#include "stdafx.h"
#include "LiteZip.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include "globaldata.h"
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

#define UNZIP_MEMORY		0x01
#define UNZIP_FILENAME		0x02
#define UNZIP_HANDLE		0x04
#define UNZIP_RAW			0x08
#define UNZIP_ALREADYINIT	0x40000000
#define UNZIP_UNICODE		0x80000000

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

typedef struct
{
	ULONGLONG		compressed_size;	// compressed size					4 bytes
	ULONGLONG		uncompressed_size;	// uncompressed size				4 bytes
	ULONGLONG		offset;				// Byte offset of local header		4 bytes
} ZIPENTRYINFO64;
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
	ULONGLONG	avail_in;	// # of bytes available at next_in
	ULONGLONG	total_in;	// total # of input bytes read so far
	UCH	*		next_out;	// next output byte should be put there
	ULONGLONG	avail_out;	// remaining free space at next_out
	ULONGLONG	total_out;	// total # of bytes output so far
#ifdef _DEBUG
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
	ULONGLONG	RemainingCompressed;		// Remaining number of bytes to be decompressed
	ULONGLONG	RemainingUncompressed;		// Remaining number of bytes to be obtained after decomp
	unsigned long Keys[3];					// Decryption keys, initialized by initEntry()
	ULONGLONG	RemainingEncrypt;			// The first call(s) to readEntry will read this many encryption-header bytes first
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
	ULONGLONG		ArchiveBufLen;				// Size of memory buffer
	ULONGLONG		ArchiveBufPos;				// Current position within "ArchivePtr" if TZIP_ARCMEMORY
	ULONGLONG		TotalEntries;				// Total number of entries in the current disk of this archive
	DWORD			CommentSize;				// Size of the global comment of the archive
	DWORD			ByteBeforeZipArchive;		// Byte before the archive, (>0 for sfx)
	ULONGLONG		CurrentEntryNum;			// Number of the entry (in the archive) that is currently selected for
	// unzipping. -1 if none.
	ULONGLONG		CurrEntryPosInCentralDir;	// Position of the current entry's header within the central dir
	//	DWORD			CentralDirPos;				// Byte offset to the beginning of the central dir
	ULONGLONG		CentralDirOffset;			// Offset of start of central directory with respect to the starting disk number
	unsigned char	*Password;					// Password, or 0 if none.
	unsigned char	*OutBuffer;					// Output buffer (where we decompress the current entry when unzipping it).
	ZIPENTRYINFO	CurrentEntryInfo;			// Info about the currently selected entry (gotten from the Central Dir)
	ZIPENTRYINFO64	CurrentEntryInfo64;			// Info about the currently selected entry (gotten from the Central Dir)
	ENTRYREADVARS	EntryReadVars;				// Variables/buffers for decompressing the current entry
	unsigned char	Rootdir[MAX_PATH];			// Root dir for unzipping entries. Includes a trailing slash. Must be the last field!!!
} TUNZIP;

































// ======================== Function Declarations ======================

// Diagnostic functions
#ifdef _DEBUG
#define LuAssert(cond,msg)
#define LuTrace(x)
#define LuTracev(x)
#define LuTracevv(x)
#define LuTracec(c,x)
#define LuTracecv(c,x)
#endif
static int inflate_trees_bits(uInt *, uInt *, INFLATE_HUFT **, INFLATE_HUFT *, Z_STREAM *);
static int inflate_trees_dynamic(uInt, uInt, uInt *, uInt *, uInt *, INFLATE_HUFT **, INFLATE_HUFT **, INFLATE_HUFT *, Z_STREAM *);
static INFLATE_CODES_STATE *inflate_codes_new(uInt, uInt, const INFLATE_HUFT *, const INFLATE_HUFT *, Z_STREAM *);
static int inflate_codes(INFLATE_BLOCKS_STATE *, Z_STREAM *, int);
static int inflate_flush(INFLATE_BLOCKS_STATE *, Z_STREAM *, int);
static int inflate_fast(uInt, uInt, const INFLATE_HUFT *, const INFLATE_HUFT *, INFLATE_BLOCKS_STATE *, Z_STREAM *);
//static ULG adler32(ULG, const UCH *, DWORD);
static DWORD setCurrentEntry(TUNZIP *, ZIPENTRY *, DWORD);


// simplify the use of the INFLATE_HUFT type with some defines
// defines for inflate input/output
//   update pointers and return 
#define UPDBITS {s->bitb = b; s->bitk = k;}
#define UPDIN {z->avail_in = n; z->total_in += (ULG)(p-z->next_in); z->next_in = p;}
#define UPDOUT {s->write = q;}
#define UPDATE {UPDBITS UPDIN UPDOUT}
#define LEAVE {UPDATE return(inflate_flush(s,z,r));}
//   get bytes and bits 
#define LOADIN {p = (UCH*)z->next_in; n = (uInt)z->avail_in; b = s->bitb; k = s->bitk;}
#define NEEDBYTE {if (n) r = Z_OK; else LEAVE}
#define NEXTBYTE (n--, *p++)
#define NEEDBITS(j) {while(k<(j)){NEEDBYTE;b|=((ULG)NEXTBYTE)<<k;k+=8;}}
#define DUMPBITS(j) {b >>= (j); k -= (j);}
//   output bytes 
#define WAVAIL (uInt)(q < s->read ? s->read - q - 1: s->end - q)
#define LOADOUT {q=s->write;m=(uInt)WAVAIL;}
#define WRAP {if(q==s->end&&s->read!=s->window){q=s->window;m=(uInt)WAVAIL;}}
#define FLUSH {UPDOUT r=inflate_flush(s,z,r); LOADOUT}
#define NEEDOUT {if(m==0){WRAP if(m==0){FLUSH WRAP if(m==0) LEAVE}}r=Z_OK;}
#define OUTBYTE(a) {*q++=(UCH)(a);m--;}
//   load local pointers 
#define LOAD {LOADIN LOADOUT}







// NOTE: I specify this data section to be Shared (ie, each running rexx
// script shares these variables, rather than getting its own copies of these
// variables). This is because, since I have only globals that are read-only
// or whose value is the same for all processes, I don't need a separate copy
// of these for each process that uses this DLL. In Visual C++'s Linker
// settings, I add "/section:Shared,rws"

#pragma data_seg("ZIPSHARE")

static HINSTANCE	ThisInstance;

// Table of CRC-32's of all single-byte values (made by make_Crc_table)
static const ULG Crc_table[256] = {
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
	0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
	0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
	0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
	0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
	0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
	0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
	0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
	0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
	0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
	0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
	0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
	0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
	0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
	0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
	0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
	0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
	0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
	0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
	0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
	0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
	0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
	0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
	0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
	0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
	0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
	0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
	0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
	0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
	0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
	0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
	0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
	0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
	0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
	0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
	0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
	0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
	0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
	0x2d02ef8dL
};

// Masks for lower bits. And'ing with mask[n] masks the lower n bits
static const uInt inflate_mask[17] = { 0x0000,
0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

static const uInt Fixed_bl = 9;
static const uInt Fixed_bd = 5;
static const INFLATE_HUFT Fixed_tl[] = {
	{ { { 96, 7 } }, 256 }, { { { 0, 8 } }, 80 }, { { { 0, 8 } }, 16 }, { { { 84, 8 } }, 115 },
	{ { { 82, 7 } }, 31 }, { { { 0, 8 } }, 112 }, { { { 0, 8 } }, 48 }, { { { 0, 9 } }, 192 },
	{ { { 80, 7 } }, 10 }, { { { 0, 8 } }, 96 }, { { { 0, 8 } }, 32 }, { { { 0, 9 } }, 160 },
	{ { { 0, 8 } }, 0 }, { { { 0, 8 } }, 128 }, { { { 0, 8 } }, 64 }, { { { 0, 9 } }, 224 },
	{ { { 80, 7 } }, 6 }, { { { 0, 8 } }, 88 }, { { { 0, 8 } }, 24 }, { { { 0, 9 } }, 144 },
	{ { { 83, 7 } }, 59 }, { { { 0, 8 } }, 120 }, { { { 0, 8 } }, 56 }, { { { 0, 9 } }, 208 },
	{ { { 81, 7 } }, 17 }, { { { 0, 8 } }, 104 }, { { { 0, 8 } }, 40 }, { { { 0, 9 } }, 176 },
	{ { { 0, 8 } }, 8 }, { { { 0, 8 } }, 136 }, { { { 0, 8 } }, 72 }, { { { 0, 9 } }, 240 },
	{ { { 80, 7 } }, 4 }, { { { 0, 8 } }, 84 }, { { { 0, 8 } }, 20 }, { { { 85, 8 } }, 227 },
	{ { { 83, 7 } }, 43 }, { { { 0, 8 } }, 116 }, { { { 0, 8 } }, 52 }, { { { 0, 9 } }, 200 },
	{ { { 81, 7 } }, 13 }, { { { 0, 8 } }, 100 }, { { { 0, 8 } }, 36 }, { { { 0, 9 } }, 168 },
	{ { { 0, 8 } }, 4 }, { { { 0, 8 } }, 132 }, { { { 0, 8 } }, 68 }, { { { 0, 9 } }, 232 },
	{ { { 80, 7 } }, 8 }, { { { 0, 8 } }, 92 }, { { { 0, 8 } }, 28 }, { { { 0, 9 } }, 152 },
	{ { { 84, 7 } }, 83 }, { { { 0, 8 } }, 124 }, { { { 0, 8 } }, 60 }, { { { 0, 9 } }, 216 },
	{ { { 82, 7 } }, 23 }, { { { 0, 8 } }, 108 }, { { { 0, 8 } }, 44 }, { { { 0, 9 } }, 184 },
	{ { { 0, 8 } }, 12 }, { { { 0, 8 } }, 140 }, { { { 0, 8 } }, 76 }, { { { 0, 9 } }, 248 },
	{ { { 80, 7 } }, 3 }, { { { 0, 8 } }, 82 }, { { { 0, 8 } }, 18 }, { { { 85, 8 } }, 163 },
	{ { { 83, 7 } }, 35 }, { { { 0, 8 } }, 114 }, { { { 0, 8 } }, 50 }, { { { 0, 9 } }, 196 },
	{ { { 81, 7 } }, 11 }, { { { 0, 8 } }, 98 }, { { { 0, 8 } }, 34 }, { { { 0, 9 } }, 164 },
	{ { { 0, 8 } }, 2 }, { { { 0, 8 } }, 130 }, { { { 0, 8 } }, 66 }, { { { 0, 9 } }, 228 },
	{ { { 80, 7 } }, 7 }, { { { 0, 8 } }, 90 }, { { { 0, 8 } }, 26 }, { { { 0, 9 } }, 148 },
	{ { { 84, 7 } }, 67 }, { { { 0, 8 } }, 122 }, { { { 0, 8 } }, 58 }, { { { 0, 9 } }, 212 },
	{ { { 82, 7 } }, 19 }, { { { 0, 8 } }, 106 }, { { { 0, 8 } }, 42 }, { { { 0, 9 } }, 180 },
	{ { { 0, 8 } }, 10 }, { { { 0, 8 } }, 138 }, { { { 0, 8 } }, 74 }, { { { 0, 9 } }, 244 },
	{ { { 80, 7 } }, 5 }, { { { 0, 8 } }, 86 }, { { { 0, 8 } }, 22 }, { { { 192, 8 } }, 0 },
	{ { { 83, 7 } }, 51 }, { { { 0, 8 } }, 118 }, { { { 0, 8 } }, 54 }, { { { 0, 9 } }, 204 },
	{ { { 81, 7 } }, 15 }, { { { 0, 8 } }, 102 }, { { { 0, 8 } }, 38 }, { { { 0, 9 } }, 172 },
	{ { { 0, 8 } }, 6 }, { { { 0, 8 } }, 134 }, { { { 0, 8 } }, 70 }, { { { 0, 9 } }, 236 },
	{ { { 80, 7 } }, 9 }, { { { 0, 8 } }, 94 }, { { { 0, 8 } }, 30 }, { { { 0, 9 } }, 156 },
	{ { { 84, 7 } }, 99 }, { { { 0, 8 } }, 126 }, { { { 0, 8 } }, 62 }, { { { 0, 9 } }, 220 },
	{ { { 82, 7 } }, 27 }, { { { 0, 8 } }, 110 }, { { { 0, 8 } }, 46 }, { { { 0, 9 } }, 188 },
	{ { { 0, 8 } }, 14 }, { { { 0, 8 } }, 142 }, { { { 0, 8 } }, 78 }, { { { 0, 9 } }, 252 },
	{ { { 96, 7 } }, 256 }, { { { 0, 8 } }, 81 }, { { { 0, 8 } }, 17 }, { { { 85, 8 } }, 131 },
	{ { { 82, 7 } }, 31 }, { { { 0, 8 } }, 113 }, { { { 0, 8 } }, 49 }, { { { 0, 9 } }, 194 },
	{ { { 80, 7 } }, 10 }, { { { 0, 8 } }, 97 }, { { { 0, 8 } }, 33 }, { { { 0, 9 } }, 162 },
	{ { { 0, 8 } }, 1 }, { { { 0, 8 } }, 129 }, { { { 0, 8 } }, 65 }, { { { 0, 9 } }, 226 },
	{ { { 80, 7 } }, 6 }, { { { 0, 8 } }, 89 }, { { { 0, 8 } }, 25 }, { { { 0, 9 } }, 146 },
	{ { { 83, 7 } }, 59 }, { { { 0, 8 } }, 121 }, { { { 0, 8 } }, 57 }, { { { 0, 9 } }, 210 },
	{ { { 81, 7 } }, 17 }, { { { 0, 8 } }, 105 }, { { { 0, 8 } }, 41 }, { { { 0, 9 } }, 178 },
	{ { { 0, 8 } }, 9 }, { { { 0, 8 } }, 137 }, { { { 0, 8 } }, 73 }, { { { 0, 9 } }, 242 },
	{ { { 80, 7 } }, 4 }, { { { 0, 8 } }, 85 }, { { { 0, 8 } }, 21 }, { { { 80, 8 } }, 258 },
	{ { { 83, 7 } }, 43 }, { { { 0, 8 } }, 117 }, { { { 0, 8 } }, 53 }, { { { 0, 9 } }, 202 },
	{ { { 81, 7 } }, 13 }, { { { 0, 8 } }, 101 }, { { { 0, 8 } }, 37 }, { { { 0, 9 } }, 170 },
	{ { { 0, 8 } }, 5 }, { { { 0, 8 } }, 133 }, { { { 0, 8 } }, 69 }, { { { 0, 9 } }, 234 },
	{ { { 80, 7 } }, 8 }, { { { 0, 8 } }, 93 }, { { { 0, 8 } }, 29 }, { { { 0, 9 } }, 154 },
	{ { { 84, 7 } }, 83 }, { { { 0, 8 } }, 125 }, { { { 0, 8 } }, 61 }, { { { 0, 9 } }, 218 },
	{ { { 82, 7 } }, 23 }, { { { 0, 8 } }, 109 }, { { { 0, 8 } }, 45 }, { { { 0, 9 } }, 186 },
	{ { { 0, 8 } }, 13 }, { { { 0, 8 } }, 141 }, { { { 0, 8 } }, 77 }, { { { 0, 9 } }, 250 },
	{ { { 80, 7 } }, 3 }, { { { 0, 8 } }, 83 }, { { { 0, 8 } }, 19 }, { { { 85, 8 } }, 195 },
	{ { { 83, 7 } }, 35 }, { { { 0, 8 } }, 115 }, { { { 0, 8 } }, 51 }, { { { 0, 9 } }, 198 },
	{ { { 81, 7 } }, 11 }, { { { 0, 8 } }, 99 }, { { { 0, 8 } }, 35 }, { { { 0, 9 } }, 166 },
	{ { { 0, 8 } }, 3 }, { { { 0, 8 } }, 131 }, { { { 0, 8 } }, 67 }, { { { 0, 9 } }, 230 },
	{ { { 80, 7 } }, 7 }, { { { 0, 8 } }, 91 }, { { { 0, 8 } }, 27 }, { { { 0, 9 } }, 150 },
	{ { { 84, 7 } }, 67 }, { { { 0, 8 } }, 123 }, { { { 0, 8 } }, 59 }, { { { 0, 9 } }, 214 },
	{ { { 82, 7 } }, 19 }, { { { 0, 8 } }, 107 }, { { { 0, 8 } }, 43 }, { { { 0, 9 } }, 182 },
	{ { { 0, 8 } }, 11 }, { { { 0, 8 } }, 139 }, { { { 0, 8 } }, 75 }, { { { 0, 9 } }, 246 },
	{ { { 80, 7 } }, 5 }, { { { 0, 8 } }, 87 }, { { { 0, 8 } }, 23 }, { { { 192, 8 } }, 0 },
	{ { { 83, 7 } }, 51 }, { { { 0, 8 } }, 119 }, { { { 0, 8 } }, 55 }, { { { 0, 9 } }, 206 },
	{ { { 81, 7 } }, 15 }, { { { 0, 8 } }, 103 }, { { { 0, 8 } }, 39 }, { { { 0, 9 } }, 174 },
	{ { { 0, 8 } }, 7 }, { { { 0, 8 } }, 135 }, { { { 0, 8 } }, 71 }, { { { 0, 9 } }, 238 },
	{ { { 80, 7 } }, 9 }, { { { 0, 8 } }, 95 }, { { { 0, 8 } }, 31 }, { { { 0, 9 } }, 158 },
	{ { { 84, 7 } }, 99 }, { { { 0, 8 } }, 127 }, { { { 0, 8 } }, 63 }, { { { 0, 9 } }, 222 },
	{ { { 82, 7 } }, 27 }, { { { 0, 8 } }, 111 }, { { { 0, 8 } }, 47 }, { { { 0, 9 } }, 190 },
	{ { { 0, 8 } }, 15 }, { { { 0, 8 } }, 143 }, { { { 0, 8 } }, 79 }, { { { 0, 9 } }, 254 },
	{ { { 96, 7 } }, 256 }, { { { 0, 8 } }, 80 }, { { { 0, 8 } }, 16 }, { { { 84, 8 } }, 115 },
	{ { { 82, 7 } }, 31 }, { { { 0, 8 } }, 112 }, { { { 0, 8 } }, 48 }, { { { 0, 9 } }, 193 },
	{ { { 80, 7 } }, 10 }, { { { 0, 8 } }, 96 }, { { { 0, 8 } }, 32 }, { { { 0, 9 } }, 161 },
	{ { { 0, 8 } }, 0 }, { { { 0, 8 } }, 128 }, { { { 0, 8 } }, 64 }, { { { 0, 9 } }, 225 },
	{ { { 80, 7 } }, 6 }, { { { 0, 8 } }, 88 }, { { { 0, 8 } }, 24 }, { { { 0, 9 } }, 145 },
	{ { { 83, 7 } }, 59 }, { { { 0, 8 } }, 120 }, { { { 0, 8 } }, 56 }, { { { 0, 9 } }, 209 },
	{ { { 81, 7 } }, 17 }, { { { 0, 8 } }, 104 }, { { { 0, 8 } }, 40 }, { { { 0, 9 } }, 177 },
	{ { { 0, 8 } }, 8 }, { { { 0, 8 } }, 136 }, { { { 0, 8 } }, 72 }, { { { 0, 9 } }, 241 },
	{ { { 80, 7 } }, 4 }, { { { 0, 8 } }, 84 }, { { { 0, 8 } }, 20 }, { { { 85, 8 } }, 227 },
	{ { { 83, 7 } }, 43 }, { { { 0, 8 } }, 116 }, { { { 0, 8 } }, 52 }, { { { 0, 9 } }, 201 },
	{ { { 81, 7 } }, 13 }, { { { 0, 8 } }, 100 }, { { { 0, 8 } }, 36 }, { { { 0, 9 } }, 169 },
	{ { { 0, 8 } }, 4 }, { { { 0, 8 } }, 132 }, { { { 0, 8 } }, 68 }, { { { 0, 9 } }, 233 },
	{ { { 80, 7 } }, 8 }, { { { 0, 8 } }, 92 }, { { { 0, 8 } }, 28 }, { { { 0, 9 } }, 153 },
	{ { { 84, 7 } }, 83 }, { { { 0, 8 } }, 124 }, { { { 0, 8 } }, 60 }, { { { 0, 9 } }, 217 },
	{ { { 82, 7 } }, 23 }, { { { 0, 8 } }, 108 }, { { { 0, 8 } }, 44 }, { { { 0, 9 } }, 185 },
	{ { { 0, 8 } }, 12 }, { { { 0, 8 } }, 140 }, { { { 0, 8 } }, 76 }, { { { 0, 9 } }, 249 },
	{ { { 80, 7 } }, 3 }, { { { 0, 8 } }, 82 }, { { { 0, 8 } }, 18 }, { { { 85, 8 } }, 163 },
	{ { { 83, 7 } }, 35 }, { { { 0, 8 } }, 114 }, { { { 0, 8 } }, 50 }, { { { 0, 9 } }, 197 },
	{ { { 81, 7 } }, 11 }, { { { 0, 8 } }, 98 }, { { { 0, 8 } }, 34 }, { { { 0, 9 } }, 165 },
	{ { { 0, 8 } }, 2 }, { { { 0, 8 } }, 130 }, { { { 0, 8 } }, 66 }, { { { 0, 9 } }, 229 },
	{ { { 80, 7 } }, 7 }, { { { 0, 8 } }, 90 }, { { { 0, 8 } }, 26 }, { { { 0, 9 } }, 149 },
	{ { { 84, 7 } }, 67 }, { { { 0, 8 } }, 122 }, { { { 0, 8 } }, 58 }, { { { 0, 9 } }, 213 },
	{ { { 82, 7 } }, 19 }, { { { 0, 8 } }, 106 }, { { { 0, 8 } }, 42 }, { { { 0, 9 } }, 181 },
	{ { { 0, 8 } }, 10 }, { { { 0, 8 } }, 138 }, { { { 0, 8 } }, 74 }, { { { 0, 9 } }, 245 },
	{ { { 80, 7 } }, 5 }, { { { 0, 8 } }, 86 }, { { { 0, 8 } }, 22 }, { { { 192, 8 } }, 0 },
	{ { { 83, 7 } }, 51 }, { { { 0, 8 } }, 118 }, { { { 0, 8 } }, 54 }, { { { 0, 9 } }, 205 },
	{ { { 81, 7 } }, 15 }, { { { 0, 8 } }, 102 }, { { { 0, 8 } }, 38 }, { { { 0, 9 } }, 173 },
	{ { { 0, 8 } }, 6 }, { { { 0, 8 } }, 134 }, { { { 0, 8 } }, 70 }, { { { 0, 9 } }, 237 },
	{ { { 80, 7 } }, 9 }, { { { 0, 8 } }, 94 }, { { { 0, 8 } }, 30 }, { { { 0, 9 } }, 157 },
	{ { { 84, 7 } }, 99 }, { { { 0, 8 } }, 126 }, { { { 0, 8 } }, 62 }, { { { 0, 9 } }, 221 },
	{ { { 82, 7 } }, 27 }, { { { 0, 8 } }, 110 }, { { { 0, 8 } }, 46 }, { { { 0, 9 } }, 189 },
	{ { { 0, 8 } }, 14 }, { { { 0, 8 } }, 142 }, { { { 0, 8 } }, 78 }, { { { 0, 9 } }, 253 },
	{ { { 96, 7 } }, 256 }, { { { 0, 8 } }, 81 }, { { { 0, 8 } }, 17 }, { { { 85, 8 } }, 131 },
	{ { { 82, 7 } }, 31 }, { { { 0, 8 } }, 113 }, { { { 0, 8 } }, 49 }, { { { 0, 9 } }, 195 },
	{ { { 80, 7 } }, 10 }, { { { 0, 8 } }, 97 }, { { { 0, 8 } }, 33 }, { { { 0, 9 } }, 163 },
	{ { { 0, 8 } }, 1 }, { { { 0, 8 } }, 129 }, { { { 0, 8 } }, 65 }, { { { 0, 9 } }, 227 },
	{ { { 80, 7 } }, 6 }, { { { 0, 8 } }, 89 }, { { { 0, 8 } }, 25 }, { { { 0, 9 } }, 147 },
	{ { { 83, 7 } }, 59 }, { { { 0, 8 } }, 121 }, { { { 0, 8 } }, 57 }, { { { 0, 9 } }, 211 },
	{ { { 81, 7 } }, 17 }, { { { 0, 8 } }, 105 }, { { { 0, 8 } }, 41 }, { { { 0, 9 } }, 179 },
	{ { { 0, 8 } }, 9 }, { { { 0, 8 } }, 137 }, { { { 0, 8 } }, 73 }, { { { 0, 9 } }, 243 },
	{ { { 80, 7 } }, 4 }, { { { 0, 8 } }, 85 }, { { { 0, 8 } }, 21 }, { { { 80, 8 } }, 258 },
	{ { { 83, 7 } }, 43 }, { { { 0, 8 } }, 117 }, { { { 0, 8 } }, 53 }, { { { 0, 9 } }, 203 },
	{ { { 81, 7 } }, 13 }, { { { 0, 8 } }, 101 }, { { { 0, 8 } }, 37 }, { { { 0, 9 } }, 171 },
	{ { { 0, 8 } }, 5 }, { { { 0, 8 } }, 133 }, { { { 0, 8 } }, 69 }, { { { 0, 9 } }, 235 },
	{ { { 80, 7 } }, 8 }, { { { 0, 8 } }, 93 }, { { { 0, 8 } }, 29 }, { { { 0, 9 } }, 155 },
	{ { { 84, 7 } }, 83 }, { { { 0, 8 } }, 125 }, { { { 0, 8 } }, 61 }, { { { 0, 9 } }, 219 },
	{ { { 82, 7 } }, 23 }, { { { 0, 8 } }, 109 }, { { { 0, 8 } }, 45 }, { { { 0, 9 } }, 187 },
	{ { { 0, 8 } }, 13 }, { { { 0, 8 } }, 141 }, { { { 0, 8 } }, 77 }, { { { 0, 9 } }, 251 },
	{ { { 80, 7 } }, 3 }, { { { 0, 8 } }, 83 }, { { { 0, 8 } }, 19 }, { { { 85, 8 } }, 195 },
	{ { { 83, 7 } }, 35 }, { { { 0, 8 } }, 115 }, { { { 0, 8 } }, 51 }, { { { 0, 9 } }, 199 },
	{ { { 81, 7 } }, 11 }, { { { 0, 8 } }, 99 }, { { { 0, 8 } }, 35 }, { { { 0, 9 } }, 167 },
	{ { { 0, 8 } }, 3 }, { { { 0, 8 } }, 131 }, { { { 0, 8 } }, 67 }, { { { 0, 9 } }, 231 },
	{ { { 80, 7 } }, 7 }, { { { 0, 8 } }, 91 }, { { { 0, 8 } }, 27 }, { { { 0, 9 } }, 151 },
	{ { { 84, 7 } }, 67 }, { { { 0, 8 } }, 123 }, { { { 0, 8 } }, 59 }, { { { 0, 9 } }, 215 },
	{ { { 82, 7 } }, 19 }, { { { 0, 8 } }, 107 }, { { { 0, 8 } }, 43 }, { { { 0, 9 } }, 183 },
	{ { { 0, 8 } }, 11 }, { { { 0, 8 } }, 139 }, { { { 0, 8 } }, 75 }, { { { 0, 9 } }, 247 },
	{ { { 80, 7 } }, 5 }, { { { 0, 8 } }, 87 }, { { { 0, 8 } }, 23 }, { { { 192, 8 } }, 0 },
	{ { { 83, 7 } }, 51 }, { { { 0, 8 } }, 119 }, { { { 0, 8 } }, 55 }, { { { 0, 9 } }, 207 },
	{ { { 81, 7 } }, 15 }, { { { 0, 8 } }, 103 }, { { { 0, 8 } }, 39 }, { { { 0, 9 } }, 175 },
	{ { { 0, 8 } }, 7 }, { { { 0, 8 } }, 135 }, { { { 0, 8 } }, 71 }, { { { 0, 9 } }, 239 },
	{ { { 80, 7 } }, 9 }, { { { 0, 8 } }, 95 }, { { { 0, 8 } }, 31 }, { { { 0, 9 } }, 159 },
	{ { { 84, 7 } }, 99 }, { { { 0, 8 } }, 127 }, { { { 0, 8 } }, 63 }, { { { 0, 9 } }, 223 },
	{ { { 82, 7 } }, 27 }, { { { 0, 8 } }, 111 }, { { { 0, 8 } }, 47 }, { { { 0, 9 } }, 191 },
	{ { { 0, 8 } }, 15 }, { { { 0, 8 } }, 143 }, { { { 0, 8 } }, 79 }, { { { 0, 9 } }, 255 }
};

static const INFLATE_HUFT Fixed_td[] = {
	{ { { 80, 5 } }, 1 }, { { { 87, 5 } }, 257 }, { { { 83, 5 } }, 17 }, { { { 91, 5 } }, 4097 },
	{ { { 81, 5 } }, 5 }, { { { 89, 5 } }, 1025 }, { { { 85, 5 } }, 65 }, { { { 93, 5 } }, 16385 },
	{ { { 80, 5 } }, 3 }, { { { 88, 5 } }, 513 }, { { { 84, 5 } }, 33 }, { { { 92, 5 } }, 8193 },
	{ { { 82, 5 } }, 9 }, { { { 90, 5 } }, 2049 }, { { { 86, 5 } }, 129 }, { { { 192, 5 } }, 24577 },
	{ { { 80, 5 } }, 2 }, { { { 87, 5 } }, 385 }, { { { 83, 5 } }, 25 }, { { { 91, 5 } }, 6145 },
	{ { { 81, 5 } }, 7 }, { { { 89, 5 } }, 1537 }, { { { 85, 5 } }, 97 }, { { { 93, 5 } }, 24577 },
	{ { { 80, 5 } }, 4 }, { { { 88, 5 } }, 769 }, { { { 84, 5 } }, 49 }, { { { 92, 5 } }, 12289 },
	{ { { 82, 5 } }, 13 }, { { { 90, 5 } }, 3073 }, { { { 86, 5 } }, 193 }, { { { 192, 5 } }, 24577 }
};

// Order of the bit length code lengths
static const uInt Border[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

// Tables for deflate from PKZIP's appnote.txt. 
static const uInt CpLens[31] = { // Copy lengths for literal codes 257..285
	3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
	35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0 };
// see note #13 above about 258
static const uInt CpLExt[31] = { // Extra bits for literal codes 257..285
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
	3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 112, 112 }; // 112==invalid
static const uInt CpDist[30] = { // Copy offsets for distance codes 0..29
	1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
	257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
	8193, 12289, 16385, 24577 };
static const uInt CpDExt[30] = { // Extra bits for distance codes 
	0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
	7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
	12, 12, 13, 13 };

// Error messages
static const char UnknownErr[] = "Unknown zip result code";
static const char ErrorMsgs[] = "Success\0\
								Can't create/open file\0\
								Failed to allocate memory\0\
								Error writing to file\0\
								Entry not found in the zip archive\0\
								Still more data to unzip\0\
								Zip archive is corrupt or not a zip archive\0\
								Error reading file\0\
								The entry is in a format that can't be decompressed by this Unzip add-on\0\
								Faulty arguments\0\
								Can get memory only of a memory-mapped zip\0\
								Not enough space allocated for memory zip\0\
								There was a previous error\0\
								Additions to the zip have already been ended\0\
								The anticipated size turned out wrong\0\
								Mixing creation and opening of zip\0\
								Trying to seek the unseekable\0\
								Tried to change mind, but not allowed\0\
								An internal error during flation\0\
								Password is incorrect\0\
								Aborted\0";

#pragma data_seg()












/***************** timet2filetime() ******************
* Converts a Unix time_t to a Windows FILETIME.
*/

static void timet2filetime(FILETIME *ft, const lutime_t t)
{
	LONGLONG i;

	i = Int32x32To64(t, 10000000) + 116444736000000000;
	ft->dwLowDateTime = (DWORD)i;
	ft->dwHighDateTime = (DWORD)(i >> 32);
}

/*************** dosdatetime2filetime() ***************
* Converts a DOS timestamp (date in high word, time in
* low word) to a Windows FILETIME.
*/

static void dosdatetime2filetime(FILETIME *ft, DWORD dosdate, DWORD dostime)
{
	SYSTEMTIME		st;

	// date: bits 0-4 are day of month 1-31. Bits 5-8 are month 1..12. Bits 9-15 are year-1980
	// time: bits 0-4 are seconds/2, bits 5-10 are minute 0..59. Bits 11-15 are hour 0..23
	st.wYear = (WORD)(((dosdate >> 9) & 0x7f) + 1980);
	st.wMonth = (WORD)((dosdate >> 5) & 0xf);
	st.wDay = (WORD)(dosdate & 0x1f);
	st.wHour = (WORD)((dostime >> 11) & 0x1f);
	st.wMinute = (WORD)((dostime >> 5) & 0x3f);
	st.wSecond = (WORD)((dostime & 0x1f) * 2);
	st.wMilliseconds = 0;

	SystemTimeToFileTime(&st, ft);
}









// ======================= Low level DEFLATE code =====================


// copy as much as possible from the sliding window to the output area
static int inflate_flush(INFLATE_BLOCKS_STATE *s, Z_STREAM * z, int r)
{
	uInt	n;
	UCH		*p;
	UCH		*q;

	// local copies of source and destination pointers 
	p = z->next_out;
	q = s->read;

	// compute number of bytes to copy as far as end of window 
	n = (uInt)((q <= s->write ? s->write : s->end) - q);
	if (n > (uInt)z->avail_out) n = (uInt)z->avail_out;
	if (n && r == Z_BUF_ERROR) r = Z_OK;

	// update counters
	z->avail_out -= n;
	z->total_out += n;

	// update check information
	//	if (!z->state->nowrap) z->adler = s->check = adler32(s->check, q, n);

	// copy as far as end of window 
	if (n)          // check for n!=0 to avoid waking up CodeGuard
	{
		CopyMemory(p, q, n);
		p += n;
		q += n;
	}

	// see if more to copy at beginning of window
	if (q == s->end)
	{
		// wrap pointers 
		q = s->window;
		if (s->write == s->end) s->write = s->window;

		// compute bytes to copy 
		n = (uInt)(s->write - q);
		if (n > (uInt)z->avail_out) n = (uInt)z->avail_out;
		if (n && r == Z_BUF_ERROR) r = Z_OK;

		// update counters 
		z->avail_out -= n;
		z->total_out += n;

		// update check information 
		//		if (!z->state->nowrap) z->adler = s->check = adler32(s->check, q, n);

		// copy
		if (n)
		{
			CopyMemory(p, q, n);
			p += n;
			q += n;
		}
	}

	// update pointers
	z->next_out = p;
	s->read = q;

	// done
	return r;
}




static INFLATE_CODES_STATE *inflate_codes_new(uInt bl, uInt bd, const INFLATE_HUFT *tl, const INFLATE_HUFT *td, Z_STREAM * z)
{
	INFLATE_CODES_STATE *c;

	if ((c = (INFLATE_CODES_STATE *)GlobalAlloc(GMEM_FIXED, sizeof(INFLATE_CODES_STATE))))
	{
		ZeroMemory(c, sizeof(INFLATE_CODES_STATE));
		c->mode = START;
		c->lbits = (UCH)bl;
		c->dbits = (UCH)bd;
		c->ltree = tl;
		c->dtree = td;
#ifdef _DEBUG
		LuTracev((stderr, "inflate:       codes new\n"));
#endif
	}

	return c;
}


static int inflate_codes(INFLATE_BLOCKS_STATE *s, Z_STREAM * z, int r)
{
	uInt				j;		// temporary storage
	const INFLATE_HUFT *t;		// temporary pointer
	uInt				e;		// extra bits or operation
	ULG					b;		// bit buffer
	uInt				k;		// bits in bit buffer
	UCH					*p;		// input data pointer
	uInt				n;		// bytes available there
	UCH					*q;		// output window write pointer
	uInt				m;		// bytes to end of window or read pointer
	UCH					*f;		// pointer to copy strings from
	INFLATE_CODES_STATE *c;		// codes state

	c = s->sub.decode.codes;

	// copy input/output information to locals (UPDATE macro restores)
	LOAD

		// process input and output based on current state
		for (;;)
		{
			switch (c->mode)
			{             // waiting for "i:"=input, "o:"=output, "x:"=nothing
			case START:         // x: set up for LEN
#ifndef SLOW
				if (m >= 258 && n >= 10)
				{
					UPDATE
						r = inflate_fast(c->lbits, c->dbits, c->ltree, c->dtree, s, z);
					LOAD
						if (r != Z_OK)
						{
							c->mode = r == Z_STREAM_END ? WASH : BADCODE;
							break;
						}
				}
#endif // !SLOW

				c->sub.code.need = c->lbits;
				c->sub.code.tree = c->ltree;
				c->mode = LEN;

			case LEN:           // i: get length/literal/eob next

				j = c->sub.code.need;
				NEEDBITS(j)
					t = c->sub.code.tree + ((uInt)b & inflate_mask[j]);
				DUMPBITS(t->word.what.Bits)
					e = (uInt)(t->word.what.Exop);
				if (e == 0)               // literal 
				{
					c->sub.lit = t->base;
#ifdef _DEBUG
					LuTracevv((stderr, t->base >= 0x20 && t->base < 0x7f ? "inflate:         literal '%c'\n" : "inflate:         literal 0x%02x\n", t->base));
#endif
					c->mode = LIT;
					break;
				}

				if (e & 16)               // length 
				{
					c->sub.copy.get = e & 15;
					c->len = t->base;
					c->mode = LENEXT;
					break;
				}

				if ((e & 64) == 0)        // next table 
				{
					c->sub.code.need = e;
					c->sub.code.tree = t + t->base;
					break;
				}

				if (e & 32)               // end of block 
				{
#ifdef _DEBUG
					LuTracevv((stderr, "inflate:         end of block\n"));
#endif
					c->mode = WASH;
					break;
				}

				c->mode = BADCODE;        // invalid code 
#ifdef _DEBUG
				z->msg = (char*)"invalid literal/length code";
#endif
				r = Z_DATA_ERROR;
				LEAVE

			case LENEXT:        // i: getting length extra (have base) 

				j = c->sub.copy.get;
				NEEDBITS(j)
					c->len += (uInt)b & inflate_mask[j];
				DUMPBITS(j)
					c->sub.code.need = c->dbits;
				c->sub.code.tree = c->dtree;
#ifdef _DEBUG
				LuTracevv((stderr, "inflate:         length %u\n", c->len));
#endif
				c->mode = DIST;

			case DIST:          // i: get distance next 

				j = c->sub.code.need;
				NEEDBITS(j)
					t = c->sub.code.tree + ((uInt)b & inflate_mask[j]);
				DUMPBITS(t->word.what.Bits)
					e = (uInt)(t->word.what.Exop);
				if (e & 16)               // distance 
				{
					c->sub.copy.get = e & 15;
					c->sub.copy.dist = t->base;
					c->mode = DISTEXT;
					break;
				}

				if ((e & 64) == 0)        // next table 
				{
					c->sub.code.need = e;
					c->sub.code.tree = t + t->base;
					break;
				}

				c->mode = BADCODE;        // invalid code 
#ifdef _DEBUG
				z->msg = (char*)"invalid distance code";
#endif
				r = Z_DATA_ERROR;
				LEAVE

			case DISTEXT:       // i: getting distance extra 

				j = c->sub.copy.get;
				NEEDBITS(j)
					c->sub.copy.dist += (uInt)b & inflate_mask[j];
				DUMPBITS(j)
#ifdef _DEBUG
					LuTracevv((stderr, "inflate:         distance %u\n", c->sub.copy.dist));
#endif
				c->mode = COPY;

			case COPY:          // o: copying bytes in window, waiting for space 

				f = q - c->sub.copy.dist;
				while (f < s->window)             // modulo window size-"while" instead
					f += s->end - s->window;        // of "if" handles invalid distances 

				while (c->len)
				{
					NEEDOUT
						OUTBYTE(*f++)
						if (f == s->end) f = s->window;
					--c->len;
				}

				c->mode = START;
				break;

			case LIT:           // o: got literal, waiting for output space 

				NEEDOUT
					OUTBYTE(c->sub.lit)
					c->mode = START;
				break;

			case WASH:          // o: got eob, possibly more output 

				if (k > 7)        // return unused byte, if any 
				{
					k -= 8;
					++n;
					--p;            // can always return one 
				}

				FLUSH
					if (s->read != s->write)
						LEAVE
						c->mode = END;

			case END:

				r = Z_STREAM_END;
				LEAVE

			case BADCODE:       // x: got error

				r = Z_DATA_ERROR;
				LEAVE

			default:

				r = Z_STREAM_ERROR;
				LEAVE
			}
		}
}







// infblock.c -- interpret and process block types to last block
// Copyright (C) 1995-1998 Mark Adler
// For conditions of distribution and use, see copyright notice in zlib.h


// Notes beyond the 1.93a appnote.txt:
//
// 1. Distance pointers never point before the beginning of the output stream.
// 2. Distance pointers can point back across blocks, up to 32k away.
// 3. There is an implied maximum of 7 bits for the bit length table and
//    15 bits for the actual data.
// 4. If only one code exists, then it is encoded using one bit.  (Zero
//    would be more efficient, but perhaps a little confusing.)  If two
//    codes exist, they are coded using one bit each (0 and 1).
// 5. There is no way of sending zero distance codes--a dummy must be
//    sent if there are none.  (History: a pre 2.0 version of PKZIP would
//    store blocks with no distance codes, but this was discovered to be
//    too harsh a criterion.)  Valid only for 1.93a.  2.04c does allow
//    zero distance codes, which is sent as one code of zero bits in
//    length.
// 6. There are up to 286 literal/length codes.  Code 256 represents the
//    end-of-block.  Note however that the static length tree defines
//    288 codes just to fill out the Huffman codes.  Codes 286 and 287
//    cannot be used though, since there is no length base or extra bits
//    defined for them.  Similarily, there are up to 30 distance codes.
//    However, static trees define 32 codes (all 5 bits) to fill out the
//    Huffman codes, but the last two had better not show up in the data.
// 7. Unzip can check dynamic Huffman blocks for complete code sets.
//    The exception is that a single code would not be complete (see #4).
// 8. The five bits following the block type is really the number of
//    literal codes sent minus 257.
// 9. Length codes 8,16,16 are interpreted as 13 length codes of 8 bits
//    (1+6+6).  Therefore, to output three times the length, you output
//    three codes (1+1+1), whereas to output four times the same length,
//    you only need two codes (1+3).  Hmm.
//10. In the tree reconstruction algorithm, Code = Code + Increment
//    only if BitLength(i) is not zero.  (Pretty obvious.)
//11. Correction: 4 Bits: # of Bit Length codes - 4     (4 - 19)
//12. Note: length code 284 can represent 227-258, but length code 285
//    really is 258.  The last length deserves its own, short code
//    since it gets used a lot in very redundant files.  The length
//    258 is special since 258 - 3 (the min match length) is 255.
//13. The literal/length and distance code bit lengths are read as a
//    single stream of lengths. It is possible (and advantageous) for
//    a repeat code (16, 17, or 18) to go across the boundary between
//    the two sets of lengths.


static void inflate_blocks_reset(Z_STREAM *z)
{
	register INFLATE_BLOCKS_STATE *s;

	s = &z->state->blocks;

	z->state->sub.check.was = s->check;

	if (s->mode == IBM_BTREE || s->mode == IBM_DTREE) GlobalFree(s->sub.trees.blens);
	if (s->mode == IBM_CODES) GlobalFree(s->sub.decode.codes);

	s->mode = IBM_TYPE;
	s->bitk = s->bitb = 0;
	s->read = s->write = s->window;
	//	if (!z->state->nowrap) z->adler = s->check = adler32(0, 0, 0);
#ifdef _DEBUG
	LuTracev((stderr, "inflate:   blocks reset\n"));
#endif
}





static int inflate_blocks(Z_STREAM * z, int r)
{
	uInt		t;				// temporary storage
	ULG			b;				// bit buffer
	uInt		k;				// bits in bit buffer
	UCH			*p;				// input data pointer
	uInt		n;				// bytes available there
	UCH			*q;				// output window write pointer
	uInt		m;				// bytes to end of window or read pointer 
	register INFLATE_BLOCKS_STATE *s;

	s = &z->state->blocks;

	// copy input/output information to locals (UPDATE macro restores) 
	LOAD

		// process input based on current state 
		for (;;)
		{
			switch (s->mode)
			{
			case IBM_TYPE:
			{
				NEEDBITS(3)
					t = (uInt)b & 7;
				s->last = t & 1;
				switch (t >> 1)
				{
					// Stored
				case 0:
				{
#ifdef _DEBUG
					LuTracev((stderr, "inflate:     stored block%s\n", s->last ? " (last)" : ""));
#endif
					DUMPBITS(3)
						t = k & 7;                    // go to byte boundary 
					DUMPBITS(t)
						s->mode = IBM_LENS;               // get length of stored block
					break;
				}

				// Fixes
				case 1:
				{
					uInt bl, bd;
					const INFLATE_HUFT *tl, *td;

#ifdef _DEBUG
					LuTracev((stderr, "inflate:     fixed codes block%s\n", s->last ? " (last)" : ""));
#endif
					bl = Fixed_bl;
					bd = Fixed_bd;
					tl = Fixed_tl;
					td = Fixed_td;
					s->sub.decode.codes = inflate_codes_new(bl, bd, tl, td, z);
					if (s->sub.decode.codes == 0)
					{
						r = Z_MEM_ERROR;
						LEAVE
					}

					DUMPBITS(3)
						s->mode = IBM_CODES;
					break;
				}

				// Dynamic
				case 2:
				{
#ifdef _DEBUG
					LuTracev((stderr, "inflate:     dynamic codes block%s\n", s->last ? " (last)" : ""));
#endif
					DUMPBITS(3)
						s->mode = IBM_TABLE;
					break;
				}

				// Illegal
				case 3:
				{
					DUMPBITS(3)
						s->mode = IBM_BAD;
#ifdef _DEBUG
					z->msg = (char*)"invalid block type";
#endif
					r = Z_DATA_ERROR;
					LEAVE
				}
				}

				break;
			}

			case IBM_LENS:
			{
				NEEDBITS(32)
					if ((((~b) >> 16) & 0xffff) != (b & 0xffff))
					{
						s->mode = IBM_BAD;
#ifdef _DEBUG
						z->msg = (char*)"invalid stored block lengths";
#endif
						r = Z_DATA_ERROR;
						LEAVE
					}

				s->sub.left = (uInt)b & 0xffff;
				b = k = 0;                      // dump bits 
#ifdef _DEBUG
				LuTracev((stderr, "inflate:       stored length %u\n", s->sub.left));
#endif
				s->mode = s->sub.left ? IBM_STORED : (s->last ? IBM_DRY : IBM_TYPE);
				break;
			}

			case IBM_STORED:
			{
				if (n == 0)
					LEAVE
					NEEDOUT
					t = s->sub.left;
				if (t > n) t = n;
				if (t > m) t = m;
				CopyMemory(q, p, t);
				p += t;  n -= t;
				q += t;  m -= t;
				if ((s->sub.left -= t) != 0)
					break;
#ifdef _DEBUG
				LuTracev((stderr, "inflate:       stored end, %lu total out\n", z->total_out + (q >= s->read ? q - s->read : (s->end - s->read) + (q - s->window))));
#endif
				s->mode = s->last ? IBM_DRY : IBM_TYPE;
				break;
			}

			case IBM_TABLE:
			{
				NEEDBITS(14)
					s->sub.trees.table = t = (uInt)b & 0x3fff;
				// remove this section to workaround bug in pkzip
				if ((t & 0x1f) > 29 || ((t >> 5) & 0x1f) > 29)
				{
					s->mode = IBM_BAD;
#ifdef _DEBUG
					z->msg = (char*)"too many length or distance symbols";
#endif
					r = Z_DATA_ERROR;
					LEAVE
				}

				// end remove
				t = 258 + (t & 0x1f) + ((t >> 5) & 0x1f);
				if (!(s->sub.trees.blens = (uInt *)GlobalAlloc(GMEM_FIXED, t * sizeof(uInt))))
				{
					r = Z_MEM_ERROR;
					LEAVE
				}

				DUMPBITS(14)
					s->sub.trees.index = 0;
#ifdef _DEBUG
				LuTracev((stderr, "inflate:       table sizes ok\n"));
#endif
				s->mode = IBM_BTREE;
			}

			case IBM_BTREE:
			{
				while (s->sub.trees.index < 4 + (s->sub.trees.table >> 10))
				{
					NEEDBITS(3)
						s->sub.trees.blens[Border[s->sub.trees.index++]] = (uInt)b & 7;
					DUMPBITS(3)
				}

				while (s->sub.trees.index < 19)
					s->sub.trees.blens[Border[s->sub.trees.index++]] = 0;
				s->sub.trees.bb = 7;
				t = inflate_trees_bits(s->sub.trees.blens, &s->sub.trees.bb, &s->sub.trees.tb, s->hufts, z);
				if (t != Z_OK)
				{
					r = t;
					if (r == Z_DATA_ERROR)
					{
						GlobalFree(s->sub.trees.blens);
						s->mode = IBM_BAD;
					}

					LEAVE
				}

				s->sub.trees.index = 0;
#ifdef _DEBUG
				LuTracev((stderr, "inflate:       bits tree ok\n"));
#endif
				s->mode = IBM_DTREE;
			}

			case IBM_DTREE:
			{
				while (t = s->sub.trees.table, s->sub.trees.index < 258 + (t & 0x1f) + ((t >> 5) & 0x1f))
				{
					INFLATE_HUFT *h;
					uInt i, j, c;

					t = s->sub.trees.bb;
					NEEDBITS(t)
						h = s->sub.trees.tb + ((uInt)b & inflate_mask[t]);
					t = h->word.what.Bits;
					c = h->base;
					if (c < 16)
					{
						DUMPBITS(t)
							s->sub.trees.blens[s->sub.trees.index++] = c;
					}
					else // c == 16..18 
					{
						i = c == 18 ? 7 : c - 14;
						j = c == 18 ? 11 : 3;
						NEEDBITS(t + i)
							DUMPBITS(t)
							j += (uInt)b & inflate_mask[i];
						DUMPBITS(i)
							i = s->sub.trees.index;
						t = s->sub.trees.table;
						if (i + j > 258 + (t & 0x1f) + ((t >> 5) & 0x1f) || (c == 16 && i < 1))
						{
							GlobalFree(s->sub.trees.blens);
							s->mode = IBM_BAD;
#ifdef _DEBUG
							z->msg = (char*)"invalid bit length repeat";
#endif
							r = Z_DATA_ERROR;
							LEAVE
						}
						c = c == 16 ? s->sub.trees.blens[i - 1] : 0;
						do
						{
							s->sub.trees.blens[i++] = c;
						} while (--j);
						s->sub.trees.index = i;
					}
				}

				s->sub.trees.tb = 0;

				{
					uInt bl, bd;
					INFLATE_HUFT *tl, *td;
					INFLATE_CODES_STATE *c;

					bl = 9;         // must be <= 9 for lookahead assumptions 
					bd = 6;         // must be <= 9 for lookahead assumptions
					t = s->sub.trees.table;
					t = inflate_trees_dynamic(257 + (t & 0x1f), 1 + ((t >> 5) & 0x1f), s->sub.trees.blens, &bl, &bd, &tl, &td, s->hufts, z);
					if (t != Z_OK)
					{
						if (t == (uInt)Z_DATA_ERROR)
						{
							GlobalFree(s->sub.trees.blens);
							s->mode = IBM_BAD;
						}
						r = t;
						LEAVE
					}
#ifdef _DEBUG
					LuTracev((stderr, "inflate:       trees ok\n"));
#endif
					if ((c = inflate_codes_new(bl, bd, tl, td, z)) == 0)
					{
						r = Z_MEM_ERROR;
						LEAVE
					}
					s->sub.decode.codes = c;
				}

				GlobalFree(s->sub.trees.blens);
				s->mode = IBM_CODES;
			}

			case IBM_CODES:
			{
				UPDATE
					if ((r = inflate_codes(s, z, r)) != Z_STREAM_END)
						return inflate_flush(s, z, r);
				r = Z_OK;
				GlobalFree(s->sub.decode.codes);
				LOAD
#ifdef _DEBUG
					LuTracev((stderr, "inflate:       codes end, %lu total out\n", z->total_out + (q >= s->read ? q - s->read : (s->end - s->read) + (q - s->window))));
#endif
				if (!s->last)
				{
					s->mode = IBM_TYPE;
					break;
				}
				s->mode = IBM_DRY;
			}

			case IBM_DRY:
			{
				FLUSH
					if (s->read != s->write)
						LEAVE
						s->mode = IBM_DONE;
			}

			case IBM_DONE:
			{
				r = Z_STREAM_END;
				LEAVE
			}

			case IBM_BAD:
			{
				r = Z_DATA_ERROR;
				LEAVE
			}

			default:
			{
				r = Z_STREAM_ERROR;
				LEAVE
			}
			}
		}
}



// inftrees.c -- generate Huffman trees for efficient decoding
// Copyright (C) 1995-1998 Mark Adler
// For conditions of distribution and use, see copyright notice in zlib.h


/************************* huft_build() ************************
* Huffman code decoding is performed using a multi-level table
* lookup.
//   The fastest way to decode is to simply build a lookup table whose
//   size is determined by the longest code.  However, the time it takes
//   to build this table can also be a factor if the data being decoded
//   is not very long.  The most common codes are necessarily the
//   shortest codes, so those codes dominate the decoding time, and hence
//   the speed.  The idea is you can have a shorter table that decodes the
//   shorter, more probable codes, and then point to subsidiary tables for
//   the longer codes.  The time it costs to decode the longer codes is
//   then traded against the time it takes to make longer tables.
//
//   This results of this trade are in the variables lbits and dbits
//   below.  lbits is the number of bits the first level table for literal/
//   length codes can decode in one step, and dbits is the same thing for
//   the distance codes.  Subsequent tables are also less than or equal to
//   those sizes.  These values may be adjusted either when all of the
//   codes are shorter than that, in which case the longest code length in
//   bits is used, or when the shortest code is *longer* than the requested
//   table size, in which case the length of the shortest code in bits is
//   used.
//
//   There are two different values for the two tables, since they code a
//   different number of possibilities each.  The literal/length table
//   codes 286 possible values, or in a flat code, a little over eight
//   bits.  The distance table codes 30 possible values, or a little less
//   than five bits, flat.  The optimum values for speed end up being
//   about one bit more than those, so lbits is 8+1 and dbits is 5+1.
//   The optimum values may differ though from machine to machine, and
//   possibly even between compilers.  Your mileage may vary.
*/


// If BMAX needs to be larger than 16, then h and x[] should be ULG. 
#define BMAX 15         // maximum bit length of any code

static int huft_build(
	uInt *b,               // code lengths in bits (all assumed <= BMAX)
	uInt n,                 // number of codes (assumed <= 288)
	uInt s,                 // number of simple-valued codes (0..s-1)
	const uInt *d,         // list of base values for non-simple codes
	const uInt *e,         // list of extra bits for non-simple codes
	INFLATE_HUFT * *t,  // result: starting table
	uInt *m,               // maximum lookup bits, returns actual
	INFLATE_HUFT *hp,       // space for trees
	uInt *hn,               // hufts used in space
	uInt *v)               // working area: values in order of bit length
	// Given a list of code lengths and a maximum table size, make a set of
	// tables to decode that set of codes.  Return Z_OK on success, Z_BUF_ERROR
	// if the given code set is incomplete (the tables are still built in this
	// case), or Z_DATA_ERROR if the input is invalid.
{

	uInt a;                       // counter for codes of length k
	uInt c[BMAX + 1];               // bit length count table
	uInt f;                       // i repeats in table every f entries 
	int g;                        // maximum code length 
	int h;                        // table level 
	register uInt i;              // counter, current code 
	register uInt j;              // counter
	register int k;               // number of bits in current code 
	int l;                        // bits per table (returned in m) 
	uInt mask;                    // (1 << w) - 1, to avoid cc -O bug on HP 
	register uInt	*p;				// pointer into c[], b[], or v[]
	INFLATE_HUFT	*q;				// points to current table 
	INFLATE_HUFT	r;				// table entry for structure assignment 
	INFLATE_HUFT	*u[BMAX];		// table stack 
	register int	w;				// bits before this table == (l * h) 
	uInt			x[BMAX + 1];		// bit offsets, then code stack 
	uInt			*xp;			// pointer into x 
	int				y;				// number of dummy codes added 
	uInt			z;				// number of entries in current table 

	// Generate counts for each bit length 
	p = c;
#define C0 *p++ = 0;
#define C2 C0 C0 C0 C0
#define C4 C2 C2 C2 C2
	C4;
	// clear c[]--assume BMAX+1 is 16
	p = b;  i = n;
	do
	{
		c[*p++]++;                  // assume all entries <= BMAX 
	} while (--i);

	if (c[0] == n)                // null input--all zero length codes 
	{
		*t = (INFLATE_HUFT *)0;
		*m = 0;
		return Z_OK;
	}

	// Find minimum and maximum length, bound *m by those 
	l = *m;
	for (j = 1; j <= BMAX; j++)
	{
		if (c[j]) break;
	}
	k = j;                        // minimum code length 
	if ((uInt)l < j) l = j;
	for (i = BMAX; i; i--)
	{
		if (c[i]) break;
	}

	g = i;                        // maximum code length 
	if ((uInt)l > i) l = i;
	*m = l;


	// Adjust last length count to fill out codes, if needed 
	for (y = 1 << j; j < i; j++, y <<= 1)
	{
		if ((y -= c[j]) < 0) return Z_DATA_ERROR;
	}
	if ((y -= c[i]) < 0) return Z_DATA_ERROR;
	c[i] += y;


	// Generate starting offsets into the value table for each length 
	x[1] = j = 0;
	p = c + 1;  xp = x + 2;

	while (--i) // note that i == g from above 
		*xp++ = (j += *p++);

	// Make a table of values in order of bit lengths 
	p = b;  i = 0;
	do
	{
		if ((j = *p++) != 0) v[x[j]++] = i;
	} while (++i < n);
	n = x[g];                     // set n to length of v 


	// Generate the Huffman codes and for each, make the table entries 
	x[0] = i = 0;                 // first Huffman code is zero 
	p = v;                        // grab values in bit order 
	h = -1;                       // no tables yet--level -1 
	w = -l;                       // bits decoded == (l * h) 
	u[0] = (INFLATE_HUFT *)0;        // just to keep compilers happy 
	q = (INFLATE_HUFT *)0;   // ditto 
	z = 0;                        // ditto 

	// go through the bit lengths (k already is bits in shortest code) 
	for (; k <= g; k++)
	{
		a = c[k];
		while (a--)
		{
			// here i is the Huffman code of length k bits for value *p 
			// make tables up to required level 
			while (k > w + l)
			{
				h++;
				w += l;                 // previous table always l bits 

				// compute minimum size table less than or equal to l bits
				z = g - w;
				z = z > (uInt)l ? l : z;        // table size upper limit 
				if ((f = 1 << (j = k - w)) > a + 1)     // try a k-w bit table 
				{                       // too few codes for k-w bit table 
					f -= a + 1;           // deduct codes from patterns left 
					xp = c + k;
					if (j < z)
					{
						while (++j < z)     // try smaller tables up to z bits 
						{
							if ((f <<= 1) <= *++xp)
								break;          // enough codes to use up j bits 
							f -= *xp;         // else deduct codes from patterns
						}
					}
				}

				z = 1 << j;             // table entries for j-bit table 

				// allocate new table
				if (*hn + z > MANY)     // (note: doesn't matter for fixed)
					return Z_DATA_ERROR;  // overflow of MANY
				u[h] = q = hp + *hn;
				*hn += z;

				// connect to last table, if there is one 
				if (h)
				{
					x[h] = i;             // save pattern for backing up
					r.word.what.Bits = (UCH)l;     // bits to dump before this table 
					r.word.what.Exop = (UCH)j;     // bits in this table 
					j = i >> (w - l);
					r.base = (uInt)(q - u[h - 1] - j);   // offset to this table 
					u[h - 1][j] = r;        // connect to last table 
				}
				else
					*t = q;               // first table is returned result 
			}

			// set up table entry in r 
			r.word.what.Bits = (UCH)(k - w);
			if (p >= v + n)
				r.word.what.Exop = 128 + 64;      // out of values--invalid code 
			else if (*p < s)
			{
				r.word.what.Exop = (UCH)(*p < 256 ? 0 : 32 + 64);     // 256 is end-of-block 
				r.base = *p++;          // simple code is just the value 
			}
			else
			{
				r.word.what.Exop = (UCH)(e[*p - s] + 16 + 64);// non-simple--look up in lists 
				r.base = d[*p++ - s];
			}

			// fill code-like entries with r
			f = 1 << (k - w);
			for (j = i >> w; j < z; j += f)	q[j] = r;

			// backwards increment the k-bit code i 
			for (j = 1 << (k - 1); i & j; j >>= 1) i ^= j;
			i ^= j;

			// backup over finished tables 
			mask = (1 << w) - 1;      // needed on HP, cc -O bug 
			while ((i & mask) != x[h])
			{
				h--;                    // don't need to update q
				w -= l;
				mask = (1 << w) - 1;
			}
		}
	}


	// Return Z_BUF_ERROR if we were given an incomplete table 
	return y != 0 && g != 1 ? Z_BUF_ERROR : Z_OK;
}





/******************** inflate_trees_bits() ********************
* c =		19 code lengths
* bb =		Bits tree desired/actual depth
* tb =		Bits tree result
* hp =		Space for trees
*/

static int inflate_trees_bits(uInt *c, uInt *bb, INFLATE_HUFT * *tb, INFLATE_HUFT *hp, Z_STREAM * z)
{
	int		r;
	uInt	hn;			// hufts used in space 
	uInt	*v;

	// Allocate work area for huft_build 
	if (!(v = (uInt *)GlobalAlloc(GMEM_FIXED, 19 * sizeof(uInt))))
		return(Z_MEM_ERROR);

	hn = 0;
	r = huft_build(c, 19, 19, 0, 0, tb, bb, hp, &hn, v);
#ifdef _DEBUG
	if (r == Z_DATA_ERROR)
		z->msg = (char*)"oversubscribed dynamic bit lengths tree";
	else if (r == Z_BUF_ERROR || *bb == 0)
	{
		z->msg = (char*)"incomplete dynamic bit lengths tree";
		r = Z_DATA_ERROR;
	}
#else
	if (r == Z_BUF_ERROR || *bb == 0) r = Z_DATA_ERROR;
#endif

	GlobalFree(v);
	return(r);
}





static int inflate_trees_dynamic(
	uInt nl,                // number of literal/length codes
	uInt nd,                // number of distance codes
	uInt *c,               // that many (total) code lengths
	uInt *bl,              // literal desired/actual bit depth
	uInt *bd,              // distance desired/actual bit depth
	INFLATE_HUFT * *tl, // literal/length tree result
	INFLATE_HUFT * *td, // distance tree result
	INFLATE_HUFT *hp,       // space for trees
	Z_STREAM * z)            // for messages
{
	int		r;
	uInt	hn;			// hufts used in space 
	uInt	*v;			// work area for huft_build 

	// Allocate work area 
	if (!(v = (uInt *)GlobalAlloc(GMEM_FIXED, 288 * sizeof(uInt))))
		return(Z_MEM_ERROR);

	// Build literal/length tree 
	hn = 0;
	r = huft_build(c, nl, 257, CpLens, CpLExt, tl, bl, hp, &hn, v);
	if (r != Z_OK || *bl == 0)
	{
#ifdef _DEBUG
		if (r == Z_DATA_ERROR)
			z->msg = (char *)"oversubscribed literal/length tree";
		else if (r != Z_MEM_ERROR)
		{
			z->msg = (char *)"incomplete literal/length tree";
			r = Z_DATA_ERROR;
		}
#else
		if (r == Z_MEM_ERROR) r = Z_DATA_ERROR;
#endif
		goto bad;
	}

	// Build distance tree 
	r = huft_build(c + nl, nd, 0, CpDist, CpDExt, td, bd, hp, &hn, v);
	if (r != Z_OK || (*bd == 0 && nl > 257))
	{
#ifdef _DEBUG
		if (r == Z_DATA_ERROR)
			z->msg = (char*)"oversubscribed distance tree";
		else if (r == Z_BUF_ERROR)
		{
			z->msg = (char*)"incomplete distance tree";
			r = Z_DATA_ERROR;
		}
		else if (r != Z_MEM_ERROR)
		{
			z->msg = (char*)"empty distance tree with lengths";
			r = Z_DATA_ERROR;
		}
#else
		if (r == Z_MEM_ERROR || r == Z_BUF_ERROR) r = Z_DATA_ERROR;
#endif
	bad:	GlobalFree(v);
		return(r);
	}

	// done 
	GlobalFree(v);
	return(Z_OK);
}





// inffast.c -- process literals and length/distance pairs fast
// Copyright (C) 1995-1998 Mark Adler
// For conditions of distribution and use, see copyright notice in zlib.h
//




// macros for bit input with no checking and for returning unused bytes 
#define GRABBITS(j) {while(k<(j)){b|=((ULG)NEXTBYTE)<<k;k+=8;}}
#define UNGRAB {c=(uInt)z->avail_in-n;c=(k>>3)<c?k>>3:c;n+=c;p-=c;k-=c<<3;}

// Called with number of bytes left to write in window at least 258
// (the maximum string length) and number of input bytes available
// at least ten.  The ten bytes are six bytes for the longest length/
// distance pair plus four bytes for overloading the bit buffer. 

static int inflate_fast(uInt bl, uInt bd, const INFLATE_HUFT *tl, const INFLATE_HUFT *td, INFLATE_BLOCKS_STATE *s, Z_STREAM * z)
{
	const INFLATE_HUFT *t;      // temporary pointer 
	uInt e;               // extra bits or operation 
	ULG b;              // bit buffer 
	uInt k;               // bits in bit buffer 
	UCH *p;             // input data pointer 
	uInt n;               // bytes available there 
	UCH *q;             // output window write pointer 
	uInt m;               // bytes to end of window or read pointer 
	uInt ml;              // mask for literal/length tree
	uInt md;              // mask for distance tree 
	uInt c;               // bytes to copy 
	uInt d;               // distance back to copy from 
	UCH *r;             // copy source pointer 

	// load input, output, bit values 
	LOAD

		// initialize masks 
		ml = inflate_mask[bl];
	md = inflate_mask[bd];

	// do until not enough input or output space for fast loop 
	do
	{		// assume called with m >= 258 && n >= 10 
		// get literal/length code 
		GRABBITS(20)                // max bits for literal/length code 
			if ((e = (t = tl + ((uInt)b & ml))->word.what.Exop) == 0)
			{
				DUMPBITS(t->word.what.Bits)
#ifdef _DEBUG
					LuTracevv((stderr, t->base >= 0x20 && t->base < 0x7f ? "inflate:         * literal '%c'\n" : "inflate:         * literal 0x%02x\n", t->base));
#endif
				*q++ = (UCH)t->base;
				--m;
				continue;
			}

		for (;;)
		{
			DUMPBITS(t->word.what.Bits)
				if (e & 16)
				{
					// get extra bits for length 
					e &= 15;
					c = t->base + ((uInt)b & inflate_mask[e]);
					DUMPBITS(e)
#ifdef _DEBUG
						LuTracevv((stderr, "inflate:         * length %u\n", c));
#endif
					// decode distance base of block to copy 
					GRABBITS(15);           // max bits for distance code 
					e = (t = td + ((uInt)b & md))->word.what.Exop;
					for (;;)
					{
						DUMPBITS(t->word.what.Bits)
							if (e & 16)
							{
								// get extra bits to add to distance base 
								e &= 15;
								GRABBITS(e)         // get extra bits (up to 13) 
									d = t->base + ((uInt)b & inflate_mask[e]);
								DUMPBITS(e)
#ifdef _DEBUG
									LuTracevv((stderr, "inflate:         * distance %u\n", d));
#endif
								// do the copy
								m -= c;
								r = q - d;
								if (r < s->window)                  // wrap if needed
								{
									do
									{
										r += s->end - s->window;        // force pointer in window
									} while (r < s->window);          // covers invalid distances
									e = (uInt)(s->end - r);
									if (c > e)
									{
										c -= e;                         // wrapped copy
										do
										{
											*q++ = *r++;
										} while (--e);
										r = s->window;

										do
										{
											*q++ = *r++;
										} while (--c);
									}
									else                              // normal copy
									{
										*q++ = *r++;  c--;
										*q++ = *r++;  c--;
										do
										{
											*q++ = *r++;
										} while (--c);
									}
								}
								else                                /* normal copy */
								{
									*q++ = *r++;  c--;
									*q++ = *r++;  c--;
									do
									{
										*q++ = *r++;
									} while (--c);
								}
								break;
							}
							else if ((e & 64) == 0)
							{
								t += t->base;
								e = (t += ((uInt)b & inflate_mask[e]))->word.what.Exop;
							}
							else
							{
#ifdef _DEBUG
								z->msg = (char*)"invalid distance code";
#endif
								UNGRAB
									UPDATE
									return Z_DATA_ERROR;
							}
					};

					break;
				}

			if ((e & 64) == 0)
			{
				t += t->base;
				if ((e = (t += ((uInt)b & inflate_mask[e]))->word.what.Exop) == 0)
				{
					DUMPBITS(t->word.what.Bits)
#ifdef _DEBUG
						LuTracevv((stderr, t->base >= 0x20 && t->base < 0x7f ? "inflate:         * literal '%c'\n" : "inflate:         * literal 0x%02x\n", t->base));
#endif
					*q++ = (UCH)t->base;
					--m;
					break;
				}
			}
			else if (e & 32)
			{
#ifdef _DEBUG
				LuTracevv((stderr, "inflate:         * end of block\n"));
#endif
				UNGRAB
					UPDATE
					return Z_STREAM_END;
			}
			else
			{
#ifdef _DEBUG
				z->msg = (char*)"invalid literal/length code";
#endif
				UNGRAB
					UPDATE
					return Z_DATA_ERROR;
			}
		};
	} while (m >= 258 && n >= 10);

	// not enough input or output--restore pointers and return
	UNGRAB
		UPDATE
		return Z_OK;
}






/************************ ucrc32() ********************
* Computes the CRC-32 of the bytes in the specified
* buffer.
*
* crc =	Initial CRC-32 value.
* buf =	Pointer to buffer containing bytes.
* len =	Length of "buf" in bytes.
*
* RETURNS: The updated CRC-32.
*
* Copyright (C) 1995-1998 Mark Adler. For conditions of
* distribution and use, see copyright notice in zlib.h
*/

#define CRC_DO1(buf) crc = Crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define CRC_DO2(buf)  CRC_DO1(buf); CRC_DO1(buf);
#define CRC_DO4(buf)  CRC_DO2(buf); CRC_DO2(buf);
#define CRC_DO8(buf)  CRC_DO4(buf); CRC_DO4(buf);

static ULG ucrc32(ULG crc, const UCH *buf, DWORD len)
{
	crc = crc ^ 0xffffffffL;
	while (len >= 8)
	{
		CRC_DO8(buf);
		len -= 8;
	}
	if (len)
	{
		do
		{
			CRC_DO1(buf);
		} while (--len);
	}
	return(crc ^ 0xffffffffL);
}




/************************ adler32() ******************
* Computes the Adler-32 checksum of the bytes in the
* specified buffer.
*
* crc =	Initial Adler-32 value.
* buf =	Pointer to buffer containing bytes.
* len =	Length of "buf" in bytes.
*
* RETURNS: The updated Adler-32.
*
* An Adler-32 checksum is almost as reliable as a CRC32
* but can be computed much faster. Usage example:
*
* Copyright (C) 1995-1998 Mark Adler. For conditions of
* distribution and use, see copyright notice in zlib.h
*/
/*
#define BASE 65521L // largest prime smaller than 65536
#define NMAX 5552
// NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1

#define AD_DO1(buf,i)  {s1 += buf[i]; s2 += s1;}
#define AD_DO2(buf,i)  AD_DO1(buf,i); AD_DO1(buf,i+1);
#define AD_DO4(buf,i)  AD_DO2(buf,i); AD_DO2(buf,i+2);
#define AD_DO8(buf,i)  AD_DO4(buf,i); AD_DO4(buf,i+4);
#define AD_DO16(buf)   AD_DO8(buf,0); AD_DO8(buf,8);

static ULG adler32(ULG adler, const UCH *buf, DWORD len)
{
unsigned long s1 = adler & 0xffff;
unsigned long s2 = (adler >> 16) & 0xffff;
int k;

while (len > 0)
{
k = len < NMAX ? len : NMAX;
len -= k;
while (k >= 16)
{
AD_DO16(buf);
buf += 16;
k -= 16;
}
if (k != 0) do
{
s1 += *buf++;
s2 += s1;
} while (--k);
s1 %= BASE;
s2 %= BASE;
}
return (s2 << 16) | s1;
}
*/




// Does decryption and encryption of data
#define CRC32(c, b) (Crc_table[((int)(c)^(b))&0xff]^((c)>>8))
static void Uupdate_keys(unsigned long *keys, char c)
{
	keys[0] = CRC32(keys[0], c);
	keys[1] += keys[0] & 0xFF;
	keys[1] = keys[1] * 134775813L + 1;
	keys[2] = CRC32(keys[2], keys[1] >> 24);
}






/************************** inflate() ***********************
* Decompresses as much data as possible, and stops when the
* input buffer becomes empty or the output buffer becomes
* full. This introduces some output latency (reading input
* without producing any output) except when forced to flush.
*
* inflate performs one or both of the following actions:
*
* - Decompress more input starting at next_in and update
*   next_in and avail_in accordingly. If not all input can
*   be processed (because there is not enough room in the
*   output buffer), next_in is updated and processing will
*   resume at this point for the next call of inflate().
*
* - Provide more output starting at next_out and update
*   next_out and avail_out accordingly. inflate() provides
*   as much output as possible, until there is no more input
*   data or no more space in the output buffer (see below
*   about the flush parameter).
*
* Before calling inflate(), the caller should ensure
* that at least one of the actions is possible, by
* providing more input and/or consuming more output, and
* updating the next_* and avail_* values accordingly.
* The caller can consume the uncompressed output when
* it wants, for example when the output buffer is full
* (avail_out == 0), or after each call of inflate(). If
* inflate returns Z_OK and with zero avail_out, it must be
* called again after making room in the output buffer because
* there could be more output pending.
*
* If the parameter flush is set to Z_SYNC_FLUSH, inflate
* flushes as much output as possible to the output buffer.
* The flushing behavior of inflate is not specified for
* values of the flush parameter other than Z_SYNC_FLUSH
* and Z_FINISH, but the current implementation actually
* flushes as much output as possible anyway.
*
* inflate() should normally be called until it returns
* Z_STREAM_END or an error. However if all decompression is
* to be performed in a single step (a single call of inflate),
* the parameter flush should be set to Z_FINISH. In this case,
* all pending input is processed and all pending output is
* flushed; avail_out must be large enough to hold all the
* uncompressed data. (The size of the uncompressed data may
* have been saved by the compressor for this purpose.) The
* next operation on this stream must be inflateEnd to
* deallocate the decompression state. The use of Z_FINISH
* is never required, but can be used to inform inflate that a
* faster routine may be used for the single inflate() call.
*
* If a preset dictionary is needed at this point (see
* inflateSetDictionary below), inflate sets strm-adler to the
* adler32 checksum of the dictionary chosen by the compressor
* and returns Z_NEED_DICT; otherwise it sets strm->adler to the
* adler32 checksum of all output produced so far (that is,
* total_out bytes) and returns Z_OK, Z_STREAM_END or an error
* code as described below. At the end of the stream, inflate()
* checks that its computed adler32 checksum is equal to that
* saved by the compressor and returns Z_STREAM_END only if the
* checksum is correct.
*
* RETURNS: Z_OK if some progress has been made (more input processed
* or more output produced), Z_STREAM_END if the end of the compressed data has
* been reached and all uncompressed output has been produced, Z_NEED_DICT if a
* preset dictionary is needed at this point, Z_DATA_ERROR if the input data was
* corrupted (input stream not conforming to the zlib format or incorrect
* adler32 checksum), Z_STREAM_ERROR if the stream structure was inconsistent
* (for example if next_in or next_out was NULL), Z_MEM_ERROR if there was not
* enough memory, Z_BUF_ERROR if no progress is possible or if there was not
* enough room in the output buffer when Z_FINISH is used. In the Z_DATA_ERROR
* case, the application may then call inflateSync to look for a good
* compression block.
*/

#define IM_NEEDBYTE {if (z->avail_in == 0) return r; r=f;}
#define IM_NEXTBYTE (--z->avail_in, ++z->total_in, *(z->next_in)++)

static int inflate(Z_STREAM * z, int f)
{
	int		r;
	uInt	b;

	f = f == Z_FINISH ? Z_BUF_ERROR : Z_OK;
	r = Z_BUF_ERROR;
	for (;;)
	{
		switch (z->state->mode)
		{
		case IM_METHOD:
		{
			IM_NEEDBYTE
				if (((z->state->sub.method = IM_NEXTBYTE) & 0xf) != Z_DEFLATED)
				{
					z->state->mode = IM_BAD;
#ifdef _DEBUG
					z->msg = (char*)"unknown compression method";
#endif
					z->state->sub.marker = 5;       // can't try inflateSync
					break;
				}

			if ((z->state->sub.method >> 4) + 8 > z->state->wbits)
			{
				z->state->mode = IM_BAD;
#ifdef _DEBUG
				z->msg = (char*)"invalid window size";
#endif
				z->state->sub.marker = 5;       // can't try inflateSync
				break;
			}
			z->state->mode = IM_FLAG;
		}

		case IM_FLAG:
		{
			IM_NEEDBYTE
				b = IM_NEXTBYTE;
			if (((z->state->sub.method << 8) + b) % 31)
			{
				z->state->mode = IM_BAD;
#ifdef _DEBUG
				z->msg = (char*)"incorrect header check";
#endif
				z->state->sub.marker = 5;       // can't try inflateSync 
				break;
			}
#ifdef _DEBUG
			LuTracev((stderr, "inflate: zlib header ok\n"));
#endif
			if (!(b & PRESET_DICT))
			{
				z->state->mode = IM_BLOCKS;
				break;
			}
			z->state->mode = IM_DICT4;
		}

		case IM_DICT4:
		{
			IM_NEEDBYTE
				z->state->sub.check.need = (ULG)IM_NEXTBYTE << 24;
			z->state->mode = IM_DICT3;
		}

		case IM_DICT3:
		{
			IM_NEEDBYTE
				z->state->sub.check.need += (ULG)IM_NEXTBYTE << 16;
			z->state->mode = IM_DICT2;
		}

		case IM_DICT2:
		{
			IM_NEEDBYTE
				z->state->sub.check.need += (ULG)IM_NEXTBYTE << 8;
			z->state->mode = IM_DICT1;
		}

		case IM_DICT1:
		{
			IM_NEEDBYTE;
			z->state->sub.check.need += (ULG)IM_NEXTBYTE;
			//				z->adler = z->state->sub.check.need;
			z->state->mode = IM_DICT0;
			return Z_NEED_DICT;
		}

		case IM_DICT0:
		{
			z->state->mode = IM_BAD;
#ifdef _DEBUG
			z->msg = (char*)"need dictionary";
#endif
			z->state->sub.marker = 0;       // can try inflateSync 
			return Z_STREAM_ERROR;
		}

		case IM_BLOCKS:
		{
			r = inflate_blocks(z, r);
			if (r == Z_DATA_ERROR)
			{
				z->state->mode = IM_BAD;
				z->state->sub.marker = 0;       // can try inflateSync 
				break;
			}
			if (r == Z_OK)
				r = f;
			if (r != Z_STREAM_END)
				return r;
			r = f;
			inflate_blocks_reset(z);
			//			if (z->state->nowrap)
			//			{
			z->state->mode = IM_DONE;
			break;
			//			}
			//			z->state->mode = IM_CHECK4;
		}

		case IM_CHECK4:
		{
			IM_NEEDBYTE
				z->state->sub.check.need = (ULG)IM_NEXTBYTE << 24;
			z->state->mode = IM_CHECK3;
		}

		case IM_CHECK3:
		{
			IM_NEEDBYTE
				z->state->sub.check.need += (ULG)IM_NEXTBYTE << 16;
			z->state->mode = IM_CHECK2;
		}

		case IM_CHECK2:
		{
			IM_NEEDBYTE
				z->state->sub.check.need += (ULG)IM_NEXTBYTE << 8;
			z->state->mode = IM_CHECK1;
		}

		case IM_CHECK1:
		{
			IM_NEEDBYTE
				z->state->sub.check.need += (ULG)IM_NEXTBYTE;

			if (z->state->sub.check.was != z->state->sub.check.need)
			{
				z->state->mode = IM_BAD;
#ifdef _DEBUG
				z->msg = (char*)"incorrect data check";
#endif
				z->state->sub.marker = 5;       // can't try inflateSync 
				break;
			}

#ifdef _DEBUG
			LuTracev((stderr, "inflate: zlib check ok\n"));
#endif
			z->state->mode = IM_DONE;
		}

		case IM_DONE:
			return(Z_STREAM_END);

		case IM_BAD:
			return(Z_DATA_ERROR);

		default:
			return(Z_STREAM_ERROR);
		}
	}
}





/********************** seekInZip() *********************
* Sets the current "file position" within the ZIP archive.
*/

static int seekInZip(TUNZIP *tunzip, LONGLONG offset, DWORD reference)
{
	if (!(tunzip->Flags & TZIP_ARCMEMORY))
	{
		LARGE_INTEGER aPointer;
		if (reference == FILE_BEGIN)
		{
			aPointer.QuadPart = tunzip->InitialArchiveOffset + offset;
			if (!SetFilePointerEx(tunzip->ArchivePtr, aPointer, &aPointer, FILE_BEGIN))
				return(ZR_SEEK);
			offset = aPointer.QuadPart;
		}
		else if (reference == FILE_CURRENT)
		{
			aPointer.QuadPart = offset;
			if (!SetFilePointerEx(tunzip->ArchivePtr, aPointer, &aPointer, FILE_CURRENT))
				return(ZR_SEEK);
			offset = aPointer.QuadPart;
		}
		//		else if (reference == FILE_END) offset = SetFilePointer(tunzip->ArchivePtr, offset, 0, FILE_END);
		//		if (offset == -1) return(ZR_SEEK);
	}
	else
	{
		if (reference == FILE_BEGIN) tunzip->ArchiveBufPos = offset;
		else if (reference == FILE_CURRENT) tunzip->ArchiveBufPos += offset;
		//		else if (reference == FILE_END) tunzip->ArchiveBufPos = tunzip->ArchiveBufLen + offset;
		//		if (tunzip->ArchiveBufPos > tunzip->ArchiveBufLen) tunzip->ArchiveBufPos = tunzip->ArchiveBufLen;
	}
	return(0);
}





/********************** readFromZip() *********************
* Reads the specified number of bytes from the ZIP archive
* (starting at the current position) and copies them to
* the specified buffer.
*
* RETURNS: The number of bytes actually read (could be less
* than the requested number if the end of file is reached)
* or 0 if no more bytes to read (or an error).
*
* NOTE: If an error, than TUNZIP->LastErr is non-zero.
*/

static DWORD readFromZip(TUNZIP *tunzip, void *ptr, DWORD toread)
{
	// Reading from a handle?
	if (!(tunzip->Flags & TZIP_ARCMEMORY))
	{
		DWORD	readb;

		if (!ReadFile(tunzip->ArchivePtr, ptr, toread, &readb, 0))
		{
			tunzip->LastErr = ZR_READ;
			readb = 0;
		}
		return(readb);
	}

	// Reading from memory
	if (tunzip->ArchiveBufPos + toread > tunzip->ArchiveBufLen) toread = (DWORD)(tunzip->ArchiveBufLen - tunzip->ArchiveBufPos);
	CopyMemory(ptr, (unsigned char *)tunzip->ArchivePtr + tunzip->ArchiveBufPos, toread);
	tunzip->ArchiveBufPos += toread;
	return(toread);
}






static unsigned char * reformat_short(unsigned char *ptr)
{
	*(unsigned short *)ptr = ((unsigned short)*ptr) | (((unsigned short)*(ptr + 1)) << 8);
	return (unsigned char *)((unsigned short *)ptr + 1);
}

// Reads a short in LSB order from the given file.
static unsigned short getArchiveShort(TUNZIP *tunzip)
{
	unsigned short		x;

	if (!tunzip->LastErr && readFromZip(tunzip, &x, sizeof(unsigned short)))
		reformat_short((unsigned char *)&x);
	return(x);
}

static unsigned char * reformat_long(unsigned char *ptr)
{
	*(unsigned long *)ptr = ((unsigned long)*ptr) | (((unsigned long)*(ptr + 1)) << 8) | (((unsigned long)*(ptr + 2)) << 16) | (((unsigned long)*(ptr + 3)) << 24);
	return (unsigned char *)((unsigned long *)ptr + 1);
}

// Reads a long in LSB order from the given file
static ULG getArchiveLong(TUNZIP *tunzip)
{
	ULG		x;

	x = 0;
	if (!tunzip->LastErr && readFromZip(tunzip, &x, sizeof(ULG)))
		reformat_long((unsigned char *)&x);
	return(x);
}


static unsigned char * reformat_longlong(unsigned char *ptr)
{
	*(ULONGLONG *)ptr = ((ULONGLONG)*ptr) | (((ULONGLONG)*(ptr + 1)) << 8) | (((ULONGLONG)*(ptr + 2)) << 16) | (((ULONGLONG)*(ptr + 3)) << 24) | (((ULONGLONG)*(ptr + 4)) << 32) | (((ULONGLONG)*(ptr + 5)) << 40) | (((ULONGLONG)*(ptr + 6)) << 48) | (((ULONGLONG)*(ptr + 7)) << 56);
	return (unsigned char *)((ULONGLONG *)ptr + 1);
}
// Reads a longlong in LSB order from the given file
static ULONGLONG getArchiveLongLong(TUNZIP *tunzip)
{
	ULONGLONG	x;

	x = 0;
	if (!tunzip->LastErr && readFromZip(tunzip, &x, sizeof(ULONGLONG)))
		reformat_longlong((unsigned char *)&x);
	return(x);
}





/********************* skipToEntryEnd() *******************
* Skips the remainder of the currently selected entry's
* table in the ZIP archive.
*
* TUNZIP must be initialized to contain information about
* that entry via a call to goToFirstEntry() or goToNextEntry(),
* and getEntryInfo() must be called prior to this.
*
* NOTE: If an error, than TUNZIP->LastErr is set non-zero,
* so caller must clear it first if needed.
*/
/*
static void skipToEntryEnd(TUNZIP *tunzip, char **extraField)
{
register DWORD	lSeek;

lSeek = tunzip->CurrentEntryInfo.size_file_extra;

// If he doesn't want extra info returned, then he wants to skip past the
// extra header, and comment fields. Otherwise, we read just the extra
// header (and don't bother skipping the comment). Caller is responsible
// for GlobalFree'ing the extra info
if (extraField)
{
char	*extra;

if (!(extra = GlobalAlloc(GMEM_FIXED, lSeek)))
tunzip->LastErr = ZR_NOALLOC;
else
{
if (readFromZip(tunzip, extra, lSeek) != lSeek)
{
GlobalFree(extra);
goto bad;
}

*extraField = extra;
}
}

// Skip the extra header and comment
else if ((lSeek += tunzip->CurrentEntryInfo.size_file_comment) && seekInZip(tunzip, lSeek, FILE_CURRENT))
bad:	tunzip->LastErr = ZR_CORRUPT;
}
*/





/********************* getEntryFN() *******************
* Gets the currently selected entry's filename in the
* zip archive.
*
* TUNZIP must be initialized to contain information about
* that entry via a call to goToFirstEntry() or goToNextEntry(),
* and getEntryInfo() must be called prior to this.
*
* NOTE: If an error, than TUNZIP->LastErr is set non-zero,
* so caller must clear it first if needed.
*/

static void getEntryFN(TUNZIP *tunzip, char *szFileName)
{
	register DWORD		uSizeRead;
	register DWORD		lSeek;

	lSeek = tunzip->CurrentEntryInfo.size_filename;

	// Make sure filename will fit in the supplied buffer (MAX_PATH)
	if (lSeek < MAX_PATH)
		uSizeRead = lSeek;
	else
		uSizeRead = MAX_PATH - 1;

	// Read the filename
	if (uSizeRead && readFromZip(tunzip, szFileName, uSizeRead) != uSizeRead)
	bad:	tunzip->LastErr = ZR_CORRUPT;
	else
	{
		// If there are any bytes left that we didn't read, skip them
		lSeek -= uSizeRead;
		if (lSeek && seekInZip(tunzip, lSeek, FILE_CURRENT)) goto bad;
	}

	szFileName[uSizeRead] = 0;
}





/********************* getEntryInfo() *******************
* Gets info about the currently selected entry in the
* zip archive.
*
* TUNZIP must be initialized to contain information about
* that entry via a call to goToFirstEntry() or goToNextEntry().
*
* NOTE: If an error, than TUNZIP->LastErr is set non-zero,
* so caller must clear it first.
*/

static void getEntryInfo(register TUNZIP *tunzip)
{
	ULONGLONG		uSizeRead;
	DWORD		lSeek;

	// Seek to the start of this entry's info in the Central Directory
	if (!seekInZip(tunzip, tunzip->CurrEntryPosInCentralDir + tunzip->ByteBeforeZipArchive, FILE_BEGIN))
	{
		if (tunzip->Flags & TZIP_GZIP)
		{
			BYTE	flag, chr;

			ZeroMemory(&tunzip->CurrentEntryInfo, sizeof(ZIPENTRYINFO));

			if (tunzip->Flags & TZIP_RAW)
			{
				tunzip->CurrentEntryInfo.compression_method = Z_DEFLATED;
				tunzip->CurrentEntryInfo.offset = (DWORD)(tunzip->CurrEntryPosInCentralDir + tunzip->ByteBeforeZipArchive);
				goto out;
			}

			if (readFromZip(tunzip, &flag, 1) != 1 ||
				readFromZip(tunzip, &tunzip->CurrentEntryInfo.dosDate, 4) != 4 ||
				seekInZip(tunzip, 2, FILE_CURRENT))
			{
				goto bad;
			}

			if ((flag & 0x04) && (readFromZip(tunzip, &tunzip->CurrentEntryInfo.disk_num_start, 2) != 2 || seekInZip(tunzip, tunzip->CurrentEntryInfo.disk_num_start, FILE_CURRENT))) goto bad;

			// Save offset to filename
			if (!(tunzip->Flags & TZIP_ARCMEMORY))
			{
				LARGE_INTEGER aOffset;
				aOffset.QuadPart = 0;
				SetFilePointerEx(tunzip->ArchivePtr, aOffset, &aOffset, FILE_CURRENT);
				uSizeRead = aOffset.QuadPart;
			}
			else
				uSizeRead = tunzip->ArchiveBufPos;

			// Skip over filename, counting length
			if (flag & 0x08)
			{
				for (;;)
				{
					if (readFromZip(tunzip, &chr, 1) != 1) goto bad;
					if (!chr) break;
					++tunzip->CurrentEntryInfo.size_filename;
				}
			}

			// Skip over comment
			if (flag & 0x10)
			{
				do
				{
					if (readFromZip(tunzip, &chr, 1) != 1) goto bad;
				} while (chr);
			}

			// Skip compress info and OS bytes
			if ((flag & 0x02) && seekInZip(tunzip, 2, FILE_CURRENT)) goto bad;

			tunzip->CurrentEntryInfo.compression_method = Z_DEFLATED;

			if (!(tunzip->Flags & TZIP_ARCMEMORY))
			{
				// Save offset to data
				tunzip->CurrentEntryInfo.offset = SetFilePointer(tunzip->ArchivePtr, 0, 0, FILE_CURRENT);
				tunzip->CurrentEntryInfo.compressed_size = SetFilePointer(tunzip->ArchivePtr, -8, 0, FILE_END) - tunzip->CurrentEntryInfo.offset;
				tunzip->CurrentEntryInfo.crc = getArchiveLong(tunzip);
				tunzip->CurrentEntryInfo.uncompressed_size = getArchiveLong(tunzip);
			}
			else
			{
				tunzip->CurrentEntryInfo.offset = (DWORD)tunzip->ArchiveBufPos;
				tunzip->CurrentEntryInfo.compressed_size = (DWORD)(tunzip->ArchiveBufLen - tunzip->ArchiveBufPos - 8);
				CopyMemory((void *)&tunzip->CurrentEntryInfo.crc, (unsigned char *)tunzip->ArchivePtr + tunzip->ArchiveBufLen - 8, 4);
				reformat_long((unsigned char *)&tunzip->CurrentEntryInfo.crc);
				CopyMemory((void *)&tunzip->CurrentEntryInfo.uncompressed_size, (unsigned char *)tunzip->ArchivePtr + tunzip->ArchiveBufLen - 4, 4);
				reformat_long((unsigned char *)&tunzip->CurrentEntryInfo.uncompressed_size);
			}

			// Seek to name offset upon return
			seekInZip(tunzip, uSizeRead, FILE_BEGIN);
			goto out;
		}
		DWORD aSizeRead;
		// Get the ZIP signature and check it
		aSizeRead = getArchiveLong(tunzip);
		if (aSizeRead == 0x02014b50 &&

			// Read in the ZIPENTRYINFO
			readFromZip(tunzip, &tunzip->CurrentEntryInfo, sizeof(ZIPENTRYINFO)) == sizeof(ZIPENTRYINFO))
		{
			unsigned char	*ptr;

			// Adjust the various fields to this CPU's byte order
			aSizeRead = ZIP_FIELDS_REFORMAT;
			ptr = (unsigned char *)&tunzip->CurrentEntryInfo;
			for (lSeek = 0; lSeek < NUM_FIELDS_REFORMAT; lSeek++)
			{
				if (0x00000001 & aSizeRead) ptr = reformat_long(ptr);
				else ptr = reformat_short(ptr);
				aSizeRead >>= 1;
			}

		out:		return;
		}
	}
bad:
	tunzip->LastErr = ZR_CORRUPT;
}





/******************** goToFirstEntry() *******************
* Set the current entry to the first entry in the ZIP
* archive.
*/

static void goToFirstEntry(register TUNZIP *tunzip)
{
	if (tunzip->TotalEntries)
	{
		tunzip->CurrEntryPosInCentralDir = tunzip->CentralDirOffset;
		tunzip->CurrentEntryNum = 0;
		getEntryInfo(tunzip);
	}
	else
		tunzip->LastErr = ZR_NOTFOUND;
}





/******************** goToNextEntry() *******************
* Set the current entry to the next entry in the ZIP
* archive.
*/

static void goToNextEntry(register TUNZIP *tunzip)
{
	if (tunzip->CurrentEntryNum + 1 < tunzip->TotalEntries)
	{
		tunzip->CurrEntryPosInCentralDir += SIZECENTRALDIRITEM + tunzip->CurrentEntryInfo.size_filename + tunzip->CurrentEntryInfo.size_file_extra + tunzip->CurrentEntryInfo.size_file_comment;
		++tunzip->CurrentEntryNum;
		getEntryInfo(tunzip);
	}
	else
		tunzip->LastErr = ZR_NOTFOUND;
}





/******************* inflateEnd() ********************
* Frees low level DEFLATE buffers/structs.
*
* NOTE: Must not alter TUNZIP->LastErr!
*/

static void inflateEnd(register ENTRYREADVARS *entryReadVars)
{
	register void *ptr;

	if (entryReadVars->stream.state)
	{
		switch (entryReadVars->stream.state->blocks.mode)
		{
		case IBM_BTREE:
		case IBM_DTREE:
			if ((ptr = entryReadVars->stream.state->blocks.sub.trees.blens))
			{
				g_memset(ptr, 0, GlobalSize(ptr));
				GlobalFree(ptr);
			}
			break;

		case IBM_CODES:
			if ((ptr = entryReadVars->stream.state->blocks.sub.decode.codes))
			{
				g_memset(ptr, 0, GlobalSize(ptr));
				GlobalFree(ptr);
			}
		}

		if ((ptr = entryReadVars->stream.state->blocks.window))
		{
			g_memset(ptr, 0, GlobalSize(ptr));
			GlobalFree(ptr);
		}
		if ((ptr = entryReadVars->stream.state->blocks.hufts))
		{
			g_memset(ptr, 0, GlobalSize(ptr));
			GlobalFree(ptr);
		}
		g_memset(entryReadVars->stream.state, 0, GlobalSize(entryReadVars->stream.state));
		GlobalFree(entryReadVars->stream.state);

#ifdef _DEBUG
		LuTracev((stderr, "inflate: freed\n"));
#endif
	}
}





/****************** cleanupEntry() ******************
* Frees resources allocated by initEntry().
*
* NOTE: Must not alter TUNZIP->LastErr!
*/

static void cleanupEntry(register TUNZIP * tunzip)
{
	// Free the input buffer
	if (tunzip->EntryReadVars.InputBuffer)
	{
		g_memset(tunzip->EntryReadVars.InputBuffer, 0, GlobalSize(tunzip->EntryReadVars.InputBuffer));
		GlobalFree(tunzip->EntryReadVars.InputBuffer);
	}
	tunzip->EntryReadVars.InputBuffer = 0;

	// Free stuff allocated for decompressing in DEFLATE mode
	if (tunzip->EntryReadVars.stream.state) inflateEnd(&tunzip->EntryReadVars);
	tunzip->EntryReadVars.stream.state = 0;

	// No currently selected entry
	tunzip->CurrentEntryNum = (ULONGLONG)-1;
}





/********************** initEntry() *********************
* Initializes structures in preparation of unzipping
* the current entry.
*
* NOTE: Sets TUNZIP->LastErr non-zero if an error, so this
* must be cleared before calling.
*/

static void initEntry(register TUNZIP *tunzip, ZIPENTRY *ze)
{
	register ULONGLONG	offset;

	// Clear out the ENTRYREADVARS struct
	ZeroMemory(&tunzip->EntryReadVars, sizeof(ENTRYREADVARS));

	// Get a read buffer to input, and decrypt, bytes from the ZIP archive
	if (!(tunzip->EntryReadVars.InputBuffer = (unsigned char *)GlobalAlloc(GMEM_FIXED, UNZ_BUFSIZE)))
	{
	badalloc:
		tunzip->LastErr = ZR_NOALLOC;
	badout:	cleanupEntry(tunzip);
		return;
	}

	// If DEFLATE compression method, we need to get some resources for the deflate routines
	if (tunzip->CurrentEntryInfo.compression_method)
	{
		if (!(tunzip->EntryReadVars.stream.state = (INTERNAL_STATE *)GlobalAlloc(GMEM_FIXED, sizeof(INTERNAL_STATE)))) goto badalloc;
		ZeroMemory(tunzip->EntryReadVars.stream.state, sizeof(INTERNAL_STATE));

		// MAX_WBITS: 32K LZ77 window
		tunzip->EntryReadVars.stream.state->wbits = 15;

		// Handle undocumented nowrap option (no zlib header or check)
		//		tunzip->EntryReadVars.stream->state.nowrap = 1;

		tunzip->EntryReadVars.stream.state->mode = IM_BLOCKS;
		//		tunzip->EntryReadVars.stream.state->mode = tunzip->EntryReadVars.stream.state->nowrap ? IM_BLOCKS : IM_METHOD;

		// Initialize INFLATE_BLOCKS_STATE
		tunzip->EntryReadVars.stream.state->blocks.mode = IBM_TYPE;
		if (!(tunzip->EntryReadVars.stream.state->blocks.hufts = (INFLATE_HUFT *)GlobalAlloc(GMEM_FIXED, sizeof(INFLATE_HUFT) * MANY))) goto badalloc;
		if (!(tunzip->EntryReadVars.stream.state->blocks.window = (UCH *)GlobalAlloc(GMEM_FIXED, 1 << 15))) goto badalloc;
		tunzip->EntryReadVars.stream.state->blocks.end = tunzip->EntryReadVars.stream.state->blocks.window + (1 << 15);
		tunzip->EntryReadVars.stream.state->blocks.read = tunzip->EntryReadVars.stream.state->blocks.write = tunzip->EntryReadVars.stream.state->blocks.window;

#ifdef _DEBUG
		LuTracev((stderr, "inflate: allocated\n"));
#endif
	}

	// If raw mode, app must supply the compressed and uncompressed sizes
	if (tunzip->Flags & TZIP_RAW)
	{
		tunzip->CurrentEntryInfo.compressed_size = (DWORD)ze->CompressedSize;
		tunzip->CurrentEntryInfo.uncompressed_size = (DWORD)ze->UncompressedSize;
		tunzip->CurrentEntryInfo64.compressed_size = ze->CompressedSize;
		tunzip->CurrentEntryInfo64.uncompressed_size = ze->UncompressedSize;
	}

	// Initialize running counts
	tunzip->EntryReadVars.RemainingCompressed = tunzip->CurrentEntryInfo64.compressed_size;
	tunzip->EntryReadVars.RemainingUncompressed = tunzip->CurrentEntryInfo64.uncompressed_size;

	// Initialize for CRC checksum
	if (tunzip->CurrentEntryInfo.flag & 8) tunzip->EntryReadVars.CrcEncTest = (char)((tunzip->CurrentEntryInfo.dosDate >> 8) & 0xff);
	else tunzip->EntryReadVars.CrcEncTest = (char)(tunzip->CurrentEntryInfo.crc >> 24);

	if (tunzip->Flags & TZIP_GZIP)
		offset = tunzip->CurrentEntryInfo64.offset;
	else
	{
		{
			// Initialize encryption stuff
			register const unsigned char	*cp;

			// Entry is encrypted?
			if (tunzip->CurrentEntryInfo.flag & 1)
			{
				tunzip->EntryReadVars.RemainingEncrypt = 12;	// There is an additional 12 bytes at the beginning of each local header
				tunzip->EntryReadVars.Keys[0] = 305419896L;
				tunzip->EntryReadVars.Keys[1] = 591751049L;
				tunzip->EntryReadVars.Keys[2] = 878082192L;
				if ((cp = tunzip->Password))
				{
					while (*cp) Uupdate_keys(&tunzip->EntryReadVars.Keys[0], *(cp)++);
				}
			}
		}

		{
			unsigned short	extra_offset;

			// Seek to the entry's LOCALHEADER->extra_field_size
			if (seekInZip(tunzip, tunzip->CurrentEntryInfo64.offset + tunzip->ByteBeforeZipArchive + 28, FILE_BEGIN) ||
				!readFromZip(tunzip, &extra_offset, sizeof(unsigned short)))
			{
			badseek:	tunzip->LastErr = ZR_READ;
				goto badout;
			}

			// Get the offset to where the entry's compressed data starts within the archive
			//		tunzip->EntryReadVars.PosInArchive = (DWORD)tunzip->CurrentEntryInfo.offset + SIZEZIPLOCALHEADER + (DWORD)tunzip->CurrentEntryInfo.size_filename + (DWORD)extra_offset;
			offset = (DWORD)tunzip->CurrentEntryInfo64.offset + SIZEZIPLOCALHEADER + (DWORD)tunzip->CurrentEntryInfo.size_filename + (DWORD)extra_offset;
		}
	}

	// Seek so that a subsequent call to readEntry() will be at the correct position. (ie, We assume that
	// the DEFLATE routines will not be seeking around within the archive while the entry is decompressed)
	//	if (seekInZip(tunzip, tunzip->EntryReadVars.PosInArchive, FILE_BEGIN)) goto badseek;
	if (seekInZip(tunzip, offset, FILE_BEGIN)) goto badseek;
}





/************************* readEntry() **********************
* Reads bytes from the current position of the currently
* selected entry (within the archive), unencrypts them,
* and decompresses them into the passed buffer.
*
* buf =	Pointer to buffer where data must be copied.
* len =	The size of buf.
*
* RETURNS: The number of byte copied. Could be 0 if the
* end of file was reached. Sets TUNZIP->LastErr if an
* error.
*/

ULONGLONG readEntry(register TUNZIP *tunzip, void *buf, ULONGLONG len)
{
	int							err;
	ULONGLONG					iRead;

	iRead = 0;
	tunzip->EntryReadVars.stream.next_out = (UCH *)buf;

	// Caller is asking for more bytes than remaining? If so, just give him the
	// remaining bytes
	if (len > tunzip->EntryReadVars.RemainingUncompressed) len = tunzip->EntryReadVars.RemainingUncompressed;
	tunzip->EntryReadVars.stream.avail_out = len;

	// More room in the output buffer?
	while (tunzip->EntryReadVars.stream.avail_out)
	{
		// Input buffer is empty? Fill it
		if (!tunzip->EntryReadVars.stream.avail_in && tunzip->EntryReadVars.RemainingCompressed)
		{
			ULONGLONG	uReadThis;

			// Fill up the input buffer, or read as much as is available
			uReadThis = UNZ_BUFSIZE;
			if (uReadThis > tunzip->EntryReadVars.RemainingCompressed) uReadThis = tunzip->EntryReadVars.RemainingCompressed;

			// No more input (ie, done decompressing the entry)?
			if (!uReadThis)
			{
				// Check that checksum works out to what we expected
				if (tunzip->EntryReadVars.RunningCrc != tunzip->CurrentEntryInfo.crc)
					tunzip->LastErr = ZR_FLATE;
			none:			return(0);
			}

			// Seek to where we last left off in the ZIP archive and fill the input buffer
			if (!readFromZip(tunzip, tunzip->EntryReadVars.InputBuffer, (DWORD)uReadThis))
				//			if (seekInZip(tunzip, tunzip->EntryReadVars.PosInArchive + tunzip->ByteBeforeZipArchive, FILE_BEGIN) ||
				//				!readFromZip(tunzip, tunzip->EntryReadVars.InputBuffer, uReadThis))
			{
				tunzip->LastErr = ZR_READ;
				goto none;
			}

			// Increment current position within archive
			//			tunzip->EntryReadVars.PosInArchive += uReadThis;

			// Decrement remaining bytes to be decompressed
			tunzip->EntryReadVars.RemainingCompressed -= uReadThis;

			tunzip->EntryReadVars.stream.next_in = tunzip->EntryReadVars.InputBuffer;
			tunzip->EntryReadVars.stream.avail_in = uReadThis;

			// If encryption was applied, then decrypt the bytes
			if (tunzip->CurrentEntryInfo.flag & 1)
			{
				DWORD	i;

				for (i = 0; i < uReadThis; i++)
				{
					DWORD	temp;

					temp = ((DWORD)tunzip->EntryReadVars.Keys[2] & 0xffff) | 2;
					tunzip->EntryReadVars.InputBuffer[i] ^= (char)(((temp * (temp ^ 1)) >> 8) & 0xff);
					Uupdate_keys(&tunzip->EntryReadVars.Keys[0], tunzip->EntryReadVars.InputBuffer[i]);
				}
			}
		}

		// Read the encrpytion header that is at the start of the entry, if we haven't already done so
		{
			register ULONGLONG	uDoEncHead;

			uDoEncHead = tunzip->EntryReadVars.RemainingEncrypt;
			if (uDoEncHead > tunzip->EntryReadVars.stream.avail_in) uDoEncHead = tunzip->EntryReadVars.stream.avail_in;
			if (uDoEncHead)
			{
				char	bufcrc;

				bufcrc = tunzip->EntryReadVars.stream.next_in[uDoEncHead - 1];
				// tunzip->EntryReadVars.RemainingUncompressed -= uDoEncHead;
				tunzip->EntryReadVars.stream.avail_in -= uDoEncHead;
				tunzip->EntryReadVars.stream.next_in += uDoEncHead;
				tunzip->EntryReadVars.RemainingEncrypt -= uDoEncHead;

				// If we've finished the encryption header, then do the CRC check with the password
				if (!tunzip->EntryReadVars.RemainingEncrypt && bufcrc != tunzip->EntryReadVars.CrcEncTest)
				{
					tunzip->LastErr = ZR_PASSWORD;
					goto none;
				}
			}
		}

		// STORE?
		if (!tunzip->CurrentEntryInfo.compression_method)
		{
			ULONGLONG	uDoCopy;

			if (tunzip->EntryReadVars.stream.avail_out < tunzip->EntryReadVars.stream.avail_in)
				uDoCopy = tunzip->EntryReadVars.stream.avail_out;
			else
				uDoCopy = tunzip->EntryReadVars.stream.avail_in;
			CopyMemory(tunzip->EntryReadVars.stream.next_out, tunzip->EntryReadVars.stream.next_in, (DWORD)uDoCopy & 0xFFFFFFFF);

			tunzip->EntryReadVars.RunningCrc = ucrc32(tunzip->EntryReadVars.RunningCrc, tunzip->EntryReadVars.stream.next_out, (DWORD)uDoCopy & 0xFFFFFFFF);
			tunzip->EntryReadVars.RemainingUncompressed -= uDoCopy;
			tunzip->EntryReadVars.stream.avail_in -= uDoCopy;
			tunzip->EntryReadVars.stream.avail_out -= uDoCopy;
			tunzip->EntryReadVars.stream.next_out += uDoCopy;
			tunzip->EntryReadVars.stream.next_in += uDoCopy;
			tunzip->EntryReadVars.stream.total_out += uDoCopy;
			iRead += uDoCopy;

			if (!tunzip->EntryReadVars.RemainingUncompressed) break;
		}

		// DEFLATE
		else
		{
			ULONGLONG	uTotalOutBefore, uTotalOutAfter;
			const UCH	*bufBefore;
			ULONGLONG		uOutThis;

			uTotalOutBefore = tunzip->EntryReadVars.stream.total_out;
			bufBefore = tunzip->EntryReadVars.stream.next_out;

			if ((err = inflate(&tunzip->EntryReadVars.stream, Z_SYNC_FLUSH)) && err != Z_STREAM_END)
			{
				tunzip->LastErr = ZR_FLATE;
				goto none;
			}

			uTotalOutAfter = tunzip->EntryReadVars.stream.total_out;
			uOutThis = uTotalOutAfter - uTotalOutBefore;

			tunzip->EntryReadVars.RunningCrc = ucrc32(tunzip->EntryReadVars.RunningCrc, bufBefore, (DWORD)uOutThis);

			tunzip->EntryReadVars.RemainingUncompressed -= uOutThis;
			iRead += (uTotalOutAfter - uTotalOutBefore);

			if (err == Z_STREAM_END || !tunzip->EntryReadVars.RemainingUncompressed) break;
		}
	}

	return(iRead);
}






//  Get the global comment string of the ZipFile, in the szComment buffer.
//  uSizeBuf is the size of the szComment buffer.
//  return the number of byte copied or an error code <0
/*
int unzGetGlobalComment(TUNZIP * tunzip, char *szComment, ULG uSizeBuf)
{
DWORD		uReadThis;

uReadThis = uSizeBuf;
if (uReadThis > tunzip->CommentSize) uReadThis = tunzip->CommentSize;

if (seekInZip(tunzip, tunzip->CentralDirPos + 22, FILE_BEGIN)) return ZR_CORRUPT;

*szComment = 0;
if (readFromZip(tunzip, szComment, uReadThis) != uReadThis) return ZR_READ;

if (szComment && uSizeBuf > tunzip->CommentSize) *(szComment + tunzip->CommentSize) = 0;

return((int)uReadThis);
}
*/














































/************************ findEntry() ***********************
* Locates an entry (within the archive) by name.
*
* flags =	1 (case-insensitive) perhaps OR'ed with ZIP_UNICODE.
* index =	Where to return the index number of the located entry.
* ze =		Where to return a filled in ZIPENTRY.
*
* NOTE: ZIPENTRY->Name[] must contain the name to match. This
* will be overwritten.
*
* RETURNS: ZR_OK if success, ZR_NOTFOUND if not located, or some
* other error.
*/

static DWORD findEntry(TUNZIP *tunzip, ZIPENTRY *ze, DWORD flags)
{
	char			name[MAX_PATH];

	if (flags & UNZIP_UNICODE)
		WideCharToMultiByte(CP_UTF8, 0, (const WCHAR *)&ze->Name[0], -1, &name[0], MAX_PATH, 0, 0);
	else
		lstrcpyA(&name[0], (const char *)&ze->Name[0]);
	if (lstrlenA(&name[0]) >= UNZ_MAXFILENAMEINZIP) return(ZR_ARGS);

	{
		register char		*d;

		// Next we need to replace '\' with '/' chars
		d = name;
		while (*d)
		{
			if (*d == '\\') *d = '/';
			++d;
		}
	}

	// If there's a currently selected entry, free it
	cleanupEntry(tunzip);

	// No error yet
	tunzip->LastErr = 0;

	// Start with first entry and read its table from the Central Directory
	goToFirstEntry(tunzip);
	while (!tunzip->LastErr)
	{
		// Get this entry's filename
		getEntryFN(tunzip, (char *)&ze->Name[0]);
		if (!tunzip->LastErr)
		{
			// Do names match?
			if (!(flags & 0x01 ? lstrcmpiA(&name[0], (const char *)&ze->Name[0]) : lstrcmpA(&name[0], (const char *)&ze->Name[0])))
			{
				// Fill in caller's ZIPENTRY
				ze->Index = tunzip->CurrentEntryNum;
				return(setCurrentEntry(tunzip, ze, (flags & UNZIP_UNICODE) | UNZIP_ALREADYINIT));
			}

			goToNextEntry(tunzip);
			/*
			// Another entry?
			if (tunzip->CurrentEntryNum + 1 < tunzip->TotalEntries)
			{
			// Skip the remainder of the current entry's table
			skipToEntryEnd(tunzip, 0);
			if (!tunzip->LastErr)
			{
			// Read the info for the next entry
			tunzip->CurrEntryPosInCentralDir += SIZECENTRALDIRITEM + tunzip->CurrentEntryInfo.size_filename + tunzip->CurrentEntryInfo.size_file_extra + tunzip->CurrentEntryInfo.size_file_comment;
			++tunzip->CurrentEntryNum;
			getEntryInfo(tunzip);
			}
			}
			else
			tunzip->LastErr = ZR_NOTFOUND;
			*/
		}
	}

	cleanupEntry(tunzip);
	return(tunzip->LastErr);
}






/********************** setCurrentEntry() *********************
* Sets the specified entry to the currently selected
* entry within the ZIP archive, and fills in the
* passed ZIPENTRY with information about that entry.
*
* tunzip =		Handle returned by UnzipOpen*() functions.
* ze =			Pointer to a ZIPENTRY to fill in.
* flags =		One of the following:
*				ZIP_ALREADYINIT - ZIPENTRY already filled
*				in by a call to findEntry().
*				ZIP_UNICODE - Caller's ZIPENTRY uses UNICODE name.
*
* RETURNS: Z_OK if success, otherwise an error code.
*
* NOTE: ZIPENTRY->Index must be set to the desired
* entry number before calling setCurrentEntry(). The value -1
* means to return how many entries are in the ZIP archive.
*/

static DWORD setCurrentEntry(register TUNZIP *tunzip, ZIPENTRY *ze, DWORD flags)
{
	unsigned char	*extra;

	// No error yet
	tunzip->LastErr = 0;

	// Did findEntry() already init the ZIPENTRY?
	if (!(flags & UNZIP_ALREADYINIT))
	{
		// Does caller want general information about the ZIP archive?
		if (ze->Index == (ULONGLONG)-1)
		{
			ze->Index = tunzip->TotalEntries;
			goto good;
		}

		// If there is currently a selected entry, free its resources
		cleanupEntry(tunzip);

		// Seek to the point in the ZIP archive where this entry is found
		// and fill in the TUNZIP->CurrentEntryNum
		if (ze->Index < tunzip->CurrentEntryNum) goToFirstEntry(tunzip);
		while (!tunzip->LastErr && tunzip->CurrentEntryNum < ze->Index) goToNextEntry(tunzip);

		if (tunzip->LastErr)
		{
		reterr:		cleanupEntry(tunzip);
			return(tunzip->LastErr);
		}

		// Get the entry's filename
		getEntryFN(tunzip, (char *)&ze->Name[0]);
	}

	// We support only STORE and DEFLATE compression methods
	if (tunzip->CurrentEntryInfo.compression_method && tunzip->CurrentEntryInfo.compression_method != Z_DEFLATED)
		tunzip->LastErr = ZR_NOTSUPPORTED;
	if (tunzip->LastErr) goto reterr;

	// If raw mode, app must supply the compressed and uncompressed sizes
	if (tunzip->Flags & TZIP_RAW) goto good2;

	// Get the extra header (which may contain some extra timestamp stuff)
	{
		DWORD	size;

		extra = 0;
		if ((size = tunzip->CurrentEntryInfo.size_file_extra))
		{
			if (!(extra = (unsigned char*)GlobalAlloc(GMEM_FIXED, size)))
			{
				tunzip->LastErr = ZR_NOALLOC;
				goto reterr;
			}

			if (readFromZip(tunzip, extra, size) != size)
			{
				GlobalFree(extra);
				tunzip->LastErr = ZR_CORRUPT;
				goto reterr;
			}
		}
	}

	// Copy the entry's name to ZIPENTRY->name[] (UNICODE or ANSI)
	{
		register char	*sfn;
		register char	*dfn;
		char			fn[MAX_PATH];
		unsigned char	previous;

		// As a safety feature: if the zip filename had sneaky stuff like "c:\windows\file.txt" or
		// "\windows\file.txt" or "fred\..\..\..\windows\file.txt" then we get rid of them all. That
		// way, when the application does UnzipItem(), it won't be a problem. (If the
		// programmer really does want to get the full evil information, then he can edit out this
		// security feature below). In particular, we chop off any prefixes that are "c:\" or
		// "\" or "/" or "[stuff]\.." or "[stuff]/.."

		// Copy the root dir name
		lstrcpyA(&fn[0], (const char *)&tunzip->Rootdir[0]);

		// Prepare to append entry's name
		sfn = &fn[0] + lstrlenA(&fn[0]);
		dfn = (char *)&ze->Name[0];

		// Skip the drive
		if (dfn[0] && dfn[1] == ':') dfn += 2;

		previous = DIRSLASH_CHAR;			// Skip leading slashes
		while (*dfn)
		{
			// Skip any "\..\" or "/../"
			if (dfn[0] == '\\' || dfn[0] == '/')
			{
				dfn[0] = DIRSLASH_CHAR;		// Change all '/' to '\' for Windows
				if (dfn[1] == '.' && dfn[2] == '.' && (dfn[3] == '\\' || dfn[3] == '/'))
				{
					dfn += 4;
					lstrcpyA(&fn[0], (const char *)&tunzip->Rootdir[0]);
					sfn = &fn[0] + lstrlenA(&fn[0]);
					previous = DIRSLASH_CHAR;
					continue;
				}

				if (previous == DIRSLASH_CHAR)
				{
					previous = 0;
					continue;
				}
			}

			*(sfn)++ = previous = *(dfn)++;
		}
		*sfn = 0;

		if (flags & UNZIP_UNICODE)
		{
			MultiByteToWideChar(CP_UTF8, 0, &fn[0], -1, (WCHAR *)&ze->Name[0], MAX_PATH);
		}
		else
			lstrcpyA((char *)&ze->Name[0], &fn[0]);

		// Copy the attributes. zip has an 'attribute' 32bit value. Its lower half
		// is windows stuff. Its upper half is standard unix stat.st_mode. We use the
		// UNIX half, but in normal hostmodes these are overridden by the lower half
		{
			unsigned long host;

			host = tunzip->CurrentEntryInfo.version >> 8;
			if (!host || host == 6 || host == 10 || host == 14)
				ze->Attributes = tunzip->CurrentEntryInfo.external_fa & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE);
			else
			{
				ze->Attributes = FILE_ATTRIBUTE_ARCHIVE;
				if (tunzip->CurrentEntryInfo.external_fa & 0x40000000) ze->Attributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_DIRECTORY;
				if (!(tunzip->CurrentEntryInfo.external_fa & 0x00800000)) ze->Attributes |= FILE_ATTRIBUTE_READONLY;
			}
		}

		// Copy sizes
		ze->CompressedSize = tunzip->CurrentEntryInfo.compressed_size;
		ze->UncompressedSize = tunzip->CurrentEntryInfo.uncompressed_size;
		ze->offset = tunzip->CurrentEntryInfo.offset;

		// Copy timestamp
		{
			FILETIME ftd;
			FILETIME ft;

			dosdatetime2filetime(&ftd, (tunzip->CurrentEntryInfo.dosDate >> 16) & 0xFFFF, tunzip->CurrentEntryInfo.dosDate & 0xFFFF);
			LocalFileTimeToFileTime(&ftd, &ft);
			ze->AccessTime = ft;
			ze->CreateTime = ft;
			ze->ModifyTime = ft;
		}

		{
			// the zip will always have at least that dostime. But if it also has
			// an extra header, then we'll instead get the info from that
			DWORD	epos;

			epos = 0;
			while (epos + 4 < tunzip->CurrentEntryInfo.size_file_extra)
			{
				char	etype[3];
				DWORD	flags;
				int		size;

				etype[0] = extra[epos + 0];
				etype[1] = extra[epos + 1];
				etype[2] = 0;
				size = extra[epos + 2];
				if (!lstrcmpA(etype, "UT"))
				{
					flags = extra[epos + 4];
					epos += 5;
					if ((flags & 1) && epos < tunzip->CurrentEntryInfo.size_file_extra)
					{
						lutime_t modifyTime = ((extra[epos + 0]) << 0) | ((extra[epos + 1]) << 8) | ((extra[epos + 2]) << 16) | ((extra[epos + 3]) << 24);
						timet2filetime(&ze->ModifyTime, modifyTime);
						epos += 4;
					}

					if ((flags & 2) && epos < tunzip->CurrentEntryInfo.size_file_extra)
					{
						lutime_t accessTime = ((extra[epos + 0]) << 0) | ((extra[epos + 1]) << 8) | ((extra[epos + 2]) << 16) | ((extra[epos + 3]) << 24);
						timet2filetime(&ze->AccessTime, accessTime);
						epos += 4;
					}

					if ((flags & 4) && epos < tunzip->CurrentEntryInfo.size_file_extra)
					{
						lutime_t createTime = ((extra[epos + 0]) << 0) | ((extra[epos + 1]) << 8) | ((extra[epos + 2]) << 16) | ((extra[epos + 3]) << 24);
						timet2filetime(&ze->CreateTime, createTime);
						epos += 4;
					}

					break;
				}
				else if (*(SHORT*)etype == 1 && size)
				{
					DWORD apos = epos + 4;
					if (tunzip->CurrentEntryInfo.compressed_size == -1)
					{
						ze->CompressedSize = *(ULONGLONG*)&extra[apos];
						apos += 8;
					}
					if (tunzip->CurrentEntryInfo.uncompressed_size == -1)
					{
						ze->UncompressedSize = *(ULONGLONG*)&extra[apos];
						apos += 8;
					}
					if (tunzip->CurrentEntryInfo.offset == -1)
						ze->offset = *(ULONGLONG*)&extra[apos];
				}
				
				epos += 4 + size;
			}
		}

		if (extra) GlobalFree(extra);
	}
good2:
	// We clear the internal_fa field as a signal to unzipEntry() that a memory-buffer unzip is
	// starting for the first time
	tunzip->CurrentEntryInfo.internal_fa = 0;
	tunzip->CurrentEntryInfo64.compressed_size = ze->CompressedSize;
	tunzip->CurrentEntryInfo64.uncompressed_size = ze->UncompressedSize;
	tunzip->CurrentEntryInfo64.offset = ze->offset;
good:
	return(ZR_OK);
}









/************************ str_chrA **********************
* Searches for the first occurence of 'chr' in
* nul-terminated 'str'. Same as C library's strchr().
*
* str =	pointer to string to search
* chr =	character to search for
*
* RETURNS:	pointer to where 'chr' is found in 'str', or 0
*			if not found.
*/

static char * str_chrA(char *str, char chr)
{
	register char	tempch;

	if ((tempch = *str))
	{
		do
		{
			if (tempch == chr) return(str);
		} while ((tempch = *(++str)));
	}

	// Not found
	return(0);
}





/********************* createMultDirsA() *******************
* Creates as many dirs as are specified in the
* nul-terminated string pointed to by dirname.
*
* dirname =	The names of directories to create, each
*				separated by a backslash (but no backslash
*				at the head or tail of the string).
*
* RETURNS: 1 if success, 0 if error.
*/

unsigned long createMultDirsA(char *dirname, BOOL isDir)
{
	register char *		ptr;
	register char *		pathbuf;
	SECURITY_ATTRIBUTES	sc;

	sc.nLength = sizeof(SECURITY_ATTRIBUTES);
	sc.lpSecurityDescriptor = 0;
	sc.bInheritHandle = TRUE;

	pathbuf = dirname;

	// Skip drive
	if (dirname[0] && dirname[1] == ':') dirname += 2;
	if (dirname[0] == DIRSLASH_CHAR) ++dirname;

	// Another sub-dir to create?
	while (*dirname)
	{
		// Isolate next sub-dir name
		if (!(ptr = str_chrA(dirname, DIRSLASH_CHAR)))
		{
			if (!isDir) break;
		}
		else
		{
			// Nul-term path
			*ptr = 0;
		}

		if (!CreateDirectoryA(pathbuf, &sc) && GetLastError() != ERROR_ALREADY_EXISTS)
		{
			if (ptr) *ptr = DIRSLASH_CHAR;
			return(0);
		}

		if (!ptr) break;

		// Restore overwritten char
		*ptr = DIRSLASH_CHAR;
		dirname = ++ptr;
	}

	return(1);
}





/************************ str_chrW **********************
* Wide character version of str_chrA().
*/

static WCHAR * str_chrW(WCHAR *str, WCHAR chr)
{
	register WCHAR	ch;

	if ((ch = *str))
	{
		do
		{
			if (ch == chr) return(str);
		} while ((ch = *(++str)));
	}

	// Not found
	return(0);
}

/********************* createMultDirsW *******************
* Wide character version of createMultDirsA().
*/

unsigned long createMultDirsW(WCHAR *dirname, BOOL isDir)
{
	register WCHAR *		ptr;
	register WCHAR *		pathbuf;
	SECURITY_ATTRIBUTES	sc;

	sc.nLength = sizeof(SECURITY_ATTRIBUTES);
	sc.lpSecurityDescriptor = 0;
	sc.bInheritHandle = TRUE;

	pathbuf = dirname;

	// Skip drive
	if (dirname[0] && dirname[1] == ':') dirname += 2;
	if (dirname[0] == DIRSLASH_CHAR) ++dirname;

	// Another sub-dir to create?
	while (*dirname)
	{
		// Isolate next sub-dir name
		if (!(ptr = str_chrW(dirname, DIRSLASH_CHAR)))
		{
			if (!isDir) break;
		}
		else
		{
			// Nul-term path
			*ptr = 0;
		}

		if (!CreateDirectoryW(pathbuf, &sc) && GetLastError() != ERROR_ALREADY_EXISTS)
		{
			if (ptr) *ptr = DIRSLASH_CHAR;
			return(0);
		}

		if (!ptr) break;

		// Restore overwritten char
		*ptr = DIRSLASH_CHAR;
		dirname = ++ptr;
	}

	return(1);
}

/********************* unzipEntry() *******************
* Unzips the specified entry in the specified ZIP
* archive. Can be unzipped to a pipe/handle, disk file,
* or memory buffer.
*
* dst =	Handle to file where the entry is decompressed,
*			or filename, or memory buffer pointer.
* ze =		Filled in ZIPENTRY struct.
* flags =	ZIP_MEMORY, ZIP_FILENAME, or ZIP_HANDLE. Also
*			may be ZIP_UNICODE.
*/

static DWORD unzipEntry(register TUNZIP *tunzip, void *dst, ZIPENTRY *ze, DWORD flags)
{
	HANDLE		h;

	// Make sure TUNZIP if valid
	if (IsBadReadPtr(tunzip, 1) ||
		// Make sure we have a currently selected entry
		tunzip->CurrentEntryNum == (ULONGLONG)-1)
	{
		return(ZR_ARGS);
	}

	// No error
	tunzip->LastErr = 0;

	// ============ Unzipping to memory ============
	if (flags & UNZIP_MEMORY)
	{
		// Don't reinitialize if this is called as a result of ZR_MORE
		if (!tunzip->CurrentEntryInfo.internal_fa)
		{
			initEntry(tunzip, ze);
			if (tunzip->LastErr) goto out;
			tunzip->CurrentEntryInfo.internal_fa = 1;
		}

		ze->CompressedSize = readEntry(tunzip, dst, ze->CompressedSize);
		if (tunzip->LastErr || !tunzip->EntryReadVars.RemainingUncompressed)
		{
			tunzip->CurrentEntryInfo.internal_fa = 0;
			goto out;
		}

		// Filled the output buffer. Return ZR_MORE so the caller can flush it somewhere,
		// but don't close the entry, and leave internal_fa set to 1
		return(ZR_MORE);
	}

	// ============ Unzipping to disk/pipe ============

	// Is this entry a directory name?
	if (ze->Attributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		// NOTE: We can't create a directory when spooling to a pipe
		if (flags & UNZIP_FILENAME)
		{
			if (flags & UNZIP_UNICODE) flags = createMultDirsW((WCHAR *)dst, 1);
			else  flags = createMultDirsA((char *)dst, 1);
			if (!flags) goto badf;
		}
		return(ZR_OK);
	}

	// Write the entry to a file/handle
	if (flags == UNZIP_HANDLE)
		h = (HANDLE)dst;
	else
	{
		DWORD		res;

		// Create any needed directories
		if (flags & UNZIP_UNICODE) res = createMultDirsW((WCHAR *)dst, 0);
		else res = createMultDirsA((char *)dst, 0);
		if (!res) goto badf;

		// Create the file to which we'll write the uncompressed entry
		if (flags & UNZIP_UNICODE)
			h = CreateFileW((WCHAR *)dst, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, ze->Attributes, 0);
		else
			h = CreateFileA((char *)dst, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, ze->Attributes, 0);
	}
	if (h == INVALID_HANDLE_VALUE)
	badf:	tunzip->LastErr = ZR_NOFILE;
	else
	{
		// Allocate resources for decompressing
		initEntry(tunzip, ze);
		if (!tunzip->LastErr)
		{
			// Get an output buffer (where we decompress bytes)
			if (!tunzip->OutBuffer && !(tunzip->OutBuffer = (unsigned char *)GlobalAlloc(GMEM_FIXED, 16384))) tunzip->LastErr = ZR_NOALLOC;

			while (!tunzip->LastErr)
			{
				DWORD				writ;
				register DWORD		read;

				// Decompress the bytes into the input buffer. If EOF, then get out of this loop
				if (!(read = (DWORD)readEntry(tunzip, tunzip->OutBuffer, 16384))) break;

				// Write the bytes
				if (!WriteFile(h, tunzip->OutBuffer, read, &writ, 0) || writ != read)
					tunzip->LastErr = ZR_WRITE;
			}
		}

		// Set the file's timestamp
		if (!tunzip->LastErr)
			SetFileTime(h, &ze->CreateTime, &ze->AccessTime, &ze->ModifyTime); // may fail if it was a pipe

		// Close the file if we opened it above	
		if (flags != UNZIP_HANDLE)
			CloseHandle(h);
	}
out:
	cleanupEntry(tunzip);
	return(tunzip->LastErr);
}

DWORD WINAPI UnzipItemToHandle(HUNZIP tunzip, HANDLE h, ZIPENTRY *ze)
{
	return(unzipEntry((TUNZIP*)tunzip, (void *)h, ze, UNZIP_HANDLE));
}

DWORD WINAPI UnzipItemToFileA(HUNZIP tunzip, const char *fn, ZIPENTRY *ze)
{
	return(unzipEntry((TUNZIP*)tunzip, (void *)fn, ze, UNZIP_FILENAME));
}

DWORD WINAPI UnzipItemToFileW(HUNZIP tunzip, const WCHAR *fn, ZIPENTRY *ze)
{
	return(unzipEntry((TUNZIP*)tunzip, (void *)fn, ze, UNZIP_FILENAME | UNZIP_UNICODE));
}

DWORD WINAPI UnzipItemToBuffer(HUNZIP tunzip, void *z, DWORD len, ZIPENTRY *ze)
{
	ze->CompressedSize = len;
	return(unzipEntry((TUNZIP*)tunzip, z, ze, UNZIP_MEMORY));
}









DWORD WINAPI UnzipFormatMessageA(DWORD code, char *buf, DWORD len)
{
	return ZipFormatMessageA(code, buf, len);
}

DWORD WINAPI UnzipFormatMessageW(DWORD code, WCHAR *buf, DWORD len)
{
	return ZipFormatMessageW(code, buf, len);
}











/********************* closeArchive() *****************
* Closes the ZIP archive opened with openArchive().
*/

static void closeArchive(register TUNZIP *tunzip)
{
	cleanupEntry(tunzip);
	if (tunzip->Flags & TZIP_ARCCLOSEFH)
		CloseHandle(tunzip->ArchivePtr);
	if (tunzip->Password){
		g_memset(tunzip->Password, 0, GlobalSize(tunzip->Password));
		GlobalFree(tunzip->Password);
	}
	if (tunzip->OutBuffer){
		g_memset(tunzip->OutBuffer, 0, GlobalSize(tunzip->OutBuffer));
		GlobalFree(tunzip->OutBuffer);
	}
	g_memset(tunzip, 0, GlobalSize(tunzip));
	GlobalFree(tunzip);
}

/********************* UnzipClose() **********************
* Closes the ZIP file that was created/opened with one
* of the UnzipOpen* functions.
*/

DWORD WINAPI UnzipClose(HUNZIP tunzip)
{
	register DWORD	result;

	// Make sure TUNZIP if valid
	if (IsBadReadPtr(tunzip, 1))
		result = ZR_ARGS;
	else
	{
		result = ZR_OK;
		closeArchive((TUNZIP *)tunzip);
	}
	return(result);
}







DWORD WINAPI UnzipGetItemA(HUNZIP tunzip, ZIPENTRY *ze)
{
	if (IsBadReadPtr(tunzip, 1))
		return(ZR_ARGS);
	return(setCurrentEntry((TUNZIP *)tunzip, ze, 0));
}

DWORD WINAPI UnzipGetItemW(HUNZIP tunzip, ZIPENTRY *ze)
{
	if (IsBadReadPtr(tunzip, 1))
		return(ZR_ARGS);
	return(setCurrentEntry((TUNZIP *)tunzip, ze, UNZIP_UNICODE));
}

DWORD WINAPI UnzipFindItemA(HUNZIP tunzip, ZIPENTRY *ze, BOOL ic)
{
	if (IsBadReadPtr(tunzip, 1))
		return(ZR_ARGS);
	return(findEntry((TUNZIP*)tunzip, ze, (DWORD)ic));
}

DWORD WINAPI UnzipFindItemW(HUNZIP tunzip, ZIPENTRY *ze, BOOL ic)
{
	if (IsBadReadPtr(tunzip, 1))
		return(ZR_ARGS);
	return(findEntry((TUNZIP*)tunzip, ze, (DWORD)ic | UNZIP_UNICODE));
}





/******************* openArchive() ********************
* Opens a ZIP archive, in preparation of unzipping
* its entries.
*
* z =	A handle, pointer to filename, or pointer to a
*		memory buffer where the ZIP archive resides.
* len = If a memory buffer, its size.
* flags = ZIP_HANDLE, ZIP_FILENAME, or ZIP_MEMORY. Also,
*		can be OR'd with ZIP_UNICODE.
*
* RETURNS: Z_OK if success, otherwise an error number.
*/

#define BUFREADCOMMENT (0x400)

static DWORD openArchive(HANDLE *ptr, void *z, DWORD len, DWORD flags, const char *pwd)
{
	register TUNZIP		*tunzip;
	ULONGLONG			centralDirPos;
	bool				iszip64 = false;

	// Get a TUNZIP
	if (!(tunzip = (TUNZIP *)GlobalAlloc(GMEM_FIXED, sizeof(TUNZIP))))
	{
	badalloc:
		flags = ZR_NOALLOC;
	bad:	return(flags);
	}
	ZeroMemory(tunzip, sizeof(TUNZIP) - MAX_PATH);

	// Store password if supplied
	if (pwd)
	{
		if (!(tunzip->Password = (unsigned char *)GlobalAlloc(GMEM_FIXED, lstrlenA(pwd) + 1)))
		{
			GlobalFree(tunzip);
			goto badalloc;
		}
		lstrcpyA((char *)tunzip->Password, pwd);
	}

	// No currently selected entry
	tunzip->CurrentEntryNum = (ULONGLONG)-1;

	switch (flags & ~(UNZIP_UNICODE | UNZIP_ALREADYINIT | UNZIP_RAW))
	{
		// ZIP archive is from a pipe or already open handle
	case UNZIP_HANDLE:
	{
		if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)z, GetCurrentProcess(), &tunzip->ArchivePtr, 0, 0, DUPLICATE_SAME_ACCESS)) goto mustclose;
		tunzip->ArchivePtr = (HANDLE)z;
		goto chk_handle;
	}

	// ZIP archive is a disk file
	case UNZIP_FILENAME:
	{
		if (flags & UNZIP_UNICODE)
			tunzip->ArchivePtr = CreateFileW((const WCHAR *)z, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		else
			tunzip->ArchivePtr = CreateFileA((const char *)z, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (tunzip->ArchivePtr == INVALID_HANDLE_VALUE)
		{
			flags = ZR_NOFILE;
		bad2:			closeArchive(tunzip);
			goto bad;
		}

	mustclose:
		tunzip->Flags = TZIP_ARCCLOSEFH;

	chk_handle:	// Test if we can seek on it
		if ((tunzip->InitialArchiveOffset = SetFilePointer(tunzip->ArchivePtr, 0, 0, FILE_CURRENT)) == (DWORD)-1)
		{
			// Our unzip code requires that we be able to seek on the ZIP archive
			flags = ZR_SEEK;
			goto bad2;
		}

		break;
	}

	// A memory buffer
	case UNZIP_MEMORY:
	{
		// If buffer pointer is supplied, then he passed the ZIP archive
		if ((tunzip->ArchivePtr = z))
			tunzip->ArchiveBufLen = len;

		// Otherwise, he wants the ZIP archive extracted from the EXE's resource, and "len" is the ID number
		else
		{
			HRSRC		hrsrc;
			HANDLE		hglob;

			z = GetModuleHandle(0);

			// Load the ZIP archive, which is in our EXE's resources. It is an RT_RCDATA type
			// of resource with an ID number of 'len'
			if (!(hrsrc = FindResource((HMODULE)z, MAKEINTRESOURCE(len), RT_RCDATA)) || !(hglob = LoadResource((HMODULE)z, hrsrc)))
			{
				flags = ZR_NOTFOUND;
				goto bad2;
			}

			// Get the size and ptr to the ZIP archive in memory
			if (!(tunzip->ArchivePtr = LockResource(hglob))) goto memerr;
			tunzip->ArchiveBufLen = SizeofResource((HMODULE)z, hrsrc);
		}

		tunzip->Flags = TZIP_ARCMEMORY;
		break;
	}

	//		default:
	//		{
	//			flags = ZR_ARGS;
	//			goto bad2;
	//		}
	}

	// If RAW flag, then it's assumed there is only one file in the archive, no central directory,
	// and not even any GZIP indentification ID3 tag compression seems to be this way. Sigh... Wish
	// some programmers put a little more thought into their dodgy file formats _before_ releasing
	// code
	if (flags & UNZIP_RAW)
	{
		ZeroMemory(&tunzip->CurrentEntryInfo, sizeof(ZIPENTRYINFO));
		tunzip->CurrentEntryInfo.compression_method = Z_DEFLATED;
		tunzip->CurrentEntryInfo.offset = tunzip->InitialArchiveOffset;
		tunzip->CurrentEntryInfo64.offset = tunzip->InitialArchiveOffset;
		tunzip->Flags |= TZIP_RAW;
		tunzip->CurrentEntryNum = 0;
		goto raw;
	}

	{
		// Find the central directory's offset within the archive
		LONGLONG	uMaxBack;
		LONGLONG	uSizeFile;
		LONGLONG	uBackRead;
		unsigned char	*buf;

		if (!(tunzip->Flags & TZIP_ARCMEMORY))
		{
			LARGE_INTEGER aSizeFile;
			aSizeFile.QuadPart = 0;
			SetFilePointerEx(tunzip->ArchivePtr, aSizeFile, &aSizeFile, FILE_END);
			uSizeFile = aSizeFile.QuadPart - tunzip->InitialArchiveOffset; // SetFilePointer(tunzip->ArchivePtr, 0, 0, FILE_END) - tunzip->InitialArchiveOffset;
		}
		else
			uSizeFile = tunzip->ArchiveBufPos = tunzip->ArchiveBufLen;

		uMaxBack = 0xffff; // maximum size of global comment
		if (uMaxBack > uSizeFile) uMaxBack = uSizeFile;

		if ((buf = (unsigned char *)GlobalAlloc(GMEM_FIXED, BUFREADCOMMENT + 4)))
		{
			uBackRead = 4;
			while (uBackRead < uMaxBack)
			{
				DWORD		uReadSize;
				int			i;

				if (uBackRead + BUFREADCOMMENT > uMaxBack) uBackRead = uMaxBack;
				else uBackRead += BUFREADCOMMENT;
				centralDirPos = uSizeFile - uBackRead;
				uReadSize = (DWORD)(((BUFREADCOMMENT + 4) < (uSizeFile - centralDirPos)) ? (BUFREADCOMMENT + 4) : (uSizeFile - centralDirPos));
				if (seekInZip(tunzip, centralDirPos, FILE_BEGIN) || !readFromZip(tunzip, buf, uReadSize)) break;
				for (i = (int)uReadSize - 3; (i--) >= 0;)
				{
					if (*(buf + i) == 0x50 && *(buf + i + 1) == 0x4b && *(buf + i + 2) == 0x05 && *(buf + i + 3) == 0x06)
					{
						if (*(buf + i - 20) == 0x50 && *(buf + i + 1 - 20) == 0x4b && *(buf + i + 2 - 20) == 0x06 && *(buf + i + 3 - 20) == 0x07)
						{
							iszip64 = true;
							centralDirPos = *(ULONGLONG*)(buf + i - 12);
						}
						else
						{
							centralDirPos += i;
						}
						GlobalFree(buf);
						seekInZip(tunzip, centralDirPos, FILE_BEGIN);
						goto gotdir;
					}
				}
			}

			GlobalFree(buf);
		}

		// Memory error?
		if (!buf)
		{
		memerr:
			flags = ZR_NOALLOC;
			goto bad2;
		}
	}

	{
		unsigned int	number_disk;			// # of the current dist, used for spanning ZIP, unsupported here
		unsigned int	number_disk_with_CD;	// # of the disk with central dir, used for spanning ZIP, unsupported here
		ULONGLONG		totalEntries_CD;		// total number of entries in the central dir (same as TotalEntries on nospan)
		ULONGLONG		centralDirSize;

		// Assume GZIP format (ie, no central dir because there is only 1 file)
		seekInZip(tunzip, 0, FILE_BEGIN);
		flags = getArchiveShort(tunzip);
		if (flags != 0x00008b1f)
		{
		badzip:	flags = ZR_CORRUPT;
			goto bad2;
		}
		tunzip->CentralDirOffset = 3;
	raw:
		tunzip->ByteBeforeZipArchive = tunzip->InitialArchiveOffset;
		tunzip->LastErr = 0;
		tunzip->Flags |= TZIP_GZIP;
		tunzip->TotalEntries = 1;
		goto gotgzip;

	gotdir:
		// Get/skip the signature, already checked above
		getArchiveLong(tunzip);
		ULONGLONG size_of_zip64 = 0;
		if (iszip64)
		{
			size_of_zip64 = getArchiveLongLong(tunzip);
			getArchiveLong(tunzip);
		}
		// Get the number of this disk. Used for spanning ZIP, unsupported here
		if (iszip64)
			number_disk = getArchiveLong(tunzip);
		else
			number_disk = getArchiveShort(tunzip);

		// Get number of the disk with the start of the central directory
		if (iszip64)
			number_disk_with_CD = getArchiveLong(tunzip);
		else
			number_disk_with_CD = getArchiveShort(tunzip);

		// Get total number of entries in the central dir on this disk
		if (iszip64)
			tunzip->TotalEntries = getArchiveLongLong(tunzip);
		else
			tunzip->TotalEntries = getArchiveShort(tunzip);

		// Get total number of entries in the central dir
		if (iszip64)
			totalEntries_CD = getArchiveLongLong(tunzip);
		else
			totalEntries_CD = getArchiveShort(tunzip);

		if (tunzip->LastErr) goto badzip;

		// We don't support disk-spanning here
		if (totalEntries_CD != tunzip->TotalEntries || number_disk_with_CD || number_disk)
		{
			flags = ZR_NOTSUPPORTED;
			goto bad2;
		}

		// Size of the central directory
		if (iszip64)
			centralDirSize = getArchiveLongLong(tunzip);
		else
			centralDirSize = getArchiveLong(tunzip);

		// Offset of start of central directory with respect to the starting disk number
		if (iszip64)
			tunzip->CentralDirOffset = getArchiveLongLong(tunzip);
		else
			tunzip->CentralDirOffset = getArchiveLong(tunzip);
		// zipfile comment length
		if (iszip64)
			seekInZip(tunzip, tunzip->InitialArchiveOffset + tunzip->CentralDirOffset + centralDirSize + size_of_zip64 + 12 + 20 + 20, FILE_BEGIN);
		tunzip->CommentSize = getArchiveShort(tunzip);

		if (tunzip->LastErr || centralDirPos + tunzip->InitialArchiveOffset < tunzip->CentralDirOffset + centralDirSize) goto badzip;
		tunzip->ByteBeforeZipArchive = (DWORD)(centralDirPos + tunzip->InitialArchiveOffset - (tunzip->CentralDirOffset + centralDirSize));
		//	tunzip->CentralDirPos = centralDirPos;
	gotgzip:
		// Set Rootdir to current directory. (Assume we unzip there)
		if (!(centralDirPos = GetCurrentDirectoryA(MAX_PATH - 1, (LPSTR)&tunzip->Rootdir[0])) ||
			tunzip->Rootdir[centralDirPos - 1] != '\\')
		{
			tunzip->Rootdir[centralDirPos++] = DIRSLASH_CHAR;
			tunzip->Rootdir[centralDirPos] = 0;
		}

		// Return the TUNZIP
		*ptr = tunzip;

		return(ZR_OK);
	}
}

/******************** UnzipOpen*() ********************
* Opens a ZIP archive, in preparation of decompressing
* entries from it. The archive can be on disk, or in
* memory, or from a pipe or an already open file.
*/

DWORD WINAPI UnzipOpenHandle(HUNZIP *tunzip, HANDLE h, const char *password)
{
	return(openArchive(tunzip, h, 0, UNZIP_HANDLE, password));
}

DWORD WINAPI UnzipOpenHandleRaw(HUNZIP *tunzip, HANDLE h, const char *password)
{
	return(openArchive(tunzip, h, 0, UNZIP_HANDLE | UNZIP_RAW, password));
}

DWORD WINAPI UnzipOpenFileA(HUNZIP *tunzip, const char *fn, const char *password)
{
	return(openArchive(tunzip, (void *)fn, 0, UNZIP_FILENAME, password));
}

DWORD WINAPI UnzipOpenFileW(HUNZIP *tunzip, const WCHAR *fn, const char *password)
{
	return(openArchive(tunzip, (void *)fn, 0, UNZIP_FILENAME | UNZIP_UNICODE, password));
}

DWORD WINAPI UnzipOpenFileRawA(HUNZIP *tunzip, const char *fn, const char *password)
{
	return(openArchive(tunzip, (void *)fn, 0, UNZIP_FILENAME | UNZIP_RAW, password));
}

DWORD WINAPI UnzipOpenFileRawW(HUNZIP *tunzip, const WCHAR *fn, const char *password)
{
	return(openArchive(tunzip, (void *)fn, 0, UNZIP_FILENAME | UNZIP_UNICODE | UNZIP_RAW, password));
}

DWORD WINAPI UnzipOpenBuffer(HUNZIP *tunzip, void *z, DWORD len, const char *password)
{
	return(openArchive(tunzip, z, len, UNZIP_MEMORY, password));
}

DWORD WINAPI UnzipOpenBufferRaw(HUNZIP *tunzip, void *z, DWORD len, const char *password)
{
	return(openArchive(tunzip, z, len, UNZIP_MEMORY | UNZIP_RAW, password));
}






































/******************** unzipSetBaseDir() ******************
* Sets the root directory.
*/

static DWORD unzipSetBaseDir(TUNZIP *tunzip, const void *dir, DWORD isUnicode)
{
	// Make sure TUNZIP if valid
	if (IsBadReadPtr(tunzip, 1))
		isUnicode = ZR_ARGS;
	else
	{
		unsigned char	*lastchar;

		if (isUnicode)
			WideCharToMultiByte(CP_UTF8, 0, (const WCHAR *)dir, -1, (LPSTR)&tunzip->Rootdir[0], MAX_PATH, 0, 0);
		else
			lstrcpyA((char *)&tunzip->Rootdir[0], (const char *)dir);

		// Make sure it ends with '\\'
		lastchar = &tunzip->Rootdir[lstrlenA((char *)&tunzip->Rootdir[0]) - 1];
		if (*lastchar != DIRSLASH_CHAR)
		{
			lastchar[1] = DIRSLASH_CHAR;
			lastchar[2] = 0;
		}

		isUnicode = ZR_OK;
	}

	return(isUnicode);
}

DWORD WINAPI UnzipSetBaseDirA(HUNZIP tunzip, const char *dir)
{
	return(unzipSetBaseDir((TUNZIP*)tunzip, dir, 0));
}

DWORD WINAPI UnzipSetBaseDirW(HUNZIP tunzip, const WCHAR *dir)
{
	return(unzipSetBaseDir((TUNZIP*)tunzip, dir, 1));
}












































































































// ================== CONSTANTS =====================

typedef unsigned char UCH;      // unsigned 8-bit value
typedef unsigned short USH;     // unsigned 16-bit value
typedef unsigned long ULG;      // unsigned 32-bit value

// internal file attribute
#define UNKNOWN (-1)
#define BINARY  0
#define ASCII   1

#define BEST		-1		// Use best method (deflation or store)
#define STORE		0		// Store method
#define DEFLATE		8		// Deflation method

// MSDOS file or directory attributes
#define MSDOS_HIDDEN_ATTR	0x02
#define MSDOS_DIR_ATTR		0x10

// Lengths of headers after signatures in bytes
#define LOCHEAD		26
#define CENHEAD		42
#define END64HEAD	44
#define END64LOCHEAD 16
#define ENDHEAD		18

// Definitions for extra field handling:
#define EB_HEADSIZE			4		// length of a extra field block header
#define EB_LEN				2		// offset of data length field in header
#define EB_UT_MINLEN		1		// minimal UT field contains Flags byte
#define EB_UT_FLAGS			0		// byte offset of Flags field
#define EB_UT_TIME1			1		// byte offset of 1st time value
#define EB_UT_FL_MTIME		(1 << 0)	// mtime present
#define EB_UT_FL_ATIME		(1 << 1)	// atime present
#define EB_UT_FL_CTIME		(1 << 2)	// ctime present
#define EB_UT_LEN(n)		(EB_UT_MINLEN + 4 * (n))
#define EB_L_UT_SIZE		(EB_HEADSIZE + EB_UT_LEN(3))
#define EB_C_UT_SIZE		(EB_HEADSIZE + EB_UT_LEN(1))

// Signatures for zip file information headers
#define LOCSIG     0x04034b50L
#define CENSIG     0x02014b50L
#define ENDSIG     0x06054b50L
#define END64SIG   0x06064b50L
#define END64LOCSIG 0x07064b50L
#define EXTLOCSIG  0x08074b50L

// The minimum and maximum match lengths
#define MIN_MATCH  3
#define MAX_MATCH  258

// Maximum window size = 32K. If you are really short of memory, compile
// with a smaller WSIZE but this reduces the compression ratio for files
// of size > WSIZE. WSIZE must be a power of two in the current implementation.
#define WSIZE  (0x8000)

// Minimum amount of lookahead, except at the end of the input file.
// See deflate.c for comments about the MIN_MATCH+1.
#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)

// In order to simplify the code, particularly on 16 bit machines, match
// distances are limited to MAX_DIST instead of WSIZE.
#define MAX_DIST  (WSIZE-MIN_LOOKAHEAD)

// All codes must not exceed MAX_BITS bits
#define MAX_BITS		15

// Bit length codes must not exceed MAX_BL_BITS bits
#define MAX_BL_BITS		7

// number of length codes, not counting the special END_BLOCK code
#define LENGTH_CODES	29

// number of literal bytes 0..255
#define LITERALS		256

// end of block literal code
#define END_BLOCK		256

// number of Literal or Length codes, including the END_BLOCK code
#define L_CODES			(LITERALS+1+LENGTH_CODES)

// number of distance codes
#define D_CODES			30

// number of codes used to transfer the bit lengths
#define BL_CODES		19

// The three kinds of block type
#define STORED_BLOCK	0
#define STATIC_TREES	1
#define DYN_TREES		2

// Sizes of match buffers for literals/lengths and distances.  There are
// 4 reasons for limiting LIT_BUFSIZE to 64K:
//   - frequencies can be kept in 16 bit counters
//   - if compression is not successful for the first block, all input data is
//     still in the window so we can still emit a stored block even when input
//     comes from standard input.  (This can also be done for all blocks if
//     LIT_BUFSIZE is not greater than 32K.)
//   - if compression is not successful for a file smaller than 64K, we can
//     even emit a stored file instead of a stored block (saving 5 bytes).
//   - creating new Huffman trees less frequently may not provide fast
//     adaptation to changes in the input data statistics. (Take for
//     example a binary file with poorly compressible code followed by
//     a highly compressible string table.) Smaller buffer sizes give
//     fast adaptation but have of course the overhead of transmitting trees
//     more frequently.
//   - I can't count above 4
// The current code is general and allows DIST_BUFSIZE < LIT_BUFSIZE (to save
// memory at the expense of compression). Some optimizations would be possible
// if we rely on DIST_BUFSIZE == LIT_BUFSIZE.
#define LIT_BUFSIZE		0x8000
#define DIST_BUFSIZE	LIT_BUFSIZE

// repeat previous bit length 3-6 times (2 bits of repeat count)
#define REP_3_6			16

// repeat a zero length 3-10 times  (3 bits of repeat count)
#define REPZ_3_10		17

// repeat a zero length 11-138 times  (7 bits of repeat count)
#define REPZ_11_138		18

// maximum heap size
#define HEAP_SIZE		(2*L_CODES+1)

// Number of bits used within bi_buf. (bi_buf may be implemented on
// more than 16 bits on some systems.)
#define ZIP_BUF_SIZE	(8 * 2*sizeof(char))

// For portability to 16 bit machines, do not use values above 15.
#define HASH_BITS	15

#define HASH_SIZE	(unsigned)(1<<HASH_BITS)
#define HASH_MASK	(HASH_SIZE-1)
#define WMASK		(WSIZE-1)
// HASH_SIZE and WSIZE must be powers of two

#define FAST		4
#define SLOW		2
// speed options for the general purpose bit flag

#define TOO_FAR		4096
// Matches of length 3 are discarded if their distance exceeds TOO_FAR

// Number of bits by which ins_h and del_h must be shifted at each
// input step. It must be such that after MIN_MATCH steps, the oldest
// byte no longer takes part in the hash key, that is:
//   H_SHIFT * MIN_MATCH >= HASH_BITS
#define H_SHIFT		((HASH_BITS+MIN_MATCH-1)/MIN_MATCH)

// Index within the heap array of least frequent node in the Huffman tree
#define SMALLEST 1















// ================== STRUCTS =====================

typedef struct {
	USH good_length; // reduce lazy search above this match length
	USH max_lazy;    // do not perform lazy search above this match length
	USH nice_length; // quit search above this match length
	USH max_chain;
} CONFIG;

// Data structure describing a single value and its code string.
typedef struct {
	union {
		USH		freq;		// frequency count
		USH		code;		// bit string
	} fc;
	union {
		USH		dad;		// father node in Huffman tree
		USH		len;		// length of bit string
	} dl;
} CT_DATA;

typedef struct {
	CT_DATA		*dyn_tree;		// the dynamic tree
	CT_DATA		*static_tree;	// corresponding static tree or NULL
	const int	*extra_bits;	// extra bits for each code or NULL
	int			extra_base;		// base index for extra_bits
	int			elems;			// max number of elements in the tree
	int			max_length;		// max bit length for the codes
	int			max_code;		// largest code with non zero frequency
} TREE_DESC;

typedef struct
{
	// ... Since the bit lengths are imposed, there is no need for the L_CODES
	// extra codes used during heap construction. However the codes 286 and 287
	// are needed to build a canonical tree (see ct_init below).
	CT_DATA		dyn_ltree[HEAP_SIZE];		// literal and length tree
	CT_DATA		dyn_dtree[2 * D_CODES + 1];		// distance tree
	CT_DATA		static_ltree[L_CODES + 2];	// the static literal tree...
	CT_DATA		static_dtree[D_CODES];		// the static distance tree...
	// ... (Actually a trivial tree since all codes use 5 bits.)
	CT_DATA		bl_tree[2 * BL_CODES + 1];		// Huffman tree for the bit lengths

	TREE_DESC	l_desc;
	TREE_DESC	d_desc;
	TREE_DESC	bl_desc;

	USH			bl_count[MAX_BITS + 1];	// number of codes at each bit length for an optimal tree

	// The sons of heap[n] are heap[2*n] and heap[2*n+1]. heap[0] is not used.
	// The same heap array is used to build all trees.
	int			heap[2 * L_CODES + 1];		// heap used to build the Huffman trees
	int			heap_len;				// number of elements in the heap
	int			heap_max;				// element of largest frequency

	// Depth of each subtree used as tie breaker for trees of equal frequency
	UCH			depth[2 * L_CODES + 1];

	UCH			length_code[MAX_MATCH - MIN_MATCH + 1];
	// length code for each normalized match length (0 == MIN_MATCH)

	// distance codes. The first 256 values correspond to the distances
	// 3 .. 258, the last 256 values correspond to the top 8 bits of
	// the 15 bit distances.
	UCH			dist_code[512];

	// First normalized length for each code (0 = MIN_MATCH)
	int			base_length[LENGTH_CODES];

	// First normalized distance for each code (0 = distance of 1)
	int			base_dist[D_CODES];

	UCH			l_buf[LIT_BUFSIZE];		// buffer for literals/lengths
	USH			d_buf[DIST_BUFSIZE];	// buffer for distances

	// flag_buf is a bit array distinguishing literals from lengths in
	// l_buf, and thus indicating the presence or absence of a distance.
	UCH			flag_buf[(LIT_BUFSIZE / 8)];

	// bits are filled in flags starting at bit 0 (least significant).
	// Note: these flags are overkill in the current code since we don't
	// take advantage of DIST_BUFSIZE == LIT_BUFSIZE.
	unsigned	last_lit;			// running index in l_buf
	unsigned	last_dist;			// running index in d_buf
	unsigned	last_flags;			// running index in flag_buf
	UCH			flags;				// current flags not yet saved in flag_buf
	UCH			flag_bit;			// current bit used in flags

	ULG			opt_len;			// bit length of current block with optimal trees
	ULG			static_len;			// bit length of current block with static trees

	ULONGLONG	cmpr_bytelen;		// total byte length of compressed file (within the ZIP)
	ULONGLONG	cmpr_len_bits;		// number of bits past 'cmpr_bytelen'

	USH			*file_type;			// pointer to UNKNOWN, BINARY or ASCII

#ifdef _DEBUG
	// input_len is for debugging only since we can't get it by other means.
	ULONGLONG	input_len;			// total byte length of source file
#endif

} TTREESTATE;

typedef struct
{
	unsigned	bi_buf;			// Output buffer. bits are inserted starting at the bottom (least significant
	// bits). The width of bi_buf must be at least 16 bits.
	int			bi_valid;		// Number of valid bits in bi_buf. All bits above the last valid bit are always zero.
	char		*out_buf;		// Current output buffer.
	DWORD		out_offset;		// Current offset in output buffer
	DWORD		out_size;		// Size of current output buffer
#ifdef _DEBUG
	ULG			bits_sent;		// bit length of the compressed data  only needed for debugging
#endif
} TBITSTATE;

typedef struct
{
	// Sliding window. Input bytes are read into the second half of the window,
	// and move to the first half later to keep a dictionary of at least WSIZE
	// bytes. With this organization, matches are limited to a distance of
	// WSIZE-MAX_MATCH bytes, but this ensures that IO is always
	// performed with a length multiple of the block size. Also, it limits
	// the window size to 64K, which is quite useful on MSDOS.
	// To do: limit the window size to WSIZE+CBSZ if SMALL_MEM (the code would
	// be less efficient since the data would have to be copied WSIZE/CBSZ times)
	UCH				window[2L * WSIZE];

	// Link to older string with same hash index. To limit the size of this
	// array to 64K, this link is maintained only for the last 32K strings.
	// An index in this array is thus a window index modulo 32K.
	unsigned		prev[WSIZE];

	// Heads of the hash chains or 0. If your compiler thinks that
	// HASH_SIZE is a dynamic value, recompile with -DDYN_ALLOC.
	unsigned		head[HASH_SIZE];

	// window size, 2*WSIZE except for MMAP or BIG_MEM, where it is the
	// input file length plus MIN_LOOKAHEAD.
	ULG				window_size;

	// window position at the beginning of the current output block. Gets
	// negative when the window is moved backwards.
	long			block_start;

	// hash index of string to be inserted
	unsigned		ins_h;

	// Length of the best match at previous step. Matches not greater than this
	// are discarded. This is used in the lazy match evaluation.
	unsigned int	prev_length;

	unsigned		strstart;		// start of string to insert
	unsigned		match_start;	// start of matching string
	unsigned		lookahead;		// number of valid bytes ahead in window

	// To speed up deflation, hash chains are never searched beyond this length.
	// A higher limit improves compression ratio but degrades the speed.
	unsigned		max_chain_length;

	// Attempt to find a better match only when the current match is strictly
	// smaller than this value. This mechanism is used only for compression
	// levels >= 4.
	unsigned int	max_lazy_match;
	unsigned		good_match;		// Use a faster search when the previous match is longer than this
	int				nice_match;		// Stop searching when current match exceeds this

	unsigned char	eofile;			// flag set at end of source file
	unsigned char	sliding;		// Set to false when the source is already in memory

} TDEFLATESTATE;

typedef struct
{
	struct _TZIP	*tzip;
	TTREESTATE		ts;
	TBITSTATE		bs;
	TDEFLATESTATE	ds;
#ifdef _DEBUG
	const char		*err;
#endif
	unsigned char	level;		// compression level
	//	unsigned char	seekable;	// 1 if we can seek() in the source
}  TSTATE;


// Holds the Access, Modify, Create times, and DOS timestamp. Also the file attributes
typedef struct {
	lutime_t		atime, mtime, ctime;
	unsigned long	timestamp;
	unsigned long	attributes;
} IZTIMES;

// For storing values to be written to the ZIP Central Directory. Note: We write
// default values for some of the fields
typedef struct _TZIPFILEINFO {
	USH			flg, how;
	ULG			tim, crc;
	ULONGLONG	siz, len;
	DWORD		nam, ext, cext;			// offset of ext must be >= LOCHEAD
	USH			dsk, att, lflg;			// offset of lflg must be >= LOCHEAD
	ULG			atx;
	ULONGLONG	off;
	char		*extra;					// Extra field (set only if ext != 0)
	char		*cextra;				// Extra in central (set only if cext != 0)
	char		iname[MAX_PATH];		// Internal file name after cleanup
	struct _TZIPFILEINFO	*nxt;		// Pointer to next header in list
} TZIPFILEINFO;

// For TZIP->flags
#define TZIP_DESTMEMORY			0x0000001	// Set if TZIP->destination is memory, instead of a file, handle
#define TZIP_DESTCLOSEFH		0x0000002	// Set if we open the file handle in zipCreate() and must close it later
#define TZIP_CANSEEK			0x0000004	// Set if the destination (where we write the ZIP) is seekable
#define TZIP_DONECENTRALDIR		0x0000008	// Set after we've written out the Central Directory
#define TZIP_ENCRYPT			0x0000010	// Set if we should apply encrpytion to the output
#define TZIP_SRCCANSEEK			0x0000020	// Set if source (that supplies the data to add to the ZIP file) is seekable
#define TZIP_SRCCLOSEFH			0x0000040	// Set if we've opened the source file handle, and therefore need to close it
#define TZIP_SRCMEMORY			0x0000080	// Set if TZIP->source is memory, instead of a file, handle
//#define TZIP_OPTION_ABORT		0x4000000	// Defined in LiteZip.h. Must not be used for another purpose
//#define TZIP_OPTION_GZIP		0x8000000	// Defined in LiteZip.h. Must not be used for another purpose

typedef struct _TZIP
{
	DWORD		flags;

	// ====================================================================
	// These variables are for the destination (that we're writing the ZIP to).
	// We can write to pipe, file-by-handle, file-by-name, memory-to-memmapfile
	HANDLE		destination;	// If not TZIP_DESTMEMORY, this is the handle to the zip file we write to. Otherwise.
	// it points to a memory buffer where we write the zip.
	char		*password;		// A copy of the password from the application.
	ULONGLONG	writ;			// How many bytes we've written to destination. This is maintained by addSrc(), not
	// writeDestination(), to avoid confusion over seeks.
	ULONGLONG	ooffset;		// The initial offset where we start writing the zip within destination. (This allows
	// the app to write the zip to an already open file that has data in it).
	DWORD		lasterr;		// The last error code.

	// Memory map stuff
	HANDLE		memorymap;		// If not 0, then this is a memory mapped file handle.
	ULONGLONG	opos;			// Current (byte) position in "destination".
	DWORD		mapsize;		// The size of the memory buffer.

	// Encryption
	unsigned long keys[3];		// keys are initialised inside addSrc()
	char		*encbuf;		// If encrypting, then this is a temporary workspace for encrypting the data (to
	unsigned int encbufsize;	// be used and resized inside writeDestination(), and deleted when the TZIP is freed)

	TZIPFILEINFO	*zfis;		// Each file gets added onto this list, for writing the table at the end

	// ====================================================================
	// These variables are for the source (that supplies the data to be added to the ZIP)
	ULONGLONG	isize, totalRead;	// size is not set until close() on pipes
	ULG			crc;				// crc is not set until close(). iwrit is cumulative
	HANDLE		source;
	DWORD		lenin, posin;		// These are for a memory buffer source
	// and a variable for what we've done with the input: (i.e. compressed it!)
	ULONGLONG	csize;				// Compressed size, set by the compression routines.
	TSTATE		*state;				// We allocate just one state object per zip, because it's big (500k), and store a ptr here. It is freed when the TZIP is freed
	char		buf[16384];			// Used by some of the compression routines. This must be last!!
} TZIP;











// ========================== DATA ============================

// NOTE: I specify this data section to be Shared (ie, each running rexx
// script shares these variables, rather than getting its own copies of these
// variables). This is because, since I have only globals that are read-only
// or whose value is the same for all processes, I don't need a separate copy
// of these for each process that uses this DLL. In Visual C++'s Linker
// settings, I add "/section:Shared,rws"

#pragma data_seg("shared")

static const int Extra_lbits[LENGTH_CODES] // extra bits for each length code
= { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };

static const int Extra_dbits[D_CODES] // extra bits for each distance code
= { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

static const int Extra_blbits[BL_CODES]// extra bits for each bit length code
= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 7 };

// Values for max_lazy_match, good_match, nice_match and max_chain_length,
// depending on the desired pack level (0..9). The values given below have
// been tuned to exclude worst case performance for pathological files.
// Better values may be found for specific files.
//

static const CONFIG ConfigurationTable[10] = {
	//  good lazy nice chain
	{ 0, 0, 0, 0 },  // 0 store only
	{ 4, 4, 8, 4 },  // 1 maximum speed, no lazy matches
	{ 4, 5, 16, 8 },  // 2
	{ 4, 6, 32, 32 },  // 3
	{ 4, 4, 16, 16 },  // 4 lazy matches */
	{ 8, 16, 32, 32 },  // 5
	{ 8, 16, 128, 128 },  // 6
	{ 8, 32, 128, 256 },  // 7
	{ 32, 128, 258, 1024 }, // 8
	{ 32, 258, 258, 4096 } };// 9 maximum compression */

// Note: the deflate() code requires max_lazy >= MIN_MATCH and max_chain >= 4
// For deflate_fast() (levels <= 3) good is ignored and lazy has a different meaning.

static const ULG CrcTable[256] = {
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
	0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
	0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
	0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
	0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
	0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
	0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
	0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
	0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
	0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
	0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
	0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
	0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
	0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
	0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
	0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
	0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
	0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
	0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
	0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
	0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
	0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
	0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
	0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
	0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
	0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
	0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
	0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
	0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
	0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
	0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
	0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
	0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
	0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
	0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
	0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
	0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
	0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
	0x2d02ef8dL
};

static const char ZipSuffixes[] = { ".z\0\
																		.zip\0\
																											.zoo\0\
																																				.arc\0\
																																													.lzh\0\
																																																						.arj\0\
																																																															.gz\0\
																																																																								.tgz\0" };

// The lengths of the bit length codes are sent in order of decreasing
// probability, to avoid transmitting the lengths for unused bit length codes.
static const UCH BL_order[BL_CODES] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

static const WCHAR		AllFilesStrW[] = L"\\*.*";
static const char		AllFilesStrA[] = "\\*.*";

// Error messages
#pragma data_seg()

















// ====================== LOCAL DECLARATIONS =======================

static void			writeDestShort(register TZIP *, DWORD);
static void			writeDestination(register TZIP *, const char *, DWORD);
static unsigned		readFromSource(register TZIP *, char *buf, unsigned size);
static void			pqdownheap(register TSTATE *, CT_DATA *, int);
static void			gen_codes(register TSTATE *, CT_DATA *, int);
static void			compress_block(register TSTATE *, CT_DATA *, CT_DATA *);
static BOOL			send_bits(register TSTATE *, int, int);
static unsigned		bi_reverse(register unsigned, register unsigned char);
static void			bi_windup(register TSTATE *);
static void			copy_block(register TSTATE *, char *, DWORD, DWORD);
static void			fill_window(register TSTATE *);







// ==================== Unix/Windows Time conversion ====================

static void filetime2dosdatetime(const FILETIME ft, WORD *dosdate, WORD *dostime)
{
	// date: bits 0-4 are day of month 1-31. Bits 5-8 are month 1..12. Bits 9-15 are year-1980
	// time: bits 0-4 are seconds/2, bits 5-10 are minute 0..59. Bits 11-15 are hour 0..23
	SYSTEMTIME st;

	FileTimeToSystemTime(&ft, &st);
	*dosdate = (WORD)(((st.wYear - 1980) & 0x7f) << 9);
	*dosdate |= (WORD)((st.wMonth & 0xf) << 5);
	*dosdate |= (WORD)((st.wDay & 0x1f));
	*dostime = (WORD)((st.wHour & 0x1f) << 11);
	*dostime |= (WORD)((st.wMinute & 0x3f) << 5);
	*dostime |= (WORD)((st.wSecond * 2) & 0x1f);
}

static lutime_t filetime2timet(const FILETIME ft)
{
	LONGLONG i;

	i = *(LONGLONG*)&ft;
	return (lutime_t)((i - 116444736000000000) / 10000000);
}


static void getNow(lutime_t *pft, WORD *dosdate, WORD *dostime)
{
	SYSTEMTIME	st;
	FILETIME	ft;

	GetLocalTime(&st);
	SystemTimeToFileTime(&st, &ft);
	filetime2dosdatetime(ft, dosdate, dostime);
	*pft = filetime2timet(ft);
}





/********************* getFileInfo() ***********************
* Retrieves the attributes, size, and timestamps (in ZIP
* format) from the file handle.
*/

static DWORD getFileInfo(TZIP *tzip, IZTIMES *times)
{
	register ULG					a;

	// The date and time is returned in a long with the date most significant to allow
	// unsigned integer comparison of absolute times. The attributes have two
	// high bytes unix attr, and two low bytes a mapping of that to DOS attr.

	BY_HANDLE_FILE_INFORMATION	bhi;

	// translate windows file attributes into zip ones.
	if (!GetFileInformationByHandle(tzip->source, &bhi)) return(ZR_NOFILE);

	a = 0;

	// Zip uses the lower word for its interpretation of windows attributes
	if (bhi.dwFileAttributes & FILE_ATTRIBUTE_READONLY) a = 0x01;
	if (bhi.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) a |= 0x02;
	if (bhi.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) a |= 0x04;
	if (bhi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) a |= 0x10;
	if (bhi.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) a |= 0x20;
	// It uses the upper word for standard unix attr, which we manually construct
	if (bhi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		a |= 0x40000000;	// directory
	else
		a |= 0x80000000;	// normal file
	a |= 0x01000000;		// readable
	if (!(bhi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
		a |= 0x00800000;	// writeable

	// Check if it's an executable
	if ((tzip->isize = GetFileSize(tzip->source, 0)) > 40 && !tzip->ooffset)
	{
		DWORD			read;
		unsigned short	magic;
		unsigned long	hpos;

		SetFilePointer(tzip->source, 0, 0, FILE_BEGIN);
		ReadFile(tzip->source, &magic, sizeof(magic), &read, 0);
		SetFilePointer(tzip->source, 36, 0, FILE_BEGIN);
		ReadFile(tzip->source, &hpos, sizeof(hpos), &read, 0);
		if (magic == 0x54AD && tzip->isize > hpos + 4 + 20 + 28)
		{
			unsigned long signature;

			SetFilePointer(tzip->source, hpos, 0, FILE_BEGIN);
			ReadFile(tzip->source, &signature, sizeof(signature), &read, 0);
			if (signature == IMAGE_DOS_SIGNATURE || signature == IMAGE_OS2_SIGNATURE || signature == IMAGE_OS2_SIGNATURE_LE || signature == IMAGE_NT_SIGNATURE)
				a |= 0x00400000; // executable
		}

		// Reset file pointer back to where it initially was
		SetFilePointer(tzip->source, 0, 0, FILE_BEGIN);
	}

	times->attributes = a;

	// time_t is 32bit number of seconds elapsed since 0:0:0GMT, Jan1, 1970.
	// but FILETIME is 64bit number of 100-nanosecs since Jan1, 1601
	times->atime = filetime2timet(bhi.ftLastAccessTime);
	times->mtime = filetime2timet(bhi.ftLastWriteTime);
	times->ctime = filetime2timet(bhi.ftCreationTime);

	// Get MSDOS timestamp (ie, date is high word, time is low word)
	{
		WORD	dosdate, dostime;

		filetime2dosdatetime(bhi.ftLastWriteTime, &dosdate, &dostime);
		times->timestamp = (WORD)dostime | (((DWORD)dosdate) << 16);
	}

	return(ZR_OK);
}









// ==================== DEBUG STUFF ====================

#ifdef _DEBUG
static void Assert(TSTATE *state, BOOL cond, const char *msg)
{
	if (!cond) state->err = msg;
}

static void Trace(const char *x, ...)
{
	va_list paramList;
	va_start(paramList, x);
	paramList;
	va_end(paramList);
}
#endif








// =================== Low level compression ==================

// Send a code of the given tree. c and tree must not have side effects
#define send_code(state, c, tree) send_bits(state, tree[c].fc.code, tree[c].dl.len)

// Mapping from a distance to a distance code. dist is the distance - 1 and
// must not have side effects. dist_code[256] and dist_code[257] are never used.
#define d_code(dist) ((dist) < 256 ? state->ts.dist_code[dist] : state->ts.dist_code[256+((dist)>>7)])

// the arguments must not have side effects
#define Max(a,b) (a >= b ? a : b)





/********************* init_block() *********************
* Initializes a new block.
*/

static void init_block(register TSTATE *state)
{
	register int n; // iterates over tree elements

	// Initialize the trees
	for (n = 0; n < L_CODES; n++) state->ts.dyn_ltree[n].fc.freq = 0;
	for (n = 0; n < D_CODES; n++) state->ts.dyn_dtree[n].fc.freq = 0;
	for (n = 0; n < BL_CODES; n++) state->ts.bl_tree[n].fc.freq = 0;

	state->ts.dyn_ltree[END_BLOCK].fc.freq = 1;
	state->ts.opt_len = state->ts.static_len = 0;
	state->ts.last_lit = state->ts.last_dist = state->ts.last_flags = 0;
	state->ts.flags = 0; state->ts.flag_bit = 1;
}





/************************ ct_init() ******************
* Allocates the match buffer, initializes the various
* tables and saves the location of the internal file
* attribute (ascii/binary) and method (DEFLATE/STORE).
*/

static void ct_init(register TSTATE *state, USH *attr)
{
	int		n;			// iterates over tree elements
	int		bits;		// bit counter
	int		length;		// length value
	int		code;		// code value
	int		dist;		// distance index

	state->ts.file_type = attr;

	state->ts.cmpr_bytelen = state->ts.cmpr_len_bits = 0;
#ifdef _DEBUG
	state->ts.input_len = 0;
#endif

	if (state->ts.static_dtree[0].dl.len) return;	// ct_init already called

	// Initialize the mapping length (0..255) -> length code (0..28)
	length = 0;
	for (code = 0; code < LENGTH_CODES - 1; code++)
	{
		state->ts.base_length[code] = length;
		for (n = 0; n < (1 << Extra_lbits[code]); n++)
			state->ts.length_code[length++] = (UCH)code;
	}

	// Note that the length 255 (match length 258) can be represented
	// in two different ways: code 284 + 5 bits or code 285, so we
	// overwrite length_code[255] to use the best encoding:
	state->ts.length_code[length - 1] = (UCH)code;

	// Initialize the mapping dist (0..32K) -> dist code (0..29)
	dist = 0;
	for (code = 0; code < 16; code++)
	{
		state->ts.base_dist[code] = dist;
		for (n = 0; n < (1 << Extra_dbits[code]); n++)
			state->ts.dist_code[dist++] = (UCH)code;
	}

	dist >>= 7; // from now on, all distances are divided by 128
	for (; code < D_CODES; code++)
	{
		state->ts.base_dist[code] = dist << 7;
		for (n = 0; n < (1 << (Extra_dbits[code] - 7)); n++)
			state->ts.dist_code[256 + dist++] = (UCH)code;
	}

	// Construct the codes of the static literal tree
	for (bits = 0; bits <= MAX_BITS; bits++) state->ts.bl_count[bits] = 0;
	n = 0;
	while (n <= 143) state->ts.static_ltree[n++].dl.len = 8, state->ts.bl_count[8]++;
	while (n <= 255) state->ts.static_ltree[n++].dl.len = 9, state->ts.bl_count[9]++;
	while (n <= 279) state->ts.static_ltree[n++].dl.len = 7, state->ts.bl_count[7]++;
	while (n <= 287) state->ts.static_ltree[n++].dl.len = 8, state->ts.bl_count[8]++;
	// fc.codes 286 and 287 do not exist, but we must include them in the
	// tree construction to get a canonical Huffman tree (longest code
	// all ones)
	gen_codes(state, (CT_DATA *)state->ts.static_ltree, L_CODES + 1);

	// The static distance tree is trivial
	for (n = 0; n < D_CODES; n++)
	{
		state->ts.static_dtree[n].dl.len = 5;
		state->ts.static_dtree[n].fc.code = (USH)bi_reverse(n, 5);
	}

	// Initialize the first block of the first file
	init_block(state);
}





/********************** pqremove() ********************
* Removes the smallest element from the heap and recreates
* the heap with one less element. Updates heap and heap_len.
*/

#define pqremove(tree, top) \
{\
	top = state->ts.heap[SMALLEST]; \
	state->ts.heap[SMALLEST] = state->ts.heap[state->ts.heap_len--]; \
	pqdownheap(state, tree, SMALLEST); \
}





/*********************** smaller() *******************
* Compares two subtrees, using the tree depth as tie
* breaker when the subtrees have equal frequency. This
* minimizes the worst case length.
*/

#define smaller(tree, n, m) \
	(tree[n].fc.freq < tree[m].fc.freq || \
	(tree[n].fc.freq == tree[m].fc.freq && state->ts.depth[n] <= state->ts.depth[m]))




/********************** pdownheap() ********************
* Restores the heap property by moving down the tree
* starting at node k, exchanging a node with the smallest
* of its two sons if necessary, stopping when the heap
* property is re-established (each father smaller than its
* two sons).
*/

static void pqdownheap(register TSTATE *state, CT_DATA *tree, int k)
{
	int		v;
	register int		j;

	v = state->ts.heap[k];
	j = k << 1;  // left son of k

	while (j <= state->ts.heap_len)
	{
		// Set j to the smallest of the two sons:
		if (j < state->ts.heap_len && smaller(tree, state->ts.heap[j + 1], state->ts.heap[j])) j++;

		// Exit if v is smaller than both sons
		if (smaller(tree, v, state->ts.heap[j])) break;

		// Exchange v with the smallest son
		state->ts.heap[k] = state->ts.heap[j];
		k = j;

		// And continue down the tree, setting j to the left son of k
		j <<= 1;
	}
	state->ts.heap[k] = v;
}





/********************** gen_bitlen() ********************
* Computes the optimal bit lengths for a tree and updates
* the total bit length for the current block.
*
* IN assertion: the fields freq and dad are set, heap[heap_max] and
*    above are the tree nodes sorted by increasing frequency.
* OUT assertions: the field len is set to the optimal bit length, the
*     array bl_count contains the frequencies for each bit length.
*     The length opt_len is updated; static_len is also updated if stree is
*     not null.
*/
static void gen_bitlen(register TSTATE *state, TREE_DESC *desc)
{
	CT_DATA		*tree = desc->dyn_tree;
	const int	*extra = desc->extra_bits;
	int			base = desc->extra_base;
	int			max_code = desc->max_code;
	int			max_length = desc->max_length;
	CT_DATA		*stree = desc->static_tree;
	int			h;              // heap index
	int			n, m;           // iterate over the tree elements
	int			bits;           // bit length
	int			xbits;          // extra bits
	USH			f;              // frequency
	int			overflow = 0;   // number of elements with bit length too large

	for (bits = 0; bits <= MAX_BITS; bits++) state->ts.bl_count[bits] = 0;

	// In a first pass, compute the optimal bit lengths (which may
	// overflow in the case of the bit length tree)
	tree[state->ts.heap[state->ts.heap_max]].dl.len = 0; // root of the heap

	for (h = state->ts.heap_max + 1; h < HEAP_SIZE; h++)
	{
		n = state->ts.heap[h];
		bits = tree[tree[n].dl.dad].dl.len + 1;
		if (bits > max_length)
		{
			bits = max_length;
			++overflow;
		}
		tree[n].dl.len = (USH)bits;
		// We overwrite tree[n].dl.dad which is no longer needed

		if (n > max_code) continue; // not a leaf node

		state->ts.bl_count[bits]++;
		xbits = 0;
		if (n >= base) xbits = extra[n - base];
		f = tree[n].fc.freq;
		state->ts.opt_len += (ULG)f * (bits + xbits);
		if (stree) state->ts.static_len += (ULG)f * (stree[n].dl.len + xbits);
	}
	if (overflow == 0) return;

#ifdef _DEBUG
	Trace("\nbit length overflow\n");
#endif
	// This happens for example on obj2 and pic of the Calgary corpus

	// Find the first bit length which could increase:
	do
	{
		bits = max_length - 1;
		while (state->ts.bl_count[bits] == 0) bits--;
		state->ts.bl_count[bits]--;           // move one leaf down the tree
		state->ts.bl_count[bits + 1] += (USH)2; // move one overflow item as its brother
		state->ts.bl_count[max_length]--;
		// The brother of the overflow item also moves one step up,
		// but this does not affect bl_count[max_length]
		overflow -= 2;
	} while (overflow > 0);

	// Now recompute all bit lengths, scanning in increasing frequency.
	// h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
	// lengths instead of fixing only the wrong ones. This idea is taken
	// from 'ar' written by Haruhiko Okumura)
	for (bits = max_length; bits != 0; bits--)
	{
		n = state->ts.bl_count[bits];
		while (n != 0)
		{
			m = state->ts.heap[--h];
			if (m > max_code) continue;
			if (tree[m].dl.len != (USH)bits)
			{
#ifdef _DEBUG
				Trace("code %d bits %d->%d\n", m, tree[m].dl.len, bits);
#endif
				state->ts.opt_len += ((long)bits - (long)tree[m].dl.len)*(long)tree[m].fc.freq;
				tree[m].dl.len = (USH)bits;
			}
			--n;
		}
	}
}





/************************ gen_codes() *******************
* Generates the codes for a given tree and bit counts
* (which need not be optimal).
*
* IN assertion: the array bl_count contains the bit length statistics for
* the given tree and the field len is set for all tree elements.
*
* OUT assertion: the field code is set for all tree elements of non
* zero code length.
*/
static void gen_codes(register TSTATE *state, CT_DATA *tree, int max_code)
{
	USH				next_code[MAX_BITS + 1];	// next code value for each bit length
	{
		register DWORD	bits;
		USH				code;

		// The distribution counts are first used to generate the code values
		// without bit reversal
		code = 0;
		for (bits = 1; bits <= MAX_BITS; bits++)
			next_code[bits] = code = (USH)((code + state->ts.bl_count[bits - 1]) << 1);

		// Check that the bit counts in bl_count are consistent. The last code
		// must be all ones
#ifdef _DEBUG
		Assert(state, code + state->ts.bl_count[MAX_BITS] - 1 == (1 << ((USH)MAX_BITS)) - 1, "inconsistent bit counts");
		Trace("\ngen_codes: max_code %d ", max_code);
#endif
	}

	{
		int		n;

		for (n = 0; n <= max_code; n++)
		{
			register DWORD len;

			// Reverse the bits
			if ((len = tree[n].dl.len)) tree[n].fc.code = (USH)bi_reverse(next_code[len]++, (unsigned char)len);
		}
	}
}





/************************** build_tree() **********************
* Constructs one Huffman tree and assigns the code bit strings
* and lengths. Updates the total bit length for the current block.
*
* RETURNS: The fields len and code are set to the optimal bit length
* and corresponding code. The length opt_len is updated; static_len is
* also updated if stree is not null. The field max_code is set.
*
* NOTE: Before calling here, the "freq" field of all tree
* elements must be set to an appropriate value.
*/

static void build_tree(register TSTATE *state, TREE_DESC *desc)
{
	CT_DATA		*tree = desc->dyn_tree;
	CT_DATA		*stree = desc->static_tree;
	int			elems = desc->elems;
	int			n, m;				// iterate over heap elements
	int			max_code = -1;		// largest code with non zero frequency
	int			node = elems;		// next internal node of the tree

	// Construct the initial heap, with least frequent element in
	// heap[SMALLEST]. The sons of heap[n] are heap[2*n] and heap[2*n+1].
	// heap[0] is not used
	state->ts.heap_len = 0;
	state->ts.heap_max = HEAP_SIZE;

	for (n = 0; n < elems; n++)
	{
		if (tree[n].fc.freq)
		{
			state->ts.heap[++state->ts.heap_len] = max_code = n;
			state->ts.depth[n] = 0;
		}
		else
			tree[n].dl.len = 0;
	}

	// The pkzip format requires that at least one distance code exists,
	// and that at least one bit should be sent even if there is only one
	// possible code. So to avoid special checks later on we force at least
	// two codes of non zero frequency
	while (state->ts.heap_len < 2)
	{
		int newcp = state->ts.heap[++state->ts.heap_len] = (max_code < 2 ? ++max_code : 0);
		tree[newcp].fc.freq = 1;
		state->ts.depth[newcp] = 0;
		state->ts.opt_len--;
		if (stree) state->ts.static_len -= stree[newcp].dl.len;
		// new is 0 or 1 so it does not have extra bits
	}
	desc->max_code = max_code;

	// The elements heap[heap_len/2+1 .. heap_len] are leaves of the tree,
	// establish sub-heaps of increasing lengths:
	for (n = state->ts.heap_len / 2; n >= 1; n--) pqdownheap(state, tree, n);

	// Construct the Huffman tree by repeatedly combining the least two
	// frequent nodes
	do
	{
		if (state->tzip->flags & TZIP_OPTION_ABORT)
		{
			state->tzip->lasterr = ZR_ABORT;
			return;
		}

		pqremove(tree, n);						// n = node of least frequency
		m = state->ts.heap[SMALLEST];			// m = node of next least frequency

		state->ts.heap[--state->ts.heap_max] = n; // keep the nodes sorted by frequency
		state->ts.heap[--state->ts.heap_max] = m;

		// Create a new node father of n and m
		tree[node].fc.freq = (USH)(tree[n].fc.freq + tree[m].fc.freq);
		state->ts.depth[node] = (UCH)(Max(state->ts.depth[n], state->ts.depth[m]) + 1);
		tree[n].dl.dad = tree[m].dl.dad = (USH)node;
		// and insert the new node in the heap
		state->ts.heap[SMALLEST] = node++;
		pqdownheap(state, tree, SMALLEST);

	} while (state->ts.heap_len >= 2);

	state->ts.heap[--state->ts.heap_max] = state->ts.heap[SMALLEST];

	// At this point, the fields freq and dad are set. We can now
	// generate the bit lengths
	gen_bitlen(state, desc);

	// The field len is now set, we can generate the bit codes
	gen_codes(state, tree, max_code);
}





/************************** scan_tree() **********************
* Scans a literal or distance tree to determine the frequencies
* of the codes in the bit length tree. Updates opt_len to take
* into account the repeat counts. (The contribution of the bit
* length codes will be added later during the construction of
* bl_tree.)
*/

static void scan_tree(register TSTATE *state, CT_DATA *tree, int max_code)
{
	int				n;			// iterates over all tree elements
	int				prevlen;	// last emitted length
	int				curlen;		// length of current code
	int				nextlen;	// length of next code
	register BYTE	count;		// repeat count of the current code
	register BYTE	max_count;	// max repeat count
	register BYTE	min_count;	// min repeat count

	count = 0;
	nextlen = tree[0].dl.len;
	prevlen = -1;

	max_count = 7;
	min_count = 4;
	if (!nextlen)
	{
		max_count = 138;
		min_count = 3;
	}

	// Set to -1 as a sort of "guard marker" that shouldn't be crossed
	tree[max_code + 1].dl.len = (USH)-1;

	for (n = 0; n <= max_code; n++)
	{
		curlen = nextlen;
		nextlen = tree[n + 1].dl.len;
		if (++count < max_count && curlen == nextlen) continue;
		if (count < min_count)
			state->ts.bl_tree[curlen].fc.freq = (USH)(state->ts.bl_tree[curlen].fc.freq + count);
		else if (curlen)
		{
			if (curlen != prevlen) ++state->ts.bl_tree[curlen].fc.freq;
			++state->ts.bl_tree[REP_3_6].fc.freq;
		}
		else if (count <= 10)
			++state->ts.bl_tree[REPZ_3_10].fc.freq;
		else
			++state->ts.bl_tree[REPZ_11_138].fc.freq;

		count = 0;
		prevlen = curlen;
		if (!nextlen)
		{
			max_count = 138;
			min_count = 3;
		}
		else if (curlen == nextlen)
		{
			max_count = 6;
			min_count = 3;
		}
		else
		{
			max_count = 7;
			min_count = 4;
		}
	}
}





/*********************** send_tree() ************************
* Sends a literal or distance tree in compressed form, using
* the codes in bl_tree().
*/

static BOOL send_tree(register TSTATE *state, CT_DATA *tree, int max_code)
{
	int				n;			// iterates over all tree elements
	int				prevlen;	// last emitted length
	int				curlen;		// length of current code
	int				nextlen;	// length of next code
	register BYTE	count;		// repeat count of the current code
	register BYTE	max_count;	// max repeat count
	register BYTE	min_count;	// min repeat count

	count = 0;
	nextlen = tree[0].dl.len;
	prevlen = -1;

	max_count = 7;
	min_count = 4;
	if (!nextlen)
	{
		max_count = 138;
		min_count = 3;
	}

	for (n = 0; n <= max_code; n++)
	{
		if (state->tzip->flags & TZIP_OPTION_ABORT) goto out;

		curlen = nextlen;
		nextlen = tree[n + 1].dl.len;
		if (++count < max_count && curlen == nextlen) continue;

		if (count < min_count)
		{
			do
			{
				if (!send_code(state, curlen, state->ts.bl_tree)) goto out;
			} while (--count);
		}
		else if (curlen)
		{
			if (curlen != prevlen)
			{
				if (!send_code(state, curlen, state->ts.bl_tree)) goto out;
				--count;
			}
#ifdef _DEBUG
			Assert(state, count >= 3 && count <= 6, " 3_6?");
#endif
			if (!send_code(state, REP_3_6, state->ts.bl_tree) || !send_bits(state, count - 3, 2)) goto out;

		}
		else if (count <= 10)
		{
			if (!send_code(state, REPZ_3_10, state->ts.bl_tree) || !send_bits(state, count - 3, 3)) goto out;
		}
		else
		{
			if (!send_code(state, REPZ_11_138, state->ts.bl_tree) || !send_bits(state, count - 11, 7))
			out:			return(0);
		}

		count = 0;
		prevlen = curlen;
		if (!nextlen)
		{
			max_count = 138;
			min_count = 3;
		}
		else if (curlen == nextlen)
		{
			max_count = 6;
			min_count = 3;
		}
		else
		{
			max_count = 7;
			min_count = 4;
		}
	}

	return(1);
}





/************************* build_bl_tree() *******************
* Constructs the Huffman tree for the bit lengths and returns
* the index in BL_order[] of the last bit length code to send.
*/

static int build_bl_tree(register TSTATE *state)
{
	register int		max_blindex;		// index of last bit length code of non zero freq

	// Determine the bit length frequencies for literal and distance trees
	scan_tree(state, state->ts.dyn_ltree, state->ts.l_desc.max_code);
	scan_tree(state, state->ts.dyn_dtree, state->ts.d_desc.max_code);

	// Build the bit length tree:
	build_tree(state, (TREE_DESC *)(&state->ts.bl_desc));
	if (!(state->tzip->flags & TZIP_OPTION_ABORT))
	{
		// opt_len now includes the length of the tree representations, except
		// the lengths of the bit lengths codes and the 5+5+4 bits for the counts.

		// Determine the number of bit length codes to send. The pkzip format
		// requires that at least 4 bit length codes be sent. (appnote.txt says
		// 3 but the actual value used is 4.)
		for (max_blindex = BL_CODES - 1; max_blindex >= 3; max_blindex--)
		{
			if (state->ts.bl_tree[BL_order[max_blindex]].dl.len != 0) break;
		}

		// Update opt_len to include the bit length tree and counts
		state->ts.opt_len += 3 * (max_blindex + 1) + 5 + 5 + 4;
#ifdef _DEBUG
		Trace("\ndyn trees: dyn %ld, stat %ld", state->ts.opt_len, state->ts.static_len);
#endif
	}
	return(max_blindex);
}





/************************* send_all_trees() *******************
* Sends the header for a block using dynamic Huffman trees:
* the counts, the lengths of the bit length codes, the literal
* tree and the distance tree.
* IN assertion: lcodes >= 257, dcodes >= 1, blcodes >= 4.
*/

/************************* send_all_trees() *******************
* Sends the header for a block using dynamic Huffman trees:
* the counts, the lengths of the bit length codes, the literal
* tree and the distance tree.
* IN assertion: lcodes >= 257, dcodes >= 1, blcodes >= 4.
*/

static BOOL send_all_trees(register TSTATE *state, int lcodes, int dcodes, int blcodes)
{
	int		rank;	// index into BL_order[]

#ifdef _DEBUG
	Assert(state, lcodes >= 257 && dcodes >= 1 && blcodes >= 4, "not enough codes");
	Assert(state, lcodes <= L_CODES && dcodes <= D_CODES && blcodes <= BL_CODES, "too many codes");
	Trace("\nbl counts: ");
#endif

	if (send_bits(state, lcodes - 257, 5) &&	// not +255 as stated in appnote.txt 1.93a or -256 in 2.04c
		send_bits(state, dcodes - 1, 5) &&
		send_bits(state, blcodes - 4, 4))		// not -3 as stated in appnote.txt
	{
		for (rank = 0; rank < blcodes; rank++)
		{
#ifdef _DEBUG
			Trace("\nbl code %2d ", BL_order[rank]);
#endif
			if ((state->tzip->flags & TZIP_OPTION_ABORT) || !send_bits(state, state->ts.bl_tree[BL_order[rank]].dl.len, 3)) goto out;
		}
#ifdef _DEBUG
		Trace("\nbl tree: sent %ld", state->bs.bits_sent);
#endif

		// Send the literal tree
		if (send_tree(state, (CT_DATA *)state->ts.dyn_ltree, lcodes - 1))
		{
#ifdef _DEBUG
			Trace("\nlit tree: sent %ld", state->bs.bits_sent);
#endif
			// Send the distance tree
			if (send_tree(state, state->ts.dyn_dtree, dcodes - 1))
			{
#ifdef _DEBUG
				Trace("\ndist tree: sent %ld", state->bs.bits_sent);
#endif
				return(1);
			}
		}
	}
out:
	return(0);
}





/********************* set_file_type() ********************
* Sets the file type to ASCII or BINARY, using a crude
* approximation: binary if more than 20% of the bytes are
* <= 6 or >= 128, ascii otherwise.
*
* IN assertion: the fields freq of dyn_ltree are set and
* the total of all frequencies does not exceed 64K (to fit
* in an int on 16 bit machines).
*/

static void set_file_type(register TSTATE *state)
{
	unsigned	n;
	unsigned	ascii_freq;
	unsigned	bin_freq;

	n = ascii_freq = bin_freq = 0;

	while (n < 7)        bin_freq += state->ts.dyn_ltree[n++].fc.freq;
	while (n < 128)    ascii_freq += state->ts.dyn_ltree[n++].fc.freq;
	while (n < LITERALS) bin_freq += state->ts.dyn_ltree[n++].fc.freq;
	*state->ts.file_type = (USH)(bin_freq >(ascii_freq >> 2) ? BINARY : ASCII);
}





/************************* flush_block() ********************
* Determines the best encoding for the current block: dynamic
* trees, static trees or store, and outputs the encoded block
* to the zip file.
*
* RETURNS: The total compressed length (in bytes) for the
* file so far.
*/

static void flush_block(register TSTATE *state, char *buf, ULG stored_len, DWORD eof)
{
	ULG		opt_lenb, static_lenb;	// opt_len and static_len in bytes
	int		max_blindex;			// index of last bit length code of non zero freq

	state->ts.flag_buf[state->ts.last_flags] = state->ts.flags; // Save the flags for the last 8 items 

	// Check if the file is ascii or binary
	if (*state->ts.file_type == (USH)UNKNOWN) set_file_type(state);

	// Construct the literal and distance trees
	build_tree(state, (TREE_DESC *)(&state->ts.l_desc));
	if (!(state->tzip->flags & TZIP_OPTION_ABORT))
	{
#ifdef _DEBUG
		Trace("\nlit data: dyn %ld, stat %ld", state->ts.opt_len, state->ts.static_len);
#endif

		build_tree(state, (TREE_DESC *)(&state->ts.d_desc));
#ifdef _DEBUG
		Trace("\ndist data: dyn %ld, stat %ld", state->ts.opt_len, state->ts.static_len);
#endif
		if (!(state->tzip->flags & TZIP_OPTION_ABORT))
		{
			// At this point, opt_len and static_len are the total bit lengths of
			// the compressed block data, excluding the tree representations.

			// Build the bit length tree for the above two trees, and get the index
			// in BL_order[] of the last bit length code to send.
			max_blindex = build_bl_tree(state);
			if (!(state->tzip->flags & TZIP_OPTION_ABORT))
			{
				// Determine the best encoding. Compute first the block length in bytes
				opt_lenb = (state->ts.opt_len + 3 + 7) >> 3;
				static_lenb = (state->ts.static_len + 3 + 7) >> 3;

#ifdef _DEBUG
				state->ts.input_len += stored_len;
				Trace("\nopt %lu(%lu) stat %lu(%lu) stored %lu lit %u dist %u ", opt_lenb, state->ts.opt_len, static_lenb, state->ts.static_len, stored_len, state->ts.last_lit, state->ts.last_dist);
#endif

				if (static_lenb <= opt_lenb) opt_lenb = static_lenb;

				if (stored_len + 4 <= opt_lenb && buf)
				{
					// two words for the lengths
					// The test buf != NULL is only necessary if LIT_BUFSIZE > WSIZE.
					// Otherwise we can't have processed more than WSIZE input bytes since
					// the last block flush, because compression would have been
					// successful. If LIT_BUFSIZE <= WSIZE, it is never too late to
					// transform a block into a stored block.

					// Send block type
					send_bits(state, (STORED_BLOCK << 1) + eof, 3);
					state->ts.cmpr_bytelen += ((state->ts.cmpr_len_bits + 3 + 7) >> 3) + stored_len + 4;
					state->ts.cmpr_len_bits = 0;
					copy_block(state, buf, (unsigned)stored_len, 1); // with header
				}
				else if (static_lenb == opt_lenb)
				{
					send_bits(state, (STATIC_TREES << 1) + eof, 3);
					compress_block(state, (CT_DATA *)state->ts.static_ltree, (CT_DATA *)state->ts.static_dtree);
					state->ts.cmpr_len_bits += 3 + state->ts.static_len;
					goto upd;
				}
				else
				{
					send_bits(state, (DYN_TREES << 1) + eof, 3);
					send_all_trees(state, state->ts.l_desc.max_code + 1, state->ts.d_desc.max_code + 1, max_blindex + 1);
					compress_block(state, state->ts.dyn_ltree, state->ts.dyn_dtree);
					state->ts.cmpr_len_bits += 3 + state->ts.opt_len;
				upd:				state->ts.cmpr_bytelen += state->ts.cmpr_len_bits >> 3;
					state->ts.cmpr_len_bits &= 7;
				}
#ifdef _DEBUG
				Assert(state, ((state->ts.cmpr_bytelen << 3) + state->ts.cmpr_len_bits) == state->bs.bits_sent, "bad compressed size");
#endif
				if (!state->tzip->lasterr)
				{
					init_block(state);

					if (eof)
					{
						bi_windup(state);
						state->ts.cmpr_len_bits += 7;	// align on byte boundary
					}
#ifdef _DEBUG
					Trace("\n");
#endif
					// Set compressed size so far
					state->tzip->csize = state->ts.cmpr_bytelen + (state->ts.cmpr_len_bits >> 3);
				}
			}
		}
	}
}





/********************* ct_tally() **********************
* Saves the match info and tallies the frequency counts.
*
* RETURNS: TRUE if the current block must be flushed.
*/

static unsigned char ct_tally(register TSTATE *state, int dist, int lc)
{
	state->ts.l_buf[state->ts.last_lit++] = (UCH)lc;
	if (!dist)
	{
		// lc is the unmatched char
		state->ts.dyn_ltree[lc].fc.freq++;
	}
	else
	{
		// Here, lc is the match length - MIN_MATCH
		--dist;				// dist = match distance - 1
#ifdef _DEBUG
		Assert(state, (USH)dist < (USH)MAX_DIST && (USH)lc <= (USH)(MAX_MATCH - MIN_MATCH) && (USH)d_code(dist) < (USH)D_CODES, "ct_tally: bad match");
#endif
		state->ts.dyn_ltree[state->ts.length_code[lc] + LITERALS + 1].fc.freq++;
		state->ts.dyn_dtree[d_code(dist)].fc.freq++;

		state->ts.d_buf[state->ts.last_dist++] = (USH)dist;
		state->ts.flags |= state->ts.flag_bit;
	}
	state->ts.flag_bit <<= 1;

	// Store the flags if they fill a byte
	if ((state->ts.last_lit & 7) == 0)
	{
		state->ts.flag_buf[state->ts.last_flags++] = state->ts.flags;
		state->ts.flags = 0, state->ts.flag_bit = 1;
	}

	// Try to guess if it is profitable to stop the current block here 
	if (state->level > 2 && (state->ts.last_lit & 0xfff) == 0)
	{
		DWORD	dcode;
		ULG		out_length;
		ULG		in_length;

		// Compute an upper bound for the compressed length
		out_length = (ULG)state->ts.last_lit * 8L;
		in_length = (ULG)state->ds.strstart - state->ds.block_start;
		dcode = D_CODES;
		while (dcode--) out_length += (ULG)state->ts.dyn_dtree[dcode].fc.freq * (5L + Extra_dbits[dcode]);
		out_length >>= 3;

#ifdef _DEBUG
		Trace("\nlast_lit %u, last_dist %u, in %ld, out ~%ld(%ld%%) ", state->ts.last_lit, state->ts.last_dist, in_length, out_length, 100L - out_length * 100L / in_length);
#endif
		// Should we stop the block here?
		if (state->ts.last_dist < state->ts.last_lit / 2 && out_length < in_length / 2) return(1);
	}

	// NOTE: We avoid equality with LIT_BUFSIZE because of wraparound at 64K
	// on 16 bit machines and because stored blocks are restricted to
	// 64K-1 bytes
	return(state->ts.last_lit == LIT_BUFSIZE - 1 || state->ts.last_dist == DIST_BUFSIZE);
}





/********************* compress_block() ********************
* Sends the block data compressed using the given Huffman
* trees.
*/

static void compress_block(register TSTATE *state, CT_DATA *ltree, CT_DATA *dtree)
{
	unsigned	dist;		// distance of matched string
	int			lc;			// match length or unmatched char (if dist == 0)
	DWORD		lx;			// running index in l_buf
	DWORD		dx;			// running index in d_buf
	DWORD		fx;			// running index in flag_buf
	UCH			flag;		// current flags
	unsigned	code;		// the code to send
	int			extra;		// number of extra bits to send

	lx = dx = fx = flag = 0;

	if (state->ts.last_lit)
	{
		do
		{
			if ((lx & 7) == 0) flag = state->ts.flag_buf[fx++];
			lc = state->ts.l_buf[lx++];

			if ((flag & 1) == 0)
			{
				// send a literal byte
				if (!send_code(state, lc, ltree)) goto out;
			}			// bug in VC4.0 if these {} are removed!!!
			else
			{
				// Here, lc is the match length - MIN_MATCH

				code = state->ts.length_code[lc];

				// send the length code
				if (!send_code(state, code + LITERALS + 1, ltree))
				out:				return;

				// send the extra length bits
				extra = Extra_lbits[code];
				if (extra)
				{
					lc -= state->ts.base_length[code];
					if (!send_bits(state, lc, extra)) goto out;
				}

				dist = state->ts.d_buf[dx++];
				// Here, dist is the match distance - 1

				// send the distance code
				code = d_code(dist);
#ifdef _DEBUG
				Assert(state, code < D_CODES, "bad d_code");
#endif
				if (!send_code(state, code, dtree)) goto out;

				// send the extra distance bits
				extra = Extra_dbits[code];
				if (extra)
				{
					dist -= state->ts.base_dist[code];
					if (!send_bits(state, dist, extra)) goto out;
				}
			} // literal or match pair ?

			flag >>= 1;

		} while (lx < state->ts.last_lit);
	}

	send_code(state, END_BLOCK, ltree);
}






/*********************** send_bits() **********************
* Sends a value on a given number of bits.
*
* IN assertion: length <= 16 and value fits in length bits.
*/

static BOOL send_bits(register TSTATE *state, int value, int length)
{
#ifdef _DEBUG
	Assert(state, length > 0 && length <= 15, "invalid length");
	state->bs.bits_sent += (ULG)length;
#endif
	// If not enough room in bi_buf, use (bi_valid) bits from bi_buf and
	// (ZIP_BUF_SIZE - bi_valid) bits from value to flush the filled bi_buf,
	// then fill in the rest of (value), leaving (length - (ZIP_BUF_SIZE-bi_valid))
	// unused bits in bi_buf.
	state->bs.bi_buf |= (value << state->bs.bi_valid);
	state->bs.bi_valid += length;
	if (state->bs.bi_valid > ZIP_BUF_SIZE)
	{
		// If we've filled the output buffer, flush it
		if (state->bs.out_offset + 1 >= state->bs.out_size)
		{
			writeDestination(state->tzip, state->bs.out_buf, state->bs.out_offset);
			if (state->tzip->lasterr) return(0);
			state->bs.out_offset = 0;
		}

		// Store the short in Intel-order
		state->bs.out_buf[state->bs.out_offset++] = (char)((state->bs.bi_buf) & 0xff);
		state->bs.out_buf[state->bs.out_offset++] = (char)((USH)((state->bs.bi_buf) & 0xFFFF) >> 8);

		state->bs.bi_valid -= ZIP_BUF_SIZE;
		state->bs.bi_buf = (unsigned)value >> (length - state->bs.bi_valid);
	}

	return(1);
}





/********************** bi_reverse() *********************
* Reverses the first "len" bits of a code.
*
* code =	The number whose bits are to be shifted.
* len =	The number of bits to reverse (1 to 15).
*/

static unsigned bi_reverse(register unsigned code, register unsigned char len)
{
	register unsigned res;

	res = 0;
	goto rev;
	do
	{
		res <<= 1;
		code >>= 1;
	rev:	res |= code & 1;
	} while (--len);
	return(res);
}





/*********************** bi_windup() ********************
* Writes out any remaining bits in an incomplete byte.
*/

static void bi_windup(register TSTATE *state)
{
	if (state->bs.bi_valid > 8)
	{
		// If we've filled the output buffer, flush it
		if (state->bs.out_offset + 1 >= state->bs.out_size)
		{
			writeDestination(state->tzip, state->bs.out_buf, state->bs.out_offset);
			state->bs.out_offset = 0;
		}

		// Store the short in Intel-order
		state->bs.out_buf[state->bs.out_offset++] = (char)((state->bs.bi_buf) & 0xff);
		state->bs.out_buf[state->bs.out_offset++] = (char)((USH)((state->bs.bi_buf) & 0xFFFF) >> 8);
	}
	else if (state->bs.bi_valid > 0)
	{
		// If we've filled the output buffer, flush it
		if (state->bs.out_offset >= state->bs.out_size)
		{
			writeDestination(state->tzip, state->bs.out_buf, state->bs.out_offset);
			state->bs.out_offset = 0;
		}

		// Store the byte
		state->bs.out_buf[state->bs.out_offset++] = (char)state->bs.bi_buf & 0xFF;
	}

	// Flush the buffer to the ZIP archive
	writeDestination(state->tzip, state->bs.out_buf, state->bs.out_offset);
	state->bs.bi_buf = state->bs.out_offset = state->bs.bi_valid = 0;
#ifdef _DEBUG
	state->bs.bits_sent = (state->bs.bits_sent + 7) & ~7;
#endif
}





/************************ copy_block() ********************
* Copies a stored block to the zip file, storing first the
* length and its one's complement if requested.
*/

static void copy_block(register TSTATE *state, char *block, DWORD len, DWORD header)
{
	// Align on a byte boundary by writing out any previous, uncompleted bytes
	bi_windup(state);

	// If a header, write the length and one's complement
	if (header)
	{
		// If we don't have room for 2 shorts, flush the output buffer
		if (state->bs.out_offset + 3 >= state->bs.out_size)
		{
			writeDestination(state->tzip, state->bs.out_buf, state->bs.out_offset);
			state->bs.out_offset = 0;
		}

		// Store the short in Intel-order
		state->bs.out_buf[state->bs.out_offset++] = (char)((len)& 0xff);
		state->bs.out_buf[state->bs.out_offset++] = (char)((USH)(len & 0xFFFF) >> 8);

		// Store one's complement
		state->bs.out_buf[state->bs.out_offset++] = (char)((~len) & 0xff);
		state->bs.out_buf[state->bs.out_offset++] = (char)((USH)((~len) & 0xFF) >> 8);

		// Flush the 2 shorts now (because we're going to flush the block
		// which is in a different memory buffer)
		writeDestination(state->tzip, state->bs.out_buf, state->bs.out_offset);
		state->bs.out_offset = 0;


#ifdef _DEBUG
		state->bs.bits_sent += (2 * 16);
#endif
	}

	// Write out the block
	writeDestination(state->tzip, block, len);

#ifdef _DEBUG
	state->bs.bits_sent += (ULG)len << 3;
#endif
}






/* ===========================================================================
* Update a hash value with the given input byte
* IN  assertion: all calls to to UPDATE_HASH are made with consecutive
*    input characters, so that a running hash key can be computed from the
*    previous key instead of complete recalculation each time.
*/
#define UPDATE_HASH(h,c) (h = (((h)<<H_SHIFT) ^ (c)) & HASH_MASK)





/* ===========================================================================
* Insert string s in the dictionary and set match_head to the previous head
* of the hash chain (the most recent string with same hash key). Return
* the previous length of the hash chain.
* IN  assertion: all calls to to INSERT_STRING are made with consecutive
*    input characters and the first MIN_MATCH bytes of s are valid
*    (except for the last MIN_MATCH-1 bytes of the input file).
*/
#define INSERT_STRING(s, match_head) \
   (UPDATE_HASH(state->ds.ins_h, state->ds.window[(s) + (MIN_MATCH-1)]), \
    state->ds.prev[(s) & WMASK] = match_head = state->ds.head[state->ds.ins_h], \
    state->ds.head[state->ds.ins_h] = (s))





/************************ lm_init() ***********************
* Initializes the "longest match" routines in preparation of
* writing a new zip file.
*
* NOTE: state->ds.window_size is > 0 if the source file in
* its entirety is already read into the state->ds.window[]
* array, 0 otherwise. In the first case, window_size is
* sufficient to contain the whole input file plus MIN_LOOKAHEAD
* bytes (to avoid referencing memory beyond the end
* of window[] when looking for matches towards the end).
*/

static void lm_init(register TSTATE *state, DWORD pack_level, USH *flags)
{
	register unsigned j;

	// Do not slide the window if the whole input is already in memory (window_size > 0)
	//	state->ds.sliding = 0;
	//	if (!state->ds.window_size)
	//	{
	state->ds.sliding = 1;
	state->ds.window_size = (ULG)(2L * WSIZE);
	//	}

	// Initialize the hash table (avoiding 64K overflow for 16 bit systems).
	// prev[] will be initialized on the fly
	state->ds.head[HASH_SIZE - 1] = 0;
	ZeroMemory(state->ds.head, (HASH_SIZE - 1) * sizeof(*state->ds.head));

	// Set the default configuration parameters:
	state->ds.max_lazy_match = ConfigurationTable[pack_level].max_lazy;
	state->ds.good_match = ConfigurationTable[pack_level].good_length;
	state->ds.nice_match = ConfigurationTable[pack_level].nice_length;
	state->ds.max_chain_length = ConfigurationTable[pack_level].max_chain;
	if (pack_level <= 2) *flags |= FAST;
	else if (pack_level >= 8) *flags |= SLOW;

	// ??? reduce max_chain_length for binary files


	// Fill the state->ds.window[] buffer with source bytes
	if (!(state->ds.lookahead = readFromSource(state->tzip, (char *)state->ds.window, WSIZE * 2))) return;

	// At start of input buffer, and haven't reached the end of the source yet
	state->ds.strstart = state->ds.block_start = state->ds.eofile = 0;

	// Make sure that we always have enough lookahead
	if (state->ds.lookahead < MIN_LOOKAHEAD) fill_window(state);

	// If lookahead < MIN_MATCH, ins_h is garbage, but this is
	// not important since only literal bytes will be emitted
	state->ds.ins_h = 0;
	for (j = 0; j < MIN_MATCH - 1; j++) UPDATE_HASH(state->ds.ins_h, state->ds.window[j]);
}





/********************** longest_match() *******************
* Sets match_start to the longest match starting at the
* given string and return its length. Any match shorter or
* equal to prev_length ia discarded, in which case the
* result is equal to prev_length and match_start is
* garbage.
*
* IN assertions: cur_match is the head of the hash chain for the current
*   string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
*/

static int longest_match(register TSTATE *state, unsigned cur_match)
{
	unsigned chain_length = state->ds.max_chain_length;   // max hash chain length
	register UCH *scan = state->ds.window + state->ds.strstart; // current string
	register UCH *match;                    // matched string
	register int len;                           // length of current match
	int best_len = state->ds.prev_length;                 // best match length so far
	unsigned limit = state->ds.strstart > (unsigned)MAX_DIST ? state->ds.strstart - (unsigned)MAX_DIST : 0;
	register UCH *strend;
	register UCH scan_end1;
	register UCH scan_end;

	// Stop when cur_match becomes <= limit. To simplify the code,
	// we prevent matches with the string of window index 0.

	// The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
	// It is easy to get rid of this optimization if necessary.
#ifdef _DEBUG
	Assert(state, HASH_BITS >= 8 && MAX_MATCH == 258, "Code too clever");
#endif

	strend = state->ds.window + state->ds.strstart + MAX_MATCH;
	scan_end1 = scan[best_len - 1];
	scan_end = scan[best_len];

	// Do not waste too much time if we already have a good match:
	if (state->ds.prev_length >= state->ds.good_match) chain_length >>= 2;

#ifdef _DEBUG
	Assert(state, state->ds.strstart <= state->ds.window_size - MIN_LOOKAHEAD, "insufficient lookahead");
#endif

	do
	{
#ifdef _DEBUG
		Assert(state, cur_match < state->ds.strstart, "no future");
#endif
		match = state->ds.window + cur_match;

		// Skip to next match if the match length cannot increase
		// or if the match length is less than 2:
		if (match[best_len] != scan_end ||
			match[best_len - 1] != scan_end1 ||
			*match != *scan ||
			*++match != scan[1])      continue;

		// The check at best_len-1 can be removed because it will be made
		// again later. (This heuristic is not always a win.)
		// It is not necessary to compare scan[2] and match[2] since they
		// are always equal when the other bytes match, given that
		// the hash keys are equal and that HASH_BITS >= 8.
		scan += 2, match++;

		// We check for insufficient lookahead only every 8th comparison;
		// the 256th check will be made at strstart+258.
		do
		{
		} while (*++scan == *++match && *++scan == *++match &&
			*++scan == *++match && *++scan == *++match &&
			*++scan == *++match && *++scan == *++match &&
			*++scan == *++match && *++scan == *++match &&
			scan < strend);

#ifdef _DEBUG
		Assert(state, scan <= state->ds.window + (unsigned)(state->ds.window_size - 1), "wild scan");
#endif

		len = MAX_MATCH - (int)(strend - scan);
		scan = strend - MAX_MATCH;

		if (len > best_len)
		{
			state->ds.match_start = cur_match;
			best_len = len;
			if (len >= state->ds.nice_match) break;
			scan_end1 = scan[best_len - 1];
			scan_end = scan[best_len];
		}
	} while ((cur_match = state->ds.prev[cur_match & WMASK]) > limit && --chain_length != 0);

	return(best_len);
}





#define check_match(state,start, match, length)





/*********************** fill_window() **********************
* Fills the window when the lookahead becomes insufficient.
* Updates strstart and lookahead, and sets eofile if end of
* input file.
*
* IN assertion: lookahead < MIN_LOOKAHEAD && strstart + lookahead > 0
* OUT assertions: strstart <= window_size-MIN_LOOKAHEAD
*    At least one byte has been read, or eofile is set; file reads are
*    performed for at least two bytes (required for the translate_eol option).
*/

static void fill_window(register TSTATE *state)
{
	register unsigned	n, m;
	unsigned			more;	// Amount of free space at the end of the window

	do
	{
		more = (unsigned)(state->ds.window_size - state->ds.lookahead - state->ds.strstart);

		// If the window is almost full and there is insufficient lookahead,
		// move the upper half to the lower one to make room in the upper half.
		if (more == (unsigned)-1)
		{
			// Very unlikely, but possible on 16 bit machine if strstart == 0
			// and lookahead == 1 (input done one byte at time)
			--more;
		}

		// For a memory-block source, the whole source is already in memory so
		// we don't need to perform sliding. We must however call readFromSource() in
		// order to compute the crc, update lookahead, and possibly set eofile
		else if (state->ds.strstart >= WSIZE + MAX_DIST && state->ds.sliding)
		{
			// By the IN assertion, the window is not empty so we can't confuse
			// more == 0 with more == 64K on a 16 bit machine
			CopyMemory(state->ds.window, (char *)state->ds.window + WSIZE, WSIZE);
			state->ds.match_start -= WSIZE;
			state->ds.strstart -= WSIZE;		// we now have strstart >= MAX_DIST

			state->ds.block_start -= WSIZE;

			for (n = 0; n < HASH_SIZE; n++)
			{
				m = state->ds.head[n];
				state->ds.head[n] = (unsigned)(m >= WSIZE ? m - WSIZE : 0);
			}
			for (n = 0; n < WSIZE; n++)
			{
				if (state->tzip->flags & TZIP_OPTION_ABORT)
				{
					state->tzip->lasterr = ZR_ABORT;
					goto bad;
				}

				m = state->ds.prev[n];
				state->ds.prev[n] = (unsigned)(m >= WSIZE ? m - WSIZE : 0);
				// If n is not on any hash chain, prev[n] is garbage but
				// its value will never be used.
			}
			more += WSIZE;
		}

		if (state->ds.eofile) break;

		// If there was no sliding:
		//    strstart <= WSIZE+MAX_DIST-1 && lookahead <= MIN_LOOKAHEAD - 1 &&
		//    more == window_size - lookahead - strstart
		// => more >= window_size - (MIN_LOOKAHEAD-1 + WSIZE + MAX_DIST-1)
		// => more >= window_size - 2*WSIZE + 2
		// In the memory-block case (not yet supported in gzip),
		//   window_size == input_size + MIN_LOOKAHEAD  &&
		//   strstart + lookahead <= input_size => more >= MIN_LOOKAHEAD.
		// Otherwise, window_size == 2*WSIZE so more >= 2.
		// If there was sliding, more >= WSIZE. So in all cases, more >= 2.
#ifdef _DEBUG
		Assert(state, more >= 2, "more < 2");
#endif
		if (!(n = readFromSource(state->tzip, (char *)(state->ds.window + state->ds.strstart + state->ds.lookahead), more)))
		{
			state->ds.eofile = 1;
		bad:		break;
		}

		state->ds.lookahead += n;
	} while (state->ds.lookahead < MIN_LOOKAHEAD);
}






/*************************** deflate_fast() *******************
* Processes a new input file and return its compressed length.
* Does not perform lazy evaluation of matches and inserts
* new strings in the dictionary only for unmatched strings or
* for short matches. It is used only for the fast compression
* options.
*/

#if 0

static void deflate_fast(register TSTATE *state)
{
	unsigned		hash_head;		// head of the hash chain
	unsigned		match_length;	// length of best match
	unsigned char	flush;			// set if current block must be flushed

	hash_head = match_length = 0;
	state->ds.prev_length = MIN_MATCH - 1;
	while (state->ds.lookahead)
	{
		// Insert the string window[strstart .. strstart+2] in the
		// dictionary, and set hash_head to the head of the hash chain:
		if (state->ds.lookahead >= MIN_MATCH)
			INSERT_STRING(state->ds.strstart, hash_head);

		// Find the longest match, discarding those <= prev_length.
		// At this point we have always match_length < MIN_MATCH
		if (hash_head != 0 && state->ds.strstart - hash_head <= MAX_DIST)
		{
			// To simplify the code, we prevent matches with the string
			// of window index 0 (in particular we have to avoid a match
			// of the string with itself at the start of the input file).

			// Do not look for matches beyond the end of the input.
			// This is necessary to make deflate deterministic.
			if ((unsigned)state->ds.nice_match > state->ds.lookahead) state->ds.nice_match = (int)state->ds.lookahead;
			match_length = longest_match(state, hash_head);
			// longest_match() sets match_start
			if (match_length > state->ds.lookahead) match_length = state->ds.lookahead;
		}
		if (match_length >= MIN_MATCH)
		{
			check_match(state, state->ds.strstart, state->ds.match_start, match_length);

			flush = ct_tally(state, state->ds.strstart - state->ds.match_start, match_length - MIN_MATCH);

			state->ds.lookahead -= match_length;

			// Insert new strings in the hash table only if the match length
			// is not too large. This saves time but degrades compression.
			if (match_length <= state->ds.max_lazy_match && state->ds.lookahead >= MIN_MATCH)
			{
				--match_length;		// string at strstart already in hash table
				do
				{
					++state->ds.strstart;
					INSERT_STRING(state->ds.strstart, hash_head);
					// strstart never exceeds WSIZE-MAX_MATCH, so there are
					// always MIN_MATCH bytes ahead.
				} while (--match_length);
				++state->ds.strstart;
			}
			else
			{
				state->ds.strstart += match_length;
				match_length = 0;
				state->ds.ins_h = state->ds.window[state->ds.strstart];
				UPDATE_HASH(state->ds.ins_h, state->ds.window[state->ds.strstart + 1]);
#ifdef _DEBUG
				Assert(state, MIN_MATCH == 3, "Call UPDATE_HASH() MIN_MATCH-3 more times");
#endif
			}
		}
		else
		{
			// No match, output a literal byte
			flush = ct_tally(state, 0, state->ds.window[state->ds.strstart]);
			--state->ds.lookahead;
			++state->ds.strstart;
		}

		if (flush)
		{
			flush_block(state, state->ds.block_start >= 0 ? &state->ds.window[(unsigned)state->ds.block_start] :
				0, (long)state->ds.strstart - state->ds.block_start, 0)
				if (state->tzip->lasterr) goto bad;

			state->ds.block_start = state->ds.strstart;
		}

		// Make sure that we always have enough lookahead, except
		// at the end of the input file. We need MAX_MATCH bytes
		// for the next match, plus MIN_MATCH bytes to insert the
		// string following the next match
		if (state->ds.lookahead < MIN_LOOKAHEAD)
		{
			fill_window(state);
			if (state->tzip->lasterr)
			bad:			return(state->tzip->lasterr);
		}
	}

	// EOF
	flush_block(state, state->ds.block_start >= 0 ? &state->ds.window[(unsigned)state->ds.block_start] :
		0, (long)state->ds.strstart - state->ds.block_start, 1));
}

#endif




/************************* deflate() ***********************
* Same as above, but achieves better compression. We use a
* lazy evaluation for matches: a match is finally adopted
* only if there is no better match at the next window position.
*/

static void deflate(register TSTATE *state)
{
	unsigned			hash_head;				// head of hash chain
	unsigned			prev_match;				// previous match
	unsigned char		flush;					// set if current block must be flushed
	unsigned char		match_available;		// set if previous match exists
	register unsigned	match_length;			// length of best match

	hash_head = match_available = 0;
	match_length = MIN_MATCH - 1;

	// Process the input bloc.
	while (state->ds.lookahead)
	{
		// Insert the string window[strstart .. strstart+2] in the
		// dictionary, and set hash_head to the head of the hash chain:
		if (state->ds.lookahead >= MIN_MATCH)
			INSERT_STRING(state->ds.strstart, hash_head);

		// Find the longest match, discarding those <= prev_length.
		state->ds.prev_length = match_length, prev_match = state->ds.match_start;
		match_length = MIN_MATCH - 1;

		if (hash_head != 0 && state->ds.prev_length < state->ds.max_lazy_match && state->ds.strstart - hash_head <= MAX_DIST)
		{
			// To simplify the code, we prevent matches with the string
			// of window index 0 (in particular we have to avoid a match
			// of the string with itself at the start of the input file).

			// Do not look for matches beyond the end of the source.
			// This is necessary to make deflate deterministic
			if ((unsigned)state->ds.nice_match > state->ds.lookahead) state->ds.nice_match = (int)state->ds.lookahead;
			match_length = longest_match(state, hash_head);
			// longest_match() sets match_start
			if (match_length > state->ds.lookahead) match_length = state->ds.lookahead;

			// Ignore a length 3 match if it is too distant
			if (match_length == MIN_MATCH && state->ds.strstart - state->ds.match_start > TOO_FAR)
			{
				// If prev_match is also MIN_MATCH, match_start is garbage
				// but we will ignore the current match anyway
				match_length = MIN_MATCH - 1;
			}
		}

		// If there was a match at the previous step and the current
		// match is not better, output the previous match
		if (state->ds.prev_length >= MIN_MATCH && match_length <= state->ds.prev_length)
		{
			unsigned	max_insert;

			max_insert = state->ds.strstart + state->ds.lookahead - MIN_MATCH;
			check_match(state, state->ds.strstart - 1, prev_match, state->ds.prev_length);
			flush = ct_tally(state, state->ds.strstart - 1 - prev_match, state->ds.prev_length - MIN_MATCH);

			// Insert in hash table all strings up to the end of the match.
			// strstart-1 and strstart are already inserted
			state->ds.lookahead -= state->ds.prev_length - 1;
			state->ds.prev_length -= 2;
			do
			{
				if (++state->ds.strstart <= max_insert)
				{
					INSERT_STRING(state->ds.strstart, hash_head);
					// strstart never exceeds WSIZE-MAX_MATCH, so there are always MIN_MATCH bytes ahead
				}
			} while (--state->ds.prev_length);
			++state->ds.strstart;
			match_available = 0;
			match_length = MIN_MATCH - 1;

			if (flush)
			{
				flush_block(state, state->ds.block_start >= 0 ? (char *)&state->ds.window[(unsigned)state->ds.block_start] :
					0, (long)(state->ds.strstart - state->ds.block_start), 0);
				if (state->tzip->lasterr) goto bad;
				state->ds.block_start = state->ds.strstart;
			}
		}
		else if (match_available)
		{
			// If there was no match at the previous position, output a
			// single literal. If there was a match but the current match
			// is longer, truncate the previous match to a single literal
			if (ct_tally(state, 0, state->ds.window[state->ds.strstart - 1]))
			{
				flush_block(state, state->ds.block_start >= 0 ? (char *)&state->ds.window[(unsigned)state->ds.block_start] :
					0, (long)state->ds.strstart - state->ds.block_start, 0);
				if (state->tzip->lasterr) goto bad;
				state->ds.block_start = state->ds.strstart;
			}

			++state->ds.strstart;
			--state->ds.lookahead;
		}
		else
		{
			// There is no previous match to compare with, wait for the next step to decide
			match_available = 1;
			++state->ds.strstart;
			--state->ds.lookahead;
		}
		//		Assert(state,strstart <= isize && lookahead <= isize, "a bit too far");

		// Make sure that we always have enough lookahead, except
		// at the end of the input file. We need MAX_MATCH bytes
		// for the next match, plus MIN_MATCH bytes to insert the
		// string following the next match.
		if (state->ds.lookahead < MIN_LOOKAHEAD)
		{
			fill_window(state);
			if (state->tzip->lasterr)
			bad:			return;
		}
	}

	if (match_available) ct_tally(state, 0, state->ds.window[state->ds.strstart - 1]);

	// EOF
	flush_block(state, state->ds.block_start >= 0 ? (char *)&state->ds.window[(unsigned)state->ds.block_start] :
		0, (long)state->ds.strstart - state->ds.block_start, 1);

}











// ========================== Headers ========================

// Writes an extended local header to destination zip. Returns a ZE_ code

static void putextended(TZIPFILEINFO *z, TZIP *tzip)
{
	writeDestShort(tzip, EXTLOCSIG);
	writeDestShort(tzip, EXTLOCSIG >> 16);
	writeDestShort(tzip, z->crc);
	writeDestShort(tzip, z->crc >> 16);
	if (z->siz >= ULONG_MAX)
	{
		writeDestShort(tzip, -1);
		writeDestShort(tzip, -1);
	}
	else
	{
		writeDestShort(tzip, (DWORD)z->siz);
		writeDestShort(tzip, (DWORD)(z->siz >> 16));
	}
	if (z->len >= ULONG_MAX)
	{
		writeDestShort(tzip, -1);
		writeDestShort(tzip, -1);
	}
	else
	{
		writeDestShort(tzip, (DWORD)z->len);
		writeDestShort(tzip, (DWORD)(z->len >> 16));
	}
}

static void putpartial(TZIPFILEINFO *z, TZIP *tzip)
{
	writeDestShort(tzip, z->tim);
	writeDestShort(tzip, z->tim >> 16);
	writeDestShort(tzip, z->crc);
	writeDestShort(tzip, z->crc >> 16);
	if (z->siz >= ULONG_MAX)
	{
		writeDestShort(tzip, -1);
		writeDestShort(tzip, -1);
	}
	else
	{
		writeDestShort(tzip, (DWORD)z->siz);
		writeDestShort(tzip, (DWORD)(z->siz >> 16));
	}
	if (z->len >= ULONG_MAX)
	{
		writeDestShort(tzip, -1);
		writeDestShort(tzip, -1);
	}
	else
	{
		writeDestShort(tzip, (DWORD)z->len);
		writeDestShort(tzip, (DWORD)(z->len >> 16));
	}
	writeDestShort(tzip, z->nam);
}

// Writes a local header to destination zip. Return a ZE_ error code.

static void putlocal(TZIPFILEINFO *z, TZIP *tzip)
{
	// GZIP format?
	if (tzip->flags & TZIP_OPTION_GZIP)
	{
		writeDestShort(tzip, 0x8B1F);
		writeDestShort(tzip, 0x0808);
		writeDestShort(tzip, z->tim);
		writeDestShort(tzip, z->tim >> 16);
		writeDestShort(tzip, 0x0B02);		// was 0xFF02
		writeDestination(tzip, z->iname, z->nam + 1);
	}

	// PkZip format
	else
	{
		writeDestShort(tzip, LOCSIG);
		writeDestShort(tzip, LOCSIG >> 16);
		writeDestShort(tzip, (USH)45);		// Needs PKUNZIP 4.5 to unzip it
		writeDestShort(tzip, z->lflg);
		writeDestShort(tzip, z->how);
		//putpartial(z, tzip);
		writeDestShort(tzip, z->tim);
		writeDestShort(tzip, z->tim >> 16);
		writeDestShort(tzip, z->crc);
		writeDestShort(tzip, z->crc >> 16);
		writeDestShort(tzip, -1);
		writeDestShort(tzip, -1);
		writeDestShort(tzip, -1);
		writeDestShort(tzip, -1);
		writeDestShort(tzip, z->nam);
		writeDestShort(tzip, z->ext + 20);
		writeDestination(tzip, z->iname, z->nam);
		
		writeDestShort(tzip, 1);
		writeDestShort(tzip, 16);
		writeDestShort(tzip, (DWORD)z->len & 0xFFFFFFFF);
		writeDestShort(tzip, (DWORD)((z->len >> 16) & 0xFFFFFFFF));
		writeDestShort(tzip, (DWORD)(z->len >> 32));
		writeDestShort(tzip, (DWORD)(z->len >> 48));
		writeDestShort(tzip, (DWORD)z->siz & 0xFFFFFFFF);
		writeDestShort(tzip, (DWORD)((z->siz >> 16) & 0xFFFFFFFF));
		writeDestShort(tzip, (DWORD)(z->siz >> 32));
		writeDestShort(tzip, (DWORD)(z->siz >> 48));
		if (z->ext) writeDestination(tzip, z->extra, z->ext);
	}
}

/********************* addCentral() ********************
* Writes the ZIP file's central directory.
*
* NOTE: Once the central directory is written, no
* further files can be added.
*/

static void addCentral(register TZIP *tzip)
{
	// If there was an error adding files, then don't write the Central directory
	if (!tzip->lasterr && !(tzip->flags & TZIP_DONECENTRALDIR))
	{
		ULONGLONG	numentries = 0;
		ULONGLONG	pos_at_start_of_central;

		{
			register TZIPFILEINFO	*zfi;

			pos_at_start_of_central = tzip->writ;
			for (zfi = tzip->zfis; zfi;)
			{
				TZIPFILEINFO	*zfinext;

				// Write this TZIPFILEINFO entry to the Central directory
				writeDestShort(tzip, CENSIG);
				writeDestShort(tzip, CENSIG >> 16);
				writeDestShort(tzip, (USH)45);		// zip 4.5
				writeDestShort(tzip, (USH)45);			// Needs PKUNZIP 4.5 to unzip it
				writeDestShort(tzip, zfi->flg);
				writeDestShort(tzip, zfi->how);
				putpartial(zfi, tzip);
				writeDestShort(tzip, zfi->cext + (zfi->off >= ULONG_MAX || zfi->len >= ULONG_MAX || zfi->siz >= ULONG_MAX ? 4 : 0) 
								+ (zfi->off >= ULONG_MAX ? 8 : 0) + (zfi->siz >= ULONG_MAX ? 8 : 0) + (zfi->len >= ULONG_MAX ? 8 : 0));
				writeDestShort(tzip, 0);
				writeDestShort(tzip, zfi->dsk);
				writeDestShort(tzip, zfi->att);
				writeDestShort(tzip, zfi->atx);
				writeDestShort(tzip, zfi->atx >> 16);
				if (zfi->off >= ULONG_MAX)
				{
					writeDestShort(tzip, -1);
					writeDestShort(tzip, -1);
				}
				else
				{
					writeDestShort(tzip, (DWORD)zfi->off & 0xFFFFFFFF);
					writeDestShort(tzip, (DWORD)(zfi->off >> 16));
				}
				writeDestination(tzip, zfi->iname, zfi->nam);
				if (zfi->off >= ULONG_MAX || zfi->len >= ULONG_MAX || zfi->siz >= ULONG_MAX)
				{
					writeDestShort(tzip, 1);
					writeDestShort(tzip, (zfi->off >= ULONG_MAX ? 8 : 0) + (zfi->siz >= ULONG_MAX ? 8 : 0) + (zfi->len >= ULONG_MAX ? 8 : 0));
					if (zfi->len >= ULONG_MAX)
					{
						writeDestShort(tzip, (DWORD)zfi->len & 0xFFFFFFFF);
						writeDestShort(tzip, (DWORD)(zfi->len >> 16));
						writeDestShort(tzip, (DWORD)(zfi->len >> 32));
						writeDestShort(tzip, (DWORD)(zfi->len >> 48));
					}
					if (zfi->siz >= ULONG_MAX)
					{
						writeDestShort(tzip, (DWORD)zfi->siz & 0xFFFFFFFF);
						writeDestShort(tzip, (DWORD)(zfi->siz >> 16));
						writeDestShort(tzip, (DWORD)(zfi->siz >> 32));
						writeDestShort(tzip, (DWORD)(zfi->siz >> 48));
					}
					if (zfi->off >= ULONG_MAX)
					{
						writeDestShort(tzip, (DWORD)zfi->off & 0xFFFFFFFF);
						writeDestShort(tzip, (DWORD)(zfi->off >> 16));
						writeDestShort(tzip, (DWORD)(zfi->off >> 32));
						writeDestShort(tzip, (DWORD)(zfi->off >> 48));
					}
				}
				if (zfi->cext) writeDestination(tzip, zfi->cextra, zfi->cext);

				// Update count of bytes written
				tzip->writ += 4 + CENHEAD + (unsigned int)zfi->nam + (unsigned int)zfi->cext 
								+ (zfi->off >= ULONG_MAX || zfi->len >= ULONG_MAX || zfi->siz >= ULONG_MAX ? 4 : 0) 
								+ (zfi->off >= ULONG_MAX ? 8 : 0) + (zfi->siz >= ULONG_MAX ? 8 : 0) + (zfi->len >= ULONG_MAX ? 8 : 0);

				// Another entry added
				++numentries;

				// Free the TZIPFILEINFO now that we've written it out to the Central directory
				zfinext = zfi->nxt;
				GlobalFree(zfi);
				zfi = zfinext;
			}
		}
		if (tzip->writ >= ULONG_MAX)
		{
			ULONGLONG pos_at_start_of_end64 = tzip->writ + tzip->ooffset;
			// Write the Zip64 end of central directory record to the zip.
			writeDestShort(tzip, END64SIG);
			writeDestShort(tzip, END64SIG >> 16);
			writeDestShort(tzip, END64HEAD);
			writeDestShort(tzip, 0);
			writeDestShort(tzip, 0);
			writeDestShort(tzip, 0);
			writeDestShort(tzip, (USH)45);
			writeDestShort(tzip, (USH)45);
			writeDestShort(tzip, 0);
			writeDestShort(tzip, 0);
			writeDestShort(tzip, 0);
			writeDestShort(tzip, 0);
			writeDestShort(tzip, (DWORD)numentries & 0xFFFFFFFF);
			writeDestShort(tzip, (DWORD)(numentries >> 16));
			writeDestShort(tzip, (DWORD)(numentries >> 32));
			writeDestShort(tzip, (DWORD)(numentries >> 48));
			writeDestShort(tzip, (DWORD)numentries & 0xFFFFFFFF);
			writeDestShort(tzip, (DWORD)(numentries >> 16));
			writeDestShort(tzip, (DWORD)(numentries >> 32));
			writeDestShort(tzip, (DWORD)(numentries >> 48));
			ULONGLONG size_of_central = tzip->writ - pos_at_start_of_central;
			writeDestShort(tzip, (DWORD)size_of_central & 0xFFFFFFFF);
			writeDestShort(tzip, (DWORD)(size_of_central >> 16));
			writeDestShort(tzip, (DWORD)(size_of_central >> 32));
			writeDestShort(tzip, (DWORD)(size_of_central >> 48));
			ULONGLONG offset_central = pos_at_start_of_central + tzip->ooffset;
			writeDestShort(tzip, (DWORD)offset_central & 0xFFFFFFFF);
			writeDestShort(tzip, (DWORD)(offset_central >> 16));
			writeDestShort(tzip, (DWORD)(offset_central >> 32));
			writeDestShort(tzip, (DWORD)(offset_central >> 48));
			tzip->writ += 4 + 8 + END64HEAD + 0;

			// Write the Zip64 end of central directory locator to the zip.
			writeDestShort(tzip, END64LOCSIG);
			writeDestShort(tzip, END64LOCSIG >> 16);
			writeDestShort(tzip, 0);
			writeDestShort(tzip, 0);
			writeDestShort(tzip, (DWORD)pos_at_start_of_end64 & 0xFFFFFFFF);
			writeDestShort(tzip, (DWORD)(pos_at_start_of_end64 >> 16));
			writeDestShort(tzip, (DWORD)(pos_at_start_of_end64 >> 32));
			writeDestShort(tzip, (DWORD)(pos_at_start_of_end64 >> 48));
			writeDestShort(tzip, 1);
			writeDestShort(tzip, 0);
			tzip->writ += 4 + END64LOCHEAD + 0;

			// Write the end of the central-directory-data to the zip.
			writeDestShort(tzip, ENDSIG);
			writeDestShort(tzip, ENDSIG >> 16);
			writeDestShort(tzip, 0);
			writeDestShort(tzip, 0);
			if (numentries >= UCHAR_MAX)
			{
				writeDestShort(tzip, -1);
				writeDestShort(tzip, -1);
			}
			else
			{
				writeDestShort(tzip, (DWORD)numentries & 0xFFFFFFFF);
				writeDestShort(tzip, (DWORD)numentries & 0xFFFFFFFF);
			}
			if (size_of_central >= ULONG_MAX)
			{
				writeDestShort(tzip, -1);
				writeDestShort(tzip, -1);
			}
			else
			{
				writeDestShort(tzip, (DWORD)size_of_central & 0xFFFFFFFF);
				writeDestShort(tzip, (DWORD)(size_of_central >> 16));
			}
			if (offset_central >= ULONG_MAX)
			{
				writeDestShort(tzip, -1);
				writeDestShort(tzip, -1);
			}
			else
			{
				writeDestShort(tzip, (DWORD)offset_central & 0xFFFFFFFF);
				writeDestShort(tzip, (DWORD)(offset_central >> 16));
			}
			writeDestShort(tzip, 0);
			tzip->writ += 4 + ENDHEAD + 0;
		} 
		else
		{
			// Write the end of the central-directory-data to the zip.
			writeDestShort(tzip, ENDSIG);
			writeDestShort(tzip, ENDSIG >> 16);
			writeDestShort(tzip, 0);
			writeDestShort(tzip, 0);
			writeDestShort(tzip, (DWORD)numentries & 0xFFFFFFFF);
			writeDestShort(tzip, (DWORD)numentries & 0xFFFFFFFF);
			numentries = (DWORD)(tzip->writ - pos_at_start_of_central);
			writeDestShort(tzip, (DWORD)numentries & 0xFFFFFFFF);
			writeDestShort(tzip, (DWORD)(numentries >> 16));
			pos_at_start_of_central += (DWORD)tzip->ooffset;
			writeDestShort(tzip, (DWORD)pos_at_start_of_central & 0xFFFFFFFF);
			writeDestShort(tzip, (DWORD)(pos_at_start_of_central >> 16));
			writeDestShort(tzip, 0);
			tzip->writ += 4 + ENDHEAD + 0;
		}
		// Done writing the central dir. Flag this so that another call here does not
		// write another central dir
		tzip->flags |= TZIP_DONECENTRALDIR;
	}
}






















// ========================== Encryption ========================

#define DO1(buf)  crc = CRC32(crc, *buf++)
#define DO2(buf)  DO1(buf); DO1(buf)
#define DO4(buf)  DO2(buf); DO2(buf)
#define DO8(buf)  DO4(buf); DO4(buf)

static ULG crc32(ULG crc, const UCH *buf, DWORD len)
{
	if (!buf) return(0);
	crc = crc ^ 0xffffffffL;
	while (len >= 8) { DO8(buf); len -= 8; }
	if (len) do { DO1(buf); } while (--len);
	return crc ^ 0xffffffffL;  // (instead of ~c for 64-bit machines)
}

static void update_keys(unsigned long *keys, char c)
{
	keys[0] = CRC32(keys[0], c);
	keys[1] += keys[0] & 0xFF;
	keys[1] = keys[1] * 134775813L + 1;
	keys[2] = CRC32(keys[2], keys[1] >> 24);
}

static char decrypt_byte(unsigned long *keys)
{
	register unsigned temp = ((unsigned)keys[2] & 0xffff) | 2;
	return((char)(((temp * (temp ^ 1)) >> 8) & 0xff));
}

static char zencode(unsigned long *keys, char c)
{
	register int t = decrypt_byte(keys);
	update_keys(keys, c);
	return((char)(t^c));
}



























// =============== Source and Destination "file" handling ==============


/****************** writeDestShort() *******************
* Writes the specified short (from the memory buffer) to
* the current position of the ZIP destination (ZIP output
* file). Writes in Intel format.
*
* NOTE: If encryption is enabled, this encrypts the output
* data without altering the contents of the memory buffer.
*
* NOTE: If an error, then tzip->lasterr holds the
* (non-zero) error #. It is assumed that tzip->lasterr is
* cleared before any write to the destination. Otherwise
* writeDestination() does nothing.
*/

static void writeDestShort(register TZIP *tzip, DWORD data)
{
	unsigned char	bytes[2];

	// If a previous error, do not write anything more
	if (!tzip->lasterr)
	{
		bytes[0] = (unsigned char)(data & 0xff);
		bytes[1] = (unsigned char)((data >> 8) & 0xFF);

		// Encrypting data?
		if (tzip->flags & TZIP_ENCRYPT)
		{
			bytes[0] = zencode(tzip->keys, bytes[0]);
			bytes[1] = zencode(tzip->keys, bytes[1]);
		}

		// If writing to a destination memory buffer, make sure it is large enough, and then
		// copy data to it
		if (tzip->flags & TZIP_DESTMEMORY)
		{
			if (tzip->opos + 2 >= tzip->mapsize)
			{
				{
					tzip->lasterr = ZR_MEMSIZE;
					goto out;
				}
			}

			CopyMemory((unsigned char *)tzip->destination + tzip->opos, &bytes[0], 2);
			tzip->opos += 2;
		}

		// If writing to a handle, write out the data
		else
		{
			DWORD	written;

			if (!WriteFile(tzip->destination, &bytes[0], 2, &written, 0) || written != 2)
				tzip->lasterr = ZR_WRITE;
		}

		// Check for app abort
		if (tzip->flags & TZIP_OPTION_ABORT) tzip->lasterr = ZR_ABORT;
	}
out:
	return;
}





/****************** writeDestination() *******************
* Writes the specified bytes (from the memory buffer) to
* the current position of the ZIP destination (ZIP output
* file).
*
* NOTE: If encryption is enabled, this encrypts the output
* data without altering the contents of the memory buffer.
*
* NOTE: If an error, then tzip->lasterr holds the
* (non-zero) error #. It is assumed that tzip->lasterr is
* cleared before any write to the destination. Otherwise
* writeDestination() does nothing.
*/

static void writeDestination(register TZIP *tzip, const char *buf, DWORD size)
{
	// If a previous error, do not write anything more
	if (size && !tzip->lasterr)
	{
		// Encrypting data? If so, make sure the encrpytion buffer is
		// big enough, then encrpyt to that buffer and write out that
		if (tzip->flags & TZIP_ENCRYPT)
		{
			unsigned int i;

			if (tzip->encbuf && tzip->encbufsize < size)
			{
				GlobalFree(tzip->encbuf);
				tzip->encbuf = 0;
			}

			if (!tzip->encbuf)
			{
				tzip->encbufsize = size << 1;
				if (!(tzip->encbuf = (char *)GlobalAlloc(GMEM_FIXED, tzip->encbufsize)))
				{
					tzip->lasterr = ZR_NOALLOC;
					goto out;
				}
			}

			CopyMemory(tzip->encbuf, buf, size);
			for (i = 0; i < size; i++) tzip->encbuf[i] = zencode(tzip->keys, tzip->encbuf[i]);
			buf = tzip->encbuf;
		}

		// If writing to a destination memory buffer, make sure it is large enough, and then
		// copy data to it
		if (tzip->flags & TZIP_DESTMEMORY)
		{
			if (tzip->opos + size >= tzip->mapsize)
			{
				{
					tzip->lasterr = ZR_MEMSIZE;
					goto out;
				}
			}
			CopyMemory((unsigned char *)tzip->destination + tzip->opos, buf, size);
			tzip->opos += size;
		}

		// If writing to a handle, write out the data
		else
		{
			DWORD	written;

			if (!WriteFile(tzip->destination, buf, size, &written, 0) || written != size) tzip->lasterr = ZR_WRITE;
		}

		// Check for app abort
	out:	if (tzip->flags & TZIP_OPTION_ABORT) tzip->lasterr = ZR_ABORT;

	}
}





/****************** seekDestination() *******************
* Seeks to the specified position from the start of the
* ZIP destination (ZIP output file).
*/

static BOOL seekDestination(TZIP *tzip, LONGLONG pos)
{
	if (!(tzip->flags & TZIP_CANSEEK))
	{
	seekerr:
		tzip->lasterr = ZR_SEEK;
	bad:	return(0);
	}

	if (tzip->flags & TZIP_DESTMEMORY)
	{
		if (pos >= tzip->mapsize)
		{
			tzip->lasterr = ZR_MEMSIZE;
			goto bad;
		}
		tzip->opos = pos;
	}
	else
	{
		LONGLONG offset = pos + tzip->ooffset;
		if (offset > LONG_MAX)
		{
			LONG highpart = 0;
			highpart = offset >> 32;
			if (SetFilePointer(tzip->destination, (LONG)(offset & 0xFFFFFFFF), &highpart, FILE_BEGIN) == 0xFFFFFFFF)
				goto seekerr;
		}
		else
		{
			if (SetFilePointer(tzip->destination, (LONG)((pos + tzip->ooffset) & 0xFFFFFFFF), 0, FILE_BEGIN) == 0xFFFFFFFF)
				goto seekerr;
		}
	}
	return(1);
}






/****************** srcHandleInfo() *******************
* Fills in the TZIP with some information about an
* open source file handle.
*/

static DWORD srcHandleInfo(TZIP *tzip, ULONGLONG len, IZTIMES *times)
{
	DWORD	res;
	LARGE_INTEGER aOffset;
	aOffset.QuadPart = 0;
	if (SetFilePointerEx(tzip->source, aOffset, &aOffset, FILE_CURRENT))
	{
		tzip->ooffset = aOffset.QuadPart;
		tzip->flags |= TZIP_SRCCANSEEK;
		if ((res = getFileInfo(tzip, times)) != ZR_OK) return(res);
	}
	else
	{
		tzip->ooffset = 0;
		if (!len) len = (ULONGLONG)-1;		// Can't know size until at the end unless we were told explicitly!
		tzip->isize = len;
	}

	return(ZR_OK);
}





/****************** readFromSource() *******************
* Reads the specified bytes (from the current position of
* the source) and copies them to the current position of
* the specified memory buffer.
*
* RETURNS: The number of bytes read, or 0 if the end of
* the source.
*/

static unsigned readFromSource(register TZIP *tzip, char *buf, unsigned size)
{
	DWORD	bytes;

	// Check for app abort
	if (tzip->flags & TZIP_OPTION_ABORT)
	{
		tzip->lasterr = ZR_ABORT;
		goto bad;
	}

	// Memory buffer
	if (tzip->flags & TZIP_SRCMEMORY)
	{
		if (tzip->posin >= tzip->lenin) goto bad;	// end of input
		bytes = tzip->lenin - tzip->posin;
		if (bytes > size) bytes = size;
		CopyMemory(buf, (unsigned char *)tzip->source + tzip->posin, bytes);
		tzip->posin += bytes;
	}
	else
	{
		if (!ReadFile(tzip->source, buf, size, &bytes, 0))
		{
			tzip->lasterr = ZR_READ;
		bad:		return(0);
		}
	}

	tzip->totalRead += bytes;
	tzip->crc = crc32(tzip->crc, (UCH *)buf, bytes);
	return(bytes);
}





/****************** closeSource() *******************
* Closes the source (that we added to the ZIP file).
*/

static DWORD closeSource(register TZIP *tzip)
{
	register DWORD	ret;

	ret = ZR_OK;

	// See if we need to close the source
	if (tzip->flags & TZIP_SRCCLOSEFH)
		CloseHandle(tzip->source);
	tzip->flags &= ~TZIP_SRCCLOSEFH;

	// Check that we've accounted for all the source bytes (assuming we knew the size initially)
	if (tzip->isize != (ULONGLONG)-1 && tzip->isize != tzip->totalRead) ret = ZR_MISSIZE;

	// Update the count. The CRC has been done anyway, so we may as well
	tzip->isize = tzip->totalRead;

	return(ret);
}





/********************* ideflate() *********************
* Adds the current source to the ZIP file, using the
* deflate method.
*/

static void ideflate(TZIP *tzip, TZIPFILEINFO *zfi)
{
	register TSTATE		*state;

	// Make sure we have a TSTATE struct. It's a very big object -- 500k!
	// We allocate it on the heap, because PocketPC's stack breaks if
	// we try to put it all on the stack. It will be deleted when the
	// TZIP is deleted
	if ((state = tzip->state)) goto skip;

	if ((tzip->state = state = (TSTATE *)GlobalAlloc(GMEM_FIXED, sizeof(TSTATE))))
	{
		// Once-only initialization
		state->tzip = tzip;
		state->bs.out_buf = tzip->buf;
		state->bs.out_size = sizeof(tzip->buf);	// it used to be just 1024 bytes, not 16384 as here
		state->ts.static_dtree[0].dl.len = 0;	// this will make ct_init() realise it has to perform the init

		state->ts.l_desc.extra_bits = Extra_lbits;
		state->ts.d_desc.extra_bits = Extra_dbits;
		state->ts.bl_desc.extra_bits = Extra_blbits;
		state->ts.l_desc.dyn_tree = state->ts.dyn_ltree;
		state->ts.l_desc.static_tree = state->ts.static_ltree;
		state->ts.d_desc.dyn_tree = state->ts.dyn_dtree;
		state->ts.d_desc.static_tree = state->ts.static_dtree;
		state->ts.l_desc.extra_base = LITERALS + 1;
		state->ts.l_desc.elems = L_CODES;
		state->ts.d_desc.elems = D_CODES;
		state->ts.l_desc.max_length = state->ts.d_desc.max_length = MAX_BITS;
		state->ts.bl_desc.dyn_tree = state->ts.bl_tree;
		state->ts.bl_desc.static_tree = 0;
		state->ts.d_desc.extra_base = state->ts.bl_desc.extra_base = 0;
		state->ts.bl_desc.elems = BL_CODES;
		state->ts.bl_desc.max_length = MAX_BL_BITS;
		state->ts.l_desc.max_code = state->ts.d_desc.max_code = state->ts.bl_desc.max_code = 0;

	skip:	state->level = g->ZipCompressionLevel;
		//		state->seekable = tzip->flags & TZIP_SRCCANSEEK ? 1 : 0;

		//		state->ds.window_size =
		state->ts.last_lit = state->ts.last_dist = state->ts.last_flags =
			state->bs.out_offset = state->bs.bi_buf = state->bs.bi_valid = 0;
#ifdef _DEBUG
		state->bs.bits_sent = 0;
#endif
		ct_init(state, &zfi->att);

		lm_init(state, state->level, &zfi->flg);
		if (!tzip->lasterr)
		{
			// Compress the source into the zip
			//			if (state->level <= 3) deflate_fast(state);
			deflate(state);
		}
	}
	else
		tzip->lasterr = ZR_NOALLOC;
}





/********************** istore() *********************
* Adds the current source to the ZIP file, using the
* store method.
*/

static void istore(register TZIP *tzip)
{
	register DWORD		cin;

	// If a memory buffer, we can write out those bytes all at once
	if (tzip->flags & TZIP_SRCMEMORY)
	{
		cin = tzip->lenin;
		writeDestination(tzip, (const char*)tzip->source, cin);
		tzip->totalRead += cin;
		tzip->crc = crc32(tzip->crc, (UCH *)tzip->source, cin);
		tzip->posin += cin;
	}

	// Otherwise, we read in the source in 16384 byte chunks, and write it out.

	// Read upto the next 16384 bytes from the source. If no more, we're done
	else while ((cin = readFromSource(tzip, tzip->buf, 16384)) && !tzip->lasterr)
	{
		// Write out those bytes
		writeDestination(tzip, tzip->buf, cin);
	}

	// Increment total amount of bytes written (ie, the total size of the archive)
	tzip->csize += cin;
}






/******************** checkSuffix() *******************
* Checks if a filename has a ZIP extension.
*/

static BOOL checkSuffix(const char *fn)
{
	const char *ext;

	// Locate the extension
	ext = fn + lstrlenA(fn);
	while (ext > fn && *ext != '.') ext--;
	if (ext != fn || *ext == '.')
	{
		const char		*strs;

		// Check for a ZIP extension
		strs = &ZipSuffixes[0];
		while (*strs)
		{
			if (!lstrcmpiA(ext, strs)) return(1);
			strs += lstrlenA(strs) + 1;
		}
	}
	return(0);
}



#define ZIP_HANDLE		0x00000001
#define ZIP_FILENAME	0x00000002
#define ZIP_MEMORY		0x00000004
#define ZIP_FOLDER		0x00000008
#define ZIP_UNICODE		0x00000010
#define ZIP_RAW			0x00000020

/******************** hasExtension() *******************
* Returns 1 if there's an extension on a filename, or
* 0 if none.
*/

static BOOL hasExtension(const void * pchName, DWORD flags)
{
	if (flags & ZIP_UNICODE)
	{
		register const WCHAR		*pch;

		pch = (const WCHAR *)pchName + (lstrlenW((WCHAR *)pchName) - 1);

		// Back up to '.'
		while (*pch != '.')
		{
			if (pch <= (const WCHAR *)pchName || *pch == '/' || *pch == '\\') goto none;
			--pch;
		}
	}
	else
	{
		register const char		*pch;

		pch = (const char *)pchName + (lstrlenA((char *)pchName) - 1);

		// Back up to '.'
		while (*pch != '.')
		{
			if (pch <= (const char *)pchName || *pch == '/' || *pch == '\\')
			none:			return(0);
			--pch;
		}
	}

	return(1);
}



/************************* addSrc() ***********************
* Compresses a source to the ZIP file.
*
* tzip =	Handle to TZIP gotten via one of the
*			ZipCreate*() functions.
*
* destname = Desired name for the source when it is
*			added to the ZIP.
*
* src =	Handle to the source to be added to the ZIP. This
*			could be a pointer to a filename, a handle to an
*			open file, a pointer to a memory buffer
*			containing the contents to ZIP, or 0 if destname
*			is a directory.
*
* flags =	ZIP_HANDLE, ZIP_FILENAME, ZIP_MEMORY, or ZIP_FOLDER.
*			Also ZIP_UNICODE may be set.
*/

static DWORD addSrc(register TZIP *tzip, const void *destname, const void *src, DWORD len, DWORD flags)
{
	DWORD			passex;
	TZIPFILEINFO	*zfi;
	unsigned char	method;
	IZTIMES			times;

	if (IsBadReadPtr(tzip, 1))
		goto badargs;

	// Can't add any more if the app did a ZipGetMemory
	if (tzip->flags & TZIP_DONECENTRALDIR) return(ZR_ENDED);

	// Re-init some stuff potentially left over from a previous addSrc()
	tzip->ooffset = tzip->totalRead = tzip->csize = 0;
	tzip->crc = 0;
	tzip->flags &= ~(TZIP_SRCCANSEEK | TZIP_SRCCLOSEFH | TZIP_SRCMEMORY | TZIP_ENCRYPT);

	// ==================== Get the source (to compress to the ZIP) ===================

	switch (flags & ~(ZIP_UNICODE | ZIP_RAW))
	{
		// Zipping a file by name?
	case ZIP_FILENAME:
	{
		if (!src) goto badargs;
		if (flags & ZIP_UNICODE)
			tzip->source = CreateFileW((const WCHAR *)src, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
		else
			tzip->source = CreateFileA((const char *)src, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
		if (tzip->source == INVALID_HANDLE_VALUE)
			passex = ZR_NOFILE;
		else
		{
			if ((passex = srcHandleInfo(tzip, 0, &times)) == ZR_OK)
			{
				tzip->flags |= TZIP_SRCCLOSEFH;

				// If he didn't supply the destname, then use the same name as the source
				if (!destname) destname = src;

				goto chktime;
			}

			CloseHandle(tzip->source);
		}

	badopen:	return(passex);
	}

	// Zipping a file by its open handle?
	case ZIP_HANDLE:
	{
		if (!(tzip->source = (HANDLE)src) || (HANDLE)src == INVALID_HANDLE_VALUE) goto badargs;
		if ((passex = srcHandleInfo(tzip, len, &times)) != ZR_OK) goto badopen;
	chktime:	if (!(tzip->flags & TZIP_SRCCANSEEK)) goto gettime2;
		break;
	}

	// Zipping a folder name?
	case ZIP_FOLDER:
	{
		tzip->source = 0;
		tzip->isize = 0;
		times.attributes = 0x41C00010; // a readable writable directory, and again directory
		goto gettime;
	}

	// Zipping a memory buffer?
	default:
		//		case ZIP_MEMORY:
	{

		if (!src || !len) goto badargs;

		// Set the TZIP_SRCMEMORY flag because this source is from a memory buffer
		tzip->source = (HANDLE)src;
		tzip->isize = tzip->lenin = len;
		tzip->flags |= (TZIP_SRCMEMORY | TZIP_SRCCANSEEK);
		tzip->posin = 0;

	gettime2:	times.attributes = 0x80000000 | 0x01000000 | 0x00800000;	// just a normal file, readable/writeable
		// If there's not an extension on the name, assume it's
		// also executable
		if (!hasExtension(destname, flags))
			times.attributes = 0x80000000 | 0x01000000 | 0x00800000 | 0x00400000;

	gettime:	if (!(flags & ZIP_RAW))
	{
		WORD	dosdate, dostime;

		// Set the time stamps to the current time
		getNow(&times.atime, &dosdate, &dostime);
		times.mtime = times.atime;
		times.ctime = times.atime;
		times.timestamp = (WORD)dostime | (((DWORD)dosdate) << 16);
	}
	}
	}

	// ==================== Initialize the local header ===================
	// A zip "entry" consists of a local header (which includes the file name),
	// then the compressed data, and possibly an extended local header.

	// We need to allocate a TZIPFILEINFO + sizeof(EB_C_UT_SIZE)
	if (!(zfi = (TZIPFILEINFO *)GlobalAlloc(GMEM_FIXED, sizeof(TZIPFILEINFO) + EB_C_UT_SIZE)))
	{
		passex = ZR_NOALLOC;
		goto badout2;
	}
	ZeroMemory(zfi, sizeof(TZIPFILEINFO));

	if (flags & ZIP_RAW)
	{
		zfi->how = method = DEFLATE;
		zfi->len = zfi->siz = tzip->isize;
		zfi->off = tzip->writ;
		tzip->flags |= TZIP_OPTION_GZIP;
		goto compress;
	}

	// zip has its own notion of what filenames should look like, so we have to reformat
	// the name. First of all, ZIP does not support UNICODE names. Must be ANSI
	if (flags & ZIP_UNICODE)
	{
		zfi->nam = WideCharToMultiByte(CP_UTF8, 0, (const WCHAR *)destname, -1, zfi->iname, MAX_PATH, 0, 0);
		if (zfi->nam)
			zfi->nam--;
	}
	else
	{
		lstrcpyA(zfi->iname, (const char *)destname);
		zfi->nam = lstrlenA((const char *)destname);
	}
	if (!zfi->nam)
	{
		GlobalFree(zfi);
	badargs:
		passex = ZR_ARGS;
		goto badout2;
	}

	// Next we need to replace '\' with '/' chars
	{
		register char	*d;

		d = zfi->iname;
		while (*d)
		{
			if (*d == '\\') *d = '/';
			++d;
		}
	}

	// Determine whether app wants encryption, and whether we should use DEFLATE or STORE compression method
	passex = 0;
	zfi->flg = 8;		// 8 means 'there is an extra header'. Assume for the moment that we need it.
	method = STORE;
	zfi->tim = times.timestamp;
	zfi->atx = times.attributes;

	// Stuff the 'times' struct into zfi->extra
	{
		char xloc[EB_L_UT_SIZE];

		zfi->extra = xloc;
		zfi->ext = EB_L_UT_SIZE;
		zfi->cextra = (char *)zfi + sizeof(TZIPFILEINFO);
		zfi->cext = EB_C_UT_SIZE;
		xloc[0] = 'U';
		xloc[1] = 'T';
		xloc[2] = EB_UT_LEN(3);       // length of data part of e.f.
		xloc[3] = 0;
		xloc[4] = EB_UT_FL_MTIME | EB_UT_FL_ATIME | EB_UT_FL_CTIME;
		xloc[5] = (char)(times.mtime & 0xFF);
		xloc[6] = (char)((times.mtime >> 8) & 0xFF);
		xloc[7] = (char)((times.mtime >> 16) & 0xFF);
		xloc[8] = (char)(times.mtime >> 24);
		xloc[9] = (char)(times.atime & 0xFF);
		xloc[10] = (char)((times.atime >> 8) & 0xFF);
		xloc[11] = (char)((times.atime >> 16) & 0xFF);
		xloc[12] = (char)(times.atime >> 24);
		xloc[13] = (char)(times.ctime & 0xFF);
		xloc[14] = (char)((times.ctime >> 8) & 0xFF);
		xloc[15] = (char)((times.ctime >> 16) & 0xFF);
		xloc[16] = (char)(times.ctime >> 24);
		CopyMemory(zfi->cextra, zfi->extra, EB_C_UT_SIZE);
		zfi->cextra[EB_LEN] = EB_UT_LEN(1);
		zfi->cextra[EB_HEADSIZE] = EB_UT_FL_MTIME;
	}

	if (tzip->flags & TZIP_OPTION_GZIP)
	{
		method = DEFLATE;
		zfi->ext = 0;
	}
	else if (flags & ZIP_FOLDER)
	{
		// If a folder, make sure its name ends with a '/'. Also we'll use STORE
		// method, and no encryption
		if (zfi->iname[zfi->nam - 1] != '/') zfi->iname[zfi->nam++] = '/';
	}
	else
	{
		// If password encryption, then every tzip->isize and tzip->csize is 12 bytes bigger
		if (tzip->password)
		{
			passex = 12;
			zfi->flg = 9;	// 8 + 1 = 'password-encrypted', with extra header
		}

		// If zipping another ZIP file, then use STORE (since it's already compressed). Otherwise, DEFLATE
		if (!checkSuffix(zfi->iname)) method = DEFLATE;
	}

	// Initialize other info for writing the local header. For some of this information,
	// we have to make assumptions since we won't know the real information until after
	// compression is done
	zfi->lflg = zfi->flg;
	zfi->how = (USH)method;
	// If STORE method, we know the "compressed" size right now, so set it. DEFLATE method, we'll get it later
	if (method == STORE && tzip->isize != (ULONGLONG)-1) zfi->siz = tzip->isize + passex;
	zfi->len = tzip->isize;

	// Set the (byte) offset within the ZIP archive where this local record starts
	zfi->off = (tzip->writ + tzip->ooffset);

	// ============ Compress the source to the ZIP archive ================

	// Assume no error
	tzip->lasterr = ZR_OK;

	// Write the local header. Later, it will have to be rewritten, since we don't
	// know compressed size or crc yet. We get that info only after the compression
	// is done
	putlocal(zfi, tzip);
	if (tzip->lasterr != ZR_OK)
	{
	reterr:	passex = tzip->lasterr;
	badout:	GlobalFree(zfi);
	badout2:
		if (tzip->flags & TZIP_SRCCLOSEFH)
			CloseHandle(tzip->source);
		return(passex);
	}

	// Increment the amount of bytes written
	if (tzip->flags & TZIP_OPTION_GZIP)
		tzip->writ = 11 + (unsigned int)zfi->nam;
	else
	{
		tzip->writ += 4 + LOCHEAD + (unsigned int)zfi->nam + (unsigned int)zfi->ext + 20; // + 20 = Zip64

		// If needed, write the encryption header
		if (passex)
		{
			char		encbuf[12];
			int			i;
			const char	*cp;

			tzip->keys[0] = 305419896L;
			tzip->keys[1] = 591751049L;
			tzip->keys[2] = 878082192L;
			for (cp = tzip->password; cp && *cp; cp++) update_keys(tzip->keys, *cp);

			// generate some random bytes
			for (i = 0; i < 12; i++) encbuf[i] = (char)((rand() >> 7) & 0xff);
			encbuf[11] = (char)((zfi->tim >> 8) & 0xff);
			for (i = 0; i < 12; i++) encbuf[i] = zencode(tzip->keys, encbuf[i]);
			{
				writeDestination(tzip, encbuf, 12);
				if (tzip->lasterr != ZR_OK) goto reterr;
				tzip->writ += 12;
			}

			// Enable encryption for below
			tzip->flags |= TZIP_ENCRYPT;
		}
	}
compress:
	// Compress the source contents to the zip file
	if (tzip->source)
	{
		if (method == DEFLATE)
			ideflate(tzip, zfi);
		else
			istore(tzip);

		// Check for an error
		if (tzip->lasterr != ZR_OK) goto reterr;

		// No more encryption below
		tzip->flags &= ~TZIP_ENCRYPT;

		// Done with the source, so close it
		closeSource(tzip);

		tzip->writ += tzip->csize;
	}

	// For GZIP format, we allow only 1 file in the archive and no central directory
	if (tzip->flags & TZIP_OPTION_GZIP)
	{
		tzip->flags |= TZIP_DONECENTRALDIR;

		if (!(flags & ZIP_RAW))
		{
			// Write out the CRC and uncompressed size
			writeDestShort(tzip, tzip->crc);
			writeDestShort(tzip, tzip->crc >> 16);
			writeDestShort(tzip, (DWORD)tzip->totalRead & 0xFFFFFFFF);
			writeDestShort(tzip, (DWORD)(tzip->totalRead >> 16));
			if (tzip->lasterr != ZR_OK) goto reterr;
		}
	}
	else
	{
		{
			// Update the local header now that we have some final information about the compression
			unsigned char	first_header_has_size_right;

			first_header_has_size_right = (zfi->siz == tzip->csize + passex);

			// Update the CRC, compressed size, original size. Also save them for when we write the central directory
			zfi->crc = tzip->crc;
			zfi->siz = tzip->csize + passex;
			zfi->len = tzip->isize;

			// If we can seek in the source, seek to the local header and rewrite it with correct information
			if ((tzip->flags & TZIP_CANSEEK) && !passex)
			{
				// Update what compression method we used
				zfi->how = (USH)method;

				// Clear the extended local header flag and update the local header's flags
				if (!(zfi->flg & 1)) zfi->flg &= ~8;
				zfi->lflg = zfi->flg;

				// Rewrite the local header
				if (!seekDestination(tzip, zfi->off - tzip->ooffset))
				{
				badseek:		passex = ZR_SEEK;
					goto badout;
				}
				putlocal(zfi, tzip);
				if (tzip->lasterr != ZR_OK) goto reterr;
				if (!seekDestination(tzip, tzip->writ)) goto badseek;
			}

			// Otherwise, we put an updated (extended) header at the end
			else
			{
				// We can't change the compression method from our initial assumption
				if (zfi->how != (USH)method || (method == STORE && !first_header_has_size_right))
				{
					passex = ZR_NOCHANGE;
					goto badout;
				}
				putextended(zfi, tzip);
				if (tzip->lasterr != ZR_OK) goto reterr;

				tzip->writ += 16;

				// Store final flg for writing the central directory, just in case it was modified by inflate()
				zfi->flg = zfi->lflg;
			}
		}

		// ============ Book-keeping for Central Directory ================

		// Keep the ZIPFILEINFO, for when we write our end-of-zip directory later.
		// Link it at the end of the list
		if (!tzip->zfis) tzip->zfis = zfi;
		else
		{
			register TZIPFILEINFO *z;

			z = tzip->zfis;
			while (z->nxt) z = z->nxt;
			z->nxt = zfi;
		}
	}

	return(ZR_OK);
}






/*********************** searchDirW() *********************
* This recursively searches for files to add.
*
* path =	The full pathname of the folder to be searched.
*			Sub-directories within this folder are also searched.
*			This buffer must be of size MAX_PATH. Any ending '\'
*			char must be stripped off of the directory name.
*
* size =	The length (ie, number of chars in "path", not counting
*			the final null char).
*
* offset = WCHAR offset to the part of "path" that is skipped when
*			saving the name to the ZIP archive.
*
* RETURNS: 1 if continue or 0 to abort.
*
* NOTE: "path" may be altered upon return.
*/

static DWORD searchDirW(TZIP *tzip, WCHAR *path, unsigned long size, unsigned long offset, WIN32_FIND_DATAW *data)
{
	register HANDLE			fh;

	// Append "\*.*" to PathNameBuffer[]. We search all items in this one directory
	lstrcpyW(&path[size], &AllFilesStrW[0]);

	// Get the first item
	if ((fh = FindFirstFileW(&path[0], data)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			register unsigned long	len;

			// Append this item's name to the full pathname of this dir
			len = lstrlenW(&data->cFileName[0]);
			path[size] = '\\';
			lstrcpyW(&path[size + 1], &data->cFileName[0]);

			// Is this item itself a subdir? If so, we need to recursively search it
			if (data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// Skip the subdirs "." and ".."
				if (data->cFileName[0] != '.' || (data->cFileName[1] && data->cFileName[1] != '.'))
				{
					// Search this one subdir (and *its* subdirs recursively)
					if ((data->nFileSizeHigh = searchDirW(tzip, path, len + size + 1, offset, data)))
					bad:					return(data->nFileSizeHigh);
				}
			}
			else
			{
				// Zip this file
				if ((data->nFileSizeHigh = addSrc(tzip, (path + offset), path, 0, ZIP_FILENAME | ZIP_UNICODE))) goto bad;
			}

			// Another file or subdir?
		} while (FindNextFileW(fh, data));

		// Close this dir handle
		FindClose(fh);
	}

	return(0);
}

/*********************** searchDirA() *********************
* This recursively searches for files to add.
*
* path =	The full pathname of the folder to be searched.
*			Sub-directories within this folder are also searched.
*			This buffer must be of size MAX_PATH. Any ending '\'
*			char must be stripped off of the directory name.
*
* size =	The length (ie, number of chars in "path", not counting
*			the final null char).
*
* offset = Char offset to the part of "path" that is skipped when
*			saving the name to the ZIP archive.
*
* RETURNS: 1 if continue or 0 to abort.
*
* NOTE: "path" may be altered upon return.
*/

static DWORD searchDirA(TZIP *tzip, char *path, unsigned long size, unsigned long offset, WIN32_FIND_DATAA *data)
{
	register HANDLE			fh;

	// Append "\*.*" to PathNameBuffer[]. We search all items in this one directory
	lstrcpyA(&path[size], &AllFilesStrA[0]);

	// Get the first item
	if ((fh = FindFirstFileA(&path[0], data)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			register unsigned long	len;

			// Append this item's name to the full pathname of this dir
			len = lstrlenA(&data->cFileName[0]);
			path[size] = '\\';
			lstrcpyA(&path[size + 1], &data->cFileName[0]);

			// Is this item itself a subdir? If so, we need to recursively search it
			if (data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// Skip the subdirs "." and ".."
				if (data->cFileName[0] != '.' || (data->cFileName[1] && data->cFileName[1] != '.'))
				{
					// Search this one subdir (and *its* subdirs recursively)
					if ((data->nFileSizeHigh = searchDirA(tzip, path, len + size + 1, offset, data)))
					bad:					return(data->nFileSizeHigh);
				}
			}
			else
			{
				// Zip this file
				if ((data->nFileSizeHigh = addSrc(tzip, (path + offset), path, 0, ZIP_FILENAME))) goto bad;
			}

			// Another file or subdir?
		} while (FindNextFileA(fh, data));

		// Close this dir handle
		FindClose(fh);
	}

	return(0);
}






// ======================= Callable API ========================

DWORD WINAPI ZipAddFileA(HZIP tzip, const char *destname, const char *fn)
{
	return(addSrc((TZIP *)tzip, (void *)destname, (void *)fn, 0, ZIP_FILENAME));
}

DWORD WINAPI ZipAddFileW(HZIP tzip, const WCHAR *destname, const WCHAR *fn)
{
	return(addSrc((TZIP *)tzip, (void *)destname, (void *)fn, 0, ZIP_FILENAME | ZIP_UNICODE));
}

DWORD WINAPI ZipAddFileRawA(HZIP tzip, const char *fn)
{
	return(addSrc((TZIP *)tzip, (void *)&Extra_lbits[0], (void *)fn, 0, ZIP_FILENAME | ZIP_RAW));
}

DWORD WINAPI ZipAddFileRawW(HZIP tzip, const WCHAR *fn)
{
	return(addSrc((TZIP *)tzip, (void *)&Extra_lbits[0], (void *)fn, 0, ZIP_FILENAME | ZIP_UNICODE | ZIP_RAW));
}

DWORD WINAPI ZipAddBufferA(HZIP tzip, const char *destname, const void *src, DWORD len)
{
	return(addSrc((TZIP *)tzip, (void *)destname, src, len, ZIP_MEMORY));
}

DWORD WINAPI ZipAddBufferW(HZIP tzip, const WCHAR *destname, const void *src, DWORD len)
{
	return(addSrc((TZIP *)tzip, (void *)destname, src, len, ZIP_MEMORY | ZIP_UNICODE));
}

DWORD WINAPI ZipAddBufferRaw(HZIP tzip, const void *src, DWORD len)
{
	return(addSrc((TZIP *)tzip, (void *)&Extra_lbits[0], src, len, ZIP_MEMORY | ZIP_RAW));
}

DWORD WINAPI ZipAddHandleA(HZIP tzip, const char *destname, HANDLE h)
{
	return(addSrc((TZIP *)tzip, (void *)destname, h, 0, ZIP_HANDLE));
}

DWORD WINAPI ZipAddHandleW(HZIP tzip, const WCHAR *destname, HANDLE h)
{
	return(addSrc((TZIP *)tzip, (void *)destname, h, 0, ZIP_HANDLE | ZIP_UNICODE));
}

DWORD WINAPI ZipAddHandleRaw(HZIP tzip, HANDLE h)
{
	return(addSrc((TZIP *)tzip, (void *)&Extra_lbits[0], h, 0, ZIP_HANDLE | ZIP_RAW));
}

DWORD WINAPI ZipAddPipeA(HZIP tzip, const char *destname, HANDLE h, DWORD len)
{
	return(addSrc((TZIP *)tzip, (void *)destname, h, len, ZIP_HANDLE));
}

DWORD WINAPI ZipAddPipeRaw(HZIP tzip, HANDLE h, DWORD len)
{
	return(addSrc((TZIP *)tzip, (void *)&Extra_lbits[0], h, len, ZIP_HANDLE | ZIP_RAW));
}

DWORD WINAPI ZipAddPipeW(HZIP tzip, const WCHAR *destname, HANDLE h, DWORD len)
{
	return(addSrc((TZIP *)tzip, (void *)destname, h, len, ZIP_HANDLE | ZIP_UNICODE));
}

DWORD WINAPI ZipAddFolderA(HZIP tzip, const char *destname)
{
	return(addSrc((TZIP *)tzip, (void *)destname, 0, 0, ZIP_FOLDER));
}

DWORD WINAPI ZipAddFolderW(HZIP tzip, const WCHAR *destname)
{
	return(addSrc((TZIP *)tzip, (void *)destname, 0, 0, ZIP_FOLDER | ZIP_UNICODE));
}

static unsigned int replace_slashesA(char *to, const char *from)
{
	register char	chr;
	register char	*to2;

	to2 = to;
	do
	{
		chr = *(from)++;
		if (chr == '/') chr = '\\';
	} while ((*(to2)++ = chr));

	return (unsigned int)((--to2 - to));
}

static unsigned int replace_slashesW(short *to, const short *from)
{
	register short	chr;
	register short	*to2;

	to2 = to;
	do
	{
		chr = *(from)++;
		if (chr == '/') chr = '\\';
	} while ((*(to2)++ = chr));

	return (unsigned int)((--to2 - to) / sizeof(short));
}

DWORD WINAPI ZipAddDirA(HZIP tzip, const char *destname, DWORD offset)
{
	WIN32_FIND_DATAA	data;
	char				buffer[MAX_PATH];

	if (IsBadReadPtr(tzip, 1))	return(ZR_ARGS);

	// Copy dir name to buffer[] and trim off any trailing backslash
	if ((data.nFileSizeHigh = replace_slashesA(&buffer[0], destname)) &&
		buffer[data.nFileSizeHigh - 1] == '\\') buffer[--data.nFileSizeHigh] = 0;
	if (offset == (DWORD)-1) offset = data.nFileSizeHigh + 1;
	return(searchDirA((TZIP *)tzip, (char *)&buffer[0], data.nFileSizeHigh, offset, &data));
}

DWORD WINAPI ZipAddDirW(HZIP tzip, const WCHAR *destname, DWORD offset)
{
	WIN32_FIND_DATAW	data;
	WCHAR				buffer[MAX_PATH];
	if (IsBadReadPtr(tzip, 1))	return(ZR_ARGS);

	// Copy dir name to buffer[] and trim off any trailing backslash
	if ((data.nFileSizeHigh = replace_slashesW((short*)&buffer[0], (const short*)destname)) &&
		buffer[data.nFileSizeHigh - 1] == '\\') buffer[--data.nFileSizeHigh] = 0;
	if (offset == (DWORD)-1) offset = data.nFileSizeHigh + 1;
	return(searchDirW((TZIP *)tzip, (WCHAR *)&buffer[0], data.nFileSizeHigh, offset, &data));
}






DWORD WINAPI ZipFormatMessageA(DWORD code, char *buf, DWORD len)
{
	register const char	*str;
	register char 			*dest;

	str = &ErrorMsgs[0];
	while (code-- && *str) str += (strlen(str) + 1);
	if (!(*str)) str = &UnknownErr[0];
	code = 0;
	if (len)
	{
		dest = buf;
		do
		{
			if (!(dest[code] = str[code])) goto out;
		} while (++code < len);
		dest[--code] = 0;
	}
out:
	return(code);
}

DWORD WINAPI ZipFormatMessageW(DWORD code, WCHAR *buf, DWORD len)
{
	register const char	*str;
	register WCHAR 		*dest;

	str = &ErrorMsgs[0];
	while (code-- && *str) str += (strlen(str) + 1);
	if (!(*str)) str = &UnknownErr[0];
	code = 0;
	if (len)
	{
		dest = buf;
		do
		{
			if (!(dest[code] = (WCHAR)((unsigned char)str[code]))) goto out;
		} while (++code < len);
		dest[--code] = 0;
	}
out:
	return(code);
}






static void free_tzip(TZIP *tzip)
{
	// Free various buffers
	if (tzip->state) GlobalFree(tzip->state);
	if (tzip->encbuf) GlobalFree(tzip->encbuf);
	if (tzip->password) GlobalFree(tzip->password);

	// Free the TZIP itself
	GlobalFree(tzip);
}

/********************* ZipClose() **********************
* Closes the ZIP file that was created/opened with one
* of the ZipCreate* functions.
*/

DWORD WINAPI ZipClose(HZIP tzip)
{
	DWORD	result;

	// Make sure TZIP if valid
	if (IsBadReadPtr(tzip, 1))
		result = ZR_ARGS;
	else
	{
		result = ZR_OK;
		if (((TZIP *)tzip)->destination)
		{
			// If the directory wasn't already added via a call to ZipGetMemory, then we do it now
			addCentral((TZIP *)tzip);
			result = ((TZIP *)tzip)->lasterr;

			// If we created a memory-mapped file, free it now
			if (((TZIP *)tzip)->flags & TZIP_DESTMEMORY)
			{
				UnmapViewOfFile(((TZIP *)tzip)->destination);
				CloseHandle(((TZIP *)tzip)->memorymap);
			}

			// If we opened the handle, close it now
			if (((TZIP *)tzip)->flags & TZIP_DESTCLOSEFH)
				CloseHandle(((TZIP *)tzip)->destination);
		}

		// Free various buffers and the TZIP
		free_tzip((TZIP *)tzip);
	}

	return(result);
}








/********************* createZip() **********************
* Does all the real work for the ZipCreate* functions.
*/

static DWORD createZip(HZIP *zipHandle, void *z, DWORD len, DWORD flags, const char *pwd)
{
	register TZIP	*tzip;
	register DWORD	result;

	// Get a TZIP struct
	if (!(tzip = (TZIP *)GlobalAlloc(GMEM_FIXED, sizeof(TZIP))))
	{
		result = ZR_NOALLOC;
		goto done;
	}

	// Clear out all fields except the temporary buffer at the end
	ZeroMemory(tzip, sizeof(TZIP) - 16384);

	// If password is passed, make a copy of it
	if (pwd && *pwd)
	{
		if (!(tzip->password = (char *)GlobalAlloc(GMEM_FIXED, (lstrlenA(pwd) + 1)))) goto badalloc;
		lstrcpyA(tzip->password, pwd);
	}

	switch (flags & ~ZIP_UNICODE)
	{
		// Passed a file handle?
	case ZIP_HANDLE:
	{
		// Try to duplicate the handle. If successful, flag that we have to close it later
		if (DuplicateHandle(GetCurrentProcess(), z, GetCurrentProcess(), &tzip->destination, 0, 0, DUPLICATE_SAME_ACCESS))
			tzip->flags |= TZIP_DESTCLOSEFH;
		else
			tzip->destination = z;

		// See if we can seek on it. If so, get the current position
		if ((tzip->ooffset = SetFilePointer(tzip->destination, 0, 0, FILE_CURRENT)) != (DWORD)-1)
			tzip->flags |= TZIP_CANSEEK;
		else
			tzip->ooffset = 0;

		goto good;
	}

	// Passed a filename?
	case ZIP_FILENAME:
	{
		// Open the file and store the handle
		if (flags & ZIP_UNICODE)
			tzip->destination = CreateFileW((const WCHAR *)z, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		else
			tzip->destination = CreateFileA((const char *)z, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (tzip->destination == INVALID_HANDLE_VALUE)
		{
			result = ZR_NOFILE;
			goto freeit;
		}

		tzip->flags |= TZIP_CANSEEK | TZIP_DESTCLOSEFH;
		goto good;
	}

	// Passed a memory buffer where we store the zip file?
	case ZIP_MEMORY:
	{
		// Make sure he passed a non-zero length
		if (!len)
		{
			result = ZR_MEMSIZE;
			goto freeit;
		}

		// If he supplied the buffer, store it. Otherwise, we need to create some
		// memory-mapped file of the requested size
		if (!(tzip->destination = z))
		{
			if (!(tzip->memorymap = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, len, 0))) goto badalloc;
			if (!(tzip->destination = (HANDLE)MapViewOfFile(tzip->memorymap, FILE_MAP_ALL_ACCESS, 0, 0, len)))
			{
				CloseHandle(tzip->memorymap);
			badalloc:			result = ZR_NOALLOC;
				goto freeit;
			}
		}
		else if (IsBadReadPtr(z, len))
			goto argerr;
		tzip->flags |= (TZIP_CANSEEK | TZIP_DESTMEMORY);
		tzip->mapsize = len;

		// Success
	good:		result = ZR_OK;
		break;
	}

	default:
	{
	argerr:
		result = ZR_ARGS;
	freeit:		ZipClose((HZIP)tzip);
		tzip = 0;
	}
	}
done:
	// Return TZIP * to the caller
	*zipHandle = (HZIP)tzip;

	// Return error code to caller
	return(result);
}

/******************** ZipCreate*() ********************
* Creates/Opens a ZIP archive, in preparation of
* compressing files into it. The archive can be
* created on disk, or in memory, or to a pipe or
* an already open file.
*/

DWORD WINAPI ZipCreateHandle(HZIP *zipHandle, HANDLE h, const char *password)
{
	return(createZip(zipHandle, h, 0, ZIP_HANDLE, password));
}

DWORD WINAPI ZipCreateFileA(HZIP *zipHandle, const char *fn, const char *password)
{
	return(createZip(zipHandle, (void *)fn, 0, ZIP_FILENAME, password));
}

DWORD WINAPI ZipCreateFileW(HZIP *zipHandle, const WCHAR *fn, const char *password)
{
	return(createZip(zipHandle, (void *)fn, 0, ZIP_FILENAME | ZIP_UNICODE, password));
}

DWORD WINAPI ZipCreateBuffer(HZIP *zipHandle, void *z, DWORD len, const char *password)
{
	return(createZip(zipHandle, z, len, ZIP_MEMORY, password));
}








/*********************** ZipGetMemory() ***********************
* Called by an application to get the memory-mapped address
* created by ZipCreateBuffer() (after the app has called the
* ZipAdd* functions to zip up some contents into it, and before
* the app ZipClose()'s it).
*
*/

DWORD WINAPI ZipGetMemory(HZIP tzip, void **pbuf, DWORD *plen, HANDLE *base)
{
	DWORD	result;

	if (IsBadReadPtr(tzip, 1))
	{
		result = ZR_ARGS;
	zero:	*pbuf = 0;
		*plen = 0;
		if (base)
		{
			*base = 0;
			if (result != ZR_ARGS) free_tzip((TZIP *)tzip);
		}
	}
	else
	{
		// When the app calls ZipGetMemory, it has presumably finished
		// adding all of files to the ZIP. In any case, we have to write
		// the central directory now, otherwise the memory we return won't
		// be a complete ZIP file
		addCentral((TZIP *)tzip);
		result = ((TZIP *)tzip)->lasterr;

		if (!((TZIP *)tzip)->memorymap)
		{
			if (!result) result = ZR_NOTMMAP;
			goto zero;
		}
		else
		{
			// Return the memory buffer and size
			*pbuf = (void *)((TZIP *)tzip)->destination;
			*plen = (DWORD)((TZIP *)tzip)->writ;
		}

		// Does caller want everything freed except for the memory?
		if (base)
		{
			*base = ((TZIP *)tzip)->memorymap;
			free_tzip((TZIP *)tzip);
		}
	}

	return(result);
}





/*********************** ZipResetMemory() ***********************
* Called by an application to reset a memory-mapped HZIP for
* compressing a new ZIP file within it.
*/

DWORD WINAPI ZipResetMemory(HZIP tzip)
{
	register HANDLE		memorymap;

	if (IsBadReadPtr(tzip, 1) ||
		!(((TZIP *)tzip)->flags & TZIP_DESTMEMORY)) return(ZR_ARGS);

	// Did we create the memory?
	if ((memorymap = ((TZIP *)tzip)->memorymap))
	{
		UnmapViewOfFile(((TZIP *)tzip)->destination);
		if (!(((TZIP *)tzip)->destination = (HANDLE)MapViewOfFile(memorymap, FILE_MAP_ALL_ACCESS, 0, 0, ((TZIP *)tzip)->mapsize)))
		{
			CloseHandle(memorymap);
			free_tzip((TZIP *)tzip);
			return(ZR_NOALLOC);
		}
	}

	// Reset certain fields of the TZIP
	((TZIP *)tzip)->lasterr = 0;
	((TZIP *)tzip)->opos = ((TZIP *)tzip)->writ = 0;
	((TZIP *)tzip)->state->ts.static_dtree[0].dl.len = 0;
	return(ZR_OK);
}





/*********************** ZipOptions() ***********************
* Called by an application to reset a memory-mapped HZIP for
* compressing a new ZIP file within it.
*/

DWORD WINAPI ZipOptions(HZIP tzip, DWORD flags)
{
	if (IsBadReadPtr(tzip, 1) ||
		(flags & ~(TZIP_OPTION_GZIP | TZIP_OPTION_ABORT))) return(ZR_ARGS);
	((TZIP *)tzip)->flags |= flags;
	return(ZR_OK);
}