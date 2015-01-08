#include "LiteUnzip.h"
// ======================== Function Declarations ======================

// Diagnostic functions
#ifndef NDEBUG
#define LuAssert(cond,msg)
#define LuTrace(x)
#define LuTracev(x)
#define LuTracevv(x)
#define LuTracec(c,x)
#define LuTracecv(c,x)
#endif
static int inflate_trees_bits (uInt *, uInt *, INFLATE_HUFT **, INFLATE_HUFT *, Z_STREAM *);
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
#define LOADIN {p = z->next_in; n = z->avail_in; b = s->bitb; k = s->bitk;}
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
static const uInt inflate_mask[17] = {0x0000,
    0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

static const uInt Fixed_bl = 9;
static const uInt Fixed_bd = 5;
static const INFLATE_HUFT Fixed_tl[] = {
	{{{96,7}},256}, {{{0,8}},80}, {{{0,8}},16}, {{{84,8}},115},
	{{{82,7}},31}, {{{0,8}},112}, {{{0,8}},48}, {{{0,9}},192},
	{{{80,7}},10}, {{{0,8}},96}, {{{0,8}},32}, {{{0,9}},160},
	{{{0,8}},0}, {{{0,8}},128}, {{{0,8}},64}, {{{0,9}},224},
	{{{80,7}},6}, {{{0,8}},88}, {{{0,8}},24}, {{{0,9}},144},
	{{{83,7}},59}, {{{0,8}},120}, {{{0,8}},56}, {{{0,9}},208},
	{{{81,7}},17}, {{{0,8}},104}, {{{0,8}},40}, {{{0,9}},176},
	{{{0,8}},8}, {{{0,8}},136}, {{{0,8}},72}, {{{0,9}},240},
	{{{80,7}},4}, {{{0,8}},84}, {{{0,8}},20}, {{{85,8}},227},
	{{{83,7}},43}, {{{0,8}},116}, {{{0,8}},52}, {{{0,9}},200},
	{{{81,7}},13}, {{{0,8}},100}, {{{0,8}},36}, {{{0,9}},168},
	{{{0,8}},4}, {{{0,8}},132}, {{{0,8}},68}, {{{0,9}},232},
	{{{80,7}},8}, {{{0,8}},92}, {{{0,8}},28}, {{{0,9}},152},
	{{{84,7}},83}, {{{0,8}},124}, {{{0,8}},60}, {{{0,9}},216},
	{{{82,7}},23}, {{{0,8}},108}, {{{0,8}},44}, {{{0,9}},184},
	{{{0,8}},12}, {{{0,8}},140}, {{{0,8}},76}, {{{0,9}},248},
	{{{80,7}},3}, {{{0,8}},82}, {{{0,8}},18}, {{{85,8}},163},
	{{{83,7}},35}, {{{0,8}},114}, {{{0,8}},50}, {{{0,9}},196},
	{{{81,7}},11}, {{{0,8}},98}, {{{0,8}},34}, {{{0,9}},164},
	{{{0,8}},2}, {{{0,8}},130}, {{{0,8}},66}, {{{0,9}},228},
	{{{80,7}},7}, {{{0,8}},90}, {{{0,8}},26}, {{{0,9}},148},
	{{{84,7}},67}, {{{0,8}},122}, {{{0,8}},58}, {{{0,9}},212},
	{{{82,7}},19}, {{{0,8}},106}, {{{0,8}},42}, {{{0,9}},180},
	{{{0,8}},10}, {{{0,8}},138}, {{{0,8}},74}, {{{0,9}},244},
	{{{80,7}},5}, {{{0,8}},86}, {{{0,8}},22}, {{{192,8}},0},
	{{{83,7}},51}, {{{0,8}},118}, {{{0,8}},54}, {{{0,9}},204},
	{{{81,7}},15}, {{{0,8}},102}, {{{0,8}},38}, {{{0,9}},172},
	{{{0,8}},6}, {{{0,8}},134}, {{{0,8}},70}, {{{0,9}},236},
	{{{80,7}},9}, {{{0,8}},94}, {{{0,8}},30}, {{{0,9}},156},
	{{{84,7}},99}, {{{0,8}},126}, {{{0,8}},62}, {{{0,9}},220},
	{{{82,7}},27}, {{{0,8}},110}, {{{0,8}},46}, {{{0,9}},188},
	{{{0,8}},14}, {{{0,8}},142}, {{{0,8}},78}, {{{0,9}},252},
	{{{96,7}},256}, {{{0,8}},81}, {{{0,8}},17}, {{{85,8}},131},
	{{{82,7}},31}, {{{0,8}},113}, {{{0,8}},49}, {{{0,9}},194},
	{{{80,7}},10}, {{{0,8}},97}, {{{0,8}},33}, {{{0,9}},162},
	{{{0,8}},1}, {{{0,8}},129}, {{{0,8}},65}, {{{0,9}},226},
	{{{80,7}},6}, {{{0,8}},89}, {{{0,8}},25}, {{{0,9}},146},
	{{{83,7}},59}, {{{0,8}},121}, {{{0,8}},57}, {{{0,9}},210},
	{{{81,7}},17}, {{{0,8}},105}, {{{0,8}},41}, {{{0,9}},178},
	{{{0,8}},9}, {{{0,8}},137}, {{{0,8}},73}, {{{0,9}},242},
	{{{80,7}},4}, {{{0,8}},85}, {{{0,8}},21}, {{{80,8}},258},
	{{{83,7}},43}, {{{0,8}},117}, {{{0,8}},53}, {{{0,9}},202},
	{{{81,7}},13}, {{{0,8}},101}, {{{0,8}},37}, {{{0,9}},170},
	{{{0,8}},5}, {{{0,8}},133}, {{{0,8}},69}, {{{0,9}},234},
	{{{80,7}},8}, {{{0,8}},93}, {{{0,8}},29}, {{{0,9}},154},
	{{{84,7}},83}, {{{0,8}},125}, {{{0,8}},61}, {{{0,9}},218},
	{{{82,7}},23}, {{{0,8}},109}, {{{0,8}},45}, {{{0,9}},186},
	{{{0,8}},13}, {{{0,8}},141}, {{{0,8}},77}, {{{0,9}},250},
	{{{80,7}},3}, {{{0,8}},83}, {{{0,8}},19}, {{{85,8}},195},
	{{{83,7}},35}, {{{0,8}},115}, {{{0,8}},51}, {{{0,9}},198},
	{{{81,7}},11}, {{{0,8}},99}, {{{0,8}},35}, {{{0,9}},166},
	{{{0,8}},3}, {{{0,8}},131}, {{{0,8}},67}, {{{0,9}},230},
	{{{80,7}},7}, {{{0,8}},91}, {{{0,8}},27}, {{{0,9}},150},
	{{{84,7}},67}, {{{0,8}},123}, {{{0,8}},59}, {{{0,9}},214},
	{{{82,7}},19}, {{{0,8}},107}, {{{0,8}},43}, {{{0,9}},182},
	{{{0,8}},11}, {{{0,8}},139}, {{{0,8}},75}, {{{0,9}},246},
	{{{80,7}},5}, {{{0,8}},87}, {{{0,8}},23}, {{{192,8}},0},
	{{{83,7}},51}, {{{0,8}},119}, {{{0,8}},55}, {{{0,9}},206},
	{{{81,7}},15}, {{{0,8}},103}, {{{0,8}},39}, {{{0,9}},174},
	{{{0,8}},7}, {{{0,8}},135}, {{{0,8}},71}, {{{0,9}},238},
	{{{80,7}},9}, {{{0,8}},95}, {{{0,8}},31}, {{{0,9}},158},
	{{{84,7}},99}, {{{0,8}},127}, {{{0,8}},63}, {{{0,9}},222},
	{{{82,7}},27}, {{{0,8}},111}, {{{0,8}},47}, {{{0,9}},190},
	{{{0,8}},15}, {{{0,8}},143}, {{{0,8}},79}, {{{0,9}},254},
	{{{96,7}},256}, {{{0,8}},80}, {{{0,8}},16}, {{{84,8}},115},
	{{{82,7}},31}, {{{0,8}},112}, {{{0,8}},48}, {{{0,9}},193},
	{{{80,7}},10}, {{{0,8}},96}, {{{0,8}},32}, {{{0,9}},161},
	{{{0,8}},0}, {{{0,8}},128}, {{{0,8}},64}, {{{0,9}},225},
	{{{80,7}},6}, {{{0,8}},88}, {{{0,8}},24}, {{{0,9}},145},
	{{{83,7}},59}, {{{0,8}},120}, {{{0,8}},56}, {{{0,9}},209},
	{{{81,7}},17}, {{{0,8}},104}, {{{0,8}},40}, {{{0,9}},177},
	{{{0,8}},8}, {{{0,8}},136}, {{{0,8}},72}, {{{0,9}},241},
	{{{80,7}},4}, {{{0,8}},84}, {{{0,8}},20}, {{{85,8}},227},
	{{{83,7}},43}, {{{0,8}},116}, {{{0,8}},52}, {{{0,9}},201},
	{{{81,7}},13}, {{{0,8}},100}, {{{0,8}},36}, {{{0,9}},169},
	{{{0,8}},4}, {{{0,8}},132}, {{{0,8}},68}, {{{0,9}},233},
	{{{80,7}},8}, {{{0,8}},92}, {{{0,8}},28}, {{{0,9}},153},
	{{{84,7}},83}, {{{0,8}},124}, {{{0,8}},60}, {{{0,9}},217},
	{{{82,7}},23}, {{{0,8}},108}, {{{0,8}},44}, {{{0,9}},185},
	{{{0,8}},12}, {{{0,8}},140}, {{{0,8}},76}, {{{0,9}},249},
	{{{80,7}},3}, {{{0,8}},82}, {{{0,8}},18}, {{{85,8}},163},
	{{{83,7}},35}, {{{0,8}},114}, {{{0,8}},50}, {{{0,9}},197},
	{{{81,7}},11}, {{{0,8}},98}, {{{0,8}},34}, {{{0,9}},165},
	{{{0,8}},2}, {{{0,8}},130}, {{{0,8}},66}, {{{0,9}},229},
	{{{80,7}},7}, {{{0,8}},90}, {{{0,8}},26}, {{{0,9}},149},
	{{{84,7}},67}, {{{0,8}},122}, {{{0,8}},58}, {{{0,9}},213},
	{{{82,7}},19}, {{{0,8}},106}, {{{0,8}},42}, {{{0,9}},181},
	{{{0,8}},10}, {{{0,8}},138}, {{{0,8}},74}, {{{0,9}},245},
	{{{80,7}},5}, {{{0,8}},86}, {{{0,8}},22}, {{{192,8}},0},
	{{{83,7}},51}, {{{0,8}},118}, {{{0,8}},54}, {{{0,9}},205},
	{{{81,7}},15}, {{{0,8}},102}, {{{0,8}},38}, {{{0,9}},173},
	{{{0,8}},6}, {{{0,8}},134}, {{{0,8}},70}, {{{0,9}},237},
	{{{80,7}},9}, {{{0,8}},94}, {{{0,8}},30}, {{{0,9}},157},
	{{{84,7}},99}, {{{0,8}},126}, {{{0,8}},62}, {{{0,9}},221},
	{{{82,7}},27}, {{{0,8}},110}, {{{0,8}},46}, {{{0,9}},189},
	{{{0,8}},14}, {{{0,8}},142}, {{{0,8}},78}, {{{0,9}},253},
	{{{96,7}},256}, {{{0,8}},81}, {{{0,8}},17}, {{{85,8}},131},
	{{{82,7}},31}, {{{0,8}},113}, {{{0,8}},49}, {{{0,9}},195},
	{{{80,7}},10}, {{{0,8}},97}, {{{0,8}},33}, {{{0,9}},163},
	{{{0,8}},1}, {{{0,8}},129}, {{{0,8}},65}, {{{0,9}},227},
	{{{80,7}},6}, {{{0,8}},89}, {{{0,8}},25}, {{{0,9}},147},
	{{{83,7}},59}, {{{0,8}},121}, {{{0,8}},57}, {{{0,9}},211},
	{{{81,7}},17}, {{{0,8}},105}, {{{0,8}},41}, {{{0,9}},179},
	{{{0,8}},9}, {{{0,8}},137}, {{{0,8}},73}, {{{0,9}},243},
	{{{80,7}},4}, {{{0,8}},85}, {{{0,8}},21}, {{{80,8}},258},
	{{{83,7}},43}, {{{0,8}},117}, {{{0,8}},53}, {{{0,9}},203},
	{{{81,7}},13}, {{{0,8}},101}, {{{0,8}},37}, {{{0,9}},171},
	{{{0,8}},5}, {{{0,8}},133}, {{{0,8}},69}, {{{0,9}},235},
	{{{80,7}},8}, {{{0,8}},93}, {{{0,8}},29}, {{{0,9}},155},
	{{{84,7}},83}, {{{0,8}},125}, {{{0,8}},61}, {{{0,9}},219},
	{{{82,7}},23}, {{{0,8}},109}, {{{0,8}},45}, {{{0,9}},187},
	{{{0,8}},13}, {{{0,8}},141}, {{{0,8}},77}, {{{0,9}},251},
	{{{80,7}},3}, {{{0,8}},83}, {{{0,8}},19}, {{{85,8}},195},
	{{{83,7}},35}, {{{0,8}},115}, {{{0,8}},51}, {{{0,9}},199},
	{{{81,7}},11}, {{{0,8}},99}, {{{0,8}},35}, {{{0,9}},167},
	{{{0,8}},3}, {{{0,8}},131}, {{{0,8}},67}, {{{0,9}},231},
	{{{80,7}},7}, {{{0,8}},91}, {{{0,8}},27}, {{{0,9}},151},
	{{{84,7}},67}, {{{0,8}},123}, {{{0,8}},59}, {{{0,9}},215},
	{{{82,7}},19}, {{{0,8}},107}, {{{0,8}},43}, {{{0,9}},183},
	{{{0,8}},11}, {{{0,8}},139}, {{{0,8}},75}, {{{0,9}},247},
	{{{80,7}},5}, {{{0,8}},87}, {{{0,8}},23}, {{{192,8}},0},
	{{{83,7}},51}, {{{0,8}},119}, {{{0,8}},55}, {{{0,9}},207},
	{{{81,7}},15}, {{{0,8}},103}, {{{0,8}},39}, {{{0,9}},175},
	{{{0,8}},7}, {{{0,8}},135}, {{{0,8}},71}, {{{0,9}},239},
	{{{80,7}},9}, {{{0,8}},95}, {{{0,8}},31}, {{{0,9}},159},
	{{{84,7}},99}, {{{0,8}},127}, {{{0,8}},63}, {{{0,9}},223},
	{{{82,7}},27}, {{{0,8}},111}, {{{0,8}},47}, {{{0,9}},191},
	{{{0,8}},15}, {{{0,8}},143}, {{{0,8}},79}, {{{0,9}},255}
};

static const INFLATE_HUFT Fixed_td[] = {
	{{{80,5}},1}, {{{87,5}},257}, {{{83,5}},17}, {{{91,5}},4097},
	{{{81,5}},5}, {{{89,5}},1025}, {{{85,5}},65}, {{{93,5}},16385},
	{{{80,5}},3}, {{{88,5}},513}, {{{84,5}},33}, {{{92,5}},8193},
	{{{82,5}},9}, {{{90,5}},2049}, {{{86,5}},129}, {{{192,5}},24577},
	{{{80,5}},2}, {{{87,5}},385}, {{{83,5}},25}, {{{91,5}},6145},
	{{{81,5}},7}, {{{89,5}},1537}, {{{85,5}},97}, {{{93,5}},24577},
	{{{80,5}},4}, {{{88,5}},769}, {{{84,5}},49}, {{{92,5}},12289},
	{{{82,5}},13}, {{{90,5}},3073}, {{{86,5}},193}, {{{192,5}},24577}
};

// Order of the bit length code lengths
static const uInt Border[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

// Tables for deflate from PKZIP's appnote.txt. 
static const uInt CpLens[31] = { // Copy lengths for literal codes 257..285
			3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
			35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
        // see note #13 above about 258
static const uInt CpLExt[31] = { // Extra bits for literal codes 257..285
			0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
			3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 112, 112}; // 112==invalid
static const uInt CpDist[30] = { // Copy offsets for distance codes 0..29
			1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
			257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
			8193, 12289, 16385, 24577};
static const uInt CpDExt[30] = { // Extra bits for distance codes 
			0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
			7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
			12, 12, 13, 13};

#pragma data_seg()












// ======================= Unix/DOS time-conversion stuff =====================

/***************** timet2filetime() ******************
 * Converts a Unix time_t to a Windows FILETIME.
 */

static void timet2filetime(FILETIME *ft, const lutime_t t)
{
	LONGLONG i;

	i = Int32x32To64(t, 10000000) + 116444736000000000;
	ft->dwLowDateTime = (DWORD)i;
	ft->dwHighDateTime = (DWORD)(i >>32);
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
	st.wYear = (WORD)(((dosdate>>9) & 0x7f) + 1980);
	st.wMonth = (WORD)((dosdate>>5) & 0xf);
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
	if (n > z->avail_out) n = z->avail_out;
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
		if (n > z->avail_out) n = z->avail_out;
		if (n && r == Z_BUF_ERROR) r = Z_OK;

		// update counters 
		z->avail_out -= n;
		z->total_out += n;

		// update check information 
//		if (!z->state->nowrap) z->adler = s->check = adler32(s->check, q, n);

		// copy
		if (n)
		{
			CopyMemory(p,q,n);
			p+=n;
			q+=n;
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
#ifndef NDEBUG
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
	for(;;)
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
#ifndef NDEBUG
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
#ifndef NDEBUG
				LuTracevv((stderr, "inflate:         end of block\n"));
#endif
				c->mode = WASH;
				break;
			}

			c->mode = BADCODE;        // invalid code 
#ifndef NDEBUG
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
#ifndef NDEBUG
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
#ifndef NDEBUG
			z->msg = (char*)"invalid distance code";
#endif
			r = Z_DATA_ERROR;
			LEAVE

		case DISTEXT:       // i: getting distance extra 

			j = c->sub.copy.get;
			NEEDBITS(j)
			c->sub.copy.dist += (uInt)b & inflate_mask[j];
			DUMPBITS(j)
#ifndef NDEBUG
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
#ifndef NDEBUG
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
	for(;;)
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
	#ifndef NDEBUG
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

	#ifndef NDEBUG
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
	#ifndef NDEBUG
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
#ifndef NDEBUG
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
#ifndef NDEBUG
					z->msg = (char*)"invalid stored block lengths";
#endif
					r = Z_DATA_ERROR;
					LEAVE
				}

				s->sub.left = (uInt)b & 0xffff;
				b = k = 0;                      // dump bits 
	#ifndef NDEBUG
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
	#ifndef NDEBUG
				LuTracev((stderr, "inflate:       stored end, %lu total out\n",  z->total_out + (q >= s->read ? q - s->read : (s->end - s->read) + (q - s->window))));
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
#ifndef NDEBUG
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
	#ifndef NDEBUG
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
	#ifndef NDEBUG
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
#ifndef NDEBUG
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
	#ifndef NDEBUG
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
	#ifndef NDEBUG
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
	uInt c[BMAX+1];               // bit length count table
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
	uInt			x[BMAX+1];		// bit offsets, then code stack 
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
					r.base = (uInt)(q - u[h-1] - j);   // offset to this table 
					u[h-1][j] = r;        // connect to last table 
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
#ifndef NDEBUG
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
#ifndef NDEBUG
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
#ifndef NDEBUG
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
#define UNGRAB {c=z->avail_in-n;c=(k>>3)<c?k>>3:c;n+=c;p-=c;k-=c<<3;}

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
#ifndef NDEBUG
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
#ifndef NDEBUG
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
#ifndef NDEBUG
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
							e = (uInt) (s->end - r);
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
#ifndef NDEBUG
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
#ifndef NDEBUG
					LuTracevv((stderr, t->base >= 0x20 && t->base < 0x7f ? "inflate:         * literal '%c'\n" : "inflate:         * literal 0x%02x\n", t->base));
#endif
					*q++ = (UCH)t->base;
					--m;
					break;
				}
			}
			else if (e & 32)
			{
#ifndef NDEBUG
				LuTracevv((stderr, "inflate:         * end of block\n"));
#endif
				UNGRAB
				UPDATE
				return Z_STREAM_END;
			}
			else
			{
#ifndef NDEBUG
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




// Does decryption of encrypted data
#define CRC32(c, b) (Crc_table[((int)(c)^(b))&0xff]^((c)>>8))
static void Uupdate_keys(unsigned long *keys, char c)
{
	keys[0] = CRC32(keys[0], c);
	keys[1] += keys[0] & 0xFF;
	keys[1] = keys[1]*134775813L +1;
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
#ifndef NDEBUG
					z->msg = (char*)"unknown compression method";
#endif
					z->state->sub.marker = 5;       // can't try inflateSync
					break;
				}

				if ((z->state->sub.method >> 4) + 8 > z->state->wbits)
				{
					z->state->mode = IM_BAD;
#ifndef NDEBUG
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
#ifndef NDEBUG
					z->msg = (char*)"incorrect header check";
#endif
					z->state->sub.marker = 5;       // can't try inflateSync 
					break;
				}
#ifndef NDEBUG
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
#ifndef NDEBUG
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
#ifndef NDEBUG
					z->msg = (char*)"incorrect data check";
#endif
					z->state->sub.marker = 5;       // can't try inflateSync 
					break;
				}

#ifndef NDEBUG
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

static int seekInZip(TUNZIP *tunzip, long offset, DWORD reference)
{
	if (!(tunzip->Flags & TZIP_ARCMEMORY))
	{
		if (reference == FILE_BEGIN) offset = SetFilePointer(tunzip->ArchivePtr, tunzip->InitialArchiveOffset + offset, 0, FILE_BEGIN);
		else if (reference == FILE_CURRENT) offset = SetFilePointer(tunzip->ArchivePtr, offset, 0, FILE_CURRENT);
//		else if (reference == FILE_END) offset = SetFilePointer(tunzip->ArchivePtr, offset, 0, FILE_END);
		if (offset == -1) return(ZR_SEEK);
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
	if (tunzip->ArchiveBufPos + toread > tunzip->ArchiveBufLen) toread = tunzip->ArchiveBufLen - tunzip->ArchiveBufPos;
	CopyMemory(ptr, (unsigned char *)tunzip->ArchivePtr + tunzip->ArchiveBufPos, toread);
	tunzip->ArchiveBufPos += toread;
	return(toread);
}






static unsigned char * reformat_short(unsigned char *ptr)
{
	register unsigned short	x;

	x = (unsigned short)*ptr;
	x |= (((unsigned short)*(ptr + 1)) << 8);
	*((unsigned short *)ptr)++ = x;
	return(ptr);
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
	register unsigned long	x;

	x = (unsigned long)*ptr;
	x |= (((unsigned long)*(ptr + 1)) << 8);
	x |= (((unsigned long)*(ptr + 2)) << 16);
	x |= (((unsigned long)*(ptr + 3)) << 24);
	*((unsigned long *)ptr)++ = x;
	return(ptr);
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
	DWORD			uSizeRead;
	DWORD			lSeek;

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
				tunzip->CurrentEntryInfo.offset = tunzip->CurrEntryPosInCentralDir + tunzip->ByteBeforeZipArchive;
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
				uSizeRead = SetFilePointer(tunzip->ArchivePtr, 0, 0, FILE_CURRENT);
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
				tunzip->CurrentEntryInfo.offset = tunzip->ArchiveBufPos;
				tunzip->CurrentEntryInfo.compressed_size = tunzip->ArchiveBufLen - tunzip->ArchiveBufPos - 8;
				CopyMemory((void *)tunzip->CurrentEntryInfo.crc, (unsigned char *)tunzip->ArchivePtr + tunzip->ArchiveBufLen - 8, 4);
				reformat_long((unsigned char *)&tunzip->CurrentEntryInfo.crc);
				CopyMemory((void *)tunzip->CurrentEntryInfo.uncompressed_size, (unsigned char *)tunzip->ArchivePtr + tunzip->ArchiveBufLen - 4, 4);
				reformat_long((unsigned char *)&tunzip->CurrentEntryInfo.uncompressed_size);
			}

			// Seek to name offset upon return
			seekInZip(tunzip, uSizeRead, FILE_BEGIN);
			goto out;
		}

		// Get the ZIP signature and check it
		uSizeRead = getArchiveLong(tunzip);
		if (uSizeRead == 0x02014b50 &&

			// Read in the ZIPENTRYINFO
			readFromZip(tunzip, &tunzip->CurrentEntryInfo, sizeof(ZIPENTRYINFO)) == sizeof(ZIPENTRYINFO))
		{
			unsigned char	*ptr;

			// Adjust the various fields to this CPU's byte order
			uSizeRead = ZIP_FIELDS_REFORMAT;
			ptr = (unsigned char *)&tunzip->CurrentEntryInfo;
			for (lSeek = 0; lSeek < NUM_FIELDS_REFORMAT; lSeek++)
			{
				if (0x00000001 & uSizeRead) ptr = reformat_long(ptr);
				else ptr = reformat_short(ptr);
				uSizeRead >>= 1;
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
					SecureZeroMemory(ptr, GlobalSize(ptr));
					GlobalFree(ptr);
				}
				break;

			case IBM_CODES:
				if ((ptr = entryReadVars->stream.state->blocks.sub.decode.codes))
				{
					SecureZeroMemory(ptr, GlobalSize(ptr));
					GlobalFree(ptr);
				}
		}

		if ((ptr = entryReadVars->stream.state->blocks.window))
		{
			SecureZeroMemory(ptr, GlobalSize(ptr));
			GlobalFree(ptr);
		}
		if ((ptr = entryReadVars->stream.state->blocks.hufts))
		{
			SecureZeroMemory(ptr, GlobalSize(ptr));
			GlobalFree(ptr);
		}
		SecureZeroMemory(entryReadVars->stream.state, GlobalSize(entryReadVars->stream.state));
		GlobalFree(entryReadVars->stream.state);

#ifndef NDEBUG
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
		SecureZeroMemory(tunzip->EntryReadVars.InputBuffer, GlobalSize(tunzip->EntryReadVars.InputBuffer));
		GlobalFree(tunzip->EntryReadVars.InputBuffer);
	}
	tunzip->EntryReadVars.InputBuffer = 0;

	// Free stuff allocated for decompressing in DEFLATE mode
	if (tunzip->EntryReadVars.stream.state) inflateEnd(&tunzip->EntryReadVars);
	tunzip->EntryReadVars.stream.state = 0;

	// No currently selected entry
	tunzip->CurrentEntryNum = (DWORD)-1;
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
	register DWORD	offset;

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

