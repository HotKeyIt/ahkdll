#include "stdafx.h" // pre-compiled headers
#include "defines.h"
#include "application.h"
#include "globaldata.h"
#include "script.h"

#include "script_object.h"


//
// Struct::Create - Called by BIF_ObjCreate to create a new object, optionally passing key/value pairs to set.
//

Struct *Struct::Create(ExprTokenType *aParam[], int aParamCount)
// This code is very similar to BIF_sizeof so should be maintained together
{
	int ptrsize = sizeof(UINT_PTR); // Used for pointers on 32/64 bit system
	int offset = 0;					// also used to calculate total size of structure
	int arraydef = 0;				// count arraysize to update offset
	int unionoffset[100];			// backup offset before we enter union or structure
	int unionsize[100];				// calculate unionsize
	bool unionisstruct[100];			// updated to move offset for structure in structure
	int totalunionsize = 0;			// total size of all unions and structures in structure
	int uniondepth = 0;				// count how deep we are in union/structure
	int ispointer = NULL;			// identify pointer and how deep it goes
#ifdef _WIN64
	int aligntotal = 0;				// pointer alignment for total structure
#endif
	int thissize;					// used to check if type was found in above array.
	
	// following are used to find variable and also get size of a structure defined in variable
	// this will hold the variable reference and offset that is given to size() to align if necessary in 64-bit
	ResultType Result = OK;
	ExprTokenType ResultToken;
	ExprTokenType Var1,Var2,Var3;
	ExprTokenType *param[] = {&Var1,&Var2,&Var3};
	Var1.symbol = SYM_VAR;
	Var2.symbol = SYM_INTEGER;

	// will hold pointer to structure definition string while we parse trough it
	TCHAR *buf;
	size_t buf_size;

	// Use enough buffer to accept any definition in a field.
	TCHAR tempbuf[LINE_SIZE]; // just in case if we have a long comment

	// definition and field name are same max size as variables
	// also add enough room to store pointers (**) and arrays [1000]
	TCHAR defbuf[MAX_VAR_NAME_LENGTH*2 + 40]; // give more room to use local or static variable Function(variable)
	TCHAR keybuf[MAX_VAR_NAME_LENGTH + 40];

	// buffer for arraysize + 2 for bracket ] and terminating character
	TCHAR intbuf[MAX_INTEGER_LENGTH + 2];

	FieldType *field;				// used to define a field
	// Structure object is saved in fixed order
	// insert_pos is simply increased each time
	// for loop will enumerate items in same order as it was created
	IndexType insert_pos = 0;

	// the new structure object
	Struct *obj = new Struct();


	// Parameter passed to IsDefaultType needs to be ' Definition '
	// this is because spaces are used as delimiters ( see IsDefaultType function )
	// So first character will be always a space
	defbuf[0] = ' ';

	if (TokenToObject(*aParam[0]))
	{
		obj->Release();
		obj = ((Struct *)TokenToObject(*aParam[0]))->Clone();
		if (aParamCount > 2)
		{
			obj->mStructMem = (UINT_PTR*)TokenToInt64(*aParam[1]);
			obj->ObjectToStruct(TokenToObject(*aParam[2]));
		}
		else if (aParamCount > 1)
		{
			if (TokenToObject(*aParam[1]))
			{
				obj->mStructMem = (UINT_PTR *)malloc(obj->mSize);
				obj->mMemAllocated = obj->mSize;
				memset(obj->mStructMem,NULL,offset);
				obj->ObjectToStruct(TokenToObject(*aParam[1]));
			}
			else
				obj->mStructMem = (UINT_PTR*)TokenToInt64(*aParam[1]);
		}
		else
		{
			obj->mStructMem = (UINT_PTR *)malloc(obj->mSize);
			obj->mMemAllocated = obj->mSize;
			memset(obj->mStructMem,NULL,obj->mSize);
		}
		return obj;
	}

	// Set initial capacity to avoid multiple expansions.
	// For simplicity, failure is handled by the loop below.
	obj->SetInternalCapacity(aParamCount >> 1);
	
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
		{	// union or structure in structure definition
			if (!uniondepth++)
				totalunionsize = 0; // done here to reduce code
			if (_tcsstr(buf,_T("struct")) && _tcsstr(buf,_T("struct")) < _tcschr(buf,'{'))
				unionisstruct[uniondepth] = true; // mark that union is a structure
			else 
				unionisstruct[uniondepth] = false;
			// backup offset because we need to set it back after this union / struct was parsed
			// unionsize is initialized to 0 and buffer moved to next character
			unionoffset[uniondepth] = offset; // backup offset
			unionsize[uniondepth] = 0;
			// ignore even any wrong input here so it is even {mystructure...} for struct and  {anyother string...} for union
			buf = _tcschr(buf,'{') + 1;
			continue;
		} 
		else if (*buf == '}')
		{	// update union
			// restore offset even if we had a structure in structure
			if (unionsize[uniondepth]>totalunionsize)
				totalunionsize = unionsize[uniondepth];
			// last item in union or structure, update offset now if not struct, for struct offset is up to date
			if (--uniondepth == 0)
			{
				if (!unionisstruct[uniondepth + 1]) // because it was decreased above
					offset += totalunionsize;
			}
			else 
				offset = unionoffset[uniondepth];
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
				return NULL;
			}
			_tcsncpy(tempbuf,buf,buf_size);
			tempbuf[buf_size] = '\0';
		}
		else if (_tcslen(buf) > LINE_SIZE - 1)
		{
			obj->Release();
			return NULL;
		}
		else
			_tcscpy(tempbuf,buf);

		// Trim trailing spaces
		rtrim(tempbuf);
		
		// Pointer
		if (_tcschr(tempbuf,'*'))
			ispointer = StrReplace(tempbuf, _T("*"), _T(""), SCS_SENSITIVE, UINT_MAX, LINE_SIZE);
		
		// Array
		if (_tcschr(tempbuf,'['))
		{
			_tcsncpy(intbuf,_tcschr(tempbuf,'['),MAX_INTEGER_LENGTH);
			intbuf[_tcscspn(intbuf,_T("]")) + 1] = '\0';
			arraydef = (int)ATOI64(intbuf + 1);
			// remove array definition from temp buffer to identify key easier
			StrReplace(tempbuf, intbuf, _T(""), SCS_SENSITIVE, UINT_MAX, LINE_SIZE);
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
				return NULL;
			}
			_tcsncpy(defbuf + 1,tempbuf,buf_size);
			//_tcscpy(defbuf + 1 + _tcscspn(tempbuf,_T("\t ")),_T(" "));
			defbuf[1 + buf_size] = ' ';
			defbuf[2 + buf_size] = '\0';
			if (StrChrAny(tempbuf, _T(" \t")))
			{
				if (_tcslen(StrChrAny(tempbuf, _T(" \t"))) > MAX_VAR_NAME_LENGTH + 30)
				{
					obj->Release();
					return NULL;
				}
				_tcscpy(keybuf,StrChrAny(tempbuf, _T(" \t")) + 1);
				ltrim(keybuf);
			}
			else 
				keybuf[0] = '\0';
		}
		else // Not 'TypeOnly' definition because there are more than one fields in array so use default type UInt
		{
			_tcscpy(defbuf,_T(" UInt "));
			_tcscpy(keybuf,tempbuf);
		}
		// Now find size in default types array and create new field
		// If Type not found, resolve type to variable and get size of struct defined in it
		if ((thissize = IsDefaultType(defbuf)))
		{
			if (!(field = obj->Insert(keybuf, insert_pos++,ispointer,offset,arraydef,NULL,ispointer ? ptrsize : thissize
						,ispointer ? true : !tcscasestr(_T(" FLOAT DOUBLE PFLOAT PDOUBLE "),defbuf)
						,!tcscasestr(_T(" PTR SHORT INT INT64 CHAR VOID HALF_PTR BOOL INT32 LONG LONG32 LONGLONG LONG64 USN INT_PTR LONG_PTR POINTER_64 POINTER_SIGNED SSIZE_T WPARAM __int64 "),defbuf)
						,tcscasestr(_T(" TCHAR LPTSTR LPCTSTR LPWSTR LPCWSTR WCHAR "),defbuf) ? 1200 : tcscasestr(_T(" CHAR LPSTR LPCSTR LPSTR UCHAR "),defbuf) ? 0 : -1)))
			{	// Out of memory.
				obj->Release();
				return NULL;
			}
			offset += (ispointer ? ptrsize : thissize) * (arraydef ? arraydef : 1);
#ifdef _WIN64
			// align offset for pointer
			if (ispointer)
			{
				if (offset % ptrsize)
					offset += (ptrsize - (offset % ptrsize)) * (arraydef ? arraydef : 1);
				if (ptrsize > aligntotal)
					aligntotal = ptrsize;
			}
			else
			{
				if (offset % thissize)
					offset += thissize - (offset % thissize);
				if (thissize > aligntotal)
					aligntotal = thissize;
			}
#endif
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
					obj->Release();
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
				if (!ispointer && !_tcsncmp(tempbuf,buf,_tcslen(buf)) && !*keybuf)
				{   // Whole definition is not a pointer and no key was given so create Structure from variable
					obj->Release();
					if (aParamCount == 1)
					{
						if (TokenToObject(*param[0]))
						{   // assume variable is a structure object
							obj = ((Struct *)TokenToObject(*param[0]))->Clone();
							obj->mStructMem = (UINT_PTR *)malloc(obj->mSize);
							obj->mMemAllocated = obj->mSize;
							memset(obj->mStructMem,NULL,obj->mSize);
							return obj;
						}
						// else create structure from string definition
						return Struct::Create(param,1);
					}
					else if (aParamCount > 1)
					{   // more than one parameter was given, copy aParam to param
						param[1]->symbol = aParam[1]->symbol;
						param[1]->object = aParam[1]->object;
						param[1]->value_int64 = aParam[1]->value_int64;
						param[1]->var = aParam[1]->var;
					}
					if (aParamCount > 2)
					{   // more than 2 parameters were given, copy aParam to param
						param[2]->symbol = aParam[2]->symbol;
						param[2]->object = aParam[2]->object;
						param[2]->var = aParam[2]->var;
						// definition variable is a structure object, clone it, assign memory and init object
						if (TokenToObject(*param[0]))
						{
							obj = ((Struct *)TokenToObject(*param[0]))->Clone();
							obj->mMemAllocated = 0;
							obj->mStructMem = (UINT_PTR*)aParam[1]->value_int64;
							obj->ObjectToStruct(TokenToObject(*aParam[2]));
							return obj;
						}
						return Struct::Create(param,3);
					}
					else if (TokenToObject(*param[0]))
					{   // definition variable is a structure object, clone it and assign memory or init object
						obj = ((Struct *)TokenToObject(*param[0]))->Clone();
						if (TokenToObject(*aParam[1]))
						{
							obj->mStructMem = (UINT_PTR *)malloc(obj->mSize);
							obj->mMemAllocated = obj->mSize;
							memset(obj->mStructMem,NULL,obj->mSize);
							obj->ObjectToStruct(TokenToObject(*aParam[1]));
						}
						else
							obj->mStructMem = (UINT_PTR*)aParam[1]->value_int64;
						return obj;
					}
					// else simply create structure from variable and given memory/initobject
					return Struct::Create(param,2);
				}
				// Call BIF_sizeof passing offset in second parameter to align offset if necessary
				// if field is a pointer we will need its size only
				param[1]->value_int64 = (__int64)ispointer ? 0 : offset;
				BIF_sizeof(Result,ResultToken,param,ispointer ? 1 : 2);
				if (ResultToken.symbol != SYM_INTEGER)
				{	// could not resolve structure
					obj->Release();
					return NULL;
				}
				// Insert new field in our structure
				if (!(field = obj->Insert(keybuf, insert_pos++,ispointer,offset,arraydef,Var1.var,ispointer ? 4 : (int)ResultToken.value_int64,1,1,-1)))
				{	// Out of memory.
					obj->Release();
					return NULL;
				}
				if (ispointer)
					offset += (int)(ispointer ? ptrsize : ResultToken.value_int64) * (arraydef ? arraydef : 1);
				else
				// sizeof was given an offset that it applied and aligned if necessary, so set offset =  and not +=
					offset = (int)(ispointer ? ptrsize : ResultToken.value_int64) * (arraydef ? arraydef : 1);
#ifdef _WIN64
				// align offset for pointer
				if (ispointer)
				{
					if (offset % ptrsize)
						offset += ptrsize - (offset % ptrsize);
				}
#endif
			}
			else // No variable was found and it is not default type so we can't determine size.
			{
				obj->Release();
				return  NULL;
			}
		}
		// update union size
		if (uniondepth)
		{
			if ((offset - unionoffset[uniondepth]) > unionsize[uniondepth])
				unionsize[uniondepth] = offset - unionoffset[uniondepth];
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
		{
			// identify that structure object has no fields
			if (!*keybuf)
				obj->mTypeOnly = true;
			buf += _tcslen(buf);
		}
	}
