#include "stdafx.h" // pre-compiled headers
#include "defines.h"
#include "application.h"
#include "globaldata.h"
#include "script.h"
#include "TextIO.h"
#include "script_object.h"

//
// Struct::Create - Called by BIF_ObjCreate to create a new object, optionally passing key/value pairs to set.
//

Struct *Struct::Create(ExprTokenType *aParam[], int aParamCount)
// This code is very similar to BIF_sizeof so should be maintained together
{
	int ptrsize = sizeof(UINT_PTR); // Used for pointers on 32/64 bit system
	int offset = 0;					// also used to calculate total size of structure
	int mod = 0;
	int arraydef = 0;				// count arraysize to update offset
	int unionoffset[16] = { 0 };			// backup offset before we enter union or structure
	int unionsize[16] = { 0 };				// calculate unionsize
	bool unionisstruct[16] = { 0 };			// updated to move offset for structure in structure
	int structalign[16] = { 0 };			// keep track of struct alignment
	int totalunionsize = 0;			// total size of all unions and structures in structure
	int uniondepth = 0;				// count how deep we are in union/structure
	int ispointer = 0;				// identify pointer and how deep it goes
	int aligntotal = 0;				// pointer alignment for total structure
	int thissize = 0;				// used to check if type was found in above array.
	int maxsize = 0;				// max size of union or struct
	int toalign = 0;					// custom alignment
	int thisalign = 0;

	// following are used to find variable and also get size of a structure defined in variable
	// this will hold the variable reference and offset that is given to size() to align if necessary in 64-bit
	ResultType Result = OK;
	ExprTokenType result_token, obj_token, this_token, ResultToken;
	ExprTokenType Var1,Var2,Var3,Var4,Var5;
	ExprTokenType *param[] = {&Var1,&Var2,&Var3,&Var4,&Var5};
	ExprTokenType VarObj1,VarObj2;
	ExprTokenType *paramObj[] = {&VarObj1,&VarObj2};
	Var1.symbol = SYM_VAR;
	Var2.symbol = SYM_INTEGER;
	Var3.symbol = SYM_INTEGER;
	Var4.symbol = SYM_INTEGER;
	Var5.symbol = SYM_OBJECT;

	// will hold pointer to structure definition string while we parse trough it
	TCHAR *buf;
	size_t buf_size;

	// Use enough buffer to accept any definition in a field.
	TCHAR tempbuf[LINE_SIZE]; // just in case if we have a long comment

	// definition and field name are same max size as variables
	// also add enough room to store pointers (**) and arrays [1000]
	// give more room to use local or static variable Function(variable)
	// Parameter passed to IsDefaultType needs to be ' Definition '
	// this is because spaces are used as delimiters ( see IsDefaultType function )
	TCHAR defbuf[MAX_VAR_NAME_LENGTH * 2 + 40] = _T(" UInt "); // Set default UInt definition
	TCHAR subdefbuf[MAX_VAR_NAME_LENGTH * 2 + 40];

	TCHAR keybuf[MAX_VAR_NAME_LENGTH + 40];
	// buffer for arraysize + 3 for bracket ] and terminating character
	TCHAR arrbuf[MAX_INTEGER_LENGTH + 3];

	LPTSTR bitfield = NULL;
	BYTE bitsize = 0;
	BYTE bitsizetotal = 0;
	LPTSTR isBit;

	FieldType *field;				// used to define a field
	// Structure object is saved in fixed order
	// insert_pos is simply increased each time
	// for loop will enumerate items in same order as it was created
	IndexType insert_pos = 0;
	Struct *obj;
	
	if (aParamCount > 4)
		obj = (Struct *)aParam[4]->object; // use given handle (for substruct)
	else
	{
		if (TokenToObject(*aParam[0]))
		{
			g_script.ScriptError(ERR_INVALID_STRUCT);
			return NULL;
		}

		obj = new Struct();
		obj->mHeap = HeapCreate(0, 0, 0);
		obj->mOwnHeap = true;

		// create new structure object and Heap
		if (!obj || !obj->mHeap)
		{
			if (obj)
				obj->Release();
			g_script.ScriptError(ERR_OUTOFMEM, TokenToString(*aParam[0]));
			return NULL;
		}
		BIF_sizeof(Result,result_token, aParam, 1);
		if (result_token.symbol != SYM_INTEGER)
		{	// could not resolve structure
			g_script.ScriptError(ERR_INVALID_STRUCT, TokenToString(*aParam[0]));
			return NULL;
		}
		obj->mMain = (Struct *)Object::Create();
		if (!obj->mMain)
		{
			g_script.ScriptError(ERR_OUTOFMEM, TokenToString(*aParam[0]));
			return NULL;
		}
		obj->mStructSize = (int)result_token.value_int64;
		if (aParamCount > 1 && TokenToInt64(*aParam[1]))
			// second parameter exist and it is digit assumme this is new pointer for our structure
			// caller must ensure zero initialize memory
			obj->mStructMem = (UINT_PTR *)TokenToInt64(*aParam[1]);
		else // no pointer given so allocate memory and fill memory with 0
			// setting the memory after parsing definition saves a call to BIF_sizeof
			obj->mStructMem = (UINT_PTR *)HeapAlloc(obj->mHeap, HEAP_ZERO_MEMORY, obj->mStructSize);
	}

	VarObj1.symbol = SYM_STRING;
	VarObj2.symbol = SYM_OBJECT;
	this_token.symbol = SYM_OBJECT;
	this_token.object = obj->mMain;
	// Set buf to beginning of structure definition
	buf = TokenToString(*aParam[0]);
	
	toalign = ATOI(buf);
	TCHAR alignbuf[MAX_INTEGER_LENGTH];
	if (*(buf + _tcslen(ITOA(toalign, alignbuf))) != ':' || toalign <= 0)
	{
		if (aParamCount == 5)
		{
			toalign = (int)TokenToInt64(*aParam[3]);
			Var4.value_int64 = (__int64)toalign;
		}
		else
			Var4.value_int64 = toalign = 0;
	}
	else
	{
		buf += _tcslen(alignbuf) + 1;
		Var4.value_int64 = toalign;
	}

	// continue as long as we did not reach end of string / structure definition
	while (*buf)
	{
		if (!_tcsncmp(buf,_T("//"),2)) // exclude comments
		{
			buf = StrChrAny(buf,_T("\n\r")) ? StrChrAny(buf,_T("\n\r")) : (buf + _tcslen(buf));
			if (!*buf)
				break; // end of definition reached
		}
		if (buf == StrChrAny(buf,_T("\n\r\t ")))
		{	// Ignore spaces, tabs and new lines before field definition
			buf++;
			continue;
		}
		else if (_tcschr(buf,'{') && (!StrChrAny(buf, _T(";,")) || _tcschr(buf,'{') < StrChrAny(buf, _T(";,"))))
		{	// union or structure in structure definition
			if (!uniondepth++)
				totalunionsize = 0; // done here to reduce code
			if (_tcsstr(buf,_T("struct")) && _tcsstr(buf,_T("struct")) < _tcschr(buf,'{'))
				unionisstruct[uniondepth] = true; // mark that union is a structure
			else 
				unionisstruct[uniondepth] = false;
			// backup offset because we need to set it back after this union / struct was parsed
			// unionsize is initialized to 0 and buffer moved to next character
			if (mod = offset % STRUCTALIGN((maxsize = sizeof_maxsize(buf))))
				offset += (thisalign - mod) % thisalign;
			structalign[uniondepth] = aligntotal > thisalign ? STRUCTALIGN(aligntotal) : thisalign;
			aligntotal = 0;
			unionoffset[uniondepth] = offset; // backup offset
			unionsize[uniondepth] = 0;
			bitsizetotal = bitsize = 0;
			// ignore even any wrong input here so it is even {mystructure...} for struct and  {anyother string...} for union
			buf = _tcschr(buf,'{') + 1;
			continue;
		} 
		else if (*buf == '}')
		{	// update union
			// update size of union in case it was not updated below (e.g. last item was a union or struct)
			if ((maxsize = offset - unionoffset[uniondepth]) > unionsize[uniondepth])
				unionsize[uniondepth] = maxsize;
			// restore offset even if we had a structure in structure
			if (uniondepth > 1 && unionisstruct[uniondepth - 1])
			{
				if (mod = offset % structalign[uniondepth])
					offset += (structalign[uniondepth] - mod) % structalign[uniondepth];
			}
			else
				offset = unionoffset[uniondepth];
			if (structalign[uniondepth] > aligntotal)
				aligntotal = structalign[uniondepth];
			if (unionsize[uniondepth]>totalunionsize)
				totalunionsize = unionsize[uniondepth];
			// last item in union or structure, update offset now if not struct, for struct offset is up to date
			if (--uniondepth == 0)
			{
				// EDIT, no need to align here, next item will be aligned if necessary
				// end of structure, align it. 
				//if (mod = totalunionsize % aligntotal)
				//	totalunionsize += (aligntotal - mod) % aligntotal;
				offset += totalunionsize;
			}
			bitsizetotal = bitsize = 0;
			buf++;
			if (buf == StrChrAny(buf,_T(";,")))
				buf++;
			continue;
		}
		// set defaults
		ispointer = false;
		arraydef = 0;

		// copy current definition field to temporary buffer
		if (StrChrAny(buf, _T("};,")))
		{
			if ((buf_size = _tcscspn(buf, _T("};,"))) > LINE_SIZE - 1)
			{
				obj->Release();
				g_script.ScriptError(ERR_INVALID_STRUCT, buf);
				return NULL;
			}
			_tcsncpy(tempbuf,buf,buf_size);
			tempbuf[buf_size] = '\0';
		}
		else if (_tcslen(buf) > LINE_SIZE - 1)
		{
			obj->Release();
			g_script.ScriptError(ERR_INVALID_STRUCT, buf);
			return NULL;
		}
		else
			_tcscpy(tempbuf,buf);

		// Trim trailing spaces
		rtrim(tempbuf);

		// Pointer
		if (_tcschr(tempbuf, '*'))
		{
			if (_tcschr(tempbuf, ':'))
			{
				obj->Release();
				g_script.ScriptError(ERR_INVALID_STRUCT_BIT_POINTER, tempbuf);
				return NULL;
			}
			ispointer = StrReplace(tempbuf, _T("*"), _T(""), SCS_SENSITIVE, UINT_MAX, LINE_SIZE);
		}
		
		// Array
		if (_tcschr(tempbuf,'['))
		{
			_tcsncpy(arrbuf,_tcschr(tempbuf,'['),MAX_INTEGER_LENGTH);
			arrbuf[_tcscspn(arrbuf,_T("]")) + 1] = '\0';
			arraydef = (int)ATOI64(arrbuf + 1);
			// remove array definition from temp buffer to identify key easier
			StrReplace(tempbuf, arrbuf, _T(""), SCS_SENSITIVE, 1, LINE_SIZE);
			
			if (_tcschr(tempbuf, '['))
			{	// array to array and similar not supported
				obj->Release();
				g_script.ScriptError(ERR_INVALID_STRUCT, tempbuf);
				return NULL;
			}
			// Trim trailing spaces in case we had a definition like UInt [10]
			rtrim(tempbuf);
		}
		// copy type 
		// if offset is 0 and there are no };, characters, it means we have a pure definition
		if (StrChrAny(tempbuf, _T(" \t")) || StrChrAny(tempbuf,_T("};,")) || (!StrChrAny(buf,_T("};,")) && !offset))
		{
			if ((buf_size = _tcscspn(tempbuf,_T("\t "))) > MAX_VAR_NAME_LENGTH*2 + 30)
			{
				obj->Release();
				g_script.ScriptError(ERR_INVALID_STRUCT, tempbuf);
				return NULL;
			}
			isBit = StrChrAny(omit_leading_whitespace(tempbuf), _T(" \t"));
			if (!isBit || *isBit != ':')
			{
				if (_tcsnicmp(defbuf + 1, tempbuf,_tcslen(defbuf)-2))
					bitsizetotal = bitsize = 0;
				_tcsncpy(defbuf + 1, tempbuf, buf_size);
				//_tcscpy(defbuf + 1 + _tcscspn(tempbuf,_T("\t ")),_T(" "));
				defbuf[1 + buf_size] = ' ';
				defbuf[2 + buf_size] = '\0';
			}
			if (StrChrAny(tempbuf, _T(" \t:")))
			{
				if (_tcslen(StrChrAny(tempbuf, _T(" \t:"))) > MAX_VAR_NAME_LENGTH + 30)
				{
					obj->Release();
					g_script.ScriptError(ERR_INVALID_STRUCT, tempbuf);
					return NULL;
				}
				_tcscpy(keybuf, (!isBit || *isBit != ':') ? StrChrAny(tempbuf, _T(" \t:")) : tempbuf);
				if (bitfield = _tcschr(keybuf, ':'))
				{
					*bitfield = '\0';
					bitfield++;
					if (bitsizetotal/8 == thissize)
						bitsizetotal = bitsize = 0;
					bitsizetotal += bitsize = ATOI(bitfield);
				}
				else
					bitsizetotal = bitsize = 0;
				trim(keybuf);
			}
			else 
				keybuf[0] = '\0';
		}
		else // Not 'TypeOnly' definition because there are more than one fields in structure so use previous type
		{
			// Commented following line to keep previous definition like in c++, e.g. "Int x,y,Char a,b", 
			// Note: separator , or ; can be still used but
			// _tcscpy(defbuf,_T(" UInt "));
			if (_tcslen(tempbuf) > _countof(keybuf))
			{
				obj->Release();
				g_script.ScriptError(ERR_INVALID_STRUCT, tempbuf);
				return NULL;
			}
			_tcsncpy(keybuf,tempbuf, _countof(keybuf));
			if (bitfield = _tcschr(keybuf, ':'))
			{
				*bitfield = '\0';
				bitfield++;
				if (bitsizetotal / 8 == thissize)
					bitsizetotal = bitsize = 0;
				bitsizetotal += bitsize = ATOI(bitfield);
			}
			trim(keybuf);
		}

		if (obj->mOwnHeap && !*keybuf && !arraydef)
		//if (!*keybuf && !arraydef)
			arraydef = 1;
		if (_tcschr(keybuf, '('))
		{
			obj->Release();
			g_script.ScriptError(ERR_INVALID_STRUCT, keybuf);
			return NULL;
		}
		// Now find size in default types array and create new field
		// If Type not found, resolve type to variable and get size of struct defined in it
		if ((!_tcscmp(defbuf, _T(" bool ")) && (thissize = 1)) || (thissize = IsDefaultType(defbuf)))
		{
			
			// align offset
			if (ispointer)
			{
				if (mod = offset % STRUCTALIGN(ptrsize))
					offset += (thisalign - mod) % thisalign;
				if (thisalign > aligntotal)
					aligntotal = thisalign;
			}
			else
			{
				if ((!bitsize || bitsizetotal == bitsize) && (mod = offset % STRUCTALIGN(thissize)))
					offset += (thisalign - mod) % thisalign;
				if (thisalign > aligntotal)
					aligntotal = thisalign; // > ptrsize ? ptrsize : thissize;
			}
			if (!(field = obj->Insert(keybuf, insert_pos
			/* pointer*/	, ispointer //!*keybuf ? ispointer : 0
			/* offset */	, (offset == 0 || !bitsize || bitsizetotal == bitsize) ? offset : offset - thissize // !*keybuf ? 0 : 
			/* arraysize */	, arraydef //!*keybuf ? arraydef : 0
			/* fieldsize */	, thissize
			/* isinteger */	, (ispointer || arraydef) ? false : ispointer ? true : !tcscasestr(_T(" FLOAT DOUBLE PFLOAT PDOUBLE "),defbuf)
			/* isunsigned*/	, (ispointer || arraydef) ? 0 : !tcscasestr(_T(" PTR SHORT INT INT8 INT16 INT32 INT64 CHAR ACCESS_MASK PVOID VOID HALF_PTR BOOL LONG LONG32 LONGLONG LONG64 USN INT_PTR LONG_PTR POINTER_64 POINTER_SIGNED SIGNED SSIZE_T WPARAM __int64 "),defbuf)
			/* encoding */	, (ispointer || arraydef) ? -1 : tcscasestr(_T(" TCHAR LPTSTR LPCTSTR "), defbuf) ? UorA(CP_UTF16, CP_ACP) : tcscasestr(_T(" LPWSTR LPCWSTR WCHAR "),defbuf) ? 1200 : tcscasestr(_T(" CHAR LPSTR LPCSTR UCHAR "),defbuf) ? 0 : -1
			/* bitsize */	, (ispointer || arraydef) ? 0 : bitsize
			/* bitfield */	, (ispointer || arraydef) ? 0 : bitsizetotal - bitsize)))
			{	// Out of memory.
				obj->Release();
				return NULL;
			}
			// Dynamicly handle Arrays and pointers
			if (arraydef || ispointer)
			{
				_tcscpy(subdefbuf, defbuf);
				if (ispointer)
				{	// add pointer to definition
					for (int i = ispointer - (arraydef || *keybuf ? 0 : 1); i; i--)
					{
						for (int f = _tcslen(subdefbuf); f; f--)
							subdefbuf[f + 1] = subdefbuf[f];
						subdefbuf[1] = '*';
					}
				}
				if (*keybuf && arraydef)
					_tcscpy(subdefbuf + _tcslen(subdefbuf), arrbuf);
				VarObj1.marker = subdefbuf;
				if (OK == obj->mMain->Invoke(obj_token, this_token, IT_GET, paramObj, 1))
				{	// no addref needed
					field->mStruct = (Struct *)obj_token.object;
				}
				else
				{
					field->mStruct = new Struct();
					if (!field->mStruct)
					{
						obj->Release();
						g_script.ScriptError(ERR_OUTOFMEM);
						return NULL;
					}
					VarObj2.object = field->mStruct;
					obj->mMain->Invoke(obj_token, this_token, IT_SET, paramObj, 2);
					obj_token.object->Release();
					field->mStruct->mMain = obj->mMain;
					obj->mMain->AddRef();
					field->mStruct->mHeap = obj->mHeap;
					field->mStruct->mStructSize = *keybuf && arraydef ? (ispointer ? ptrsize : field->mSize) * arraydef : _tcschr(subdefbuf, '*') ? ptrsize : field->mSize;

					Var1.symbol = SYM_STRING;
					Var1.marker = subdefbuf;
					Var3.symbol = SYM_MISSING;
					Var5.object = field->mStruct;
					if (!Struct::Create(param, 5))
					{
						obj->Release();
						return NULL;
					}
					obj->mMain->Release();
					field->mStruct->mMain = NULL; // won't be required anymore
					Var1.symbol = SYM_VAR;
					Var3.symbol = SYM_INTEGER;
				}
			}
			if (!bitsize || bitsizetotal == bitsize)
				offset += (ispointer ? ptrsize : thissize) * (arraydef ? arraydef : 1);
		}
		else // type was not found, check for user defined type in variables
		{
			Var1.var = NULL;			// init to not found
			Func *bkpfunc = NULL;
			// check if we have a local/static declaration and resolve to function
			// For example Struct("*MyFunc(mystruct) mystr")
			if (_tcschr(defbuf,'('))
			{
				bkpfunc = g->CurrentFunc; // don't bother checking, just backup and restore later
				g->CurrentFunc = (Func *)g_script.FindFunc(defbuf + 1,_tcscspn(defbuf,_T("(")) - 1);
				if (g->CurrentFunc) // break if not found to identify error
				{
					Var1.var = g_script.FindVar(defbuf + _tcscspn(defbuf,_T("(")) + 1,_tcslen(defbuf) - _tcscspn(defbuf,_T("(")) - 3,NULL,FINDVAR_LOCAL,NULL);
					g->CurrentFunc = bkpfunc;
				}
				else // release object and return
				{
					g->CurrentFunc = bkpfunc;
					obj->Release();
					g_script.ScriptError(ERR_INVALID_STRUCT_IN_FUNC, defbuf);
					return NULL;
				}
			}
			else if (g->CurrentFunc) // try to find local variable first
				Var1.var = g_script.FindVar(defbuf + 1,_tcslen(defbuf) - 2,NULL,FINDVAR_LOCAL,NULL);
			// try to find global variable if local was not found or we are not in func
			if (Var1.var == NULL)
				Var1.var = g_script.FindVar(defbuf + 1,_tcslen(defbuf) - 2,NULL,FINDVAR_GLOBAL,NULL);
			// variable found
			if (Var1.var != NULL)
			{
				/*if (!ispointer && !_tcsncmp(tempbuf,buf,_tcslen(buf)) && !*keybuf)
				{   // Whole definition is not a pointer and no key was given so create Structure from variable
					obj->Release();
					for (int i = aParamCount - 1; i; i--)
						param[i] = aParam[i];
					return Struct::Create(param,aParamCount);
				}*/
				// Call BIF_sizeof passing offset in second parameter to align offset if necessary
				// if field is a pointer we will need its size only
				if (!ispointer)
				{
					STRUCTALIGN((sizeof_maxsize(TokenToString(Var1))));
					if (thisalign > aligntotal)
						aligntotal = thisalign;
					if ((!bitsize || bitsizetotal == bitsize) && offset && (mod = offset % aligntotal))
						offset += (aligntotal - mod) % aligntotal;
					Var2.value_int64 = (__int64)offset;
					Var3.value_int64 = (__int64)&aligntotal;
					BIF_sizeof(Result, result_token,param,ispointer ? 1 : 4);
					if (result_token.symbol != SYM_INTEGER)
					{	// could not resolve structure
						obj->Release();
						g_script.ScriptError(ERR_INVALID_STRUCT, defbuf);
						return NULL;
					}
					if ((!bitsize || bitsizetotal == bitsize) && (mod = offset % aligntotal))
						offset += (aligntotal - mod) % aligntotal;
				} 
				else
				{
					Var2.value_int64 = (__int64)0;
					BIF_sizeof(Result, result_token,param,1);
					if (result_token.symbol != SYM_INTEGER)
					{	// could not resolve structure
						obj->Release();
						g_script.ScriptError(ERR_INVALID_STRUCT, defbuf);
						return NULL;
					}
					if (mod = offset % STRUCTALIGN(ptrsize))
						offset += (thisalign - mod) % thisalign;
					if (thisalign > aligntotal)
						aligntotal = thisalign;
				}

				// Insert new field in our structure, all custom structures are dynamically resolved
				if (!(field = obj->Insert(keybuf, insert_pos
					/* pointer*/	, ispointer // !*keybuf ? ispointer : 0
					/* offset */	, (offset == 0 || !bitsize || bitsizetotal == bitsize) ? offset : offset - thissize // !*keybuf ? 0 : 
					/* arraysize */	, arraydef //!*keybuf ? arraydef : 0
					/* fieldsize */	, (int)result_token.value_int64 - (ispointer ? 0 : offset)
					/* isinteger */	, false
					/* isunsigned*/	, false
					/* encoding */	, -1
					/* bitsize */	, 0
					/* bitfield */	, 0)))
				{	// Out of memory.
					obj->Release();
					return NULL;
				}
				_tcscpy(subdefbuf, defbuf);
				if (ispointer)
				{	// add pointer to definition
					for (int i = ispointer - (arraydef || *keybuf ? 0 : 1); i; i--)
					{
						for (int f = _tcslen(subdefbuf); f; f--)
							subdefbuf[f + 1] = subdefbuf[f];
						subdefbuf[1] = '*';
					}
				}
				if (*keybuf && arraydef)
					_tcscpy(subdefbuf + _tcslen(subdefbuf), arrbuf);
				// Structure in Structure
				VarObj1.marker = subdefbuf;
				if (OK == obj->mMain->Invoke(obj_token, this_token, IT_GET, paramObj, 1))
				{	// No AddRef needed
					field->mStruct = (Struct *)obj_token.object;
				}
				else
				{
					field->mStruct = new Struct();
					if (!field->mStruct)
					{
						obj->Release();
						g_script.ScriptError(ERR_OUTOFMEM);
						return NULL;
					}
					VarObj2.object = field->mStruct;
					obj->mMain->Invoke(obj_token, this_token, IT_SET, paramObj, 2);
					field->mStruct->mMain = obj->mMain;
					obj->mMain->AddRef();
					field->mStruct->mHeap = obj->mHeap;
					field->mStruct->mStructSize = *keybuf && arraydef ? (ispointer ? ptrsize : field->mSize) * arraydef : _tcschr(subdefbuf, '*') ? ptrsize : field->mSize;
					
					if (!(!_tcschr(subdefbuf, '*') && !_tcschr(subdefbuf, '[')))
					{
						Var1.symbol = SYM_STRING;
						Var1.marker = subdefbuf;
					}
					// else Var1.var = Var1.var;
					//Var2.value_int64 = (UINT_PTR)(obj->mStructMem + offset);
					Var3.symbol = SYM_MISSING;
					Var5.object = field->mStruct;
					if (!Struct::Create(param, 5))
					{
						obj->Release();
						return NULL; 
					}
					obj->mMain->Release();
					field->mStruct->mMain = NULL; // won't be required anymore
					Var1.symbol = SYM_VAR;
					Var3.symbol = SYM_INTEGER;
				}
				if (ispointer)
					offset += (int)ptrsize * (arraydef ? arraydef : 1);
				else if (!bitsize || bitsizetotal == bitsize)
				// sizeof was given an offset that it applied and aligned if necessary, so set offset =  and not +=
					offset = (int)result_token.value_int64 + (arraydef ? ((arraydef - 1) * ((int)result_token.value_int64 - offset)) : 0);

			}
			else // No variable was found and it is not default type so we can't determine size.
			{
				obj->Release();
				g_script.ScriptError(ERR_INVALID_STRUCT, defbuf);
				return  NULL;
			}
		}
		// update union size
		if (uniondepth)
		{
			if ((maxsize = offset - unionoffset[uniondepth]) > unionsize[uniondepth])
				unionsize[uniondepth] = maxsize;
			if (!unionisstruct[uniondepth])
			{
				// reset offset if in union and union is not a structure
				offset = unionoffset[uniondepth];
				// reset bit offset and size
				bitsize = bitsizetotal = 0;
			}
		}
		// Move buffer pointer now
		if (_tcschr(buf,'}') && (!StrChrAny(buf, _T(";,")) || _tcschr(buf,'}') < StrChrAny(buf, _T(";,"))))
			buf += _tcscspn(buf,_T("}")); // keep } character to update union
		else if (StrChrAny(buf, _T(";,")))
			buf += _tcscspn(buf,_T(";,")) + 1;
		else
			buf += _tcslen(buf);
	}
	// align total structure if necessary
	if (aligntotal && (mod = offset % aligntotal))
		offset += (aligntotal - mod) % aligntotal;
	if (!offset) // structure could not be build
	{
		obj->Release();
		g_script.ScriptError(ERR_INVALID_STRUCT, TokenToString(*aParam[0]));
		return NULL;
	}
	else if (obj->mStructSize != offset)
	{
		obj->Release();
		g_script.ScriptError(_T("Invalid structure Length."), TokenToString(*aParam[0]));
		return NULL;
	}

	if (obj->mOwnHeap)
	{
		obj->mMain->Release();
		if (obj->mFieldCount > 1 || *obj->mFields[0].key)
		{	// set up a copy of structure to be able to access it dynamically
			if (!(obj->mMain = obj->CloneStruct()))
			{
				obj->Release();
				return NULL;
			}
		}
		else
			obj->mMain = NULL;

		// an object was passed to initialize fields
		// enumerate trough object and assign values
		if (aParamCount > 2 && TokenToObject(*aParam[2]))
			obj->ObjectToStruct(TokenToObject(*aParam[2]));
	}
	// set isValue if structure has no items, is not an array, not a pointer and not a reference to structure
	if (obj->mFieldCount == 1)
	{
		FieldType &aField = obj->mFields[0];
		if (aField.key && !aField.mIsPointer && !aField.mArraySize && !aField.mStruct)
			obj->mIsValue = true;
	}
	return obj;
}

