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

#include "jni.h"
#define JNI_JAVA(type, className, methodName) JNIEXPORT type JNICALL Java_com_amd_okra_##className##_##methodName
#include "com_amd_okra_OkraContext.h"
#include "com_amd_okra_OkraKernel.h"
#include "okraContext.h"
#include "arrayBuffer.h"
#include "objBuffer.h"
#include <vector>
#include <algorithm>

typedef unsigned char byte;
using namespace std;

static void * getPtrFromObjRef(jobject obj) {
	// hack to make a pointer from the obj refererence
	// this probably only works with compressed oops off, or with zero-based compressed oops
	void **holderPtr = (void **) ((long) obj * 1);
	void *ptr = *holderPtr;
	return ptr;
}


class OkraContextHolder {
public:
	OkraContext *realContext;
	JNIEnv *jenv;
	ArrayBuffer * dummyArrayBuf;

	OkraContextHolder(OkraContext *_realContext, JNIEnv *_jenv, jarray _dummyArray) :
		realContext(_realContext),
	    jenv(_jenv) {
        realContext->setVerbose(getenv("OKRA_VERBOSE") != NULL);
		dummyArrayBuf = new ArrayBuffer(_dummyArray, sizeof(jint), -1, _jenv);
	}

	bool isVerbose() {return realContext->isVerbose();}
};


class OkraKernelHolder {
public:
	vector<ArrayBuffer *> arrayBufs;
	vector<ObjBuffer *> objBufs;
	int arg_count;
	OkraContextHolder *okraContextHolder;
	OkraContext::Kernel *realOkraKernel;
	JNIEnv *jenv;

	OkraKernelHolder(OkraContext::Kernel *_realKernel, OkraContextHolder *_okraContextHolder, JNIEnv *_jenv) :
		realOkraKernel(_realKernel),
		okraContextHolder(_okraContextHolder),
	    jenv(_jenv),
	    arg_count(0) {
	}

	void pushArrayBuffer(ArrayBuffer *arrayBuffer) {
		arrayBufs.push_back(arrayBuffer);
	}

	void clearArrayBuffers() {
		for (int i=0; i<arrayBufs.size(); i++) {
			ArrayBuffer *arrayBuffer = arrayBufs.at(i);
			delete arrayBuffer;
		}
		arrayBufs.clear();
	}

	void unpinArrays() {
		// check if no real array arguments and if so, unpin our dummyArray 
		if (arrayBufs.size() == 0) {
			okraContextHolder->dummyArrayBuf->unpinCommit(jenv);
		}
		else {
			for (int i=0; i<arrayBufs.size(); i++) {
				ArrayBuffer *arrayBuffer = arrayBufs.at(i);
				arrayBuffer->unpinCommit(jenv);
			}
		}
	}

	void pinArrays() {
		// if no real array arguments, use our dummyArray arg so we can pin something
		if (arrayBufs.size() == 0) {
			okraContextHolder->dummyArrayBuf->pin(jenv);
		}
		else {
			for (int i=0; i<arrayBufs.size(); i++) {
				ArrayBuffer *arrayBuffer = arrayBufs.at(i);
				// FIXME, should be logic here to check for movement?
				if (!arrayBuffer->isPinned) {
					arrayBuffer->pin(jenv);
					// change the appropriate pointer argument in the arg stack
					realOkraKernel->setPointerArg(arrayBuffer->arg_idx, arrayBuffer->addr);
					// note: must also call registerArrayMemory (until we can make this call go away)
					okraContextHolder->realContext->registerArrayMemory(arrayBuffer->addr, arrayBuffer->length * arrayBuffer->elementSize + 32);
				}
			}
		}
	}
	bool isVerbose() {return okraContextHolder->isVerbose();}


	void pushObjBuffer(ObjBuffer *objBuffer) {
		objBufs.push_back(objBuffer);
	}

	void clearObjBuffers() {
		for (int i=0; i<objBufs.size(); i++) {
			ObjBuffer *objBuffer = objBufs.at(i);
			delete objBuffer;
		}
		objBufs.clear();
	}

	void saveObjAddresses() {
		for (int i=0; i<objBufs.size(); i++) {
			ObjBuffer *objBuffer = objBufs.at(i);
			void * ptr = getPtrFromObjRef(objBuffer->javaObj);
			realOkraKernel->setPointerArg(objBuffer->arg_idx, ptr);
		}
	}
	
};


