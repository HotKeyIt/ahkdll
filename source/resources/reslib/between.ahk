between(v,lh,hl){
  return v>=(lh<hl?lh:hl)&&v<=(hl>lh?hl:lh)
}