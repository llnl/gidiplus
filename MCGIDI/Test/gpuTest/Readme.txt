This tests MCGIDI on the GPU. To compile this, first build GIDI with nvcc / rocm.


Then make this test with the following command after changing the directory to MCGIDI/Test/gpuTest:

For RZAnsel Cuda11 opt:
gmake CXX=/usr/tce/packages/cuda/cuda-11.7.0/bin/nvcc CXXFLAGS='-x cu --relocatable-device-code=true -lineinfo -g -O2 -std=c++11 -gencode=arch=compute_70,code=sm_70 -I$(CUDA_PATH)/include'

For RZadams Rocm 6.2.4 (note -O0 in CXXFLAGS):
gmake CXX=/usr/tce/packages/cray-mpich/cray-mpich-8.1.31-rocmcc-6.2.4-cce-18.0.1c-magic/bin/mpiamdclang++ CXXFLAGS='-g -ggdb -O0 -std=c++14 -ffp-contract=off -w -x hip -fgpu-rdc --hip-link --offload-arch="""gfx942""" -w -D __HIP__ -DLUPI_HIP_INLINE -I/usr/tce/packages/cray-mpich/cray-mpich-8.1.31-rocmcc-6.2.4-cce-18.0.1c-magic/include' HDF5_LIBS='/usr/gapps/bdiv/toss_4_x86_64_ib_cray/rocmcc-6.2.4-cce-18.0.1c/hdf5/1.14.3/lib/libhdf5.a'

To run it, grab a process like
RZansel:
lalloc 1

RZadams:
flux alloc -N 1

And then execute it like
gputest <doPrint> <numCollisions> <numIsotopes> <doCompare>

default is
gpuTest 0 100000 1 0

doPrint - If nonzero, print out the list of reactions on the original host, unpacked host, and gpu for the last isotope
numCollisions - Sample these number of collisions on the CPU and GPU
numIsotopes - Number of isotopes to load in. Collisions is only on last isotope. Up to 100.
doCompare - If 0, do nothing. If 1, copy the protare back from the GPU and write it to disk. If 2, copy the protare back from the GPU and compare to the one on disk.


For timing on rzansel use nvprof like

/usr/tce/packages/cuda/cuda-11.2.0-beta/bin/nvprof gpuTest 0 0 100 0

/usr/tce/packages/cuda/cuda-11.2.0-beta/bin/nvprof gpuTest 0 10000000 1 0

For cpu timing, try:
/usr/tce/packages/cuda/cuda-11.2.0-beta/bin/nvprof --cpu-profiling on gpuTest 0 10000000 1 0
