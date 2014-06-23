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

#ifndef OKRACONTEXT_H
#define OKRACONTEXT_H

#define __EXPORT__

#include "okra.h"
#include "cCommon.h"

// Abstract interface to an Okra Implementation
class OkraContext{
public:
	class Kernel {
	public:
		// various methods for setting different types of args into the arg stack
		virtual okra_status_t  pushFloatArg(jfloat) = 0;
		virtual okra_status_t  pushIntArg(jint) = 0;
		virtual okra_status_t  pushBooleanArg(jboolean) = 0;
		virtual okra_status_t  pushByteArg(jbyte) = 0;
		virtual okra_status_t  pushLongArg(jlong) = 0;
		virtual okra_status_t  pushDoubleArg(jdouble) = 0;
		virtual okra_status_t  pushPointerArg(void *addr) = 0;
		virtual okra_status_t  clearArgs() = 0;
		// allow a previously pushed arg to be changed
		virtual bool setPointerArg(int idx, void *addr) = 0;

		// setting number of dimensions and sizes of each
		virtual okra_status_t setLaunchAttributes(int dims, uint32_t *globalDims, uint32_t *localDims) = 0;

		// run a kernel and wait until complete
		virtual okra_status_t dispatchKernelWaitComplete(OkraContext* context) = 0;
              
                // dispose kernel
                virtual okra_status_t dispose() = 0;
	};

	// create a kernel object from the specified HSAIL text source and entrypoint
	virtual okra_status_t createKernel(const char *source, const char *entryName, Kernel ** kernel) = 0;

	// create a kernel object from the specified Brig binary source and entrypoint
	virtual okra_status_t createKernelFromBinary(const char *binary, size_t size, const char *entryName, Kernel** kernel) = 0;

        //dispose the context
        virtual okra_status_t dispose() = 0;

	void setVerbose(bool b) {verbose = b;}
	bool isVerbose() {return verbose;}

	static okra_status_t getContext(OkraContext** context);

	static bool isSimulator();

	static okra_status_t setCoherence(bool isCoherent);
	static bool getCoherence();

private:
    bool verbose;
    static OkraContext* m_pContext;
};


#endif // OKRACONTEXT_H
