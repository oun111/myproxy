#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include "dbug.h"

static int do_sha1_digest(uint8_t *digest, ...)
{
  va_list args;
  SHA_CTX ctx;
  const uint8_t *str;

  if (!SHA1_Init(&ctx)) {
    printd("can't init sha\n");
    return -1;
  }
  va_start(args, digest);
  for (str= va_arg(args, const uint8_t*); str; 
     str= va_arg(args, const uint8_t*)) {
    SHA1_Update(&ctx, str, va_arg(args, size_t));
  }
  va_end(args);
  SHA1_Final(digest,&ctx);
  OPENSSL_cleanse(&ctx,sizeof(ctx));
  return 0;
}

/* copy from $(mysql source)/sql/password.c */
static void do_encrypt(uint8_t *to, const uint8_t *s1, 
  const uint8_t *s2, uint8_t len)
{
  const uint8_t *s1_end= s1 + len;

  while (s1 < s1_end)
    *to++= *s1++ ^ *s2++;
}

int gen_auth_data(char *scramble, size_t sc_len, 
  const char *passwd, uint8_t *out)
{
  uint8_t hash_stage1[SHA_DIGEST_LENGTH],
    hash_stage2[SHA_DIGEST_LENGTH];

  /* generate hash on password */
  if (do_sha1_digest(hash_stage1,passwd,strlen(passwd),0)) {
    printd("digest on password #1 failed\n");
    return -1;
  }
  if (do_sha1_digest(hash_stage2,hash_stage1,SHA_DIGEST_LENGTH,0)) {
    printd("digest on password #2 failed\n");
    return -1;
  }
  /* generate hash on 'password' + 'auth data' */
  if (do_sha1_digest(out,scramble,sc_len,
     hash_stage2,SHA_DIGEST_LENGTH,0)) {
    printd("digest on password failed\n");
    return -1;
  }
  /* encrypt the 'out' buffer */
  do_encrypt(out,out,hash_stage1,SHA_DIGEST_LENGTH);
  return 0;
}

uint32_t str2id(char *s)
{
  uint32_t ret=0;
  uint16_t i=0;
  char *p = s;

  if (!p) 
    return 0;
  for (;p&&*p;p++) 
    ret += (*p^i) ;
  ret ^= (32&strlen(s)) ; 
  return ret ;
}

