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

#include <iostream>
#include <string>
#include "utils.h"
#include <dlfcn.h>


/*************************
 * This example shows use of the C interface functions in Okra to dynamically
 * load libokra, look up the functions and use them to run a kernel.
 * 
 * This example is reusing the kernel code from Squares, an hsail kernel that 
 * might be generated from something like:
 *
 *       (gid) -> { outArray[gid] = inArray[gid] * inArray[gid]; };
 *
 * where the lambda is a static lambda and inArray, outArray are
 * captures passed in to that static lambda.
 *
 ******************/


static const int NUMELEMENTS = 40;
float *inArray = new float[NUMELEMENTS];
float *outArray = new float[NUMELEMENTS];

typedef void* (*okra_ctx_create_func_t)();
typedef void* (*okra_kernel_create_func_t)(void*, const char *, const char *);
typedef bool (*okra_push_object_func_t)(void*, void*);
typedef bool (*okra_execute_with_range_func_t)(void*, int);
typedef bool (*okra_clearargs_func_t)(void*);

okra_ctx_create_func_t      _okra_ctx_create;
okra_kernel_create_func_t   _okra_kernel_create;
okra_push_object_func_t    _okra_push_object;
okra_execute_with_range_func_t    _okra_execute_with_range;
okra_clearargs_func_t       _okra_clearargs;

int main(int argc, char *argv[]) {
	// initialize inArray
	for (int i=0; i<NUMELEMENTS; i++) {
		inArray[i] = (float)i;
	}
	
        void* okraLib = dlopen("libokra_x86_64.so", RTLD_LAZY);
        
        if (okraLib == NULL) {
            cout << "...unable to load libokra_x86_64.so\n";
            exit(-1);
        }
        
        _okra_ctx_create          = (okra_ctx_create_func_t)dlsym(okraLib, "okra_create_context");
        _okra_kernel_create       = (okra_kernel_create_func_t)dlsym(okraLib, "okra_create_kernel");
        _okra_push_object         = (okra_push_object_func_t)dlsym(okraLib, "okra_push_object");
        _okra_execute_with_range  = (okra_execute_with_range_func_t)dlsym(okraLib, "okra_execute_with_range");
        _okra_clearargs           = (okra_clearargs_func_t)dlsym(okraLib, "okra_clearargs");

	string sourceFileName = "Squares.hsail";
	char* squaresSource = buildStringFromSourceFile(sourceFileName);

	void *context = _okra_ctx_create();
	if (context == NULL) {cout << "...unable to create context\n"; exit(-1);}
	void *kernel = _okra_kernel_create(context, squaresSource, (const char *) "&run");
	if (kernel == NULL) {cout << "...unable to create kernel\n"; exit(-1);}

	_okra_clearargs(kernel);
	_okra_push_object(kernel, outArray);
	_okra_push_object(kernel, inArray);

        bool success = _okra_execute_with_range(kernel, NUMELEMENTS);
        if (success == false) {
            cout << "Failed to run kernel\n";
            exit(-1);
        }
	bool passed = true;
	for (int i=0; i<NUMELEMENTS; i++) {
		cout << i << "->" << outArray[i] << ",  ";
		if (outArray[i] != i*i) passed = false;
	}
	
	cout << endl << (passed ? "PASSED" : "FAILED") << endl;
	
	return 0;
}