OkraContextHolder * getOkraContextHolderPointer(JNIEnv *jenv, jobject fromObj) {
	jclass fromClass = jenv->GetObjectClass(fromObj);
	jlong handle = jenv->GetLongField(fromObj, jenv->GetFieldID(fromClass, "contextHandle", "J"));
	// convert to OkraContext object and return
	return (OkraContextHolder *) handle;
}

OkraKernelHolder * getOkraKernelHolderPointer(JNIEnv *jenv, jobject fromObj) {
	jclass fromClass = jenv->GetObjectClass(fromObj);
	jlong handle = jenv->GetLongField(fromObj, jenv->GetFieldID(fromClass, "kernelHandle", "J"));
	// convert to OkraKernelHolder object and return
	return (OkraKernelHolder *) handle;
}

jint pushArrayArgInternal(JNIEnv *jenv , jobject javaOkraKernel, jarray ary, jint elementSize) {
	OkraKernelHolder * kernelHolder = getOkraKernelHolderPointer(jenv, javaOkraKernel);

	// create a new ArrayBuffer object to hold info of this array
	// and keep it on an internal list so we can unpin it after execution
	ArrayBuffer *arrayBuffer = new ArrayBuffer(ary, elementSize, kernelHolder->arg_count, jenv);
	kernelHolder->pushArrayBuffer(arrayBuffer);

	// note: pinning and registering of memory will happen at exec time
	// for now we push a dummy value
	kernelHolder->arg_count++;
	return kernelHolder->realOkraKernel->pushPointerArg(0);
}



JNI_JAVA(jlong, OkraContext, createOkraContextJNI) (JNIEnv *jenv, jclass clazz, jintArray dummyArray) {
	// if we really can create a context, return a handle to an internal ContextHOlder
	// otherwise return a null handle
	OkraContext *realOkraContext =  OkraContext::Create();
	if (realOkraContext == NULL)
		return (jlong) 0;
	else
		return (jlong) new OkraContextHolder(realOkraContext, jenv, (jarray) dummyArray);
}

JNI_JAVA(jlong, OkraContext, createKernelJNI)  (JNIEnv *jenv , jobject javaOkraContext, jstring source, jstring entryName) {
	const char *source_cstr = jenv->GetStringUTFChars(source, NULL);
	const char *entryName_cstr = jenv->GetStringUTFChars(entryName, NULL);
	OkraContextHolder * okraContextHolder = getOkraContextHolderPointer(jenv, javaOkraContext);

	// if we really can create a kernel, return a handle to an internal kernel holder wrapping the real okra kernel
	// otherwise return a null handle
	OkraContext::Kernel *realOkraKernel = okraContextHolder->realContext->createKernel(source_cstr, entryName_cstr);
	if (realOkraKernel == NULL) 
		return (jlong) 0;
	else
		return (jlong) new OkraKernelHolder(realOkraKernel, okraContextHolder, jenv);
}

JNI_JAVA(jint, OkraContext, dispose)  (JNIEnv *jenv , jobject javaOkraContext) {
	OkraContextHolder * okraContextHolder = getOkraContextHolderPointer(jenv, javaOkraContext);
	return okraContextHolder->realContext->dispose();
}

// if RegisterHeapMemory works, this might go away
JNI_JAVA(jint, OkraContext, registerObjectMemory)  (JNIEnv *jenv , jobject javaOkraContext, jobject obj, jint len) {
	OkraContextHolder * okraContextHolder = getOkraContextHolderPointer(jenv, javaOkraContext);
	void *ptr = getPtrFromObjRef(obj);
	return okraContextHolder->realContext->registerArrayMemory(ptr, len);
}

// Register the whole java heap
JNI_JAVA(jint, OkraContext, registerHeapMemory)  (JNIEnv *jenv , jobject javaOkraContext, jobject obj) {
	OkraContextHolder * okraContextHolder = getOkraContextHolderPointer(jenv, javaOkraContext);
	bool show = okraContextHolder->isVerbose();
#ifdef  _WIN32
	// hack to make a pointer from the obj refererence
	// this probably only works with compressed oops off, or with zero-based compressed oops
	void *ptr = getPtrFromObjRef(obj);

	// this is a no-op on the simulator.
	// for hardware targets we would find the bounds of the heap and register it
	size_t size;
	void *startAddr = vqueryLargest(ptr, &size, show);
	void *endAddr = ((byte *)startAddr) + size-1;
	if (okraContextHolder->isVerbose()) {
		cerr << "from object reference at " << ptr << ", we think heap region is from " << startAddr << " to " << endAddr
			 << ", size=" << dec << size/(1024*1024) << "m" << endl;
	}

	// it seems we cannot register the parts that are just reserved vas (unbacked by real memory)
	// so instead of just trying to register the "live" pieces, we will touch the whole heap
	// (so it is all committed) and then register that big chunk with hsa
	commitAndRegisterWholeHeap(startAddr, endAddr);
#endif
	return 0;
}

