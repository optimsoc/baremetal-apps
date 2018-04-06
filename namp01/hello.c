/* Copyright (c) 2013-2015 by the author(s)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * =============================================================================
 *
 * Simple hello world example.
 *
 * Author(s):
 *   Stefan Wallentowitz <stefan.wallentowitz@tum.de>
 */

#include <stdio.h>
#include <optimsoc-baremetal.h>
#include "namp.h"

uint8_t data0[1024];
uint8_t data1[1024];

struct endpoint_local ep_local;
struct endpoint_remote ep_remote;

int main() {
  uint32_t size;

  optimsoc_init(0);

  endpoint_init(&ep_local, 0);

  for (size_t i = 0; i < 32; i++) {
    data0[i] = i & 0xff;
  }

  if (optimsoc_get_ctrank() == 1) {
    ep_remote.domain = 0;
    ep_remote.addr = (uint32_t) &ep_local;
    ep_remote.credit = 1 << ep_local.rbuf->capacity;
    ep_remote.idx = 0;
    ep_remote.capacity = ep_local.rbuf->capacity;
    ep_remote.buffer = (uint32_t) ep_local.rbuf->data;
    REG32(0xe0200004) = (uint32_t) &ep_remote;
    REG32(0xe0200008) = (uint32_t) ep_local.sbuf;
    REG32(0xe0200000) = NAMP_CCM | NAMP_BUF | NAMP_COM | NAMP_VALID;
    for (int i = 3; i < 8; i++) {
      for (int j = 0; j < 16; j++) {
	uint32_t idx;
	while (mp_cbuffer_push(ep_local.sbuf, data0, (1 << i), &idx) != 0) {}
      }
    }
  } else if (optimsoc_get_ctrank() == 0) {
    REG32(0xe0200004) = (uint32_t) &ep_remote;
    REG32(0xe0200008) = (uint32_t) 1;
    for (int i = 0; i < 80; i++) {
      size = 1024;
      while (mp_cbuffer_pop(ep_local.rbuf,data0,&size) != 0) {}
    }
  }

  return 0;
}
