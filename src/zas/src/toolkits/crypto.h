
#ifndef __CRYPTO_H__
#define __CRYPTO_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int gen_auth_data(char*, size_t, 
  const char*, uint8_t*);

extern uint32_t str2id(char*);

#ifdef __cplusplus
}
#endif

#endif /* __CRYPTO_H__*/