//
// Struct::Delete - Called immediately before the object is deleted.
//					Returns false if object should not be deleted yet.
//

Struct* Struct::CloneStruct(bool aSeparate, HANDLE aHeap)
{
	Struct *obj = new Struct();
	if (!obj)
		return NULL;
	obj->mIsValue = mIsValue;
	obj->mStructSize = mStructSize;
	if (aSeparate && aHeap == NULL)
	{
		obj->mHeap = HeapCreate(0, 0, 0);
		if (!obj->mHeap)
			return NULL;
		obj->mOwnHeap = true;
		obj->mStructMem = (UINT_PTR *)HeapAlloc(obj->mHeap, HEAP_ZERO_MEMORY, obj->mStructSize);
		if (!obj->mStructMem)
			return NULL;
		memmove(obj->mStructMem, mStructMem, mStructSize);
	}
	else if (aSeparate)
		obj->mHeap = aHeap;
	else
		obj->mHeap = mHeap;
	LPTSTR key;
	for (int i = 0; i < mFieldCount; i++)
	{	// copy fields since they will be freed on release
		if (obj->mFieldCount == obj->mFieldCountMax && !obj->Expand()  // Attempt to expand if at capacity.
			|| !(key = _tcsdup(mFields[i].key)))  // Attempt to duplicate key-string.
		{	// OutOfMem
			obj->Release();
			return NULL;
		}
		FieldType &field = obj->mFields[i], &from = mFields[i];
		++obj->mFieldCount; // Only after memmove above.

		field.mSize = from.mSize; // Init to ensure safe behaviour in Assign().
		field.key = key; // Above has already copied string
		field.mArraySize = from.mArraySize;
		field.mIsPointer = from.mIsPointer;
		field.mOffset = from.mOffset;
		field.mBitSize = from.mBitSize;
		field.mBitOffset = from.mBitOffset;
		field.mIsInteger = from.mIsInteger;
		field.mIsUnsigned = from.mIsUnsigned;
		field.mEncoding = from.mEncoding;
		if (from.mStruct == NULL)
			field.mStruct = NULL;
		else if (aSeparate)
		{	// clone - separate and use heap of new object or heap forwarded here
			field.mStruct = from.mStruct->CloneStruct(true, aHeap == NULL ? obj->mHeap : aHeap);
			if (!field.mStruct)
			{
				obj->Release();
				return NULL;
			}
		}
		else
		{	// clone to same structure object mMain, onyl AddRef
			field.mStruct = from.mStruct;
			if (field.mStruct)
				field.mStruct->AddRef();
		}
	}
	if (aSeparate && aHeap == NULL && (obj->mFieldCount > 1 || *obj->mFields[0].key))
	{
		obj->mMain = obj->CloneStruct();
		if (!obj->mMain)
		{
			obj->Release();
			return NULL;
		}
	}
	else
		obj->mMain = NULL;
	return obj;
}

