#ifndef __DYNTAPES_H__
#define __DYNTAPES_H__

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define DYNTYPES_REALLOC(c,p,s) realloc(p,s)
#define DYNTYPES_FREE(c,p)      free(p)

#define arrlen      dyntypes_arrlen
#define arrlenu     dyntypes_arrlenu
#define arrput      dyntypes_arrput
#define arrpush     dyntypes_arrput
#define arrpop      dyntypes_arrpop
#define arrfree     dyntypes_arrfree
#define arraddn     dyntypes_arraddn // deprecated, use one of the following instead:
#define arraddnptr  dyntypes_arraddnptr
#define arraddnindex dyntypes_arraddnindex
#define arrsetlen   dyntypes_arrsetlen
#define arrlast     dyntypes_arrlast
#define arrins      dyntypes_arrins
#define arrinsn     dyntypes_arrinsn
#define arrdel      dyntypes_arrdel
#define arrdeln     dyntypes_arrdeln
#define arrdelswap  dyntypes_arrdelswap
#define arrcap      dyntypes_arrcap
#define arrsetcap   dyntypes_arrsetcap

#define hmput       dyntypes_hmput
#define hmputs      dyntypes_hmputs
#define hmget       dyntypes_hmget
#define hmget_ts    dyntypes_hmget_ts
#define hmgets      dyntypes_hmgets
#define hmgetp      dyntypes_hmgetp
#define hmgetp_ts   dyntypes_hmgetp_ts
#define hmgetp_null dyntypes_hmgetp_null
#define hmgeti      dyntypes_hmgeti
#define hmgeti_ts   dyntypes_hmgeti_ts
#define hmdel       dyntypes_hmdel
#define hmlen       dyntypes_hmlen
#define hmlenu      dyntypes_hmlenu
#define hmfree      dyntypes_hmfree
#define hmdefault   dyntypes_hmdefault
#define hmdefaults  dyntypes_hmdefaults

#define shput       dyntypes_shput
#define shputi      dyntypes_shputi
#define shputs      dyntypes_shputs
#define shget       dyntypes_shget
#define shgeti      dyntypes_shgeti
#define shgets      dyntypes_shgets
#define shgetp      dyntypes_shgetp
#define shgetp_null dyntypes_shgetp_null
#define shdel       dyntypes_shdel
#define shlen       dyntypes_shlen
#define shlenu      dyntypes_shlenu
#define shfree      dyntypes_shfree
#define shdefault   dyntypes_shdefault
#define shdefaults  dyntypes_shdefaults
#define sh_new_arena  dyntypes_sh_new_arena
#define sh_new_strdup dyntypes_sh_new_strdup

#define stralloc    dyntypes_stralloc
#define strreset    dyntypes_strreset

#ifdef _MSC_VER
#define DYNTYPES_NOTUSED(v)  (void)(v)
#else
#define DYNTYPES_NOTUSED(v)  (void)sizeof(v)
#endif

__BEGIN_EXTERN_C__

// for security against attackers, seed the library with a random number, at least time() but stronger is better
extern void dyntypes_rand_seed(size_t seed);

// these are the hash functions used internally if you want to test them or use them for other purposes
extern size_t dyntypes_hash_bytes(void *p, size_t len, size_t seed);
extern size_t dyntypes_hash_string(char *str, size_t seed);

// this is a simple string arena allocator, initialize with e.g. 'dyntypes_string_arena my_arena={0}'.
typedef struct dyntypes_string_arena dyntypes_string_arena;
extern char * dyntypes_stralloc(dyntypes_string_arena *a, char *str);
extern void   dyntypes_strreset(dyntypes_string_arena *a);

// have to #define DYNTYPES_UNIT_TESTS to call this
extern void dyntypes_unit_tests(void);

///////////////
//
// Everything below here is implementation details
//

