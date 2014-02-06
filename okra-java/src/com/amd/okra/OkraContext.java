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

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.InputStream;
import java.nio.file.Path;
import java.nio.file.Files;
import java.util.logging.*;
import java.util.jar.*;
import java.util.ArrayList;
import java.util.List;

public class OkraContext {

    /***
     * enum OkraStatus { OKRA_OK, OKRA_OTHER_ERROR };
     ***/

    public interface LibraryLocator {
       String getPath();
    }

    static {
        loadOkraNativeLibrary();
    } // end static

    static boolean isNativeLibraryLoaded;
    private static String mappedOkraLibraryName;
    public static String getMappedOkraLibraryName() {
        return mappedOkraLibraryName;
    }
    public static void setIsNativeLibraryLoaded( boolean b) {
        isNativeLibraryLoaded = b;
    }
    static void loadOkraNativeLibrary() {
        if (isNativeLibraryLoaded) return;
        String arch = System.getProperty("os.arch");
        String okraLibraryName = null;
        String okraLibraryRoot = "okra";
        
        if (arch.equals("amd64") || arch.equals("x86_64")) {
            okraLibraryName = okraLibraryRoot + "_x86_64";
        } else if (arch.equals("x86") || arch.equals("i386")) {
            okraLibraryName = okraLibraryRoot + "_x86";
        } else {
			throw new UnsatisfiedLinkError("Expected property os.arch to contain amd64, x86_64, x86 or i386 but instead found " + arch + " as a result we don't know which okra to attempt to load.");
        }

        mappedOkraLibraryName = System.mapLibraryName(okraLibraryName);
        try {
            if (okraLibraryName != null) {
                // since the library directory (okra/dist/bin) also needs to be part of the PATH env variable, check that
                String pathStr = System.getenv("PATH");
                String[] paths = pathStr.split(File.pathSeparator);
                for (String path : paths) {
                    String okraLibraryFullName = path + File.separator + mappedOkraLibraryName;
                    // System.out.println("testing for okra library at " + okraLibraryFullName);
                    if (new File(okraLibraryFullName).isFile()) {
                        System.load(okraLibraryFullName);
                        // System.out.println("loaded okra library from " + okraLibraryFullName);
                        isNativeLibraryLoaded = true;
                        return;
                    }
                }
            }
        } catch (UnsatisfiedLinkError e) {
            isNativeLibraryLoaded = false;
        }

        // if we get this far we didn't find it on the path, try
        // whatever boot.library.path was set up using loadLibrary
        // without an absolute pathname
        try {
            System.loadLibrary(okraLibraryName);
            // System.out.println("loaded okra library using System.loadLibrary()");
            isNativeLibraryLoaded = true;
            return;
        } catch (UnsatisfiedLinkError e) {
            isNativeLibraryLoaded = false;
        }

        // and if we still haven't loaded it use the special
        // OkraResourceExtractor to unjar it from the resources
        // directory of its jar file into a temporary directory and
        // tell us where it put it.
        try {
            // note that OkraResourceExtractor is always loaded by the SystemClassLoader, not the bootpath loader
		    // whereas OkraContext might be on the bootclasspath, hence the Class.forName usage
            Class<?> c = Class.forName("com.amd.okra.OkraResourceExtractor", true, ClassLoader.getSystemClassLoader());
            String okraLibraryNameExtracted = ((LibraryLocator) c.newInstance()).getPath();
			if (okraLibraryNameExtracted == null) {
			    throw new UnsatisfiedLinkError("no resource found for "  + mappedOkraLibraryName);
			}
            System.load(okraLibraryNameExtracted);
            isNativeLibraryLoaded = true;
            return;
        } catch (InstantiationException | IllegalAccessException |  UnsatisfiedLinkError | ClassNotFoundException e) {
            throw new UnsatisfiedLinkError("unable to load "  + mappedOkraLibraryName + ", " + e.toString());
        } catch (Exception e) {
            // we have run out of ways to try to find the library
            throw new UnsatisfiedLinkError("unable to load "  + mappedOkraLibraryName + ", " + e.toString());
        }

    }


    public boolean isValid() {
        return (contextHandle != 0);
    }

    private long contextHandle;
    private int[] dummyArray;   // used for pinning if no other arrays passed

    public OkraContext() {
        dummyArray = new int[1];
        // call the JNI routine and store it locally
        contextHandle = createOkraContextJNI(dummyArray);
    }

    long getContextHandle() {
        return contextHandle;
    }

    // create a c++ okraContext object
    private static native long createOkraContextJNI(int[] ary);

    // create a c++ kernel object from the specified source and entrypoint
    native long createKernelJNI(String source, String entryName);

    // dispose of an environment including all programs
    public native int dispose();

    // I hope this one will go away
    public native int registerObjectMemory(Object obj, int len);

    public native int registerHeapMemory(Object obj);

    public native void setVerbose(boolean b);

    static native long createRefHandle(Object obj);

    // for testing only
    public static native void useRefHandle(long handle);

    public static native boolean isSimulator();

    public static int version = 1;
}
