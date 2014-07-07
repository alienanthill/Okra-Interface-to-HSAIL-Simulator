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
#include <iostream>
#include <string>
#include "utils.h"

using namespace std;

static const int NUMELEMENTS = 80;
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
	
	string sourceFileName = "AtomicExch.hsail";
	char* squaresSource = buildStringFromSourceFile(sourceFileName);
	
	OkraContext *context = OkraContext::Create();
	if (context == NULL) {cout << "...unable to create context\n"; exit(-1);}
	OkraContext::Kernel *kernel = context->createKernel(squaresSource, "&run");
	if (kernel == NULL) {cout << "...unable to create kernel\n"; exit(-1);}
	
	// register calls may go away
	context->registerArrayMemory(outArray,  NUMELEMENTS * NUMELEMENTS * sizeof(bool));       // ditto
	context->registerArrayMemory(&atomicInt,  sizeof(int));       // ditto
	
	kernel->clearArgs();
	kernel->pushPointerArg(&atomicInt);
	kernel->pushPointerArg(outArray);
	kernel->pushIntArg(NUMELEMENTS);
	
	size_t globalDims[] = {NUMELEMENTS - 1}; 
	size_t localDims[] = {NUMELEMENTS - 1}; 
	kernel->setLaunchAttributes(1, globalDims, localDims);  // 1 dimension
	
	kernel->dispatchKernelWaitComplete();

	// make a fake link from the final object to the first object
	aryset(atomicInt, 0, true);

	bool passed = true;
	if (false) {
	  for (int r=0; r<NUMELEMENTS; r++) {
		for (int c=0; c<NUMELEMENTS; c++) {
		  cout << (aryget(r, c) ? '*' : '.');
		}
		cout << endl;
	  }
	  cout << endl;
	}

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
	
	return 0;
}