extern void * dyntypes_arrgrowf(void *a, size_t elemsize, size_t addlen, size_t min_cap);
extern void   dyntypes_arrfreef(void *a);
extern void   dyntypes_hmfree_func(void *p, size_t elemsize);
extern void * dyntypes_hmget_key(void *a, size_t elemsize, void *key, size_t keysize, int mode);
extern void * dyntypes_hmget_key_ts(void *a, size_t elemsize, void *key, size_t keysize, ptrdiff_t *temp, int mode);
extern void * dyntypes_hmput_default(void *a, size_t elemsize);
extern void * dyntypes_hmput_key(void *a, size_t elemsize, void *key, size_t keysize, int mode);
extern void * dyntypes_hmdel_key(void *a, size_t elemsize, void *key, size_t keysize, size_t keyoffset, int mode);
extern void * dyntypes_shmode_func(size_t elemsize, int mode);

__END_EXTERN_C__

#if defined(__GNUC__) || defined(__clang__)
#define DYNTYPES_HAS_TYPEOF
#ifdef __cplusplus
//#define DYNTYPES_HAS_LITERAL_ARRAY  // this is currently broken for clang
#endif
#endif

#if !defined(__cplusplus)
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define DYNTYPES_HAS_LITERAL_ARRAY
#endif
#endif

// this macro takes the address of the argument, but on gcc/clang can accept rvalues
#if defined(DYNTYPES_HAS_LITERAL_ARRAY) && defined(DYNTYPES_HAS_TYPEOF)
  #if __clang__
  #define DYNTYPES_ADDRESSOF(typevar, value)     ((__typeof__(typevar)[1]){value}) // literal array decays to pointer to value
  #else
  #define DYNTYPES_ADDRESSOF(typevar, value)     ((typeof(typevar)[1]){value}) // literal array decays to pointer to value
  #endif
#else
#define DYNTYPES_ADDRESSOF(typevar, value)     &(value)
#endif

#define DYNTYPES_OFFSETOF(var,field)           ((char *) &(var)->field - (char *) (var))

#define dyntypes_header(t)  ((dyntypes_array_header *) (t) - 1)
#define dyntypes_temp(t)    dyntypes_header(t)->temp
#define dyntypes_temp_key(t) (*(char **) dyntypes_header(t)->hash_table)

#define dyntypes_arrsetcap(a,n)   (dyntypes_arrgrow(a,0,n))
#define dyntypes_arrsetlen(a,n)   ((dyntypes_arrcap(a) < (size_t) (n) ? dyntypes_arrsetcap((a),(size_t)(n)),0 : 0), (a) ? dyntypes_header(a)->length = (size_t) (n) : 0)
#define dyntypes_arrcap(a)        ((a) ? dyntypes_header(a)->capacity : 0)
#define dyntypes_arrlen(a)        ((a) ? (ptrdiff_t) dyntypes_header(a)->length : 0)
#define dyntypes_arrlenu(a)       ((a) ?             dyntypes_header(a)->length : 0)
#define dyntypes_arrput(a,v)      (dyntypes_arrmaybegrow(a,1), (a)[dyntypes_header(a)->length++] = (v))
#define dyntypes_arrpush          dyntypes_arrput  // synonym
#define dyntypes_arrpop(a)        (dyntypes_header(a)->length--, (a)[dyntypes_header(a)->length])
#define dyntypes_arraddn(a,n)     ((void)(dyntypes_arraddnindex(a, n)))    // deprecated, use one of the following instead:
#define dyntypes_arraddnptr(a,n)  (dyntypes_arrmaybegrow(a,n), (n) ? (dyntypes_header(a)->length += (n), &(a)[dyntypes_header(a)->length-(n)]) : (a))
#define dyntypes_arraddnindex(a,n)(dyntypes_arrmaybegrow(a,n), (n) ? (dyntypes_header(a)->length += (n), dyntypes_header(a)->length-(n)) : dyntypes_arrlen(a))
#define dyntypes_arraddnoff       dyntypes_arraddnindex
#define dyntypes_arrlast(a)       ((a)[dyntypes_header(a)->length-1])
#define dyntypes_arrfree(a)       ((void) ((a) ? DYNTYPES_FREE(NULL,dyntypes_header(a)) : (void)0), (a)=NULL)
#define dyntypes_arrdel(a,i)      dyntypes_arrdeln(a,i,1)
#define dyntypes_arrdeln(a,i,n)   (memmove(&(a)[i], &(a)[(i)+(n)], sizeof *(a) * (dyntypes_header(a)->length-(n)-(i))), dyntypes_header(a)->length -= (n))
#define dyntypes_arrdelswap(a,i)  ((a)[i] = dyntypes_arrlast(a), dyntypes_header(a)->length -= 1)
#define dyntypes_arrinsn(a,i,n)   (dyntypes_arraddn((a),(n)), memmove(&(a)[(i)+(n)], &(a)[i], sizeof *(a) * (dyntypes_header(a)->length-(n)-(i))))
#define dyntypes_arrins(a,i,v)    (dyntypes_arrinsn((a),(i),1), (a)[i]=(v))

