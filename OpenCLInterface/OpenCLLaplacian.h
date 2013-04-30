#pragma once
#include "OpenCLAlgorithm.h"

/** Parametes for OpenCLLaplacian.
 * Only paramter to set is sigma - gain to output pixel.
 */
class OpenCLLaplacianParams : public OpenCLAlgorithmParams
{
  unsigned int sigma;
  friend class OpenCLLaplacian;

public:
  /**
   * Set gain.
   * @param gain usigned int.
   */
  void setSigma(unsigned int sigma);
};


/** Algoritm performs Laplacian on image.
 * 
 */
class OpenCLLaplacian : public OpenCLImageAlgorithm
{
public:
  OpenCLLaplacian(void);
  ~OpenCLLaplacian(void);

  //from OpenCLAlgorithm
  void setParams(const OpenCLAlgorithmParams & params);
  void setParams(const OpenCLLaplacianParams & params);
  void run(const unsigned char * data_input, size_t di_size, unsigned char * data_output, size_t do_size);
  void setKernelArgs(size_t di_size, size_t do_size);
  
  void releaseMem();

protected:
  //from OpenCLImageAlgorithm
  void copyDataToGPUStream();
  void setKernelArgsForStream();

private:
  //gaussian kernel
  void * laplacian;
  unsigned int size;
  unsigned int size_to_pass;
  int sigma;

  cl_image_format laplacian_format;
  cl_mem laplacian_memory;
  cl_mem size_memory;
  cl_mem sigma_memory;
};

