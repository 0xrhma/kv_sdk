#ifndef kv_h
#define kv_h

#define KV_TYPES

#include <stdbool.h>
#include <stdio.h>

// datatypes used
typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long ulong;
typedef const char *cstring;

struct KvObj
{
  const char *key;
  const char **values;
  ulong nvalues;
  struct KvObj *child;
  struct KvObj *next;
};

struct KvResult
{
  struct KvObj *obj;
  uchar *__buffers[2]; // internal buffers malloced donot manually free
};

#ifdef KV_TYPES
typedef struct KvObj KvObj;
typedef struct KvResult KvResult;
#endif

// KvObj methods

// not required to be called bofore in a single threaded usecase
void KvInit();
int KvLoadFile(struct KvResult *res, const char *path);
void KvObjFree(struct KvResult *res);

#endif
