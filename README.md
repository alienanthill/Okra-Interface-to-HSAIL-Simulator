Okra-Interface-to-HSAIL-Simulator
=================================

A thin Java and C++ layer on top of the simulator which allows creation and dispatching of HSAIL kernels.

Build Prequisites
    java jdk, with JAVA_HOME environment variable
    ant, with ANT_HOME environment variable
    git
    # the following are prequisites of the simulator or libhsail
    # and will be checked for in the build-okra-sim.xml below
    svn
    libelf-dev
    libdwarf-dev
    flex
    cmake

To build:
    git clone https://github.com/HSAFoundation/Okra-Interface-to-HSAIL-Simulator.git okra
    cd okra
    ant -f build-okra-sim.xml

    Now add okra/dist/bin to your PATH and to your LD_LIBRARY_PATH environment variables

Sanity Test (C++):
    cd okra/samples
    chmod +x *.sh
    ./build.sh
    ./run.sh

Sanity Test (Java):
    cd okra/samples.jni
    chmod +x *.sh
    ./build.sh
    ./run.sh

