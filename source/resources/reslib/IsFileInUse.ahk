IsFileInUse(f){
return FileExist(f)&&!FileOpen(f,"w -w")
}