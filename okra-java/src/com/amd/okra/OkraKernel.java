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
package com.amd.okra;

public class OkraKernel {

    static {
        OkraContext.loadOkraNativeLibrary();
    } // end static

    public OkraKernel(OkraContext okraContextInput, String source, String entryName) {
        okraContext = okraContextInput;
        contextHandle = okraContextInput.getContextHandle();
        kernelHandle = okraContextInput.createKernelJNI(source, entryName);
        argsVecHandle = 0;
        // register the whole range of heap memory in the device
        // we give it one object and it deduces the range
        //okraContext.registerHeapMemory(new Object());
    }

    private long kernelHandle;
    private long contextHandle;  // used by the JNI side
    private long argsVecHandle;  // only used by JNI side
    private OkraContext okraContext;

    public boolean isValid() {
        return (kernelHandle != 0);
    }

    // various methods for setting different types of args into the arg stack
    public native int pushFloatArg(float f);

    public native int pushIntArg(int i);

    public native int pushBooleanArg(boolean z);

    public native int pushByteArg(byte b);

    public native int pushLongArg(long j);

    public native int pushDoubleArg(double d);

    public native int pushIntArrayArg(int[] a);

    public native int pushFloatArrayArg(float[] a);

    public native int pushDoubleArrayArg(double[] a);

    public native int pushBooleanArrayArg(boolean[] a);

    public native int pushByteArrayArg(byte[] a);

    public native int pushLongArrayArg(long[] a);

    public native int clearArgs();

    private native int pushObjectArrayArgJNI(Object[] a);    // for possibly supporting oop array

    private native int pushObjectArgJNI(Object obj);

    public int pushObjectArg(Object obj) {
        // since we registered the heap when okraContext was created,
        // we believe no further memory registration is needed here

        return pushObjectArgJNI(obj);
    }

    public int pushObjectArrayArg(Object[] a) {    // for possibly supporting oop array
        // since we registered the heap when okraContext was created,
        // we believe no further memory registration is needed here

        return pushObjectArrayArgJNI(a);
    }

    // for the setLaunchAttributes calls, we are just assuming 1D support for now.
    // version that explicitly specifies numWorkItems and groupSize
    public native int setLaunchAttributes(int numWorkItems, int groupSize);

    // version that just specifies numWorkItems and we will pick a "best" groupSize
    public int setLaunchAttributes(int numWorkItems) {
        return setLaunchAttributes(numWorkItems, 0);
    }

    // run a kernel and wait until complete
    private native int dispatchKernelWaitCompleteJNI();

    public int dispatchKernelWaitComplete() {
        // here we would push any "constant" arguments (currently we have none)
        return dispatchKernelWaitCompleteJNI();
    }

    // if it is primitive, calls the appropriate push routine and
    // returns true else returns false
    private boolean pushPrimitiveArg(Class<?> argclass, Object arg) {
        if (argclass.equals(Float.class)) {
            pushFloatArg((float) arg);
        } else if (argclass.equals(Integer.class)) {
            pushIntArg((int) arg);
        } else if (argclass.equals(Long.class)) {
            pushLongArg((long) arg);
        } else if (argclass.equals(Double.class)) {
            pushDoubleArg((double) arg);
        } else if (argclass.equals(Boolean.class)) {
            pushBooleanArg((boolean) arg);
        } else if (argclass.equals(Byte.class)) {
            pushByteArg((byte) arg);
        } else {
            return false;
        }
        // if we took one of the push paths, return true
        return true;
    }

    // a "convenience routine" if you know the arguments to match the
    // requirements of the graal compiler, all arrays will push the
    // simple reference, rather than calling pushXXXArrayArg which
    // ends up pushing the raw array data pointer.

    public int dispatchWithArgs(Object... args) {
        clearArgs();
        for (Object arg : args) {
            Class<?> argclass = arg.getClass();

            if (!pushPrimitiveArg(argclass, arg)) {
                // in this usage everything that is not a primitive is pushed as an "object"
                pushObjectArg(arg);
            }
        }
        return dispatchKernelWaitComplete();
    }

    // the following version would instead push arrays as old-aparapi-opencl style raw array data
// pointers
    public int dispatchWithArgsUsingRawArrays(Object... args) {
        clearArgs();
        for (Object arg : args) {
            Class<?> argclass = arg.getClass();

            if (!pushPrimitiveArg(argclass, arg)) {
                if (argclass.equals(float[].class)) {
                    pushFloatArrayArg((float[]) arg);
                } else if (argclass.equals(int[].class)) {
                    pushIntArrayArg((int[]) arg);
                } else if (argclass.equals(long[].class)) {
                    pushLongArrayArg((long[]) arg);
                } else if (argclass.equals(double[].class)) {
                    pushDoubleArrayArg((double[]) arg);
                } else if (argclass.equals(boolean[].class)) {
                    pushBooleanArrayArg((boolean[]) arg);
                } else if (argclass.equals(byte[].class)) {
                    pushByteArrayArg((byte[]) arg);
                } else if (arg instanceof Object[]) {

                    pushObjectArrayArg((Object[]) arg);
                } else {

                    // since we registered the heap when okraContext was created,
                    // we believe no further memory registration is needed here
                    pushObjectArg(arg);
                }
            }
        }
        return dispatchKernelWaitComplete();
    }

}
