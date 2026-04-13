#include <stdio.h>
#include <sys/types.h>
#define KV_BOUNDS_CHECK
#define KV_UARENA_BOUNDS_CHECK
#define DEBUG
#define KV_REALLOC

#include "kv.h"

#ifndef KV_TYPES
typedef struct KvObj KvObj;
typedef struct KvValue KvValue;
typedef const char *string;
#endif

// internal methods, data structures and includes
#ifdef KV_BOUNDS_CHECK
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define print(...) fprintf(stderr, __VA_ARGS__)
#define printc(val) putc(val, stderr)

#ifdef DEBUG
#define dbg_out stderr
#define print_dbg(...) fprintf(dbg_out, __VA_ARGS__)
#else
#define print_dbg(...) ((void)0)
#endif

// for ascii formatted out
#ifdef __linux
#define CS_RESET "\033[0m"

#define CS_BOLD "\033[1m"
#define CS_UNDERLINE "\033[4m"

#define CS_RED "\033[31m"
#define CS_GREEN "\033[32m"
#define CS_YELLOW "\033[33m"
#else
// to do for other platforms
#define CS_RESET

#define CS_BOLD
#define CS_UNDERLINE

#define CS_RED
#define CS_GREEN
#define CS_YELLOW
#endif

unsigned char clut[256]; // lookuptable for char symbols during tokenization
static bool __kv_init = false;

// tokenizer prefix
char psym = 'c'; // prefix for symbols
char pstr = 's'; // prefix for strings

// "data types"

typedef struct {
  unsigned char *buffer;
  ulong offset;
#ifdef KV_UARENA_BOUNDS_CHECK
  ulong tsize;
#endif
} uarena;

#ifdef KV_UARENA_BOUNDS_CHECK
void uarena_init(uarena *arena, unsigned char *addr, ulong tsize) {
  assert(addr != NULL);
  arena->buffer = addr;
  arena->offset = 0L;
  arena->tsize = tsize;
}

unsigned char *uarena_alloc(uarena *arena, ulong size) {
  unsigned char *addr = arena->buffer + arena->offset;
  arena->offset += size;
  assert(arena->offset <= arena->tsize);
  return addr;
}
#else
void uarena_init(uarena *arena, unsigned char *addr) {
  arena->buffer = addr;
  arena->offset = 0L;
}

unsigned char *uarena_alloc(uarena *arena, ulong size) {
  unsigned char *addr = arena->buffer + arena->offset;
  arena->offset += size;
  return addr;
}
#endif

// internal util functions

typedef enum {
  Start, // "Start"
  KEntry,
  SeekColon,
  VEntry,
  SeekComma,
  LEntry,
  LSeekComma,
  End // "Accepted Phase"
} PStates;

typedef struct {
  PStates cstate; // current state
  char *tokens;   // tokens buffer
  ulong ntokens;  // total no of tokens
  ulong cindex;   // current token index
  ulong coffset;  // current token offset
  // upload params
  KvObj *cobj;    // current object
  char* cstrpos; // current string pos in string buffer
  char** cpstr;  // current string pointer pos in string pointer buffer
} ParseCtx;


struct Obj 
{
    const char* key;
    const char** values;
    ulong nvalues;
    struct Obj* child;
    struct Obj* next;

};

struct Obj objs[1000];
const char* sptrs[1000];
ulong iobj = 0;
ulong istpr = 0;

int RootParse(ParseCtx *ctx, bool root);
int KvParse(char *tokens, ulong ntokens, ulong nstrings, ulong mstrings);