#define dyntypes_arrmaybegrow(a,n)  ((!(a) || dyntypes_header(a)->length + (n) > dyntypes_header(a)->capacity) \
                                  ? (dyntypes_arrgrow(a,n,0),0) : 0)

#define dyntypes_arrgrow(a,b,c)   ((a) = dyntypes_arrgrowf_wrapper((a), sizeof *(a), (b), (c)))

#define dyntypes_hmput(t, k, v) \
    ((t) = dyntypes_hmput_key_wrapper((t), sizeof *(t), (void*) DYNTYPES_ADDRESSOF((t)->key, (k)), sizeof (t)->key, 0),   \
     (t)[dyntypes_temp((t)-1)].key = (k),    \
     (t)[dyntypes_temp((t)-1)].value = (v))

#define dyntypes_hmputs(t, s) \
    ((t) = dyntypes_hmput_key_wrapper((t), sizeof *(t), &(s).key, sizeof (s).key, DYNTYPES_HM_BINARY), \
     (t)[dyntypes_temp((t)-1)] = (s))

#define dyntypes_hmgeti(t,k) \
    ((t) = dyntypes_hmget_key_wrapper((t), sizeof *(t), (void*) DYNTYPES_ADDRESSOF((t)->key, (k)), sizeof (t)->key, DYNTYPES_HM_BINARY), \
      dyntypes_temp((t)-1))

#define dyntypes_hmgeti_ts(t,k,temp) \
    ((t) = dyntypes_hmget_key_ts_wrapper((t), sizeof *(t), (void*) DYNTYPES_ADDRESSOF((t)->key, (k)), sizeof (t)->key, &(temp), DYNTYPES_HM_BINARY), \
      (temp))

#define dyntypes_hmgetp(t, k) \
    ((void) dyntypes_hmgeti(t,k), &(t)[dyntypes_temp((t)-1)])

#define dyntypes_hmgetp_ts(t, k, temp) \
    ((void) dyntypes_hmgeti_ts(t,k,temp), &(t)[temp])

#define dyntypes_hmdel(t,k) \
    (((t) = dyntypes_hmdel_key_wrapper((t),sizeof *(t), (void*) DYNTYPES_ADDRESSOF((t)->key, (k)), sizeof (t)->key, DYNTYPES_OFFSETOF((t),key), DYNTYPES_HM_BINARY)),(t)?dyntypes_temp((t)-1):0)

#define dyntypes_hmdefault(t, v) \
    ((t) = dyntypes_hmput_default_wrapper((t), sizeof *(t)), (t)[-1].value = (v))

#define dyntypes_hmdefaults(t, s) \
    ((t) = dyntypes_hmput_default_wrapper((t), sizeof *(t)), (t)[-1] = (s))

#define dyntypes_hmfree(p)        \
    ((void) ((p) != NULL ? dyntypes_hmfree_func((p)-1,sizeof*(p)),0 : 0),(p)=NULL)

