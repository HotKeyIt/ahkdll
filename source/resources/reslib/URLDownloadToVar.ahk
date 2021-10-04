URLDownloadToVar(url) => (hObject:=ComObject("WinHttp.WinHttpRequest.5.1"),hObject.Option[2] := 65001,hObject.Open("GET",url),hObject.Send(),hObject.ResponseText)
