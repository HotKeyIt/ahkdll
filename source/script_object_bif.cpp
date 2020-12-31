#include "stdafx.h" // pre-compiled headers
#include "defines.h"
#include "globaldata.h"
#include "script.h"
#include "script_func_impl.h"
#include "script_object.h"
#include "application.h"

//
// BIF_Struct - Create structure
//

BIF_DECL(BIF_Struct)
{
	// At least the definition for structure must be given
	if (!aParamCount)
		return;
	Struct *obj = Struct::Create(aParam, aParamCount);
	if (obj)
	{
		aResultToken.symbol = SYM_OBJECT;
		aResultToken.object = obj;
		return;
		// DO NOT ADDREF: after we return, the only reference will be in aResultToken.
	}
	else
	{
		aResult = FAIL;
		return;
	}
}

//
// sizeof_maxsize - get max size of union or structure, helper function for BIF_Sizeof and BIF_Struct
//
BYTE sizeof_maxsize(TCHAR *buf)
{
	ResultType Result = OK;
	ExprTokenType ResultToken;
	int align = 0;
	ExprTokenType Var1, Var2, Var3;
	Var1.symbol = SYM_STRING;
	TCHAR tempbuf[LINE_SIZE];
	Var1.marker = tempbuf;
	Var2.symbol = SYM_INTEGER;
	Var2.value_int64 = 0;
	// used to pass aligntotal counter to structure in structure
	Var3.symbol = SYM_INTEGER;
	Var3.value_int64 = (__int64)&align;
	ExprTokenType *param[] = { &Var1, &Var2, &Var3};
	int depth = 0;
	int max = 0,thissize = 0;
	LPCTSTR comments = 0;
	for (int i = 0;buf[i];)
	{
		if (buf[i] == '{')
		{
			depth++;
			i++;
		}
		else if (buf[i] == '}')
		{
			if (--depth < 1)
				break;
			i++;
		}
		else if (StrChrAny(&buf[i], _T(";, \t\n\r")) == &buf[i])
		{
			i++;
			continue;
		}
		else if (comments = RegExMatch(&buf[i], _T("i)^(union|struct)?\\s*(\\{|\\})\\s*\\K")))
		{
			i += (int)(comments - &buf[i]);
			continue;
		}
		else if (buf[i] == '\\' && buf[i + 1] == '\\')
		{
			if (!(comments = StrChrAny(&buf[i], _T("\r\n"))))
				break; // end of structure
			i += (int)(comments - &buf[i]);
			continue;
		}
		else
		{
			if (StrChrAny(&buf[i], _T(";,")))
			{
				_tcsncpy(tempbuf, &buf[i], thissize = (int)(StrChrAny(&buf[i], _T(";,}")) - &buf[i]));
				i += thissize + (buf[i+thissize] == '}' ? 0 : 1);
			}
			else
			{
				_tcscpy(tempbuf, &buf[i]);
				thissize = (int)_tcslen(&buf[i]);
				i += thissize;
			}
			*(tempbuf + thissize) = '\0';
			if (StrChrAny(tempbuf, _T("\t ")))
			{
				align = 0;
 				BIF_sizeof(Result, ResultToken, param, 3);
				if (max < align)
					max = (BYTE)align;
			}
			else if (max < 4)
				max = 4;
		}
	}
	return max;
}

//
// BIF_sizeof - sizeof() for structures and default types
//

