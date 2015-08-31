isbom(address,bom:="UTF-8"){
if (bom="UTF-8")
return 0xBFBBEF=NumGet(address,"UInt")&0xFFFFFF
else if (bom="UTF-16")
return 0xFEFF=NumGet(address,"UShort")&0xFFFF
else if (bom="UTF-16BE")
return 0xFFFE=NumGet(address,"UShort")&0xFFFF
else if (bom="UTF-32")
return 0x0000FEFF=NumGet(address,"UInt64")&0xFFFFFFFF
else if (bom="UTF-32BE")
return 0x0000FFFE=NumGet(address,"UInt64")&0xFFFFFFFF
}