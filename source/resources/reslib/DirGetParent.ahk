dirgetparent(path,parent:=1){
path:=RTrim(path,"\")
while parent>=idx:=A_Index
Loop Files, path,"FD"
if idx=parent
return A_LoopFileDir (A_LoopFileDir?"\":"")
else if path:=A_LoopFileDir
break
}