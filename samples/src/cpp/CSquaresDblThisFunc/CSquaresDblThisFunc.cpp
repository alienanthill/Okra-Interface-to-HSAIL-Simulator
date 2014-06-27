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

/*************************
 * This example dispatches an hsail kernel that might be generated from
 * something like:
 *
 *       (gid) -> { outArray[gid] = squareWithAdjustment(inArray[gid]); };
 *
 *        double squareWithAdjustment(double val) {return val*val + adjustment; }
 *
 * where the lambda is a member lambda of a class containing the
 * inArray, outArray and adjustment fields and squareWithAdjustment is
 * also a member method of that class
 *
 ******************/


static const int NUMELEMENTS = 40;

class SquaresDblThisFunc {
private:
	double *inArray;
	double *outArray;
	double adjustment;

public:
	SquaresDblThisFunc() {
		inArray = new double[NUMELEMENTS];
		outArray = new double[NUMELEMENTS];
	}

	void runTest() {
		// initialize inArray
		for (int i=0; i<NUMELEMENTS; i++) {
			inArray[i] = (double)i;
		}
	
		string sourceFileName = "CSquaresDblThisFunc.hsail";
		char* squaresSource = buildStringFromSourceFile(sourceFileName);
                
                okra_status_t status;
      
		//create okra context
		okra_context_t* context = NULL;

		status = okra_get_context(&context);
		
		if (status != OKRA_SUCCESS) {cout << "Error while creating context:" << (int)status << endl; exit(-1);}

		//create kernel from hsail
		okra_kernel_t* kernel = NULL;
	
		status = okra_create_kernel(context, squaresSource, "&run", &kernel);

		if (status != OKRA_SUCCESS) {cout << "Error while creating kernel:" << (int)status << endl; exit(-1);}		
		
		//setup execution range
		okra_range_t range;
		range.dimension=1;
		range.global_size[0] = NUMELEMENTS;
		range.global_size[1] = range.global_size[2] = 1;
		range.group_size[0] = NUMELEMENTS;
		range.group_size[1] = range.group_size[2] = 1;

		// set up the single "this" arg (once)
		okra_clear_args(kernel);
                okra_push_pointer(kernel, this);

		// then call with different values of "adjustment" field
		for (int j=1; j<=3; j++) {
			adjustment = j* 0.123;
			//execute kernel and wait for completion
			status = okra_execute_kernel(context, kernel, &range);
			if(status != OKRA_SUCCESS) {cout << "Error while executing kernel:" << (int)status << endl; exit(-1);}
			
			bool passed = true;
			for (int i=0; i<NUMELEMENTS; i++) {
				cout << i << "->" << outArray[i] << ",  ";
				if (outArray[i] != i*i + adjustment) passed = false;
			}
			cout << endl << (passed ? "PASSED" : "FAILED") << endl;
		}
            //dispose okra resources
			okra_dispose_kernel(kernel);
	        okra_dispose_context(context);
	}
};
	
int main(int argc, char *argv[]) {
	(new SquaresDblThisFunc())->runTest();
	return 0;
}
