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


#include <stdio.h> // For printf

#include <optimsoc-mp.h>
#include <or1k-support.h>
#include <optimsoc-baremetal.h>

#include <assert.h>

// The main function
int main() {
    optimsoc_mp_result_t ret;
    if (or1k_coreid() != 0)
        return 0;

    // Initialize optimsoc library
    optimsoc_init(0);
    ret = optimsoc_mp_initialize(0);
    if (ret != OPTIMSOC_MP_SUCCESS) {
      printf("initialization error\n");
    }

    optimsoc_trace_config_set(TRACE_ALL);
    optimsoc_mp_trace_config_set(TRACE_MP_ALL);

    // Determine tiles rank
    uint32_t rank = optimsoc_get_ctrank();

    optimsoc_mp_endpoint_handle ep;
    ret = optimsoc_mp_endpoint_create(&ep, optimsoc_get_tileid(), 0, 0, 2, 16, OPTIMSOC_MP_EP_DEFAULTS);
    if (ret != OPTIMSOC_MP_SUCCESS) {
      printf("Cannot create endpoint: %d\n", ret);
      return 1;
    }

    if (rank==0) {
      mgmt_print_db();
      while(1) {}
        size_t count = 0;
        while(count < (optimsoc_get_numct() - 1)) {
            uint32_t remote;
            size_t received;
            //optimsoc_mp_msg_recv(ep, (uint8_t*) &remote, 4, &received);
            printf("Received from %d\n", remote);
            count++;
        }
    } else {
        optimsoc_mp_endpoint_handle ep_remote;
        ret = optimsoc_mp_endpoint_get(&ep_remote, 0, 0, 0);
	if (ret != OPTIMSOC_MP_SUCCESS) {
	  printf("Cannot get remote endpoint: %d\n", ret);
	  return 1;
	}
	mgmt_print_db();

	ret = optimsoc_mp_msg_send(ep, ep_remote, (uint32_t*) &rank, sizeof(rank));
	if (ret != OPTIMSOC_MP_SUCCESS) {
	  printf("Cannot send message: %d\n", ret);
	  return 1;
	}
    }

    return 0;
}
