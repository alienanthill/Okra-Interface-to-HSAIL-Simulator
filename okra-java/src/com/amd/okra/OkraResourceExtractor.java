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

public class OkraResourceExtractor implements OkraContext.LibraryLocator {

  private static boolean verboseResourceExtractor = Boolean.getBoolean("okra.resourceextractor.verbose");
  private static String okraLibraryFullName;   // will get set up by the constructor

  // the LibraryLocator interface
  @Override
  public String getPath() {
      return okraLibraryFullName;
  }

  static {
	String mappedOkraLibraryName = OkraContext.getMappedOkraLibraryName();
	String resourcesOkraDir = "resources/okra/" + System.getProperty("os.name") + "/";
        Path tmpdirPath;
        try {
            tmpdirPath = Files.createTempDirectory("okraresource.dir_");
            if (verboseResourceExtractor) {
                System.out.println("created temp directory" + tmpdirPath);
            }
            // if we couldn't even do that, give up
            if (!tmpdirPath.toFile().exists()) {
                throw new UnsatisfiedLinkError();
            }
            tmpdirPath.toFile().deleteOnExit();

            copyResourcesToTmpdir(getResourcesList(resourcesOkraDir), resourcesOkraDir, tmpdirPath);
    
        } catch (IOException e) {
            throw new RuntimeException("error creating temporary directory for jar resources");
        }

        // now set the full path for the newly created library
        // which will get picked up and loaded by OkraContext
        String fullName = tmpdirPath + File.separator + mappedOkraLibraryName;
		okraLibraryFullName = (new File(fullName).exists() ? fullName : null);
    }


    private static List<String> getResourcesList(String resourcesOkraDir) {
        try {
            // list files in a jar file resources directory
            List<String> files = new ArrayList<String>();
            
            // Stream the jar file 
            String jarpath = OkraResourceExtractor.class.getResource("OkraResourceExtractor.class").getPath();
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
                if (verboseResourceExtractor) {
                    System.out.println("copying " + fname + " to tmpdir...");
                }
                // Prepare buffer for data copying
                byte[] buffer = new byte[32768];
                int readBytes;
                
                // Open and check input stream
                InputStream myis = OkraResourceExtractor.class.getResourceAsStream("/" + resourcesOkraDir + fname);
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

}
