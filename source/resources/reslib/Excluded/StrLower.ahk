StrLower(inputVar,T:=0){
  static CharUpper:=DynaCall("CharUpper","s=t"),CharLower:=DynaCall("CharLower","s=t"),IsCharAlpha:=DynaCall("IsCharAlpha","c")
  if T{
    convert_next_alpha_char_to_upper := true
  	LoopParse,%inputVar%
  	{
  		if IsCharAlpha[NumGet(&inputVar,(A_Index-1)*2,"Char")]{ ; Use this to better support chars from non-English languages.
  			if (convert_next_alpha_char_to_upper)
  				CharUpper[&inputVar+(A_Index-1)*2]
  				,convert_next_alpha_char_to_upper := false
  			else
  				CharLower[&inputVar+(A_Index-1)*2]
  		}
  		else if (A_LoopField=" ")
  				convert_next_alpha_char_to_upper := true
  		; Otherwise, it's a digit, punctuation mark, etc. so nothing needs to be done.
  	}
  	return inputVar
  }
  return CharLower[&inputVar]
}