//
// Struct::Delete - Called immediately before the object is deleted.
//					Returns false if object should not be deleted yet.
//

bool Struct::Delete()
{
	return ObjectBase::Delete();
}


Struct::~Struct()
{
	if (mFields)
	{
		if (mFieldCount)
		{
			int i = mFieldCount - 1;
			// Free keys
			for ( ; i >= 0 ; --i)
			{
				FieldType &aField = mFields[i];
				if (aField.mStruct)
					aField.mStruct->Release();
				free(mFields[i].key);
			}
		}
		// Free fields array.
		free(mFields);
	}
	if (mMain)
		mMain->Release();
	if (mOwnHeap)
	{
#ifdef _DEBUG
		// vld.h throws memory leaks when HeapFree is not used
		// We cannot use HeapFree directly when walking the Heap
		// get 127 allocated blocks to be freed, free them, then reset Heap Entry and start over again until end of Heap.
		PROCESS_HEAP_ENTRY hEntry = { 0 };
		PVOID *arr = new PVOID[128];
		int i = 0;
		while (HeapWalk(mHeap, &hEntry))
		{
			if (hEntry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
			{	// entry is allocated block, backup pointers to free
				arr[++i] = hEntry.lpData;
				if (i == 127)
				{	// array is full, free allocated blocks and start over again
					for (; i; i--)
						HeapFree(mHeap, 0, arr[i]);
					// reset HeapWalk
					hEntry.lpData = NULL;
				}
			}
		}
		// free remaining blocks and destroy the heap.
		for (; i; i--)
			HeapFree(mHeap, 0, arr[i]);
		delete arr;
#endif
		HeapDestroy(mHeap);
	}
}


//
// Inserts a single field with the given key at the given offset.
// Caller must ensure 'at' is the correct offset for this key.
//

Struct::FieldType *Struct::Insert(LPTSTR key, IndexType &at, USHORT aIspointer, int aOffset, int aArrsize, int aFieldsize, bool aIsInteger, bool aIsunsigned, USHORT aEncoding, BYTE aBitSize, BYTE aBitField)
{
	if (this->FindField(key))
	{
		g_script.ScriptError(ERR_DUPLICATE_DECLARATION, key);
		return NULL;
	}
	if (mFieldCount == mFieldCountMax && !Expand()  // Attempt to expand if at capacity.
		|| !(key = _tcsdup(key)))  // Attempt to duplicate key-string.
	{	// Out of memory.
		g_script.ScriptError(ERR_OUTOFMEM);
		return NULL;
	}
	// There is now definitely room in mFields for a new field.

	FieldType &field = mFields[at++];
	if (at < mFieldCount)
		// Move existing fields to make room.
		memmove(&field + 1, &field, (size_t)(mFieldCount - at) * sizeof(FieldType));
	++mFieldCount; // Only after memmove above.

	// Update key-type offsets based on where and what was inserted; also update this key's ref count:

	field.mSize = aFieldsize; // Init to ensure safe behaviour in Assign().
	field.key = key; // Above has already copied string
	field.mArraySize = aArrsize;
	field.mIsPointer = aIspointer;
	field.mOffset = aOffset;
	field.mBitSize = aBitSize;
	field.mBitOffset = aBitField;
	field.mIsInteger = aIsInteger;
	field.mIsUnsigned = aIsunsigned;
	field.mEncoding = aEncoding;
	field.mStruct = NULL;
	return &field;
}

//
// Struct::ObjectToStruct - Initialize structure from array, object or structure.
//

void Struct::ObjectToStruct(IObject *objfrom)
{
	ExprTokenType ResultToken, this_token, enum_token, param_tokens[3];
	ExprTokenType *params[] = { param_tokens, param_tokens + 1, param_tokens + 2 };
	TCHAR defbuf[MAX_PATH], buf[MAX_PATH];
	int param_count = 3;

	// Set up enum_token the way Invoke expects:
	enum_token.symbol = SYM_STRING;
	enum_token.marker = _T("");
	enum_token.mem_to_free = NULL;
	enum_token.buf = defbuf;

	// Prepare to call object._NewEnum():
	param_tokens[0].symbol = SYM_STRING;
	param_tokens[0].marker = _T("_NewEnum");

	objfrom->Invoke(enum_token, ResultToken, IT_CALL, params, 1);

	if (enum_token.mem_to_free)
		// Invoke returned memory for us to free.
		free(enum_token.mem_to_free);

	// Check if object returned an enumerator, otherwise return
	if (enum_token.symbol != SYM_OBJECT)
		return;

	// create variables to use in for loop / for enumeration
	// these will be deleted afterwards

	Var *var1 = (Var*)alloca(sizeof(Var));
	Var *var2 = (Var*)alloca(sizeof(Var));
	var1->mType = var2->mType = VAR_NORMAL;
	var1->mAttrib = var2->mAttrib = 0;
	var1->mByteCapacity = var2->mByteCapacity = 0;
	var1->mHowAllocated = var2->mHowAllocated = ALLOC_MALLOC;

	// Prepare parameters for the loop below: enum.Next(var1 [, var2])
	param_tokens[0].marker = _T("Next");
	param_tokens[1].symbol = SYM_VAR;
	param_tokens[1].var = var1;
	param_tokens[1].mem_to_free = 0;
	param_tokens[2].symbol = SYM_VAR;
	param_tokens[2].var = var2;
	param_tokens[2].mem_to_free = 0;

	ExprTokenType result_token;
	IObject &enumerator = *enum_token.object; // Might perform better as a reference?

	this_token.symbol = SYM_OBJECT;
	this_token.object = this;

	for (;;)
	{
		// Set up result_token the way Invoke expects; each Invoke() will change some or all of these:
		result_token.symbol = SYM_STRING;
		result_token.marker = _T("");
		result_token.mem_to_free = NULL;
		result_token.buf = buf;

		// Call enumerator.Next(var1, var2)
		enumerator.Invoke(result_token, enum_token, IT_CALL, params, param_count);

		bool next_returned_true = TokenToBOOL(result_token, TokenIsPureNumeric(result_token));
		if (!next_returned_true)
			break;

		this->Invoke(ResultToken, this_token, IT_SET, params + 1, 2);

		// release object if it was assigned prevoiously when calling enum.Next
		if (var1->IsObject())
			var1->ReleaseObject();
		if (var2->IsObject())
			var2->ReleaseObject();

		// Free any memory or object which may have been returned by Invoke:
		if (result_token.mem_to_free)
			free(result_token.mem_to_free);
		if (result_token.symbol == SYM_OBJECT)
			result_token.object->Release();
	}
	// release enumerator and free vars
	enumerator.Release();
	var1->Free();
	var2->Free();
}


#define CHECKHEAPADDR(aAddress) \
{\
	Entry.lpData = NULL;\
	aHeapAddr = false;\
	while (HeapWalk(mHeap, &Entry) != FALSE) {\
		if ((Entry.wFlags & PROCESS_HEAP_REGION) != 0) {\
			if (aHeapAddr = Entry.Region.lpFirstBlock <= aAddress && Entry.Region.lpLastBlock >= aAddress)\
				break;\
		}\
	}\
}
//
// Struct::SetPointer - used to set pointer for a field or array item
//

UINT_PTR Struct::SetPointer(UINT_PTR aPointer,int aArrayItem)
{
	if (mFields[0].mIsPointer) //mIsPointer
		*((UINT_PTR*)((UINT_PTR)mStructMem + (aArrayItem - 1)*sizeof(UINT_PTR))) = aPointer;
	else
		*((UINT_PTR*)((UINT_PTR)mStructMem + (aArrayItem - 1)*mFields[0].mSize)) = aPointer;
	return aPointer;
}


//
// Struct::Invoke - Called by BIF_ObjInvoke when script explicitly interacts with an object.
//

ResultType Struct::Invoke(
						ExprTokenType &aResultToken,
						ExprTokenType &aThisToken,
						int aFlags,
						ExprTokenType *aParam[],
						int aParamCount
						)
// L40: Revised base mechanism for flexibility and to simplify some aspects.
//		obj[] -> obj.base.__Get -> obj.base[] -> obj.base.__Get etc.
{
	int ptrsize = sizeof(UINT_PTR);
    FieldType *field = NULL; // init to NULL to use in IS_INVOKE_CALL
	ResultType Result = OK;

	// Used to resolve dynamic structures
	ExprTokenType Var1,Var2;
	Var1.symbol = SYM_VAR;
	Var2.symbol = SYM_INTEGER;
	ExprTokenType *param[] = { &Var1, &Var2 },ResultToken;
	
	// used to clone a dynamic field or structure
	Struct *subobj = NULL;
	IObject *aInitObject = NULL;
	SIZE_T aSize;

	UINT_PTR *aBkpMem = NULL;
	SIZE_T aBkpSize;
	UINT_PTR *aNewMem;
	SIZE_T aNewSize;
	// used for StrGet/StrPut
	LPCVOID source_string;
	int source_length;
	DWORD flags = WC_NO_BEST_FIT_CHARS;
	int length = -1;
	int char_count;

	int param_count_excluding_rvalue = aParamCount;

	// target may be altered here to resolve dynamic structure so hold it separately
	UINT_PTR *target = mStructMem;
	PROCESS_HEAP_ENTRY Entry;
	bool aHeapAddr;

	if (IS_INVOKE_SET)
	{
		// Prior validation of ObjSet() param count ensures the result won't be negative:
		--param_count_excluding_rvalue;
		
		// Since defining base[key] prevents base.base.__Get and __Call from being invoked, it seems best
		// to have it also block __Set. The code below is disabled to achieve this, with a slight cost to
		// performance when assigning to a new key in any object which has a base object. (The cost may
		// depend on how many key-value pairs each base object has.) Note that this doesn't affect meta-
		// functions defined in *this* base object, since they were already invoked if present.
	}
	
	if (!param_count_excluding_rvalue || (param_count_excluding_rvalue == 1 && TokenIsEmptyString(*aParam[0])))
	{   // for struct[] and struct[""...] / struct[] := ptr and struct[""...] := ptr 
		if (IS_INVOKE_SET)
		{
			if (aInitObject = TokenToObject(*aParam[param_count_excluding_rvalue]))
			{   // Initialize structure using an object. e.g. struct[]:={x:1,y:2} and return our object to caller
				this->ObjectToStruct(aInitObject);
				aResultToken.SetValue(aInitObject);
				aInitObject->AddRef();
			}
			else if (mOwnHeap) // assign a custom pointer
			{
				if (!TokenToInt64(*aParam[param_count_excluding_rvalue]))
					return g_script.ScriptError(ERR_PARAM_INVALID);
				CHECKHEAPADDR(mStructMem);
				if (aHeapAddr)
					HeapFree(mHeap, 0, mStructMem);
				// assign new pointer to structure
				mStructMem = (UINT_PTR *)TokenToInt64(*aParam[param_count_excluding_rvalue]);
				aResultToken.SetValue(TokenToInt64(*aParam[param_count_excluding_rvalue]));
			}
			else // assign new pointer to dynamic structure (field)
			{
				if (!TokenToInt64(*aParam[param_count_excluding_rvalue]))
					return g_script.ScriptError(ERR_PARAM_INVALID);
				aBkpMem = (UINT_PTR *)*((UINT_PTR*)((UINT_PTR)target));
				aHeapAddr = false;
				if (aBkpMem)
					CHECKHEAPADDR(aBkpMem);
				if (aHeapAddr)
					HeapFree(mHeap, 0, aBkpMem);
				*((UINT_PTR*)((UINT_PTR)target)) = (UINT_PTR)TokenToInt64(*aParam[param_count_excluding_rvalue]);
				aResultToken.SetValue((__int64)*((UINT_PTR*)((UINT_PTR)target)));
			}
		}
		else if (mOwnHeap) // else return structure address
			aResultToken.SetValue((__int64)mStructMem);
		else // return pointer
			aResultToken.SetValue((__int64)(UINT_PTR *)*((UINT_PTR*)((UINT_PTR)target)));
		return OK;
	}
	else
	{	// Array access, struct.1 or struct[1] or struct[1].x ...
		if (TokenToInt64(*aParam[0]))
		{
			if (param_count_excluding_rvalue > 1 && TokenIsEmptyString(*aParam[1])) 
			{	// caller wants set/get pointer. E.g. struct.2[""] or struct.2[""] := ptr
				if (IS_INVOKE_SET)
				{
					if (param_count_excluding_rvalue < 3) // simply set pointer
						aResultToken.value_int64 = SetPointer((UINT_PTR)TokenToInt64(*aParam[2]),(int)TokenToInt64(*aParam[0]));
					else
					{	// resolve pointer to pointer and set it
						UINT_PTR *aDeepPointer = ((UINT_PTR*)((mFields[0].mIsPointer ? *target : (UINT_PTR)target) + (TokenToInt64(*aParam[0])-1)*mFields[0].mSize));
						for (int i = param_count_excluding_rvalue - 2;i && aDeepPointer;i--)
							aDeepPointer = (UINT_PTR*)*aDeepPointer;
						*aDeepPointer = (UINT_PTR)TokenToInt64(*aParam[aParamCount]);
						aResultToken.value_int64 = *aDeepPointer;
					}
				}
				else // GET pointer
				{
					if (param_count_excluding_rvalue < 3)
						aResultToken.value_int64 = ((mFields[0].mIsPointer ? *target : (UINT_PTR)target) + (TokenToInt64(*aParam[0])-1)*mFields[0].mSize);
					else
					{	// resolve pointer to pointer
						UINT_PTR *aDeepPointer = ((UINT_PTR*)((UINT_PTR)target + (TokenToInt64(*aParam[0])-1)*mFields[0].mSize));
						for (int i = param_count_excluding_rvalue - 2;i && *aDeepPointer;i--)
							aDeepPointer = (UINT_PTR*)*aDeepPointer;
						aResultToken.value_int64 = (__int64)aDeepPointer;
					}
				}
				aResultToken.symbol = SYM_INTEGER;
				return OK;
			}

			// struct must have unnamed field to be accessed dynamically, otherwise it is main structure
			if (mOwnHeap && mMain)
			{
				subobj = mMain;
				aSize = mStructSize;
			}
			else if (mFieldCount > 1 || *mFields[0].key || !(subobj = mFields[0].mStruct))
				return INVOKE_NOT_HANDLED;

			// Get size of structure
			else if (mFields[0].mIsPointer > 1)
				aSize = ptrsize; // array of pointers or pointer to array
			else
				aSize = mFields[0].mSize; // array or pointer to array of structs
			if (mFields[0].mIsPointer && !IS_INVOKE_CALL) // IS_INVOKE_CALL will need the address and not pointer // removed: '&& !mFields[0].mArraySize' because it is a pointer to array, not array of pointers
				// Pointer to array
				subobj->mStructMem = target = (UINT_PTR*)((UINT_PTR)*target + ((TokenToInt64(*aParam[0]) - 1)*aSize));
			else // assume array
				subobj->mStructMem = target = (UINT_PTR*)((UINT_PTR)target + ((TokenToInt64(*aParam[0]) - 1)*aSize));
			

			if (param_count_excluding_rvalue > 1)
			{	// MULTIPARAM
				subobj->Invoke(aResultToken, aThisToken, aFlags, aParam + 1, aParamCount - 1);
				return OK;
			}
			else if (IS_INVOKE_SET && (aInitObject = TokenToObject(*aParam[1])))
			{	// init Structure from Object
				subobj->ObjectToStruct(aInitObject);
				aInitObject->AddRef();
				aResultToken.SetValue(aInitObject);
				return OK;
			}
			else if (IS_INVOKE_GET && !subobj->mIsValue)
			{	// field is a structure
				subobj->AddRef();
				aResultToken.SetValue(subobj);
				return OK;
			}
			
			// structure has only 1 unnamed field (array or pointer
			field = &subobj->mFields[0];
		}
		else if (!IS_INVOKE_CALL) // IS_INVOKE_CALL will handle the field itself
		{
			if (mFieldCount == 1 && !*mFields[0].key)
			{	// Direct Array access e.g. struct.Field instead of struct.1.Field
				subobj = mMain;
				if (mFields[0].mIsPointer && !mFields[0].mArraySize && !IS_INVOKE_CALL) // IS_INVOKE_CALL will need the address and not pointer
					// Pointer to array
					target = (UINT_PTR*)((UINT_PTR)*target);
				field = mFields[0].mStruct->FindField(TokenToString(*aParam[0]));
			}
			else
				field = FindField(TokenToString(*aParam[0]));
		}
	}

	//
	// OPERATE ON A FIELD WITHIN THIS OBJECT
	//

	// CALL
	if (IS_INVOKE_CALL)
	{
		LPTSTR name = TokenToString(*aParam[0]);
		if (*name == '_')
			++name; // ++ to exclude '_' from further consideration.
		++aParam; --aParamCount; // Exclude the method identifier.  A prior check ensures there was at least one param in this case.
		if (!_tcsicmp(name, _T("NewEnum")))
			return _NewEnum(aResultToken, aParam, aParamCount);
		// if first function parameter is a field get it
		if (*mFields[0].key && aParamCount && !TokenToInt64(*aParam[0]))
		{
			if (field = FindField(TokenToString(*aParam[0])))
			{	// exclude parameter in aParam
				++aParam; --aParamCount;
			}
		}
		aResultToken.SetValue((__int64)0); // set default, mainly used
		if (!_tcsicmp(name, _T("Size")))
		{
			if (aParamCount && ((field && (field->mArraySize || field->mIsPointer)) || mFields[0].mArraySize || mFields[0].mIsPointer) && TokenToInt64(*aParam[0]))
				aResultToken.value_int64 = field ? field->mStruct->mFields[0].mSize : mFields[0].mSize;
			else
				aResultToken.value_int64 = field ? field->mSize : mStructSize;
			return OK;
		}
		else if (!_tcsicmp(name, _T("SetCapacity")))
		{	// Set strcuture its capacity or fields capacity
			aBkpMem = NULL;
			aBkpSize = 0;
			if (!field && !*mFields[0].key && (mFields[0].mArraySize || mFields[0].mIsPointer))
			{	// Set capacity for array
				if (!aParamCount || !TokenToInt64(*aParam[0]))
					return g_script.ScriptError(ERR_PARAM_INVALID);
				aBkpMem = (UINT_PTR *)*((UINT_PTR*)((UINT_PTR)target + (field ? field->mOffset : 0) 
						+ (TokenToInt64(*aParam[0])-1) * (field 
							? (field->mIsPointer ? ptrsize : field->mStruct->mFields[0].mSize) 
							: (mFields[0].mIsPointer ? ptrsize : mFields[0].mSize))));
				aHeapAddr = false;
				if (aBkpMem)
					CHECKHEAPADDR(aBkpMem);
				if (aHeapAddr)
					aBkpSize = HeapSize(mHeap, 0, aBkpMem);
				if (aParamCount == 1 || (aParamCount > 1 && TokenToInt64(*aParam[1]) == 0))
				{	// 0 or no parameters were given -> free memory only
					if (aHeapAddr)
						HeapFree(mHeap, 0, aBkpMem);
					return OK;
				}
				// allocate memory
				if (aNewMem = (UINT_PTR*)HeapAlloc(mHeap, HEAP_ZERO_MEMORY, aNewSize = TokenToInt64(*aParam[1])))
				{
					if (aHeapAddr)	// fill existent structure
						memmove(aNewMem, aBkpMem, aNewSize < aBkpSize ? aNewSize : aBkpSize);
					*((UINT_PTR*)((UINT_PTR)target + (field ? field->mOffset : 0)
						+ (TokenToInt64(*aParam[0]) - 1) * (field
							? (field->mIsPointer ? ptrsize : field->mStruct->mFields[0].mSize)
							: (mFields[0].mIsPointer ? ptrsize : mFields[0].mSize)))) = (UINT_PTR)aNewMem;
					aResultToken.value_int64 = aNewSize;
					if (aHeapAddr)
						HeapFree(mHeap, 0, aBkpMem);
				}
				else
					return g_script.ScriptError(ERR_OUTOFMEM, name);
				return OK;
			}
			else if (!field)
			{	// change structure memory
				if (!aParamCount || TokenToInt64(*aParam[0]) == 0)
					// 0 or no parameters were given -> free memory not allowed since structure might become unusable
					return g_script.ScriptError(ERR_PARAM1_REQUIRED, name);
				else if (!mOwnHeap) // trying to set main structure memory from dynamic field
					return g_script.ScriptError(ERR_INVALID_BASE, name);
				CHECKHEAPADDR((aBkpMem = mStructMem));
				if (aHeapAddr)
					aBkpSize = HeapSize(mHeap, 0, aBkpMem);
				// allocate memory
				if (aNewMem = (UINT_PTR*)HeapAlloc(mHeap, HEAP_ZERO_MEMORY, aNewSize = TokenToInt64(*aParam[0])))
				{
					if (aHeapAddr)	// fill existent structure
						memmove(aNewMem, aBkpMem, aNewSize < aBkpSize ? aNewSize : aBkpSize);
					mStructMem = (UINT_PTR*)aNewMem;
					aResultToken.value_int64 = aNewSize;
					if (aHeapAddr)
						HeapFree(mHeap, 0, aBkpMem);
				}
				else
					return g_script.ScriptError(ERR_OUTOFMEM, name);
			}
			else
			{	// e.g. struct.SetCapacity("field",100)
				aBkpMem = (UINT_PTR *)*((UINT_PTR*)((UINT_PTR)target + (field ? field->mOffset : 0)));
				aHeapAddr = false;
				if (aBkpMem)
					CHECKHEAPADDR(aBkpMem);
				if (aHeapAddr)
					aBkpSize = HeapSize(mHeap, 0, aBkpMem);
				if (!aParamCount || TokenToInt64(*aParam[0]) == 0)
				{	// 0 or no parameters were given -> free memory only
					if (aHeapAddr)
						HeapFree(mHeap, 0, aBkpMem);
					return OK;
				}
				// allocate memory
				if (aNewMem = (UINT_PTR*)HeapAlloc(mHeap, HEAP_ZERO_MEMORY, aNewSize = TokenToInt64(*aParam[0])))
				{
					if (aHeapAddr)	// fill existent structure
						memmove(aNewMem, aBkpMem, aNewSize < aBkpSize ? aNewSize : aBkpSize);
					*((UINT_PTR*)((UINT_PTR)target + (field ? field->mOffset : 0))) = (UINT_PTR)aNewMem;
					aResultToken.value_int64 = aNewSize;
					if (aHeapAddr)
						HeapFree(mHeap, 0, aBkpMem);
				}
				else
					return g_script.ScriptError(ERR_OUTOFMEM, name);
			}
			return OK;
		}
		else if (!_tcsicmp(name, _T("Clone")))
		{
			if (field)
			{
				if (field->mStruct)
					aResultToken.object = field->mStruct->CloneStruct(true);
				else
					return g_script.ScriptError(ERR_INVALID_BASE);
			}
			else
				aResultToken.object = this->CloneStruct(true);
			if (!aResultToken.object)
				return g_script.ScriptError(ERR_OUTOFMEM);
			aResultToken.symbol = SYM_OBJECT;
			return OK;
		}
		else if (!_tcsicmp(name, _T("Encoding")))
		{
			if (field)
				aResultToken.value_int64 = field->mEncoding == 65535 ? -1 : field->mEncoding;
			else if (!*mFields[0].key)
				aResultToken.value_int64 = mFields[0].mEncoding == 65535 ? -1 : mFields[0].mEncoding;
			return OK;
		}
		else if (!_tcsicmp(name, _T("GetCapacity")))
		{
			if (aParamCount)
			{
				if ((field && !field->mArraySize) || (!field && !mFields[0].mArraySize && mFields[0].key) || !TokenToInt64(*aParam[0]))
					return g_script.ScriptError(ERR_PARAM_INVALID, TokenToString(*aParam[0]));
				aBkpMem = (UINT_PTR *)*((UINT_PTR*)((UINT_PTR)target + (field ? field->mOffset : 0)
					+ (TokenToInt64(*aParam[0]) - 1) * (field
						? (field->mIsPointer ? ptrsize : field->mStruct->mFields[0].mSize)
						: (mFields[0].mIsPointer ? ptrsize : mFields[0].mSize))));
				aHeapAddr = false;
				if (aBkpMem)
					CHECKHEAPADDR(aBkpMem);
				if (aHeapAddr)
					aResultToken.value_int64 = HeapSize(mHeap, 0, aBkpMem);
			}
			else
			{
				aBkpMem = (UINT_PTR *)*((UINT_PTR*)((UINT_PTR)target + (field ? field->mOffset : 0)));
				aHeapAddr = false;
				if (aBkpMem)
					CHECKHEAPADDR(aBkpMem);
				if (aHeapAddr)
					aResultToken.value_int64 = HeapSize(mHeap, 0, aBkpMem);
			}
			return OK;
		}
		else if (!_tcsicmp(name, _T("GetAddress")))
		{
			if (aParamCount)
			{
				if ((field && !field->mArraySize && !field->mIsPointer) || (!field && !mFields[0].mArraySize && !mFields[0].mIsPointer) || !TokenToInt64(*aParam[0]))
					return g_script.ScriptError(ERR_PARAM_INVALID, TokenToString(*aParam[0]));
				aResultToken.value_int64 = (UINT_PTR)target + (field ? field->mOffset : 0)
					+ ((TokenToInt64(*aParam[0]) - 1) * (field
						? (field->mIsPointer ? ptrsize : field->mStruct->mFields[0].mSize)
						: (mFields[0].mIsPointer ? ptrsize : mFields[0].mSize)));
			}
			else
				aResultToken.value_int64 = (UINT_PTR)target + (field ? field->mOffset : 0);
			return OK;
		}
		else if (!_tcsicmp(name, _T("GetPointer")))
		{
			if (aParamCount)
			{
				if ((field && !field->mIsPointer) || (!field && !mFields[0].mIsPointer) || !TokenToInt64(*aParam[0]))
					return g_script.ScriptError(ERR_PARAM_INVALID, TokenToString(*aParam[0]));
				aResultToken.value_int64 = *((UINT_PTR*)((UINT_PTR)target + (field ? field->mOffset : 0)
					+ ((TokenToInt64(*aParam[0]) - 1) * (field
						? (field->mIsPointer ? ptrsize : field->mStruct->mFields[0].mSize)
						: (mFields[0].mIsPointer ? ptrsize : mFields[0].mSize)))));
			}
			else
				aResultToken.value_int64 = *((UINT_PTR*)((UINT_PTR)target + (field ? field->mOffset : 0)));
			return OK;
		}
		else if (!_tcsicmp(name, _T("IsPointer")))
		{
			aResultToken.value_int64 = field ? field->mIsPointer : mFields[0].mIsPointer;
			return OK;
		}
		else if (!_tcsicmp(name, _T("Offset")))
		{
			if (aParamCount)
			{
				if ((field && !field->mArraySize) || (!field && !mFields[0].mArraySize && !mFields[0].mIsPointer) || !TokenToInt64(*aParam[0]))
					return g_script.ScriptError(ERR_PARAM_INVALID, TokenToString(*aParam[0]));
				aResultToken.value_int64 = (field ? field->mOffset : 0)
					+ (TokenToInt64(*aParam[0]) - 1) * (field
						? (field->mIsPointer ? ptrsize : field->mStruct->mFields[0].mSize)
						: (mFields[0].mIsPointer ? ptrsize : mFields[0].mSize));
			}
			else
				aResultToken.value_int64 = field ? field->mOffset : 0;
			return OK;
		}
		else if (!_tcsicmp(name, _T("CountOf")))
		{
			if (!field)
				aResultToken.value_int64 = mFields[0].mArraySize;
			else
				aResultToken.value_int64 = field->mArraySize;
			return OK;
		}
		// For maintainability: explicitly return since above has done ++aParam, --aParamCount.
		aResultToken.SetValue(_T("")); // identify that method was not found
		return INVOKE_NOT_HANDLED;
	}
	else if (!field)
	{	// field was not found. The structure doesn't handle this method/property.
		return INVOKE_NOT_HANDLED;
	}
	else if (!target)
		return g_script.ScriptError(ERR_INVALID_USAGE);


	// MULTIPARAM[x,y] -- may be SET[x,y]:=z or GET[x,y], but always treated like GET[x].
	else if (param_count_excluding_rvalue > 1)
	{	// second parameter = "", so caller wants get/set field pointer
		if (TokenIsEmptyString(*aParam[1]))
		{
			aResultToken.symbol = SYM_INTEGER;
			if (IS_INVOKE_SET)
			{
				if (param_count_excluding_rvalue < 3)
				{   // set simple pointer
					*((UINT_PTR*)((UINT_PTR)target + field->mOffset)) = (UINT_PTR)TokenToInt64(*aParam[2]);
					aResultToken.value_int64 = *(UINT_PTR*)((UINT_PTR)target + field->mOffset);
				}
				else // set pointer to pointer
				{
					UINT_PTR *aDeepPointer = ((UINT_PTR*)((mFields[0].mIsPointer ? *target : (UINT_PTR)target) + field->mOffset));
					for (int i = param_count_excluding_rvalue - 2;i && aDeepPointer;i--)
						aDeepPointer = (UINT_PTR*)*aDeepPointer;
					*aDeepPointer = (UINT_PTR)TokenToInt64(*aParam[aParamCount]);
					aResultToken.value_int64 = *aDeepPointer;
				}
			}
			else // GET pointer
			{
				if (param_count_excluding_rvalue < 3)
					aResultToken.value_int64 = (mFields[0].mIsPointer ? *target : (UINT_PTR)target) + field->mOffset;
				else
				{	// get pointer to pointer
					UINT_PTR *aDeepPointer = ((UINT_PTR*)((mFields[0].mIsPointer ? *target : (UINT_PTR)target) + field->mOffset));
					for (int i = param_count_excluding_rvalue - 2;i && *aDeepPointer;i--)
						aDeepPointer = (UINT_PTR*)*aDeepPointer;
					aResultToken.value_int64 = (__int64)aDeepPointer;
				}
			}
			return OK;
		}
		else // advandce param and invoke again
		{
			field->mStruct->mStructMem = (UINT_PTR*)((UINT_PTR)target + field->mOffset);
			field->mStruct->Invoke(aResultToken, aThisToken, aFlags, aParam + 1, aParamCount - 1);
			return OK;
		}
	} // MULTIPARAM[x,y] x[y]
	
	// SET
	else if (IS_INVOKE_SET)
	{
		if (field->mStruct)
		{	// field is structure
			if (aInitObject = TokenToObject(*aParam[1]))
			{ // Init structure from objct
				field->mStruct->mStructMem = ((UINT_PTR*)((UINT_PTR)target + field->mOffset));
				field->mStruct->ObjectToStruct(aInitObject);
				aResultToken.SetValue(aInitObject);
				aInitObject->AddRef();
				return OK;
			} 
			else // trying to assign a value to structure and not field -> error
				return g_script.ScriptError(ERR_INVALID_ASSIGNMENT);
		}
		// StrPut (code taken from BIF_StrPut())
		if (field->mEncoding != 65535)
		{	// field is [T|W|U]CHAR or LP[TC]STR, set get character or string
			source_string = (LPCVOID)TokenToString(*aParam[1], aResultToken.buf);
			source_length = (int)((aParam[1]->symbol == SYM_VAR) ? aParam[1]->var->CharLength() : _tcslen((LPCTSTR)source_string));
			aNewSize = (source_length + 1) * (field->mEncoding == 1200 ? sizeof(WCHAR) : sizeof(CHAR));
			if (field->mSize > 2)
			{
				aBkpMem = (UINT_PTR*)*((UINT_PTR*)((UINT_PTR)target + field->mOffset));
				aHeapAddr = false;
				if (aBkpMem)
					CHECKHEAPADDR(aBkpMem);
				if (aHeapAddr)
				{
					if (aNewSize != HeapSize(mHeap, 0, aBkpMem))
					{
						if (!(aNewMem = (UINT_PTR*)HeapReAlloc(mHeap, HEAP_ZERO_MEMORY, aBkpMem, aNewSize)))
							return g_script.ScriptError(ERR_OUTOFMEM, field->key);
						else
							*((UINT_PTR*)((UINT_PTR)target + field->mOffset)) = (UINT_PTR)aNewMem;
						HeapFree(mHeap, 0, aBkpMem);
					}
				}
				else
				{
					if (!(aNewMem = (UINT_PTR*)HeapAlloc(mHeap, HEAP_ZERO_MEMORY, aNewSize)))
						return g_script.ScriptError(ERR_OUTOFMEM, field->key);
					else
						*((UINT_PTR*)((UINT_PTR)target + field->mOffset)) = (UINT_PTR)aNewMem;
				}
			}
			if (field->mSize > 2) // not [T|W|U]CHAR
				source_length++; // for terminating character
			if (field->mEncoding == UorA(CP_UTF16, CP_ACP))
			{
				if (TokenIsEmptyString(*aParam[1]))
					*((LPWSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset))) = '\0';
				else
				{
					tmemcpy((LPWSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)), (LPTSTR)source_string, field->mSize < 4 ? 1 : source_length);
					if (field->mSize > 2) // NOT TCHAR or CHAR or WCHAR
						((LPWSTR)*(UINT_PTR*)((UINT_PTR)target + field->mOffset))[source_length - 1] = '\0';
				}
			}
			else
			{
				// Conversion is required. For Unicode builds, this means encoding != CP_UTF16;
#ifndef UNICODE // therefore, this section is relevant only to ANSI builds:
				if (field->mEncoding == CP_UTF16)
				{
					// See similar section below for comments.
					char_count = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)source_string, source_length, NULL, 0);
					if (!char_count)
					{
						aResultToken.value_int64 = char_count;
						return OK;
					}
					if (field->mSize > 2) // Not TCHAR or CHAR or WCHAR
						++char_count; // + 1 for null-terminator (source_length causes it to be excluded from char_count).
					length = char_count;
					char_count = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)source_string, source_length, (LPWSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)), length);
					if (field->mSize > 2 && char_count && char_count < length)
						((LPWSTR)*(UINT_PTR*)((UINT_PTR)target + field->mOffset))[char_count] = '\0';
				}
				else // encoding != CP_UTF16
				{
					// Convert native ANSI string to UTF-16 first.
					CStringWCharFromChar wide_buf((LPCSTR)source_string, source_length, CP_ACP);
					source_string = wide_buf.GetString();
					source_length = wide_buf.GetLength();
#endif
					char_count = WideCharToMultiByte(field->mEncoding, WC_NO_BEST_FIT_CHARS, (LPCWSTR)source_string, source_length, NULL, 0, NULL, NULL);
					if (!char_count) // Above has ensured source is not empty, so this must be an error.
					{
						if (GetLastError() == ERROR_INVALID_FLAGS)
						{
							// Try again without flags.  MSDN lists a number of code pages for which flags must be 0, including UTF-7 and UTF-8 (but UTF-8 is handled above).
							flags = 0; // Must be set for this call and the call further below.
							char_count = WideCharToMultiByte(field->mEncoding, flags, (LPCWSTR)source_string, source_length, NULL, 0, NULL, NULL);
						}
						if (!char_count)
						{
							aResultToken.SetValue(_T(""));
							return OK;
						}
					}
					// Assume there is sufficient buffer space and hope for the best:
					length = char_count;
					// Convert to target encoding.
					char_count = WideCharToMultiByte(field->mEncoding, flags, (LPCWSTR)source_string, source_length, (LPSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)), char_count, NULL, NULL);

					// Since above did not null-terminate, check for buffer space and null-terminate if there's room.
					// It is tempting to always null-terminate (potentially replacing the last byte of data),
					// but that would exclude this function as a means to copy a string into a fixed-length array.
					if (field->mSize > 2 && char_count && char_count < length) // NOT TCHAR or CHAR or WCHAR
						((LPSTR)*(UINT_PTR*)((UINT_PTR)target + field->mOffset))[char_count] = '\0';
