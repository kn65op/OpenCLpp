#include "OpenCLGaussian.h"

#define ASSERT_OPENCL_ERR(ERR,MSG) if(ERR != CL_SUCCESS) \
{ \
  throw OpenCLAlgorithmException(MSG, ERR); \
}

OpenCLGaussian::OpenCLGaussian(void)
{
}

OpenCLGaussian::~OpenCLGaussian(void)
{
}

void OpenCLGaussian::setParams(const OpenCLAlgorithmParams & params)
{
}

void OpenCLGaussian::prepare()
{
  command_queue = device.getCommandQueue();
  program = device.createAndBuildProgramFromFile("empty.cl");  // not supported
  cl_int err;
  kernel = clCreateKernel(program, "gaussian", &err);
  ASSERT_OPENCL_ERR(err, "Cant create kernel");
}

void OpenCLGaussian::run(const unsigned char * data_input, size_t di_size, unsigned char * data_output, size_t do_size)
{
  cl_mem input, output, mask_mem;
  cl_int err;

  float mask[] = {(float) 0.1, (float) 0.1, (float) 0.1, (float) 0.1, (float) 0.1, (float) 0.1, (float) 0.1, (float) 0.1, (float) 0.1};

  input = clCreateBuffer(device.getContext(), CL_MEM_READ_ONLY, di_size, NULL, &err);
  ASSERT_OPENCL_ERR(err, "Error while creating buffer input");

  output = clCreateBuffer(device.getContext(), CL_MEM_WRITE_ONLY, do_size, NULL, &err);
  ASSERT_OPENCL_ERR(err, "Error while creating buffer output");

  mask_mem = clCreateBuffer(device.getContext(), CL_MEM_READ_ONLY, sizeof (mask), NULL, &err);
  ASSERT_OPENCL_ERR(err, "Error while creating buffer mask_mem");

  err = clEnqueueWriteBuffer(command_queue, input, CL_TRUE, 0, di_size, data_input, 0, NULL, NULL);
  ASSERT_OPENCL_ERR(err, "Error while enqueing wirte buffer for input");

  err = clEnqueueWriteBuffer(command_queue, mask_mem, CL_TRUE, 0, sizeof (mask), &mask, 0, NULL, NULL);
  ASSERT_OPENCL_ERR(err, "Error while enqueing wirte buffer for mask");

  err = clSetKernelArg(kernel, 0, sizeof (cl_mem), (void*) &mask_mem);
  ASSERT_OPENCL_ERR(err, "Cant set kernel arg 0");

  err = clSetKernelArg(kernel, 1, sizeof (cl_mem), (void*) &input);
  ASSERT_OPENCL_ERR(err, "Cant set kernel arg 1");

  err = clSetKernelArg(kernel, 2, sizeof (cl_mem), (void*) &output);
  ASSERT_OPENCL_ERR(err, "Cant set kernel arg 2")

  size_t global_work_size[] = {498, 498};

  enqueueNDRangeKernelWithTimeMeasurment(2, NULL, global_work_size, NULL, 0);

  //read data
  err = clEnqueueReadBuffer(command_queue, output, CL_TRUE, 0, do_size, data_output, 0, NULL, NULL);
  ASSERT_OPENCL_ERR(err, "Can't enqueue read buffer, output");

  clReleaseMemObject(input);
  clReleaseMemObject(output);
  clReleaseMemObject(mask_mem);
}

void OpenCLGaussian::releaseMem()
{
}

void OpenCLGaussian::setKernelArgs(size_t di_size, size_t do_size)
{
}

/******************** OpenCLGaussanImage ***********************/

OpenCLGaussianImage::OpenCLGaussianImage(unsigned int gaussian_size)
{
  input_image_format.image_channel_data_type = CL_FLOAT;
  input_image_format.image_channel_order = CL_LUMINANCE;

  output_image_format.image_channel_data_type = CL_FLOAT;
  output_image_format.image_channel_order = CL_LUMINANCE;

  gaussian_format.image_channel_data_type = CL_FLOAT;
  gaussian_format.image_channel_order = CL_LUMINANCE;  

  kernel_name = "convolution";
  //source_filename = "convolution.cl";
  source =
"const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;\n"
"\n"
"\n"
"__kernel void  convolution(__read_only image2d_t input, __write_only image2d_t output, __global float * gaussian, __private __read_only uint size) \n"
"{\n"
"  int2 pos = {get_global_id(0), get_global_id(1)};\n"
  "float sum = 0.0;\n"
"  \n"
  "//int sizeq = size;\n"
  "int gi = 0;\n"
  "int j_gaussian;\n"
  "for (int i_gaussian = -SIZE; i_gaussian <= SIZE; ++i_gaussian)\n"
  "{\n"
"	for (j_gaussian = -SIZE; j_gaussian <= SIZE; ++j_gaussian)\n"
  "{\n"
    "sum += (read_imagef(input, sampler, pos + (int2)(i_gaussian, j_gaussian)).x * gaussian[gi++]);\n"
  "}\n"
  "}\n"
"  \n"
  "write_imagef(output, pos, sum);  \n"
"}\n"
"\n";

  size = 0;

  defines = "-D SIZE=" + std::to_string(gaussian_size);

  gaussian_memory = nullptr;
  size_memory = nullptr;

  gaussian = nullptr;
}

