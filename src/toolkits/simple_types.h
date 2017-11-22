
#ifndef __SIMPLE_TYPES_H__
#define __SIMPLE_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif


extern void double8store(double v, char out[8]);

/* big endian version */
extern void double8store_be(double v, char out[8]);

extern void float4store(float v, char out[4]);

extern void float4store_be(float v, char out[4]);

extern void ul2store(uint16_t v, char out[2]);

/* big-endian storage of ul2store */
extern void ul2store_be(uint16_t v, char out[2]);

extern void ul3store(uint32_t v, char out[3]);

extern void ul4store(uint32_t v, char out[4]);

/* big-endian storage of ul4store */
extern void ul4store_be(uint32_t v, char out[4]);

/* big-endian storage of ul8store */
extern void ul8store_be(uint64_t v, char out[8]);

/* convert a 4-byte-array into a long int */
extern uint32_t byte4_2_ul(char b[4]);

/* convert a 3-byte-array into a long int */
extern uint32_t byte3_2_ul(char b[3]);

/* convert a 2-byte-array into a long int */
extern uint32_t byte2_2_ul(char b[2]);

/* convert a 8-byte-array into a long long int */
extern uint64_t byte8_2_ul(char b[8]);

/* convert a 4-byte-array into a float value */
extern float byte4_2_float(char b[4]);

/* convert a 8-byte-array into a double value */
extern double byte8_2_double(char b[8]);
extern double byte8_2_double_be(char b[8]);

/* search to string ending */
extern char* strend(char*);

/* parse length-encode int */
extern uint32_t lenenc_int_get(char**);

/* get size of size bytes of length-encode int */
extern uint32_t lenenc_int_size_get(char*);

/* writes length-encode int */
extern int lenenc_int_set(uint64_t, char*);

/* writes length-encode string */
extern int lenenc_str_set(char*, char*);

/* read length-encode string */
extern char* lenenc_str_get(char*, char*, int);
extern size_t lenenc_str_size_get(char *in);
extern size_t get_str_lenenc_size(char *in);

extern void ul2ipv4(char*, uint32_t);

extern char* to_lower(char*);

#ifdef __cplusplus
}
#endif

#endif /* __SIMPLE_TYPES_H__*/

