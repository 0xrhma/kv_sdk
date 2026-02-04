#include "kv.h"

int main(int argc, char* const argv[]) {
  if (argc < 2)
    return -1;
  KvObj res;
  if (!KvLoadFile(&res, argv[1]))
    return -1;
  // KvValue *val = KvObjGetValue(&res, "font_family");
  // if (!val)
  //   return -1;
  // if (!val->obj) {
  //   string font_color = "";
  //   printf("font_family : %s\n", font_color);
  // }
  // KvObjFree(&res);
  return 0;
}