#ifndef NDEBUG
		LuTracev((stderr, "inflate: allocated\n"));
#endif
	}

	// If raw mode, app must supply the compressed and uncompressed sizes
	if (tunzip->Flags & TZIP_RAW)
	{
		tunzip->CurrentEntryInfo.compressed_size = ze->CompressedSize;
		tunzip->CurrentEntryInfo.uncompressed_size = ze->UncompressedSize;
	}

	// Initialize running counts
	tunzip->EntryReadVars.RemainingCompressed = tunzip->CurrentEntryInfo.compressed_size;
	tunzip->EntryReadVars.RemainingUncompressed = tunzip->CurrentEntryInfo.uncompressed_size;

	// Initialize for CRC checksum
	if (tunzip->CurrentEntryInfo.flag & 8) tunzip->EntryReadVars.CrcEncTest = (char)((tunzip->CurrentEntryInfo.dosDate >> 8) & 0xff);
	else tunzip->EntryReadVars.CrcEncTest = (char)(tunzip->CurrentEntryInfo.crc >> 24);

	if (tunzip->Flags & TZIP_GZIP)
		offset = tunzip->CurrentEntryInfo.offset;
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
				while(*cp) Uupdate_keys(&tunzip->EntryReadVars.Keys[0], *(cp)++);
			}
		}
		}

		{
		unsigned short	extra_offset;

		// Seek to the entry's LOCALHEADER->extra_field_size
		if (seekInZip(tunzip, tunzip->CurrentEntryInfo.offset + tunzip->ByteBeforeZipArchive + 28, FILE_BEGIN) ||
			!readFromZip(tunzip, &extra_offset, sizeof(unsigned short)))
		{
badseek:	tunzip->LastErr = ZR_READ;
			goto badout;
		}

		// Get the offset to where the entry's compressed data starts within the archive
//		tunzip->EntryReadVars.PosInArchive = (DWORD)tunzip->CurrentEntryInfo.offset + SIZEZIPLOCALHEADER + (DWORD)tunzip->CurrentEntryInfo.size_filename + (DWORD)extra_offset;
		offset = (DWORD)tunzip->CurrentEntryInfo.offset + SIZEZIPLOCALHEADER + (DWORD)tunzip->CurrentEntryInfo.size_filename + (DWORD)extra_offset;
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

DWORD readEntry(register TUNZIP *tunzip, void *buf, DWORD len)
{
	int							err;
	DWORD						iRead;

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
			DWORD	uReadThis;

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
			if (!readFromZip(tunzip, tunzip->EntryReadVars.InputBuffer, uReadThis))
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

				for (i=0; i < uReadThis; i++)
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
		register DWORD	uDoEncHead;

		uDoEncHead = tunzip->EntryReadVars.RemainingEncrypt;
		if (uDoEncHead > tunzip->EntryReadVars.stream.avail_in) uDoEncHead = tunzip->EntryReadVars.stream.avail_in;
		if (uDoEncHead)
		{
			char	bufcrc;

			bufcrc = tunzip->EntryReadVars.stream.next_in[uDoEncHead - 1];
			tunzip->EntryReadVars.RemainingUncompressed -= uDoEncHead;
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
			DWORD	uDoCopy;

			if (tunzip->EntryReadVars.stream.avail_out < tunzip->EntryReadVars.stream.avail_in)
				uDoCopy = tunzip->EntryReadVars.stream.avail_out;
			else
				uDoCopy = tunzip->EntryReadVars.stream.avail_in;
			CopyMemory(tunzip->EntryReadVars.stream.next_out, tunzip->EntryReadVars.stream.next_in, uDoCopy);

			tunzip->EntryReadVars.RunningCrc = ucrc32(tunzip->EntryReadVars.RunningCrc, tunzip->EntryReadVars.stream.next_out, uDoCopy);
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
			DWORD		uTotalOutBefore,uTotalOutAfter;
			const UCH	*bufBefore;
			DWORD		uOutThis;

			uTotalOutBefore = tunzip->EntryReadVars.stream.total_out;
			bufBefore = tunzip->EntryReadVars.stream.next_out;

			if ((err = inflate(&tunzip->EntryReadVars.stream, Z_SYNC_FLUSH)) && err != Z_STREAM_END)
			{
				tunzip->LastErr = ZR_FLATE;
				goto none;
			}

			uTotalOutAfter = tunzip->EntryReadVars.stream.total_out;
			uOutThis = uTotalOutAfter - uTotalOutBefore;

			tunzip->EntryReadVars.RunningCrc = ucrc32(tunzip->EntryReadVars.RunningCrc, bufBefore, uOutThis);

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

	if (flags & ZIP_UNICODE)
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
				return(setCurrentEntry(tunzip, ze, (flags & ZIP_UNICODE) | ZIP_ALREADYINIT));
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
	if (!(flags & ZIP_ALREADYINIT))
	{
		// Does caller want general information about the ZIP archive?
		if (ze->Index == (DWORD)-1)
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
		if (!(extra = GlobalAlloc(GMEM_FIXED, size)))
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

	if (flags & ZIP_UNICODE)
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
	if (!host || host==6 || host==10 || host==14)
		ze->Attributes = tunzip->CurrentEntryInfo.external_fa & (FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_ARCHIVE);
	else
	{
		ze->Attributes = FILE_ATTRIBUTE_ARCHIVE;
		if (tunzip->CurrentEntryInfo.external_fa & 0x40000000) ze->Attributes = FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_DIRECTORY;
		if (!(tunzip->CurrentEntryInfo.external_fa & 0x00800000)) ze->Attributes |= FILE_ATTRIBUTE_READONLY;
	}
	}

	// Copy sizes
	ze->CompressedSize = tunzip->CurrentEntryInfo.compressed_size;
	ze->UncompressedSize = tunzip->CurrentEntryInfo.uncompressed_size;

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

		etype[0] = extra[epos+0];
		etype[1] = extra[epos+1];
		etype[2] = 0;
		size = extra[epos+2];
		if (!lstrcmpA(etype, "UT"))
		{
			flags = extra[epos + 4];
			epos += 5;
			if ((flags & 1) && epos < tunzip->CurrentEntryInfo.size_file_extra)
			{
				lutime_t modifyTime = ((extra[epos+0])<<0) | ((extra[epos+1])<<8) |((extra[epos+2])<<16) | ((extra[epos+3])<<24);
				timet2filetime(&ze->ModifyTime, modifyTime);
				epos += 4;
			}

			if ((flags & 2) && epos < tunzip->CurrentEntryInfo.size_file_extra)
			{
				lutime_t accessTime = ((extra[epos+0])<<0) | ((extra[epos+1])<<8) |((extra[epos+2])<<16) | ((extra[epos+3])<<24);
				timet2filetime(&ze->AccessTime, accessTime);
				epos += 4;
			}

			if ((flags & 4) && epos < tunzip->CurrentEntryInfo.size_file_extra)
			{
				lutime_t createTime = ((extra[epos+0])<<0) | ((extra[epos+1])<<8) |((extra[epos+2])<<16) | ((extra[epos+3])<<24);
				timet2filetime(&ze->CreateTime, createTime);
				epos += 4;
			}

			break;
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

DWORD unzipEntry(register TUNZIP *tunzip, void *dst, ZIPENTRY *ze, DWORD flags)
{
	HANDLE		h;

	// Make sure TUNZIP if valid
	if (IsBadReadPtr(tunzip, 1) ||
		// Make sure we have a currently selected entry
		tunzip->CurrentEntryNum == (DWORD)-1)
	{
		return(ZR_ARGS);
	}

	// No error
	tunzip->LastErr = 0;

	// ============ Unzipping to memory ============
	if (flags & ZIP_MEMORY)
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
		if (flags & ZIP_FILENAME)
		{
			if (flags & ZIP_UNICODE) flags = createMultDirsW((WCHAR *)dst, 1);
			else  flags = createMultDirsA((char *)dst, 1);
			if (!flags) goto badf;
		}
		return(ZR_OK);
	}

	// Write the entry to a file/handle
	if (flags == ZIP_HANDLE)
		h = (HANDLE)dst;
	else
	{
		DWORD		res;

		// Create any needed directories
		if (flags & ZIP_UNICODE) res = createMultDirsW((WCHAR *)dst, 0);
		else res = createMultDirsA((char *)dst, 0);
		if (!res) goto badf;

		// Create the file to which we'll write the uncompressed entry
		if (flags & ZIP_UNICODE)
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
				register DWORD read;

				// Decompress the bytes into the input buffer. If EOF, then get out of this loop
				if (!(read = readEntry(tunzip, tunzip->OutBuffer, 16384))) break;

				// Write the bytes
				if (!WriteFile(h, tunzip->OutBuffer, read, &writ, 0) || writ != read)
					tunzip->LastErr = ZR_WRITE;
			}
		}

		// Set the file's timestamp
		if (!tunzip->LastErr)
			SetFileTime(h, &ze->CreateTime, &ze->AccessTime, &ze->ModifyTime); // may fail if it was a pipe

		// Close the file if we opened it above	
		if (flags != ZIP_HANDLE)
			CloseHandle(h);
	}
out:
	cleanupEntry(tunzip);
	return(tunzip->LastErr);
}







/********************* closeArchive() *****************
 * Closes the ZIP archive opened with openArchive().
 */

void closeArchive(register TUNZIP *tunzip)
{
	cleanupEntry(tunzip);
	if (tunzip->Flags & TZIP_ARCCLOSEFH)
		CloseHandle(tunzip->ArchivePtr);
	if (tunzip->Password)
	{
		SecureZeroMemory(tunzip->Password, GlobalSize(tunzip->Password));
		GlobalFree(tunzip->Password);
	}
	if (tunzip->OutBuffer)
	{
		SecureZeroMemory(tunzip->OutBuffer, GlobalSize(tunzip->OutBuffer));
		GlobalFree(tunzip->OutBuffer);
	}
	SecureZeroMemory(tunzip, GlobalSize(tunzip));
	GlobalFree(tunzip);
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

DWORD openArchive(HANDLE *ptr, void *z, DWORD len, DWORD flags, const char *pwd)
{
	register TUNZIP		*tunzip;
	DWORD				centralDirPos;

	// Get a TUNZIP
	if (!(tunzip = (TUNZIP *)GlobalAlloc(GMEM_FIXED, sizeof(TUNZIP))))
		return ZR_NOALLOC;
	ZeroMemory(tunzip, sizeof(TUNZIP) - MAX_PATH);

	// Store password if supplied
	if (pwd)
	{
		if (!(tunzip->Password = (unsigned char *)GlobalAlloc(GMEM_FIXED, lstrlenA(pwd) + 1)))
		{
			GlobalFree(tunzip);
			return flags;
		}
		lstrcpyA((char *)tunzip->Password, pwd);
	}

	// No currently selected entry
	tunzip->CurrentEntryNum = (DWORD)-1;

	// If buffer pointer is supplied, then he passed the ZIP archive
	if ((tunzip->ArchivePtr = z))
		tunzip->ArchiveBufLen = len;
	else
		return ZR_ARGS;
	tunzip->Flags = TZIP_ARCMEMORY;

	// If RAW flag, then it's assumed there is only one file in the archive, no central directory,
	// and not even any GZIP indentification ID3 tag compression seems to be this way. Sigh... Wish
	// some programmers put a little more thought into their dodgy file formats _before_ releasing
	// code
	ZeroMemory(&tunzip->CurrentEntryInfo, sizeof(ZIPENTRYINFO));
	tunzip->CurrentEntryInfo.compression_method = Z_DEFLATED;
	tunzip->CurrentEntryInfo.offset = tunzip->InitialArchiveOffset;
	tunzip->Flags |= TZIP_RAW;
	tunzip->CurrentEntryNum = 0;
	tunzip->ByteBeforeZipArchive = tunzip->InitialArchiveOffset;
	tunzip->LastErr = 0;
	tunzip->Flags |= TZIP_GZIP;
	tunzip->TotalEntries = 1;
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