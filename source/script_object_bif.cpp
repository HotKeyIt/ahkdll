#include "stdafx.h" // pre-compiled headers
#include "defines.h"
#include "globaldata.h"
#include "script.h"

#include "script_object.h"


extern ExprOpFunc g_ObjGet, g_ObjSet;

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
	int thissize;					// used to save size returned from IsDefaultType
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

	// Set result to empty string to identify error
	aResultToken.symbol = SYM_STRING;
	aResultToken.marker = _T("");
	
	// if first parameter is an object (struct), simply return its size
	if (TokenToObject(*aParam[0]))
	{
		aResultToken.symbol = SYM_INTEGER;
		Struct *obj = (Struct*)TokenToObject(*aParam[0]);
		aResultToken.value_int64 = obj->mSize + (aParamCount > 1 ? TokenToInt64(*aParam[1]) : 0);
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
		//else // Not 'TypeOnly' definition because there are more than one fields in structure so use default type UInt
		//{
			// Commented out following line to keep previous or default UInt definition like in c++, e.g. "Int x,y,Char a,b", 
			// Note: separator , or ; can be still used but
			// _tcscpy(defbuf,_T(" UInt "));
		//}
		
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
				BIF_sizeof(ResultToken,param,3);
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
// Object()
//

BIF_DECL(BIF_Object)
{
	IObject *obj = NULL;

	if (aParamCount == 1) // L33: POTENTIALLY UNSAFE - Cast IObject address to object reference.
	{
		obj = (IObject *)TokenToInt64(*aParam[0]);
		if (obj < (IObject *)65536) // Prevent some obvious errors.
			obj = NULL;
		else
			obj->AddRef();
	}
	else
		obj = Object::Create(aParam, aParamCount);

	if (obj)
	{
		// DO NOT ADDREF: the caller takes responsibility for the only reference.
		_f_return(obj);
	}
	else
		_f_throw(aParamCount == 1 ? ERR_PARAM1_INVALID : ERR_OUTOFMEM);
}


//
// BIF_Array - Array(items*)
//

BIF_DECL(BIF_Array)
{
	if (aResultToken.object = Object::CreateArray(aParam, aParamCount))
	{
		aResultToken.symbol = SYM_OBJECT;
		return;
	}
	_f_throw(ERR_OUTOFMEM);
}
	

//
// BIF_IsObject - IsObject(obj)
//

BIF_DECL(BIF_IsObject)
{
	int i;
	for (i = 0; i < aParamCount && TokenToObject(*aParam[i]); ++i);
	_f_return_b(i == aParamCount); // TRUE if all are objects.  Caller has ensured aParamCount > 0.
}
	

//
// Op_ObjInvoke - Handles the object operators: x.y, x[y], x.y(), x.y := z, etc.
//

BIF_DECL(Op_ObjInvoke)
{
    int invoke_type = _f_callee_id;
    IObject *obj;
    ExprTokenType *obj_param;

	// Set default return value for Invoke().
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
    
	ResultType result;
    if (obj)
	{
		bool param_is_var = obj_param->symbol == SYM_VAR;
		if (param_is_var)
			// Since the variable may be cleared as a side-effect of the invocation, call AddRef to ensure the object does not expire prematurely.
			// This is not necessary for SYM_OBJECT since that reference is already counted and cannot be released before we return.  Each object
			// could take care not to delete itself prematurely, but it seems more proper, more reliable and more maintainable to handle it here.
			obj->AddRef();
        result = obj->Invoke(aResultToken, *obj_param, invoke_type, aParam, aParamCount);
		if (param_is_var)
			obj->Release();
	}
	// Invoke meta-functions of g_MetaObject.
	else if (INVOKE_NOT_HANDLED == (result = g_MetaObject.Invoke(aResultToken, *obj_param, invoke_type | IF_META, aParam, aParamCount)))
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
				result = g_MetaObject.Invoke(aResultToken, base_token, invoke_type, aParam + 1, aParamCount - 1);
			}
			else					// "".base
			{
				// Return a reference to g_MetaObject.  No need to AddRef as g_MetaObject ignores it.
				aResultToken.symbol = SYM_OBJECT;
				aResultToken.object = &g_MetaObject;
				result = OK;
			}
		}
		else
		{
			// Since it wasn't handled (not even by g_MetaObject), maybe warn at this point:
			if (obj_param->symbol == SYM_VAR)
				obj_param->var->MaybeWarnUninitialized();
		}
	}
	if (result == INVOKE_NOT_HANDLED)
	{
		// Invocation not handled. Either there was no target object, or the object doesn't handle
		// this method/property.  For Object (associative arrays), only CALL should give this result.
		if (!obj)
			_f_throw(ERR_NO_OBJECT, g->ExcptDeref ? g->ExcptDeref->marker : _T(""));
		else
			_f_throw(ERR_NO_MEMBER, aParamCount && *TokenToString(*aParam[0]) ? TokenToString(*aParam[0]) : g->ExcptDeref ? g->ExcptDeref->marker : _T(""));
	}
	else if (result == FAIL || result == EARLY_EXIT) // For maintainability: SetExitResult() might not have been called.
	{
		aResultToken.SetExitResult(result);
	}
}
	

