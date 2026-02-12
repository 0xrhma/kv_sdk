#ifndef kv_h
#define kv_h

#define KV_TYPES

#include <stdio.h>
#include <stdbool.h>

//datatypes used
typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long ulong;
typedef const char* string;

struct KvObj {
  string* keys;
  struct KvValue* values;
  ulong count; // total key entries
  // ulong ksize; // total size consumed by keys in bytes
};

struct KvValue{
  string* entries;
  struct KvObj* obj; 
  ulong count; // total value entries
  // ulong vsize; // total size consumed by values in bytes
};

#ifdef KV_TYPES
typedef struct KvObj KvObj;
typedef struct KvValue KvValue;
#endif

// KvObj methods


// not required to be called bofore in a single threaded usecase
void KvInit();
KvObj* KvLoadFile(const char *path);
void KvObjFree(struct KvObj* pobj);
struct KvValue* KvObjGetValue(struct KvObj* pobj, string key);

// KvValue methods
string KvValueGetValueIndex(struct KvValue* pval, ulong index);
bool KvValueIsObj(struct KvValue* pval);

#endif
