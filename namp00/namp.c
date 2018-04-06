
#include "namp.h"

#include <optimsoc-baremetal.h>
#include <string.h>

static int _skip_copy = 0;
void skip_copy(int set) {
  _skip_copy = set;
}

void *memalign(size_t alignment, size_t size);

void mp_cbuffer_new(struct mp_cbuffer **buf) {
  struct mp_cbuffer *b = memalign(32,32);
  b->wr_idx = 0;
  b->rd_idx = 0;
  b->capacity = BUFFERSIZE;
  b->max_msg_size = 12;
  b->size = calloc(sizeof(uint32_t),(1<<b->capacity));

  b->data = malloc((1<<b->max_msg_size)*(1<<b->capacity));

  *buf = b;
}

int mp_cbuffer_reserve(struct mp_cbuffer *buf, uint32_t *idx) {
  uint32_t cur_idx, new_idx, rd_idx;

  do {
    cur_idx = buf->wr_idx;
    *idx = cur_idx % (1<<buf->capacity);
    rd_idx = buf->rd_idx;
    if ((rd_idx != cur_idx) && (*idx == (rd_idx % (1 << buf->capacity)))) return -1; // full
    new_idx = cur_idx + 1;
    if (buf->size[*idx] != 0) return -1; // full
  } while (or1k_sync_cas((uint32_t*) &buf->wr_idx, cur_idx, new_idx) != cur_idx);
  return 0;
}

void mp_cbuffer_commit(struct mp_cbuffer *buf, uint32_t idx, uint32_t size) {
  buf->size[idx] = size;
}

int mp_cbuffer_push(struct mp_cbuffer *buf, uint8_t *data,
		     uint32_t size, uint32_t *idx) {
  if (mp_cbuffer_reserve(buf, idx) != 0) return -1;
  memcpy(buf->data + *idx * (1 << buf->max_msg_size), data, size);
  mp_cbuffer_commit(buf, *idx, size);
  return 0;
}

int mp_cbuffer_read(struct mp_cbuffer *buf, uint32_t *idx) {
  uint32_t cur_idx, new_idx;
  do {
    cur_idx = buf->rd_idx;
    new_idx  = cur_idx + 1;
    *idx = cur_idx % (1 << buf->capacity);
    if (buf->size[*idx]==0) return -1; // empty
  } while(or1k_sync_cas((uint32_t*) &buf->rd_idx, cur_idx, new_idx) != cur_idx);
  return 0;
}

void mp_cbuffer_free(struct mp_cbuffer *buf, uint32_t idx) {
  buf->size[idx] = 0;
}

int mp_cbuffer_pop(struct mp_cbuffer *buf, uint8_t *data, uint32_t *size) {
  uint32_t idx;
  if (mp_cbuffer_read(buf, &idx) != 0) return -1;
  *size = buf->size[idx];
  if (_skip_copy == 0) {
    memcpy(data, buf->data + idx*(1 << buf->max_msg_size), *size);
  }
  mp_cbuffer_free(buf, idx);
  return 0;
}

void endpoint_init(struct endpoint_local *ep, uint8_t buffered) {
  mp_cbuffer_new(&ep->rbuf);
  if (buffered != 0) {
    mp_cbuffer_new(&ep->sbuf);
  } else {
    ep->sbuf = 0;
  }
}

void endpoint_send(struct endpoint_local *lep, struct endpoint_remote *rep, uint8_t *data, uint32_t size, uint8_t namp_slot) {
  uint32_t idx;
  OPTIMSOC_TRACE(0x400, namp_slot);

  uint32_t base_addr = 0xe0200000 | (namp_slot << 14);

  if (lep->sbuf) {
    while(mp_cbuffer_push(lep->sbuf, data, size, &idx) != 0);
    REG32(base_addr) = NAMP_TRIGGER;
  } else {
    while ((REG32(base_addr) & NAMP_DONE) == 0) {}

    OPTIMSOC_TRACE(0x401, namp_slot);

    REG32(base_addr + 4) = (uint32_t) rep;
    REG32(base_addr + 8) = (uint32_t) data;
    REG32(base_addr + 12) = size;
    if (lep->channel) {
      REG32(base_addr) = NAMP_CON | NAMP_COM | NAMP_VALID;
    } else {
      REG32(base_addr) = NAMP_COM | NAMP_VALID;
    }
  }

  OPTIMSOC_TRACE(0x402, namp_slot);
}

void endpoint_receive(struct endpoint_local *ep, uint8_t *data, uint32_t *size, uint8_t namp_slot) {
  OPTIMSOC_TRACE(0x410, ep);
  while (mp_cbuffer_pop(ep->rbuf, data, size) != 0) {}

  if (ep->channel) {
    uint32_t base_addr = 0xe0200000 | (namp_slot << 14);
    while ((REG32(base_addr) & NAMP_DONE) == 0) {}
    REG32(base_addr + 4) = (uint32_t) ep->channel;
    REG32(base_addr + 8) = (uint32_t) 1;
    REG32(base_addr) = NAMP_VALID;
  }
  OPTIMSOC_TRACE(0x411, *size);
}

void setup_ccm(struct endpoint_local *lep, struct endpoint_remote *rep, uint8_t namp_slot) {
  if (!lep->sbuf) {
    mp_cbuffer_new(&lep->sbuf);
  }

  uint32_t base_addr = 0xe0200000 | (namp_slot << 14);
  REG32(base_addr + 4) = (uint32_t) rep;
  REG32(base_addr + 8) = (uint32_t) lep->sbuf;
  if (lep->channel) {
    REG32(base_addr) = NAMP_CCM | NAMP_BUF | NAMP_CON | NAMP_COM | NAMP_VALID;
  } else {
    REG32(base_addr) = NAMP_CCM | NAMP_BUF | NAMP_COM | NAMP_VALID;
  }
}
