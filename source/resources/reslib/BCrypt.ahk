BCrypt(Algorithm, Buffer, Key:=""){
	static hBCRYPT := LoadLibrary("bcrypt.dll"),BCryptOpenAlgorithmProvider:=DynaCall("bcrypt\BCryptOpenAlgorithmProvider","tstui")
            ,BCryptGetProperty:=DynaCall("bcrypt\BCryptGetProperty","tttuitui"),BCryptCreateHash:=DynaCall("bcrypt\BCryptCreateHash","tttuituiui")
            ,BCryptHashData:=DynaCall("bcrypt\BCryptHashData","ttuiui"),BCryptFinishHash:=DynaCall("bcrypt\BCryptFinishHash","ttuiui")
            ,BCryptDestroyHash:=DynaCall("bcrypt\BCryptDestroyHash","t"),BCryptCloseAlgorithmProvider:=DynaCall("bcrypt\BCryptCloseAlgorithmProvider","tui")
    if BCryptOpenAlgorithmProvider(getvar(hAlgo:=0), StrUpper(Algorithm), 0, Key?0x00000008:0)
		|| BCryptGetProperty(hAlgo, StrPtr("ObjectLength"), getvar(cbHashObj:=0), 4, getvar(cbRes:=0), 0)
		|| BCryptGetProperty(hAlgo, StrPtr("HashDigestLength"), getvar(cbHash:=0), 4, getvar(cbRes), 0)
        return
    pbHashObj:=Buffer(cbHashObj)
    ,Key ? (pbSecret:=Buffer(cbSecret := StrPut(Key, "UTF-8")),StrPut(Key, pbSecret, "UTF-8"),pb:=pbSecret.Ptr,cb:=cbSecret - 1) : (pb:=cb:=0)
    ,aBuffer:=Type(Buffer)!="Buffer" ? (aBuffer:=Buffer(size := StrPut(Buffer, "UTF-8")),StrPut(Buffer, aBuffer, "UTF-8"),size--,aBuffer) : Buffer
    ,hash:=(pbHash:=Buffer(cbHash))  && !BCryptCreateHash(hAlgo, getvar(hHash:=0), pbHashObj, cbHashObj, pb, cb , 0) && !BCryptHashData(hHash, aBuffer, aBuffer.size, 0)	&& !BCryptFinishHash(hHash, pbHash, cbHash, 0) ? BinToHex(pbHash, cbHash) : ""
    BCryptDestroyHash(hHash),BCryptCloseAlgorithmProvider(hAlgo, 0)
    return hash
}