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

#ifndef ARRAYBUFFER_H
#define ARRAYBUFFER_H
#include "common.h"

class ArrayBuffer{
   public:
      jobject javaArray;        // The java array that this arg is mapped to 
      jint length;              // the number of elements for arrays (used only when ARRAYLENGTH bit is set for this arg)
      void *addr;               // the last address where we saw this java array object
      int  arg_idx;             // its position in the arg stack
	  int  elementSize;
      jboolean isCopy;
      jboolean isPinned;

	ArrayBuffer(jarray _ary, int _elementSize, int _idx, JNIEnv *_jenv):
		javaArray(_jenv->NewWeakGlobalRef(_ary)), 
		length(_jenv->GetArrayLength(_ary)),
		elementSize(_elementSize),
		addr(NULL),
		arg_idx(_idx),
		isCopy(false),
		isPinned(false){
   }

	void dispose(JNIEnv *_jenv) {
		if (javaArray) {
         //cout << "deleted weak ref " << javaArray <<endl;
			_jenv->DeleteWeakGlobalRef(javaArray); 
         javaArray = NULL;
		}
	}

	~ArrayBuffer() {
		if (javaArray) {
         cerr << "failed to dispose of ArrayBuff" << javaArray <<endl;
		}
	}

	void unpinAbort(JNIEnv *jenv){
		if (isPinned) {
         //cout << "unpinning abort " << addr << " " << javaArray << endl;
			jenv->ReleasePrimitiveArrayCritical((jarray)javaArray, addr,JNI_ABORT);
			isPinned = JNI_FALSE;
		}
	}

	void unpinCommit(JNIEnv *jenv){
		if (isPinned) {
         //cout << "unpinning commit " << addr << " " <<javaArray << endl;
			jenv->ReleasePrimitiveArrayCritical((jarray)javaArray, addr, 0);
			isPinned = JNI_FALSE;
		}
	}

	void pin(JNIEnv *jenv){
		void *ptr = addr;
		addr = jenv->GetPrimitiveArrayCritical((jarray)javaArray,&isCopy);
      //cout << "pinning " << addr << " " <<javaArray << endl;
		isPinned = JNI_TRUE;
	}

};

#endif // ARRAYBUFFER_H
