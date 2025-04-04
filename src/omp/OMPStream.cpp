
// Copyright (c) 2015-16 Tom Deakin, Simon McIntosh-Smith,
// University of Bristol HPC
//
// For full license terms please see the LICENSE file distributed with this
// source code

#include <cstdlib>  // For aligned_alloc
#include "OMPStream.h"

#ifdef USE_ARIELAPI
#include "arielapi.h"
#endif

#ifdef USE_PAPI
#include <papi.h>
#endif

#ifndef ALIGNMENT
#define ALIGNMENT (2*1024*1024) // 2MB
#endif

template <class T>
OMPStream<T>::OMPStream(const int ARRAY_SIZE, int device)
{
  array_size = ARRAY_SIZE;

  // Allocate on the host
  this->a = (T*)aligned_alloc(ALIGNMENT, sizeof(T)*array_size);
  this->b = (T*)aligned_alloc(ALIGNMENT, sizeof(T)*array_size);
  this->c = (T*)aligned_alloc(ALIGNMENT, sizeof(T)*array_size);

#ifdef OMP_TARGET_GPU
  omp_set_default_device(device);
  T *a = this->a;
  T *b = this->b;
  T *c = this->c;
  // Set up data region on device
  #pragma omp target enter data map(alloc: a[0:array_size], b[0:array_size], c[0:array_size])
  {}
#endif

}

template <class T>
OMPStream<T>::~OMPStream()
{
#ifdef OMP_TARGET_GPU
  // End data region on device
  int array_size = this->array_size;
  T *a = this->a;
  T *b = this->b;
  T *c = this->c;
  #pragma omp target exit data map(release: a[0:array_size], b[0:array_size], c[0:array_size])
  {}
#endif
  free(a);
  free(b);
  free(c);
}

template <class T>
void OMPStream<T>::init_arrays(T initA, T initB, T initC)
{

#ifdef USE_PAPI
  int retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT) {
    fprintf(stderr, "PAPI library initialization error!\n");
    exit(1);
  }
  retval = PAPI_thread_init((unsigned long (*)(void))(omp_get_thread_num));
  if (retval != PAPI_OK) {
	  fprintf(stderr, "PAPI thread init error!\n");
	  exit(1);
  }
#endif
  int array_size = this->array_size;
#ifdef OMP_TARGET_GPU
  T *a = this->a;
  T *b = this->b;
  T *c = this->c;
  #pragma omp target teams distribute parallel for simd
#else
  #pragma omp parallel for
#endif
  for (int i = 0; i < array_size; i++)
  {
    a[i] = initA;
    b[i] = initB;
    c[i] = initC;
  }
  #if defined(OMP_TARGET_GPU) && defined(_CRAYC)
  // If using the Cray compiler, the kernels do not block, so this update forces
  // a small copy to ensure blocking so that timing is correct
  #pragma omp target update from(a[0:0])
  #endif
}

template <class T>
void OMPStream<T>::read_arrays(std::vector<T>& h_a, std::vector<T>& h_b, std::vector<T>& h_c)
{

#ifdef OMP_TARGET_GPU
  T *a = this->a;
  T *b = this->b;
  T *c = this->c;
  #pragma omp target update from(a[0:array_size], b[0:array_size], c[0:array_size])
  {}
#endif

  #pragma omp parallel for
  for (int i = 0; i < array_size; i++)
  {
    h_a[i] = a[i];
    h_b[i] = b[i];
    h_c[i] = c[i];
  }

}

template <class T>
void OMPStream<T>::copy()
{

#ifdef USE_ARIELAPI
  ariel_output_stats();
  ariel_enable();
#endif

#ifdef OMP_TARGET_GPU
  int array_size = this->array_size;
  T *a = this->a;
  T *c = this->c;
  #pragma omp target teams distribute parallel for simd
#else
  #pragma omp parallel
  {
#ifdef USE_PAPI
  // Create an event set
  int event_set = PAPI_NULL;
  long long values[1];
  int retval = PAPI_create_eventset(&event_set);
  if (retval != PAPI_OK) {
	  fprintf(stderr, "PAPI create event set error!\n");
	  exit(1);
  }

  // Add the L1D cache read event to the event set
  //retval = PAPI_add_named_event(event_set, "perf::PERF_COUNT_HW_CACHE_L1D:READ");
  retval = PAPI_add_named_event(event_set, "perf::L1-DCACHE-LOADS:u=1:k=0");
  if (retval != PAPI_OK) {
	  fprintf(stderr, "PAPI add event error!\n");
	  exit(1);
  }

  // Start counting the event
  retval = PAPI_start(event_set);
  if (retval != PAPI_OK) {
	  fprintf(stderr, "PAPI start counters error!\n");
	  exit(1);
  }

#endif

  #pragma omp for
#endif
  for (int i = 0; i < array_size; i++)
  {
    c[i] = a[i];
  }
#ifndef OMP_TARGET_GPU
#ifdef USE_PAPI
  // Stop counting the event
  retval = PAPI_stop(event_set, values);
  if (retval != PAPI_OK) {
	  fprintf(stderr, "PAPI stop counters error!\n");
	  exit(1);
  }

  // Print the counter value
#pragma omp critical
  {
  printf("L1D cache reads: %lld\n", values[0]);
  }

  // Cleanup
  retval = PAPI_cleanup_eventset(event_set);
  if (retval != PAPI_OK) {
	  fprintf(stderr, "PAPI cleanup event set error!\n");
	  exit(1);
  }

  retval = PAPI_destroy_eventset(&event_set);
  if (retval != PAPI_OK) {
	  fprintf(stderr, "PAPI destroy event set error!\n");
	  exit(1);
  }
#endif
  }
#endif
  #if defined(OMP_TARGET_GPU) && defined(_CRAYC)
  // If using the Cray compiler, the kernels do not block, so this update forces
  // a small copyeto ensure blocking so that timing is correct
  #pragma omp target update from(a[0:0])
  #endif

#ifdef USE_ARIELAPI
  ariel_disable();
  ariel_output_stats();
#endif


}

