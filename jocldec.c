/*
 * jocldec.c
 *
 * Copyright (C) 2012-2013, MulticoreWare Inc.
 * In July 2013, Written by Peixuan Zhang <zhangpeixuan.cn@gmail.com>
 * The OpenCL kernel code is written by
 *   Chunli  Zhang <chunli@multicorewareinc.com> and
 *   Peixuan Zhang <peixuan@multicorewareinc.com>
 * Based on the OpenCL extension for IJG JPEG library,
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the decoding JPEG code with OpenCL.
 */

#include "jinclude.h"
#include "jpeglib.h"
#ifdef WITH_OPENCL_DECODING_SUPPORTED
#include "CL/opencl.h"
#include "joclinit.h"
#include "jocldec.h"
#include "joclidct.h"

#define  OPENCL_DEC_UPSAMPLE_RGB
#include "jocldsample.h"
#undef   OPENCL_DEC_UPSAMPLE_RGB

#define  OPENCL_DEC_UPSAMPLE_BGR
#include "jocldsample.h"
#undef   OPENCL_DEC_UPSAMPLE_BGR

#define  OPENCL_DEC_UPSAMPLE_RGBA
#include "jocldsample.h"
#undef   OPENCL_DEC_UPSAMPLE_RGBA


cl_bool jocldec_build_kernels(j_decompress_ptr cinfo)
{
  cl_int  err_code;
  cl_uint data_m = 6;
  cl_uint blocksWidth  = 64;
  size_t  global_ws[1], local_ws[1];

  static  const char **jocldec_cl_source;
  char    **jocldec_cl_source_inter;
  int     i, j;
  OCL_STATUS *ocl_status = (OCL_STATUS *)cinfo->jocl_openClinfo;
  jocldec_cl_source_inter = (char**) malloc(7 * sizeof(char*));
  for(i=0; i<7; ++i) {
    jocldec_cl_source_inter[i] = (char*) malloc(60000 * sizeof(char));
  }
  strcpy(jocldec_cl_source_inter[0], jocldec_cl_source1);
  strcpy(jocldec_cl_source_inter[1], jocldec_cl_source2);
  strcpy(jocldec_cl_source_inter[2], jocldec_cl_source3);
  strcpy(jocldec_cl_source_inter[3], jocldec_cl_source4);
  strcpy(jocldec_cl_source_inter[4], jocldec_cl_source5);
  strcpy(jocldec_cl_source_inter[5], jocldec_cl_source6);
  strcpy(jocldec_cl_source_inter[6], jocldec_cl_source7);
  jocldec_cl_source = jocldec_cl_source_inter;

  global_ws[0] = 256;
  local_ws[0] = 128;

  if (!ocl_status->jocldec_cl_rundata)  {
    const char* kernel_names[] = {"IDCT_FAST_SHORT",
                                  "IDCT_SLOW_INT",
                                  "IDCT_FAST_FLOAT",

                                  "UPSAMPLE_H1V1_RGB",
                                  "UPSAMPLE_H1V2_RGB",
                                  "UPSAMPLE_H2V1_RGB",
                                  "UPSAMPLE_H2V2_RGB",

                                  "UPSAMPLE_H2V1_FANCY_RGB",
                                  "UPSAMPLE_H2V2_FANCY_RGB",

                                  "UPSAMPLE_H1V1_RGBA",
                                  "UPSAMPLE_H1V2_RGBA",
                                  "UPSAMPLE_H2V1_RGBA",
                                  "UPSAMPLE_H2V2_RGBA",

                                  "UPSAMPLE_H2V1_FANCY_RGBA",
                                  "UPSAMPLE_H2V2_FANCY_RGBA",

                                  "UPSAMPLE_H1V1_BGR",
                                  "UPSAMPLE_H1V2_BGR",
                                  "UPSAMPLE_H2V1_BGR",
                                  "UPSAMPLE_H2V2_BGR",

                                  "UPSAMPLE_H2V1_FANCY_BGR",
                                  "UPSAMPLE_H2V2_FANCY_BGR",
                                  "RESET_ZERO",
                                   NULL};
    if (ocl_status->jocldec_cl_rundata = jocl_cl_compile_and_build(ocl_status,jocldec_cl_source, kernel_names)) {
#if 1
      /* IDCT FAST SHORT */
      for(j = 0; j < BUFFERNUMS; ++j) {
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[0],
                                                     0,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_input[j]),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[0],
                                                     1,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_inter[j]),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[0],
                                                     2,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_qutable),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[0],
                                                     3,
                                                     1024*sizeof(int),
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[0],
                                                     4,
                                                     sizeof(int),
                                                     &data_m),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clEnqueueNDRangeKernel(jocl_cl_get_command_queue(ocl_status),
                                                     ocl_status->jocldec_cl_rundata->kernel[0],
                                                     1,
                                                     0,
                                                     global_ws,
                                                     local_ws,
                                                     0,
                                                     NULL,
                                                     NULL),
                                                     return CL_FALSE);
        /* IDCT SLOW INT */
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[1],
                                                     0,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_input[j]),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[1],
                                                     1,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_inter[j]),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[1],
                                                     2,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_qutable),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[1],
                                                     3,
                                                     1024*sizeof(int),
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[1],
                                                     4,
                                                     sizeof(int),
                                                     &data_m),
                                                     return CL_FALSE);
        
        CL_SAFE_CALL0(err_code = jocl_clEnqueueNDRangeKernel(jocl_cl_get_command_queue(ocl_status),
                                                     ocl_status->jocldec_cl_rundata->kernel[1],
                                                     1,
                                                     0,
                                                     global_ws,
                                                     local_ws,
                                                     0,
                                                     NULL,
                                                     NULL),
                                                     return CL_FALSE);
        /* IDCT FAST FLOAT */
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[2],
                                                     0,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_input[j]),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[2],
                                                     1,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_inter[j]),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[2],
                                                     2,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_qutable),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[2],
                                                     3,
                                                     1024*sizeof(int),
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[2],
                                                     4,
                                                     sizeof(float),
                                                     &data_m),
                                                     return CL_FALSE);
        
        CL_SAFE_CALL0(err_code = jocl_clEnqueueNDRangeKernel(jocl_cl_get_command_queue(ocl_status),
                                                     ocl_status->jocldec_cl_rundata->kernel[2],
                                                     1,
                                                     0,
                                                     global_ws,
                                                     local_ws,
                                                     0,
                                                     NULL,
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[21],
                                                    0,
                                                    sizeof(cl_mem),
                                                    &ocl_status->jocl_global_data_mem_input[j]),
                                                    return CL_FALSE);
        
        CL_SAFE_CALL0(err_code = jocl_clEnqueueNDRangeKernel(jocl_cl_get_command_queue(ocl_status),
                                                    ocl_status->jocldec_cl_rundata->kernel[21],
                                                    1,
                                                    0,
                                                    global_ws,
                                                    local_ws,
                                                    0,
                                                    NULL,
                                                    NULL),
                                                    return CL_FALSE);
      }
      CL_SAFE_CALL0(err_code = jocl_clFinish(jocl_cl_get_command_queue(ocl_status)), return CL_FALSE);
