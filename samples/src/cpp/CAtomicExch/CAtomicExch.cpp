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

#include "okra.h"
#include <iostream>
#include <string>
#include "utils.h"

using namespace std;

static const int NUMELEMENTS = 40;
bool *outArray = new bool[NUMELEMENTS * NUMELEMENTS];
int atomicInt = 0;  // initialize

static void aryset(int r, int c, bool val) {
  outArray[r * NUMELEMENTS + c] = val;
}

static bool aryget(int r, int c) {
  return outArray[r * NUMELEMENTS + c];
}

int main(int argc, char *argv[]) {
	// initialize outArray
	for (int r=0; r<NUMELEMENTS; r++) {
	  for (int c=0; c<NUMELEMENTS; c++) {
		aryset(r, c, false);
	  }
	}
	
	string sourceFileName = "CAtomicExch.hsail";
	char* hsailSource = buildStringFromSourceFile(sourceFileName);
	
	okra_status_t status;
      
        //create okra context
	okra_context_t* context = NULL;

        status = okra_get_context(&context);
        
	if (status != OKRA_SUCCESS) {cout << "Error while creating context:" << (int)status << endl; exit(-1);}

        //create kernel from hsail
        okra_kernel_t* kernel = NULL;
	
        status = okra_create_kernel(context, hsailSource, "&run", &kernel);

	if (status != OKRA_SUCCESS) {cout << "Error while creating kernel:" << (int)status << endl; exit(-1);}
	
        //setup kernel arguments
        okra_clear_args(kernel);
        okra_push_pointer(kernel, &atomicInt);
        okra_push_pointer(kernel, outArray);
        okra_push_int(kernel, NUMELEMENTS);

        //setup execution range
        okra_range_t range;
        range.dimension=1;
        range.global_size[0] = NUMELEMENTS - 1;
        range.global_size[1] = range.global_size[2] = 1;
        range.group_size[0] = NUMELEMENTS - 1;
        range.group_size[1] = range.group_size[2] = 1;
	
        //execute kernel and wait for completion
        status = okra_execute_kernel(context, kernel, &range);
        if(status != OKRA_SUCCESS) {cout << "Error while executing kernel:" << (int)status << endl; exit(-1);}	

	// make a fake link from the final object to the first object
	aryset(atomicInt, 0, true);

	bool passed = true;
	for (int r=0; r<NUMELEMENTS; r++) {
	  for (int c=0; c<NUMELEMENTS; c++) {
		cout << (aryget(r, c) ? '*' : '.');
	  }
	  cout << endl;
	}
	cout << endl;

	// check for 1 entry per row
	for (int r=0; r<NUMELEMENTS; r++) {
	  int count = 0;
	  for (int c=0; c<NUMELEMENTS; c++) {
		if (aryget(r, c)) {
		  count++;
		}
	  }
	  if (count != 1) {
		cout << "count for row " << r << " = " << count << endl;
		passed = false;
	  }
	}

	// check for 1 entry per col
	for (int c=0; c<NUMELEMENTS; c++) {
	  int count = 0;
	  for (int r=0; r<NUMELEMENTS; r++) {
		if (aryget(r, c)) {
		  count++;
		}
	  }
	  if (count != 1) {
		cout << "count for col " << c << " = " << count << endl;
		passed = false;
	  }
	}


	cout << endl << (passed ? "PASSED" : "FAILED") << endl;

        //dispose okra resources
		okra_kernel_dispose(kernel);
        okra_context_dispose(context);
	
	return 0;
}