#define dyntypes_hmgets(t, k)    (*dyntypes_hmgetp(t,k))
#define dyntypes_hmget(t, k)     (dyntypes_hmgetp(t,k)->value)
#define dyntypes_hmget_ts(t, k, temp)  (dyntypes_hmgetp_ts(t,k,temp)->value)
#define dyntypes_hmlen(t)        ((t) ? (ptrdiff_t) dyntypes_header((t)-1)->length-1 : 0)
#define dyntypes_hmlenu(t)       ((t) ?             dyntypes_header((t)-1)->length-1 : 0)
#define dyntypes_hmgetp_null(t,k)  (dyntypes_hmgeti(t,k) == -1 ? NULL : &(t)[dyntypes_temp((t)-1)])

#define dyntypes_shput(t, k, v) \
    ((t) = dyntypes_hmput_key_wrapper((t), sizeof *(t), (void*) (k), sizeof (t)->key, DYNTYPES_HM_STRING),   \
     (t)[dyntypes_temp((t)-1)].value = (v))

#define dyntypes_shputi(t, k, v) \
    ((t) = dyntypes_hmput_key_wrapper((t), sizeof *(t), (void*) (k), sizeof (t)->key, DYNTYPES_HM_STRING),   \
     (t)[dyntypes_temp((t)-1)].value = (v), dyntypes_temp((t)-1))

#define dyntypes_shputs(t, s) \
    ((t) = dyntypes_hmput_key_wrapper((t), sizeof *(t), (void*) (s).key, sizeof (s).key, DYNTYPES_HM_STRING), \
     (t)[dyntypes_temp((t)-1)] = (s), \
     (t)[dyntypes_temp((t)-1)].key = dyntypes_temp_key((t)-1)) // above line overwrites whole structure, so must rewrite key here if it was allocated internally

#define dyntypes_pshput(t, p) \
    ((t) = dyntypes_hmput_key_wrapper((t), sizeof *(t), (void*) (p)->key, sizeof (p)->key, DYNTYPES_HM_PTR_TO_STRING), \
     (t)[dyntypes_temp((t)-1)] = (p))

#define dyntypes_shgeti(t,k) \
     ((t) = dyntypes_hmget_key_wrapper((t), sizeof *(t), (void*) (k), sizeof (t)->key, DYNTYPES_HM_STRING), \
      dyntypes_temp((t)-1))

#define dyntypes_pshgeti(t,k) \
     ((t) = dyntypes_hmget_key_wrapper((t), sizeof *(t), (void*) (k), sizeof (*(t))->key, DYNTYPES_HM_PTR_TO_STRING), \
      dyntypes_temp((t)-1))

#define dyntypes_shgetp(t, k) \
    ((void) dyntypes_shgeti(t,k), &(t)[dyntypes_temp((t)-1)])

#define dyntypes_pshget(t, k) \
    ((void) dyntypes_pshgeti(t,k), (t)[dyntypes_temp((t)-1)])

#define dyntypes_shdel(t,k) \
    (((t) = dyntypes_hmdel_key_wrapper((t),sizeof *(t), (void*) (k), sizeof (t)->key, DYNTYPES_OFFSETOF((t),key), DYNTYPES_HM_STRING)),(t)?dyntypes_temp((t)-1):0)
#define dyntypes_pshdel(t,k) \
    (((t) = dyntypes_hmdel_key_wrapper((t),sizeof *(t), (void*) (k), sizeof (*(t))->key, DYNTYPES_OFFSETOF(*(t),key), DYNTYPES_HM_PTR_TO_STRING)),(t)?dyntypes_temp((t)-1):0)

#define dyntypes_sh_new_arena(t)  \
    ((t) = dyntypes_shmode_func_wrapper(t, sizeof *(t), DYNTYPES_SH_ARENA))
#define dyntypes_sh_new_strdup(t) \
    ((t) = dyntypes_shmode_func_wrapper(t, sizeof *(t), DYNTYPES_SH_STRDUP))

