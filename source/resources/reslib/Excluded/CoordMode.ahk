CoordMode(m,v){
if m="ToolTip"
A_CoordModeToolTip:=v
else if m="Pixel"
A_CoordModePixel:=v
else if m="Mouse"
A_CoordModeMouse:=v
else if m="Caret"
A_CoordModeCaret:=v
else if m="Menu "
A_CoordModeMenu:=v
}