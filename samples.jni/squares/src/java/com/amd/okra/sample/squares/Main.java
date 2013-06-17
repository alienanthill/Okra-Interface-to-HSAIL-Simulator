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

package com.amd.okra.sample.squares;

import com.amd.okra.OkraContext;
import com.amd.okra.OkraKernel;
import java.nio.file.Files;
import java.nio.file.FileSystems;
import java.io.IOException;

class Main {
	public static void main(String[] _args) {

		final int NUMELEMENTS = 40;
		float[] inArray = new float[NUMELEMENTS];
		float[] outArray = new float[NUMELEMENTS];

		// initialize inArray
		for (int i=0; i<NUMELEMENTS; i++) {
			inArray[i] = (float)i;
		}
	
		String sourceFileName = "Squares.hsail";
		String squaresSource = null;
		try {
			squaresSource = new String(Files.readAllBytes(FileSystems.getDefault().getPath( sourceFileName)));
		}
		catch(IOException e) {
			e.printStackTrace();
			System.exit(-1);
		}

		OkraContext context = new OkraContext();
		if (!context.isValid()) {System.out.println("...unable to create context"); System.exit(-1);}
		context.setVerbose(true);
		OkraKernel kernel = new OkraKernel(context, squaresSource, "&run");
		if (!kernel.isValid()) {System.out.println("...unable to create kernel"); System.exit(-1);}
		
		// there is currently only one list of arguments maintained at a time
		kernel.clearArgs();
		kernel.pushFloatArrayArg(outArray);
		kernel.pushFloatArrayArg(inArray);

		kernel.setLaunchAttributes(NUMELEMENTS); // 1 dimension

		kernel.dispatchKernelWaitComplete();

		for (int i=0; i<NUMELEMENTS; i++) {
			System.out.print(i + "->" +  outArray[i] + ",  ");
		}
		System.out.println("\n");
	}
}
