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

#include "common.h"
//for hsa
#include "hsa.h"

#include "okraContext.h"
#include "fileUtils.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include "stdio.h"
#include "pthread.h"

#ifdef __GNUC__
// the following logic to determine the pathname of our library and
// create an hsail_pathname from that is linux specific but for now
// that's the only target the simulator supports
#include <dlfcn.h>
#include <stdio.h>
using namespace std;

void append_to_env_var(const char *name, const char *addition) {
  const char *envname = (getenv(name) == NULL ? "" : getenv(name));
  char newval[strlen(envname) + strlen(addition) + 100];
  if (strlen(envname)) {
	sprintf(newval, "%s:%s", envname, addition);
  } else {
	sprintf(newval, "%s", addition);
  }
  setenv(name, newval, 1);
  // fprintf(stderr, "new value for %s=%s\n", name, getenv(name));
}

__attribute__((constructor))
void on_load(void) {
	Dl_info dl_info;
    dladdr((void *)on_load, &dl_info);
    // fprintf(stderr, "module %s loaded\n", dl_info.dli_fname);
    // separate pathname into directory part
	char dldir[strlen(dl_info.dli_fname) + 1];
	strcpy(dldir, dl_info.dli_fname);
	char *pend = strrchr(dldir, '/');   
	*pend = '\0';
    // fprintf(stderr, "dldir is %s\n", dldir);
	append_to_env_var("PATH", dldir);
	append_to_env_var("LD_LIBRARY_PATH", dldir);
	setenv("_OKRA_SIM_LIB_PATH_", dl_info.dli_fname, 1);  // for dlopen from JVM
}
#endif //__GNUC__


// An OkraContext interface to the simulator

class OkraContextSimulatorImpl : public OkraContext {
	friend okra_status_t OkraContext::getContext(OkraContext**); 
	
private:
	class KernelImpl : public OkraContext::Kernel {
	public:
		hsa::Kernel* hsaKernel;
		OkraContextSimulatorImpl* context;
		//add hsaargs here
		hsacommon::vector<hsa::KernelArg> hsaArgs;
		
		//Hsa launch attributes
		hsa::LaunchAttributes hsaLaunchAttr;
		
		KernelImpl(hsa::Kernel* _hsaKernel, OkraContextSimulatorImpl* _context) {
			hsaKernel = _hsaKernel;
			context = _context;
		}
	
		okra_status_t argsPushBack(hsa::KernelArg *harg) {
			hsaArgs.push_back(*harg);
			return OKRA_SUCCESS;
		}
		
		okra_status_t pushFloatArg(jfloat f) {
			hsa::KernelArg harg;
			harg.fvalue = f;
			return argsPushBack(&harg);
		}
	
		okra_status_t pushIntArg(jint i) {
			hsa::KernelArg harg;
			harg.s32value = i;
			return argsPushBack(&harg);
		}
	
		okra_status_t pushBooleanArg(jboolean z) {
			hsa::KernelArg harg;
			harg.u32value = z;
			return argsPushBack(&harg);
		}
	
		okra_status_t pushByteArg(jbyte b) {
			hsa::KernelArg harg;
			harg.s32value = b;  //not sure if this is right, verify later
			return argsPushBack(&harg);
		}
	
		okra_status_t pushLongArg(jlong j) {
			hsa::KernelArg harg;
			harg.s64value = j; 
			return argsPushBack(&harg);
		}

		okra_status_t pushDoubleArg(jdouble d) {
			hsa::KernelArg harg;
			harg.dvalue = d; 
			return argsPushBack(&harg);
		}
		
		
		okra_status_t pushPointerArg(void *addr) {
			//add the kernelarg for hsa runtime into the vector
			hsa::KernelArg harg;
			harg.addr = addr;
			if (context->isVerbose()) cerr<<"pushPointerArg, addr=" << addr <<endl;
			return argsPushBack(&harg);
		}

