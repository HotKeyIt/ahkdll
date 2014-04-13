#include "stdafx.h" // pre-compiled headers
#include "globaldata.h" // for access to many global vars
#include "application.h" // for MsgSleep()
#include "exports.h"
#include "script.h"

BIF_DECL(BIF_Getvar)
{
	if (aParam[0]->symbol == SYM_STRING && !(TokenToInt64(*aParam[0])))
		aResultToken.value_int64 = (__int64)g_script.FindOrAddVar(aParam[0]->marker);
	else if (aParam[0]->symbol == SYM_VAR)
		aResultToken.value_int64 = (__int64)aParam[0]->var;
	else
		aResultToken.value_int64 = 0;
}

BIF_DECL(BIF_Alias)
{
	ExprTokenType &aParam0 = *aParam[0];
	ExprTokenType &aParam1 = *aParam[1];
	aResultToken.symbol = SYM_STRING;
	aResultToken.marker = _T("");
	if (aParam0.symbol == SYM_VAR)
	{
		Var &var = *aParam0.var;

		UINT_PTR len = 0;
		if (aParamCount == 2)
			switch (aParam1.symbol)
			{
			case SYM_VAR:
				len = (UINT_PTR)(aParam[1]->var->mType == VAR_ALIAS ? aParam1.var->ResolveAlias() : aParam1.var);
				break;
			case SYM_INTEGER:
				// HotKeyIt added to accept var pointer
				len = (UINT_PTR)aParam[1]->value_int64;
				break;
				// HotKeyIt H10 added to accept dynamic text and also when value is returned by ahkgetvar in AutoHotkey.dll
			case SYM_STRING:
				len = (UINT_PTR)ATOI64(aParam1.marker);
			}
		var.mType = len ? VAR_ALIAS : VAR_NORMAL;
		var.mByteLength = len;
	}
}
BIF_DECL(BIF_CacheEnable)
{
	if (aParam[0]->symbol == SYM_VAR)
	{
		(aParam[0]->var->mType == VAR_ALIAS ? aParam[0]->var->mAliasFor : aParam[0]->var)
			->mAttrib &= ~VAR_ATTRIB_CACHE;
	}
}

BIF_DECL(BIF_getTokenValue)
{
   ExprTokenType *token = aParam[0];
   
   if (token->symbol != SYM_INTEGER)
      return;
   
   token = (ExprTokenType*) token->value_int64;

    if (token->symbol == SYM_VAR)
    {
      Var &var = *token->var;

      VarAttribType cache_attrib = var.mAttrib & (VAR_ATTRIB_IS_INT64 | VAR_ATTRIB_IS_DOUBLE);
      if (cache_attrib)
      {
         aResultToken.symbol = (SymbolType) (cache_attrib >> 4);
         aResultToken.value_int64 = var.mContentsInt64;
      }

      else if (var.mAttrib & VAR_ATTRIB_IS_OBJECT)
      {
         aResultToken.symbol = SYM_OBJECT;
         aResultToken.object = var.mObject;
      }
      else
      {
         aResultToken.symbol = SYM_STRING;
         aResultToken.marker = var.mCharContents;
      }
    }
    else
    {
        aResultToken.symbol = token->symbol;
        aResultToken.value_int64 = token->value_int64;
    }
    if (aResultToken.symbol == SYM_OBJECT)
        aResultToken.object->AddRef();
}