BIF_DECL(BIF_sizeof)
// This code is very similar to BIF_Struct so should be maintained together
{
	int ptrsize = sizeof(UINT_PTR); // Used for pointers on 32/64 bit system
	int offset = 0;					// also used to calculate total size of structure
	int mod = 0;
	int arraydef = 0;				// count arraysize to update offset
	int unionoffset[16] = { 0 };			// backup offset before we enter union or structure
	int unionsize[16] = { 0 };				// calculate unionsize
	bool unionisstruct[16] = { 0 };		// updated to move offset for structure in structure
	int structalign[16] = { 0 };			// keep track of struct alignment
	int totalunionsize = 0;			// total size of all unions and structures in structure
	int uniondepth = 0;				// count how deep we are in union/structure
	int align = 0;
	int *aligntotal = &align;		// pointer alignment for total structure
	int thissize = 0;					// used to save size returned from IsDefaultType
	int maxsize = 0;				// max size of union or struct
	int toalign = 0;				// custom alignment
	int thisalign = 0;

	// following are used to find variable and also get size of a structure defined in variable
	// this will hold the variable reference and offset that is given to size() to align if necessary in 64-bit
	ResultType Result = OK;
	ExprTokenType ResultToken;
	ExprTokenType Var1,Var2,Var3,Var4;
	Var1.symbol = SYM_VAR;
	Var2.symbol = SYM_INTEGER;

	// used to pass aligntotal counter to structure in structure
	Var3.symbol = SYM_INTEGER;
	Var3.value_int64 = (__int64)&align;
	Var4.symbol = SYM_INTEGER;
	ExprTokenType *param[] = {&Var1,&Var2,&Var3,&Var4};
	
	// will hold pointer to structure definition while we parse it
	TCHAR *buf;
	size_t buf_size;
	// Should be enough buffer to accept any definition and name.
	TCHAR tempbuf[LINE_SIZE]; // just in case if we have a long comment

	// definition and field name are same max size as variables
	// also add enough room to store pointers (**) and arrays [1000]
	// give more room to use local or static variable Function(variable)
	// Parameter passed to IsDefaultType needs to be ' Definition '
	// this is because spaces are used as delimiters ( see IsDefaultType function )
	TCHAR defbuf[MAX_VAR_NAME_LENGTH*2 + 40] = _T(" UInt "); // Set default UInt definition
	
	// buffer for arraysize + 2 for bracket ] and terminating character
	TCHAR intbuf[MAX_INTEGER_LENGTH + 2];

	LPTSTR bitfield = NULL;
	BYTE bitsize = 0;
	BYTE bitsizetotal = 0;
	LPTSTR isBit;

	// Set result to empty string to identify error
	aResultToken.symbol = SYM_STRING;
	aResultToken.marker = _T("");
	
	// Parameter passed to IsDefaultType needs to be ' Definition '
	// this is because spaces are used as delimiters ( see IsDefaultType function )
	// So first character will be always a space
	defbuf[0] = ' ';
	
	// if first parameter is an object (struct), simply return its size
	if (TokenToObject(*aParam[0]))
	{
		aResultToken.symbol = SYM_INTEGER;
		Struct *obj = (Struct*)TokenToObject(*aParam[0]);
		aResultToken.value_int64 = obj->mStructSize + (aParamCount > 1 ? TokenToInt64(*aParam[1],true) : 0);
		return;
	}

	if (aParamCount > 1 && TokenIsPureNumeric(*aParam[1]))
	{	// an offset was given, set starting offset 
		offset = (int)TokenToInt64(*aParam[1]);
		Var2.value_int64 = (__int64)offset;
	}
	if (aParamCount > 2 && TokenIsPureNumeric(*aParam[2]))
	{   // a pointer was given to return memory to align
		aligntotal = (int*)TokenToInt64(*aParam[2], true);
		Var3.value_int64 = (__int64)aligntotal;
	}
	// Set buf to beginning of structure definition
	buf = TokenToString(*aParam[0]);


	toalign = ATOI(buf);
	TCHAR alignbuf[MAX_INTEGER_LENGTH];
	if (*(buf + _tcslen(ITOA(toalign, alignbuf))) != ':' || toalign <= 0)
		Var4.value_int64 = toalign = 0;
	else
	{
		buf += _tcslen(alignbuf) + 1;
		Var4.value_int64 = toalign;
	}

	if (aParamCount > 3 && TokenIsPureNumeric(*aParam[3]))
	{   // a pointer was given to return memory to align
		toalign = (int)TokenToInt64(*aParam[3], true);
		Var4.value_int64 = (__int64)toalign;
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
		{   // union or structure in structure definition
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
			structalign[uniondepth] = *aligntotal > thisalign ? STRUCTALIGN(*aligntotal) : thisalign;
			*aligntotal = 0;
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
			if (structalign[uniondepth] > *aligntotal)
				*aligntotal = structalign[uniondepth];
			if (unionsize[uniondepth]>totalunionsize)
				totalunionsize = unionsize[uniondepth];
			// last item in union or structure, update offset now if not struct, for struct offset is up to date
			if (--uniondepth == 0)
			{
				// end of structure, align it
				//if (mod = totalunionsize % *aligntotal)
				//	totalunionsize += (*aligntotal - mod) % *aligntotal;
				// correct offset
				offset += totalunionsize;
			}
			bitsizetotal = bitsize = 0;
			buf++;
			if (buf == StrChrAny(buf,_T(";,")))
				buf++;
			continue;
		}
		// set default
		arraydef = 0;

		// copy current definition field to temporary buffer
		if (StrChrAny(buf, _T("};,")))
		{
			if ((buf_size = _tcscspn(buf, _T("};,"))) > LINE_SIZE - 1)
			{
				g_script.ScriptError(ERR_INVALID_STRUCT, buf);
				return;
			}
			_tcsncpy(tempbuf,buf,buf_size);
			tempbuf[buf_size] = '\0';
		}
		else if (_tcslen(buf) > LINE_SIZE - 1)
		{
			g_script.ScriptError(ERR_INVALID_STRUCT, buf);
			return;
		}
		else
			_tcscpy(tempbuf,buf);
		
		// Trim trailing spaces
		rtrim(tempbuf);

		// Array
		if (_tcschr(tempbuf,'['))
		{
			_tcsncpy(intbuf,_tcschr(tempbuf,'['),MAX_INTEGER_LENGTH);
			intbuf[_tcscspn(intbuf,_T("]")) + 1] = '\0';
			arraydef = (int)ATOI64(intbuf + 1);
			// remove array definition
			StrReplace(tempbuf, intbuf, _T(""), SCS_SENSITIVE, 1, LINE_SIZE);
		}
		if (_tcschr(tempbuf, '['))
		{	// array to array and similar not supported
			g_script.ScriptError(ERR_INVALID_STRUCT, tempbuf);
			return;
		}
		// Pointer, while loop will continue here because we only need size
		if (_tcschr(tempbuf,'*'))
		{
			if (_tcschr(tempbuf, ':'))
			{
				g_script.ScriptError(ERR_INVALID_STRUCT_BIT_POINTER, tempbuf);
				return;
			}
			// align offset for pointer
			if (mod = offset % STRUCTALIGN(ptrsize))
				offset += (thisalign - mod) % thisalign;
			offset += ptrsize * (arraydef ? arraydef : 1);
			if (thisalign > *aligntotal)
				*aligntotal = thisalign;
			// update offset
			if (uniondepth)
			{
				if ((maxsize = offset - unionoffset[uniondepth]) > unionsize[uniondepth])
					unionsize[uniondepth] = maxsize;
				// reset offset if in union and union is not a structure
				if (!unionisstruct[uniondepth])
					offset = unionoffset[uniondepth];
			}

			// Move buffer pointer now and continue
			if (_tcschr(buf,'}') && (!StrChrAny(buf, _T(";,")) || _tcschr(buf,'}') < StrChrAny(buf, _T(";,"))))
				buf += _tcscspn(buf,_T("}")); // keep } character to update union
			else if (StrChrAny(buf, _T(";,")))
				buf += _tcscspn(buf,_T(";,")) + 1;
			else
				buf += _tcslen(buf);
			continue;
		}
		
		// if offset is 0 and there are no };, characters, it means we have a pure definition
		if (StrChrAny(tempbuf, _T(" \t")) || StrChrAny(tempbuf,_T("};,")) || (!StrChrAny(buf,_T("};,")) && !offset))
		{
			if ((buf_size = _tcscspn(tempbuf,_T("\t ["))) > MAX_VAR_NAME_LENGTH*2 + 30)
			{
				g_script.ScriptError(ERR_INVALID_STRUCT, tempbuf);
				return;
			}
			isBit = StrChrAny(omit_leading_whitespace(tempbuf), _T(" \t"));
			if (!isBit || *isBit != ':')
			{
				if (_tcsnicmp(defbuf + 1, tempbuf, _tcslen(defbuf) - 2))
					bitsizetotal = bitsize = 0;
				_tcsncpy(defbuf + 1, tempbuf, _tcscspn(tempbuf, _T("\t [")));
				_tcscpy(defbuf + 1 + _tcscspn(tempbuf, _T("\t [")), _T(" "));
			}
			if (bitfield = _tcschr(tempbuf, ':'))
			{
				if (bitsizetotal / 8 == thissize)
					bitsizetotal = bitsize = 0;
				bitsizetotal += bitsize = ATOI(bitfield + 1);
			}
			else
				bitsizetotal = bitsize = 0;
		}
		else // Not 'TypeOnly' definition because there are more than one fields in structure so use default type UInt
		{
			// Commented out following line to keep previous or default UInt definition like in c++, e.g. "Int x,y,Char a,b", 
			// Note: separator , or ; can be still used but
			// _tcscpy(defbuf,_T(" UInt "));
			if (bitfield = _tcschr(tempbuf, ':'))
			{
				if (bitsizetotal / 8 == thissize)
					bitsizetotal = bitsize = 0;
				bitsizetotal += bitsize = ATOI(bitfield + 1);
			}
			else
				bitsizetotal = bitsize = 0;
		}

		// Now find size in default types array and create new field
		// If Type not found, resolve type to variable and get size of struct defined in it
		if ((!_tcscmp(defbuf, _T(" bool ")) && (thissize = 1)) || (thissize = IsDefaultType(defbuf)))
		{
			// align offset
			if ((!bitsize || bitsizetotal == bitsize) && thissize > 1 && (mod = offset % STRUCTALIGN(thissize)))
				offset += (thisalign - mod) % thisalign;
			if (!bitsize || bitsizetotal == bitsize)
				offset += thissize * (arraydef ? arraydef : 1);
			if (thisalign > *aligntotal)
				*aligntotal = thisalign; // > ptrsize ? ptrsize : thissize;
		}
		else // type was not found, check for user defined type in variables
		{
			Var1.var = NULL;
			Func *bkpfunc = NULL;
			// check if we have a local/static declaration and resolve to function
			// For example Struct("MyFunc(mystruct) mystr")
			if (_tcschr(defbuf,'('))
			{
				bkpfunc = g->CurrentFunc; // don't bother checking, just backup and restore later
				g->CurrentFunc = g_script.FindFunc(defbuf + 1,_tcscspn(defbuf,_T("(")) - 1);
				if (g->CurrentFunc) // break if not found to identify error
				{
					_tcscpy(tempbuf,defbuf + 1);
					_tcscpy(defbuf + 1,tempbuf + _tcscspn(tempbuf,_T("(")) + 1); //,_tcschr(tempbuf,')') - _tcschr(tempbuf,'('));
					_tcscpy(_tcschr(defbuf,')'),_T(" \0"));
					Var1.var = g_script.FindVar(defbuf + 1,_tcslen(defbuf) - 2,NULL,FINDVAR_LOCAL,NULL);
					g->CurrentFunc = bkpfunc;
				}
				else // release object and return
				{
					g->CurrentFunc = bkpfunc;
					g_script.ScriptError(ERR_INVALID_STRUCT_IN_FUNC, defbuf);
					return;
				}
			}
			else if (g->CurrentFunc) // try to find local variable first
				Var1.var = g_script.FindVar(defbuf + 1,_tcslen(defbuf) - 2,NULL,FINDVAR_LOCAL,NULL);
			// try to find global variable if local was not found or we are not in func
			if (Var1.var == NULL)
				Var1.var = g_script.FindVar(defbuf + 1,_tcslen(defbuf) - 2,NULL,FINDVAR_GLOBAL,NULL);
			if (Var1.var != NULL)
			{
				// Call BIF_sizeof passing offset in second parameter to align if necessary
				int newaligntotal = sizeof_maxsize(TokenToString(Var1));
				if (STRUCTALIGN(newaligntotal) > *aligntotal)
					*aligntotal = thisalign;
				if ((!bitsize || bitsizetotal == bitsize) && offset && (mod = offset % *aligntotal))
					offset += (*aligntotal - mod) % *aligntotal;
				param[1]->value_int64 = (__int64)offset;
				BIF_sizeof(Result,ResultToken,param,3);
				if (ResultToken.symbol != SYM_INTEGER)
				{	// could not resolve structure
					g_script.ScriptError(ERR_INVALID_STRUCT, defbuf);
					return;
				}
				// sizeof was given an offset that it applied and aligned if necessary, so set offset =  and not +=
				if (!bitsize || bitsizetotal == bitsize)
					offset = (int)ResultToken.value_int64 + (arraydef ? ((arraydef - 1) * ((int)ResultToken.value_int64 - offset)) : 0);
			}
			else // No variable was found and it is not default type so we can't determine size, return empty string.
			{
				g_script.ScriptError(ERR_INVALID_STRUCT, defbuf);
				return;
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
	if (*aligntotal && (mod = offset % *aligntotal)) // align even if offset was given e.g. for _NMHDR:="HWND hwndFrom,UINT_PTR idFrom,UINT code", _NMTVGETINFOTIP: = "_NMHDR hdr,UINT uFlags,UInt link"
		offset += (*aligntotal - mod) % *aligntotal;
	aResultToken.symbol = SYM_INTEGER;
	aResultToken.value_int64 = offset;
}

//
// ObjRawSize()
//

__int64 ObjRawSize(IObject *aObject, bool aCopyBuffer, IObject *aObjects)
{
	ExprTokenType Result, this_token, enum_token, aCall, aKey, aValue;
	ExprTokenType *params[] = { &aCall, &aKey, &aValue };
	TCHAR buf[MAX_PATH];
	
	// Set up enum_token the way Invoke expects:
	enum_token.symbol = SYM_STRING;
	enum_token.marker = _T("");
	enum_token.mem_to_free = NULL;
	enum_token.buf = buf;

	// Prepare to call object._NewEnum():
	aCall.symbol = SYM_STRING;
	aCall.marker = _T("_NewEnum");

	aObject->Invoke(enum_token, Result, IT_CALL, params, 1);

	if (enum_token.mem_to_free)
		// Invoke returned memory for us to free.
		free(enum_token.mem_to_free);

	// Check if object returned an enumerator, otherwise return
	if (enum_token.symbol != SYM_OBJECT)
		return 0;

	// create variables to use in for loop / for enumeration
	// these will be deleted afterwards

	Var *var1 = (Var*)alloca(sizeof(Var));
	Var *var2 = (Var*)alloca(sizeof(Var));
	var1->mType = var2->mType = VAR_NORMAL;
	var1->mAttrib = var2->mAttrib = 0;
	var1->mByteCapacity = var2->mByteCapacity = 0;
	var1->mHowAllocated = var2->mHowAllocated = ALLOC_MALLOC;

	if (!aObjects)
	{
		aCall.symbol = SYM_OBJECT;
		aCall.object = aObject;
		aKey.symbol = SYM_STRING;
		aKey.marker = _T("");
		aKey.marker_length = 0;
		aObjects = Object::Create(params, 2);
	}
	else
	{
		aObjects->AddRef();
		aCall.marker = _T("HasKey");
		aKey.symbol = SYM_OBJECT;
		aKey.object = aObject;
		aObjects->Invoke(Result, this_token, IT_CALL, params, 2);
		if (!Result.value_int64)
		{
			aCall.symbol = SYM_OBJECT;
			aCall.object = aObject;
			aKey.symbol = SYM_STRING;
			aKey.marker = _T("");
			aKey.marker_length = 0;
			aObjects->Invoke(Result, this_token, IT_SET, params, 2);
		}
	}
	// Prepare parameters for the loop below: enum.Next(var1 [, var2])
	aCall.symbol = SYM_STRING;
	aCall.marker = _T("Next");
	aKey.symbol = SYM_VAR;
	aKey.var = var1;
	aKey.var->mCharContents = _T("");
	aKey.mem_to_free = 0;
	aValue.symbol = SYM_VAR;
	aValue.var = var2;
	aValue.var->mCharContents = _T("");
	aValue.mem_to_free = 0;

	ExprTokenType result_token;
	IObject &enumerator = *enum_token.object; // Might perform better as a reference?
	__int64 aSize = 0;
	IObject *aIsObject;
	__int64 aIsValue;
	SymbolType aVarType;

	for (;;)
	{
		// Set up result_token the way Invoke expects; each Invoke() will change some or all of these:
		result_token.symbol = SYM_STRING;
		result_token.marker = _T("");
		result_token.mem_to_free = NULL;
		result_token.buf = buf;

		// Call enumerator.Next(var1, var2)
		enumerator.Invoke(result_token, enum_token, IT_CALL, params, 3);

		bool next_returned_true = TokenToBOOL(result_token);
		if (!next_returned_true)
			break;

		if (aIsObject = TokenToObject(aKey))
		{
			aCall.marker = _T("HasKey");
			aObjects->Invoke(Result, this_token, IT_CALL, params, 2);
			if (Result.value_int64)
				aSize += 9;
			else
				aSize += ObjRawSize(aIsObject, aCopyBuffer, aObjects) + 9;
			aCall.marker = _T("Next");
		}
		else if ((aVarType = aKey.var->IsNonBlankIntegerOrFloat()) == SYM_STRING || RegExMatch(aKey.var->Contents(), _T("\\s")) || _tcscmp(ITOA64(ATOI64(aKey.var->Contents()), buf), aKey.var->Contents()))
			aSize += (aKey.var->ByteLength() ? aKey.var->ByteLength() + sizeof(TCHAR) : 0) + 9;
		else
			aSize += (aVarType == SYM_FLOAT || (aIsValue = TokenToInt64(aKey)) > 4294967295) ? 9 : aIsValue > 65535 ? 5 : aIsValue > 255 ? 3 : aIsValue > -129 ? 2 : aIsValue > -32769 ? 3 : aIsValue >= INT_MIN ? 5 : 9;

		if (aIsObject = TokenToObject(aValue))
		{
			aCall.marker = _T("HasKey");
			aKey.symbol = SYM_OBJECT;
			aKey.object = aIsObject;
			aObjects->Invoke(Result, this_token, IT_CALL, params, 2);
			if (Result.value_int64)
				aSize += 9;
			else
				aSize += ObjRawSize(aIsObject, aCopyBuffer, aObjects) + 9;
			aCall.marker = _T("Next");
		}
		else if ((aVarType = aValue.var->IsNonBlankIntegerOrFloat()) == SYM_STRING || RegExMatch(aValue.var->Contents(), _T("\\s")) || _tcscmp(ITOA64(ATOI64(aValue.var->Contents()), buf), aValue.var->Contents()))
		{
			if (aCopyBuffer)
			{
				aCall.marker = _T("GetCapacity");
				aObject->Invoke(Result, this_token, IT_CALL, params, 2);
				aSize += Result.value_int64 + 9;
				aCall.marker = _T("Next");
			}
			else
				aSize += (aValue.var->ByteLength() ? aValue.var->ByteLength() + sizeof(TCHAR) : 0) + 9;
		}
		else
			aSize += aVarType == SYM_FLOAT || (aIsValue = TokenToInt64(aValue)) > 4294967295 ? 9 : aIsValue > 65535 ? 5 : aIsValue > 255 ? 3 : aIsValue > -129 ? 2 : aIsValue > -32769 ? 3 : aIsValue >= INT_MIN ? 5 : 9;

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

		aKey.symbol = SYM_VAR;
		aKey.var = var1;
		aValue.symbol = SYM_VAR;
		aValue.var = var2;
	}
	// release enumerator and free vars
	enumerator.Release();
	var1->Free();
	var2->Free();
	aObjects->Release();
	return aSize;
}

//
// ObjRawDump()
//

__int64 ObjRawDump(IObject *aObject, char *aBuffer, bool aCopyBuffer, IObject *aObjects, UINT &aObjCount)
{
	char *aThisBuffer = aBuffer;

	ExprTokenType Result, this_token, enum_token, aCall, aKey, aValue;
	ExprTokenType *params[] = { &aCall, &aKey, &aValue };
	TCHAR buf[MAX_PATH];

	// Set up enum_token the way Invoke expects:
	enum_token.symbol = SYM_STRING;
	enum_token.marker = _T("");
	enum_token.mem_to_free = NULL;
	enum_token.buf = buf;

	// Prepare to call object._NewEnum():
	aCall.symbol = SYM_STRING;
	aCall.marker = _T("_NewEnum");

	aObject->Invoke(enum_token, Result, IT_CALL, params, 1);

	if (enum_token.mem_to_free)
		// Invoke returned memory for us to free.
		free(enum_token.mem_to_free);

	// Check if object returned an enumerator, otherwise return
	if (enum_token.symbol != SYM_OBJECT)
		return NULL;

	// create variables to use in for loop / for enumeration
	// these will be deleted afterwards

	Var *var1 = (Var*)alloca(sizeof(Var));
	Var *var2 = (Var*)alloca(sizeof(Var));
	var1->mType = var2->mType = VAR_NORMAL;
	var1->mAttrib = var2->mAttrib = 0;
	var1->mByteCapacity = var2->mByteCapacity = 0;
	var1->mHowAllocated = var2->mHowAllocated = ALLOC_MALLOC;

	aCall.marker = _T("HasKey");
	aKey.symbol = SYM_OBJECT;
	aKey.object = aObject;
	aObjects->Invoke(Result, this_token, IT_CALL, params, 2);
	if (!Result.value_int64)
	{
		aCall.symbol = SYM_OBJECT;
		aCall.object = aObject;
		aKey.symbol = SYM_INTEGER;
		aKey.value_int64 = aObjCount++;
		aObjects->Invoke(Result, this_token, IT_SET, params, 2);
	}

	// Prepare parameters for the loop below: enum.Next(var1 [, var2])
	aCall.symbol = SYM_STRING;
	aCall.marker = _T("Next");
	aKey.symbol = SYM_VAR;
	aKey.var = var1;
	aKey.var->mCharContents = _T("");
	aKey.mem_to_free = 0;
	aValue.symbol = SYM_VAR;
	aValue.var = var2;
	aValue.var->mCharContents = _T("");
	aValue.mem_to_free = 0;

	ExprTokenType result_token;
	IObject &enumerator = *enum_token.object; // Might perform better as a reference?
	IObject *aIsObject;
	__int64 aIsValue;
	__int64 aThisSize;
	SymbolType aVarType;

	for (;;)
	{
		// Set up result_token the way Invoke expects; each Invoke() will change some or all of these:
		result_token.symbol = SYM_STRING;
		result_token.marker = _T("");
		result_token.mem_to_free = NULL;
		result_token.buf = buf;

		// Call enumerator.Next(var1, var2)
		enumerator.Invoke(result_token, enum_token, IT_CALL, params, 3);

		bool next_returned_true = TokenToBOOL(result_token);
		if (!next_returned_true)
			break;

		// copy Key
		if (aIsObject = TokenToObject(aKey))
		{
			aCall.marker = _T("HasKey");
			aObjects->Invoke(Result, this_token, IT_CALL, params, 2);
			aCall.marker = _T("Next");
			if (Result.value_int64)
			{
				aObjects->Invoke(Result, this_token, IT_GET, params + 1, 1);
				*aThisBuffer = (char)-12;
				aThisBuffer += 1;
				*(__int64*)aThisBuffer = Result.value_int64;
				aThisBuffer += sizeof(__int64);
			}
			else
			{
				*aThisBuffer = (char)-11;
				aThisBuffer += 1;
				*(__int64*)aThisBuffer = aThisSize = ObjRawDump(aIsObject, aThisBuffer + sizeof(__int64), aCopyBuffer, aObjects, aObjCount);
				aThisBuffer += aThisSize + sizeof(__int64);
			}
		}
		else if ((aVarType = aKey.var->IsNonBlankIntegerOrFloat()) == SYM_STRING || RegExMatch(aKey.var->Contents(), _T("\\s")) || _tcscmp(ITOA64(ATOI64(aKey.var->Contents()), buf), aKey.var->Contents()))
		{
			*aThisBuffer = (char)-10;
			aThisBuffer += 1;
			*(__int64*)aThisBuffer = aThisSize = (__int64)(aKey.var->ByteLength() ? aKey.var->ByteLength() + sizeof(TCHAR) : 0);
			aThisBuffer += sizeof(__int64);
			if (aThisSize)
			{
				memcpy(aThisBuffer, aKey.var->Contents(), (size_t)aThisSize);
				aThisBuffer += aThisSize;
			}
		}
		else if (aVarType == SYM_FLOAT)
		{
			*aThisBuffer = (char)-9;
			aThisBuffer += 1;
			*(double*)aThisBuffer = TokenToDouble(aKey);
			aThisBuffer += sizeof(__int64);
		}
		else if ((aIsValue = TokenToInt64(aKey)) > 4294967295)
		{
			*aThisBuffer = (char)-8;
			aThisBuffer += 1;
			*(__int64*)aThisBuffer = aIsValue;
			aThisBuffer += sizeof(__int64);
		}
		else if (aIsValue > 65535)
		{
			*aThisBuffer = (char)-6;
			aThisBuffer += 1;
			*(UINT*)aThisBuffer = (UINT)aIsValue;
			aThisBuffer += sizeof(UINT);
		}
		else if (aIsValue > 255)
		{
			*aThisBuffer = (char)-4;
			aThisBuffer += 1;
			*(USHORT*)aThisBuffer = (USHORT)aIsValue;
			aThisBuffer += sizeof(USHORT);
		}
		else if (aIsValue > -1)
		{
			*aThisBuffer = (char)-2;
			aThisBuffer += 1;
			*aThisBuffer = (BYTE)aIsValue;
			aThisBuffer += sizeof(BYTE);
		}
		else if (aIsValue > -129)
		{
			*aThisBuffer = (char)-1;
			aThisBuffer += 1;
			*aThisBuffer = (char)aIsValue;
			aThisBuffer += sizeof(char);
		}
		else if (aIsValue > -32769)
		{
			*aThisBuffer = (char)-3;
			aThisBuffer += 1;
			*(short*)aThisBuffer = (short)aIsValue;
			aThisBuffer += sizeof(short);
		}
		else if (aIsValue >= INT_MIN)
		{
			*aThisBuffer = (char)-5;
			aThisBuffer += 1;
			*(int*)aThisBuffer = (int)aIsValue;
			aThisBuffer += sizeof(int);
		}
		else
		{
			*aThisBuffer = (char)-7;
			aThisBuffer += 1;
			*(__int64*)aThisBuffer = (__int64)aIsValue;
			aThisBuffer += sizeof(__int64);
		}

		// copy Value
		if (aIsObject = TokenToObject(aValue))
		{
			aCall.marker = _T("HasKey");
			aKey.symbol = SYM_OBJECT;
			aKey.object = aIsObject;
			aObjects->Invoke(Result, this_token, IT_CALL, params, 2);
			aCall.marker = _T("Next");
			if (Result.value_int64)
			{
				aObjects->Invoke(Result, this_token, IT_GET, params + 2, 1);
				*aThisBuffer = (char)12;
				aThisBuffer += 1;
				*(__int64*)aThisBuffer = Result.value_int64;
				aThisBuffer += sizeof(__int64);
			}
			else
			{
				*aThisBuffer = (char)11;
				aThisBuffer += 1;
				*(__int64*)aThisBuffer = aThisSize = ObjRawDump(aIsObject, aThisBuffer + sizeof(__int64), aCopyBuffer, aObjects, aObjCount);
				aThisBuffer += aThisSize + sizeof(__int64);
			}
		}
		else if ((aVarType = aValue.var->IsNonBlankIntegerOrFloat()) == SYM_STRING || RegExMatch(aValue.var->Contents(), _T("\\s")) || _tcscmp(ITOA64(ATOI64(aValue.var->Contents()), buf), aValue.var->Contents()))
		{
			*aThisBuffer = (char)10;
			aThisBuffer += 1;
			if (aCopyBuffer)
			{
				aCall.marker = _T("GetCapacity");
				aObject->Invoke(Result, this_token, IT_CALL, params, 2);
				*(__int64*)aThisBuffer = aThisSize = Result.value_int64;
				aThisBuffer += sizeof(__int64);
				if (aThisSize)
				{
					aCall.marker = _T("GetAddress");
					aObject->Invoke(Result, this_token, IT_CALL, params, 2);
					memcpy(aThisBuffer, (char*)Result.value_int64, (size_t)aThisSize);
				}
				aCall.marker = _T("Next");
			}
			else
			{
				*(__int64*)aThisBuffer = aThisSize = (__int64)(aValue.var->ByteLength() ? aValue.var->ByteLength() + sizeof(TCHAR) : 0);
				aThisBuffer += sizeof(__int64);
				if (aThisSize)
					memcpy(aThisBuffer, aValue.var->Contents(), (size_t)aThisSize);
			}
			aThisBuffer += aThisSize;
		}
		else if (aVarType == SYM_FLOAT)
		{
			*aThisBuffer = (char)9;
			aThisBuffer += 1;
			*(double*)aThisBuffer = TokenToDouble(aValue);
			aThisBuffer += sizeof(double);
		}
		else if ((aIsValue = TokenToInt64(aValue)) > 4294967295)
		{
			*aThisBuffer = (char)8;
			aThisBuffer += 1;
			*(__int64*)aThisBuffer = aIsValue;
			aThisBuffer += sizeof(__int64);
		}
		else if (aIsValue > 65535)
		{
			*aThisBuffer = (char)6;
			aThisBuffer += 1;
			*(UINT*)aThisBuffer = (UINT)aIsValue;
			aThisBuffer += sizeof(UINT);
		}
		else if (aIsValue > 255)
		{
			*aThisBuffer = (char)4;
			aThisBuffer += 1;
			*(USHORT*)aThisBuffer = (USHORT)aIsValue;
			aThisBuffer += sizeof(USHORT);
		}
		else if (aIsValue > -1)
		{
			*aThisBuffer = (char)2;
			aThisBuffer += 1;
			*aThisBuffer = (BYTE)aIsValue;
			aThisBuffer += sizeof(BYTE);
		}
		else if (aIsValue > -129)
		{
			*aThisBuffer = (char)1;
			aThisBuffer += 1;
			*aThisBuffer = (char)aIsValue;
			aThisBuffer += sizeof(char);
		}
		else if (aIsValue > -32769)
		{
			*aThisBuffer = (char)3;
			aThisBuffer += 1;
			*(short*)aThisBuffer = (short)aIsValue;
			aThisBuffer += sizeof(short);
		}
		else if (aIsValue > INT_MIN)
		{
			*aThisBuffer = (char)5;
			aThisBuffer += 1;
			*aThisBuffer = (int)aIsValue;
			aThisBuffer += sizeof(int);
		}
		else
		{
			*aThisBuffer = (char)7;
			aThisBuffer += 1;
			*(__int64*)aThisBuffer = (__int64)aIsValue;
			aThisBuffer += sizeof(__int64);
		}

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

		aKey.symbol = SYM_VAR;
		aKey.var = var1;
		aValue.symbol = SYM_VAR;
		aValue.var = var2;
	}
	// release enumerator and free vars
	enumerator.Release();
	var1->Free();
	var2->Free();
	return aThisBuffer - aBuffer;
}

//
// ObjDump()
//

BIF_DECL(BIF_ObjDump)
{
	aResultToken.symbol = SYM_INTEGER;
	IObject *aObject;
	if (!(aObject = TokenToObject(*aParam[0])) && !(aObject = TokenToObject(*aParam[1])))
	{
		aResultToken.symbol = SYM_STRING;
		aResultToken.marker = _T("");
		return;
	}
	INT aCopyBuffer = aParamCount > 2 ? (int)TokenToInt64(*aParam[2]) : 0;
	DWORD aSize = (DWORD)ObjRawSize(aObject, (aCopyBuffer == 1 || aCopyBuffer == 3), NULL);
	char *aBuffer = (char*)malloc(aSize + sizeof(__int64));
	if (!aBuffer)
	{
		g_script.ScriptError(ERR_OUTOFMEM);
		aResultToken.symbol = SYM_STRING;
		aResultToken.marker = _T("");
		return;
	}
	*(__int64*)aBuffer = aSize;
	IObject *aObjects = Object::Create();
	UINT aObjCount = 0;
	if (aSize != ObjRawDump(aObject, aBuffer + sizeof(__int64), (aCopyBuffer == 1 || aCopyBuffer == 3), aObjects, aObjCount))
	{
		aObjects->Release();
		free(aBuffer);
		g_script.ScriptError(_T("Error dumping Object."));
		aResultToken.symbol = SYM_STRING;
		aResultToken.marker = _T("");
		return;
	}
	aSize += sizeof(__int64);
	aObjects->Release();
	if (aParamCount > 2 && TokenToInt64(*aParam[2]) > 1)
	{
		LPVOID aDataBuf;
		TCHAR *pw[1024] = {};
		if (!ParamIndexIsOmittedOrEmpty(3))
		{
			TCHAR *pwd = TokenToString(*aParam[3]);
			size_t pwlen = _tcslen(TokenToString(*aParam[3]));
			for (size_t i = 0; i <= pwlen; i++)
				pw[i] = &pwd[i];
		}
		DWORD aCompressedSize = CompressBuffer((BYTE*)aBuffer, aDataBuf, aSize, pw);
		if (aCompressedSize)
		{
			free(aBuffer);
			aBuffer = (char*)malloc(aCompressedSize);
			if (!aBuffer)
			{
				g_script.ScriptError(ERR_OUTOFMEM);
				aResultToken.symbol = SYM_STRING;
				aResultToken.marker = _T("");
				return;
			}
			memcpy(aBuffer, aDataBuf, aSize = aCompressedSize);
			VirtualFree(aDataBuf, 0, MEM_RELEASE);
		}
	}
	aResultToken.value_int64 = aSize;
	if (!TokenToObject(*aParam[0]) && TokenToObject(*aParam[1]))
	{ // FileWrite mode
		FILE *hFile = _tfopen(TokenToString(*aParam[0]), _T("wb"));
		if (!hFile)
		{
			aResultToken.symbol = SYM_STRING;
			aResultToken.marker = _T("");
			return;
		}
		fwrite(aBuffer, aSize, 1, hFile);
		fclose(hFile);
		free(aBuffer);
	}
	else if (aParam[1]->symbol == SYM_VAR)
	{
		Var &var = *(aParam[1]->var->mType == VAR_ALIAS ? aParam[1]->var->mAliasFor : aParam[1]->var);
		if (var.mType != VAR_NORMAL) // i.e. VAR_CLIPBOARD or VAR_VIRTUAL.
		{
			g_script.ScriptError(ERR_VAR_IS_READONLY, var.mName);
			aResultToken.symbol = SYM_STRING;
			aResultToken.marker = _T("");
			return;
		}
		var.Free(VAR_ALWAYS_FREE); // Release the variable's old memory. This also removes flags VAR_ATTRIB_OFTEN_REMOVED.
		var.mHowAllocated = ALLOC_MALLOC; // Must always be this type to avoid complications and possible memory leaks.
		var.mByteContents = aBuffer;
		var.mByteCapacity = var.mByteLength = (VarSizeType)aResultToken.value_int64;
	}
}

//
// ObjRawLoad()
//

IObject* ObjRawLoad(char *aBuffer, IObject **&aObjects, UINT &aObjCount, UINT &aObjSize)
{
	IObject *aObject = Object::Create();
	if (aObjCount == aObjSize)
	{
		IObject **newObjects = (IObject**)malloc(aObjSize * 2 * sizeof(IObject**));
		if (!newObjects)
			return 0;
		memcpy(newObjects, aObjects, aObjSize);
		free(aObjects);
		aObjects = newObjects;
		aObjSize *= 2;
	}
	aObjects[aObjCount++] = aObject;
	char *aThisBuffer = aBuffer + sizeof(__int64);

	ExprTokenType Result, this_token, enum_token, aCall, aKey, aValue;
	ExprTokenType *params[] = { &aCall, &aKey, &aValue };
	aCall.symbol = SYM_STRING;
	size_t aSize = (size_t)*(__int64*)aBuffer;
	TCHAR buf[MAX_INTEGER_LENGTH];

	for (char *end = aThisBuffer + aSize; aThisBuffer < end;)
	{
		char type = *(char*)aThisBuffer;
		aThisBuffer += 1;
		if (type == -12)
		{
			aKey.symbol = SYM_OBJECT;
			aKey.object = aObjects[*(__int64*)aThisBuffer];
			aThisBuffer += sizeof(__int64);
		}
		else if (type == -11)
		{
			aKey.symbol = SYM_OBJECT;
			aKey.object = ObjRawLoad(aThisBuffer, aObjects, aObjCount, aObjSize);
			aThisBuffer += sizeof(__int64) + *(__int64*)aThisBuffer;
		}
		else if (type == -10)
		{
			aKey.symbol = SYM_STRING;
			__int64 aMarkerSize = *(__int64*)aThisBuffer;
			aThisBuffer += sizeof(__int64);
			if (aMarkerSize)
			{
				aKey.marker_length = (size_t)(aMarkerSize - sizeof(TCHAR)) / sizeof(TCHAR);
				aKey.marker = (LPTSTR)aThisBuffer;
				aThisBuffer += aMarkerSize;
			}
			else
			{
				aKey.marker = _T("");
				aKey.marker_length = 0;
			}
		}
		else if (type == -9)
		{
			aKey.symbol = SYM_STRING;
			aKey.marker_length = FTOA(*(double*)aThisBuffer, buf, MAX_INTEGER_LENGTH);
			aKey.marker = buf;
			aThisBuffer += sizeof(__int64);
		}
		else
		{
			aKey.symbol = SYM_INTEGER;
			if (type == -8)
			{
				aKey.value_int64 = *(__int64*)aThisBuffer;
				aThisBuffer += sizeof(__int64);
			}
			else if (type == -6)
			{
				aKey.value_int64 = *(UINT*)aThisBuffer;
				aThisBuffer += sizeof(UINT);
			}
			else if (type == -4)
			{
				aKey.value_int64 = *(USHORT*)aThisBuffer;
				aThisBuffer += sizeof(USHORT);
			}
			else if (type == -2)
			{
				aKey.value_int64 = *(BYTE*)aThisBuffer;
				aThisBuffer += sizeof(BYTE);
			}
			else if (type == -1)
			{
				aKey.value_int64 = *(char*)aThisBuffer;
				aThisBuffer += sizeof(char);
			}
			else if (type == -3)
			{
				aKey.value_int64 = *(short*)aThisBuffer;
				aThisBuffer += sizeof(short);
			}
			else if (type == -5)
			{
				aKey.value_int64 = *(int*)aThisBuffer;
				aThisBuffer += sizeof(int);
			}
			else if (type == -7)
			{
				aKey.value_int64 = *(__int64*)aThisBuffer;
				aThisBuffer += sizeof(__int64);
			}
			else
				return NULL;
		}

		type = *(char*)aThisBuffer;
		aThisBuffer += 1;
		if (type == 12)
		{
			aValue.symbol = SYM_OBJECT;
			aValue.object = aObjects[*(__int64*)aThisBuffer];
			aThisBuffer += sizeof(__int64);
		}
		else if (type == 11)
		{
			aValue.symbol = SYM_OBJECT;
			aValue.object = ObjRawLoad(aThisBuffer, aObjects, aObjCount, aObjSize);
			aThisBuffer += sizeof(__int64) + *(__int64*)aThisBuffer;
		}
		else if (type == 9)
		{
			aValue.symbol = SYM_FLOAT;
			aValue.value_double = *(double*)aThisBuffer;
			aThisBuffer += sizeof(__int64);
		}
		else if (type != 10)
		{
			aValue.symbol = SYM_INTEGER;
			if (type == 8)
			{
				aValue.value_int64 = *(__int64*)aThisBuffer;
				aThisBuffer += sizeof(__int64);
			}
			else if (type == 6)
			{
				aValue.value_int64 = *(UINT*)aThisBuffer;
				aThisBuffer += sizeof(UINT);
			}
			else if (type == 4)
			{
				aValue.value_int64 = *(USHORT*)aThisBuffer;
				aThisBuffer += sizeof(USHORT);
			}
			else if (type == 2)
			{
				aValue.value_int64 = *(BYTE*)aThisBuffer;
				aThisBuffer += sizeof(BYTE);
			}
			else if (type == 1)
			{
				aValue.value_int64 = *(char*)aThisBuffer;
				aThisBuffer += sizeof(char);
			}
			else if (type == 3)
			{
				aValue.value_int64 = *(short*)aThisBuffer;
				aThisBuffer += sizeof(short);
			}
			else if (type == 5)
			{
				aValue.value_int64 = *(int*)aThisBuffer;
				aThisBuffer += sizeof(int);
			}
			else if (type == 7)
			{
				aValue.value_int64 = *(__int64*)aThisBuffer;
				aThisBuffer += sizeof(__int64);
			}
			else
				return NULL;
		}
		if (type == 10)
		{
			aValue.symbol = SYM_STRING;
			__int64 aMarkerSize = *(__int64*)aThisBuffer;
			aThisBuffer += sizeof(__int64);
			if (aMarkerSize)
			{
				aValue.marker_length = -1;
				aValue.marker = (LPTSTR)aThisBuffer;
				aObject->Invoke(Result, this_token, IT_SET, params + 1, 2);
				aCall.marker = _T("SetCapacity");
				aValue.symbol = SYM_INTEGER;
				aValue.value_int64 = aMarkerSize;
				aObject->Invoke(Result, this_token, IT_CALL, params, 3);
				aCall.marker = _T("GetAddress");
				aObject->Invoke(Result, this_token, IT_CALL, params, 2);
				memcpy((char*)Result.value_int64, aThisBuffer, (size_t)aMarkerSize);
				aThisBuffer += aMarkerSize;
			}
			else
			{
				aValue.marker = _T("");
				aValue.marker_length = 0;
				aObject->Invoke(Result, this_token, IT_SET, params + 1, 2);
			}
		}
		else
			aObject->Invoke(Result, this_token, IT_SET, params + 1, 2);
	}
	return aObject;
}

//
// ObjLoad()
//

BIF_DECL(BIF_ObjLoad)
{
	aResultToken.symbol = SYM_OBJECT;
	bool aFreeBuffer = false;
	DWORD aSize = aParamCount > 1 ? (DWORD)TokenToInt64(*aParam[1]) : 0;
	LPTSTR aPath = TokenToString(*aParam[0]);
	char *aBuffer = (char *)TokenToInt64(*aParam[0]);
	if (!aBuffer)
	{ // FileRead Mode
		if (GetFileAttributes(aPath) == 0xFFFFFFFF)
		{
			aResultToken.symbol = SYM_STRING;
			aResultToken.marker = _T("");
			return;
		}
		FILE *fp;

		fp = _tfopen(aPath, _T("rb"));
		if (fp == NULL)
		{
			aResultToken.symbol = SYM_STRING;
			aResultToken.marker = _T("");
			return;
		}

		fseek(fp, 0, SEEK_END);
		aSize = ftell(fp);
		aBuffer = (char *)malloc(aSize);
		aFreeBuffer = true;
		fseek(fp, 0, SEEK_SET);
		fread(aBuffer, 1, aSize, fp);
		fclose(fp);
	}
	if (*(unsigned int*)aBuffer == 0x04034b50)
	{
		LPVOID aDataBuf;
		TCHAR *pw[1024] = {};
		if (!ParamIndexIsOmittedOrEmpty(1))
		{
			TCHAR *pwd = TokenToString(*aParam[1]);
			size_t pwlen = _tcslen(TokenToString(*aParam[1]));
			for (size_t i = 0; i <= pwlen; i++)
				pw[i] = &pwd[i];
		}
		aSize = *(ULONG*)((UINT_PTR)aBuffer + 8);
		if (*(ULONG*)((UINT_PTR)aBuffer + 16) > aSize)
			aSize = *(ULONG*)((UINT_PTR)aBuffer + 16);
		DWORD aSizeDeCompressed = DecompressBuffer(aBuffer, aDataBuf, aSize, pw);
		if (aSizeDeCompressed)
		{
			LPVOID buff = malloc(aSizeDeCompressed);
			aFreeBuffer = true;
			memcpy(buff, aDataBuf, aSizeDeCompressed);
			g_memset(aDataBuf, 0, aSizeDeCompressed);
			free(aDataBuf);
			aBuffer = (char*)buff;
		}
		else
		{
			aResultToken.symbol = SYM_STRING;
			aResultToken.marker = _T("");
			g_script.ScriptError(_T("ObjLoad: Password mismatch."));
			return;
		}
	}
	UINT aObjCount = 0;
	UINT aObjSize = 16;
	IObject **aObjects = (IObject**)malloc(aObjSize * sizeof(IObject**));
	if (!aObjects || !(aResultToken.object = ObjRawLoad(aBuffer, aObjects, aObjCount, aObjSize)))
	{
		if (!TokenToInt64(*aParam[0]))
			free(aBuffer);
		free(aObjects);
		aResultToken.symbol = SYM_STRING;
		aResultToken.marker = _T("");
		g_script.ScriptError(ERR_OUTOFMEM);
		return;
	}
	free(aObjects);
	if (aFreeBuffer)
		free(aBuffer);
}

//
// BIF_ObjCreate - Object()
//

BIF_DECL(BIF_ObjCreate)
{
	IObject *obj = NULL;

	if (aParamCount == 1) // L33: POTENTIALLY UNSAFE - Cast IObject address to object reference.
	{
		if (obj = TokenToObject(*aParam[0]))
		{	// Allow &obj == Object(obj), but AddRef() for equivalence with ComObjActive(comobj).
			obj->AddRef();
			aResultToken.value_int64 = (__int64)obj;
			return; // symbol is already SYM_INTEGER.
		}
		obj = (IObject *)TokenToInt64(*aParam[0]);
		if (obj < (IObject *)1024) // Prevent some obvious errors.
			obj = NULL;
		else
			obj->AddRef();
	}
	else
		obj = Object::Create(aParam, aParamCount);

	if (obj)
	{
		aResultToken.symbol = SYM_OBJECT;
		aResultToken.object = obj;
		// DO NOT ADDREF: after we return, the only reference will be in aResultToken.
	}
	else
	{
		aResultToken.symbol = SYM_STRING;
		aResultToken.marker = _T("");
	}
}


//
// BIF_ObjArray - Array(items*)
//

BIF_DECL(BIF_ObjArray)
{
	if (aResultToken.object = Object::CreateArray(aParam, aParamCount))
	{
		aResultToken.symbol = SYM_OBJECT;
		return;
	}
	aResultToken.symbol = SYM_STRING;
	aResultToken.marker = _T("");
}
	

//
// BIF_IsObject - IsObject(obj)
//

BIF_DECL(BIF_IsObject)
{
	int i;
	for (i = 0; i < aParamCount && TokenToObject(*aParam[i]); ++i);
	aResultToken.value_int64 = (__int64)(i == aParamCount); // TRUE if all are objects.
}
	

//
// BIF_ObjInvoke - Handles ObjGet/Set/Call() and get/set/call syntax.
//

BIF_DECL(BIF_ObjInvoke)
{
    int invoke_type;
    IObject *obj;
    ExprTokenType *obj_param;

	// Since ObjGet/ObjSet/ObjCall are not publicly accessible as functions, Func::mName
	// (passed via aResultToken.marker) contains the actual flag rather than a name.
	invoke_type = (int)(INT_PTR)aResultToken.marker;

	// Set default return value; ONLY AFTER THE ABOVE.
	aResultToken.symbol = SYM_STRING;
	aResultToken.marker = _T("");
    
    obj_param = *aParam; // aParam[0].  Load-time validation has ensured at least one parameter was specified.
	++aParam;
	--aParamCount;

	// The following is used in place of TokenToObject to bypass #Warn UseUnset:
	if (obj_param->symbol == SYM_OBJECT)
		obj = obj_param->object;
	else if (obj_param->symbol == SYM_VAR && obj_param->var->HasObject())
		obj = obj_param->var->Object();
	else
		obj = NULL;
    
    if (obj)
	{
		bool param_is_var = obj_param->symbol == SYM_VAR;
		if (param_is_var)
			// Since the variable may be cleared as a side-effect of the invocation, call AddRef to ensure the object does not expire prematurely.
			// This is not necessary for SYM_OBJECT since that reference is already counted and cannot be released before we return.  Each object
			// could take care not to delete itself prematurely, but it seems more proper, more reliable and more maintainable to handle it here.
			obj->AddRef();
         aResult = obj->Invoke(aResultToken, *obj_param, invoke_type, aParam, aParamCount);
		if (param_is_var)
			obj->Release();
	}
	// Invoke meta-functions of g_MetaObject.
	else if (INVOKE_NOT_HANDLED == (aResult = g_MetaObject.Invoke(aResultToken, *obj_param, invoke_type | IF_META, aParam, aParamCount)))
	{
		// Since above did not handle it, check for attempts to access .base of non-object value (g_MetaObject itself).
		if (   invoke_type != IT_CALL // Exclude things like "".base().
			&& aParamCount > (invoke_type == IT_SET ? 2 : 0) // SET is supported only when an index is specified: "".base[x]:=y
			&& !_tcsicmp(TokenToString(*aParam[0]), _T("base"))   )
		{
			if (aParamCount > 1)	// "".base[x] or similar
			{
				// Re-invoke g_MetaObject without meta flag or "base" param.
				ExprTokenType base_token;
				base_token.symbol = SYM_OBJECT;
				base_token.object = &g_MetaObject;
				g_MetaObject.Invoke(aResultToken, base_token, invoke_type, aParam + 1, aParamCount - 1);
			}
			else					// "".base
			{
				// Return a reference to g_MetaObject.  No need to AddRef as g_MetaObject ignores it.
				aResultToken.symbol = SYM_OBJECT;
				aResultToken.object = &g_MetaObject;
			}
		}
		else
		{
			// Since it wasn't handled (not even by g_MetaObject), maybe warn at this point:
			if (obj_param->symbol == SYM_VAR)
				obj_param->var->MaybeWarnUninitialized();
		}
	}
	if (aResult == INVOKE_NOT_HANDLED)
		aResult = OK;
}
	

//
// BIF_ObjGetInPlace - Handles part of a compound assignment like x.y += z.
//

BIF_DECL(BIF_ObjGetInPlace)
{
	// Since the most common cases have two params, the "param count" param is omitted in
	// those cases. Otherwise we have one visible parameter, which indicates the number of
	// actual parameters below it on the stack.
	aParamCount = aParamCount ? (int)TokenToInt64(*aParam[0]) : 2; // x[<n-1 params>] : x.y
	BIF_ObjInvoke(aResult, aResultToken, aParam - aParamCount, aParamCount);
}


//
// BIF_ObjNew - Handles "new" as in "new Class()".
//

BIF_DECL(BIF_ObjNew)
{
	aResultToken.symbol = SYM_STRING;
	aResultToken.marker = _T("");

	ExprTokenType *class_token = aParam[0]; // Save this to be restored later.

	IObject *class_object = TokenToObject(*class_token);
	if (!class_object)
		return;

	Object *new_object = Object::Create();
	if (!new_object)
		return;

	new_object->SetBase(class_object);

	ExprTokenType name_token, this_token;
	this_token.symbol = SYM_OBJECT;
	this_token.object = new_object;
	name_token.symbol = SYM_STRING;
	aParam[0] = &name_token;

	ResultType result;
	LPTSTR buf = aResultToken.buf; // In case Invoke overwrites it via the union.

	Line *curr_line = g_script.mCurrLine;

	// __Init was added so that instance variables can be initialized in the correct order
	// (beginning at the root class and ending at class_object) before __New is called.
	// It shouldn't be explicitly defined by the user, but auto-generated in DefineClassVars().
	name_token.marker = _T("__Init");
	result = class_object->Invoke(aResultToken, this_token, IT_CALL | IF_METAOBJ, aParam, 1);
	if (result != INVOKE_NOT_HANDLED)
	{
		// It's possible that __Init is user-defined (despite recommendations in the
		// documentation) or built-in, so make sure the return value, if any, is freed:
		if (aResultToken.symbol == SYM_OBJECT)
			aResultToken.object->Release();
		if (aResultToken.mem_to_free)
		{
			free(aResultToken.mem_to_free);
			aResultToken.mem_to_free = NULL;
		}
		// Reset to defaults for __New, invoked below.
		aResultToken.symbol = SYM_STRING;
		aResultToken.marker = _T("");
		aResultToken.buf = buf;
		if (result == FAIL || result == EARLY_EXIT)
		{
			new_object->Release();
			aParam[0] = class_token; // Restore it to original caller-supplied value.
			aResult = result;
			return;
		}
	}

	g_script.mCurrLine = curr_line; // Prevent misleading error reports/Exception() stack trace.
	
	// __New may be defined by the script for custom initialization code.
	name_token.marker = Object::sMetaFuncName[4]; // __New
	result = class_object->Invoke(aResultToken, this_token, IT_CALL | IF_METAOBJ, aParam, aParamCount);
	if (result == EARLY_RETURN || !TokenIsEmptyString(aResultToken))
	{
		// __New() returned a value in aResultToken, so use it as our result.  aResultToken is checked
		// for the unlikely case that class_object is not an Object (perhaps it's a ComObject) or __New
		// points to a built-in function.  The older behaviour for those cases was to free the unexpected
		// return value, but this requires less code and might be useful somehow.
		new_object->Release();
	}
	else if (result == FAIL || result == EARLY_EXIT)
	{
		// An error was raised within __New() or while trying to call it, or Exit was called.
		new_object->Release();
		aResult = result;
	}
	else
	{
		// Either it wasn't handled (i.e. neither this class nor any of its super-classes define __New()),
		// or there was no explicit "return", so just return the new object.
		aResultToken.symbol = SYM_OBJECT;
		aResultToken.object = this_token.object;
	}
	aParam[0] = class_token;
}


//
// BIF_ObjIncDec - Handles pre/post-increment/decrement for object fields, such as ++x[y].
//

BIF_DECL(BIF_ObjIncDec)
{
	// Func::mName (which aResultToken.marker is set to) has been overloaded to pass
	// the type of increment/decrement to be performed on this object's field.
	SymbolType op = (SymbolType)(INT_PTR)aResultToken.marker;

	ExprTokenType temp_result, current_value, value_to_set;

	// Set the defaults expected by BIF_ObjInvoke:
	temp_result.symbol = SYM_INTEGER;
	temp_result.marker = (LPTSTR)IT_GET;
	temp_result.buf = aResultToken.buf;
	temp_result.mem_to_free = NULL;

	// Retrieve the current value.  Do it this way instead of calling Object::Invoke
	// so that if aParam[0] is not an object, g_MetaObject is correctly invoked.
	BIF_ObjInvoke(aResult, temp_result, aParam, aParamCount);

	if (aResult == FAIL || aResult == EARLY_EXIT)
		return;

	// Change SYM_STRING to SYM_OPERAND so below may treat it as a numeric string.
	if (temp_result.symbol == SYM_STRING)
	{
		temp_result.symbol = SYM_OPERAND;
		temp_result.buf = NULL; // Indicate that this SYM_OPERAND token LACKS a pre-converted binary integer.
	}

	switch (value_to_set.symbol = current_value.symbol = TokenIsPureNumeric(temp_result))
	{
	case PURE_INTEGER:
		value_to_set.value_int64 = (current_value.value_int64 = TokenToInt64(temp_result))
			+ ((op == SYM_POST_INCREMENT || op == SYM_PRE_INCREMENT) ? +1 : -1);
		break;

	case PURE_FLOAT:
		value_to_set.value_double = (current_value.value_double = TokenToDouble(temp_result))
			+ ((op == SYM_POST_INCREMENT || op == SYM_PRE_INCREMENT) ? +1 : -1);
		break;
	}

	// Free the object or string returned by BIF_ObjInvoke, if applicable.
	if (temp_result.symbol == SYM_OBJECT)
		temp_result.object->Release();
	if (temp_result.mem_to_free)
		free(temp_result.mem_to_free);

	if (current_value.symbol == PURE_NOT_NUMERIC)
	{
		// Value is non-numeric, so assign and return "".
		value_to_set.symbol = SYM_STRING;
		value_to_set.marker = _T("");
		//current_value.symbol = SYM_STRING; // Already done (SYM_STRING == PURE_NOT_NUMERIC).
		current_value.marker = _T("");
	}

	// Although it's likely our caller's param array has enough space to hold the extra
	// parameter, there's no way to know for sure whether it's safe, so we allocate our own:
	ExprTokenType **param = (ExprTokenType **)_alloca((aParamCount + 1) * sizeof(ExprTokenType *));
	memcpy(param, aParam, aParamCount * sizeof(ExprTokenType *)); // Copy caller's param pointers.
	param[aParamCount++] = &value_to_set; // Append new value as the last parameter.

	if (op == SYM_PRE_INCREMENT || op == SYM_PRE_DECREMENT)
	{
		aResultToken.marker = (LPTSTR)IT_SET;
		// Set the new value and pass the return value of the invocation back to our caller.
		// This should be consistent with something like x.y := x.y + 1.
		BIF_ObjInvoke(aResult, aResultToken, param, aParamCount);
	}
	else // SYM_POST_INCREMENT || SYM_POST_DECREMENT
	{
		// Must be re-initialized (and must use IT_SET instead of IT_GET):
		temp_result.symbol = SYM_INTEGER;
		temp_result.marker = (LPTSTR)IT_SET;
		temp_result.buf = aResultToken.buf;
		temp_result.mem_to_free = NULL;
		
		// Set the new value.
		BIF_ObjInvoke(aResult, temp_result, param, aParamCount);
		
		// Dispose of the result safely.
		if (temp_result.symbol == SYM_OBJECT)
			temp_result.object->Release();
		if (temp_result.mem_to_free)
			free(temp_result.mem_to_free);

		// Return the previous value.
		aResultToken.symbol = current_value.symbol;
		aResultToken.value_int64 = current_value.value_int64; // Union copy.
	}
}


//
// Functions for accessing built-in methods (even if obscured by a user-defined method).
//

#define BIF_METHOD(name) \
	BIF_DECL(BIF_Obj##name) { \
		if (!BIF_ObjMethod(FID_Obj##name, aResultToken, aParam, aParamCount)) \
			aResult = FAIL; \
	}

ResultType BIF_ObjMethod(int aID, ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount)
{
	aResultToken.symbol = SYM_STRING;
	aResultToken.marker = _T("");

	Object *obj = dynamic_cast<Object*>(TokenToObject(*aParam[0]));
	if (!obj)
		return OK; // Return "".
	return obj->CallBuiltin(aID, aResultToken, aParam + 1, aParamCount - 1);
}

BIF_METHOD(Insert)
BIF_METHOD(InsertAt)
BIF_METHOD(Push)
BIF_METHOD(Pop)
BIF_METHOD(Delete)
BIF_METHOD(Remove)
BIF_METHOD(RemoveAt)
BIF_METHOD(GetCapacity)
BIF_METHOD(SetCapacity)
BIF_METHOD(GetAddress)
BIF_METHOD(Count)
BIF_METHOD(Length)
BIF_METHOD(MaxIndex)
BIF_METHOD(MinIndex)
BIF_METHOD(NewEnum)
BIF_METHOD(HasKey)
BIF_METHOD(Clone)


//
// ObjAddRef/ObjRelease - used with pointers rather than object references.
//

BIF_DECL(BIF_ObjAddRefRelease)
{
	IObject *obj = (IObject *)TokenToInt64(*aParam[0]);
	if (obj < (IObject *)4096) // Rule out some obvious errors.
	{
		aResultToken.symbol = SYM_STRING;
		aResultToken.marker = _T("");
		return;
	}
	if (ctoupper(aResultToken.marker[3]) == 'A')
		aResultToken.value_int64 = obj->AddRef();
	else
		aResultToken.value_int64 = obj->Release();
}


//
// ObjBindMethod(Obj, Method, Params...)
//

BIF_DECL(BIF_ObjBindMethod)
{
	IObject *func, *bound_func;
	if (  !(func = TokenToObject(*aParam[0]))
		&& !(func = TokenToFunc(*aParam[0]))  )
	{
		aResult = g_script.ScriptError(ERR_PARAM1_INVALID);
		return;
	}
	if (  !(bound_func = BoundFunc::Bind(func, aParam + 1, aParamCount - 1, IT_CALL))  )
	{
		aResult = g_script.ScriptError(ERR_OUTOFMEM);
		return;
	}
	aResultToken.symbol = SYM_OBJECT;
	aResultToken.object = bound_func;
}


//
// ObjRawSet - set a value without invoking any meta-functions.
//

BIF_DECL(BIF_ObjRaw)
{
	Object *obj = dynamic_cast<Object*>(TokenToObject(*aParam[0]));
	if (!obj)
	{
		aResult = g_script.ScriptError(ERR_PARAM1_INVALID);
		return;
	}
	if (ctoupper(aResultToken.marker[6]) == 'S')
	{
		if (!obj->SetItem(*aParam[1], *aParam[2]))
		{
			aResult = g_script.ScriptError(ERR_OUTOFMEM);
			return;
		}
	}
	else
	{
		ExprTokenType value;
		if (obj->GetItem(value, *aParam[1]))
		{
			switch (value.symbol)
			{
			case SYM_OPERAND:
				aResultToken.symbol = SYM_STRING;
				aResult = TokenSetResult(aResultToken, value.marker);
				break;
			case SYM_OBJECT:
				aResultToken.symbol = SYM_OBJECT;
				aResultToken.object = value.object;
				aResultToken.object->AddRef();
				break;
			default:
				aResultToken.symbol = value.symbol;
				aResultToken.value_int64 = value.value_int64;
				break;
			}
			return;
		}
	}
	aResultToken.symbol = SYM_STRING;
	aResultToken.marker = _T("");
}


//
// ObjSetBase/ObjGetBase - Change or return Object's base without invoking any meta-functions.
//

BIF_DECL(BIF_ObjBase)
{
	Object *obj = dynamic_cast<Object*>(TokenToObject(*aParam[0]));
	if (!obj)
	{
		aResult = g_script.ScriptError(ERR_PARAM1_INVALID);
		return;
	}
	if (ctoupper(aResultToken.marker[3]) == 'S') // ObjSetBase
	{
		IObject *new_base = TokenToObject(*aParam[1]);
		if (!new_base && !TokenIsEmptyString(*aParam[1]))
		{
			aResult = g_script.ScriptError(ERR_PARAM2_INVALID);
			return;
		}
		obj->SetBase(new_base);
	}
	else // ObjGetBase
	{
		if (IObject *obj_base = obj->Base())
		{
			obj_base->AddRef();
			aResultToken.SetValue(obj_base);
			return;
		}
	}
	aResultToken.SetValue(_T(""));
}
