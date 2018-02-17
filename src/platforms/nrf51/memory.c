
void memcheck(){
  int blockSize=16;
  int i=1;
  for(; true; i++){
    char* p=malloc(i*blockSize);
    if(!p) break;
    free(p);
  }
  char* x=strdup("---------------------------------");
  log_write("Can alloc %d bytes; strdup=%s\n", (i-1)*blockSize, x? x: "null!");
  if(x) free(x);
}

