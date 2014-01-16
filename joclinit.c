/*
 * joclinit.c
 *
 * Copyright (C) 2012-2013, MulticoreWare Inc.
 * In July 2012, Written by Peixuan Zhang <zhangpeixuan.cn@gmail.com>
 * Copyright (C) 2014, D. R. Commander.
 * Based on the OpenCL extension for IJG JPEG library,
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the initialization of OpenCL.
 */

#define __JOCL_CL_INIT_MAIN__
#include "jinclude.h"
#define JPEG_INTERNALS
#include "jpeglib.h"
#ifdef WITH_OPENCL_DECODING_SUPPORTED

#include "CL/opencl.h"
#include "joclinit.h"
#undef  __JOCL_CL_INIT_MAIN__

#ifdef JOCL_CL_OS_WIN32
#define CL_QUEUE_THREAD_HANDLE_AMD 0x403E
#define CL_PARAM_NUM 20
#endif

//static OCL_STATUS ocl_status = {NULL, NULL, NULL, NULL, CL_FALSE, CL_FALSE, CL_FALSE, CL_FALSE, CL_FALSE, CL_FALSE, 0};


/*
 * jocl_cl_errstring
 * Translate error code into string, support for the OpenCL 1.2
 */

const char* jocl_cl_errstring(cl_int err_code)
{
  static const char* strings[] =
  {
      "CL_SUCCESS"                                             /*   0 */
    , "CL_DEVICE_NOT_FOUND"                                    /*  -1 */
    , "CL_DEVICE_NOT_AVAILABLE"                                /*  -2 */
    , "CL_COMPILER_NOT_AVAILABLE"                              /*  -3 */
    , "CL_MEM_OBJECT_ALLOCATION_FAILURE"                       /*  -4 */
    , "CL_OUT_OF_RESOURCES"                                    /*  -5 */
    , "CL_OUT_OF_HOST_MEMORY"                                  /*  -6 */
    , "CL_PROFILING_INFO_NOT_AVAILABLE"                        /*  -7 */
    , "CL_MEM_COPY_OVERLAP"                                    /*  -8 */
    , "CL_IMAGE_FORMAT_MISMATCH"                               /*  -9 */
    , "CL_IMAGE_FORMAT_NOT_SUPPORTED"                          /* -10 */
    , "CL_BUILD_PROGRAM_FAILURE"                               /* -11 */
    , "CL_MAP_FAILURE"                                         /* -12 */
    , "CL_MISALIGNED_SUB_BUFFER_OFFSET"                        /* -13 */
    , "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST"           /* -14 */
    , "CL_COMPILE_PROGRAM_FAILURE"                             /* -15 */
    , "CL_LINKER_NOT_AVAILABLE"                                /* -16 */
    , "CL_LINK_PROGRAM_FAILURE"                                /* -17 */
    , "CL_DEVICE_PARTITION_FAILED"                             /* -18 */
    , "CL_KERNEL_ARG_INFO_NOT_AVAILABLE"                       /* -19 */
    , "UNDEFINED_ERROR_CODE"                                   /* -20 */
    , "UNDEFINED_ERROR_CODE"                                   /* -21 */
    , "UNDEFINED_ERROR_CODE"                                   /* -22 */
    , "UNDEFINED_ERROR_CODE"                                   /* -23 */
    , "UNDEFINED_ERROR_CODE"                                   /* -24 */
    , "UNDEFINED_ERROR_CODE"                                   /* -25 */
    , "UNDEFINED_ERROR_CODE"                                   /* -26 */
    , "UNDEFINED_ERROR_CODE"                                   /* -27 */
    , "UNDEFINED_ERROR_CODE"                                   /* -28 */
    , "UNDEFINED_ERROR_CODE"                                   /* -29 */
    , "CL_INVALID_VALUE"                                       /* -30 */
    , "CL_INVALID_DEVICE_TYPE"                                 /* -31 */
    , "CL_INVALID_PLATFORM"                                    /* -32 */
    , "CL_INVALID_DEVICE"                                      /* -33 */
    , "CL_INVALID_CONTEXT"                                     /* -34 */
    , "CL_INVALID_QUEUE_PROPERTIES"                            /* -35 */
    , "CL_INVALID_COMMAND_QUEUE"                               /* -36 */
    , "CL_INVALID_HOST_PTR"                                    /* -37 */
    , "CL_INVALID_MEM_OBJECT"                                  /* -38 */
    , "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR"                     /* -39 */
    , "CL_INVALID_IMAGE_SIZE"                                  /* -40 */
    , "CL_INVALID_SAMPLER"                                     /* -41 */
    , "CL_INVALID_BINARY"                                      /* -42 */
    , "CL_INVALID_BUILD_OPTIONS"                               /* -43 */
    , "CL_INVALID_PROGRAM"                                     /* -44 */
    , "CL_INVALID_PROGRAM_EXECUTABLE"                          /* -45 */
    , "CL_INVALID_KERNEL_NAME"                                 /* -46 */
    , "CL_INVALID_KERNEL_DEFINITION"                           /* -47 */
    , "CL_INVALID_KERNEL"                                      /* -48 */
    , "CL_INVALID_ARG_INDEX"                                   /* -49 */
    , "CL_INVALID_ARG_VALUE"                                   /* -50 */
    , "CL_INVALID_ARG_SIZE"                                    /* -51 */
    , "CL_INVALID_KERNEL_ARGS"                                 /* -52 */
    , "CL_INVALID_WORK_DIMENSION"                              /* -53 */
    , "CL_INVALID_WORK_GROUP_SIZE"                             /* -54 */
    , "CL_INVALID_WORK_ITEM_SIZE"                              /* -55 */
    , "CL_INVALID_GLOBAL_OFFSET"                               /* -56 */
    , "CL_INVALID_EVENT_WAIT_LIST"                             /* -57 */
    , "CL_INVALID_EVENT"                                       /* -58 */
    , "CL_INVALID_OPERATION"                                   /* -59 */
    , "CL_INVALID_GL_OBJECT"                                   /* -60 */
    , "CL_INVALID_BUFFER_SIZE"                                 /* -61 */
    , "CL_INVALID_MIP_LEVEL"                                   /* -62 */
    , "CL_INVALID_GLOBAL_WORK_SIZE"                            /* -63 */
    , "CL_INVALID_PROPERTY"                                    /* -64 */
    , "CL_INVALID_IMAGE_DESCRIPTOR"                            /* -65 */
    , "CL_INVALID_COMPILER_OPTIONS"                            /* -66 */
    , "CL_INVALID_LINKER_OPTIONS"                              /* -67 */
    , "CL_INVALID_DEVICE_PARTITION_COUNT"                      /* -68 */
  };

  if ( err_code > 0 || err_code < -68 )
    return "CL_UNKNOWN_ERROR_CODE";
  return strings[-err_code];
}


