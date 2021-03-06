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

package com.amd.okra.sample.reftest;

import com.amd.okra.OkraUtil;
import com.amd.okra.OkraContext;
import java.util.Random;

class Main {
	static class TestObj {
		public TestObj(int x) {
			this.x = x;
		}
		public int x;
	}

	static final int NUMHANDLES = 3;
	static long[] refHandles = new long[NUMHANDLES];
	static OkraContext context;


	static void testRefHandles() {
		int idx = 0;
		for (long refHandle : refHandles) {
			System.out.println("test for refHandle[" + idx++ + "]= " + Long.toHexString(refHandle));
			context.useRefHandle(refHandle);
		}
	}
		

	public static void main(String[] _args) {
		
		Random rand = new Random(0); 
		final int HOLDERSIZE = 1000;
		TestObj[] holder = new TestObj[HOLDERSIZE];
		TestObj testobj = new TestObj(0x12345678);

		if (!OkraUtil.okraLibExists()) {
			System.out.println("...unable to create context, using fake references");
			for (int i=0; i<4; i++) {
				System.out.println("Fake Reference " + OkraUtil.getRefHandle(testobj));
			}
			System.exit(0);		
		}

		context = new OkraContext();
		context.setVerbose(true);
		for (int i=0; i<3; i++) {
			refHandles[i] = OkraUtil.getRefHandle(new TestObj(0x12345678+i));
		}
	
		testRefHandles();

		for (int i=0; i<3; i++) {
			for (int j=0; j<100000; j++) {
				holder[rand.nextInt(HOLDERSIZE)] = new TestObj(j);
			}
			System.gc();
			testRefHandles();
		}
	}
}