#ifdef _WIN64
	// align total structure if necessary
	if (aligntotal)
		offset += offset % aligntotal;
#endif
	if (!offset) // structure could not be build
	{
		obj->Release();
		return NULL;
	}
	
	obj->mSize = offset;
	if (aParamCount > 1 && TokenIsNumeric(*aParam[1]))
	{	// second parameter exist and it is digit assumme this is new pointer for our structure
		obj->mStructMem = (UINT_PTR *)TokenToInt64(*aParam[1]);
		obj->mMemAllocated = 0;
	}
	else // no pointer given so allocate memory and fill memory with 0
	{	// setting the memory after parsing definition saves a call to BIF_sizeof
		obj->mStructMem = (UINT_PTR *)malloc(offset);
		obj->mMemAllocated = offset;
		memset(obj->mStructMem,NULL,offset);
	}

	// an object was passed to initialize fields
	// enumerate trough object and assign values
	if ((aParamCount > 1 && !TokenIsNumeric(*aParam[1])) || aParamCount > 2 )
		obj->ObjectToStruct(TokenToObject(aParamCount == 2 ? *aParam[1] : *aParam[2]));
	return obj;
}

//
// Struct::ObjectToStruct - Initialize structure from array, object or structure.
//

void Struct::ObjectToStruct(IObject *objfrom)
{
	ExprTokenType ResultToken, this_token,enum_token,param_tokens[3];
	ExprTokenType *params[] = { param_tokens, param_tokens+1, param_tokens+2 };
	TCHAR defbuf[MAX_PATH],buf[MAX_PATH];
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
		enumerator.Invoke(result_token,enum_token,IT_CALL, params, param_count);

		bool next_returned_true = TokenToBOOL(result_token);
		if (!next_returned_true)
			break;

		this->Invoke(ResultToken,this_token,IT_SET,params+1,2);
		
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
	if (mMemAllocated > 0)
		free(mStructMem);
	if (mFields)
	{
		if (mFieldCount)
		{
			IndexType i = mFieldCount - 1;
			// Free keys
			for ( ; i >= 0 ; --i)
			{
				if (mFields[i].mMemAllocated > 0)
					free(mFields[i].mStructMem);
				free(mFields[i].key);
			}
		}
		// Free fields array.
		free(mFields);
	}
}