int RootParse(ParseCtx *ctx, bool root) {
  const char *token;
  bool accept = false;
  struct Obj* before = NULL;
  // warning:: the tokens should only be incremented by 1 one step
  while ((ctx->ntokens != ctx->cindex) && !accept) {
    token = ctx->tokens + ctx->coffset;
    char sym = token[0];
    token += 1;
    if (sym != pstr)
      sym = token[0];

    switch (ctx->cstate) {
    case Start:
      switch (sym) {
      case ' ':
        break;
      case '\n':
        break;
      case '{':
        ctx->cstate = KEntry;
        break;
      default:
        // unexpected entry
        print("syntax error:: unexpected token \n");
        return 0;
        break;
      }
      break;
    case KEntry:
      switch (sym) {
      case '}':
        ctx->cstate = End;
        break;
      case 's':
        print_dbg("%s : ", token);
        // to string buffer
        strcpy(ctx->cstrpos, token);
        //to pointer buffer

        if (before)
        {
            before->next = &objs[iobj];
        }
        
        ctx->cpstr[0] = ctx->cstrpos; 
        iobj++;
        before = &objs[iobj - 1];
        before->values = sptrs + istpr;
        
        objs[iobj-1].key = ctx->cpstr[0];
        ctx->cstrpos += strlen(token)+1;
        ctx->cpstr += 1;

        ctx->cstate = SeekColon;
        break;
      case '\n':
        break;
      default:
        // unexpected entry
        print("syntax error:: unexpected token \n");
        return 0;
        break;
      }
      break;
    case SeekColon:
      switch (sym) {
      case ':':
        ctx->cstate = VEntry;
        break;
      case '\n':
        break;
      case ' ':
        break;
      default:
        // unexpected entry
        print("syntax error:: unexpected token \n");
        return 0;
        break;
      }
      break;
    case VEntry:
      switch (sym) {
      case 's':
        print_dbg("%s\n", token);
        sptrs[istpr] = token;
        istpr++;
        // to string buffer
        strcpy(ctx->cstrpos, token);
        //to pointer buffer
        ctx->cpstr[0] = ctx->cstrpos; 
        ctx->cstrpos += strlen(token)+1;
        ctx->cpstr += 1;
        before->nvalues++;
        ctx->cstate = SeekComma;
        break;
      case '[':
        print_dbg("[");
        ctx->cstate = LEntry;
        break;
      case '{':
        // consume token and forward
        print_dbg("\n");
        ctx->cindex++;
        ctx->coffset += strlen(token) + 2;
        ctx->cstate = KEntry; 
        
        objs[iobj-1].child = &objs[iobj];
        int status = RootParse(ctx, false);
        if (!status)
          return 0;
        ctx->cstate = SeekComma;
        goto rp_skip_increments;
        break;
      default:
        // unexpected entry
        print("syntax error:: unexpected token \n");
        return 0;
        break;
      }
      break;
    case SeekComma:
      switch (sym) {
      case ',':
        ctx->cstate = KEntry;
        break;
      case ' ':
        break;
      case '\n':
        break;
      case '}':
        ctx->cstate = End;
        break;
      default:
        // unexpected entry
        print("syntax error:: unexpected token \n");
        return 0;
        break;
      }
      break;
    case LEntry:
      switch (sym) {
      case 's':
        print_dbg("%s ", token);

        sptrs[istpr] = token;
        istpr++;
        
        // to string buffer
        strcpy(ctx->cstrpos, token);
        //to pointer buffer
        ctx->cpstr[0] = ctx->cstrpos;
        before->nvalues++;
        ctx->cstrpos += strlen(token)+1;
        ctx->cpstr += 1;
        
        ctx->cstate = LSeekComma;
        break;
      case ']':
        print_dbg("]\n");
        ctx->cstate = SeekComma;
        break;
      case ' ':
        break;
      case '\n':
        break;
      default:
        // unexpected entry
        print("syntax error:: unexpected token \n");
        return 0;
        break;
      }
      break;
    case LSeekComma:
      switch (sym) {
      case ',':
        ctx->cstate = LEntry;
        break;
      case ']':
        print_dbg("]\n");
        ctx->cstate = SeekComma;
        break;
      case '\n':
        break;
      default:
        // unexpected entry
        print("syntax error:: unexpected token \n");
        return 0;
        break;
      }
      break;
    case End:
      if (!root){
        accept = true;
        goto rp_skip_increments;
      }
      else {
        switch (sym) {
        case ' ':
          break;
        case '\n':
          break;
        default:
          // unexpected entry
          print("syntax error:: unexpected token \n");
          return 0;
          break;
        }
      }
      break;
    }

    ctx->cindex++; // incr the token index
    ctx->coffset += strlen(token) + 2; // offset to next token addr
rp_skip_increments:;
  }
  return 1;
}

