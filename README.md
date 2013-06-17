Okra-Interface-to-HSAIL-Simulator
=================================

A thin Java and C++ layer on top of the simulator which allows creation and dispatching of HSAIL kernels.

### Build Prequisites
_Note: the last 5 are prequisites of the simulator or libhsail and will be checked for in the_ "ant -f build-okra-sim.xml" _below_

  * java jdk, with JAVA_HOME environment variable set
  * ant, with ANT_HOME environment variable set
  * git
  * svn
  * libelf-dev
  * libdwarf-dev
  * flex
  * cmake

####To build:
* git clone https://github.com/HSAFoundation/Okra-Interface-to-HSAIL-Simulator.git okra
* cd okra
* ant -f build-okra-sim.xml
* Now add okra/dist/bin to your PATH and to your LD\_LIBRARY\_PATH environment variables

#### Sanity Test (C++):
  * cd okra/samples
  * chmod +x *.sh
  * ./build.sh
  * ./run.sh

#### Sanity Test (Java):
  * cd okra/samples.jni
  * chmod +x *.sh
  * ./build.sh
  * ./run.sh

