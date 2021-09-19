/*
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

#include "pch.h" // pre-compiled headers
#include "globaldata.h"
#ifdef _USRDLL


// Naveen v1. DllMain() - puts hInstance into struct nameHinstanceP 
//                        so it can be passed to OldWinMain()
// hInstance is required for script initialization 
// probably for window creation
// Todo: better cleanup in DLL_PROCESS_DETACH: windows, variables, no exit from script

BOOL APIENTRY DllMain(HMODULE hInstance,DWORD fwdReason, LPVOID lpvReserved)
 {
switch(fwdReason)
 {
 case DLL_PROCESS_ATTACH:
	 {
		g_hInstance = hInstance;
		break;
	 }
 case DLL_THREAD_ATTACH:
	 {
		break;
	 }
 case DLL_PROCESS_DETACH:
	 {
		 if (!lpvReserved)
		 {
			 HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, true, (DWORD) g_ahkThreads[0][5]);
			 if (hThread)
			 {
				 QueueUserAPC(&ThreadExitApp, hThread, 0);
				 CloseHandle(hThread);
			 }
			 if (g_KeyHistory)
				 free(g_KeyHistory);
			 if (g_hWinAPI)
			 {
				 free(g_hWinAPI);
				 free(g_hWinAPIlowercase);
			 }
			 Hotkey::AllDestructAndExit(0);
			 MemoryFreeLibrary(g_hCRYPT32);
		 }
		break;
	 }
 case DLL_THREAD_DETACH:
 break;
 }

 return(TRUE); // a FALSE will abort the DLL attach
 }
#endif