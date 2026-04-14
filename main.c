#include "kv.h"

void KvPrint(ulong spaces, struct KvObj* obj) 
{
  for (ulong i = 0; i < spaces; i++) 
  {
    fprintf(stderr, " ");
  }
  fprintf(stderr, "%s : [", obj->key);
  for (ulong i = 0; i < obj->nvalues; i++)
  {
    fprintf(stderr, " \'%s\' ", obj->values[i]);
  }
  fprintf(stderr, "]\n");
  if (obj->child)
  {
    KvPrint(spaces + 2, obj->child);
  }
  if (obj->next)
  {
    KvPrint(spaces, obj->next);
  }
}

int main(int argc, char* const argv[]) {
  if (argc < 2)
    return -1;
  KvResult res;
  if (!KvLoadFile(&res, argv[1]))
    return -1;
  KvPrint(0, res.obj);
  KvObjFree(&res);
  return 0;
}