OpenCLGaussianImage::~OpenCLGaussianImage()
{
  if (gaussian_memory != nullptr)
  {
    ASSERT_OPENCL_ERR(clReleaseMemObject(gaussian_memory), "Error during releasing gaussian memory");
  }
  if (size_memory != nullptr)
  {
    ASSERT_OPENCL_ERR(clReleaseMemObject(size_memory), "Error during releasing gaussian memory");
  }
}

void OpenCLGaussianImage::setParams(const OpenCLAlgorithmParams& params)
{
  try
  {
    setParams(dynamic_cast<const OpenCLGaussianParams&> (params));
  }
  catch(std::bad_cast ex)
  {
    throw OpenCLAlgorithmException("Parameters provided is not OpenCLGaussianParams");
  }
}

void OpenCLGaussianImage::setParams(const OpenCLGaussianParams& params)
{
  if (gaussian != nullptr)
  {
    delete gaussian;
  }
  size = params.mask_size;
  size_to_pass = size / 2;
  unsigned int mem_size = size * size * sizeof(float);
  gaussian = new float[size * size];
  memcpy(gaussian, params.gaussian_mask, mem_size);
}

/*void OpenCLGaussianImage::prepareForStream(cl_command_queue cc, cl_context c)
{

}

void OpenCLGaussianImage::runStream(const size_t* global_work_size)
{

}
*/
void OpenCLGaussianImage::copyDataToGPUStream()
{
  cl_int err;
  size_t origin[] = {0,0,0};
  size_t region[] = {size, size, 1};

  /*for (int i=0; i<size; ++i)
  {
    std::cout << ((float*)gaussian)[0] << " ";
  }
  std::cout << "\n" << size_to_pass << "\n";*/

  /*err = clEnqueueWriteBuffer(command_queue, size_memory, CL_TRUE, 0, sizeof(size_to_pass), &size_to_pass, 0, NULL, NULL);
  ASSERT_OPENCL_ERR(err, "Error while enqueing wirte buffer for size");*/

  err = clEnqueueWriteBuffer(command_queue, gaussian_memory, CL_FALSE, 0, size * size * sizeof(float) , gaussian, 0, NULL, NULL);
  ASSERT_OPENCL_ERR(err, "Error while enqueue write gaussian buffer");

  /*err = clEnqueueWriteImage(command_queue, gaussian_memory, CL_FALSE, origin, region, 0, 0, gaussian, 0, NULL, NULL);
  ASSERT_OPENCL_ERR(err, "Error while enqueue write gaussian image");*/
}

void OpenCLGaussianImage::setKernelArgsForStream()
{
  cl_int err;

  /*gaussian_memory = clCreateImage2D(context, CL_MEM_READ_ONLY, &gaussian_format, size, size, 0, NULL, &err);
  ASSERT_OPENCL_ERR(err, "Error while creating gaussian image");*/

  gaussian_memory = clCreateBuffer(context, CL_MEM_READ_ONLY, size * size * sizeof(float), NULL, &err);
  ASSERT_OPENCL_ERR(err, "Error while creating gaussian buffer");

  //gaussian = new float[9];

  //size_to_pass = new unsigned int(1);

  err = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&gaussian_memory);
  ASSERT_OPENCL_ERR(err, "Error while setting as arg: gaussian image");

  /*size_memory = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(size_to_pass), NULL, &err);
  ASSERT_OPENCL_ERR(err, "Error while creating gaussian size");*/
  
  err = clSetKernelArg(kernel, 3, sizeof(size_to_pass), (void*)&size_to_pass);
  ASSERT_OPENCL_ERR(err, "Error while setting as arg: gaussian size");
}

void OpenCLGaussianImage::run(const unsigned char * data_input, size_t di_size, unsigned char * data_output, size_t do_size)
{
  //nothing to do here
}

void OpenCLGaussianImage::releaseMem()
{
  //nothing to do here
}

void OpenCLGaussianImage::setKernelArgs(size_t di_size, size_t do_size)
{
  //nothing to do here
}

/************************************ OpenCLGaussianParams ********************************************/

void OpenCLGaussianParams::setMask(int size, void * mask)
{
  if (size % 2 != 1)
  {
    throw OpenCLAlgorithmException("Mask size has to be odd");
  }
  mask_size = size;
  gaussian_mask = mask;
}

