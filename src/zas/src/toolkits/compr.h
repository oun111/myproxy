
#ifndef __COMPR_H__
#define __COMPR_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int do_compress(uint8_t*,int, uint8_t*, int *);

extern int do_uncompress(uint8_t*,int, uint8_t*,int*);

#ifdef __cplusplus
}
#endif

#endif /* __COMPR_H__*/
