
#ifndef __TOOLSET_H__
#define __TOOLSET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern uint32_t calc_auth_key(char*);

extern int get_cpu_cores(void);


#ifdef __cplusplus
}
#endif

#endif /* __TOOLSET_H__*/