template <class T>
void OMPStream<T>::mul()
{
  const T scalar = startScalar;

#ifdef OMP_TARGET_GPU
  int array_size = this->array_size;
  T *b = this->b;
  T *c = this->c;
  #pragma omp target teams distribute parallel for simd
#else
  #pragma omp parallel for
#endif
  for (int i = 0; i < array_size; i++)
  {
    b[i] = scalar * c[i];
  }
  #if defined(OMP_TARGET_GPU) && defined(_CRAYC)
  // If using the Cray compiler, the kernels do not block, so this update forces
  // a small copy to ensure blocking so that timing is correct
  #pragma omp target update from(c[0:0])
  #endif
}

template <class T>
void OMPStream<T>::add()
{
#ifdef OMP_TARGET_GPU
  int array_size = this->array_size;
  T *a = this->a;
  T *b = this->b;
  T *c = this->c;
  #pragma omp target teams distribute parallel for simd
#else
  #pragma omp parallel for
#endif
  for (int i = 0; i < array_size; i++)
  {
    c[i] = a[i] + b[i];
  }
  #if defined(OMP_TARGET_GPU) && defined(_CRAYC)
  // If using the Cray compiler, the kernels do not block, so this update forces
  // a small copy to ensure blocking so that timing is correct
  #pragma omp target update from(a[0:0])
  #endif
}

template <class T>
void OMPStream<T>::triad()
{
  const T scalar = startScalar;

#ifdef OMP_TARGET_GPU
  int array_size = this->array_size;
  T *a = this->a;
  T *b = this->b;
  T *c = this->c;
  #pragma omp target teams distribute parallel for simd
#else
  #pragma omp parallel for
#endif
  for (int i = 0; i < array_size; i++)
  {
    a[i] = b[i] + scalar * c[i];
  }
  #if defined(OMP_TARGET_GPU) && defined(_CRAYC)
  // If using the Cray compiler, the kernels do not block, so this update forces
  // a small copy to ensure blocking so that timing is correct
  #pragma omp target update from(a[0:0])
  #endif
}

template <class T>
void OMPStream<T>::nstream()
{
  const T scalar = startScalar;

#ifdef OMP_TARGET_GPU
  int array_size = this->array_size;
  T *a = this->a;
  T *b = this->b;
  T *c = this->c;
  #pragma omp target teams distribute parallel for simd
#else
  #pragma omp parallel for
#endif
  for (int i = 0; i < array_size; i++)
  {
    a[i] += b[i] + scalar * c[i];
  }
  #if defined(OMP_TARGET_GPU) && defined(_CRAYC)
  // If using the Cray compiler, the kernels do not block, so this update forces
  // a small copy to ensure blocking so that timing is correct
  #pragma omp target update from(a[0:0])
  #endif
}

template <class T>
T OMPStream<T>::dot()
{
  T sum{};

#ifdef OMP_TARGET_GPU
  int array_size = this->array_size;
  T *a = this->a;
  T *b = this->b;
  #pragma omp target teams distribute parallel for simd map(tofrom: sum) reduction(+:sum)
#else
  #pragma omp parallel for reduction(+:sum)
#endif
  for (int i = 0; i < array_size; i++)
  {
    sum += a[i] * b[i];
  }

  return sum;
}



void listDevices(void)
{
#ifdef OMP_TARGET_GPU
  // Get number of devices
  int count = omp_get_num_devices();

  // Print device list
  if (count == 0)
  {
    std::cerr << "No devices found." << std::endl;
  }
  else
  {
    std::cout << "There are " << count << " devices." << std::endl;
  }
#else
  std::cout << "0: CPU" << std::endl;
#endif
}

std::string getDeviceName(const int)
{
  return std::string("Device name unavailable");
}

std::string getDeviceDriver(const int)
{
  return std::string("Device driver unavailable");
}
template class OMPStream<float>;
template class OMPStream<double>;
