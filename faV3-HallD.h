#pragma once
/**
 * @copyright Copyright 2024, Jefferson Science Associates, LLC.
 *            Subject to the terms in the LICENSE file found in the
 *            top-level directory.
 *
 * @author    Bryan Moffit
 *            moffit@jlab.org                   Jefferson Lab, MS-12B3
 *            Phone: (757) 269-5660             12000 Jefferson Ave.
 *                                              Newport News, VA 23606
 *
 * @author    David Abbott
 *            abbottd@jlab.org                  Jefferson Lab, MS-12B3
 *            Phone: (757) 269-7190             12000 Jefferson Ave.
 *                                              Newport News, VA 23606
 * @file      faV3-HallD.h
 *
 * @brief     Header for Library to support the HallD-Production Firmware
 *
 */

#include <stdint.h>

typedef struct
{
  /* 0x0100 */ volatile uint32_t status0;
  /* 0x0104 */ volatile uint32_t status1;
  /* 0x0108 */ volatile uint32_t status2;
  /* 0x010C */ volatile uint32_t config1;
  /* 0x0110 */ volatile uint32_t config2;
  /* 0x0114 */ volatile uint32_t config4;
  /* 0x0118 */ volatile uint32_t config5;
  /* 0x011C */ volatile uint32_t ptw;
  /* 0x0120 */ volatile uint32_t pl;
  /* 0x0124 */ volatile uint32_t nsb;
  /* 0x0128 */ volatile uint32_t nsa;
  /* 0x012C */ volatile uint32_t thres[8];
  /* 0x0118 */ volatile uint32_t config6;
  /* 0x0118 */ volatile uint32_t config7;
  /* 0x0154 */ volatile uint32_t test_wave;
  /* 0x0158 */ volatile uint32_t pedestal[16];
  /* 0x0198 */ volatile uint32_t config3;
  /* 0x019C */ volatile uint32_t status3;
  /* 0x019C */ volatile uint32_t status4;
  /* 0x019C */ volatile uint32_t roque_ptw_fall_back;
} faV3_halld_adc_t;

#define FAV3_HALLD_PROC_MODE_PULSE_PARAM  9
#define FAV3_HALLD_PROC_MODE_DEBUG       10

#define FAV3_HALLD_SUPPORTED_MODES                   9,10
#define FAV3_HALLD_SUPPORTED_NMODES                     2

extern const char *faV3_halld_mode_names[FAV3_MAX_PROC_MODE + 1];


#define FAV3_ADC_CONFIG1_CHAN_MASK         0x0F00
#define FAV3_ADC_CONFIG1_CHAN_READ_ENABLE (1<<15)

#define FAV3_ADC_CONFIG6_MNPED_MASK   0x00003C00
#define FAV3_ADC_CONFIG6_PMAXPED_MASK 0x000003FF

#define FAV3_ADC_CONFIG7_NPED_MASK    0x00003C00
#define FAV3_ADC_CONFIG7_MAXPED_MASK  0x000003FF