//
// Struct::SetPointer - used to set pointer for a field or array item
//

UINT_PTR Struct::SetPointer(UINT_PTR aPointer,int aArrayItem)
{
	*((UINT_PTR*)((UINT_PTR)mStructMem + (aArrayItem-1)*(mSize/(mArraySize ? mArraySize : 1)))) = aPointer;
	return aPointer;
}


//
// Struct::FieldType::Clone - used to clone a field to structure.
//

Struct *Struct::CloneField(FieldType *field,bool aIsDynamic)
// Creates an object and copies to it the fields at and after the given offset.
{
	Struct *objptr = new Struct();
	if (!objptr)
		return objptr;
	
	Struct &obj = *objptr;
	// if field is an array, set correct size
	if (obj.mArraySize = field->mArraySize)
		obj.mSize = field->mSize*obj.mArraySize;
	else
		obj.mSize = field->mSize;
	obj.mIsInteger = field->mIsInteger;
	obj.mIsPointer = field->mIsPointer;
	obj.mEncoding = field->mEncoding;
	obj.mIsUnsigned = field->mIsUnsigned;
	obj.mVarRef = field->mVarRef;
	obj.mTypeOnly = 1;
	obj.mMemAllocated = aIsDynamic ? -1 : 0;
	return objptr;
}

//
// Struct::Clone - used for cloning structures.
//

Struct *Struct::Clone(bool aIsDynamic)
// Creates an object and copies to it the fields at and after the given offset.
{
	Struct *objptr = new Struct();
	if (!objptr)
		return objptr;
	
	
	Struct &obj = *objptr;
	obj.mArraySize = mArraySize;
	obj.mIsInteger = mIsInteger;
	obj.mIsPointer = mIsPointer;
	obj.mEncoding = mEncoding;
	obj.mIsUnsigned = mIsUnsigned;
	obj.mSize = mSize;
	obj.mVarRef = mVarRef;
	obj.mTypeOnly = mTypeOnly;
	// -1 will identify a dynamic structure, no memory can be allocated to such
	obj.mMemAllocated = aIsDynamic ? -1 : 0;
	
	// Allocate space in destination object.
	if (!obj.SetInternalCapacity(mFieldCount))
	{
		obj.Release();
		return NULL;
	}

	FieldType *fields = obj.mFields; // Newly allocated by above.
	int failure_count = 0; // See comment below.
	IndexType i;

	obj.mFieldCount = mFieldCount;
	
	for (i = 0; i < mFieldCount; ++i)
	{
		FieldType &dst = fields[i];
		FieldType &src = mFields[i];

		if ( !(dst.key = _tcsdup(src.key)) )
		{
			// Key allocation failed.
			// Rather than trying to set up the object so that what we have
			// so far is valid in order to break out of the loop, continue,
			// make all fields valid and then allow them to be freed. 
			++failure_count;
		}
		dst.mArraySize = src.mArraySize;
		dst.mIsInteger = src.mIsInteger;
		dst.mIsPointer = src.mIsPointer;
		dst.mEncoding = src.mEncoding;
		dst.mIsUnsigned = src.mIsUnsigned;
		dst.mOffset = src.mOffset;
		dst.mSize = src.mSize;
		dst.mVarRef = src.mVarRef;
		dst.mMemAllocated = aIsDynamic ? -1 : 0;

	}
	if (failure_count)
	{
		// One or more memory allocations failed.  It seems best to return a clear failure
		// indication rather than an incomplete copy.  Now that the loop above has finished,
		// the object's contents are at least valid and it is safe to free the object:
		obj.Release();
		return NULL;
	}
	return &obj;
}


//
// Struct::Invoke - Called by BIF_ObjInvoke when script explicitly interacts with an object.
//

