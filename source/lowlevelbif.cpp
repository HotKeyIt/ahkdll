#include "pch.h" // pre-compiled headers
#include "globaldata.h" // for access to many global vars
#include "application.h" // for MsgSleep()
#include "exports.h"
#include "script.h"

BIF_DECL(BIF_Getvar)
{
	if (aParam[0]->symbol == SYM_VAR)
	{
		if (aParam[0]->var->IsAlias() && (aParamCount == 1 || TokenToBOOL(*aParam[1])))
			aResultToken.Return((__int64)aParam[0]->var->ResolveAlias());
		else
			aResultToken.Return((__int64)aParam[0]->var);
	}
	else if (aParam[0]->symbol == SYM_STRING && !(TokenToInt64(*aParam[0])))
		aResultToken.value_int64 = (__int64)g_script->FindOrAddVar(aParam[0]->marker, aParam[0]->marker_length, VAR_LOCAL | VAR_GLOBAL);
	else
		aResultToken.Return(0);
}


BIF_DECL(BIF_Alias)
{
	ExprTokenType& aParam0 = *aParam[0];
	ExprTokenType& aParam1 = *aParam[1];
	aResultToken.SetValue(_T(""));
	if (aParam0.symbol == SYM_STRING && !(TokenToInt64(aParam0)))
		if (auto v = g_script->FindOrAddVar(aParam0.marker, aParam0.marker_length, VAR_LOCAL | VAR_GLOBAL))
			aParam0.symbol = SYM_VAR, aParam0.var = v;
		else
			return;
	if (aParam0.symbol == SYM_VAR)
	{
		Var& var = *aParam0.var;
		UINT_PTR len = 0;
		if (aParamCount == 2)
			switch (aParam1.symbol)
			{
			case SYM_VAR:
				len = (UINT_PTR)(aParam1.var->ResolveAlias());
				break;
			case SYM_INTEGER:
				// HotKeyIt added to accept var pointer
				len = (UINT_PTR)aParam1.value_int64;
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