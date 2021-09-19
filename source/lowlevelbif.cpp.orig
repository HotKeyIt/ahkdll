#include "stdafx.h" // pre-compiled headers
#include "globaldata.h" // for access to many global vars
#include "application.h" // for MsgSleep()
#include "exports.h"
#include "script.h"

BIF_DECL(BIF_Getvar)
{
	if (aParam[0]->symbol == SYM_STRING && !(TokenToInt64(*aParam[0])))
		aResultToken.value_int64 = (__int64)g_script->FindOrAddVar(aParam[0]->marker);
	else if (aParam[0]->symbol == SYM_VAR)
	{
		if (aParam[0]->var->mType == VAR_ALIAS && aParamCount > 1 && TokenToInt64(*aParam[1]))
			aResultToken.value_int64 = (__int64)aParam[0]->var->mAliasFor;
		else
			aResultToken.value_int64 = (__int64)aParam[0]->var;
	}
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
		// free variable
		var.Free(VAR_ALWAYS_FREE, true);
		if (var.mAttrib & VAR_ATTRIB_UNINITIALIZED)
			var.mAttrib &= ~VAR_ATTRIB_UNINITIALIZED;
		if (len)
		{
			var.mType = VAR_ALIAS;
			var.mByteLength = len;
			if (var.mAliasFor->HasObject())
				var.mAliasFor->mObject->AddRef();
		}
	}
}
