#include <stdint.h>
#include <stdlib.h>

#define NAMP_DONE  0x1
#define NAMP_VALID 0x2
#define NAMP_COM   0x4
#define NAMP_CON   0x8
#define NAMP_BUF   0x10
#define NAMP_CCM   0x20
#define NAMP_TRIGGER 0x80000000

struct mp_cbuffer {
  uint32_t capacity;
  uint32_t max_msg_size;
  volatile uint32_t wr_idx;
  volatile uint32_t rd_idx;
  uint8_t *data;
  uint32_t *size;
};

void mp_cbuffer_new(struct mp_cbuffer **buf);

int mp_cbuffer_reserve(struct mp_cbuffer *buf, uint32_t *idx);

void mp_cbuffer_commit(struct mp_cbuffer *cbuf, uint32_t idx, uint32_t size);

int mp_cbuffer_push(struct mp_cbuffer *buf, uint8_t *data,
		    uint32_t size, uint32_t *idx);

int mp_cbuffer_read(struct mp_cbuffer *buf, uint32_t *idx);

void mp_cbuffer_free(struct mp_cbuffer *buf, uint32_t idx);

int mp_cbuffer_pop(struct mp_cbuffer *buf, uint8_t *data, uint32_t *size);

struct endpoint_local;
struct endpoint_remote;

struct endpoint_local {
  uint32_t domain;
  uint32_t node;
  uint32_t port;
  struct mp_cbuffer *sbuf;
  struct mp_cbuffer *rbuf;
  struct endpoint_remote *channel;
} __attribute__ ((aligned (64)));

struct endpoint_remote {
  uint32_t domain;
  uint32_t node;
  uint32_t port;
  uint32_t addr;
  uint32_t credit;
  uint32_t idx;
  uint32_t capacity;
  uint32_t buffer;
} __attribute__ ((aligned (32)));

void endpoint_init(struct endpoint_local *ep, uint8_t buffered);

void endpoint_send(struct endpoint_local *lep, struct endpoint_remote *rep, uint8_t *data, uint32_t size, uint8_t namp_slot);
void endpoint_receive(struct endpoint_local *ep, uint8_t *data, uint32_t *size, uint8_t namp_slot);

void setup_ccm(struct endpoint_local *lep, struct endpoint_remote *rep, uint8_t namp_slot);

void skip_copy(int set);