#endif
    }
  }
  for(i=0; i<7; ++i) {
    free(jocldec_cl_source_inter[i]);
  }
  free(jocldec_cl_source_inter);

  return CL_TRUE;
}

cl_bool jocldec_run_kernels_full_image(
                            j_decompress_ptr cinfo,
                            int data_m,
                            int mcu_num,
                            unsigned int blocksWidth,
                            unsigned int offset_mcu,
                            int total_mcu_num,
                            int decodeMCU,
                            int buffer_index,
                            cl_event* buffer_event)
{
  cl_int err_code;
  size_t global_ws,local_ws;
  int mcu_out,mcu_in;
  int data_m_inter = data_m;
  int size_map;
  unsigned int offset_output;
  cl_kernel jocldec_cl_kernel_use;
  OCL_STATUS * ocl_status =  (OCL_STATUS* )cinfo->jocl_openClinfo;
  int edge_mcu_h = 0;
  int KernelArg2 = 0;
  int KernelArg5 = 0;
  int KernelArg6 = 0;

  if (cinfo->image_width % 16 == 0)
    edge_mcu_h = 7;
  else
    edge_mcu_h = (cinfo->image_width % 16 + 1)/ 2 - 1;

  if (data_m==5) data_m_inter = 4;
  switch(data_m) {
    case 3: mcu_out   = 192;
            mcu_in    = 192;
            global_ws = (mcu_num * mcu_in /8+255)/256*256;
            break;
    case 5: mcu_out   = 384;
            mcu_in    = 256;
            global_ws = (mcu_num * mcu_in /8+255)/256*256;
            break;
    case 4: mcu_out   = 384;
            mcu_in    = 256;
            global_ws = (mcu_num * mcu_in /8+255)/256*256;
            break;
    case 6: mcu_out   = 768;
            mcu_in    = 384;
            global_ws = (mcu_num * mcu_in /8+255)/256*256;
            break;
  }
  if (CL_TRUE == jocl_cl_is_nvidia_opencl(ocl_status)) {
    CL_SAFE_CALL0(err_code = jocl_clEnqueueWriteBuffer(jocl_cl_get_command_queue(ocl_status),
      ocl_status->jocl_global_data_mem_input[buffer_index], CL_TRUE, 0,  MCUNUMS * DCTSIZE2 * 6 * sizeof(JCOEF),
      ocl_status->jocl_global_data_ptr_input[buffer_index], 0, NULL, NULL), return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clEnqueueWriteBuffer(jocl_cl_get_command_queue(ocl_status),
      ocl_status->jocl_global_data_mem_qutable, CL_TRUE, 0,  128 * sizeof(float), ocl_status->jocl_global_data_ptr_qutable,
      0, NULL, NULL), return CL_FALSE);
  }
#ifndef JOCL_CL_OS_WIN32
  if (CL_FALSE == jocl_cl_is_nvidia_opencl(ocl_status)) {
    CL_SAFE_CALL0(err_code = jocl_clEnqueueUnmapMemObject(
      jocl_cl_get_command_queue(ocl_status), ocl_status->jocl_global_data_mem_input[buffer_index],
      ocl_status->jocl_global_data_ptr_input[buffer_index], 0, NULL, NULL),return CL_FALSE);
  }
#endif
  switch (cinfo->dct_method){
    case JDCT_IFAST:
      local_ws = 256;
      jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[0];
      break;
    case JDCT_ISLOW:
      local_ws = 128;
      jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[1];
      break;
    case JDCT_FLOAT:
      local_ws = 128;
      jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[2];
      break;
  }
  CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                               0,
                                               sizeof(cl_mem),
                                               &ocl_status->jocl_global_data_mem_input[buffer_index]),
                                               return CL_FALSE);
  if (0 == offset_mcu || CL_TRUE == jocl_cl_is_nvidia_opencl(ocl_status)) {
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 1,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_inter[0]),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 2,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_qutable),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 3,
                                                 4096,
                                                 NULL),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 4,
                                                 sizeof(int),
                                                 &data_m_inter),
                                                 return CL_FALSE);
  }
  CL_SAFE_CALL0(err_code = jocl_clEnqueueNDRangeKernel(jocl_cl_get_command_queue(ocl_status),
                                                 jocldec_cl_kernel_use,
                                                 1,
                                                 0,
                                                 &global_ws,
                                                 &local_ws,
                                                 0,
                                                 NULL,
                                                 NULL),
                                                 return CL_FALSE);
  if (3 == data_m){
	global_ws     = mcu_num * 64;
    offset_output = offset_mcu * 64;
    local_ws      = 64;
    if (TRUE == cinfo->opencl_rgb_flag)
      jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[3];
    else
      jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[15];
    if (0 == offset_mcu || CL_TRUE == jocl_cl_is_nvidia_opencl(ocl_status)) {
      CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                   0,
                                                   sizeof(cl_mem),
                                                   &ocl_status->jocl_global_data_mem_inter[0]),
                                                   return CL_FALSE);
      CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                   1,
                                                   sizeof(cl_mem),
                                                   &ocl_status->jocl_global_data_mem_output),
                                                   return CL_FALSE);
      CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                   2,
                                                   256*sizeof(unsigned char),
                                                   NULL),
                                                   return CL_FALSE);
      CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                   3,
                                                   sizeof(unsigned int),
                                                   &blocksWidth),
                                                   return CL_FALSE);
    }
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 4,
                                                 sizeof(unsigned int),
                                                 &offset_output),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clEnqueueNDRangeKernel(jocl_cl_get_command_queue(ocl_status),
                                                 jocldec_cl_kernel_use,
                                                 1,
                                                 0,
                                                 &global_ws,
                                                 &local_ws,
                                                 0,
                                                 NULL,
                                                 NULL),
                                                 return CL_FALSE);
  }  /*1v1 end*/
  else if (5 == data_m){ 
	offset_output = offset_mcu * 128;
    global_ws     = mcu_num * 128;
    local_ws      = 128;
	KernelArg2 = 384 * sizeof(unsigned char);
    if (TRUE == cinfo->opencl_rgb_flag)
      jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[4];
    else
      jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[16];
    if (0 == offset_mcu || CL_TRUE == jocl_cl_is_nvidia_opencl(ocl_status)) {
      CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                   0,
                                                   sizeof(cl_mem),
                                                   &ocl_status->jocl_global_data_mem_inter[0]),
                                                   return CL_FALSE);
      CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                   1,
                                                   sizeof(cl_mem),
                                                   &ocl_status->jocl_global_data_mem_output),
                                                   return CL_FALSE);
      CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                   2,
                                                   KernelArg2,
                                                   NULL),
                                                   return CL_FALSE);
      CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                   3,
                                                   128*sizeof(unsigned char),
                                                   NULL),
                                                   return CL_FALSE);
      CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                   4,
                                                   128*sizeof(unsigned char),
                                                   NULL),
                                                   return CL_FALSE);
      CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                   5,
                                                   sizeof(unsigned int),
                                                   &blocksWidth),
                                                   return CL_FALSE);
    }
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                   6,
                                                   sizeof(unsigned int),
                                                   &offset_output),
                                                   return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clEnqueueNDRangeKernel(jocl_cl_get_command_queue(ocl_status),
                                                   jocldec_cl_kernel_use,
                                                   1,
                                                   0,
                                                   &global_ws,
                                                   &local_ws,
                                                   0,
                                                   NULL,
                                                   NULL),
                                                   return CL_FALSE);
  }/*1v2 end*/
  else if ((4 == data_m ) || (6 == data_m )){
    if (4 == data_m){
      KernelArg2 = 384 * sizeof(unsigned char);
	  KernelArg5 = 128 * sizeof(unsigned char);
	  KernelArg6 = 128 * sizeof(unsigned char); 	  
      local_ws   = 64;
	  if (CL_TRUE == jocl_cl_get_fancy_status(ocl_status)){
        if (decodeMCU != total_mcu_num)
	      global_ws = (mcu_num - 1)* 64;
        else
	  	  global_ws = mcu_num * 64;
        offset_output = offset_mcu * (MCUNUMS - 1) * 64;
        if (TRUE == cinfo->opencl_rgb_flag)
          jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[7];
        else
          jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[19];
      }
	  else{
        offset_output = offset_mcu * 64;
        global_ws = mcu_num * 64;
        if (TRUE == cinfo->opencl_rgb_flag)
          jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[5];
        else
          jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[17];
      }
    }/*end if(data_m == 4)*/
    else if (6 == data_m){
      KernelArg2 = 768 * sizeof(unsigned char);
      KernelArg5 = 256 * sizeof(unsigned char);
      KernelArg6 = 256 * sizeof(unsigned char);
      offset_output = offset_mcu * 128;
      global_ws     = mcu_num * 128;
      local_ws      = 128;
      if (TRUE == cinfo->opencl_rgb_flag)
        jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[6];
      else
        jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[18];
    }/*end if(data_m == 6)*/
    if (0 == offset_mcu || CL_TRUE == jocl_cl_is_nvidia_opencl(ocl_status)) {
      if (CL_FALSE == jocl_cl_get_fancy_status(ocl_status)){
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     0,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_inter[0]),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     1,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_output),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     2,
                                                     KernelArg2,
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     3,
                                                     64*sizeof(unsigned char),
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     4,
                                                     64*sizeof(unsigned char),
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     5,
                                                     KernelArg5,
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     6,
                                                     KernelArg6,
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     7,
                                                     sizeof(unsigned int),
                                                     &blocksWidth),
                                                     return CL_FALSE);
      }
      else {
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     0,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_inter[0]),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     1,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_output),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     2,
                                                     sizeof(cl_mem),
                                                     &ocl_status->jocl_global_data_mem_prior_inter[0]),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     3,
                                                     KernelArg2,
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     4,
                                                     64*sizeof(unsigned char),
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     5,
                                                     64*sizeof(unsigned char),
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     6,
                                                     KernelArg5,
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     7,
                                                     KernelArg6,
                                                     NULL),
                                                     return CL_FALSE);
        CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                     8,
                                                     sizeof(unsigned int),
                                                     &blocksWidth),
                                                     return CL_FALSE);
      }
    }
    if (CL_FALSE == jocl_cl_get_fancy_status(ocl_status)){
      CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                   8,
                                                   sizeof(unsigned int),
                                                   &offset_output),
                                                   return CL_FALSE);
    }else {
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 9,
                                                 sizeof(unsigned int),
                                                 &offset_output),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 10,
                                                 sizeof(unsigned int),
                                                 &edge_mcu_h),
                                                 return CL_FALSE);
    }
    CL_SAFE_CALL0(err_code = jocl_clEnqueueNDRangeKernel(jocl_cl_get_command_queue(ocl_status),
                                                 jocldec_cl_kernel_use,
                                                 1,
                                                 0,
                                                 &global_ws,
                                                 &local_ws,
                                                 0,
                                                 NULL,
                                                 NULL),
                                                 return CL_FALSE);

  }
  if (CL_FALSE == jocl_cl_is_nvidia_opencl(ocl_status)) {
    global_ws = (mcu_num * mcu_in/8 + 255)/256*256;
    local_ws  = 256;
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[21],
                                                 0,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_input[buffer_index]),
                                                 return CL_FALSE);
    
    CL_SAFE_CALL0(err_code = jocl_clEnqueueNDRangeKernel(jocl_cl_get_command_queue(ocl_status),
                                                 ocl_status->jocldec_cl_rundata->kernel[21],
                                                 1,
                                                 0,
                                                 &global_ws,
                                                 &local_ws,
                                                 0,
                                                 NULL,
                                                 &buffer_event[buffer_index]),
                                                 return CL_FALSE);
  }    
  size_map = cinfo->max_h_samp_factor * cinfo->MCUs_per_row * DCTSIZE *
    cinfo->image_height * NUM_COMPONENT;

  if (decodeMCU != total_mcu_num) {
    CL_SAFE_CALL0(err_code = jocl_clFlush(jocl_cl_get_command_queue(ocl_status)),return CL_FALSE);
  }
  else {
    if (CL_FALSE == jocl_cl_is_nvidia_opencl(ocl_status)) {
      int index_event;
      for(index_event = 0; index_event < buffer_index; index_event++)
      {
        jocl_clReleaseEvent(buffer_event[index_event]);
      }
    }
    CL_SAFE_CALL0(err_code = jocl_clFinish(jocl_cl_get_command_queue(ocl_status)),return CL_FALSE);
    if (CL_TRUE == jocl_cl_is_nvidia_opencl(ocl_status)) {
      CL_SAFE_CALL0(err_code = jocl_clEnqueueReadBuffer(jocl_cl_get_command_queue(ocl_status),
        ocl_status->jocl_global_data_mem_output, CL_TRUE, 0, size_map, ocl_status->jocl_global_data_ptr_output,
        0, NULL, NULL), return CL_FALSE);  	
	}
	else {
      CL_SAFE_CALL0(ocl_status->jocl_global_data_ptr_output = (JSAMPROW)jocl_clEnqueueMapBuffer(
        jocl_cl_get_command_queue(ocl_status), ocl_status->jocl_global_data_mem_output, CL_TRUE,
        CL_MAP_READ, 0, size_map, 0, NULL, NULL, &err_code),return CL_FALSE);
      CL_SAFE_CALL0(err_code = jocl_clEnqueueUnmapMemObject(
        jocl_cl_get_command_queue(ocl_status), ocl_status->jocl_global_data_mem_output,
        ocl_status->jocl_global_data_ptr_output, 0, NULL, NULL), return CL_FALSE);
    }
    CL_SAFE_CALL0(err_code = jocl_clFinish(jocl_cl_get_command_queue(ocl_status)),return CL_FALSE);
  }
  return CL_TRUE;
}