ResultType STDMETHODCALLTYPE Struct::Invoke(
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
	ExprTokenType *param[] = {&Var1,&Var2},ResultToken;
	
	// used to clone a dynamic field or structure
	Struct *objclone = NULL;

	// used for StrGet/StrPut
	LPCVOID source_string;
	int source_length;
	DWORD flags = WC_NO_BEST_FIT_CHARS;
	int length = -1;
	int char_count;

	// Identify that we need to release/delete field or structure object
	bool deletefield = false;
	bool releaseobj = false;

	int param_count_excluding_rvalue = aParamCount;

	// target may be altered here to resolve dynamic structure so hold it separately
	UINT_PTR *target = mStructMem;

	if (IS_INVOKE_SET)
	{
		// Prior validation of ObjSet() param count ensures the result won't be negative:
		--param_count_excluding_rvalue;
		
		// Since defining base[key] prevents base.base.__Get and __Call from being invoked, it seems best
		// to have it also block __Set. The code below is disabled to achieve this, with a slight cost to
		// performance when assigning to a new key in any object which has a base object. (The cost may
		// depend on how many key-value pairs each base object has.) Note that this doesn't affect meta-
		// functions defined in *this* base object, since they were already invoked if present.
		//if (IS_INVOKE_META)
		//{
		//	if (param_count_excluding_rvalue == 1)
		//		// Prevent below from unnecessarily searching for a field, since it won't actually be assigned to.
		//		// Relies on mBase->Invoke recursion using aParamCount and not param_count_excluding_rvalue.
		//		param_count_excluding_rvalue = 0;
		//	//else: Allow SET to operate on a field of an object stored in the target's base.
		//	//		For instance, x[y,z]:=w may operate on x[y][z], x.base[y][z], x[y].base[z], etc.
		//}
	}

	if (!param_count_excluding_rvalue || (param_count_excluding_rvalue == 1 && TokenIsEmptyString(*aParam[0])))
	{   // struct[] or struct[""] / obj[] := ptr 
		if (IS_INVOKE_SET)
		{
			if (TokenToObject(*aParam[param_count_excluding_rvalue]))
			{   // Initialize structure using an object. e.g. struct[]:={x:1,y:2}
				this->ObjectToStruct(TokenToObject(*aParam[param_count_excluding_rvalue]));
				
				// return struct object
				aResultToken.symbol = SYM_OBJECT;
				aResultToken.object = this;
				this->AddRef();
				return OK;

			}
			if (mMemAllocated > 0) // free allocated memory because we will assign a custom pointer
			{
				free(mStructMem);
				mMemAllocated = 0;
			}
			// assign new pointer to structure
			// releasing/deleting structure will not free that memory
			mStructMem = (UINT_PTR *)TokenToInt64(*aParam[0]);
		}
		// Return new structure address
		aResultToken.symbol = SYM_INTEGER;
		aResultToken.value_int64 = (__int64)mStructMem;
		return OK;
	}
	else
	{
		// Array access, struct.1 or struct[1] or struct[1].x ...
		if (TokenIsNumeric(*aParam[0]))
		{
			if (param_count_excluding_rvalue > 1 && TokenIsEmptyString(*aParam[1])) 
			{	// caller wants set/get pointer. E.g. struct.2[""] or struct.2[""] := ptr
				if (IS_INVOKE_SET)
				{
					if (param_count_excluding_rvalue < 3) // simply set pointer
						aResultToken.value_int64 = SetPointer((UINT_PTR)TokenToInt64(*aParam[2]),(int)TokenToInt64(*aParam[0]));
					else
					{	// resolve pointer to pointer and set it
						UINT_PTR *aDeepPointer = ((UINT_PTR*)((UINT_PTR)target + (TokenToInt64(*aParam[0])-1)*(mSize/(mArraySize ? mArraySize : 1))));
						for (int i = param_count_excluding_rvalue - 2;i && aDeepPointer;i--)
							aDeepPointer = (UINT_PTR*)*aDeepPointer;
						*aDeepPointer = (UINT_PTR)TokenToInt64(*aParam[aParamCount]);
						aResultToken.value_int64 = *aDeepPointer;
					}
				}
				else // GET pointer
				{
					if (param_count_excluding_rvalue < 3)
						aResultToken.value_int64 = ((UINT_PTR)target + (TokenToInt64(*aParam[0])-1)*(mSize / (mArraySize ? mArraySize : 1)));
					else
					{	// resolve pointer to pointer
						UINT_PTR *aDeepPointer = ((UINT_PTR*)((UINT_PTR)target + (TokenToInt64(*aParam[0])-1)*(mSize/(mArraySize ? mArraySize : 1))));
						for (int i = param_count_excluding_rvalue - 2;i && *aDeepPointer;i--)
							aDeepPointer = (UINT_PTR*)*aDeepPointer;
						aResultToken.value_int64 = (__int64)aDeepPointer;
					}
				}
				aResultToken.symbol = SYM_INTEGER;
				return OK;
			}
			// Structure is a reference to variable and not a pointer, get size of structure
			if (mVarRef && !mIsPointer)
			{
				Var2.symbol = SYM_VAR;
				Var2.var = mVarRef;
				// Variable is a structure object, copy size
				if (TokenToObject(Var2))
					ResultToken.value_int64 = ((Struct *)TokenToObject(Var2))->mSize;
				else
				{	// use sizeof to find out the size of structure
					param[0]->symbol = SYM_STRING;
					Var1.marker = TokenToString(*param[1]);
					BIF_sizeof(Result,ResultToken,param,1);
				}
			}
			// Check if we have an array, if structure is not array and not pointer, assume array
			if (mArraySize || !mIsPointer) // if not Array and not pointer assume it is an array
				target = (UINT_PTR*)((UINT_PTR)target + (TokenToInt64(*aParam[0])-1)*(mIsPointer ? ptrsize : (mVarRef ? ResultToken.value_int64 : mSize/(mArraySize ? mArraySize : 1))));
			else if (mIsPointer) // resolve pointer
				target = (UINT_PTR*)(*target + (TokenToInt64(*aParam[0])-1)*ptrsize);
			else // amend target to memory of field
				target = (UINT_PTR*)((UINT_PTR)target + (TokenToInt64(*aParam[0])-1)*(mVarRef ? ResultToken.value_int64 : mSize));
			
			// Structure has a variable reference and might be a pointer but not pointer to pointer
			if (mVarRef && mIsPointer < 2)
			{
				Var2.symbol = SYM_VAR;
				Var2.var = mVarRef;
				// variable is a structure object, clone it
				if (TokenToObject(Var2))
				{
					objclone = ((Struct *)TokenToObject(Var2))->Clone(true);
					objclone->mStructMem = target;
					if (mArraySize)
					{
						objclone->mArraySize = 0;
						objclone->mSize = mSize / mArraySize;
					}
					// Object to Structure
					if (IS_INVOKE_SET && TokenToObject(*aParam[1]))
					{
						objclone->ObjectToStruct(TokenToObject(*aParam[1]));
						aResultToken.symbol = SYM_OBJECT;
						aResultToken.object = objclone;
						return OK;
					}
					// MULTIPARAM
					if (param_count_excluding_rvalue > 1)
					{
						objclone->Invoke(aResultToken,ResultToken,aFlags,aParam + 1,aParamCount - 1);
						objclone->Release();
						return OK;
					}
					aResultToken.object = objclone;
					aResultToken.symbol = SYM_OBJECT;
					return OK;
				}
				else
				{
					Var1.symbol = SYM_STRING;
					Var1.marker = TokenToString(Var2);
					Var2.symbol = SYM_INTEGER;
					Var2.value_int64 = (UINT_PTR)target; 
					if (objclone = Struct::Create(param,2))
					{
						Struct *tempobj = objclone;
						objclone = objclone->Clone(true);
						objclone->mStructMem = tempobj->mStructMem;
						if (mArraySize)
						{
							objclone->mArraySize = 0;
							objclone->mSize = mSize / mArraySize;
						}
						tempobj->Release();
						if (IS_INVOKE_SET && TokenToObject(*aParam[1]))
						{
							objclone->ObjectToStruct(TokenToObject(*aParam[1]));
							aResultToken.symbol = SYM_OBJECT;
							aResultToken.object = objclone;
							return OK;
						}
						if (param_count_excluding_rvalue > 1)
						{
							objclone->Invoke(aResultToken,ResultToken,aFlags,aParam + 1,aParamCount - 1);
							objclone->Release();
							return OK;
						}
						aResultToken.object = objclone;
						aResultToken.symbol = SYM_OBJECT;
						return OK;
					}
					else
						return INVOKE_NOT_HANDLED;
				}
			}
			else
			{
				objclone = Clone(true);
				releaseobj = true;
				objclone->mStructMem = target;
				if (!mArraySize && mIsPointer)
					objclone->mIsPointer--;
				else if (mArraySize)
				{
					objclone->mArraySize = 0;
					objclone->mSize = mSize / mArraySize;
				}
				if (objclone->mIsPointer || (aParamCount == 1 && !mTypeOnly))
				{
					if (param_count_excluding_rvalue > 1)
					{	// MULTIPARAM
						objclone->Invoke(aResultToken,ResultToken,aFlags,aParam + 1,aParamCount - 1);
						objclone->Release();
						return OK;
					}
					aResultToken.symbol = SYM_OBJECT;
					aResultToken.object = objclone;
					return OK;
				}
				else if (!mTypeOnly)
				{	// the given integer is now excluded from parameters
					aParamCount--;param_count_excluding_rvalue--;aParam++;
				}
			}
		}
		if (mTypeOnly && !IS_INVOKE_CALL) // IS_INVOKE_CALL does not need the tentative field, it will handle it itself
		{
			if (mVarRef)
			{
				if (releaseobj)
					objclone->Release();
				Var2.symbol = SYM_VAR;
				Var2.var = mVarRef;
				if (TokenToObject(Var2) && (objclone = ((Struct *)TokenToObject(Var2))->Clone(true)))
				{	// variable is a structure object
					objclone->mStructMem = target;
					objclone->Invoke(aResultToken,ResultToken ,aFlags,aParam,aParamCount);
					objclone->Release();
					return OK;
				}
				else
				{
					Var1.symbol = SYM_STRING;
					Var1.marker = TokenToString(Var2);
					Var2.symbol = SYM_INTEGER;

					// resolve pointer
					Var2.value_int64 = mIsPointer ? *target : (UINT_PTR)target; 

					if (objclone = Struct::Create(param,2))
					{	// create structure from variable
						Struct *tempobj = objclone->Clone(true);
						tempobj->mStructMem = objclone->mStructMem;
						tempobj->Invoke(aResultToken,aThisToken ,aFlags,aParam,aParamCount);
						tempobj->Release();
						objclone->Release();
						return OK;
					}
					return INVOKE_NOT_HANDLED;
				}
			}
			else // create field from structure
			{
				field = new FieldType();
				deletefield = true;
				if (objclone == NULL)
				{	// use this structure
					field->mMemAllocated = mMemAllocated;
					field->mIsInteger = mIsInteger;
					field->mIsPointer = mIsPointer;
					field->mEncoding = mEncoding;
					field->mIsUnsigned = mIsUnsigned;
					field->mOffset = 0;

					// structure with arrays so set to correct field size
					field->mSize = mSize / (mArraySize ? mArraySize : 1);
					field->mVarRef = 0;
				}
				else // use objclone created above
				{
					field->mMemAllocated = objclone->mMemAllocated;
					field->mIsInteger = objclone->mIsInteger;
					field->mIsPointer = objclone->mIsPointer;
					field->mEncoding = objclone->mEncoding;
					field->mIsUnsigned = objclone->mIsUnsigned;
					field->mOffset = 0;
					field->mSize = objclone->mSize / (objclone->mArraySize ? objclone->mArraySize : 1);
				}
			}
		}
		else if (!IS_INVOKE_CALL) // IS_INVOKE_CALL will handle the field itself
			field = FindField(TokenToString(*aParam[0]));
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
		{
			if (deletefield) // we created the field from a structure
				delete field;
			if (releaseobj)
				objclone->Release();
			return _NewEnum(aResultToken, aParam, aParamCount);
		}
		// if first function parameter is a field get it
		if (!mTypeOnly && aParamCount && !TokenIsNumeric(*aParam[0]))
		{
			if (field = FindField(TokenToString(*aParam[0])))
			{	// exclude parameter in aParam
				++aParam; --aParamCount;
			}
		}
		aResultToken.symbol = SYM_INTEGER;  // mostly used
		aResultToken.value_int64 = 0; // set default
		if (!_tcsicmp(name, _T("SetCapacity")))
		{	// Set strcuture its capacity or fields capacity
			if (!field)
			{
				if (!aParamCount || !TokenIsNumeric(*aParam[0]) || !TokenToInt64(*aParam[0]) || TokenToInt64(*aParam[0]) == 0)
				{	// 0 or no parameters were given to free memory
					if (mMemAllocated > 0)
					{
						mMemAllocated = 0;
						free(mStructMem);
					}
					if (deletefield) // we created the field from a structure
						delete field;
					if (releaseobj)
						objclone->Release();
					return OK;
				}
				if (mMemAllocated > 0)
					free(mStructMem);
				// allocate memory and zero-fill
				if (mStructMem = (UINT_PTR*)malloc((size_t)TokenToInt64(*aParam[0])))
				{
					mMemAllocated = (int)TokenToInt64(*aParam[0]);
					memset(mStructMem,NULL,(size_t)mMemAllocated);
					aResultToken.value_int64 = mMemAllocated;
				}
				else
					mMemAllocated = 0;
			} 
			else if (aParamCount)
			{   // we must have to parmeters here since first parameter is field
				if (!TokenIsNumeric(*aParam[0]) || !TokenToInt64(*aParam[0]) || TokenToInt64(*aParam[0]) == 0)
				{
					if (field->mMemAllocated > 0)
					{
						field->mMemAllocated = 0;
						free(field->mStructMem);
					}
					if (deletefield) // we created the field from a structure
						delete field;
					if (releaseobj)
						objclone->Release();
					return OK; // not numeric
				}
				if (field->mMemAllocated > 0)
					free(field->mStructMem);
				// allocate memory and zero-fill
				if (field->mStructMem = (UINT_PTR*)malloc((size_t)TokenToInt64(*aParam[0])))
				{
					field->mMemAllocated = (int)TokenToInt64(*aParam[0]);
					memset(field->mStructMem,NULL,(size_t)field->mMemAllocated);
					*((UINT_PTR*)((UINT_PTR)target + field->mOffset)) = (UINT_PTR)field->mStructMem;
					aResultToken.value_int64 = mMemAllocated;
				}
				else
					field->mMemAllocated = 0;
			}
			if (deletefield) // we created the field from a structure
				delete field;
			if (releaseobj)
				objclone->Release();
			return OK;
		}
		if (!_tcsicmp(name, _T("GetCapacity")))
		{
			if (field)
				aResultToken.value_int64 = field->mMemAllocated;
			else
				aResultToken.value_int64 = mMemAllocated;
			if (deletefield) // we created the field from a structure
				delete field;
			if (releaseobj)
				objclone->Release();
			return OK;
		}
		if (!_tcsicmp(name, _T("Offset")))
		{
			if (field)
				aResultToken.value_int64 = field->mOffset;
			else if (aParamCount && TokenIsNumeric(*aParam[0]))
				// calculate size if item is an array
				aResultToken.value_int64 = mSize / (mArraySize ? mArraySize : 1) * (TokenToInt64(*aParam[0])-1);
			if (deletefield) // we created the field from a structure
				delete field;
			if (releaseobj)
				objclone->Release();
			return OK;
		}
		if (!_tcsicmp(name, _T("IsPointer")))
		{
			if (field)
				aResultToken.value_int64 = field->mIsPointer;
			else
				aResultToken.value_int64 = mIsPointer;
			if (deletefield) // we created the field from a structure
				delete field;
			if (releaseobj)
				objclone->Release();
			return OK;
		}
		if (!_tcsicmp(name, _T("Encoding")))
		{
			if (field)
				aResultToken.value_int64 = field->mEncoding == 65535 ? -1 : field->mEncoding;
			else
				aResultToken.value_int64 = mEncoding == 65535 ? -1 : mEncoding;
			if (deletefield) // we created the field from a structure
				delete field;
			if (releaseobj)
				objclone->Release();
			return OK;
		}
		if (!_tcsicmp(name, _T("GetPointer")))
		{
			if (!field && aParamCount && mIsPointer)
			{	// resolve array item
				if (mArraySize && TokenIsNumeric(*aParam[0]))
					aResultToken.value_int64 = *((UINT_PTR*)((UINT_PTR)target + ((mIsPointer ? ptrsize : (mSize/mArraySize)) * (TokenToInt64(*aParam[0])-1))));
				else
					aResultToken.value_int64 = *target;
			}
			else if (field)
				aResultToken.value_int64 = *((UINT_PTR*)((UINT_PTR)target + field->mOffset));
			if (deletefield) // we created the field from a structure
				delete field;
			if (releaseobj)
				objclone->Release();
			return OK;
		}
		if (!_tcsicmp(name, _T("Fill")))
		{
			if (!field) // only allow to fill main structure
			{
				if (aParamCount && TokenIsNumeric(*aParam[0]))
					memset(objclone ? objclone->mStructMem : mStructMem,TokenIsNumeric(*aParam[0]),mSize);
				else if (aParamCount && *TokenToString(*aParam[0]))
					memset(objclone ? objclone->mStructMem : mStructMem,*TokenToString(*aParam[0]),mSize);
				else
					memset(objclone ? objclone->mStructMem : mStructMem,NULL,mSize);
			}
			if (deletefield) // we created the field from a structure
				delete field;
			if (releaseobj)
				objclone->Release();
			return OK;
		}
		if (!_tcsicmp(name, _T("GetAddress")))
		{
			if (!field)
			{
				if (mArraySize && aParamCount && TokenIsNumeric(*aParam[0]))
					aResultToken.value_int64 = (UINT_PTR)target + (mSize / mArraySize * (TokenToInt64(*aParam[0])-1));
				else
					aResultToken.value_int64 = (UINT_PTR)target;
			}
			else
				aResultToken.value_int64 = (UINT_PTR)target + field->mOffset;
			if (deletefield) // we created the field from a structure
				delete field;
			if (releaseobj)
				objclone->Release();
			return OK;
		}
		if (!_tcsicmp(name, _T("Size")))
		{
			if (!field)
			{
				if (mArraySize && aParamCount && TokenIsNumeric(*aParam[0]))
					// we do not care which item was requested because all are same size
					aResultToken.value_int64 = mSize / mArraySize;
				else
					aResultToken.value_int64 = mSize;
			}
			else
				aResultToken.value_int64 = field->mSize;
			if (deletefield) // we created the field from a structure
				delete field;
			if (releaseobj)
				objclone->Release();
			return OK;
		}
		if (!_tcsicmp(name, _T("CountOf")))
		{
			if (!field)
				aResultToken.value_int64 = mArraySize;
			else
				aResultToken.value_int64 = field->mArraySize;
			if (deletefield) // we created the field from a structure
				delete field;
			if (releaseobj)
				objclone->Release();
			return OK;
		}
		if (!_tcsicmp(name, _T("Clone")) || !_tcsicmp(name, _T("_New")))
		{
			if (!field)
			{
				if (!releaseobj) // else we have a clone already
					objclone = this->Clone();
			}
			else
			{
				Struct* tempobj = objclone;
				if (releaseobj) // release object, it is not requred anymore
				{
					objclone = objclone->CloneField(field);
					tempobj->Release();
				}
				else
					objclone = this->CloneField(field);
			}
			if (aParamCount)
			{	// structure pointer and / or init object were given
				if (TokenIsNumeric(*aParam[0]))
				{
					objclone->mStructMem = (UINT_PTR*)TokenToInt64(*aParam[0]);
					objclone->mMemAllocated = 0;
					if (aParamCount > 1 && TokenToObject(*aParam[1]))
						objclone->ObjectToStruct(TokenToObject(*aParam[1]));
				} 
				else if (TokenToObject(*aParam[0]))
				{
					objclone->mStructMem = (UINT_PTR*)malloc(objclone->mSize);
					objclone->mMemAllocated = objclone->mSize;
					memset(objclone->mStructMem,NULL,objclone->mSize);
					objclone->ObjectToStruct(TokenToObject(*aParam[0]));
				}
			}
			else
			{
				objclone->mStructMem = (UINT_PTR*)malloc(objclone->mSize);
				objclone->mMemAllocated = objclone->mSize;
				memset(objclone->mStructMem,NULL,objclone->mSize);
			}
			// small fix for _New to work properly because aThisToken contains the new object
			if (!_tcsicmp(name, _T("_New")))
			{
				if (aThisToken.symbol == SYM_OBJECT)
					aThisToken.object->Release();
				else
					aThisToken.symbol = SYM_OBJECT;
				aThisToken.object = objclone;
				objclone->AddRef();
			}
			aResultToken.symbol = SYM_OBJECT;
			aResultToken.object = objclone;
			if (deletefield) // we created the field from a structure
				delete field;
			// do not release objclone because it is returned
			return OK;
		}
		// For maintainability: explicitly return since above has done ++aParam, --aParamCount.
		if (deletefield) // we created the field from a structure
			delete field;
		if (releaseobj)
			objclone->Release();
		aResultToken.symbol = SYM_STRING;  
		aResultToken.marker = _T(""); // identify that method was not found
		return INVOKE_NOT_HANDLED;
	}
	else if (!field)
	{	// field was not found
		if (releaseobj)
			objclone->Release();
		return INVOKE_NOT_HANDLED;
	}


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
					aResultToken.value_int64 = (UINT_PTR)target + field->mOffset;
				}
				else // set pointer to pointer
				{
					UINT_PTR *aDeepPointer = ((UINT_PTR*)((UINT_PTR)target + field->mOffset));
					for (int i = param_count_excluding_rvalue - 2;i && aDeepPointer;i--)
						aDeepPointer = (UINT_PTR*)*aDeepPointer;
					*aDeepPointer = (UINT_PTR)TokenToInt64(*aParam[aParamCount]);
					aResultToken.value_int64 = *aDeepPointer;
				}
			}
			else // GET pointer
			{
				if (param_count_excluding_rvalue < 3)
					aResultToken.value_int64 = (UINT_PTR)target + field->mOffset;
				else
				{	// get pointer to pointer
					UINT_PTR *aDeepPointer = ((UINT_PTR*)((UINT_PTR)target + field->mOffset));
					for (int i = param_count_excluding_rvalue - 2;i && *aDeepPointer;i--)
						aDeepPointer = (UINT_PTR*)*aDeepPointer;
					aResultToken.value_int64 = (__int64)aDeepPointer;
				}
			}
			if (deletefield) // we created the field from a structure
				delete field;
			if (releaseobj)
				objclone->Release();
			return OK;
		}
		else // clone field to object and invoke again
		{
			if (releaseobj)
				objclone->Release();
			objclone = CloneField(field,true);
			/*
			if (!field->mArraySize && field->mIsPointer)
			{
				objclone->mStructMem = (UINT_PTR*)*((UINT_PTR*)((UINT_PTR)target + field->mOffset));
				//objclone->mIsPointer--;
				if (--objclone->mIsPointer) // it is a pointer to array of pointers, set mArraySize to 1 to identify an array
					objclone->mArraySize = 1;
			}
			else
				objclone->mStructMem = (UINT_PTR*)((UINT_PTR)target + (TokenToInt64(*aParam[1])-1)*(field->mIsPointer ? ptrsize : field->mSize));
			*/
			objclone->mStructMem = (UINT_PTR*)((UINT_PTR)target + field->mOffset);
			objclone->Invoke(aResultToken,ResultToken,aFlags,aParam + 1,aParamCount - 1);
			objclone->Release();
			if (deletefield) // we created the field from a structure
				delete field;
			return OK;
		}
	} // MULTIPARAM[x,y] x[y]
	
	// SET
	else if (IS_INVOKE_SET)
	{
		if (field->mVarRef && TokenToObject(*aParam[1]))
		{ // field is a structure, assign objct to structure
			if (releaseobj)
				objclone->Release();
			objclone = this->CloneField(field,true);
			objclone->mStructMem = (UINT_PTR*)((UINT_PTR)target + field->mOffset);
			objclone->ObjectToStruct(TokenToObject(*aParam[1]));
			aResultToken.symbol = SYM_OBJECT;
			aResultToken.object = objclone;
			return OK;
		}
		if (mIsPointer && objclone == NULL)
		{   // resolve pointer
			for (int i = mIsPointer;i;i--)
				target = (UINT_PTR*)*target;
		}
		else if (objclone && objclone->mIsPointer)
		{	// resolve pointer for objclone
			for (int i = objclone->mIsPointer;i;i--)
				target = (UINT_PTR*)*target;
		}
		if (field->mIsPointer)
		{   // field is a pointer, clone to structure and invoke again
			if (releaseobj)
				objclone->Release();
			objclone = this->CloneField(field,true);
			objclone->mIsPointer--;
			objclone->mStructMem = (UINT_PTR*)*((UINT_PTR*)((UINT_PTR)target + field->mOffset));
			objclone->Invoke(aResultToken,aThisToken,aFlags,aParam,aParamCount);
			objclone->Release();
			return OK;
		}

		// StrPut (code stolen from BIF_StrPut())
		if (field->mEncoding != 65535)
		{	// field is [T|W|U]CHAR or LP[TC]STR, set get character or string
			source_string = (LPCVOID)TokenToString(*aParam[1], aResultToken.buf);
			source_length = (int)((aParam[1]->symbol == SYM_VAR) ? aParam[1]->var->CharLength() : _tcslen((LPCTSTR)source_string));
			if (field->mSize > 2) // not [T|W|U]CHAR
				source_length++; // for terminating character
			if (field->mSize > 2 && (!*((UINT_PTR*)((UINT_PTR)target + field->mOffset)) || (field->mMemAllocated > 0 && (field->mMemAllocated<(source_length*2)))))
			{   // no memory allocated yet, allocate now
				if (field->mMemAllocated == -1 && !*((UINT_PTR*)((UINT_PTR)target + field->mOffset))){
					if (deletefield) // we created the field from a structure so no memory can be allocated
						delete field;
					if (releaseobj)
						objclone->Release();
					return g_script.ScriptError(ERR_MUST_INIT_STRUCT);
				}
				else if (field->mMemAllocated > 0)  // free previously allocated memory
					free(field->mStructMem);
				field->mMemAllocated = source_length * sizeof(TCHAR);
				field->mStructMem = (UINT_PTR*)malloc(field->mMemAllocated);
				*((UINT_PTR*)((UINT_PTR)target + field->mOffset)) = (UINT_PTR)field->mStructMem;
			}
			if (field->mEncoding == 1200)
			{
				if (TokenIsEmptyString(*aParam[1]))
					*((LPWSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset))) = '\0';
				else
					tmemcpy((LPWSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)), (LPTSTR)source_string, field->mSize < 4 ? 1 : source_length);
			}
			else
			{
				if (TokenIsEmptyString(*aParam[1]))
					*((LPSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset))) = '\0';
				else
				{
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
							aResultToken.symbol = SYM_STRING;
							aResultToken.marker = _T("");
							if (deletefield) // we created the field from a structure
								delete field;
							if (releaseobj)
								objclone->Release();
							return OK;
						}
					}
					if (field->mSize > 2) // Not TCHAR or CHAR or WCHAR
						++char_count; // + 1 for null-terminator (source_length causes it to be excluded from char_count).
					// Assume there is sufficient buffer space and hope for the best:
					length = char_count;
					// Convert to target encoding.
					char_count = WideCharToMultiByte(field->mEncoding, flags, (LPCWSTR)source_string, source_length, (LPSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)), char_count, NULL, NULL);
					// Since above did not null-terminate, check for buffer space and null-terminate if there's room.
					// It is tempting to always null-terminate (potentially replacing the last byte of data),
					// but that would exclude this function as a means to copy a string into a fixed-length array.
					if (field->mSize > 2 && char_count && char_count < length) // NOT TCHAR or CHAR or WCHAR
						((LPSTR)((UINT_PTR)target + field->mOffset))[char_count++] = '\0';
				}
			}
			aResultToken.symbol = SYM_INTEGER;
			aResultToken.value_int64 = char_count;
		}
		else // NumPut
		{	 // code stolen from BIF_NumPut
			switch(field->mSize)
			{
			case 4: // Listed first for performance.
				if (field->mIsInteger)
					*((unsigned int *)((UINT_PTR)target + field->mOffset)) = (unsigned int)TokenToInt64(*aParam[1]);
				else // Float (32-bit).
					*((float *)((UINT_PTR)target + field->mOffset)) = (float)TokenToDouble(*aParam[1]);
				break;
			case 8:
				if (field->mIsInteger)
					// v1.0.48: Support unsigned 64-bit integers like DllCall does:
					*((__int64 *)((UINT_PTR)target + field->mOffset)) = (field->mIsUnsigned && !IS_NUMERIC(aParam[1]->symbol)) // Must not be numeric because those are already signed values, so should be written out as signed so that whoever uses them can interpret negatives as large unsigned values.
						? (__int64)ATOU64(TokenToString(*aParam[1])) // For comments, search for ATOU64 in BIF_DllCall().
						: TokenToInt64(*aParam[1]);
				else // Double (64-bit).
					*((double *)((UINT_PTR)target + field->mOffset)) = TokenToDouble(*aParam[1]);
				break;
			case 2:
				*((unsigned short *)((UINT_PTR)target + field->mOffset)) = (unsigned short)TokenToInt64(*aParam[1]);
				break;
			default: // size 1
				*((unsigned char *)((UINT_PTR)target + field->mOffset)) = (unsigned char)TokenToInt64(*aParam[1]);
			}
			//*((int*)target + field->mOffset) = (int)TokenToInt64(*aParam[1]);
			aResultToken.symbol = SYM_INTEGER;
			aResultToken.value_int64 = TokenToInt64(*aParam[1]);
		}
		if (deletefield) // we created the field from a structure
			delete field;
		if (releaseobj)
			objclone->Release();
		return OK;
	}

	// GET
	else if (field)
	{
		if (field->mArraySize || field->mVarRef)
		{	// filed is an array or variable reference, return a structure object.
			if (field->mArraySize || field->mIsPointer)
			{   // field is array or a pointer to variable
				objclone = CloneField(field,true);
				objclone->mStructMem = (UINT_PTR *)((UINT_PTR)target + field->mOffset);
				aResultToken.symbol = SYM_OBJECT;
				aResultToken.object = objclone;
			}
			else // field is refference to a variable and not pointer, create structure
			{
				Var1.symbol = SYM_STRING;
				Var2.symbol = SYM_VAR;
				Var2.var = field->mVarRef;

				if (TokenToObject(Var2))
				{	// Variable is a structure object
					objclone = ((Struct *)TokenToObject(Var2))->Clone(true);
					objclone->mStructMem = target + field->mOffset;
					aResultToken.object = objclone;
					aResultToken.symbol = SYM_OBJECT;
				}
				else // Variable is a string definition
				{
					Var1.marker = TokenToString(Var2);
					Var2.symbol = SYM_INTEGER;
					Var2.value_int64 = field->mIsPointer ? *(UINT_PTR*)((UINT_PTR)target + field->mOffset) : (UINT_PTR)((UINT_PTR)target + field->mOffset);
					if (objclone = Struct::Create(param,2))
					{	// create and clone object because it is created dynamically
						Struct *tempobj = objclone;
						objclone = objclone->Clone(true);
						objclone->mStructMem = tempobj->mStructMem;
						tempobj->Release();
						aResultToken.symbol = SYM_OBJECT;
						aResultToken.object = objclone;
					}
				}
			}
			if (deletefield) // we created the field from a structure
				delete field;
			if (releaseobj)
				objclone->Release();
			return OK;
		}
		if (mIsPointer && objclone == NULL)
		{	// resolve pointer of main structure
			for (int i = mIsPointer;i;i--)
				target = (UINT_PTR*)*target;
		}
		else if (objclone && objclone->mIsPointer)
		{	// resolve pointer for objclone
			for (int i = objclone->mIsPointer;i;i--)
				target = (UINT_PTR*)*target;
		}
		if (field->mIsPointer)
		{	// field is a pointer we need to return an object
			if (releaseobj)
				objclone->Release();
			objclone = this->CloneField(field,true);
			objclone->mIsPointer--;
			objclone->mStructMem = (UINT_PTR*)*((UINT_PTR*)((UINT_PTR)target + field->mOffset));
			aResultToken.symbol = SYM_OBJECT;
			aResultToken.object = objclone;
			return OK;
		}

		// StrGet (code stolen from BIF_StrGet())
		if (field->mEncoding != 65535)
		{
			if (field->mEncoding == 1200) // no conversation required
			{
				if (field->mSize > 2 && !*((UINT_PTR*)((UINT_PTR)target + field->mOffset)))
				{
					if (deletefield) // we created the field from a structure
						delete field;
					if (releaseobj)
						objclone->Release();
					return OK;
				}
				aResultToken.symbol = SYM_STRING;
				if (!TokenSetResult(aResultToken, (LPCWSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)),field->mSize < 4 ? 1 : -1))
					aResultToken.marker = _T("");
			}
			else
			{
				int conv_length;
				if (field->mSize < 4) // TCHAR or CHAR or WCHAR
					length = 1;
				// Convert multi-byte encoded string to UTF-16.
				conv_length = MultiByteToWideChar(field->mEncoding, 0, (LPCSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)), length, NULL, 0);
				if (!TokenSetResult(aResultToken, NULL, conv_length)) // DO NOT SUBTRACT 1, conv_length might not include a null-terminator.
				{
					if (deletefield) // we created the field from a structure
						delete field;
					if (releaseobj)
						objclone->Release();
					return OK; // Out of memory.
				}
				conv_length = MultiByteToWideChar(field->mEncoding, 0, (LPCSTR)(field->mSize > 2 ? *((UINT_PTR*)((UINT_PTR)target + field->mOffset)) : ((UINT_PTR)target + field->mOffset)), length, aResultToken.marker, conv_length);

				if (conv_length && !aResultToken.marker[conv_length - 1])
					--conv_length; // Exclude null-terminator.
				else
					aResultToken.marker[conv_length] = '\0';
				aResultToken.marker_length = conv_length; // Update this in case TokenSeResult used mem_to_free.
			}
			aResultToken.symbol = SYM_STRING;
		}
		else // NumGet (code stolen from BIF_NumGet())
		{
			switch(field->mSize)
			{
			case 4: // Listed first for performance.
				if (!field->mIsInteger)
					aResultToken.value_double = *((float *)((UINT_PTR)target + field->mOffset));
				else if (!field->mIsUnsigned)
					aResultToken.value_int64 = *((int *)((UINT_PTR)target + field->mOffset)); // aResultToken.symbol was set to SYM_FLOAT or SYM_INTEGER higher above.
				else
					aResultToken.value_int64 = *((unsigned int *)((UINT_PTR)target + field->mOffset));
				break;
			case 8:
				// The below correctly copies both DOUBLE and INT64 into the union.
				// Unsigned 64-bit integers aren't supported because variables/expressions can't support them.
				aResultToken.value_int64 = *((__int64 *)((UINT_PTR)target + field->mOffset));
				break;
			case 2:
				if (!field->mIsUnsigned) // Don't use ternary because that messes up type-casting.
					aResultToken.value_int64 = *((short *)((UINT_PTR)target + field->mOffset));
				else
					aResultToken.value_int64 = *((unsigned short *)((UINT_PTR)target + field->mOffset));
				break;
			default: // size 1
				if (!field->mIsUnsigned) // Don't use ternary because that messes up type-casting.
					aResultToken.value_int64 = *((char *)((UINT_PTR)target + field->mOffset));
				else
					aResultToken.value_int64 = *((unsigned char *)((UINT_PTR)target + field->mOffset));
			}
			aResultToken.symbol = SYM_INTEGER;
		}
		if (deletefield) // we created the field from a structure
			delete field;
		if (releaseobj)
			objclone->Release();
		return OK;
	}
	if (releaseobj)
		objclone->Release();
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
	FieldType *new_fields = (FieldType *)realloc(mFields, new_capacity * sizeof(FieldType));
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
	if (++mOffset < mObject->mFieldCount)
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
			mObject->Invoke(aResultToken,aThisToken,0,&aVarToken,1);
			switch (aResultToken.symbol)
			{
				case SYM_STRING:	aVal->AssignString(aResultToken.marker);	break;
				case SYM_INTEGER:	aVal->Assign(aResultToken.value_int64);			break;
				case SYM_FLOAT:		aVal->Assign(aResultToken.value_double);		break;
				case SYM_OBJECT:	aVal->Assign(aResultToken.object);			break;
			}
			delete aVarToken;
		}
		return true;
	}
	else if (mOffset < mObject->mArraySize)
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
			mObject->Invoke(aResultToken,aThisToken,0,&aVarToken,1);
			switch (aResultToken.symbol)
			{
				case SYM_STRING:	aVal->AssignString(aResultToken.marker);	break;
				case SYM_INTEGER:	aVal->Assign(aResultToken.value_int64);			break;
				case SYM_FLOAT:		aVal->Assign(aResultToken.value_double);		break;
				case SYM_OBJECT:	aVal->Assign(aResultToken.object);			break;
			}
			delete aVarToken;
		}
		return true;
	}
	return false;
}


