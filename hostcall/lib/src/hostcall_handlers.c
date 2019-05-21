 
/*   
 *   hostcall_handlers.c:  These are the services for the hostcall system
 *
 *   Written by Greg Rodgers

MIT License

Copyright © 2019 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


#include <stdio.h>
#include "atmi_hostcall.h"
#include "atmi_runtime.h"

typedef struct atl_hcq_element_s atl_hcq_element_t;
struct atl_hcq_element_s {
  hostcall_buffer_t *   hcb;
  hostcall_consumer_t * consumer;
  hsa_queue_t *       	hsa_q;
  atl_hcq_element_t *   next_ptr;
  uint32_t              device_id;
};

//  Persistent static values for the hcq linked list 
atl_hcq_element_t * atl_hcq_front;
atl_hcq_element_t * atl_hcq_rear;
int atl_hcq_count;
*/

#include <stdlib.h>
#include "hsa/hsa_ext_amd.h"
#include "hostcall.h"
#include "atmi_runtime.h"
#include "hostcall_service_id.h"
#include "hostcall_internal.h"
 
void handler_HOSTCALL_SERVICE_PRINTF(void *cbdata, uint32_t service, uint64_t *payload) {
    size_t bufsz          = (size_t) payload[0];
    char* device_buffer   = (char*) payload[1];
    char* host_buffer     = (char*) malloc(bufsz);
    atmi_status_t copyerr = atmi_memcpy(host_buffer, device_buffer,  bufsz);
    if (copyerr != ATMI_STATUS_SUCCESS) {
       payload[0] = (uint64_t) copyerr;
       return;
    }
    hostcall_error_t err = hostcall_printf(host_buffer,bufsz);
    payload[0] = (uint64_t) err;
    free(host_buffer);
    atmi_free(device_buffer);
}

void handler_HOSTCALL_SERVICE_MALLOC(void *cbdata, uint32_t service, uint64_t *payload) {
    uint32_t device_id;
    atl_hcq_element_t * llq_elem = (atl_hcq_element_t *) cbdata;
    if (llq_elem)
       device_id = llq_elem->device_id;
    else
       device_id = 0 ; // This really should be an error
    void *ptr = NULL;
    atmi_mem_place_t place = ATMI_MEM_PLACE_GPU_MEM(0,device_id,0);
    atmi_status_t err = atmi_malloc(&ptr, payload[0], place);
    payload[0] = (uint64_t) err;
    payload[1] = (uint64_t) ptr;
}

// Stubs will not typically use the free service because, if needed, data is typically freed
// in the actual service. 
void handler_HOSTCALL_SERVICE_FREE(void *cbdata, uint32_t service, uint64_t *payload) {
    char* device_buffer   = (char*) payload[1];
    atmi_free(device_buffer);
}

int vector_product_zeros(int N, int*A, int*B, int*C) {
    int zeros = 0;
    for (int i =0 ; i<N; i++) {
       C[i] = A[i] * B[i];
       if ( C[i] == 0  )
          zeros++ ;
    }
    return zeros;
}

// This is the service for the demo of vector_product_zeros
void handler_HOSTCALL_SERVICE_DEMO(void *cbdata, uint32_t service, uint64_t *payload) {
   atmi_status_t copyerr ;
   int   N   = (int)  payload[0];
   int * A_D = (int*) payload[1];
   int * B_D = (int*) payload[2];
   int * C_D = (int*) payload[3];

   int * A = (int*)  malloc(N*sizeof(int) );
   int * B = (int*)  malloc(N*sizeof(int) );
   int * C = (int*)  malloc(N*sizeof(int) );
   copyerr = atmi_memcpy(A, A_D, N*sizeof(int));
   copyerr = atmi_memcpy(B, B_D, N*sizeof(int));

   int num_zeros = vector_product_zeros(N,A,B,C);
   copyerr = atmi_memcpy(C_D, C, N*sizeof(int));
   payload[0] = (uint64_t) copyerr;
   payload[1] = (uint64_t) num_zeros;
}

void hostcall_register_all_handlers(hostcall_consumer_t * c, void * cbdata) {
    hostcall_register_service(c,HOSTCALL_SERVICE_PRINTF, handler_HOSTCALL_SERVICE_PRINTF, cbdata);
    hostcall_register_service(c,HOSTCALL_SERVICE_MALLOC, handler_HOSTCALL_SERVICE_MALLOC, cbdata);
    hostcall_register_service(c,HOSTCALL_SERVICE_FREE, handler_HOSTCALL_SERVICE_FREE, cbdata);
    hostcall_register_service(c,HOSTCALL_SERVICE_DEMO, handler_HOSTCALL_SERVICE_DEMO, cbdata);
}