/*
 * jocl_cl_init
 * Dynamic loading the OpenCL API that need to use.
 * Get and select a platform_id, a device_id, a context and a command_queue.
 * If initialization is successful, return CL_TRUE, otherwise return CL_FALSE.
 */
#ifdef JOCL_CL_OS_WIN32

#include <Windows.h>
#define CL_LOAD_FUNCTION(func)                                                 \
  if ((jocl_##func = (h_##func) GetProcAddress(module, #func)) == NULL)        \
  {                                                                            \
    CL_DEBUG_NOTE ("symbol jocl_%s is NULL\n",#func);                          \
    FreeLibrary(module);                                                       \
    return CL_FALSE;                                                           \
  }

#else

#ifdef __APPLE__
#define CL_LIBRARY_NAME "/System/Library/Frameworks/OpenCL.framework/Versions/Current/OpenCL"
#else
#define CL_LIBRARY_NAME "libOpenCL.so"
#endif 

#include<dlfcn.h>
#define CL_LOAD_FUNCTION(func)                                                 \
  if ((jocl_##func = (h_##func) dlsym(module, #func)) == NULL)                 \ 
  {                                                                            \
    CL_DEBUG_NOTE ("symbol jocl_%s is NULL\n",#func);                          \
    dlclose(module);                                                           \
    return CL_FALSE;                                                           \
  }
#endif
cl_bool jocl_cl_init(void **jocl_openClinfo)
{
    char*  platformVendor;
    char * deviceName ;
    char *ocl_disable;
#ifdef JOCL_CL_OS_WIN32
  void *handle = INVALID_HANDLE_VALUE;
  int ret;
#endif

  OCL_STATUS * ocl_status;
  if( NULL == *jocl_openClinfo)
  {

      *jocl_openClinfo =  (void *)malloc(sizeof(OCL_STATUS));
      if( NULL == *jocl_openClinfo)
      {
          return -1;
      }
      memset(*jocl_openClinfo,0,sizeof(OCL_STATUS));
      ocl_status = ((OCL_STATUS *)(*jocl_openClinfo));
      ocl_status->is_opencl_available = CL_FALSE;
      ocl_status->is_opencl_support = CL_FALSE;
      ocl_status->fancy_index = CL_FALSE;

  }
  ocl_disable = getenv("LJT_DISABLE_OPENCL");
  if(ocl_disable != NULL)
  {
      if( 0 == strcmp("TRUE",ocl_disable))
      {
          ocl_status->is_opencl_available = CL_FALSE;
          ocl_status->is_opencl_support = CL_FALSE;
          ocl_status->fancy_index = CL_FALSE;
          return CL_FALSE;
      }
  }
  if (!ocl_status->is_opencl_available) {
    cl_int          err_code;
    cl_uint         num_platform;
    cl_uint         num_device;
    cl_platform_id* pids;
    cl_device_id*   dids;
    cl_uint         index;
	unsigned int    num_gpu_device = 0;
    char*           version_ocl_inter = NULL;
    char*           version_ocl = NULL;
    int             version_length = 0;
#ifdef JOCL_CL_OS_WIN32
    HINSTANCE       module;
    module = LoadLibraryW(L"OpenCL.dll");
#else
    void *module;
    module = dlopen(CL_LIBRARY_NAME, RTLD_LAZY);
#endif

    if(module == NULL) {
      CL_DEBUG_NOTE ("Loading OpenCL Library fails!!\n");
      return CL_FALSE;
    }

    /* Dynamic loading OpenCL API from library */
    CL_LOAD_FUNCTION(clGetPlatformIDs                 );
    CL_LOAD_FUNCTION(clGetPlatformInfo                );
    CL_LOAD_FUNCTION(clGetDeviceIDs                   );
    CL_LOAD_FUNCTION(clGetDeviceInfo                  );
    /* CL_LOAD_FUNCTION(clCreateSubDevices            );
    CL_LOAD_FUNCTION(clRetainDevice                   );
    CL_LOAD_FUNCTION(clReleaseDevice                  );*/
    CL_LOAD_FUNCTION(clCreateContext                  );
    /* CL_LOAD_FUNCTION(clCreateContextFromType       );
    CL_LOAD_FUNCTION(clRetainContext                  );*/
    CL_LOAD_FUNCTION(clReleaseContext                 );
    /* CL_LOAD_FUNCTION(clGetContextInfo              );*/
    CL_LOAD_FUNCTION(clCreateCommandQueue             );
    /* CL_LOAD_FUNCTION(clRetainCommandQueue          );*/
    CL_LOAD_FUNCTION(clReleaseCommandQueue            );
    CL_LOAD_FUNCTION(clGetCommandQueueInfo            );
    CL_LOAD_FUNCTION(clCreateBuffer                   );
    CL_LOAD_FUNCTION(clCreateSubBuffer                );
    /* CL_LOAD_FUNCTION(clCreateImage                 );
    CL_LOAD_FUNCTION(clRetainMemObject                );*/
    CL_LOAD_FUNCTION(clReleaseMemObject               );
    /* CL_LOAD_FUNCTION(clGetSupportedImageFormats    );*/
    CL_LOAD_FUNCTION(clGetMemObjectInfo               );
    /* CL_LOAD_FUNCTION(clGetImageInfo                );
    CL_LOAD_FUNCTION(clSetMemObjectDestructorCallback );
    CL_LOAD_FUNCTION(clCreateSampler                  );
    CL_LOAD_FUNCTION(clRetainSampler                  );
    CL_LOAD_FUNCTION(clReleaseSampler                 );
    CL_LOAD_FUNCTION(clGetSamplerInfo                 );*/
    CL_LOAD_FUNCTION(clCreateProgramWithSource        );
    CL_LOAD_FUNCTION(clCreateProgramWithBinary        );
    /* CL_LOAD_FUNCTION(clCreateProgramWithBuiltInKernels);
    CL_LOAD_FUNCTION(clRetainProgram                  );*/
    CL_LOAD_FUNCTION(clReleaseProgram                 );
    CL_LOAD_FUNCTION(clBuildProgram                   );
    /* CL_LOAD_FUNCTION(clCompileProgram              );
    CL_LOAD_FUNCTION(clLinkProgram                    );
    CL_LOAD_FUNCTION(clUnloadPlatformCompiler         );*/
    CL_LOAD_FUNCTION(clGetProgramInfo                 );
    CL_LOAD_FUNCTION(clGetProgramBuildInfo            );
    CL_LOAD_FUNCTION(clCreateKernel                   );
    /* CL_LOAD_FUNCTION(clCreateKernelsInProgram      );
    CL_LOAD_FUNCTION(clRetainKernel                   );*/
    CL_LOAD_FUNCTION(clReleaseKernel                  );
    CL_LOAD_FUNCTION(clSetKernelArg                   );
    /*CL_LOAD_FUNCTION(clGetKernelInfo                );
    CL_LOAD_FUNCTION(clGetKernelArgInfo               );*/
    CL_LOAD_FUNCTION(clGetKernelWorkGroupInfo         );
    CL_LOAD_FUNCTION(clWaitForEvents                  );
    CL_LOAD_FUNCTION(clGetEventInfo                   );
    CL_LOAD_FUNCTION(clCreateUserEvent                );
    /* CL_LOAD_FUNCTION(clRetainEvent                 );*/
    CL_LOAD_FUNCTION(clReleaseEvent                   );
    CL_LOAD_FUNCTION(clSetUserEventStatus             );
    /*  CL_LOAD_FUNCTION(clSetEventCallback           );
    CL_LOAD_FUNCTION(clGetEventProfilingInfo          );*/
    CL_LOAD_FUNCTION(clFlush                          );
    CL_LOAD_FUNCTION(clFinish                         );
    CL_LOAD_FUNCTION(clEnqueueReadBuffer              );
    /* CL_LOAD_FUNCTION(clEnqueueReadBufferRect       );*/
    CL_LOAD_FUNCTION(clEnqueueWriteBuffer             );
    /* CL_LOAD_FUNCTION(clEnqueueWriteBufferRect      );
    CL_LOAD_FUNCTION(clEnqueueFillBuffer              );
    CL_LOAD_FUNCTION(clEnqueueCopyBuffer              );
    CL_LOAD_FUNCTION(clEnqueueCopyBufferRect          );
    CL_LOAD_FUNCTION(clEnqueueReadImage               );
    CL_LOAD_FUNCTION(clEnqueueWriteImage              );
    CL_LOAD_FUNCTION(clEnqueueFillImage               );
    CL_LOAD_FUNCTION(clEnqueueCopyImage               );
    CL_LOAD_FUNCTION(clEnqueueCopyImageToBuffer       );
    CL_LOAD_FUNCTION(clEnqueueCopyBufferToImage       );*/
    CL_LOAD_FUNCTION(clEnqueueMapBuffer               );
    /* CL_LOAD_FUNCTION(clEnqueueMapImage             );*/
    CL_LOAD_FUNCTION(clEnqueueUnmapMemObject          );
    /* CL_LOAD_FUNCTION(clEnqueueMigrateMemObjects    );*/
    CL_LOAD_FUNCTION(clEnqueueNDRangeKernel           );
    /* CL_LOAD_FUNCTION(clEnqueueTask                 );
    CL_LOAD_FUNCTION(clEnqueueNativeKernel            );
    CL_LOAD_FUNCTION(clEnqueueMarkerWithWaitList      );
    CL_LOAD_FUNCTION(clEnqueueBarrierWithWaitList     );
    CL_LOAD_FUNCTION(clSetPrintfCallback              );*/

    /* ********* Get and select a platform. ********* */
    CL_SAFE_CALL0(err_code = jocl_clGetPlatformIDs(0, NULL, &num_platform)
      , return CL_FALSE);
    if ( num_platform < 1 ) {
      CL_DEBUG_NOTE("NO CL PLATFORM!\n");
      return CL_FALSE;
    }
    // Check for Environmental variable for opencl platform.
   // if not present select the AMD platform if present
   platformVendor=getenv("LJT_OPENCL_PLATFORMVENDOR");
   if(NULL == platformVendor)
       platformVendor = "Advanced Micro Devices, Inc.";

    pids = (cl_platform_id*)malloc(num_platform * sizeof(cl_platform_id));

    CL_SAFE_CALL1(err_code = jocl_clGetPlatformIDs(num_platform, pids, NULL),
      return CL_FALSE, pids);

    //Select the platform
    // if amd platform is not present select the last platform
    for (index = 0 ; index < num_platform ; ++index ) {
      ocl_status->platform_id = pids[index];

      CL_SAFE_CALL1(err_code = jocl_clGetPlatformInfo(ocl_status->platform_id,
        CL_PLATFORM_VENDOR, sizeof(ocl_status->platform_vendor),
        ocl_status->platform_vendor, NULL), return CL_FALSE, pids);
      if(NULL != platformVendor)
      {
        if (0 == strcmp(ocl_status->platform_vendor,
            platformVendor))
            break;
      }
    }

    free(pids);

    CL_SAFE_CALL0(err_code = jocl_clGetPlatformInfo(ocl_status->platform_id,
      CL_PLATFORM_PROFILE, sizeof(ocl_status->platform_profile),
      ocl_status->platform_profile, NULL), return CL_FALSE);

    CL_SAFE_CALL0(err_code = jocl_clGetPlatformInfo(ocl_status->platform_id,
      CL_PLATFORM_VERSION, sizeof(ocl_status->platform_version),
      ocl_status->platform_version, NULL), return CL_FALSE);

    version_ocl_inter = ocl_status->platform_version + 7;
    for(; *version_ocl_inter != ' '; version_ocl_inter++) {
      version_length++;
    }
    version_ocl = (char *) malloc((version_length + 1)* sizeof(char));
    strncpy(version_ocl, ocl_status->platform_version + 7, version_length);
    version_ocl[version_length] = '\0';

    if(strcmp(version_ocl, "1.2") >= 0) {
      ocl_status->is_version1_2_ocl = CL_TRUE;
    }
    free(version_ocl);

    CL_SAFE_CALL0(err_code = jocl_clGetPlatformInfo(ocl_status->platform_id,
      CL_PLATFORM_NAME, sizeof(ocl_status->platform_name),
      ocl_status->platform_name, NULL), return CL_FALSE);

    CL_SAFE_CALL0(err_code = jocl_clGetPlatformInfo(ocl_status->platform_id,
      CL_PLATFORM_EXTENSIONS, sizeof(ocl_status->platform_extensions),
      ocl_status->platform_extensions, NULL), return CL_FALSE);

    CL_DEBUG_NOTE("Platform:\nprofile: %s\nversion: %s\nname: %s\n"
      "vendor: %s\nextensions: %s\n"
      , ocl_status->platform_profile, ocl_status->platform_version
      , ocl_status->platform_name, ocl_status->platform_vendor
      , ocl_status->platform_extensions);

    /* ********* Get and select a device. ********* */
    CL_SAFE_CALL0(err_code = jocl_clGetDeviceIDs(ocl_status->platform_id,
      CL_DEVICE_TYPE_ALL, 0, NULL, &num_device), return CL_FALSE);
    if ( num_device < 1 ) {
      CL_DEBUG_NOTE("NO DEVICE FOUND!\n");
      return CL_FALSE;
    }

    dids = (cl_device_id*)malloc(num_device * sizeof(cl_device_id));

    CL_SAFE_CALL1(err_code = jocl_clGetDeviceIDs(ocl_status->platform_id,
    CL_DEVICE_TYPE_ALL, num_device, dids, NULL), return CL_FALSE, dids);
    deviceName = platformVendor=getenv("LJT_OPENCLDEVICE");
    if(NULL != deviceName)
    {
        // Select the device specified by user through environmental variable
        // if not select the 0th device
        for (index = (num_device-1); index >= 0; --index)
        {
            ocl_status->device_id = dids[index];
            CL_SAFE_CALL0(err_code = jocl_clGetDeviceInfo(ocl_status->device_id,
            CL_DEVICE_NAME, sizeof(ocl_status->device_name), ocl_status->device_name, NULL),
            return CL_FALSE);
            if (0 == strcmp(ocl_status->device_name,deviceName))
            break;
        }
    }
    else
    {
	    for ( index = 0 ; index < num_device ; ++index ) {
	      cl_device_type d_type;
	      ocl_status->device_id = dids[index];
	      CL_SAFE_CALL1(err_code = jocl_clGetDeviceInfo(ocl_status->device_id,
	  	    CL_DEVICE_TYPE, sizeof(cl_device_type), &d_type, NULL),
	  	    return CL_FALSE, dids);
	      if(CL_DEVICE_TYPE_GPU == d_type)
	  	    num_gpu_device++;
	    }
	    for (index = 0; index < num_device; ++index) {
	      cl_device_type d_type;
	      ocl_status->device_id = dids[index];
	      CL_SAFE_CALL1(err_code = jocl_clGetDeviceInfo(ocl_status->device_id,
	        CL_DEVICE_TYPE, sizeof(cl_device_type), &d_type, NULL),
	        return CL_FALSE, dids);
	      CL_SAFE_CALL0(err_code = jocl_clGetDeviceInfo(ocl_status->device_id,
	        CL_DEVICE_NAME, sizeof(ocl_status->device_name), ocl_status->device_name, NULL),
	        return CL_FALSE);
	      if(CL_DEVICE_TYPE_GPU == d_type) {
	        num_gpu_device--;
	        if (0 == strcmp(ocl_status->device_name,
	          "Devastator"))
	          break;
	      }
	      if(num_gpu_device == 0)
	        break;
	    }
    }
	CL_DEBUG_NOTE("Device:\nName: %s\n",ocl_status->device_name);
	/* AMD A10-4600M APU with Radeon(tm) HD Graphics */

    free(dids);
	CL_SAFE_CALL0(err_code = jocl_clGetDeviceInfo(ocl_status->device_id,
	  CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_device_type),
	  &ocl_status->mem_size, NULL), return CL_FALSE);

    /* ********* Create a context. ********* */
    ocl_status->context = CL_SAFE_CALL0(jocl_clCreateContext(NULL,
      1, &ocl_status->device_id, NULL, NULL, &err_code), return CL_FALSE);

    /* ********* Create a command queue. ********* */
    ocl_status->command_queue = CL_SAFE_CALL0(jocl_clCreateCommandQueue(
      ocl_status->context, ocl_status->device_id, 0, &err_code),
      return CL_FALSE);
#ifdef JOCL_CL_OS_WIN32
    if( jocl_clGetCommandQueueInfo( ocl_status->command_queue,
      CL_QUEUE_THREAD_HANDLE_AMD, sizeof(handle),
      &handle, NULL ) == CL_SUCCESS && handle != INVALID_HANDLE_VALUE ) {
      if(SetThreadPriority( handle, THREAD_PRIORITY_TIME_CRITICAL )) {
        CL_DEBUG_NOTE ("SetThreadPriority success\n");
        ret = GetThreadPriority(handle);
        CL_DEBUG_NOTE ("Priority level = %d\n",ret);
      }
      else {
        int ret = GetLastError();
        CL_DEBUG_NOTE ("SetThreadPriority error,code  = %d\n",ret);
      }
    }  
#endif
    /* *** OpenCL initialized successfully, modify the mark. *** */
    ocl_status->is_opencl_available = CL_TRUE;
    ocl_status->is_opencl_support = CL_TRUE;
    ocl_status->fancy_index = CL_TRUE;
    CL_DEBUG_NOTE("OpenCL is enabled.\n");
  }
  return CL_TRUE;
}

#undef CL_LOAD_FUNCTION
/*
 * jocl_cl_compile_and_build
 *
 * If it's the first time to use libjpeg-turbo on this system,
 * create and build program with source that specified in program_source,
 * or create program with binary.
 *   then create kernels that specified in kernel_name[].
 */

JOCL_CL_RUNDATA* jocl_cl_compile_and_build(void * jocl_openClinfo,
                                           const char**  program_source,
                                           const char*  kernel_name[])
{
  int i;
  size_t binarySizes;
  int num_device;
  char *binaries, *str = NULL;
  char deviceName[1024];
  char fileName[256] = { 0 },cl_name[] = "kernel";
  FILE * fp = NULL;
  int b_error, binary_status;
  char *binary;
  size_t length_binary;
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  num_device = 1;
  /* Perform this operation only when OpenCL is available. */
  if (ocl_status->is_opencl_available) {
    JOCL_CL_RUNDATA* cl_data = NULL;
    cl_int  err_code;
    cl_uint index;
    cl_uint num_kernel = 0;
    size_t  length[7];
    for(i = 0; i < 7; i++)
      length[i]= strlen(program_source[i]);

    /* Get the number of kernel functions that need to be created. */
    while (kernel_name[++num_kernel] != NULL);

    /* Allocate memory for cl_data, and create cl_program.
     * If there is any error in the following steps,
     *   it needs to release the memory step by step.
     */
    cl_data = (JOCL_CL_RUNDATA*)
      malloc(sizeof(JOCL_CL_RUNDATA));

    CL_SAFE_CALL1(err_code = jocl_clGetDeviceInfo(ocl_status->device_id,
      CL_DEVICE_NAME, sizeof(deviceName), deviceName, NULL),
      return NULL, cl_data);
    sprintf(fileName, "%s_%s.bin", cl_name, deviceName);

    if(!(fp = fopen(fileName,"rb"))){
      CL_SAFE_CALL1(cl_data->program = jocl_clCreateProgramWithSource(
        ocl_status->context, 7, program_source, length, &err_code),
        return NULL, cl_data);

      /* Compile OpenCL code. If error, output the error informations. */
      CL_SAFE_CALL1(err_code = jocl_clBuildProgram(cl_data->program,
        0, NULL, NULL, NULL, NULL), return NULL, cl_data);

      if (CL_SUCCESS != err_code) {
        char *err_msg;
        size_t sz_msg;
        cl_uint build_error = err_code;
      
        CL_SAFE_CALL1(err_code = jocl_clGetProgramBuildInfo(
          cl_data->program, ocl_status->device_id, CL_PROGRAM_BUILD_LOG,
          0, NULL, &sz_msg), return NULL, cl_data);
      
        /* Output the error informations. */
        err_msg = (char*)malloc(sz_msg);
        CL_SAFE_CALL2(err_code = jocl_clGetProgramBuildInfo(
          cl_data->program, ocl_status->device_id, CL_PROGRAM_BUILD_LOG,
          sz_msg, err_msg, NULL), return NULL, err_msg, cl_data);
      
        CL_DEBUG_NOTE("OpenCL Build Error:%s\n%s\n",
          jocl_cl_errstring(build_error),err_msg);
      
        free(err_msg);   
        free(cl_data);
        return NULL;
      }
      CL_SAFE_CALL1(jocl_clGetProgramInfo(cl_data->program, 
        CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binarySizes, NULL),
        return NULL, cl_data);

      if (binarySizes != 0) {
        binaries = (char*) malloc(sizeof(char) * binarySizes);
        if (binaries == NULL) {
            return 0;
        }
      }
      else {
         binaries = NULL;
      }
      CL_SAFE_CALL1(jocl_clGetProgramInfo(cl_data->program,
        CL_PROGRAM_BINARIES, sizeof(char *) * num_device, &binaries,
        NULL), return NULL, cl_data);
       if (binarySizes != 0) {
         fp = fopen(fileName, "wb");
         if (fp == NULL) {
           return 0;
         }          
         fwrite(binaries, sizeof(char), binarySizes, fp);
         fclose(fp);
       }
    }
    else {
      b_error = 0;
      length_binary = 0;
      b_error |= fseek(fp, 0, SEEK_END) < 0;
      b_error |= (length_binary = ftell(fp)) <= 0;
      b_error |= fseek(fp, 0, SEEK_SET) < 0;
      if (b_error) {
          return 0;
      }
      binary = (char*) malloc(length_binary + 2);
      if (!binary) {
          return 0;
      }
      memset(binary, 0, length_binary + 2);
      b_error |= fread(binary, 1, length_binary, fp) != length_binary;
      CL_SAFE_CALL1(cl_data->program = jocl_clCreateProgramWithBinary(
        ocl_status->context, 1, &ocl_status->device_id, &length_binary,
        (const unsigned char**) &binary, &binary_status,&err_code),
        NULL, cl_data);
       
      /* Compile OpenCL code. If error, output the error informations. */
      CL_SAFE_CALL1(err_code = jocl_clBuildProgram(cl_data->program,
        0, NULL, NULL, NULL, NULL), return NULL, cl_data);

      if (CL_SUCCESS != err_code) {
        char *err_msg;
        size_t sz_msg;
        cl_uint build_error = err_code;
      
        CL_SAFE_CALL1(err_code = jocl_clGetProgramBuildInfo(
          cl_data->program, ocl_status->device_id, CL_PROGRAM_BUILD_LOG,
          0, NULL, &sz_msg), return NULL, cl_data);
      
        /* Output the error informations. */
        err_msg = (char*)malloc(sz_msg);
        CL_SAFE_CALL2(err_code = jocl_clGetProgramBuildInfo(
          cl_data->program, ocl_status->device_id, CL_PROGRAM_BUILD_LOG,
          sz_msg, err_msg, NULL), return NULL, err_msg, cl_data);

        CL_DEBUG_NOTE("OpenCL Build Error:%s\n%s\n",
          jocl_cl_errstring(build_error),err_msg);

        free(err_msg);
        free(cl_data);
        return NULL;
      }
      free(binary);
      /* Build Successfully. */
      CL_DEBUG_NOTE("Compiling OpenCL code successfully.\n");
    }
    /* Allocate memory for cl_kernel and other auxiliary variables. */
    cl_data->kernel = (cl_kernel*)malloc(num_kernel * sizeof(cl_kernel));
    cl_data->work_group_size = (size_t*)malloc(num_kernel * sizeof(size_t));

    /* Create cl_kernels. */
    for ( index = 0 ; index < num_kernel ; ++index ) {
      CL_SAFE_CALL3(cl_data->kernel[index] = jocl_clCreateKernel(
        cl_data->program, kernel_name[index], &err_code)
        , return NULL, cl_data->kernel, cl_data->work_group_size, cl_data);

      CL_SAFE_CALL3(err_code = jocl_clGetKernelWorkGroupInfo(
        cl_data->kernel[index], ocl_status->device_id,
        CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t),
        &cl_data->work_group_size[index], NULL),
        return NULL, cl_data->kernel, cl_data->work_group_size, cl_data);
    }

    /* Build cl_program and create cl_kernels successfully.*/
    CL_DEBUG_NOTE("Create kernel OK!\n");
    return cl_data;
  }
  return NULL;
}


/*
 * jocl_cl_is_available
 * jocl_cl_is_support_opencl
 * jocl_cl_get_platform
 * jocl_cl_get_device
 * jocl_cl_get_context
 * jocl_cl_get_command_queue
 *
 * These functions are used to return the platform informations,
 *   so I'm not going to comment them one by one.
 */

cl_bool          jocl_cl_is_available(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  return ocl_status->is_opencl_available;
}

cl_bool          jocl_cl_is_support_opencl(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  return ocl_status->is_opencl_support;
}

cl_platform_id   jocl_cl_get_platform(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  return ocl_status->platform_id;
}

cl_device_id     jocl_cl_get_device(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  return ocl_status->device_id;
}

cl_context       jocl_cl_get_context(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  return ocl_status->context;
}

cl_command_queue jocl_cl_get_command_queue(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  return ocl_status->command_queue;
}

/*
 * jocl_cl_set_opencl_failure
 *
 * If OpenCL is unavailable (The reason may be varied),
 *   Set the flag to disable OpenCL.
 */

void jocl_cl_set_opencl_failure(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  ocl_status->is_opencl_available = CL_FALSE;
}

/*
 * jocl_cl_set_opencl_success
 *
 * If OpenCL is available, set the flag to enable OpenCL.
 */

void jocl_cl_set_opencl_success(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  ocl_status->is_opencl_available = CL_TRUE;
}

/*
 * jocl_cl_set_opencl_support_failure
 *
 * If OpenCL is not supported (The reason may be varied),
 *   Set the flag to disable OpenCL.
 */

void jocl_cl_set_opencl_support_failure(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  ocl_status->is_opencl_support = CL_FALSE;
}

/*
 * jocl_cl_is_nvidia_opencl
 *
 * If OpenCL is on NV
 * 
 */
 
cl_bool jocl_cl_is_nvidia_opencl(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  if (0 == strcmp(ocl_status->platform_vendor,
        "NVIDIA Corporation"))
    return CL_TRUE;
  else {
#ifdef __APPLE__
    return CL_TRUE;
#else
    return CL_FALSE;
#endif
  }
}
      
/*
 * jocl_cl_is_opencl_decompress
 *
 * Determine whether the OpenCL decoding will be used.
 */

j_decompress_ptr cinfo_save = NULL;

cl_bool jocl_cl_is_opencl_decompress(j_decompress_ptr cinfo)
{
  unsigned int output_buffer;
  unsigned long buffer_output_size = jocl_cl_get_buffer_unit_size((OCL_STATUS* )cinfo->jocl_openClinfo,cinfo->image_width,cinfo->image_height);

  /* output_buffer: the size of actual output */
  output_buffer = cinfo->MCUs_per_row * cinfo->total_iMCU_rows * cinfo->max_h_samp_factor * 
    cinfo->max_v_samp_factor * NUM_COMPONENT * DCTSIZE2;
  /* Determine if the opencl version will be used */
  if(cinfo->num_components==1|| cinfo->progressive_mode == 1 ||
    (cinfo->blocks_in_MCU!=6 && cinfo->blocks_in_MCU!=3 && cinfo->blocks_in_MCU!=4))
    goto bailout;
  if (rgb_red[cinfo->out_color_space] != 0 ||
      rgb_green[cinfo->out_color_space] != 1 ||
      rgb_blue[cinfo->out_color_space] != 2 ||
      rgb_pixelsize[cinfo->out_color_space] != 3)
    goto bailout;
  if(cinfo->scale_denom != 1 || cinfo->scale_num != 1)
    goto bailout;
  if(cinfo->desired_number_of_colors != 256 || cinfo->quantize_colors == TRUE)
    goto bailout;
  if(cinfo->two_pass_quantize == FALSE || cinfo->dither_mode != JDITHER_FS ||
    (cinfo->dct_method != JDCT_ISLOW && cinfo->dct_method != JDCT_IFAST))
    goto bailout;
  if(CL_TRUE == jocl_cl_get_fancy_status((OCL_STATUS* )cinfo->jocl_openClinfo) && cinfo->blocks_in_MCU == 6 &&
    (cinfo->MCUs_per_row > MCUNUMS))
    goto bailout;
  if((output_buffer > buffer_output_size))
    goto bailout;
  
  return CL_TRUE;

  bailout:
  if (cinfo != cinfo_save)
    printf("ERROR: Could not enable OpenCL due to incompatible decompression parameters.\n");
  cinfo_save = cinfo;
  return CL_FALSE;
}

/*
 * jocl_cl_get_fancy_status
 * jocl_cl_set_fancy_status
 *
 * If the -nosmooth is used.
 */

cl_bool jocl_cl_get_fancy_status(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  return ocl_status->fancy_index;
}

void jocl_cl_set_fancy_status(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  ocl_status->fancy_index = CL_FALSE;
}

/*
 * jocl_cl_get_buffer_unit_size
 *
 * Determine the buffer size.
 */

unsigned long jocl_cl_get_buffer_unit_size(void * jocl_openClinfo,unsigned int image_width,unsigned int image_height)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  unsigned long buffer_output_size = 0;
  unsigned long mem_size = ocl_status->mem_size;
  
  buffer_output_size = mem_size - (MCUNUMS * DCTSIZE2 * 18 * BUFFERNUMS);
  if (mem_size > (MAX_BUFFER_SIZE + (MCUNUMS * DCTSIZE2 * 18 * BUFFERNUMS)))
  {
  	buffer_output_size = MAX_BUFFER_SIZE;
  }
  return buffer_output_size ;
}

/*
 * jocl_cl_get_decode_support
 * jocl_cl_set_decode_support
 *
 * Determine support opencl decoding.
 */

cl_bool jocl_cl_get_decode_support(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  return ocl_status->decode_support;
}

void jocl_cl_set_decode_support(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  ocl_status->decode_support = CL_TRUE;
}

/*
 * jocl_cl_get_ocl_version
 *
 * Get OpenCL version.
 */

cl_bool jocl_cl_get_ocl_version(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  return ocl_status->is_version1_2_ocl;
}

/*
 * jocl_cl_get_output_format
 * jocl_cl_set_output_format
 *
 * Whether the output format is bmp.
 */

cl_bool jocl_cl_get_output_format(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  return ocl_status->output_format_bmp;
}

void jocl_cl_set_output_format(void * jocl_openClinfo)
{
  OCL_STATUS * ocl_status = ((OCL_STATUS *)jocl_openClinfo);
  ocl_status->output_format_bmp = CL_TRUE;
}
#endif