Struct::FieldType *Struct::Insert(LPTSTR key, IndexType at,UCHAR aIspointer,int aOffset,int aArrsize,Var *variableref,int aFieldsize,bool aIsinteger,bool aIsunsigned,USHORT aEncoding)
// Inserts a single field with the given key at the given offset.
// Caller must ensure 'at' is the correct offset for this key.
{
	if (!*key)
	{
		// empty key = only type was given so assign all to structure object
		// do not assign size here since it will be assigned in StructCreate later
		mArraySize = aArrsize;
		mIsPointer = aIspointer;
		mIsInteger = aIsinteger;
		mIsUnsigned = aIsunsigned;
		mEncoding = aEncoding;
		mVarRef = variableref;
		return (FieldType*)true;
	}
	if (mFieldCount == mFieldCountMax && !Expand()  // Attempt to expand if at capacity.
		|| !(key = _tcsdup(key)))  // Attempt to duplicate key-string.
	{	// Out of memory.
		return NULL;
	}
	// There is now definitely room in mFields for a new field.

	FieldType &field = mFields[at];
	if (at < mFieldCount)
		// Move existing fields to make room.
		memmove(&field + 1, &field, (mFieldCount - at) * sizeof(FieldType));
	++mFieldCount; // Only after memmove above.
	
	// Update key-type offsets based on where and what was inserted; also update this key's ref count:
	
	field.mSize = aFieldsize; // Init to ensure safe behaviour in Assign().
	field.key = key; // Above has already copied string
	field.mArraySize = aArrsize;
	field.mIsPointer = aIspointer;
	field.mOffset = aOffset;
	field.mIsInteger = aIsinteger;
	field.mIsUnsigned = aIsunsigned;
	field.mEncoding = aEncoding;
	field.mVarRef = variableref;
	field.mMemAllocated = 0;
	return &field;
}
#ifdef CONFIG_DEBUGGER

void Struct::DebugWriteProperty(IDebugProperties *aDebugger, int aPage, int aPageSize, int aDepth)
{
	DebugCookie cookie;
	aDebugger->BeginProperty(NULL, "object", (int)mFieldCount, cookie);

	if (aDepth)
	{
		int i = aPageSize * aPage, j = aPageSize * (aPage + 1);

		if (j > (int)mFieldCount)
			j = (int)mFieldCount;
		// For each field in the requested page...
		for ( ; i < j; ++i)
		{
			Struct::FieldType &field = mFields[i];
			
			ExprTokenType value;
			TCHAR buf[MAX_PATH];
			value.buf = buf;
			ExprTokenType aThisToken;
			aThisToken.symbol = SYM_OBJECT;
			aThisToken.object = this;
			ExprTokenType *aVarToken = new ExprTokenType();
			aVarToken->symbol = SYM_STRING;
			aVarToken->marker = field.key;
			this->Invoke(value,aThisToken,0,&aVarToken,1);
			delete aVarToken;

			if (field.mEncoding != 65535) // String
				aDebugger->WriteProperty(CStringUTF8FromTChar(field.key), value);
		}
	}

	aDebugger->EndProperty(cookie);
}
#endif
