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

import java.util.concurrent.atomic.AtomicLong;
import java.util.Map;
import java.util.HashMap;

public class OkraUtil {

    // return whether the okra library is loadable
    private static boolean libExists = testOkraLibExists();
    private static AtomicLong nextFakeRef = new AtomicLong(0x76543210);
    private static Map<Object, Long> refMap = new HashMap<>();

    public static boolean okraLibExists() {
        return libExists;
    }

    private static boolean testOkraLibExists() {
        try {
            OkraContext.loadOkraNativeLibrary();
        } catch (UnsatisfiedLinkError e) {
            return false;
        } catch (NoClassDefFoundError e) {
            return false;
        }
        // if we got this far, it exists
        return true;
    }

    public static long getRefHandle(Object obj) {
        Long ref = refMap.get(obj);
        if (ref != null) {
            return ref;
        } else {
            long handle = createRefHandle(obj);
            refMap.put(obj, handle);
            return handle;
        }
    }

    private static long createRefHandle(Object obj) {
        if (okraLibExists()) {
            return OkraContext.createRefHandle(obj);
        } else {
            // just make one up
            // this allows us to be called for codegen purposes without the okra library being
// present
            return nextFakeRef.getAndAdd(4);
        }
    }
}