#ifndef UNICODE
				}
#endif
			}
			aResultToken.symbol = SYM_STRING;
			aResultToken.marker = (LPTSTR)source_string;
		}
		else // NumPut
		{	 // code taken from BIF_NumPut and extended to handle bits
#define setbits(var, val, typesize, offset, size) (var |= ((val < 0 ? val + (2 << (size - 1)) : val) << offset) & (~( ((~0ull) << ((offset + size)-1)) << 1 )))
#define clearbit(val, pos) ((val) &= ~(1 << (pos)))
#define getbits(val, typesize, offset, size, isunsigned) (((val >> offset) & (~( ((~0ull) << ((size)-1)) << 1 ))) - ((!isunsigned && (!!((val) & (1i64 << (offset + size - 1))))) ? (2 << (size - 1)) : 0))
			if (field->mBitSize)
			{
				aResultToken.symbol = SYM_INTEGER;
				for (int i = 0; i < field->mBitSize;i++)
					clearbit(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mBitOffset + i);
				if (TokenToInt64(*aParam[1]) == 0)
					aResultToken.value_int64 = 0;
				else
				{
					setbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), (field->mIsUnsigned && !IS_NUMERIC(aParam[1]->symbol)) ? ATOI64(TokenToString(*aParam[1])) : TokenToInt64(*aParam[1]), field->mSize, field->mBitOffset, field->mBitSize);
					switch (field->mSize)
					{
					case 4: // Listed first for performance.
						if (field->mIsInteger)
						{
							if (field->mIsUnsigned)
								aResultToken.value_int64 = (unsigned int)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
							else
								aResultToken.value_int64 = (int)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
						}
						else // Float (32-bit).
						{
							aResultToken.symbol = SYM_FLOAT;
							aResultToken.value_double = (float)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
						}
						break;
					case 8:
						aResultToken.value_int64 = (__int64)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
						if (!field->mIsInteger) // Double (64-bit).
							aResultToken.symbol = SYM_FLOAT;
						break;
					case 2:
						if (field->mIsUnsigned)
							aResultToken.value_int64 = (unsigned short)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
						else
							aResultToken.value_int64 = (short)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
						break;
					default: // size 1
						if (field->mIsUnsigned)
							aResultToken.value_int64 = (unsigned char)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
						else
							aResultToken.value_int64 = (char)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
					}
				}
			}
			else
				switch(field->mSize)
				{
				case 4: // Listed first for performance.
					if (field->mIsInteger)
					{
						*((unsigned int *)((UINT_PTR)target + field->mOffset)) = (unsigned int)TokenToInt64(*aParam[1]);
						aResultToken.symbol = SYM_INTEGER;
						aResultToken.value_int64 = *((unsigned int *)((UINT_PTR)target + field->mOffset));
					}
					else // Float (32-bit).
					{
						*((float *)((UINT_PTR)target + field->mOffset)) = (float)TokenToDouble(*aParam[1]);
						aResultToken.symbol = SYM_FLOAT;
						aResultToken.value_double = *((float *)((UINT_PTR)target + field->mOffset));
					}
					break;
				case 8:
					if (field->mIsInteger)
					{
						// v1.0.48: Support unsigned 64-bit integers like DllCall does:
						*((__int64 *)((UINT_PTR)target + field->mOffset)) = (field->mIsUnsigned && !IS_NUMERIC(aParam[1]->symbol)) // Must not be numeric because those are already signed values, so should be written out as signed so that whoever uses them can interpret negatives as large unsigned values.
							? (__int64)ATOU64(TokenToString(*aParam[1])) // For comments, search for ATOU64 in BIF_DllCall().
							: TokenToInt64(*aParam[1]);
						aResultToken.symbol = SYM_INTEGER;
						aResultToken.value_int64 = TokenToInt64(*aParam[1]);
					}
					else // Double (64-bit).
					{
						*((double *)((UINT_PTR)target + field->mOffset)) = TokenToDouble(*aParam[1]);
						aResultToken.symbol = SYM_FLOAT;
						aResultToken.value_double = *((double *)((UINT_PTR)target + field->mOffset));
					}
					break;
				case 2:
					*((unsigned short *)((UINT_PTR)target + field->mOffset)) = (unsigned short)TokenToInt64(*aParam[1]);
					aResultToken.symbol = SYM_INTEGER;
					aResultToken.value_int64 = *((unsigned short *)((UINT_PTR)target + field->mOffset));
					break;
				default: // size 1
					*((unsigned char *)((UINT_PTR)target + field->mOffset)) = (unsigned char)TokenToInt64(*aParam[1]);
					aResultToken.symbol = SYM_INTEGER;
					aResultToken.value_int64 = *((unsigned char *)((UINT_PTR)target + field->mOffset));
				}
		}
		return OK;
	}

	// GET
	else if (field)
	{
		if (field->mStruct)
		{	// field is structure
			field->mStruct->mStructMem = (UINT_PTR *)((UINT_PTR)target + field->mOffset);
			aResultToken.SetValue(field->mStruct);
			field->mStruct->AddRef();
			return OK;
		} 

		// StrGet (code stolen from BIF_StrGet())
		if (field->mEncoding != 65535)
		{
			aResultToken.symbol = SYM_STRING;
			if (field->mEncoding == UorA(CP_UTF16, CP_ACP)) // no conversation required
			{
				if (field->mSize > 2 && !*((UINT_PTR*)((UINT_PTR)target + field->mOffset)))
				{
					aResultToken.SetValue(_T(""));
					return OK;
				}
				if (!TokenSetResult(aResultToken, (LPCTSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)), field->mSize < 4 ? 1 : -1))
					aResultToken.SetValue(_T(""));
			}
			else
			{
				// Conversion is required.
				int conv_length;
				if (field->mSize < 4) // TCHAR or CHAR or WCHAR
					length = 1;

#ifdef UNICODE
				// Convert multi-byte encoded string to UTF-16.
				conv_length = MultiByteToWideChar(field->mEncoding, 0, (LPCSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)), length, NULL, 0);
				if (!TokenSetResult(aResultToken, NULL, conv_length)) // DO NOT SUBTRACT 1, conv_length might not include a null-terminator.
					return OK;
				conv_length = MultiByteToWideChar(field->mEncoding, 0, (LPCSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)), length, aResultToken.marker, conv_length);
