StrBuf(string, encoding:="UTF-8"){
    return (buf := BufferAlloc(StrPut(string, encoding)),StrPut(string, buf, encoding),buf)
}