#define dyntypes_shdefault(t, v)  dyntypes_hmdefault(t,v)
#define dyntypes_shdefaults(t, s) dyntypes_hmdefaults(t,s)

#define dyntypes_shfree       dyntypes_hmfree
#define dyntypes_shlenu       dyntypes_hmlenu

#define dyntypes_shgets(t, k) (*dyntypes_shgetp(t,k))
#define dyntypes_shget(t, k)  (dyntypes_shgetp(t,k)->value)
#define dyntypes_shgetp_null(t,k)  (dyntypes_shgeti(t,k) == -1 ? NULL : &(t)[dyntypes_temp((t)-1)])
#define dyntypes_shlen        dyntypes_hmlen

typedef struct
{
  size_t      length;
  size_t      capacity;
  void      * hash_table;
  ptrdiff_t   temp;
} dyntypes_array_header;

typedef struct dyntypes_string_block
{
  struct dyntypes_string_block *next;
  char storage[8];
} dyntypes_string_block;

struct dyntypes_string_arena
{
  dyntypes_string_block *storage;
  size_t remaining;
  unsigned char block;
  unsigned char mode;  // this isn't used by the string arena itself
};

#define DYNTYPES_HM_BINARY         0
#define DYNTYPES_HM_STRING         1

enum
{
   DYNTYPES_SH_NONE,
   DYNTYPES_SH_DEFAULT,
   DYNTYPES_SH_STRDUP,
   DYNTYPES_SH_ARENA
};

#ifdef __cplusplus
// in C we use implicit assignment from these void*-returning functions to T*.
// in C++ these templates make the same code work
template<class T> static T * dyntypes_arrgrowf_wrapper(T *a, size_t elemsize, size_t addlen, size_t min_cap) {
  return (T*)dyntypes_arrgrowf((void *)a, elemsize, addlen, min_cap);
}
template<class T> static T * dyntypes_hmget_key_wrapper(T *a, size_t elemsize, void *key, size_t keysize, int mode) {
  return (T*)dyntypes_hmget_key((void*)a, elemsize, key, keysize, mode);
}
template<class T> static T * dyntypes_hmget_key_ts_wrapper(T *a, size_t elemsize, void *key, size_t keysize, ptrdiff_t *temp, int mode) {
  return (T*)dyntypes_hmget_key_ts((void*)a, elemsize, key, keysize, temp, mode);
}
template<class T> static T * dyntypes_hmput_default_wrapper(T *a, size_t elemsize) {
  return (T*)dyntypes_hmput_default((void *)a, elemsize);
}
template<class T> static T * dyntypes_hmput_key_wrapper(T *a, size_t elemsize, void *key, size_t keysize, int mode) {
  return (T*)dyntypes_hmput_key((void*)a, elemsize, key, keysize, mode);
}
template<class T> static T * dyntypes_hmdel_key_wrapper(T *a, size_t elemsize, void *key, size_t keysize, size_t keyoffset, int mode){
  return (T*)dyntypes_hmdel_key((void*)a, elemsize, key, keysize, keyoffset, mode);
}
template<class T> static T * dyntypes_shmode_func_wrapper(T *, size_t elemsize, int mode) {
  return (T*)dyntypes_shmode_func(elemsize, mode);
}
#else
#define dyntypes_arrgrowf_wrapper            dyntypes_arrgrowf
#define dyntypes_hmget_key_wrapper           dyntypes_hmget_key
#define dyntypes_hmget_key_ts_wrapper        dyntypes_hmget_key_ts
#define dyntypes_hmput_default_wrapper       dyntypes_hmput_default
#define dyntypes_hmput_key_wrapper           dyntypes_hmput_key
#define dyntypes_hmdel_key_wrapper           dyntypes_hmdel_key
#define dyntypes_shmode_func_wrapper(t,e,m)  dyntypes_shmode_func(e,m)
#endif

#endif