cl_bool jocldec_run_kernels_h2v2_fancy(
                            j_decompress_ptr cinfo,
                            int data_m,
                            int mcu_num,
                            unsigned int blocksWidth,
                            unsigned int offset_mcu,
                            int total_mcu_num,
                            int decodeMCU,
                            int buffer_index,
                            cl_event* buffer_event,
                            int mcunum_buffer)
{
  cl_int err_code;
  size_t global_ws,local_ws;
  int    mcu_out,mcu_in;
  int    data_m_inter = data_m;
  int    size_map;
  unsigned int offset_output;
  cl_kernel jocldec_cl_kernel_use;
  unsigned int mcu_total = total_mcu_num * 128;
  int KernelArg2 = 0;
  int KernelArg5 = 0;
  int KernelArg6 = 0;
  int buffer_index_prior;
  int edge_mcu_h = 0, edge_mcu_v = 0;
  unsigned int mcu_offset_buffer;
  OCL_STATUS * ocl_status =  (OCL_STATUS* )cinfo->jocl_openClinfo;

  if (buffer_index == 0) buffer_index_prior = 9;
  else buffer_index_prior = buffer_index - 1;
  if (cinfo->image_width % 16 == 0)
    edge_mcu_h = 7;
  else
    edge_mcu_h = (cinfo->image_width % 16 + 1)/ 2 - 1;

  if (cinfo->image_height % 16 == 0)
    edge_mcu_v = 8;
  else
    edge_mcu_v = (cinfo->image_height % 16 + 1)/ 2;

  mcu_out   = 768;
  mcu_in    = 384;
  global_ws = (mcu_num * mcu_in /8+255)/256*256;

  if (CL_TRUE == jocl_cl_is_nvidia_opencl(ocl_status)) {
    CL_SAFE_CALL0(err_code = jocl_clEnqueueWriteBuffer(jocl_cl_get_command_queue(ocl_status),
      ocl_status->jocl_global_data_mem_input[buffer_index], CL_TRUE, 0,  MCUNUMS * DCTSIZE2 * 6 * sizeof(JCOEF),
      ocl_status->jocl_global_data_ptr_input[buffer_index], 0, NULL, NULL), return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clEnqueueWriteBuffer(jocl_cl_get_command_queue(ocl_status),
      ocl_status->jocl_global_data_mem_qutable, CL_TRUE, 0,  128 * sizeof(float), ocl_status->jocl_global_data_ptr_qutable,
      0, NULL, NULL), return CL_FALSE);
  }
#ifndef JOCL_CL_OS_WIN32
  if (CL_FALSE == jocl_cl_is_nvidia_opencl(ocl_status)) {
    CL_SAFE_CALL0(err_code = jocl_clEnqueueUnmapMemObject(
      jocl_cl_get_command_queue(ocl_status), ocl_status->jocl_global_data_mem_input[buffer_index],
      ocl_status->jocl_global_data_ptr_input[buffer_index], 0, NULL, NULL),return CL_FALSE);
  }
#endif
  switch (cinfo->dct_method){
    case JDCT_IFAST:
      local_ws = 256;
      jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[0];
      break;
    case JDCT_ISLOW:
      local_ws = 128;
      jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[1];
      break;
    case JDCT_FLOAT:
      local_ws = 128;
      jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[2];
      break;
  }
  CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                               0,
                                               sizeof(cl_mem),
                                               &ocl_status->jocl_global_data_mem_input[buffer_index]),
                                               return CL_FALSE);
  CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                               1,
                                               sizeof(cl_mem),
                                               &ocl_status->jocl_global_data_mem_inter[buffer_index]),
                                               return CL_FALSE);
  if (0 == offset_mcu || CL_TRUE == jocl_cl_is_nvidia_opencl(ocl_status)) {
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 2,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_qutable),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 3,
                                                 4096,
                                                 NULL),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 4,
                                                 sizeof(int),
                                                 &data_m_inter),
                                                 return CL_FALSE);
  }
  CL_SAFE_CALL0(err_code = jocl_clEnqueueNDRangeKernel(jocl_cl_get_command_queue(ocl_status),
                                                 jocldec_cl_kernel_use,
                                                 1,
                                                 0,
                                                 &global_ws,
                                                 &local_ws,
                                                 0,
                                                 NULL,
                                                 NULL),
                                                 return CL_FALSE);
   if (CL_FALSE == jocl_cl_is_nvidia_opencl(ocl_status)) {
    global_ws = (mcu_num * mcu_in/8 + 255)/256*256;
    local_ws  = 256;
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(ocl_status->jocldec_cl_rundata->kernel[21],
                                                 0,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_input[buffer_index]),
                                                 return CL_FALSE);
    
    CL_SAFE_CALL0(err_code = jocl_clEnqueueNDRangeKernel(jocl_cl_get_command_queue(ocl_status),
                                                 ocl_status->jocldec_cl_rundata->kernel[21],
                                                 1,
                                                 0,
                                                 &global_ws,
                                                 &local_ws,
                                                 0,
                                                 NULL,
                                                 &buffer_event[buffer_index]),
                                                 return CL_FALSE);
  }
  if (TRUE == cinfo->opencl_rgb_flag)
    jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[8];
  else
    jocldec_cl_kernel_use = ocl_status->jocldec_cl_rundata->kernel[20];

  if (offset_mcu > 0) {
    KernelArg2 = 768 * sizeof(unsigned char);
    KernelArg5 = 256 * sizeof(unsigned char);
    KernelArg6 = 256 * sizeof(unsigned char);
    offset_output = (offset_mcu - mcunum_buffer) * 128;
    mcu_offset_buffer = (mcunum_buffer - cinfo->MCUs_per_row) * 128;
    global_ws     = mcunum_buffer * 128;
    local_ws      = 128;

    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 0,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_inter[buffer_index_prior]),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 1,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_inter[buffer_index]),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 2,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_output),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 3,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_prior_inter[buffer_index_prior]),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 4,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_prior_inter[buffer_index]),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 5,
                                                 KernelArg2, 
                                                 NULL),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 6,
                                                 200*sizeof(unsigned char),
                                                 NULL),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 7,
                                                 200*sizeof(unsigned char),
                                                 NULL),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 8,
                                                 KernelArg5,
                                                 NULL),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 9,
                                                 KernelArg6,
                                                 NULL),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 10,
                                                 sizeof(unsigned int),
                                                 &blocksWidth),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 11,
                                                 sizeof(unsigned int),
                                                 &offset_output),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 12,
                                                 sizeof(unsigned int),
                                                 &mcu_total),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 13,
                                                 sizeof(unsigned int),
                                                 &edge_mcu_h),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 14,
                                                 sizeof(unsigned int),
                                                 &edge_mcu_v),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 15,
                                                 sizeof(unsigned int),
                                                 &mcu_offset_buffer),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clEnqueueNDRangeKernel(jocl_cl_get_command_queue(ocl_status),
                                                 jocldec_cl_kernel_use,
                                                 1,
                                                 0,
                                                 &global_ws,
                                                 &local_ws,
                                                 0,
                                                 NULL,
                                                 NULL),
                                                 return CL_FALSE);
  }
  if (decodeMCU == total_mcu_num) {
    KernelArg2 = 768 * sizeof(unsigned char);
    KernelArg5 = 256 * sizeof(unsigned char);
    KernelArg6 = 256 * sizeof(unsigned char);
    offset_output = offset_mcu * 128;
    mcu_offset_buffer = (mcunum_buffer - cinfo->MCUs_per_row) * 128;
    global_ws     = mcu_num * 128;
    local_ws      = 128;
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 0,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_inter[buffer_index]),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 1,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_inter[buffer_index]),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 2,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_output),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 3,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_prior_inter[buffer_index]),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 4,
                                                 sizeof(cl_mem),
                                                 &ocl_status->jocl_global_data_mem_prior_inter[buffer_index_prior]),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 5,
                                                 KernelArg2,
                                                 NULL),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 6,
                                                 200*sizeof(unsigned char),
                                                 NULL),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 7,
                                                 200*sizeof(unsigned char),
                                                 NULL),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 8,
                                                 KernelArg5,
                                                 NULL),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 9,
                                                 KernelArg6,
                                                 NULL),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 10,
                                                 sizeof(unsigned int),
                                                 &blocksWidth),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 11,
                                                 sizeof(unsigned int),
                                                 &offset_output),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 12,
                                                 sizeof(unsigned int),
                                                 &mcu_total),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 13,
                                                 sizeof(unsigned int),
                                                 &edge_mcu_h),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 14,
                                                 sizeof(unsigned int),
                                                 &edge_mcu_v),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clSetKernelArg(jocldec_cl_kernel_use,
                                                 15,
                                                 sizeof(unsigned int),
                                                 &mcu_offset_buffer),
                                                 return CL_FALSE);
    CL_SAFE_CALL0(err_code = jocl_clEnqueueNDRangeKernel(jocl_cl_get_command_queue(ocl_status),
                                                 jocldec_cl_kernel_use,
                                                 1,
                                                 0,
                                                 &global_ws,
                                                 &local_ws,
                                                 0,
                                                 NULL,
                                                 NULL),
                                                 return CL_FALSE);
  }
  if (decodeMCU != total_mcu_num) {
    CL_SAFE_CALL0(err_code = jocl_clFlush(jocl_cl_get_command_queue(ocl_status)),return CL_FALSE);
  }
  else {
    size_map = cinfo->max_h_samp_factor * cinfo->MCUs_per_row * DCTSIZE *
      cinfo->image_height * NUM_COMPONENT;

    CL_SAFE_CALL0(err_code = jocl_clFinish(jocl_cl_get_command_queue(ocl_status)),return CL_FALSE);
    if (CL_TRUE == jocl_cl_is_nvidia_opencl(ocl_status)) {
      CL_SAFE_CALL0(err_code = jocl_clEnqueueReadBuffer(jocl_cl_get_command_queue(ocl_status),
        ocl_status->jocl_global_data_mem_output, CL_TRUE, 0, size_map, ocl_status->jocl_global_data_ptr_output,
        0, NULL, NULL), return CL_FALSE);  	
	}
	else {
      CL_SAFE_CALL0(ocl_status->jocl_global_data_ptr_output = (JSAMPROW)jocl_clEnqueueMapBuffer(
        jocl_cl_get_command_queue(ocl_status), ocl_status->jocl_global_data_mem_output, CL_TRUE,
        CL_MAP_READ, 0, size_map, 0, NULL, NULL, &err_code),return CL_FALSE);
      CL_SAFE_CALL0(err_code = jocl_clEnqueueUnmapMemObject(
        jocl_cl_get_command_queue(ocl_status), ocl_status->jocl_global_data_mem_output,
        ocl_status->jocl_global_data_ptr_output, 0, NULL, NULL), return CL_FALSE);
    }
    CL_SAFE_CALL0(err_code = jocl_clFinish(jocl_cl_get_command_queue(ocl_status)),return CL_FALSE);
  }
  return CL_TRUE;
}

