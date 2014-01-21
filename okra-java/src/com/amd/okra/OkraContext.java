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

    private static final Logger logger = Logger.getLogger("okracontext");
    /***
     * enum OkraStatus { OKRA_OK, OKRA_OTHER_ERROR };
     ***/
    static {
        loadOkraNativeLibrary();
    } // end static

	private static boolean isNativeLibraryLoaded;

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
            logger.fine("Expected property os.arch to contain amd64, x86_64, x86 or i386 but instead found " + arch + " as a result we don't know which okra to attempt to load.");
        }

		String mappedOkraLibraryName = System.mapLibraryName(okraLibraryName);
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

		// and if we still haven't loaded it check if it is in the
		// resources directory of our jar file.  If so, create a
		// temporary directory and unjar it there.
		String resourcesOkraDir = "resources/okra/";
		Path tmpdirPath;
		try {
			tmpdirPath = Files.createTempDirectory("okrasim.dir_");
			System.out.println("created temp directory" + tmpdirPath);
			// if we couldn't even do that, give up
			if (!tmpdirPath.toFile().exists()) {
				throw new UnsatisfiedLinkError();
			}
			tmpdirPath.toFile().deleteOnExit();

			copyResourcesToTmpdir(getResourcesList(resourcesOkraDir), resourcesOkraDir, tmpdirPath);
	
		} catch (IOException e) {
            throw new RuntimeException("error creating temporary directory for jar resources");
		}

		// now load the library from the tmpdirPath
		String okraLibraryFullName = tmpdirPath + File.separator + mappedOkraLibraryName;
		if (new File(okraLibraryFullName).isFile()) {
			System.load(okraLibraryFullName);
			// System.out.println("loaded okra library from " + okraLibraryFullName);
			isNativeLibraryLoaded = true;
			return;
		}
	}


	private static List<String> getResourcesList(String resourcesOkraDir) {
		try {
			// list files in a jar file resources directory
			List<String> files = new ArrayList<String>();
			
			// Stream the jar file 
			String jarpath = OkraContext.class.getResource("OkraContext.class").getPath();
			// System.out.println("jar path is " + jarpath);
			if (! jarpath.startsWith("file:")) {
				throw new RuntimeException("Unable to get Jar input stream");
			}
			String jarfilename = jarpath.substring(5, jarpath.indexOf("!"));
			// System.out.println("jarfilename is " + jarfilename );
			JarInputStream jarInputStream = new JarInputStream(new FileInputStream(jarfilename));
			JarEntry jarEntry;
			
			// Iterate the jar entries within that jar, looking for ones in resources/okra
			do {
				jarEntry = jarInputStream.getNextJarEntry();
				if (jarEntry != null) {
					String fileName = jarEntry.getName();
					
					// check against our filter
					if (fileName.startsWith(resourcesOkraDir) && !fileName.equals(resourcesOkraDir)) {
					    String root = fileName.substring(resourcesOkraDir.length());
						files.add(root);
					}
				}
			} while (jarEntry != null);

			jarInputStream.close();
			return files;
		} catch (IOException e) {
			throw new RuntimeException("error getting resource list from jar file");
		}
	}

  private static void copyResourcesToTmpdir(List<String> fnames, String resourcesOkraDir, Path tmpDirPath) {
		try {
			// now for each file in resources folder copy to tmpdir
			for (String fname : fnames) {
				System.out.println("copying " + fname + " to tmpdir...");
				// Prepare buffer for data copying
				byte[] buffer = new byte[32768];
				int readBytes;
				
				// Open and check input stream
				InputStream myis = OkraContext.class.getResourceAsStream("/" + resourcesOkraDir + fname);
				if (myis == null) {
					throw new RuntimeException("File " + fname + " was not found inside jar:" + resourcesOkraDir);
				}
				
				// Open output stream and copy data between source file in jar and the temporary file
				File outfile = new File(tmpDirPath.toString() + File.separator + fname);
				outfile.deleteOnExit();
				try (OutputStream os = new FileOutputStream(outfile);
					 InputStream is = myis) {
						while ((readBytes = is.read(buffer)) != -1) {
							os.write(buffer, 0, readBytes);
						}
					} catch (IOException e) {
					throw new RuntimeException("error copying " + fname + " from jar file");
				}
				outfile.setExecutable(true, false);
				outfile.setReadable(true, false);
				if (!outfile.canExecute()) {
					throw new RuntimeException("error setting execute on " + fname);
				}
			}
		} catch (Exception e) {
			throw new RuntimeException("error copying resource from jar file to tmpdir");
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
