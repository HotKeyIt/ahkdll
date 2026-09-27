IsFileInUse(f,access:="rwd"){
if !FileExist(f)
  return 0
if !file:=FileOpen(f,"rw -" access)
  return 1
file.Close()
return 0
}