void jocl_release_ocl_resource  (j_decompress_ptr cinfo)
{
  int j;
  int num_buffer;
    OCL_STATUS *ocl_status = (OCL_STATUS *)cinfo->jocl_openClinfo;
  //if(cinfo->max_v_samp_factor == 2 && cinfo->max_h_samp_factor == 2)
  //  num_buffer = 2;
  //else
    num_buffer = BUFFERNUMS;

  if (CL_TRUE == jocl_cl_is_available(ocl_status) && jocl_cl_get_decode_support(ocl_status)) {


    if (CL_TRUE == jocl_cl_is_nvidia_opencl(ocl_status)) {
      free(ocl_status->jocl_global_data_ptr_output);
      free(ocl_status->jocl_global_data_ptr_qutable);
      for (j = 0; j < num_buffer; ++j) {
        free(ocl_status->jocl_global_data_ptr_input[j]);
      }
    }
    else {
      jocl_clReleaseMemObject(ocl_status->jocl_global_data_mem_output);
      jocl_clReleaseMemObject(ocl_status->jocl_global_data_mem_qutable);
      for (j = 0; j < BUFFERNUMS; ++j) {
        jocl_clReleaseMemObject(ocl_status->jocl_global_data_mem_inter[j]);
        jocl_clReleaseMemObject(ocl_status->jocl_global_data_mem_prior_inter[j]);
        jocl_clReleaseMemObject(ocl_status->jocl_global_data_mem_input[j]);
      }
    }
    for (j = 0; j < 22; ++j) {
      jocl_clReleaseKernel(ocl_status->jocldec_cl_rundata->kernel[j]);
    }
    jocl_clReleaseProgram(ocl_status->jocldec_cl_rundata->program);
    jocl_clReleaseCommandQueue(jocl_cl_get_command_queue(ocl_status));
    jocl_clReleaseContext(jocl_cl_get_context(ocl_status));
    free(ocl_status->jocldec_cl_rundata->kernel);
    free(ocl_status->jocldec_cl_rundata->work_group_size);
    free(ocl_status->jocldec_cl_rundata);
    ocl_status->jocldec_cl_rundata = NULL;
  }
}
#endif
