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

#ifndef SIZE
#define SIZE 3
#endif

#ifndef SLOTS
#define SLOTS 4
#endif

struct endpoint_local ep_local;
struct endpoint_remote ep_remote[SLOTS];

int main() {
  uint32_t size;

  optimsoc_init(0);

  endpoint_init(&ep_local, 0);

  for (size_t i = 0; i < 32; i++) {
    data0[i] = i & 0xff;
  }

  uint32_t tiles = optimsoc_get_numct();

  if (optimsoc_get_ctrank() == 0) {
    for (int i = 0; i < SLOTS; i++) {
      ep_remote[i].domain = 1 + i;
      ep_remote[i].addr = (uint32_t) &ep_local;
      ep_remote[i].credit = 1 << ep_local.rbuf->capacity;
      ep_remote[i].idx = 0;
      ep_remote[i].capacity = ep_local.rbuf->capacity;
      ep_remote[i].buffer = (uint32_t) ep_local.rbuf->data;
    }

    for (int j = 0; j < 25; j++) {
      for (int i = 0; i < SLOTS; i++) {
	endpoint_send(&ep_local, &ep_remote[i], data0, 1<<SIZE, i);
      }
    }
  } else if (optimsoc_get_ctrank() < (1+SLOTS)) {
    ep_remote[0].domain = 0;
    ep_remote[0].addr = (uint32_t) &ep_remote;

    for (int i = 0; i < 25; i++) {
      size = 1024;
      endpoint_receive(&ep_local, data0, &size, 0);
    }
  }

  return 0;
}
