// University of Illinois/NCSA
// Open Source License
// 
// Copyright (c) 2013, Advanced Micro Devices, Inc.
// All rights reserved.
// 
// Developed by:
// 
//     Runtimes Team
// 
//     Advanced Micro Devices, Inc
// 
//     www.amd.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal with
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
// 
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimers.
// 
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimers in the
//       documentation and/or other materials provided with the distribution.
// 
//     * Neither the names of the LLVM Team, University of Illinois at
//       Urbana-Champaign, nor the names of its contributors may be used to
//       endorse or promote products derived from this Software without specific
//       prior written permission.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
// SOFTWARE.
//===----------------------------------------------------------------------===//

#include "okraContext.h"
#include <stdio.h>

okra_status_t OKRA_API okra_get_context(okra_context_t** context) {
    okra_status_t status = OkraContext::getContext((OkraContext**)context);
    return status;
}

okra_status_t OKRA_API okra_create_kernel(okra_context_t* context, 
                        const char *hsail_source, const char *entryName, 
                        okra_kernel_t **kernel) {
    OkraContext* ctx = (OkraContext*) context;
    if(!ctx) return OKRA_INVALID_ARGUMENT; 
    okra_status_t status = ctx->createKernel(hsail_source, entryName, 
                                                    (OkraContext::Kernel**)kernel);
    return status;
}

okra_status_t OKRA_API okra_create_kernel_from_binary(okra_context_t *context, 
                        const char *binary, size_t size, const char *entryName,
                        okra_kernel_t **kernel) {
    OkraContext* ctx = (OkraContext*) context;
    if(!ctx) return OKRA_INVALID_ARGUMENT; 
    okra_status_t status = ctx->createKernelFromBinary(binary, size, entryName, 
                                                      (OkraContext::Kernel**)kernel);
    return status;
}

okra_status_t OKRA_API okra_push_pointer(okra_kernel_t* kernel, 
                        void* address) {
    OkraContext::Kernel* realKernel = (OkraContext::Kernel*) kernel;
    if(!realKernel) return OKRA_INVALID_ARGUMENT;
    okra_status_t status = realKernel->pushPointerArg(address);
    return status;
}

okra_status_t OKRA_API okra_push_boolean(okra_kernel_t* kernel, unsigned char val) {
    OkraContext::Kernel* realKernel = (OkraContext::Kernel*) kernel;
    if(!realKernel) return OKRA_INVALID_ARGUMENT;
    okra_status_t status = realKernel->pushBooleanArg(val);
    return status;
}

okra_status_t OKRA_API okra_push_byte(okra_kernel_t* kernel, char val) {
    OkraContext::Kernel* realKernel = (OkraContext::Kernel*) kernel;
    if(!realKernel) return OKRA_INVALID_ARGUMENT;
    okra_status_t status = realKernel->pushByteArg(val);
    return status;
}

okra_status_t OKRA_API okra_push_double(okra_kernel_t* kernel, double val) {
    OkraContext::Kernel* realKernel = (OkraContext::Kernel*) kernel;
    if(!realKernel) return OKRA_INVALID_ARGUMENT;
    okra_status_t status = realKernel->pushDoubleArg(val);
    return status;
}

okra_status_t OKRA_API okra_push_float(okra_kernel_t* kernel, float val) {
    OkraContext::Kernel* realKernel = (OkraContext::Kernel*) kernel;
    if(!realKernel) return OKRA_INVALID_ARGUMENT;
    okra_status_t status = realKernel->pushFloatArg(val);
    return status;
}

okra_status_t OKRA_API okra_push_int(okra_kernel_t* kernel, int val) {
    OkraContext::Kernel* realKernel = (OkraContext::Kernel*) kernel;
    if(!realKernel) return OKRA_INVALID_ARGUMENT;
    okra_status_t status = realKernel->pushIntArg(val);
    return status;
}

okra_status_t OKRA_API okra_push_long(okra_kernel_t* kernel, long val) {
    OkraContext::Kernel* realKernel = (OkraContext::Kernel*) kernel;
    if(!realKernel) return OKRA_INVALID_ARGUMENT;
    okra_status_t status = realKernel->pushLongArg(val);
    return status;
}

// Call clearargs between executions of a kernel before setting the new args
okra_status_t OKRA_API okra_clear_args(okra_kernel_t* kernel) {
    OkraContext::Kernel* realKernel = (OkraContext::Kernel*) kernel;
    if(!realKernel) return OKRA_INVALID_ARGUMENT;
    okra_status_t status = realKernel->clearArgs();
    return status;
}

okra_status_t OKRA_API okra_execute_kernel(okra_context_t* context, okra_kernel_t* kernel,  
                                                                      okra_range_t* range) {

    OkraContext* ctx = (OkraContext*) context;    
    OkraContext::Kernel* realKernel = (OkraContext::Kernel*) kernel;
    if(!ctx || !realKernel || !range) return OKRA_INVALID_ARGUMENT;

    if(range->dimension < 1 || range->dimension > 3) return OKRA_RANGE_INVALID_DIMENSION;
    
    int dimension = range->dimension;
    uint32_t *globalDims = range->global_size;
    uint32_t *localDims = range->group_size;
    
    okra_status_t status = realKernel->setLaunchAttributes(dimension, globalDims, localDims);
    
    if(status != OKRA_SUCCESS)
       return status;
    
    status = realKernel->dispatchKernelWaitComplete(ctx);
    
    return status;
}

okra_status_t OKRA_API okra_kernel_dispose(okra_kernel_t* kernel) {

    if(!kernel) return OKRA_INVALID_ARGUMENT;
    OkraContext::Kernel* realKernel = (OkraContext::Kernel*) kernel;
    
    realKernel->dispose();
    kernel = NULL;
   
    return OKRA_SUCCESS;

}

okra_status_t OKRA_API okra_context_dispose(okra_context_t* context) {

    if(!context) return OKRA_INVALID_ARGUMENT;
    OkraContext* ctx = (OkraContext*) context;
    
    ctx->dispose();
    context = NULL;
   
    return OKRA_SUCCESS;

}


