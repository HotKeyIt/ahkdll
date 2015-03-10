#include "stdafx.h" // pre-compiled headers
#include "defines.h"
#include "globaldata.h"
#include "script.h"

#include "script_object.h"


//
// BIF_Struct - Create structure
//

BIF_DECL(BIF_Struct)
{
	// At least the definition for structure must be given
	if (!aParamCount)
		return;
	IObject *obj = Struct::Create(aParam,aParamCount);
	if (obj)
	{
		aResultToken.symbol = SYM_OBJECT;
		aResultToken.object = obj;
		return;
		// DO NOT ADDREF: after we return, the only reference will be in aResultToken.
	}
	// indicate error
	aResultToken.symbol = SYM_STRING;
	aResultToken.marker = _T("");
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
	int unionoffset[16];			// backup offset before we enter union or structure
	int unionsize[16];				// calculate unionsize
	bool unionisstruct[16];		// updated to move offset for structure in structure
	int structalign[16];			// keep track of struct alignment
	int totalunionsize = 0;			// total size of all unions and structures in structure
	int uniondepth = 0;				// count how deep we are in union/structure
	int align = 0;
	int *aligntotal = &align;		// pointer alignment for total structure
	int thissize;					// used to save size returned from IsDefaultType
	int maxsize = 0;				// max size of union or struct

	// following are used to find variable and also get size of a structure defined in variable
	// this will hold the variable reference and offset that is given to size() to align if necessary in 64-bit
	ResultType Result = OK;
	ExprTokenType ResultToken;
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
		aResultToken.value_int64 = obj->mSize + (aParamCount > 1 ? TokenToInt64(*aParam[1],true) : 0);
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
			// ignore even any wrong input here so it is even {mystructure...} for struct and  {anyother string...} for union
			buf = _tcschr(buf,'{') + 1;
			continue;
		} 
		else if (*buf == '}')
		{	// update union
			// now restore or correct offset even if we had a structure in structure
			if (uniondepth > 1 && unionisstruct[uniondepth - 1])
			{
				if (mod = offset % structalign[uniondepth])
					offset += (structalign[uniondepth] - mod) % structalign[uniondepth];
			}
			else
				offset = unionoffset[uniondepth];
			if (unionisstruct[uniondepth] && structalign[uniondepth] > *aligntotal)
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
				return;
			_tcsncpy(tempbuf,buf,buf_size);
			tempbuf[buf_size] = '\0';
		}
		else if (_tcslen(buf) > LINE_SIZE - 1)
			return;
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
			StrReplace(tempbuf, intbuf, _T(""), SCS_SENSITIVE, UINT_MAX, LINE_SIZE);
		}

		// Pointer, while loop will continue here because we only need size
		if (_tcschr(tempbuf,'*'))
		{
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
				return;
			_tcsncpy(defbuf + 1,tempbuf,_tcscspn(tempbuf,_T("\t [")));
			_tcscpy(defbuf + 1 + _tcscspn(tempbuf,_T("\t [")),_T(" "));
		}
		// else // Not 'TypeOnly' definition because there are more than one fields in array so use default type UInt
			// _tcscpy(defbuf,_T(" UInt "));
		
		// Now find size in default types array and create new field
		// If Type not found, resolve type to variable and get size of struct defined in it
		if ((thissize = IsDefaultType(defbuf)))
		{
			if (!_tcscmp(defbuf,_T(" bool ")))
				thissize = 1;
			// align offset
			if (thissize > 1 && (mod = offset % thissize))
				offset += (thissize - mod) % thissize;
			offset += thissize * (arraydef ? arraydef : 1);
			if (thissize > *aligntotal)
				*aligntotal = thissize; // > ptrsize ? ptrsize : thissize;
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
				param[1]->value_int64 = (__int64)offset;
				BIF_sizeof(Result,ResultToken,param,3);
				if (ResultToken.symbol != SYM_INTEGER)
				{	// could not resolve structure
					return;
				}
				if (offset && (mod = offset % *aligntotal))
					offset += (*aligntotal - mod) % *aligntotal;
				// sizeof was given an offset that it applied and aligned if necessary, so set offset =  and not +=
				offset = (int)ResultToken.value_int64 + (arraydef ? ((arraydef - 1) * ((int)ResultToken.value_int64 - offset)) : 0);
			}
			else // No variable was found and it is not default type so we can't determine size, return empty string.
				return;
		}
		// update union size
		if (uniondepth)
		{
			if ((maxsize = offset - unionoffset[uniondepth]) > unionsize[uniondepth])
				unionsize[uniondepth] = maxsize;
			// reset offset if in union and union is not a structure
			if (!unionisstruct[uniondepth])
				offset = unionoffset[uniondepth];
		}
		// Move buffer pointer now
		if (_tcschr(buf,'}') && (!StrChrAny(buf, _T(";,")) || _tcschr(buf,'}') < StrChrAny(buf, _T(";,"))))
			buf += _tcscspn(buf,_T("}")); // keep } character to update union
		else if (StrChrAny(buf, _T(";,")))
			buf += _tcscspn(buf,_T(";,")) + 1;
		else
			buf += _tcslen(buf);
	}
	if (*aligntotal && (mod = offset % *aligntotal)) // align only if offset was not given
		offset += (*aligntotal - mod) % *aligntotal;
	aResultToken.symbol = SYM_INTEGER;
	aResultToken.value_int64 = offset;
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

	// __Init was added so that instance variables can be initialized in the correct order
	// (beginning at the root class and ending at class_object) before __New is called.
	// It shouldn't be explicitly defined by the user, but auto-generated in DefineClassVars().
	name_token.marker = _T("__Init");
	result = class_object->Invoke(aResultToken, this_token, IT_CALL | IF_METAOBJ, aParam, 1);
	if (result != INVOKE_NOT_HANDLED)
	{
		// See similar section below for comments.
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
		if (result == FAIL)
		{
			new_object->Release();
			aParam[0] = class_token; // Restore it to original caller-supplied value.
			return;
		}
	}
	
	// __New may be defined by the script for custom initialization code.
	name_token.marker = Object::sMetaFuncName[4]; // __New
	if (class_object->Invoke(aResultToken, this_token, IT_CALL | IF_METAOBJ, aParam, aParamCount) != EARLY_RETURN)
	{
		// Although it isn't likely to happen, if __New points at a built-in function or if mBase
		// (or an ancestor) is not an Object (i.e. it's a ComObject), aResultToken can be set even when
		// the result is not EARLY_RETURN.  So make sure to clean up any result we're not going to use.
		if (aResultToken.symbol == SYM_OBJECT)
			aResultToken.object->Release();
		if (aResultToken.mem_to_free)
		{
			// This can be done by our caller, but is done here for maintainability; i.e. because
			// some callers might expect mem_to_free to be NULL when the result isn't a string.
			free(aResultToken.mem_to_free);
			aResultToken.mem_to_free = NULL;
		}
		// Either it wasn't handled (i.e. neither this class nor any of its super-classes define __New()),
		// or there was no explicit "return", so just return the new object.
		aResultToken.symbol = SYM_OBJECT;
		aResultToken.object = this_token.object;
	}
	else
		new_object->Release();
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
BIF_DECL(BIF_Obj##name) \
{ \
	aResultToken.symbol = SYM_STRING; \
	aResultToken.marker = _T(""); \
	\
	Object *obj = dynamic_cast<Object*>(TokenToObject(*aParam[0])); \
	if (obj) \
		obj->_##name(aResultToken, aParam + 1, aParamCount - 1); \
}

BIF_METHOD(Insert)
BIF_METHOD(Remove)
BIF_METHOD(GetCapacity)
BIF_METHOD(SetCapacity)
BIF_METHOD(GetAddress)
BIF_METHOD(MaxIndex)
BIF_METHOD(MinIndex)
BIF_METHOD(Count)
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