JNI_JAVA(void, OkraContext, setVerbose)  (JNIEnv *jenv , jobject javaOkraContext, jboolean isVerbose) {
	OkraContextHolder * okraContextHolder = getOkraContextHolderPointer(jenv, javaOkraContext);
	okraContextHolder->realContext->setVerbose(isVerbose);
}

static void dumpref(jobject ref) {
	jbyte * pbobj = (jbyte *) getPtrFromObjRef(ref) ;
	for (int i=0; i<24; i+=4) {
		cout << "deref ptr+" << i << "is " << hex << *(int *)(pbobj + i) << endl;
	}
}

JNI_JAVA(jlong, OkraContext, createRefHandle)  (JNIEnv *jenv , jclass clazz, jobject obj) {
	//cout << "getRefHandle, obj is " << obj << ", ptr is " << getPtrFromObjRef(obj) << endl;
	jbyte * pbobj = (jbyte *) getPtrFromObjRef(obj) ;
	//dumpref(obj);
	jobject ref = jenv->NewGlobalRef(obj);
	//cout << "getRefHandle, ref is " << ref << ", ptr is " << getPtrFromObjRef(ref) << endl;
	return (jlong) ref;

}

JNI_JAVA(void, OkraContext, useRefHandle)  (JNIEnv *jenv , jclass clazz, jlong handle) {
	jobject ref = (jobject) handle;
	//cout << "useRefHandle, ref is " << ref << ", ptr is " << getPtrFromObjRef(ref) << endl;
	//dumpref(ref);
}

// would have been nice if we could have used templates here...
JNI_JAVA(jint, OkraKernel, pushFloatArrayArg) (JNIEnv *jenv , jobject javaOkraKernel, jfloatArray ary) {
	return pushArrayArgInternal(jenv, javaOkraKernel, ary, sizeof(jfloat));
}

JNI_JAVA(jint, OkraKernel, pushDoubleArrayArg) (JNIEnv *jenv , jobject javaOkraKernel, jdoubleArray ary) {
	return pushArrayArgInternal(jenv, javaOkraKernel, ary, sizeof(jdouble));
}

JNI_JAVA(jint, OkraKernel, pushBooleanArrayArg) (JNIEnv *jenv , jobject javaOkraKernel, jbooleanArray ary) {
	return pushArrayArgInternal(jenv, javaOkraKernel, ary, sizeof(jboolean));
}

JNI_JAVA(jint, OkraKernel, pushByteArrayArg) (JNIEnv *jenv , jobject javaOkraKernel, jbyteArray ary) {
	return pushArrayArgInternal(jenv, javaOkraKernel, ary, sizeof(jbyte));
}

JNI_JAVA(jint, OkraKernel, pushIntArrayArg) (JNIEnv *jenv , jobject javaOkraKernel, jintArray ary) {
	return pushArrayArgInternal(jenv, javaOkraKernel, ary, sizeof(jint));
}

JNI_JAVA(jint, OkraKernel, pushLongArrayArg) (JNIEnv *jenv , jobject javaOkraKernel, jlongArray ary) {
	return pushArrayArgInternal(jenv, javaOkraKernel, ary, sizeof(jlong));
}

JNI_JAVA(jint, OkraKernel, pushObjectArrayArgJNI) (JNIEnv *jenv , jobject javaOkraKernel, jobjectArray ary) {
	// while we don't know exactly the size of each reference, it won't be more than 8
	// and it's ok to register memory that is bigger than necessary
	return pushArrayArgInternal(jenv, javaOkraKernel, ary, 8);
}


// would have been nice if we could have used templates here...
JNI_JAVA(jint, OkraKernel, pushFloatArg) (JNIEnv *jenv , jobject javaOkraKernel, jfloat arg) {
	OkraKernelHolder * kernelHolder = getOkraKernelHolderPointer(jenv, javaOkraKernel);
	kernelHolder->arg_count++;
	return kernelHolder->realOkraKernel->pushFloatArg(arg);
}

JNI_JAVA(jint, OkraKernel, pushDoubleArg) (JNIEnv *jenv , jobject javaOkraKernel, jdouble arg) {
	OkraKernelHolder * kernelHolder = getOkraKernelHolderPointer(jenv, javaOkraKernel);
	kernelHolder->arg_count++;
	return kernelHolder->realOkraKernel->pushDoubleArg(arg);
}

