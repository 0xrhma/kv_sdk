#include "kv.h"
#include <stdio.h>

void pkv(KvObj* obj, uint ts) {
  for (uint i = 0; i < obj->count; i++) {
    for (uint tc=0; tc<ts; tc++)
      fputc(' ', stdout);
    fprintf(stdout, "%s : ", obj->keys[i]);
    KvValue* cobj = obj->values + i;
    if (cobj->obj) {
      fputc('\n', stdout);
      pkv(cobj->obj, ts+2);
    }
    else {
      for (uint l=0; l<cobj->count; l++)
        fprintf(stdout, " %s ", cobj->entries[l]);
    }
    fputc('\n', stdout);
  }
}


int main(int argc, char* const argv[]) {
  if (argc < 2)
    return -1;
  KvObj* res = KvLoadFile(argv[1]);
  if (!res) 
    return -1;
  // KvValue *val = KvObjGetValue(res, "themes");
  // val = KvObjGetValue(val->obj, "dark");
  // val = KvObjGetValue(val->obj, "color");
  
  // fprintf(stdout, "%s\n", val->entries[0]);
  pkv(res, 0);

  KvObjFree(res);
  return 0;
}
