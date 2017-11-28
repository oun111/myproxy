
#ifndef __DUMMY_PROTO_H__
#define __DUMMY_PROTO_H__

/* dummy business protocol */
typedef struct tDummyBusinessRequest
{
  int type ; /* 0 */
  uint64_t timestamp ;
  char msg[30];
  size_t len ;
  int direction ;
  bool bConfirm ;  /* if a confirm packet is needed */
  uint32_t ossAddr;
  int ossPort;
  int sz_body ;
  int px,py ;
} tDummyBusiReq ;

typedef struct tDummyBusinessResponse
{
  int type ; /* 1 */
  uint64_t timestamp ;
  char msg[256];
  size_t len ;
  int direction ;
  int sz_body ;
} tDummyBusiResp ;

/* outstanding server protocol */
typedef struct tOutstandingSvrPacket
{
  int type ; /* 2 */
  uint64_t timestamp;
  uint32_t counter ;
  char msg[64];
} tOssPkt ;

#endif /* __DUMMY_PROTO_H__ */
