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
#include <sstream>
#include "utils.h"
#include <math.h>

using namespace std;

/*************************
 * This example dispatches an hsail kernel that might be generated from
 * something like:
 *
 *       (gid) -> {
 *                  Point pt = inArray[gid];
 *                  float distance = sqrt(pt.x * pt.x + pt.y * pt.y + pt.z * pt.z);
 *                  outArray[gid] = distance;
 *                }
 *
 * in other words inArray is an array of references to Point objects.
 * In this example, the lambda is treated as static in that both inArray and outArray 
 * are passed to the resulting kernel.
 *
 ******************/


static const int NUMELEMENTS = 40;

static string showFloat(float f) {
	ostringstream buff;
	buff << f << " (" << hex << *(unsigned int *)&f << dec << ")";
	return buff.str();
}

class Point {
public:
	float x,y,z;
	Point (float _x, float _y, float _z) {
		x = _x;  y = _y;  z = _z;
	}
	// virtual float getx() {return x;}
	void show() {
		cout << hex
			 << "x=" << showFloat(x) << ") ," 
			 << "y=" << showFloat(y) << ") ," 
			 << "z=" << showFloat(z) << ")\n";
	}
};


Point **inArray = new Point *[NUMELEMENTS];
float *outArray = new float[NUMELEMENTS];



int main(int argc, char *argv[]) {
	string sourceFileName = "ooparray.hsail";
	char* sourceStr = buildStringFromSourceFile(sourceFileName);
	
	OkraContext *context = OkraContext::Create();
	OkraContext::Kernel *kernel = context->createKernel(sourceStr, "&run");
	
	context->registerArrayMemory(inArray, NUMELEMENTS * sizeof(Point *));       // only needed first time or if moved
	context->registerArrayMemory(outArray,  NUMELEMENTS * sizeof(float));       // ditto

	// initialize inArray
	for (int i=0; i<NUMELEMENTS; i++) {
		inArray[i] = new Point(i, i+1, i+2);
		context->registerArrayMemory(inArray[i],  sizeof(Point));   // hopefully this will go away
	}

	kernel->clearArgs();
	kernel->pushPointerArg(outArray);
	kernel->pushPointerArg(inArray);

	size_t globalDims[] = {NUMELEMENTS};  // # of workitems, would need more elements for 2D or 3D range
	size_t localDims[] = {NUMELEMENTS}; 
	kernel->setLaunchAttributes(1, globalDims, localDims);  // 1 dimension

	kernel->dispatchKernelWaitComplete();

	// show results
	bool passed = true;
	for (int i=0; i<NUMELEMENTS; i++) {
		cout << dec << i << "->" << showFloat(outArray[i]) << ",  ";
		if (i%5 == 4) cout << endl;
		float expected = sqrt((float)(i*i + (i+1)*(i+1) + (i+2)*(i+2)));
		if (outArray[i] != expected) {
			// cout << "saw " << outArray[i] << ", expected " << expected << endl;
			passed = false;
		}
	}
	cout << endl << (passed ? "PASSED" : "FAILED") << endl;
	
	return 0;
}
