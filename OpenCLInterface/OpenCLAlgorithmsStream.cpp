#include "OpenCLAlgorithmsStream.h"

#include <algorithm>

#define ASSERT_OPENCL_ERR(ERR,MSG) if(ERR != CL_SUCCESS) \
{ \
  throw OpenCLAlgorithmsStreamException(MSG, ERR); \
}

OpenCLAlgorithmsStream::OpenCLAlgorithmsStream(void)
{
  width = height = 0;
  prepared = false;
}


OpenCLAlgorithmsStream::~OpenCLAlgorithmsStream(void)
{
  clearAlgorithms();
  //TODO: clear mems
}

void OpenCLAlgorithmsStream::pushAlgorithm(OpenCLAlgorithmForStream * al)
{
  if (!algorithms.empty() &&
    (algorithms.back()->output_image_format.image_channel_data_type != al->input_image_format.image_channel_data_type ||
    algorithms.back()->output_image_format.image_channel_order != al->input_image_format.image_channel_order)
    )//not first algorithm - need to check if data types for output of last algorithm and input of al is same
  {
    throw OpenCLAlgorithmsStreamException("Output from last algorithm (" + algorithms.back()->kernel_name + ") is not compatible with new algoriothm (" + al->kernel_name + ")");
  }
  algorithms.push_back(al);

  prepared = false;
}

void OpenCLAlgorithmsStream::clearAlgorithms()
{
  auto end = algorithms.end();
  for (auto al = algorithms.begin(); al != end; ++al)
  {
    delete *al; //TODO: Change after refactoring
    *al = 0;
  }
  algorithms.clear();
}

void OpenCLAlgorithmsStream::setDataSize(size_t w, size_t h)
{
  width = w;
  height = h;
}

void OpenCLAlgorithmsStream::prepare()
{
  if (algorithms.empty())
  {
    throw OpenCLAlgorithmsStreamException("Not provided any algoritm for algorithms stream");
  }

  //create all kernels
  std::for_each(algorithms.begin(), algorithms.end(), [this](OpenCLAlgorithmForStream* al)
  {
    al->width = width;
    al->height = height;
    al->setDevice(device);
    al->prepareForStream(/*command_queue, context*/);
  });

  //creates input and outputs
  cl_int err;

  //input
  input = clCreateImage2D(context, CL_MEM_READ_ONLY, &algorithms.front()->input_image_format, width, height, 0, NULL, &err);
  ASSERT_OPENCL_ERR(err, "Error while creating image2D for input");
  
  err = clSetKernelArg(algorithms.front()->kernel, 0, sizeof(cl_mem), (void*) &input);
  ASSERT_OPENCL_ERR(err, "Cant set kernel arg 0 of first algorithm");
  
  //output
  output = clCreateImage2D(context, CL_MEM_WRITE_ONLY, &algorithms.back()->output_image_format, width, height, 0, NULL, &err);
  ASSERT_OPENCL_ERR(err, "Error while creating image2D for output");
  
  err = clSetKernelArg(algorithms.back()->kernel, 1, sizeof(cl_mem), (void*) &output);
  ASSERT_OPENCL_ERR(err, "Cant set kernel arg 1 of last algorithm");

  //middle - starts with first and finish in one before last
  auto end = --algorithms.end();
  auto next = ++algorithms.begin();
  for (auto al = algorithms.begin(); al != end; ++al, ++next)
  {
    cl_mem mem_tmp = clCreateImage2D(context, CL_MEM_READ_WRITE, &(*al)->output_image_format, width, height, 0, NULL, &err);
    ASSERT_OPENCL_ERR(err, "Error while creating image2D for input/output");

    //set kernel arg for this algorithm
    err = clSetKernelArg((*al)->kernel, 1, sizeof(cl_mem), (void*) &mem_tmp);
    ASSERT_OPENCL_ERR(err, "Cant set kernel arg 0 of middle algorithm: " + (*al)->kernel_name);

    //set kernel arg for next algorithm
    err = clSetKernelArg((*next)->kernel, 0, sizeof(cl_mem), (void*) &mem_tmp);
    ASSERT_OPENCL_ERR(err, "Cant set kernel arg 1 of middle algorithm: " + (*al)->kernel_name);

    //save cl_mem for releasing
    mems.push_back(mem_tmp);
  }

  prepared = true;
}

void OpenCLAlgorithmsStream::processImage(const void * data_input, void * data_output, unsigned int from)
{
  if (!prepared)
  {
    throw OpenCLAlgorithmsStreamException("Algorithms was not prepared");
  }
  cl_int err;
  time = 0;
  
  size_t origin[] = {0,0,0};
  size_t region[] = {width, height, 1};

  const size_t global_work_size[] = {width, height};
  
  err = clEnqueueWriteImage(command_queue, input, CL_FALSE, origin, region, 0, 0, data_input, 0, NULL, NULL);
  ASSERT_OPENCL_ERR(err, "Cant set equeue write input image");

  unsigned int i = 0;

  std::for_each(algorithms.begin(), algorithms.end(), [&global_work_size, &i, &from, this](OpenCLAlgorithmForStream *al)
  {
    if (i >= from)
    {
      al->runStream(global_work_size);
      time += al->getTime();
    }
  });

  err = clEnqueueReadImage(command_queue, output, CL_TRUE, origin, region, 0, 0, data_output, 0, NULL, NULL);
  ASSERT_OPENCL_ERR(err, "Cant enqueue read buffer")

   (*(--algorithms.end()))->obtainAdditionalOutput();
}

void OpenCLAlgorithmsStream::setDevice(OpenCLDevice & d)
{
  if (d.isValid())
  {
    device = d;
    context = device.getContext();
    command_queue = device.getCommandQueue();
    return;
  }
  throw OpenCLAlgorithmsStreamException("Invalid Device");
}

double OpenCLAlgorithmsStream::getTime()
{
  return time;
}

void * OpenCLAlgorithmsStream::getLastAlgorithmAdditionalOutput() const
{
  return (*(--algorithms.end()))->additionalOutput();
}