void KvPrint(ulong spaces, struct Obj* obj) 
{
    for (ulong i = 0; i < spaces; i++) 
    {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "%s : [", obj->key);
    for (ulong i = 0; i < obj->nvalues; i++)
    {
        fprintf(stderr, " %s ", obj->values[i]);
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
};

int KvParse(char *tokens, ulong ntokens, ulong nstrings, ulong mstrings) {
  ParseCtx ctx;
  ctx.cstate = Start;
  ctx.tokens = tokens;
  ctx.ntokens = ntokens;
  ctx.cindex = 0L;
  ctx.coffset = 0L;

  ulong max_mem_size = mstrings + sizeof(char**)*nstrings + sizeof(KvValue)*nstrings;
  
  // print_dbg(CS_BOLD CS_YELLOW"dub_info::total mem prediction for KvParse:: %lu\n"CS_RESET, max_mem_size);
  
  char* buffer = malloc(mstrings + sizeof(char**)*nstrings);
  KvObj res;
  
  
  ctx.cobj = &res; 
  ctx.cstrpos = buffer;
  ctx.cpstr = (char**)(buffer + mstrings);
  int status = RootParse(&ctx, true);
  
  if (!status)
    goto jump_kv_parse_free;
  
  char* pos = buffer;
  for (ulong i = 0; i < nstrings; i++) {
    print("%s\n", pos);
    pos += strlen(pos)+1;
  }
  print("\n\n");

  char** strings = (char**)(buffer + mstrings);
  for (ulong i = 0; i < nstrings; i++) {
    print("%s\n", strings[i]);
  }

  // print all
  KvPrint(0, objs);
jump_kv_parse_free:
  free(buffer);
  return status;
}

// tokenizer errors
void perr_tokenizer_str_close(const char *src, long len_str, long line_count,
                              long line_start_index, long symbol_index);
void perr_tokenizer_escape(const char *src, long len_str, long line_count,
                           long line_start_index, long symbol_index);

void perr_tokenizer_str_close(const char *src, long len_str, long line_count,
                              long line_start_index, long symbol_index) {
  print(CS_BOLD CS_RED "syntax error (line %ld)::" CS_RESET
                       " expected [" CS_BOLD "'\"'" CS_RESET "] after \n",
        line_count);
  print(CS_BOLD "       %ld |       " CS_RESET, line_count);
  for (long i = line_start_index; i < len_str; i++) {
    if (src[i] == '\n')
      break;
    if (i == symbol_index)
      print(CS_BOLD CS_RED CS_UNDERLINE "%c" CS_RESET, src[i]);
    else
      printc(src[i]);
  }
  printc('\n');
}

void perr_tokenizer_escape(const char *src, long len_str, long line_count,
                           long line_start_index, long symbol_index) {
  print(CS_BOLD CS_RED "syntax error (line %ld)::" CS_RESET
                       " invalid escape sequence, must follow  [" CS_BOLD
                       "'\"', '\\'" CS_RESET "]\n",
        line_count);
  print(CS_BOLD "       %ld |       " CS_RESET, line_count);
  for (long i = line_start_index; i < len_str; i++) {
    if (src[i] == '\n')
      break;
    if (i == symbol_index)
      print(CS_BOLD CS_RED CS_UNDERLINE "%c" CS_RESET, src[i]);
    else
      printc(src[i]);
  }
  printc('\n');
}

// returns allocated sizes in bytes
ulong string_appendn(char *_dest, const char *src, ulong len, ulong offset,
                     bool str);
// returns allocated sizes in bytes
ulong string_appendencn(char *_dest, const char *src, ulong len, ulong offset,
                        bool str);

ulong string_appendn(char *_dest, const char *src, ulong len, ulong offset,
                     bool str) {
  char *dest = _dest + offset + 1;
  if (str)
    *(dest - 1) = pstr;
  else
    *(dest - 1) = psym;
  memcpy(dest, src, len);
  dest[len] = '\0';
  return len + 2;
}

ulong string_appendencn(char * _dest, const char *src, ulong len,
                        ulong offset, bool str) {
  char *dest = _dest + offset + 1;
  if (str)
    *(dest - 1) = pstr;
  else
    *(dest - 1) = psym;
  ulong alen = 0L;
  unsigned char cons = 0;
  for (ulong i = 0; i < len; i++) {
    unsigned char uchar = src[i];
    if (uchar == '\\') {
      if (cons) {
        dest[alen] = uchar;
        alen++;
      }
      cons = !cons;
    } else {
      if (uchar == '"')
        cons = 0;
      dest[alen] = uchar;
      alen++;
    }
  }
  dest[alen] = '\0';
  return alen + 2;
}

void KvInit() {
  unsigned char csymbols[12] = {':', '{', '}', '"', '(',  ')',
                                '[', ']', ',', ' ', '\\', '\n'};
  for (int i = 0; i < 256; i++)
    clut[i] = 0;

  for (int i = 0; i < 12; i++)
    clut[csymbols[i]] = 1;
  __kv_init = true;
}

// string KvObjGetKeyIndex(struct KvObj *pobj, ulong index) {
// #ifdef KV_BOUNDS_CHECK
//   assert(index <= pobj->kcount);
// #endif
//   string raddr = pobj->keys;
//   for (ulong i = 0L; i < pobj->kcount; i++) {
//     raddr += strlen(raddr) + 1;
//   }
//   return raddr;
// }

// KvValue *KvObjGetValue(KvObj *pobj, string key) {
//   ulong nkeys = pobj->kcount;
//   string cstr = pobj->keys;
//   for (ulong i = 0; i < nkeys; i++) {
//     if (strcmp(key, cstr) == 0)
//       return &pobj->values[i]; // match found
//     cstr += strlen(cstr) + 1;
//   }
//   return NULL;
// }

int KvLoadFile(struct KvObj *pobj, const char *path) {
  if (!__kv_init)
    KvInit();

  print_dbg(CS_YELLOW CS_BOLD "dbg_info:: reading file " CS_RESET CS_BOLD
                              "'%s'\n" CS_RESET,
            path);
  FILE *fp = fopen(path, "r");
  if (!fp) {
    print(CS_RED CS_BOLD "error:: couldn't access file::" CS_RESET CS_BOLD
                         " '%s'\n" CS_RESET,
          path);
    return 0;
  }

  fseek(fp, 0L, SEEK_END);
  long fsize = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  assert(fsize >= 0L); // solve later
  if (fsize == 0L) {
    print(CS_BOLD CS_YELLOW "warning:: " CS_RESET "file is empty '%s'\n", path);
    fclose(fp);
    return 0;
  }
  print_dbg(CS_BOLD CS_YELLOW
            "dbg_info:: 'total file size (inc null term)' :: " CS_RESET CS_BOLD
            "'%ld'\n" CS_RESET,
            fsize);

  ulong max_mem_size = 4 * fsize + 1;
  char *src = (char *)(malloc(max_mem_size));
  src[fsize] = '\0';
  char *tokens = src + fsize+1;
  
  fread(src, fsize, 1, fp);
  fclose(fp);

  long si = 0L;
  long strseek = 0L;
  long escseek = 0L;

  long cline = 1L;
  long icline = 0L;

  long sline = cline;
  long isline = icline;

  long eline = cline;
  long ieline = icline;

  unsigned long nstrings = 0L; // total no of strings
  unsigned long mstrings = 0L; // string type token memory size
  unsigned long offset = 0L;   // this will include the total token memory size
  unsigned long ntokens = 0L;

  for (long i = 0; i < fsize; i++) {
    unsigned char cchar = src[i];
    long nlen = i - si;
    if (clut[cchar]) {
      if (!strseek) {
        if (nlen) {
          offset += string_appendn(tokens, src + si, nlen, offset, true);
          nstrings++;
          mstrings += nlen + 1;
          ntokens++;
        }
        if (cchar != '"') {
          if (cchar != ' ') {
            offset += string_appendn(tokens, src + i, 1L, offset, false);
            ntokens++;
          }

        } else {
          strseek = i + 1;
          sline = cline;
          isline = icline;
        }
        si = i + 1;
      } else {
        if (cchar == '"') {
          if (!escseek) {
            if (nlen) {
              offset += string_appendencn(tokens, src + si, nlen, offset, true);
              nstrings++;
              mstrings += nlen + 1;
              ntokens++;
            } else {
              offset += string_appendn(tokens, "", 0L, offset, false);
              nstrings++;
              mstrings += 1L;
              ntokens++;
            }
            strseek = false;
            si = i + 1;
          } else
            escseek = false;
        } else if (cchar == '\\') {
          if (escseek)
            escseek = false;
          else {
            escseek = i + 1;
            eline = cline;
            ieline = icline;
          }
        }
      }
    } else {
      if (escseek) {
        perr_tokenizer_escape(src, fsize, eline, ieline, escseek - 1);
        goto ftkn_exit;
      }
      // assert(!escseek); // invalid after escape charactor '/'
    }
    if (cchar == '\n') {
      cline++;
      icline = i + 1;
    }
  }

  long nlen = fsize - si;

  if (nlen != 0) {
    if (strseek) {
      perr_tokenizer_str_close(src, fsize, sline, isline, strseek - 1);
      goto ftkn_exit;
    }
    // assert(!strseek); // invalid searching for string closure "
    offset += string_appendn(tokens, src + si, nlen, offset, true);
    nstrings++;
    mstrings += nlen + 1;
    ntokens++;
  }

  #ifdef KV_REALLOC
  src = (char *)realloc(src, offset + fsize + 1);
  tokens = src + fsize + 1;
  #endif

  // file content log
  print_dbg(CS_BOLD CS_YELLOW "dbg_info:: 'file content':: " CS_RESET CS_BOLD
                              "\n%s\n" CS_RESET,
            src);

  // log tokenizer outputs
  print_dbg(CS_BOLD CS_YELLOW "dbg_print::tokenizer tokens::\n" CS_RESET);
  #ifdef DEBUG
  uint toffset = 0L;
  string token;
  print(CS_BOLD "[ " CS_RESET);
  for (uint i = 0; i < ntokens; i++) {
    token = tokens + toffset;
    if (token[0] == pstr)
      print(CS_BOLD CS_UNDERLINE CS_GREEN "%s" CS_RESET ", ", token + 1);
    else {
      if (token[1] == '\n')
        print(CS_BOLD CS_RED "\\n, " CS_RESET);
      else
        print(CS_BOLD CS_RED "%s" CS_RESET ", ", token + 1);
    }
    toffset += strlen(token) + 1;
  }
  print(CS_BOLD "]\n" CS_RESET);
  #endif

  #ifndef KV_REALLOC
  print_dbg(CS_YELLOW CS_BOLD "dbg_print::tokenizer log::\n" CS_RESET CS_BOLD
                              "   total memory allocated (exc src) : %lu\n"
                              "   total memory used : %lu\n"
                              "   total no of tokens : %lu\n"
                              "   total no of strings : %lu\n"
                              "   total mem of strings : %lu\n" CS_RESET,
            3 * fsize, offset, ntokens, nstrings, mstrings);
  #else
  print_dbg(CS_YELLOW CS_BOLD "dbg_print::tokenizer log::\n" CS_RESET CS_BOLD
                              "   total memory allocated : %lu\n"
                              "   total memory used : %lu\n"
                              "   total memory reallocated : %lu\n"
                              "   total no of tokens : %lu\n"
                              "   total no of strings : %lu\n"
                              "   total mem of strings : %lu\n" CS_RESET,
            4 * fsize + 1, fsize + 1 + offset, fsize + 1 + offset, ntokens,
            nstrings, mstrings);
  #endif
  // recursive parsing
  print_dbg(CS_BOLD CS_YELLOW "dbg_print::parsing tokens::\n" CS_RESET);
  KvParse(tokens, ntokens, nstrings, mstrings);

ftkn_exit:
  free(src);
  return 0;
}

// string KvValueGetValueIndex(struct KvValue *pval, ulong index) {

//   #ifdef KV_BOUNDS_CHECK
//   assert(index <= pval->vcount);
//   #endif

//   string raddr = pval->values;
//   for (ulong i = 0L; i < pval->vcount; i++) {
//     raddr += strlen(raddr) + 1;
//   }
//   return raddr;
// }

// bool KvValueIsObj(struct KvValue *pval) { return pval->obj != NULL; }

// void KvObjFree(KvObj *pobj) {}