//
// Op_ObjGetInPlace - Handles part of a compound assignment like x.y += z.
//

BIF_DECL(Op_ObjGetInPlace)
{
	// Since the most common cases have two params, the "param count" param is omitted in
	// those cases. Otherwise we have one visible parameter, which indicates the number of
	// actual parameters below it on the stack.
	aParamCount = aParamCount ? (int)TokenToInt64(*aParam[0]) : 2; // x[<n-1 params>] : x.y
	Op_ObjInvoke(aResultToken, aParam - aParamCount, aParamCount);
}


//
// Op_ObjNew - Handles "new" as in "new Class()".
//

BIF_DECL(Op_ObjNew)
{
	// Set default return value for Invoke().
	aResultToken.symbol = SYM_STRING;
	aResultToken.marker = _T("");

	ExprTokenType *class_token = aParam[0]; // Save this to be restored later.

	IObject *class_object = TokenToObject(*class_token);
	if (!class_object)
		_f_throw(ERR_NEW_NO_CLASS);

	Object *new_object = Object::Create();
	if (!new_object)
		_f_throw(ERR_OUTOFMEM);

	new_object->SetBase(class_object);

	ExprTokenType name_token, this_token;
	this_token.symbol = SYM_OBJECT;
	this_token.object = new_object;
	name_token.symbol = SYM_STRING;
	aParam[0] = &name_token;

	ResultType result;
	LPTSTR buf = aResultToken.buf; // In case Invoke overwrites aResultToken.buf via the union.

	// __Init was added so that instance variables can be initialized in the correct order
	// (beginning at the root class and ending at class_object) before __New is called.
	// It shouldn't be explicitly defined by the user, but auto-generated in DefineClassVars().
	name_token.marker = _T("__Init");
	name_token.marker_length = 6;
	result = class_object->Invoke(aResultToken, this_token, IT_CALL | IF_METAOBJ, aParam, 1);
	if (result != INVOKE_NOT_HANDLED)
	{
		if (result != OK)
		{
			new_object->Release();
			aParam[0] = class_token; // Restore it to original caller-supplied value.
			aResultToken.SetExitResult(result);
			return;
		}
		// See similar section below for comments.
		aResultToken.Free();
		// Reset to defaults for __New, invoked below.
		aResultToken.mem_to_free = NULL;
		aResultToken.symbol = SYM_STRING;
		aResultToken.marker = _T("");
		aResultToken.marker_length = -1;
		aResultToken.buf = buf;
	}
	
	// __New may be defined by the script for custom initialization code.
	name_token.marker = _T("__New");
	name_token.marker_length = 5;
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
		// An error was raised within __New() or while trying to call it.
		new_object->Release();
		aResultToken.SetExitResult(result);
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
// Op_ObjIncDec - Handles pre/post-increment/decrement for object fields, such as ++x[y].
//

BIF_DECL(Op_ObjIncDec)
{
	SymbolType op = (SymbolType)_f_callee_id;

	ResultToken temp_result;
	// Set the defaults expected by Op_ObjInvoke:
	temp_result.InitResult(aResultToken.buf);
	temp_result.symbol = SYM_INTEGER;
	temp_result.func = &g_ObjGet;

	// Retrieve the current value.  Do it this way instead of calling Object::Invoke
	// so that if aParam[0] is not an object, g_MetaObject is correctly invoked.
	Op_ObjInvoke(temp_result, aParam, aParamCount);

	if (temp_result.Exited()) // Implies no return value.
	{
		aResultToken.SetExitResult(temp_result.Result());
		return;
	}

	ExprTokenType current_value, value_to_set;
	switch (value_to_set.symbol = current_value.symbol = TokenIsNumeric(temp_result))
	{
	case PURE_INTEGER:
		value_to_set.value_int64 = (current_value.value_int64 = TokenToInt64(temp_result))
			+ ((op == SYM_POST_INCREMENT || op == SYM_PRE_INCREMENT) ? +1 : -1);
		break;

	case PURE_FLOAT:
		value_to_set.value_double = (current_value.value_double = TokenToDouble(temp_result))
			+ ((op == SYM_POST_INCREMENT || op == SYM_PRE_INCREMENT) ? +1 : -1);
		break;

	default: // PURE_NOT_NUMERIC == SYM_STRING.
		// Value is non-numeric, so assign and return "".
		value_to_set.marker = _T(""); value_to_set.marker_length = 0;
		current_value.marker = _T(""); current_value.marker_length = 0;
	}

	// Free the object or string returned by Op_ObjInvoke, if applicable.
	temp_result.Free();

	// Although it's likely our caller's param array has enough space to hold the extra
	// parameter, there's no way to know for sure whether it's safe, so we allocate our own:
	ExprTokenType **param = (ExprTokenType **)_alloca((aParamCount + 1) * sizeof(ExprTokenType *));
	memcpy(param, aParam, aParamCount * sizeof(ExprTokenType *)); // Copy caller's param pointers.
	param[aParamCount++] = &value_to_set; // Append new value as the last parameter.

	if (op == SYM_PRE_INCREMENT || op == SYM_PRE_DECREMENT)
	{
		aResultToken.func = &g_ObjSet;
		// Set the new value and pass the return value of the invocation back to our caller.
		// This should be consistent with something like x.y := x.y + 1.
		Op_ObjInvoke(aResultToken, param, aParamCount);
	}
	else // SYM_POST_INCREMENT || SYM_POST_DECREMENT
	{
		// Must be re-initialized (and must use g_ObjSet instead of g_ObjGet):
		temp_result.InitResult(aResultToken.buf);
		temp_result.symbol = SYM_INTEGER;
		temp_result.func = &g_ObjSet;
		
		// Set the new value.
		Op_ObjInvoke(temp_result, param, aParamCount);

		if (temp_result.Exited()) // Implies no return value.
		{
			aResultToken.SetExitResult(temp_result.Result());
			return;
		}
		
		// Dispose of the result safely.
		temp_result.Free();

		// Return the previous value.
		aResultToken.symbol = current_value.symbol;
		aResultToken.value_int64 = current_value.value_int64; // Union copy.  Includes marker_length on x86.
#ifdef _WIN64
		aResultToken.marker_length = current_value.marker_length; // For simplicity, symbol isn't checked.
#endif
	}
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
		_f_throw(ERR_NO_OBJECT);
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
	if (  !(func = TokenToObject(*aParam[0]))
		&& !(func = TokenToFunc(*aParam[0]))  )
	{
		_f_throw(ERR_PARAM1_INVALID);
	}
	if (  !(bound_func = BoundFunc::Bind(func, aParam + 1, aParamCount - 1, IT_CALL))  )
		_f_throw(ERR_OUTOFMEM);
	_f_return(bound_func);
}


//
// ObjRawSet - set a value without invoking any meta-functions.
//

BIF_DECL(BIF_ObjRawSet)
{
	Object *obj = dynamic_cast<Object*>(TokenToObject(*aParam[0]));
	if (!obj)
		_f_throw(ERR_PARAM1_INVALID);
	if (!obj->SetItem(*aParam[1], *aParam[2]))
		_f_throw(ERR_OUTOFMEM);
	_f_return_empty;
}