#pragma once

#include "OpenCLDevice.h"
#include "OpenCLException.h"
#include "OpenCLCommon.h"

/** Exception for OpenCL3DImageAlgorithm 
 * 
 */
class OpenCL3DImageException : public OpenCLException
{
public:

  OpenCL3DImageException(std::string m, int e = 0) : OpenCLException(m, e)
  {
  }
};

/** Class for algorithms manipulating 3D images.
 * Simple framework. It does not work with many NVIDIA devices, because they not support writing 3D images.
 * Should work on some AMD devices.
 */
class OpenCL3DImageAlgorithm : public OpenCLCommon
{
public:
  /**
   * Default contructor.
   */
  OpenCL3DImageAlgorithm(void);

  /**
   * Destructor.
   */
  ~OpenCL3DImageAlgorithm(void);

  /**
   * Set 3D image size.
   * @param w Image width.
   * @param h Image height.
   * @param d Image depth.
   */
  void setDataSize(size_t w, size_t h, size_t d);

  /**
   * Prepare algorithm.
   */
  void prepare();

  /**
   * Process 3D image.
   * @param data_input Pointer to continious memory with input 3D image. 
   * @param data_output  Pointer to continious memory to store output 3D image. 
   */
  void processImage(const void * data_input, void * data_output);

  /**
   * Get gime consumed on algorithm execution.
   * @return Time in ms.
   */
  double getTime();
protected:
  size_t height, width, depth;

  double time;

  cl_mem input_image_memory;
  cl_mem output_image_memory;

  cl_image_format input_image_format;
  cl_image_format output_image_format;

  bool prepared;

private:
  /**
   * Clears algorithm.
   */
  void clearAlgorithm();
};