		// allow a previously pushed arg to be changed
		bool setPointerArg(int idx, void *addr) {
			hsa::KernelArg harg;
			harg.addr = addr;
			if (context->isVerbose()) cerr<<"setPointerArg, addr=" << addr <<endl;
			hsaArgs.at(idx) = harg;
			return true;
		}

		okra_status_t clearArgs() {
			hsaArgs.clear();
			return OKRA_SUCCESS;
		}

		okra_status_t dispatchKernelWaitComplete(OkraContext* _context) {
			hsacommon::vector<hsa::Event *> depEvent;
			hsa::DispatchEvent* hsaDispEvent = context->hsaQueue->dispatch(hsaKernel, 
										   hsaLaunchAttr,
										   depEvent,
										   hsaArgs);
	
			// in the simulator the returned hsaDispEvent is always null
			// so we just assume the kernel is finished
			// how to get a status here??
			// hsa::Status st = hsaDispEvent->wait();
			// return (mapHsaErrorToOkra(st)); 
			return OKRA_SUCCESS;
		}

		
		// NOTE: the okra "spec" treates globalDims as NDRangeSize
		// whereas the simulator currently treats what we call
		// globalDims as the number of grid blocks (so NDRangeSize =
		// grid * group).  So we need to do the conversion here.

		okra_status_t setLaunchAttributes(int dims, uint32_t *globalDims, uint32_t *localDims) {
			for (int k=0; k<dims; k++) {
				computeLaunchAttr(k, globalDims[k], localDims[k]);
			}
			return OKRA_SUCCESS;
		}

                okra_status_t dispose() {
                        return OKRA_SUCCESS;
                }

	private:
		void computeLaunchAttr(int level, int globalSize, int localSize) {
			// localSize of 0 means pick best
			// on the simulator, we might as well pick a group size of 1
			// since the simulator launch engine itself will still spawn
			// enough pthreads to use all the system processors
			if (localSize == 0) localSize = 1;
			// but also use SIMTHREADS env variable to limit the localSize
			if (context->maxSimThreads) localSize = std::min(localSize, context->maxSimThreads);

			// Check if globalSize is a multiple of localSize
			int legalGroupSize = findLargestFactor(globalSize, localSize);

			if (legalGroupSize != localSize) {
				cerr << "WARNING: groupSize[" << level << "] reduced to " << legalGroupSize << endl;
			}

			hsaLaunchAttr.groupOffsets[level] = 0;
			hsaLaunchAttr.grid[level] = globalSize / legalGroupSize;
			hsaLaunchAttr.group[level] = legalGroupSize;
			//debugging
			if (context->isVerbose()) {
				cerr << "level " << level << ", grid=" << hsaLaunchAttr.grid[level] << ", group=" << hsaLaunchAttr.group[level] << endl;
			}
		}

		// find largest factor less than or equal to start
		int findLargestFactor(int n, int start) {
			for (int div = start; div >=1; div--) {
				if (n % div == 0) return div;
			}
			return 1;
		}

		int gcd(int n, int m) {
			return (m == 0? n : gcd(m, n%m));
		}

	}; //end of kernelImpl

private:
	hsa::RuntimeApi *hsaRT;
	uint32_t numDevices;
	hsacommon::vector<hsa::Device *> devices;
	hsa::Queue *hsaQueue;
	int maxSimThreads;
	bool saveHsailSource;
  pthread_mutex_t kernelCreateMutex = PTHREAD_MUTEX_INITIALIZER;