#else
				CStringW wide_buf;
				// If the target string is not UTF-16, convert it to that first.
				if (field->mEncoding != CP_UTF16)
				{
					StringCharToWChar((LPCSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)), wide_buf, length, field->mEncoding);
					(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : *(UINT_PTR*)((UINT_PTR)target + field->mOffset)) = (UINT_PTR)wide_buf.GetString();
					length = wide_buf.GetLength();
				}

				// Now convert UTF-16 to ACP.
				conv_length = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, (LPCWSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)), length, NULL, 0, NULL, NULL);
				if (!TokenSetResult(aResultToken, NULL, conv_length)) // DO NOT SUBTRACT 1, conv_length might not include a null-terminator.
					return OK; // Out of memory.
				conv_length = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, (LPCWSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)), length, aResultToken.marker, conv_length, NULL, NULL);
#endif
				if (conv_length && !aResultToken.marker[conv_length - 1])
					--conv_length; // Exclude null-terminator.
				else
					aResultToken.marker[conv_length] = '\0';
				aResultToken.marker_length = conv_length; // Update this in case TokenSetResult used mem_to_free.
				return OK;
			}
		}
		else // NumGet (code stolen from BIF_NumGet())
		{
			switch(field->mSize)
			{
			case 4: // Listed first for performance.
				if (!field->mIsInteger)
				{
					if (field->mBitSize)
						aResultToken.value_double = (float)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
					else
						aResultToken.value_double = *((float *)((UINT_PTR)target + field->mOffset));
					aResultToken.symbol = SYM_FLOAT;
				}
				else if (!field->mIsUnsigned)
				{
					if (field->mBitSize)
						aResultToken.value_int64 = (int)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
					else
						aResultToken.value_int64 = *((int *)((UINT_PTR)target + field->mOffset)); // aResultToken.symbol was set to SYM_FLOAT or SYM_INTEGER higher above.
					aResultToken.symbol = SYM_INTEGER;
				}
				else
				{
					if (field->mBitSize)
						aResultToken.value_int64 = (unsigned int)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
					else
						aResultToken.value_int64 = *((unsigned int *)((UINT_PTR)target + field->mOffset));
					aResultToken.symbol = SYM_INTEGER;
				}
				break;
			case 8:
				// The below correctly copies both DOUBLE and INT64 into the union.
				// Unsigned 64-bit integers aren't supported because variables/expressions can't support them.
				if (field->mBitSize)
					aResultToken.value_int64 = (__int64)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
				else
					aResultToken.value_int64 = *((__int64 *)((UINT_PTR)target + field->mOffset));
				if (!field->mIsInteger)
					aResultToken.symbol = SYM_FLOAT;
				else
					aResultToken.symbol = SYM_INTEGER;
				break;
			case 2:
				if (!field->mIsUnsigned) // Don't use ternary because that messes up type-casting.
				{
					if (field->mBitSize)
						aResultToken.value_int64 = (short)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
					else
						aResultToken.value_int64 = *((short *)((UINT_PTR)target + field->mOffset));
				}
				else
				{
					if (field->mBitSize)
						aResultToken.value_int64 = (unsigned short)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
					else
						aResultToken.value_int64 = *((unsigned short *)((UINT_PTR)target + field->mOffset));
				}
				aResultToken.symbol = SYM_INTEGER;
				break;
			default: // size 1
				if (!field->mIsUnsigned) // Don't use ternary because that messes up type-casting.
				{
					if (field->mBitSize)
						aResultToken.value_int64 = (char)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
					else
						aResultToken.value_int64 = *((char *)((UINT_PTR)target + field->mOffset));
				}
				else
				{
					if (field->mBitSize)
						aResultToken.value_int64 = (unsigned char)getbits(*((UINT_PTR *)((UINT_PTR)target + field->mOffset)), field->mSize, field->mBitOffset, field->mBitSize, field->mIsUnsigned);
					else
						aResultToken.value_int64 = *((unsigned char *)((UINT_PTR)target + field->mOffset));
				}
				aResultToken.symbol = SYM_INTEGER;
			}
		}
		return OK;
	}
	// The structure doesn't handle this method/property.
	return INVOKE_NOT_HANDLED;
}

