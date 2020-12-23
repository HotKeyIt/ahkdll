#include "stdafx.h" // pre-compiled headers
#include "defines.h"
#include "globaldata.h"
#include "script.h"

#include "script_object.h"
#include "script_func_impl.h"
#include "application.h"


extern BuiltInFunc *OpFunc_GetProp, *OpFunc_GetItem, *OpFunc_SetProp, *OpFunc_SetItem;

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
		aResultToken.SetResult(FAIL);
		return;
	}
}

//
// sizeof_maxsize - get max size of union or structure, helper function for BIF_Sizeof and BIF_Struct
//
BYTE sizeof_maxsize(TCHAR *buf)
{
	ResultToken ResultToken;
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
	ExprTokenType *param[] = { &Var1, &Var2, &Var3 };
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
				_tcsncpy(tempbuf, &buf[i], thissize = (int)(StrChrAny(&buf[i], _T(";,")) - &buf[i]));
				i += thissize + 1;
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
 				BIF_sizeof(ResultToken, param, 3);
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

	// following are used to find variable and also get size of a structure defined in variable
	// this will hold the variable reference and offset that is given to size() to align if necessary in 64-bit
	ResultToken ResultToken;
	ExprTokenType Var1,Var2,Var3;
	Var1.symbol = SYM_VAR;
	Var2.symbol = SYM_INTEGER;

	// used to pass aligntotal counter to structure in structure
	Var3.symbol = SYM_INTEGER;
	Var3.value_int64 = (__int64)&align;
	ExprTokenType *param[] = {&Var1,&Var2,&Var3};
	
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
	
	// if first parameter is an object (struct), simply return its size
	if (TokenToObject(*aParam[0]))
	{
		aResultToken.symbol = SYM_INTEGER;
		Struct *obj = (Struct*)TokenToObject(*aParam[0]);
		aResultToken.value_int64 = obj->mStructSize + (aParamCount > 1 ? TokenToInt64(*aParam[1]) : 0);
		return;
	}

	if (aParamCount > 1 && TokenIsNumeric(*aParam[1]))
	{	// an offset was given, set starting offset 
		offset = (int)TokenToInt64(*aParam[1]);
		Var2.value_int64 = (__int64)offset;
	}
	if (aParamCount > 2 && TokenIsNumeric(*aParam[2]))
	{   // a pointer was given to return memory to align
		aligntotal = (int*)TokenToInt64(*aParam[2]);
		Var3.value_int64 = (__int64)aligntotal;
	}
	// Set buf to beginning of structure definition
	buf = TokenToString(*aParam[0]);

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
			if (mod = offset % (maxsize = sizeof_maxsize(buf)))
				offset += (maxsize - mod) % maxsize;
			structalign[uniondepth] = *aligntotal > maxsize ? *aligntotal : maxsize;
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
				if (mod = totalunionsize % *aligntotal)
					totalunionsize += (*aligntotal - mod) % *aligntotal;
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
				g_script->ScriptError(ERR_INVALID_STRUCT, buf);
				return;
			}
			_tcsncpy(tempbuf,buf,buf_size);
			tempbuf[buf_size] = '\0';
		}
		else if (_tcslen(buf) > LINE_SIZE - 1)
		{
			g_script->ScriptError(ERR_INVALID_STRUCT, buf);
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
			g_script->ScriptError(ERR_INVALID_STRUCT, tempbuf);
			return;
		}
		// Pointer, while loop will continue here because we only need size
		if (_tcschr(tempbuf,'*'))
		{
			if (_tcschr(tempbuf, ':'))
			{
				g_script->ScriptError(ERR_INVALID_STRUCT_BIT_POINTER, tempbuf);
				return;
			}
			// align offset for pointer
			if (mod = offset % ptrsize)
				offset += (ptrsize - mod) % ptrsize;
			offset += ptrsize * (arraydef ? arraydef : 1);
			if (ptrsize > *aligntotal)
				*aligntotal = ptrsize;
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
				g_script->ScriptError(ERR_INVALID_STRUCT, tempbuf);
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
			if ((!bitsize || bitsizetotal == bitsize) && thissize > 1 && (mod = offset % thissize))
				offset += (thissize - mod) % thissize;
			if (!bitsize || bitsizetotal == bitsize)
				offset += thissize * (arraydef ? arraydef : 1);
			if (thissize > *aligntotal)
				*aligntotal = thissize; // > ptrsize ? ptrsize : thissize;
		}
		else // type was not found, check for user defined type in variables
		{
			Var1.var = NULL;
			UserFunc *bkpfunc = NULL;
			// check if we have a local/static declaration and resolve to function
			// For example Struct("MyFunc(mystruct) mystr")
			if (_tcschr(defbuf,'('))
			{
				bkpfunc = g->CurrentFunc; // don't bother checking, just backup and restore later
				g->CurrentFunc = (UserFunc *)g_script->FindFunc(defbuf + 1,_tcscspn(defbuf,_T("(")) - 1);
				if (g->CurrentFunc) // break if not found to identify error
				{
					_tcscpy(tempbuf,defbuf + 1);
					_tcscpy(defbuf + 1,tempbuf + _tcscspn(tempbuf,_T("(")) + 1); //,_tcschr(tempbuf,')') - _tcschr(tempbuf,'('));
					_tcscpy(_tcschr(defbuf,')'),_T(" \0"));
					Var1.var = g_script->FindVar(defbuf + 1,_tcslen(defbuf) - 2,FINDVAR_LOCAL);
					g->CurrentFunc = bkpfunc;
				}
				else // release object and return
				{
					g->CurrentFunc = bkpfunc;
					g_script->ScriptError(ERR_INVALID_STRUCT_IN_FUNC, defbuf);
					return;
				}
			}
			else if (g->CurrentFunc) // try to find local variable first
				Var1.var = g_script->FindVar(defbuf + 1,_tcslen(defbuf) - 2,FINDVAR_LOCAL);
			// try to find global variable if local was not found or we are not in func
			if (Var1.var == NULL)
				Var1.var = g_script->FindVar(defbuf + 1,_tcslen(defbuf) - 2,FINDVAR_GLOBAL);
			if (Var1.var != NULL)
			{
				// Call BIF_sizeof passing offset in second parameter to align if necessary
				int newaligntotal = sizeof_maxsize(TokenToString(Var1));
				if (newaligntotal > *aligntotal)
					*aligntotal = newaligntotal;
				if ((!bitsize || bitsizetotal == bitsize) && offset && (mod = offset % *aligntotal))
					offset += (*aligntotal - mod) % *aligntotal;
				param[1]->value_int64 = (__int64)offset;
				BIF_sizeof(ResultToken,param,3);
				if (ResultToken.symbol != SYM_INTEGER)
				{	// could not resolve structure
					g_script->ScriptError(ERR_INVALID_STRUCT, defbuf);
					return;
				}
				// sizeof was given an offset that it applied and aligned if necessary, so set offset =  and not +=
				if (!bitsize || bitsizetotal == bitsize)
					offset = (int)ResultToken.value_int64 + (arraydef ? ((arraydef - 1) * ((int)ResultToken.value_int64 - offset)) : 0);
			}
			else // No variable was found and it is not default type so we can't determine size, return empty string.
			{
				g_script->ScriptError(ERR_INVALID_STRUCT, defbuf);
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

__int64 ObjRawSize(IObject *aObject, IObject *aObjects)
{
	ResultToken result_token, this_token, aKey, aValue;
	ExprTokenType *params[] = { &aKey, &aValue };
	this_token.symbol = SYM_OBJECT;

	CriticalObject *aCriticalObject;
	if (aCriticalObject = dynamic_cast<CriticalObject*>(aObject))
			aObject = (IObject*)aCriticalObject->GetObj();

	if ((_tcscmp(aObject->Type(), _T("Object")))
		&&(_tcscmp(aObject->Type(), _T("Array")))
		&&(_tcscmp(aObject->Type(), _T("Map")))
		&&(_tcscmp(aObject->Type(), _T("Buffer")))
		&&(_tcscmp(aObject->Type(), _T("Struct"))))
		g_script->ScriptError(ERR_TYPE_MISMATCH, aObject->Type());

	__int64 aSize = 1 + sizeof(__int64);
	
	if (!_tcscmp(aObject->Type(), _T("Buffer")))
	{
		this_token.object = aObject;
		aObject->Invoke(result_token, IT_GET, _T("size"), this_token, nullptr, 0);
		return result_token.value_int64 + aSize;
	}

	this_token.object = aObjects;

	IObject *enumerator;
	ResultType result;
	ResultToken enum_token;
	enum_token.object = aObject;
	enum_token.symbol = SYM_OBJECT;
	if (!_tcscmp(aObject->Type(), _T("Object")))
	{
		this_token.object = aObject;
		aObject->Invoke(result_token, IT_CALL, _T("OwnProps"), this_token, nullptr, 0);
		if (result_token.symbol == SYM_OBJECT)
		{
			result = OK;
			enumerator = result_token.object;
		}
		else
			result = FAIL;
	}
	else
		result = GetEnumerator(enumerator, enum_token, 2, false);
	// Check if object returned an enumerator, otherwise return
	if (result != OK)
		return 0;

	// create variables to use in for loop / for enumeration
	// these will be deleted afterwards

	Var vkey, vval;
	vkey.mAttrib = vval.mAttrib = VAR_NORMAL;

	if (!aObjects)
	{
		aKey.symbol = SYM_OBJECT;
		aKey.object = aObject;
		aValue.symbol = SYM_STRING;
		aValue.marker = _T("");
		aValue.marker_length = 0;
		aObjects = Map::Create(params, 2);
	}
	else
	{
		aObjects->AddRef();
		aKey.symbol = SYM_OBJECT;
		aKey.object = aObject;
		this_token.object = aObjects;
		aObjects->Invoke(result_token, IT_CALL, _T("Has"), this_token, params, 1);
		if (!result_token.value_int64)
		{
			aKey.symbol = SYM_OBJECT;
			aKey.object = aObject;
			aValue.symbol = SYM_STRING;
			aValue.marker = _T("");
			aValue.marker_length = 0;
			aObjects->Invoke(result_token, IT_SET, 0, this_token, params, 2);
		}
	}
	// Prepare parameters for the loop below: enum.Next(var1 [, var2])
	aKey.symbol = SYM_VAR;
	aKey.var = &vkey;
	aKey.var->mCharContents = _T("");
	aKey.mem_to_free = 0;
	aValue.symbol = SYM_VAR;
	aValue.var = &vval;
	aValue.var->mCharContents = _T("");
	aValue.mem_to_free = 0;

	IObject *aIsObject;
	__int64 aIsValue;
	SymbolType aVarType;
	
	for (;;)
	{
		result = CallEnumerator(enumerator, params, 2, false);
		if (result != CONDITION_TRUE)
			break;

		if (aIsObject = TokenToObject(aKey))
		{
			this_token.object = aObjects;
			aObjects->Invoke(result_token, IT_CALL, _T("Has"), this_token, params, 1);
			if (result_token.value_int64)
				aSize += 1 + sizeof(__int64);
			else
				aSize += ObjRawSize(aIsObject, aObjects);
		}
		else 
		{
			vkey.ToTokenSkipAddRef(aKey);
			if ((aVarType = aKey.symbol) == SYM_STRING)
				aSize += (aKey.marker_length ? (aKey.marker_length + 1) * sizeof(TCHAR) : 0) + 9;
			else
				aSize += (aVarType == SYM_FLOAT || (aIsValue = TokenToInt64(aKey)) > 4294967295) ? 9 : aIsValue > 65535 ? 5 : aIsValue > 255 ? 3 : aIsValue > -129 ? 2 : aIsValue > -32769 ? 3 : aIsValue >= INT_MIN ? 5 : 9;
		}

		if (aIsObject = TokenToObject(aValue))
		{
			aKey.symbol = SYM_OBJECT;
			aKey.object = aIsObject;
			this_token.object = aObjects;
			aObjects->Invoke(result_token, IT_CALL, _T("Has"), this_token, params, 1);
			if (result_token.value_int64)
				aSize += 1 + sizeof(__int64);
			else
				aSize += ObjRawSize(aIsObject, aObjects);
		}
		else
		{
			aValue.var->ToTokenSkipAddRef(aValue);
			if ((aVarType = aValue.symbol) == SYM_STRING)
				aSize += (aValue.marker_length ? (aValue.marker_length + 1) * sizeof(TCHAR) : 0) + 9;
			else
				aSize += aVarType == SYM_FLOAT || (aIsValue = TokenToInt64(aValue)) > 4294967295 ? 9 : aIsValue > 65535 ? 5 : aIsValue > 255 ? 3 : aIsValue > -129 ? 2 : aIsValue > -32769 ? 3 : aIsValue >= INT_MIN ? 5 : 9;
		}

		// release object if it was assigned prevoiously when calling enum.Next
		if (vkey.IsObject())
			vkey.ReleaseObject();
		if (vval.IsObject())
			vval.ReleaseObject();

		aKey.symbol = SYM_VAR;
		aKey.var = &vkey;
		aValue.symbol = SYM_VAR;
		aValue.var = &vval;
	}
	// release enumerator and free vars
	enumerator->Release();
	vkey.Free();
	vval.Free();
	aObjects->Release();
	return aSize;
}

//
// ObjRawDump()
//

__int64 ObjRawDump(IObject *aObject, char *aBuffer, Map *aObjects, UINT &aObjCount)
{

	ResultToken result_token, this_token, aKey, aValue;
	ExprTokenType *params[] = { &aKey, &aValue };
	
	this_token.symbol = SYM_OBJECT;
	
	CriticalObject *aCriticalObject;
	if (aCriticalObject = dynamic_cast<CriticalObject*>(aObject))
		aObject = (IObject*)aCriticalObject->GetObj();

	char *aThisBuffer = aBuffer;
	if (!_tcscmp(aObject->Type(), _T("Object")))
		*aThisBuffer = (char)-12;
	else if (!_tcscmp(aObject->Type(), _T("Array")))
		*aThisBuffer = (char)-13;
	else if (!_tcscmp(aObject->Type(), _T("Map")) || !_tcscmp(aObject->Type(), _T("Struct")))
		*aThisBuffer = (char)-14;
	else if (!_tcscmp(aObject->Type(), _T("Buffer")))
	{
		*aThisBuffer = (char)-15;
		aThisBuffer += 1 + sizeof(__int64);
		this_token.object = aObject;
		aObject->Invoke(result_token, IT_GET, _T("size"), this_token, nullptr, 0);
		__int64 bufsize = result_token.value_int64;
		aObject->Invoke(result_token, IT_GET, _T("ptr"), this_token, nullptr, 0);
		memmove(aThisBuffer, (void *)result_token.value_int64, (size_t)bufsize);
		aThisBuffer += bufsize;
		return aThisBuffer - aBuffer;
	}
	else
		g_script->ScriptError(ERR_TYPE_MISMATCH, aObject->Type());
	aThisBuffer += 1 + sizeof(__int64);

	IObject *enumerator;
	ResultType result;

	// Check if object returned an enumerator, otherwise return
	if (!_tcscmp(aObject->Type(), _T("Object")))
	{
		this_token.object = aObject;
		aObject->Invoke(result_token, IT_CALL, _T("OwnProps"), this_token, nullptr, 0);
		if (result_token.symbol == SYM_OBJECT)
			enumerator = result_token.object;
		else 
			return NULL;
	}
	else
	{
		result_token.object = aObject;
		result_token.symbol = SYM_OBJECT;
		result = GetEnumerator(enumerator, result_token, 2, false);
		if (result != OK)
			return NULL;
	}

	Var vkey, vval;
	vkey.mAttrib = vval.mAttrib = VAR_NORMAL;

	aKey.symbol = SYM_OBJECT;
	aKey.object = aObject;
	this_token.object = aObjects;
	aObjects->Invoke(result_token, IT_CALL, _T("Has"), this_token, params, 1);
	if (!result_token.value_int64)
	{
		aValue.symbol = SYM_INTEGER;
		aValue.value_int64 = aObjCount++;
		aObjects->Invoke(result_token, IT_SET, 0, this_token, params, 2);
	}

	// Prepare parameters for the loop below
	aKey.symbol = SYM_VAR;
	aKey.var = &vkey;
	aKey.var->mCharContents = _T("");
	aKey.mem_to_free = 0;
	aValue.symbol = SYM_VAR;
	aValue.var = &vval;
	aValue.var->mCharContents = _T("");
	aValue.mem_to_free = 0;

	IObject *aIsObject;
	__int64 aIsValue;
	__int64 aThisSize;
	SymbolType aVarType;

	for (;;)
	{
		result = CallEnumerator(enumerator, params, 2, false);
		if (result != CONDITION_TRUE)
			break;

		// copy Key
		if (aIsObject = TokenToObject(aKey))
		{
			aObjects->Invoke(result_token, IT_CALL, _T("Has"), this_token, params, 1);
			if (result_token.value_int64)
			{
				aObjects->Invoke(result_token, IT_GET, 0, this_token, params, 1);
				*aThisBuffer = (char)-11;
				aThisBuffer += 1;
				*(__int64*)aThisBuffer = result_token.value_int64;
				aThisBuffer += sizeof(__int64);
			}
			else
			{
				aThisSize = ObjRawDump(aIsObject, aThisBuffer, aObjects, aObjCount);
				*(__int64*)(aThisBuffer + 1) = aThisSize - 1 - sizeof(__int64);
				aThisBuffer += aThisSize;
			}
		}
		else
		{
			aKey.var->ToTokenSkipAddRef(aKey);
			if ((aVarType = aKey.symbol) == SYM_STRING)
			{
				*aThisBuffer++ = (char)-10;
				*(__int64*)aThisBuffer = aThisSize = (__int64)(aKey.marker_length ? (aKey.marker_length + 1) * sizeof(TCHAR) : 0);
				aThisBuffer += sizeof(__int64);
				if (aThisSize)
				{
					memcpy(aThisBuffer, aKey.marker, (size_t)aThisSize);
					aThisBuffer += aThisSize;
				}
			}
			else if (aVarType == SYM_FLOAT)
			{
				*aThisBuffer++ = (char)-9;
				*(double*)aThisBuffer = TokenToDouble(aKey);
				aThisBuffer += sizeof(__int64);
			}
			else if ((aIsValue = TokenToInt64(aKey)) > 4294967295)
			{
				*aThisBuffer++ = (char)-8;
				*(__int64*)aThisBuffer = aIsValue;
				aThisBuffer += sizeof(__int64);
			}
			else if (aIsValue > 65535)
			{
				*aThisBuffer++ = (char)-6;
				*(UINT*)aThisBuffer = (UINT)aIsValue;
				aThisBuffer += sizeof(UINT);
			}
			else if (aIsValue > 255)
			{
				*aThisBuffer++ = (char)-4;
				*(USHORT*)aThisBuffer = (USHORT)aIsValue;
				aThisBuffer += sizeof(USHORT);
			}
			else if (aIsValue > -1)
			{
				*aThisBuffer++ = (char)-2;
				*aThisBuffer = (BYTE)aIsValue;
				aThisBuffer += sizeof(BYTE);
			}
			else if (aIsValue > -129)
			{
				*aThisBuffer++ = (char)-1;
				*aThisBuffer = (char)aIsValue;
				aThisBuffer += sizeof(char);
			}
			else if (aIsValue > -32769)
			{
				*aThisBuffer++ = (char)-3;
				*(short*)aThisBuffer = (short)aIsValue;
				aThisBuffer += sizeof(short);
			}
			else if (aIsValue >= INT_MIN)
			{
				*aThisBuffer++ = (char)-5;
				*(int*)aThisBuffer = (int)aIsValue;
				aThisBuffer += sizeof(int);
			}
			else
			{
				*aThisBuffer++ = (char)-7;
				*(__int64*)aThisBuffer = (__int64)aIsValue;
				aThisBuffer += sizeof(__int64);
			}
		}

		// copy Value
		if (aIsObject = TokenToObject(aValue))
		{
			aKey.symbol = SYM_OBJECT;
			aKey.object = aIsObject;
			aObjects->Invoke(result_token, IT_CALL, _T("Has"), this_token, params + 1, 1);
			if (result_token.value_int64)
			{
				aObjects->Invoke(result_token, IT_GET, 0, this_token, params + 1, 1);
				*aThisBuffer++ = (char)11;
				*(__int64*)aThisBuffer = result_token.value_int64;
				aThisBuffer += sizeof(__int64);
			}
			else
			{
				aThisSize = ObjRawDump(aIsObject, aThisBuffer, aObjects, aObjCount);
				*(__int64*)(aThisBuffer + 1) = aThisSize - 1 - sizeof(__int64);
				*(char*)aThisBuffer = -1 * *(char*)aThisBuffer;
				aThisBuffer += aThisSize;
			}
		}
		else
		{
			aValue.var->ToTokenSkipAddRef(aValue);
			if ((aVarType = aValue.symbol) == SYM_STRING)
			{
				*aThisBuffer++ = (char)10;
				*(__int64*)aThisBuffer = aThisSize = (__int64)(aValue.marker_length ? (aValue.marker_length + 1) * sizeof(TCHAR) : 0);
				aThisBuffer += sizeof(__int64);
				if (aThisSize)
				{
					memcpy(aThisBuffer, aValue.marker, (size_t)aThisSize);
					aThisBuffer += aThisSize;
				}
			}
			else if (aVarType == SYM_FLOAT)
			{
				*aThisBuffer++ = (char)9;
				*(double*)aThisBuffer = TokenToDouble(aValue);
				aThisBuffer += sizeof(double);
			}
			else if ((aIsValue = TokenToInt64(aValue)) > 4294967295)
			{
				*aThisBuffer++ = (char)8;
				*(__int64*)aThisBuffer = aIsValue;
				aThisBuffer += sizeof(__int64);
			}
			else if (aIsValue > 65535)
			{
				*aThisBuffer++ = (char)6;
				*(UINT*)aThisBuffer = (UINT)aIsValue;
				aThisBuffer += sizeof(UINT);
			}
			else if (aIsValue > 255)
			{
				*aThisBuffer++ = (char)4;
				*(USHORT*)aThisBuffer = (USHORT)aIsValue;
				aThisBuffer += sizeof(USHORT);
			}
			else if (aIsValue > -1)
			{
				*aThisBuffer++ = (char)2;
				*aThisBuffer = (BYTE)aIsValue;
				aThisBuffer += sizeof(BYTE);
			}
			else if (aIsValue > -129)
			{
				*aThisBuffer++ = (char)1;
				*aThisBuffer = (char)aIsValue;
				aThisBuffer += sizeof(char);
			}
			else if (aIsValue > -32769)
			{
				*aThisBuffer++ = (char)3;
				*(short*)aThisBuffer = (short)aIsValue;
				aThisBuffer += sizeof(short);
			}
			else if (aIsValue > INT_MIN)
			{
				*aThisBuffer++ = (char)5;
				*aThisBuffer = (int)aIsValue;
				aThisBuffer += sizeof(int);
			}
			else
			{
				*aThisBuffer++ = (char)7;
				*(__int64*)aThisBuffer = (__int64)aIsValue;
				aThisBuffer += sizeof(__int64);
			}
		}

		// release object if it was assigned prevoiously when calling enum
		if (vkey.IsObject())
			vkey.ReleaseObject();
		if (vval.IsObject())
			vval.ReleaseObject();

		aKey.symbol = SYM_VAR;
		aKey.var = &vkey;
		aValue.symbol = SYM_VAR;
		aValue.var = &vval;
	}
	// release enumerator and free vars
	enumerator->Release();
	vkey.Free();
	vval.Free();
	return aThisBuffer - aBuffer;
}

//
// ObjDump()
//

BIF_DECL(BIF_ObjDump)
{
	aResultToken.symbol = SYM_INTEGER;
	IObject *aObject;
	if (!(aObject = TokenToObject(*aParam[0])))
	{
		g_script->ScriptError(ERR_INVALID_ARG_TYPE);
		aResultToken.symbol = SYM_STRING;
		aResultToken.marker = _T("");
		return;
	}
	//INT aCopyBuffer = aParamCount > 1 ? (int)TokenToInt64(*aParam[1]) : 0;
	DWORD aSize = (DWORD)ObjRawSize(aObject, NULL);
	char *aBuffer = (char*)malloc(aSize);
	if (!aBuffer)
	{
		g_script->ScriptError(ERR_OUTOFMEM);
		aResultToken.SetValue(_T(""));
		return;
	}
	*(__int64*)(aBuffer + 1) = aSize - 1 - sizeof(__int64) ;
	Map *aObjects = Map::Create();
	UINT aObjCount = 0;
	if (aSize != ObjRawDump(aObject, aBuffer, aObjects, aObjCount))
	{
		aObjects->Release();
		free(aBuffer);
		g_script->ScriptError(_T("Error dumping Object."));
		aResultToken.symbol = SYM_STRING;
		aResultToken.marker = _T("");
		return;
	}
	//aSize += sizeof(__int64);
	aObjects->Release();
	if (aParamCount > 1 && TokenToInt64(*aParam[1]) != 0)
	{
		LPVOID aDataBuf;
		TCHAR *pw[1024] = {};
		if (!ParamIndexIsOmittedOrEmpty(2))
		{
			TCHAR *pwd = TokenToString(*aParam[2]);
			size_t pwlen = _tcslen(TokenToString(*aParam[2]));
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
				g_script->ScriptError(ERR_OUTOFMEM);
				aResultToken.SetValue(_T(""));
				return;
			}
			memcpy(aBuffer, aDataBuf, aCompressedSize);
			aObject = new BufferObject(aBuffer, aCompressedSize);
			dynamic_cast<BufferObject*>(aObject)->SetBase(BufferObject::sPrototype);
			VirtualFree(aDataBuf, 0, MEM_RELEASE);
		}
	}
	else
	{
		aObject = new BufferObject(aBuffer, aSize);
		dynamic_cast<BufferObject*>(aObject)->SetBase(BufferObject::sPrototype);
	}
	aResultToken.SetValue(aObject);
}

//
// ObjRawLoad()
//

IObject* ObjRawLoad(char *aBuffer, IObject **&aObjects, UINT &aObjCount, UINT &aObjSize)
{
	IObject *aObject = NULL;
	ResultToken result_token, this_token, enum_token, aKey, aValue;
	ExprTokenType *params[] = { &aKey, &aValue };
	TCHAR buf[MAX_INTEGER_LENGTH];
	result_token.buf = buf;
	aKey.mem_to_free = NULL;
	aValue.mem_to_free = NULL;
	this_token.symbol = SYM_OBJECT;

	if (aObjCount == aObjSize)
	{
		IObject **newObjects = (IObject**)malloc(aObjSize * 2 * sizeof(IObject**));
		if (!newObjects)
			return 0;
		memcpy(newObjects, aObjects, aObjSize * sizeof(IObject**));
		free(aObjects);
		aObjects = newObjects;
		aObjSize *= 2;
	}
	
	char *aThisBuffer = aBuffer;
	char typekey,typeval, typeobj = *(char*)aThisBuffer;
	
	if (typeobj == -12 || typeobj == 12) //(_tcscmp(aObject->Type(), _T("Object")))
		aObject = Object::Create();
	else if (typeobj == -13 || typeobj == 13) //(_tcscmp(aObject->Type(), _T("Array")))
		aObject = Array::Create();
	else if (typeobj == -14 || typeobj == 14) //(_tcscmp(aObject->Type(), _T("Map")) || _tcscmp(aObject->Type(), _T("Struct")))
		aObject = Map::Create();
	else if (typeobj == -15 || typeobj == 15) //(_tcscmp(aObject->Type(), _T("Buffer")))
	{
		size_t mSize = (size_t)*(__int64*)(aThisBuffer + 1);
		aKey.buf = (LPTSTR)malloc(mSize);
		memmove((void *)aKey.buf, (aThisBuffer + 1 + sizeof(__int64)), mSize);
		if (!(aObject = new BufferObject((void *)aKey.buf, mSize)))
			g_script->ScriptError(ERR_OUTOFMEM);
		dynamic_cast<BufferObject*>(aObject)->SetBase(BufferObject::sPrototype);
		return aObject;
	}
	else
		g_script->ScriptError(ERR_TYPE_MISMATCH);
	if (!aObject)
		g_script->ScriptError(ERR_OUTOFMEM);
	aObjects[aObjCount++] = aObject;


	aThisBuffer++;
	size_t aSize = (size_t)*(__int64*)aThisBuffer;
	aThisBuffer += sizeof(__int64);

	this_token.object = aObject;

	for (char *end = aThisBuffer + aSize; aThisBuffer < end;)
	{
		typekey = *(char*)aThisBuffer++;
		if (typekey < -11)
		{
			aThisBuffer -= 1;  // required for the object type
			aKey.symbol = SYM_OBJECT;
			aKey.object = ObjRawLoad(aThisBuffer, aObjects, aObjCount, aObjSize);
			aThisBuffer += 1 + sizeof(__int64) + *(__int64*)(aThisBuffer + 1);
		}
		else if (typekey == -11)
		{
			aKey.symbol = SYM_OBJECT;
			aKey.object = aObjects[*(__int64*)aThisBuffer];
			aKey.object->AddRef();
			aThisBuffer += sizeof(__int64);
		}
		else if (typekey == -10)
		{
			aKey.symbol = SYM_STRING;
			__int64 aMarkerSize = *(__int64*)aThisBuffer;
			aThisBuffer += sizeof(__int64);
			if (aMarkerSize)
			{
				aKey.marker_length = (size_t)(aMarkerSize - 1) / sizeof(TCHAR);
				aKey.marker = (LPTSTR)aThisBuffer;
				aThisBuffer += aMarkerSize;
			}
			else
			{
				aKey.marker = _T("");
				aKey.marker_length = 0;
			}
		}
		else if (typekey == -9)
		{
			aKey.symbol = SYM_STRING;
			aKey.marker_length = FTOA(*(double*)aThisBuffer, buf, MAX_INTEGER_LENGTH);
			aKey.marker = buf;
			aThisBuffer += sizeof(__int64);
		}
		else
		{
			aKey.symbol = SYM_INTEGER;
			if (typekey == -8)
			{
				aKey.value_int64 = *(__int64*)aThisBuffer;
				aThisBuffer += sizeof(__int64);
			}
			else if (typekey == -6)
			{
				aKey.value_int64 = *(UINT*)aThisBuffer;
				aThisBuffer += sizeof(UINT);
			}
			else if (typekey == -4)
			{
				aKey.value_int64 = *(USHORT*)aThisBuffer;
				aThisBuffer += sizeof(USHORT);
			}
			else if (typekey == -2)
			{
				aKey.value_int64 = *(BYTE*)aThisBuffer;
				aThisBuffer += sizeof(BYTE);
			}
			else if (typekey == -1)
			{
				aKey.value_int64 = *(char*)aThisBuffer;
				aThisBuffer += sizeof(char);
			}
			else if (typekey == -3)
			{
				aKey.value_int64 = *(short*)aThisBuffer;
				aThisBuffer += sizeof(short);
			}
			else if (typekey == -5)
			{
				aKey.value_int64 = *(int*)aThisBuffer;
				aThisBuffer += sizeof(int);
			}
			else if (typekey == -7)
			{
				aKey.value_int64 = *(__int64*)aThisBuffer;
				aThisBuffer += sizeof(__int64);
			}
			else
			{
				g_script->ScriptError(ERR_INVALID_VALUE);
				return NULL;
			}
		}

		typeval = *(char*)aThisBuffer++;
		if (typeval > 11)
		{
			aValue.symbol = SYM_OBJECT;
			aValue.object = ObjRawLoad(--aThisBuffer, aObjects, aObjCount, aObjSize);   // aThisBuffer-- required to pass type of object
			aThisBuffer += 1 + sizeof(__int64) + *(__int64*)(aThisBuffer + 1);
		}
		else if (typeval == 11)
		{
			aValue.symbol = SYM_OBJECT;
			aValue.object = aObjects[*(__int64*)aThisBuffer];
			aValue.object->AddRef();
			aThisBuffer += sizeof(__int64);
		}
		else if (typeval == 10)
		{
			aValue.symbol = SYM_STRING;
			__int64 aMarkerSize = *(__int64*)aThisBuffer;
			aThisBuffer += sizeof(__int64);
			if (aMarkerSize)
			{
				aValue.marker_length = (size_t)(aMarkerSize - 1) / sizeof(TCHAR);
				aValue.marker = (LPTSTR)aThisBuffer;
				aThisBuffer += aMarkerSize;
			}
			else
			{
				aValue.marker = _T("");
				aValue.marker_length = 0;
			}
		}
		else if (typeval == 9)
		{
			aValue.symbol = SYM_FLOAT;
			aValue.value_double = *(double*)aThisBuffer;
			aThisBuffer += sizeof(__int64);
		}
		else
		{
			aValue.symbol = SYM_INTEGER;
			if (typeval == 8)
			{
				aValue.value_int64 = *(__int64*)aThisBuffer;
				aThisBuffer += sizeof(__int64);
			}
			else if (typeval == 6)
			{
				aValue.value_int64 = *(UINT*)aThisBuffer;
				aThisBuffer += sizeof(UINT);
			}
			else if (typeval == 4)
			{
				aValue.value_int64 = *(USHORT*)aThisBuffer;
				aThisBuffer += sizeof(USHORT);
			}
			else if (typeval == 2)
			{
				aValue.value_int64 = *(BYTE*)aThisBuffer;
				aThisBuffer += sizeof(BYTE);
			}
			else if (typeval == 1)
			{
				aValue.value_int64 = *(char*)aThisBuffer;
				aThisBuffer += sizeof(char);
			}
			else if (typeval == 3)
			{
				aValue.value_int64 = *(short*)aThisBuffer;
				aThisBuffer += sizeof(short);
			}
			else if (typeval == 5)
			{
				aValue.value_int64 = *(int*)aThisBuffer;
				aThisBuffer += sizeof(int);
			}
			else if (typeval == 7)
			{
				aValue.value_int64 = *(__int64*)aThisBuffer;
				aThisBuffer += sizeof(__int64);
			}
			else
			{
				g_script->ScriptError(ERR_INVALID_VALUE);
				return NULL;
			}
		}
		
		if (typeobj == -12 || typeobj == 12)
			aObject->Invoke(result_token, IT_SET, aKey.marker, this_token, params + 1, 1);
		else if (typeobj == -13 || typeobj == 13)
			aObject->Invoke(result_token, IT_CALL, _T("Push"), this_token, params + 1, 1);
		else
			aObject->Invoke(result_token, IT_SET, 0, this_token, params , 2);
		aKey.Free();
		aValue.Free();
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
	IObject *buffer_obj;
	char *aBuffer;
	size_t  max_bytes = SIZE_MAX;

	if (TokenIsNumeric(**aParam))
		aBuffer = (char*)TokenToInt64(**aParam);
	else if (buffer_obj = TokenToObject(**aParam))
	{
		size_t ptr;
		GetBufferObjectPtr(aResultToken, buffer_obj, ptr, max_bytes);
		if (aResultToken.Exited())
			return;
		aBuffer = (char*)ptr;
	}
	else
	{ // FileRead Mode
		if (GetFileAttributes(aPath) == 0xFFFFFFFF)
		{
			aResultToken.SetValue(_T(""));
			return;
		}
		FILE *fp;

		fp = _tfopen(aPath, _T("rb"));
		if (fp == NULL)
		{
			aResultToken.SetValue(_T(""));
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
			aResultToken.SetValue(_T(""));
			g_script->ScriptError(_T("ObjLoad: Password mismatch."));
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
		aResultToken.SetValue(_T(""));
		return;
	}
	free(aObjects);
	if (aFreeBuffer)
		free(aBuffer);
}

//
// Object()
//

BIF_DECL(BIF_Object)
{
	IObject *obj = Object::Create(aParam, aParamCount, &aResultToken);
	if (obj)
	{
		// DO NOT ADDREF: the caller takes responsibility for the only reference.
		_f_return(obj);
	}
	//else: an error was already thrown.
}

//
// UObject() - unsorted properties
//

BIF_DECL(BIF_UObject)
{
	IObject *obj = Object::Create(aParam, aParamCount, &aResultToken, true);

	if (obj)
	{
		// DO NOT ADDREF: the caller takes responsibility for the only reference.
		_f_return(obj);
	}
	//else: an error was already thrown.
}


//
// BIF_Array - Array(items*)
//

BIF_DECL(BIF_Array)
{
	if (auto arr = Array::Create(aParam, aParamCount))
		_f_return(arr);
	_f_throw(ERR_OUTOFMEM);
}


//
// BIF_UArray - Array(items*) - unsorted properties
//

BIF_DECL(BIF_UArray)
{
	if (auto arr = Array::Create(aParam, aParamCount, true))
		_f_return(arr);
	_f_throw(ERR_OUTOFMEM);
}

//
// Map()
//

BIF_DECL(BIF_Map)
{
	if (aParamCount & 1 && !(aParamCount == 1 && TokenToObject(*aParam[0])))
		_f_throw(ERR_PARAM_COUNT_INVALID);
	auto obj = Map::Create(aParam, aParamCount);
	if (!obj)
		_f_throw(ERR_OUTOFMEM);
	_f_return(obj);
}


//
// UMap() - unsorted map
//

BIF_DECL(BIF_UMap)
{
	if (aParamCount & 1 && !(aParamCount == 1 && TokenToObject(*aParam[0])))
		_f_throw(ERR_PARAM_COUNT_INVALID);
	auto obj = Map::Create(aParam, aParamCount, true);
	if (!obj)
		_f_throw(ERR_OUTOFMEM);
	_f_return(obj);
}
	

//
// IsObject
//

BIF_DECL(BIF_IsObject)
{
	_f_return_b(TokenToObject(*aParam[0]) != nullptr);
}


//
// Functions for accessing built-in methods (even if obscured by a user-defined method).
//

BIF_DECL(BIF_ObjXXX)
{
	aResultToken.symbol = SYM_STRING;
	aResultToken.marker = _T(""); // Set default for CallBuiltin().
	
	Object *obj = dynamic_cast<Object*>(TokenToObject(*aParam[0]));
	if (obj)
		obj->CallBuiltin(_f_callee_id, aResultToken, aParam + 1, aParamCount - 1);
	else
		_f_throw_type(_T("Object"), *aParam[0]);
}


//
// ObjAddRef/ObjRelease - used with pointers rather than object references.
//

BIF_DECL(BIF_ObjAddRefRelease)
{
	IObject *obj = (IObject *)TokenToInt64(*aParam[0]);
	if (obj < (IObject *)65536) // Rule out some obvious errors.
		_f_throw(ERR_PARAM1_INVALID);
	if (_f_callee_id == FID_ObjAddRef)
		_f_return_i(obj->AddRef());
	else
		_f_return_i(obj->Release());
}


//
// ObjBindMethod(Obj, Method, Params...)
//

BIF_DECL(BIF_ObjBindMethod)
{
	IObject *func, *bound_func;
	if (  !(func = TokenToFunctor(*aParam[0]))  )
		_f_throw(ERR_PARAM1_INVALID);
	LPCTSTR name = nullptr;
	if (aParamCount > 1)
	{
		if (aParam[1]->symbol != SYM_MISSING)
			name = TokenToString(*aParam[1], _f_number_buf);
		aParam += 2;
		aParamCount -= 2;
	}
	else
		aParamCount = 0;
	bound_func = BoundFunc::Bind(func, IT_CALL, name, aParam, aParamCount);
	func->Release();
	if (!bound_func)
		_f_throw(ERR_OUTOFMEM);
	_f_return(bound_func);
}


//
// ObjPtr/ObjPtrAddRef/ObjFromPtr - Convert between object reference and IObject pointer.
//

BIF_DECL(BIF_ObjPtr)
{
	if (_f_callee_id >= FID_ObjFromPtr)
	{
		auto obj = (IObject *)ParamIndexToInt64(0);
		if (obj < (IObject *)65536) // Prevent some obvious errors.
			_f_throw(ERR_PARAM1_INVALID);
		if (_f_callee_id == FID_ObjFromPtrAddRef)
			obj->AddRef();
		_f_return(obj);
	}
	else // FID_ObjPtr or FID_ObjPtrAddRef.
	{
		auto obj = ParamIndexToObject(0);
		if (!obj)
			_f_throw_type(_T("object"), *aParam[0]);
		if (_f_callee_id == FID_ObjPtrAddRef)
			obj->AddRef();
		_f_return((UINT_PTR)obj);
	}
}


//
// ObjSetBase/ObjGetBase - Change or return Object's base without invoking any meta-functions.
//

BIF_DECL(BIF_Base)
{
	Object *obj = dynamic_cast<Object*>(TokenToObject(*aParam[0]));
	if (_f_callee_id == FID_ObjSetBase)
	{
		if (!obj)
			_f_throw_type(_T("Object"), *aParam[0]);
		auto new_base = dynamic_cast<Object *>(TokenToObject(*aParam[1]));
		if (!obj->SetBase(new_base, aResultToken))
			return;
	}
	else // ObjGetBase
	{
		Object *obj_base;
		if (obj)
			obj_base = obj->Base();
		else
			obj_base = Object::ValueBase(*aParam[0]);
		if (obj_base)
		{
			obj_base->AddRef();
			_f_return(obj_base);
		}
		// Otherwise, it's something with no base, so return "".
		// Could be Object::sAnyPrototype, a ComObject or perhaps SYM_MISSING.
	}
	_f_return_empty;
}


bool Object::HasBase(ExprTokenType &aValue, IObject *aBase)
{
	if (auto obj = dynamic_cast<Object *>(TokenToObject(aValue)))
	{
		return obj->IsDerivedFrom(aBase);
	}
	if (auto value_base = Object::ValueBase(aValue))
	{
		return value_base == aBase || value_base->IsDerivedFrom(aBase);
	}
	// Something that doesn't fit in our type hierarchy, like a ComObject.
	// Returning false seems correct and more useful than throwing.
	// HasBase(ComObj, "".base.base) ; False, it's not a primitive value.
	// HasBase(ComObj, Object.Prototype) ; False, it's not one of our Objects.
	return false;
}


BIF_DECL(BIF_HasBase)
{
	auto that_base = ParamIndexToObject(1);
	if (!that_base)
		_f_throw_type(_T("object"), *aParam[1]);
	_f_return_b(Object::HasBase(*aParam[0], that_base));
}


Object *ParamToObjectOrBase(ExprTokenType &aToken, ResultToken &aResultToken)
{
	Object *obj;
	if (  (obj = dynamic_cast<Object *>(TokenToObject(aToken)))
		|| (obj = Object::ValueBase(aToken))  )
		return obj;
	aResultToken.Error(ERR_PARAM1_INVALID);
	return nullptr;
}


BIF_DECL(BIF_HasProp)
{
	auto obj = ParamToObjectOrBase(*aParam[0], aResultToken);
	if (!obj)
		return;
	_f_return_b(obj->HasProp(ParamIndexToString(1, _f_number_buf)));
}


BIF_DECL(BIF_HasMethod)
{
	auto obj = ParamToObjectOrBase(*aParam[0], aResultToken);
	if (!obj)
		return;
	_f_return_b(obj->HasMethod(ParamIndexToString(1, _f_number_buf)));
}


BIF_DECL(BIF_GetMethod)
{
	auto obj = ParamToObjectOrBase(*aParam[0], aResultToken);
	if (!obj)
		return;
	obj->GetMethod(aResultToken, ParamIndexToString(1, _f_number_buf));
}