	// constructor
	OkraContextSimulatorImpl() {
		setVerbose(false);   // can be set true by higher levels later
		char * saveHsailSourceEnvVar = getenv("OKRA_SAVEHSAILSOURCE");
		saveHsailSource =  (saveHsailSourceEnvVar == NULL ? false : strcmp(saveHsailSourceEnvVar, "1")==0);

		hsaRT = hsa::getRuntime();
		if(!hsaRT) {
			cerr<<"Fatal: Cannot get HSA Runtime"<<endl;
			exit(1);
		}

		numDevices = hsaRT->getDeviceCount();
		if(!numDevices) {
			cerr<<"Fatal: No HSA device exists"<<endl;
			exit(-1);
		}

		devices = hsaRT->getDevices();

		hsa::Device *device = devices[0];
		hsaQueue = device->createQueue(1);
		if(!hsaQueue) {
			cerr<<"Fatal: could not create hsaQueue"<<endl;
			exit(-1);
		}

		char *threnv = getenv("SIMTHREADS");
		if ((threnv != NULL) && (atoi(threnv) > 0)) {
			maxSimThreads = atoi(threnv);
		}
		else {
			maxSimThreads = 0;
		}
		
		if (isVerbose()) cerr<<"HSA Runtime successfully initialized"<<endl;
		
	}

public:
	okra_status_t createKernel(const char *hsailBuffer, const char *entryName, Kernel **kernel) {
		string *fixedHsailStr = fixHsail(hsailBuffer);
		
        char tmpHsailFileName[TMP_MAX];
        char tmpBrigFileName[TMP_MAX];
        pid_t pid = getpid();
        sprintf(tmpHsailFileName, "hsail_tmp_%d_XXXXXX", pid);
        sprintf(tmpBrigFileName, "brig_tmp_%d_XXXXXX", pid);        
        // The actual tmp file names are returned in the char[]'s
        int tmpFd = mkstemp(tmpHsailFileName);
        FILE* tmpFile = fdopen(tmpFd, "w");
        int brigFile = mkstemp(tmpBrigFileName);

        if (isVerbose()) cerr << "Fixed Hsail is\n==============\n" << *fixedHsailStr << endl;
        fprintf(tmpFile, "%s", fixedHsailStr->c_str());
        fclose(tmpFile);
        delete(fixedHsailStr);

		// use the -build hsailasm to translate source
		// use debug flag
        int bufLen = strlen(tmpHsailFileName) + strlen(tmpBrigFileName) + 128;
        char* cmdBuf = (char *) malloc(bufLen);
        sprintf(cmdBuf, "hsailasm %s -g -o %s", tmpHsailFileName, tmpBrigFileName);
        int ret = spawnProgram(cmdBuf);

        if (ret != 0) {
                       *kernel = NULL;
                       return OKRA_KERNEL_CREATE_FAILED;
                }
		if (isVerbose()) cerr << "hsailasm succeeded\n";

		size_t brigSize = 0;
		char *brigBuffer = readFile(tmpBrigFileName, brigSize);
		if (brigBuffer == NULL) {
			printf("cannot read from the %s\n", tmpBrigFileName);
			*kernel = NULL;
			return OKRA_KERNEL_CREATE_FAILED;
		}
		// delete temporary files
    remove(tmpBrigFileName);
    if (!saveHsailSource) {
        remove(tmpHsailFileName);
    }

		*kernel = createKernelCommon(brigBuffer, brigSize, entryName);
                return OKRA_SUCCESS;
	}

	okra_status_t createKernelFromBinary(const char *brigBuffer, size_t brigSize, const char *entryName, Kernel **kernel) {
		char *ptr = reinterpret_cast<char*>(malloc(brigSize));
		if (!ptr) {
                        *kernel = NULL;
			return OKRA_KERNEL_CREATE_FAILED;
		}

		memcpy(ptr, brigBuffer, brigSize);
		*kernel = createKernelCommon(ptr, brigSize, entryName);
                return OKRA_SUCCESS;
	}