//
// Struct:: Internal Methods
//

Struct::FieldType *Struct::FindField(LPTSTR val)
{
	for (int i = 0;i < mFieldCount;i++)
	{
		FieldType &field = mFields[i];
		if (!_tcsicmp(field.key,val))
			return &field;
	}
	return NULL;
}

bool Struct::SetInternalCapacity(IndexType new_capacity)
// Expands mFields to the specified number if fields.
// Caller *must* ensure new_capacity >= 1 && new_capacity >= mFieldCount.
{
	FieldType *new_fields = (FieldType *)realloc(mFields, (size_t)new_capacity * sizeof(FieldType));
	if (!new_fields)
		return false;
	mFields = new_fields;
	mFieldCountMax = new_capacity;
	return true;
}

ResultType Struct::_NewEnum(ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount)
{
	if (aParamCount == 0)
	{
		IObject *newenum;
		if (newenum = new Enumerator(this))
		{
			aResultToken.symbol = SYM_OBJECT;
			aResultToken.object = newenum;
		}
	}
	return OK;
}


int Struct::Enumerator::Next(Var *aKey, Var *aVal)
{
	if (++mOffset < mObject->mFieldCount && *mObject->mFields[0].key)
	{
		FieldType &field = mObject->mFields[mOffset];
		if (aKey)
			aKey->Assign(field.key);
		if (aVal)
		{	// We need to invoke the structure to retrieve the value
			ExprTokenType aResultToken;
			// not sure about the buffer
			TCHAR buf[MAX_PATH];
			aResultToken.buf = buf;
			ExprTokenType aThisToken;
			aThisToken.symbol = SYM_OBJECT;
			aThisToken.object = mObject;
			ExprTokenType *aVarToken = new ExprTokenType();
			aVarToken->symbol = SYM_STRING;
			aVarToken->marker = field.key;
			mObject->Invoke(aResultToken, aThisToken, 0, &aVarToken, 1);
			switch (aResultToken.symbol)
			{
			case SYM_STRING:	aVal->AssignString(aResultToken.marker);	break;
			case SYM_INTEGER:	aVal->Assign(aResultToken.value_int64);			break;
			case SYM_FLOAT:		aVal->Assign(aResultToken.value_double);		break;
			case SYM_OBJECT:	aVal->Assign(aResultToken.object);			break;
			}
			if (aResultToken.symbol == SYM_OBJECT)
				aResultToken.object->Release();
			delete aVarToken;
		}
		return true;
	}
	else if (mOffset < mObject->mFields[0].mArraySize)
	{	// structure is an array
		if (aKey)
			aKey->Assign(mOffset + 1); // mOffset starts at 1
		if (aVal)
		{	// again we need to invoke structure to retrieve the value
			ExprTokenType aResultToken;
			TCHAR buf[MAX_PATH];
			aResultToken.buf = buf;
			ExprTokenType aThisToken;
			aThisToken.symbol = SYM_OBJECT;
			aThisToken.object = mObject;
			ExprTokenType *aVarToken = new ExprTokenType();
			aVarToken->symbol = SYM_INTEGER;
			aVarToken->value_int64 = mOffset + 1; // mOffset starts at 1
			mObject->Invoke(aResultToken, aThisToken, 0, &aVarToken, 1);
			switch (aResultToken.symbol)
			{
			case SYM_STRING:	aVal->AssignString(aResultToken.marker);	break;
			case SYM_INTEGER:	aVal->Assign(aResultToken.value_int64);			break;
			case SYM_FLOAT:		aVal->Assign(aResultToken.value_double);		break;
			case SYM_OBJECT:	aVal->Assign(aResultToken.object);			break;
			}
			if (aResultToken.symbol == SYM_OBJECT)
				aResultToken.object->Release();
			delete aVarToken;
		}
		return true;
	}
	return false;
}