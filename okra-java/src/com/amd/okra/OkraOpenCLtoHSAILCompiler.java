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

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedInputStream;
import java.util.Arrays;

public class OkraOpenCLtoHSAILCompiler {

    public static native String compileToHSAILJNI(String openCLSource);

    public static String compileToHSAIL(String openCLSource, String baseName, String batchScript) {
        // return compileToHSAILJNI(openCLSource);
        try {
            final String oclFileName = baseName + ".ocl";
            final String binFileName = baseName + ".bin";
            final String hsailFileName = baseName + ".hsail";

            // Set up source input file for hsail tools
            FileOutputStream fos = new FileOutputStream(oclFileName);
            fos.write(openCLSource.getBytes());
            fos.flush();
            fos.close();

            executeCmd("aoc2", "-m64", "-I./", "-march=hsail", oclFileName);
            executeCmd("HSAILasm", "-disassemble", "-o", hsailFileName, binFileName);

            // Now the .hsail file should exist
            FileInputStream fis = new FileInputStream(hsailFileName);
            BufferedReader d = new BufferedReader(new InputStreamReader(fis));

            StringBuffer hsailSourceBuffer = new StringBuffer();
            String line;
            String cr = System.getProperty("line.separator");
            do {
                line = d.readLine();
                if (line != null) {
                    hsailSourceBuffer.append(line);
                    hsailSourceBuffer.append(cr);
                }
            } while (line != null);
            String result = hsailSourceBuffer.toString();

            return result;

        } catch (IOException e) {
            System.out.println(e);
            e.printStackTrace();
            return null;
        }
    }

    private static void executeCmd(String... cmd) {
        System.out.println("spawning" + Arrays.toString(cmd));
        try {
            ProcessBuilder pb = new ProcessBuilder(cmd);
            Process p = pb.start();
            if (true) {
                InputStream in = p.getInputStream();
                BufferedInputStream buf = new BufferedInputStream(in);
                InputStreamReader inread = new InputStreamReader(buf);
                BufferedReader bufferedreader = new BufferedReader(inread);
                String line;
                while ((line = bufferedreader.readLine()) != null) {
                    System.err.println(line);
                }
            }
            p.waitFor();
        } catch (Exception e) {
            System.err.println("could not execute <" + Arrays.toString(cmd) + ">");
        }
    }

}
