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
	friend OkraContext * OkraContext::Create(); 
	
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
	
		OkraStatus argsPushBack(hsa::KernelArg *harg) {
			hsaArgs.push_back(*harg);
			return OKRA_OK;
		}
		
		OkraStatus pushFloatArg(jfloat f) {
			hsa::KernelArg harg;
			harg.fvalue = f;
			return argsPushBack(&harg);
		}
	
		OkraStatus pushIntArg(jint i) {
			hsa::KernelArg harg;
			harg.s32value = i;
			return argsPushBack(&harg);
		}
	
		OkraStatus pushBooleanArg(jboolean z) {
			hsa::KernelArg harg;
			harg.u32value = z;
			return argsPushBack(&harg);
		}
	
		OkraStatus pushByteArg(jbyte b) {
			hsa::KernelArg harg;
			harg.s32value = b;  //not sure if this is right, verify later
			return argsPushBack(&harg);
		}
	
		OkraStatus pushLongArg(jlong j) {
			hsa::KernelArg harg;
			harg.s64value = j; 
			return argsPushBack(&harg);
		}

		OkraStatus pushDoubleArg(jdouble d) {
			hsa::KernelArg harg;
			harg.dvalue = d; 
			return argsPushBack(&harg);
		}
		
		
		OkraStatus pushPointerArg(void *addr) {
			//add the kernelarg for hsa runtime into the vector
			hsa::KernelArg harg;
			harg.addr = addr;
			if (context->isVerbose()) cerr<<"pushPointerArg, addr=" << addr <<endl;
			return argsPushBack(&harg);
		}

		// allow a previously pushed arg to be changed
		OkraStatus setPointerArg(int idx, void *addr) {
			hsa::KernelArg harg;
			harg.addr = addr;
			if (context->isVerbose()) cerr<<"setPointerArg, addr=" << addr <<endl;
			hsaArgs.at(idx) = harg;
			return OKRA_OK;
		}

		OkraStatus clearArgs() {
			hsaArgs.clear();
			return OKRA_OK;
		}

		OkraStatus dispatchKernelWaitComplete() {
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
			return OKRA_OK;
		}

		
		// NOTE: the okra "spec" treates globalDims as NDRangeSize
		// whereas the simulator currently treats what we call
		// globalDims as the number of grid blocks (so NDRangeSize =
		// grid * group).  So we need to do the conversion here.

		OkraStatus setLaunchAttributes(int dims, size_t *globalDims, size_t *localDims) {
			for (int k=0; k<dims; k++) {
				computeLaunchAttr(k, globalDims[k], localDims[k]);
			}
			return OKRA_OK;
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

	};

private:
	hsa::RuntimeApi *hsaRT;
	uint32_t numDevices;
	hsacommon::vector<hsa::Device *> devices;
	hsa::Queue *hsaQueue;
	int maxSimThreads;
	bool saveHsailSource;

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
	Kernel * createKernel(const char *hsailBuffer, const char *entryName) {
		string *fixedHsailStr = fixHsail(hsailBuffer);
		
		ofstream ofs("temp_hsa.hsail");
		if (isVerbose()) cerr << "Fixed Hsail is\n==============\n" << *fixedHsailStr << endl;
		ofs  << *fixedHsailStr;
		ofs.close();
		delete(fixedHsailStr);

		// writeToFile(fixedHsailBuffer, strlen(fixedHsailBuffer), (char *) "temp_hsa.hsail");

		// use the -build hsailasm to translate source
		// use debug flag
		// spawnProgram("which hsailasm");
		int ret = spawnProgram("hsailasm temp_hsa.hsail -g -o temp_hsa.o");
		if (ret != 0) return NULL;
		if (isVerbose()) cerr << "hsailasm succeeded\n";

		size_t brigSize = 0;
		char *brigBuffer = readFile("./temp_hsa.o", brigSize);
		if (brigBuffer == NULL) {
			printf("cannot read from the temp_hsa.o file\n"); 
			return NULL;
		}
		// delete temporary files
		remove("temp_hsa.o");
		if (!saveHsailSource) {
			remove("temp_hsa.hsail");
		}

		return createKernelCommon(brigBuffer, brigSize, entryName);
	}

	Kernel * createKernelFromBinary(const char *brigBuffer, size_t brigSize, const char *entryName) {
		char *ptr = reinterpret_cast<char*>(malloc(brigSize));
		if (!ptr) {
			return NULL;
		}

		memcpy(ptr, brigBuffer, brigSize);
		return createKernelCommon(ptr, brigSize, entryName);
	}

	OkraStatus dispose(){
#if 0
		if (hsaProgram) {
			hsaRuntime->destroyProgram(hsaProgram);
		}
#endif
		return OKRA_OK;
	}

	OkraStatus registerArrayMemory(void *addr, jint lengthInBytes) {
#if 0
		hsaDevices[0]->registerMemory(addr, lengthInBytes);
#endif
		return OKRA_OK;
	}

private:
	Kernel * createKernelCommon(char *brigBuffer, size_t brigSize, const char *entryName) {
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



	static OkraStatus mapHsaErrorToOkra(hsa::Status st) {
		return (st == hsa::RSTATUS_SUCCESS ? OKRA_OK : OKRA_OTHER_ERROR);
	}
}; // end of OkraContextSimulatorImpl

bool OkraContext::isSimulator() {
	return true;
}

// simulator only supports coherent model
OkraContext::OkraStatus  OkraContext::setCoherence(bool isCoherent) {
	return OKRA_OK; 
}

bool  OkraContext::getCoherence() {
	return true; 
}

// Create an instance thru the OkraContext interface
// DLLExport
DLLExport
OkraContext * OkraContext::Create() {		
	return new OkraContextSimulatorImpl();
}

extern "C" DLLExport void  commitAndRegisterWholeHeap(void *startAddr, void *endAddr) {
    // Nothing to here do for simulator
    return;
}

extern "C" DLLExport void * vqueryLargest(void *addr, size_t *pSize, bool show) {
    // Nothing to here do for simulator
    cerr<<"Should not call vqueryLargest in Simulator"<<endl;
    return NULL;
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