JNI_JAVA(jint, OkraKernel, pushBooleanArg) (JNIEnv *jenv , jobject javaOkraKernel, jboolean arg) {
	OkraKernelHolder * kernelHolder = getOkraKernelHolderPointer(jenv, javaOkraKernel);
	kernelHolder->arg_count++;
	return kernelHolder->realOkraKernel->pushBooleanArg(arg);
}

JNI_JAVA(jint, OkraKernel, pushByteArg) (JNIEnv *jenv , jobject javaOkraKernel, jbyte arg) {
	OkraKernelHolder * kernelHolder = getOkraKernelHolderPointer(jenv, javaOkraKernel);
	kernelHolder->arg_count++;
	return kernelHolder->realOkraKernel->pushByteArg(arg);
}

JNI_JAVA(jint, OkraKernel, pushIntArg) (JNIEnv *jenv , jobject javaOkraKernel, jint arg) {
	OkraKernelHolder * kernelHolder = getOkraKernelHolderPointer(jenv, javaOkraKernel);
	kernelHolder->arg_count++;
	return kernelHolder->realOkraKernel->pushIntArg(arg);
}

JNI_JAVA(jint, OkraKernel, pushLongArg) (JNIEnv *jenv , jobject javaOkraKernel, jlong arg) {
	OkraKernelHolder * kernelHolder = getOkraKernelHolderPointer(jenv, javaOkraKernel);
	kernelHolder->arg_count++;
	return kernelHolder->realOkraKernel->pushLongArg(arg);
}

JNI_JAVA(jint, OkraKernel, pushObjectArgJNI) (JNIEnv *jenv , jobject javaOkraKernel, jobject arg) {
	OkraKernelHolder * kernelHolder = getOkraKernelHolderPointer(jenv, javaOkraKernel);

	jweak ref = jenv->NewWeakGlobalRef(arg);
	// cout << "pushObjectArg, weakref is " << ref << endl;

	// create a new ObjBuffer object to hold info of this object
	// and keep it on an internal list so we can get the real address at execution time
	ObjBuffer *objBuffer = new ObjBuffer(ref, kernelHolder->arg_count, jenv);
	kernelHolder->pushObjBuffer(objBuffer);

	// for now we push a dummy value
	kernelHolder->arg_count++;
	return kernelHolder->realOkraKernel->pushPointerArg(0);
}

JNI_JAVA(jint, OkraKernel, clearArgs) (JNIEnv *jenv , jobject javaOkraKernel) {
	OkraKernelHolder * kernelHolder = getOkraKernelHolderPointer(jenv, javaOkraKernel);
	kernelHolder->arg_count = 0;
	// clear internal vectors as well
	kernelHolder->clearArrayBuffers();
	kernelHolder->clearObjBuffers();
	// make okra call
	return kernelHolder->realOkraKernel->clearArgs();
}


JNI_JAVA(jint, OkraKernel, setLaunchAttributes) (JNIEnv *jenv , jobject javaOkraKernel, jint numWorkItems, jint groupSize) {
	OkraKernelHolder * kernelHolder = getOkraKernelHolderPointer(jenv, javaOkraKernel);

	size_t globalDims[] = {numWorkItems}; 
	size_t localDims[] = {groupSize};

	// make okra call
	return kernelHolder->realOkraKernel->setLaunchAttributes(1, globalDims, localDims);
}


JNI_JAVA(jint, OkraKernel, dispatchKernelWaitCompleteJNI) (JNIEnv *jenv , jobject javaOkraKernel) {
	OkraKernelHolder * kernelHolder = getOkraKernelHolderPointer(jenv, javaOkraKernel);

	//cout << "before dispatch: array args = " << kernelHolder->arrayBufs.size() 
	//	 << ", object args = " << kernelHolder->objBufs.size() << endl;

	// pin any arrays that are being used as args
	kernelHolder->pinArrays();
	// and get final addresses of any objects
	kernelHolder->saveObjAddresses();
	// make okra call
	jint status = kernelHolder->realOkraKernel->dispatchKernelWaitComplete();
	// unpin any pinned arrays we had
	kernelHolder->unpinArrays();

	//cout << "after dispatch:" << endl;

	return status;
}

JNI_JAVA(jboolean, OkraContext, isSimulator)  (JNIEnv *jenv , jclass clazz) {
	return OkraContext::isSimulator();
}

