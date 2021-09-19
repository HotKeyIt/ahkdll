isbom(address,bom:=0){
return bom="UTF-8"?0xBFBBEF=NumGet(address,"UInt")&0xFFFFFF
     : bom="UTF-16"?0xFEFF=NumGet(address,"UShort")&0xFFFF
     : bom="UTF-16BE"?0xFFFE=NumGet(address,"UShort")&0xFFFF
     : bom="UTF-32"?0x0000FEFF=NumGet(address,"UInt64")&0xFFFFFFFF
     : bom="UTF-32BE"?0x0000FFFE=NumGet(address,"UInt64")&0xFFFFFFFF
     : !bom?(0xBFBBEF=NumGet(address,"UInt")&0xFFFFFF?"UTF-8":0xFEFF=NumGet(address,"UShort")&0xFFFF?"UTF-16":0xFFFE=NumGet(address,"UShort")&0xFFFF?"UTF-16BE":0x0000FEFF=NumGet(address,"UInt64")&0xFFFFFFFF?"UTF-32":0x0000FFFE=NumGet(address,"UInt64")&0xFFFFFFFF?"UTF-32BE":"CP0"):""
}