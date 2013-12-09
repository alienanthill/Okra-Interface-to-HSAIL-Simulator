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

#ifndef OBJBUFFER_H
#define OBJBUFFER_H
#include "common.h"

class ObjBuffer{
   public:
      jobject javaObj;        // The java obj that this arg is mapped to 
      int  arg_idx;             // its position in the arg stack

	ObjBuffer(jobject _javaObj, int _idx, JNIEnv *_jenv):
		javaObj(_javaObj),
		arg_idx(_idx) {
   }

	void dispose(JNIEnv* _jenv) {
		if (javaObj) {
         //cout << "deleted weak ref " << javaObj <<endl;
			_jenv->DeleteWeakGlobalRef(javaObj);
         javaObj = NULL;
		}
	}
	~ObjBuffer() {
		if (javaObj) {
         cerr << "failed to call ObjBuffer.dispose" << javaObj <<endl;
		}
   }

};

#endif // OBJBUFFER_H
