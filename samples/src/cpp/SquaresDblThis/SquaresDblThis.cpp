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

/*************************
 * This example dispatches an hsail kernel that might be generated from
 * something like:
 *
 *       (gid) -> { outArray[gid] = inArray[gid] * inArray[gid] + adjustment; };
 *
 * where the lambda is a member lambda of a class containing the
 * inArray, outArray and adjustment fields
 *
 ******************/


static const int NUMELEMENTS = 40;

class SquaresDblThis {
private:
	double *inArray;
	double *outArray;
	double adjustment;

public:
	SquaresDblThis() {
		inArray = new double[NUMELEMENTS];
		outArray = new double[NUMELEMENTS];
	}

	void runTest() {
		// initialize inArray
		for (int i=0; i<NUMELEMENTS; i++) {
			inArray[i] = (double)i;
		}
	
		string sourceFileName = "SquaresDblThis.hsail";
		char* squaresSource = buildStringFromSourceFile(sourceFileName);
		
		OkraContext *context = OkraContext::Create();
		if (context == NULL) {cout << "...unable to create context\n"; exit(-1);}
		OkraContext::Kernel *kernel = context->createKernel(squaresSource, "&run");
		if (kernel == NULL) {cout << "...unable to create kernel\n"; exit(-1);}
		
		// these registerMemory calls might go away
		context->registerArrayMemory(inArray, NUMELEMENTS * sizeof(double));       // only needed first time or if moved
		context->registerArrayMemory(outArray,  NUMELEMENTS * sizeof(double));       // ditto
		context->registerArrayMemory(this, sizeof(*this));
		
		size_t globalDims[] = {NUMELEMENTS};  // # of workitems, would need more elements for 2D or 3D range
		size_t localDims[] = {NUMELEMENTS}; 
		kernel->setLaunchAttributes(1, globalDims, localDims);  // 1 dimension

		// set up the single "this" arg (once)
		kernel->clearArgs();
		kernel->pushPointerArg(this);

		// then call with different values of "adjustment" field
		for (int j=1; j<=3; j++) {
			adjustment = j* 0.123;
			kernel->dispatchKernelWaitComplete();
			
			bool passed = true;
			for (int i=0; i<NUMELEMENTS; i++) {
				cout << i << "->" << outArray[i] << ",  ";
				if (outArray[i] != i*i + adjustment) passed = false;
			}
			cout << endl << (passed ? "PASSED" : "FAILED") << endl;
		}
	}
};
	
int main(int argc, char *argv[]) {
	(new SquaresDblThis())->runTest();
	return 0;
}