	okra_status_t dispose(){
#if 0
		if (hsaProgram) {
			hsaRuntime->destroyProgram(hsaProgram);
		}
#endif
		return OKRA_SUCCESS;
	}

private:
	Kernel * createKernelCommon(char *brigBuffer, size_t brigSize, const char *entryName) {
    // Synchronize calls to hsa
    pthread_mutex_lock(&kernelCreateMutex);
		hsa::Program *hsaProgram =	hsaRT->createProgram(brigBuffer, brigSize, &devices);
		if(!hsaProgram) {
			cerr<<"HSA create program failed"<<endl;
			return NULL;
		}
		if (isVerbose()) cerr << "createProgram succeeded\n";

		hsa::Kernel *hsaKernel = hsaProgram->compileKernel(entryName, "");
		if(!hsaKernel) {
			cerr<<"HSA create kernel failed"<<endl;
			return NULL;
		}
		if (isVerbose()) cerr << "createKernel succeeded\n";
    pthread_mutex_unlock(&kernelCreateMutex);

		// if we got this far, success
		return new KernelImpl(hsaKernel, this);
	}

	int spawnProgram (const char *cmd) {
		if (isVerbose()) cerr << "spawning Program: " << cmd << endl;
		// not sure if we really have to do anything different for windows or linux here
		// assuming the utility is in the path
		return system(cmd);
	}

	string * fixHsail(const char *hsailStr) {
		string *s = new string(hsailStr);

		// the following conversions should no longer be needed because newer hsailasm is being used
		if (false) {
			// conversions are from 0.95 Spec format to MCW assembler format
			// version string, we don't really handle the non-$full models here 
			// also this should be made more regex based so we don't depend on whitespace
			replaceAll (*s, "0:95: $full : $large", "1:0");
			replaceAll (*s, "0:95: $full : $small", "1:0:$small");
			// workitemabsid mnemonic
			replaceAll (*s, "workitemabsid_u32", "workitemabsid");
			// mul_hi mnemonic
			replaceAll (*s, "mulhi", "mul_hi");
			
			// MCW assembler is pick about DOS line endings
			replaceAll (*s, "\r", "");
		}
		if (false) {
			// the following were the conversions from June 2012 format into MCW assembler format
			replaceAll (*s, "1:0:large", "1:0");
			replaceAll (*s, "1:0:small", "1:0:$small");
			replaceAll (*s, "workitemaid", "workitemabsid");
			replaceAll (*s, "cvt_near_f64_f32", "cvt_f64_f32");
		}
		return s;
	}
	
}; // end of OkraContextSimulatorImpl

bool OkraContext::isSimulator() {
	return true;
}

// simulator only supports coherent model
okra_status_t  OkraContext::setCoherence(bool isCoherent) {
	return OKRA_SUCCESS; 
}

bool  OkraContext::getCoherence() {
	return true; 
}

// Create an instance thru the OkraContext interface
okra_status_t OkraContext::getContext(OkraContext** context) {		
	*context = new OkraContextSimulatorImpl();
        return OKRA_SUCCESS;
}


#include "brig_runtime.h"
// for debugging
hsa::brig::BrigRegState prevRegState;
using namespace std;

static void showRegChanges(hsa::brig::BrigRegState *regs) {
  for (int i=0; i<8; i++) {
	if (regs->c[i] != prevRegState.c[i]) {
	  cout << "$c" << i << "=" << regs->c[i] << endl;
	}
  }
  for (int i=0; i<128; i++) {
	if (regs->s[i] != prevRegState.s[i]) {
		cout << dec << "$s" << i << "=" << regs->s[i] << ",  (0x" << setfill('0') << setw(8) << hex << regs->s[i] << "),  FP: " << *(float *)&(regs->s[i]) << endl;
	}
  }
  for (int i=0; i<64; i++) {
	if (regs->d[i] != prevRegState.d[i]) {
		cout << dec << "$d" << i << "=" << regs->d[i] << ",  (0x" << setfill('0') << setw(16) << hex << regs->d[i] << "),  FP: " << *(double *)&(regs->d[i]) << endl;
	}
  }
  cout << "\n";
  prevRegState = *regs;
}

