﻿/*
AutoHotkey

Copyright 2003-2009 Chris Mallett (support@autohotkey.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef SimpleHeap_h
#define SimpleHeap_h

// In a large script (200 KB of text) of a typical nature, using SimpleHeap rather than malloc() saves
// nearly 200 KB of memory as shown by Task Manager's "VM Size" column (2384 vs. 2580 KB).
// This is because many callers allocate chunks of memory that are very small on average.  If each
// such chunk were allocated with malloc (or worse, "new"), the percentage of system overhead
// compared to the memory actually used for such blocks would be enormous (perhaps 40 bytes of
// overhead for each malloc(), even if it's only for 3 or 4 bytes).  In addition, SimpleHeap improves
// performance to the extent that it is faster than malloc(), which it almost certainly is.  Finally,
// the OS's overall memory fragmentation may be reduced, especially if the app uses this class over
// a long period of time (hours or days).

// The size of each block in bytes.  Use a size that's a good compromise
// of avg. wastage vs. reducing memory fragmentation and overhead.
// Update: reduced it from 64K to 32K since many scripts tend to be small.
// Update 2 (fincs): Use twice as big blocks when compiling for Unicode because
// Unicode strings are twice as large.
#define BLOCK_SIZE (32 * 1024 * sizeof(TCHAR)) // Relied upon by Malloc() to be a multiple of 4.
// The maximum size for a new allocation when a new block must be created to fulfill it.
// Allocations under this size might cause wasted space at the end of the previous block.
#define MAX_ALLOC_IN_NEW_BLOCK (1024 * sizeof(TCHAR))

class SimpleHeap
{
private:
	char *mBlock; // This object's memory block.  Although private, its contents are public.
	char *mFreeMarker;  // Address inside the above block of the first unused byte.
	size_t mSpaceAvailable;
	UINT sBlockCount;
	SimpleHeap *sFirst, *sLast;  // The first and last objects in the linked list.
	char *sMostRecentlyAllocated; // For use with Delete().
	SimpleHeap *mNextBlock, *mPrevBlock;  // The object after this one and previous in the linked list; NULL if none.

	SimpleHeap *CreateBlock(size_t aSize);
	LPTSTR strDup(LPCTSTR aBuf, size_t aLength = -1); // Return a block of memory to the caller and copy aBuf into it.

public:
	SimpleHeap();
	~SimpleHeap();
	// Return a block of memory to the caller with aBuf copied into it.  Returns nullptr on failure.
	LPTSTR Malloc(LPCTSTR aBuf, size_t aLength = -1);
	
	// Return a block of memory to the caller with aBuf copied into it.  Terminates app on failure.
	LPTSTR Alloc(LPCTSTR aBuf, size_t aLength = -1);
	
	// Return a block of memory to the caller, or nullptr on failure.
	void* Malloc(size_t aSize);
	
	// Return a block of memory to the caller, or terminate app on failure.
	void* Alloc(size_t aSize);

	void Delete(void *aPtr);
	void DeleteAll();

	void CriticalFail();

	template<typename T>
	T* Alloc(size_t aCount = 1)
	{
		return (T *)Alloc(sizeof(T) * aCount);
	}
};

#endif
