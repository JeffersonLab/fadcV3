/*
 *  delay_scan_code.c  - Program to measure and find the optimal delay for each channel
 *                       to insure that the signal timing does not shift when reprogammed
 *
 *  Authors: Ben Raydo, Ed Jastrzembski, Bryan Moffit
 *           Jefferson Lab Data Acquisition Group
 *           September 2019
 *
 *----------------------------------------------------------------------------*/

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/signal.h>
#include <time.h>
#include "jvme.h"
#include "faV3Lib.h"
#include "delay_scan_code.h"

extern pthread_mutex_t   faV3Mutex;

extern volatile faV3_t *FAV3p[(FAV3_MAX_BOARDS+1)]; /* pointers to FADC memory map */
extern int nfaV3;                                           /* Number of FADCs in Crate */
extern int faV3ID[FAV3_MAX_BOARDS];                           /* array of slot numbers for FADCs */

FILE *filep;
char *progName;

int debugScan = 1;

void
Usage()
{
  printf("\nJLAB fadc delay scan\n");
  printf("\n");
  printf("%s <output file> <FADC VME ADDRESS>\n",progName);
  printf("    OR\n");
  printf("%s <output file>\n",progName);
  printf("\n");

}

int
main(int argc, char *argv[])
{
    int status;
    char *output_filename;
    unsigned int fadc_address=0;

    printf("\nJLAB fadc delay scan\n");
    printf("----------------------------\n");

    signal(SIGINT, sig_handler);

    progName = argv[0];

    if(argc == 2)
      {
	output_filename = argv[1];
	fadc_address = (3 << 19);
	nfaV3 = 18; /* extern: Will be properly redefined in faInit */
      }
    else if(argc == 3)
      {
	output_filename = argv[1];
	fadc_address = (unsigned int) strtoll(argv[2], NULL, 16) & 0xffffffff;
	nfaV3 = 1;
      }

    /* Check for existence of the output file */
    struct stat buffer;
    if( stat(output_filename, &buffer) == 0)
      {
	printf("WARNING: File (%s) exists!\n Overwrite?\n",
	       output_filename);

	int inputchar=10;

      REPEAT2:
	printf(" (y/n): ");
	inputchar = getchar();

	if((inputchar == 'n') ||
	   (inputchar == 'N'))
	  {
	    printf("--- Exiting without reading ---\n");
	    exit(0);
	  }
	else if((inputchar == 'y') ||
		(inputchar == 'Y'))
	  {
	  }
	else
	  goto REPEAT2;

      }

    /* Open the output file */
    filep = fopen(output_filename,"w");
    if(filep==NULL)
      {
	perror("fopen");
	printf("%s: ERROR: Unable to open file for writing %s\n",
	       __func__, output_filename);

	exit(-1);
      }

    /* Open access to VME windows */
    vmeSetQuietFlag(1);
    status = vmeOpenDefaultWindows();

    vmeCheckMutexHealth(10);
    vmeBusLock();


    if(status < 0)
      {
	printf(" Unable to initialize VME driver\n");
	vmeBusUnlock();
	exit(-1);
      }

    /* Initialize FADCs */
    int iFlag = 0;
    status = faV3Init(fadc_address, (1<<19), nfaV3, iFlag);

    if((status<0) && (nfaV3 == 0))
      {
	printf(" Unable to initialize FADC.\n");
	goto CLOSE;
      }

    unsigned int cfw = faV3GetFirmwareVersions(faV3Slot(0),0);
    printf("%2d: Control Firmware Version: 0x%04x   Proc Firmware Version: 0x%04x\n",
	   faV3Slot(0),cfw&0xFFFF,(cfw>>16)&0xFFFF);

    /* DO THE SCAN */
    faV3GRunDelayScan(1);

 CLOSE:
    vmeBusUnlock();

    fclose(filep);

    status = vmeCloseDefaultWindows();
    if (status != GEF_SUCCESS)
    {
      printf("vmeCloseDefaultWindows failed: code 0x%08x\n",status);
      return -1;
    }

    exit(0);
}

void
sig_handler(int signo)
{
  switch (signo)
    {
    case SIGINT:
      printf("\n\n");
      vmeBusUnlock();
      vmeCloseDefaultWindows();
      exit(1);			/* exit if CRTL/C is issued */
    }
  return;
}

void
faV3GRunDelayScan(int pflag)
{
  int id, ifa, idelay, ich, ret = OK;
  struct timespec delay_scan_waittime, rem;
  char sn[(FAV3_MAX_BOARDS+1)][20];

  uint64_t errors_array[(FAV3_MAX_BOARDS+1)][64];
  unsigned short errors = 0;
  struct b_field_short *bit_short;
  bit_short = (struct b_field_short *) &errors;

  uint64_t delay_scan[(FAV3_MAX_BOARDS+1)][16];
  struct scan_struct scan_results;

  /* Initialize errors_array and delay_scan */
  memset(&errors_array, 0, sizeof(errors_array));
  memset(&delay_scan, 0, sizeof(delay_scan));

  /* Set Measurement time  */
  delay_scan_waittime.tv_sec  = 1;
  delay_scan_waittime.tv_nsec = 0;

  /* Setup ADC Chips */
  if(debugScan)
    {
      printf(" Setup ADC Chips: \n");
    }
  for(ifa = 0; ifa < nfaV3; ifa++)
    {
      id = faV3Slot(ifa);

      if(debugScan)
	{
	  printf(" %2d", id);
	  fflush(stdout);
	}

      if(faV3GetSerialNumber(id, (char **)&sn[id], 0) <= 0)
	printf("ERROR: Slot %d: error reading serial number\n", id);

      ret |= faV3SetPrbsMode(id, 0);
    }

  if(debugScan)
    {
      printf("\n");
    }

  if(debugScan)
    {
      printf(" Set and measure delays: \n");
    }
  for (idelay = 0; idelay < 61; idelay++)
    {
      printf(" %2d:", idelay);
      fflush(stdout);

      for(ifa = 0; ifa < nfaV3; ifa++)
	{
	  id = faV3Slot(ifa);

	  if(debugScan)
	    {
	      printf(" %2d", id);
	      fflush(stdout);
	    }

	  /* Set the delays */
	  for (ich = 0; ich < 16; ich++)
	    faV3SetIdelay(id, ich, idelay, idelay + IDELAY_PN_OFFSET);

	  /* Measure delays */
	  faV3MeasureIdelayErrors(id);
	}

      /* Wait for the measurements to complete */
      ret = nanosleep(&delay_scan_waittime, &rem);
      if(ret < 0)
	perror("nanosleep");

      for(ifa = 0; ifa < nfaV3; ifa++)
	{
	  id = faV3Slot(ifa);

	  /* Get the delay error measurements */
	  errors = faV3GetIdelayErrors(id);
	  errors_array[id][idelay] = (uint64_t) errors;

	}
      if(debugScan)
	{
	  printf("\n");
	}
    }

  // create scan array for each channel
  for (ifa = 0; ifa < nfaV3; ifa++)
    {
      id = faV3Slot(ifa);
      for (ich = 0; ich < 16; ich++)
	{
	  for (idelay = 0; idelay < 61; idelay++)
	    {
	      delay_scan[id][ich] =
		delay_scan[id][ich] | (((errors_array[id][idelay] & (1 << ich)) >> ich) << idelay);
	    }
	}
    }

  if(pflag)
    {
      /* Printout for each FADC */
      for (ifa = 0; ifa < nfaV3; ifa++)
	{
	  id = faV3Slot(ifa);
	  printf("--------------------------"
		 "---------------------------------------------------------------\n");
	  printf("  FADC250V2  sn: %s\n", sn[id]);
	  printf("--------------------------"
		 "---------------------------------------------------------------\n");
	  printf("                          "
		 "          ch 15 14 13 12  11 10  9  8   7  6  5  4   3  2  1  0\n");
	  printf("--------------------------"
		 "---------------------------------------------------------------\n");


	  for (idelay = 0; idelay < 61; idelay++)
	    {
	      bit_short = (struct b_field_short *) &errors_array[id][idelay];
	      printf("delay_p=%2d, delay_n=%2d, errors=0x%04X   "
		     "%d  %d  %d  %d   %d  %d  %d  %d   %d  %d  %d  %d   %d  %d  %d  %d\n",
		     idelay, idelay + IDELAY_PN_OFFSET, errors,
		     bit_short->b_15, bit_short->b_14, bit_short->b_13, bit_short->b_12,
		     bit_short->b_11, bit_short->b_10, bit_short->b_9, bit_short->b_8,
		     bit_short->b_7, bit_short->b_6, bit_short->b_5, bit_short->b_4,
		     bit_short->b_3, bit_short->b_2, bit_short->b_1, bit_short->b_0);
	    }

	  printf("\n");

	  for (ich = 0; ich < 16; ich++)	// create scan array for each channel
	    {
	      printf("ch %2d  delay_scan = %016lX\n", ich, delay_scan[id][ich]);
	    }
	  printf("\n");

	  for (ich = 0; ich < 16; ich++)
	    {
	      //  printf("-- delay_scan = %016llX\n", delay_scan[j]);
	      scan_results = scan_analyze(delay_scan[id][ich]);
	      printf("ch %2d   start = %2d   end = %2d   center = %2d   length = %2d   mult = %2d\n",
		     ich, scan_results.start, scan_results.end,
		     scan_results.center, scan_results.length,
		     scan_results.mult);
	    }
	  printf("\n\n");
	}
    }

  if(filep)
    {
      for (ifa = 0; ifa < nfaV3; ifa++)
	{
	  id = faV3Slot(ifa);
	  fprintf(filep,"# FADC250-V2 %s\n", sn[id]);
	  for (ich = 0; ich < 16; ich++)	// create scan array for each channel
	    {
	      fprintf(filep, "%2d 0x%016lX\n", ich, delay_scan[id][ich]);
	    }
	  fprintf(filep, "\n");
	}
    }
}

int
faV3SetPrbsMode(int id, int pflag)
{
  if(id==0) id=faV3ID[0];

  if((id<=0) || (id>21) || (FAV3p[id] == NULL))
    {
      printf("%s: ERROR : ADC in slot %d is not initialized \n",
	     __func__,id);
      return ERROR;
    }

  if(pflag)
    printf("\n%2d:   ---- Put ADC chips in  PN 23  sequence test mode ----\n", id);

  FALOCK;
  vmeWrite32(&FAV3p[id]->adc.config5, 0x0D05);
  taskDelay(1);

  vmeWrite32(&FAV3p[id]->adc.config4, 0x40);
  taskDelay(1);

  vmeWrite32(&FAV3p[id]->adc.config4, 0xC0);
  taskDelay(1);

  vmeWrite32(&FAV3p[id]->adc.config5, 0xFF01);	/* transfer register values */
  taskDelay(1);

  vmeWrite32(&FAV3p[id]->adc.config4, 0x40);
  taskDelay(1);

  vmeWrite32(&FAV3p[id]->adc.config4, 0xC0);
  taskDelay(1);
  FAUNLOCK;

  if(pflag)
    printf("%2d:   ---- ADC chips in  PN 23  sequence test mode ----\n\n", id);

  return OK;
}

int
faV3SetIdelay(int id, int ch, int delay_p, int delay_n)
{
  int val, delay_p_current, delay_n_current;
  int idelay_default_settings[16] =
    { 46, 47, 50, 53, 55, 53, 54, 52, 60, 60, 59, 59, 60, 60, 57, 53 };

  if(id==0) id=faV3ID[0];

  if((id<=0) || (id>21) || (FAV3p[id] == NULL))
    {
      printf("%s: ERROR : ADC in slot %d is not initialized \n",
	     __func__,id);
      return ERROR;
    }


  if ((delay_p < 0) || (delay_p > 63) || (delay_n < 0) || (delay_n > 63))
    {
      printf("%s: ERROR : delay_n=%d,delay_p=%d must be >=0 and <=63",
	     __func__, delay_n, delay_p);
      return ERROR;
    }

  val = (ch << 12) & FAV3_ADC_CONFIG4_IDELAY_CHAN_MASK;

  FALOCK;
  vmeWrite32(&FAV3p[id]->adc.config4, val);	/* Select channel for Idelay settings */

  val |= FAV3_ADC_CONFIG4_IDELAY_RESET;
  vmeWrite32(&FAV3p[id]->adc.config4, val);	/* Reset Idelay to default setting */

  val &= ~FAV3_ADC_CONFIG4_IDELAY_RESET;
  vmeWrite32(&FAV3p[id]->adc.config4, val);	/* Release reset */

  delay_p_current = idelay_default_settings[ch];
  delay_n_current = idelay_default_settings[ch];

  while (delay_p_current != delay_p)	/* Increment P delay until input value is reached */
    {
      val |= FAV3_ADC_CONFIG4_IDELAY_INC_P;
      vmeWrite32(&FAV3p[id]->adc.config4, val);	/* Increment P delay */

      val &= ~FAV3_ADC_CONFIG4_IDELAY_INC_P;
      vmeWrite32(&FAV3p[id]->adc.config4, val);	/* Release increment P of delay */

      delay_p_current = (delay_p_current + 1) & 0x3f;
    }

  while (delay_n_current != delay_n)	/* Increment N delay until input value is reached */
    {
      val |= FAV3_ADC_CONFIG4_IDELAY_INC_N;
      vmeWrite32(&FAV3p[id]->adc.config4, val);	/* Increment N delay */

      val &= ~FAV3_ADC_CONFIG4_IDELAY_INC_N;
      vmeWrite32(&FAV3p[id]->adc.config4, val);	/* Release increment of N delay */

      delay_n_current = (delay_n_current + 1) & 0x3f;
    }

  FAUNLOCK;

  return OK;
}

int
faV3MeasureIdelayErrors(int id)
{
  int val, ch;

  if(id==0) id=faV3ID[0];

  if((id<=0) || (id>21) || (FAV3p[id] == NULL))
    {
      printf("%s: ERROR : ADC in slot %d is not initialized \n",
	     __func__,id);
      return ERROR;
    }

  FALOCK;

  for (ch = 0; ch < 16; ch++)
    {
      val = (ch << 12) & FAV3_ADC_CONFIG4_IDELAY_CHAN_MASK;
      vmeWrite32(&FAV3p[id]->adc.config4, val);	/* Select channel for Idelay settings */

      val |= FAV3_ADC_CONFIG4_CMP_RESET;
      vmeWrite32(&FAV3p[id]->adc.config4, val);	/* Reset comparator */

      val &= ~FAV3_ADC_CONFIG4_CMP_RESET;
      vmeWrite32(&FAV3p[id]->adc.config4, val);	/* Release comparator reset */
    }

  FAUNLOCK;

  return OK;
}

unsigned short
faV3GetIdelayErrors(int id)
{
  int val, ch, errors = 0;

  if(id==0) id=faV3ID[0];

  if((id<=0) || (id>21) || (FAV3p[id] == NULL))
    {
      printf("%s: ERROR : ADC in slot %d is not initialized \n",
	     __func__,id);
      return ERROR;
    }

  FALOCK;

  for (ch = 0; ch < 16; ch++)
    {
      val = (ch << 12) & FAV3_ADC_CONFIG4_IDELAY_CHAN_MASK;
      vmeWrite32(&FAV3p[id]->adc.config4, val);	/* Select channel for error status */

      // FIXME: status4 DNE
      /* if (vmeRead32(&FAV3p[id]->adc.status4) & FAV3_ADC_STATUS4_CMP_ERR) */
      /* 	errors |= (1 << ch); */
    }

  FAUNLOCK;

  return errors;
}


struct scan_struct
scan_analyze(uint64_t delay_scan)
{
  int i;
  int start_found, end_found;
  uint64_t value;
  uint64_t one = 1;
  struct scan_struct s;

  s.start = 0;
  s.end = 0;
  s.center = 0;
  s.length = 0;
  s.mult = 0;

  start_found = 0;
  end_found = 0;

  for (i = 0; i < 61; i++)
    {
      value = delay_scan & (one << i);
      if (value && (start_found == 0))
	{
	  s.start = i;		// first 0->1 transition
	  start_found = 1;	// includes case of 1 in position 0
	}
      if ((value == 0) && (start_found == 1) && (end_found == 0))
	{
	  s.end = i - 1;	// first 1->0 transition after start
	  s.mult = 1;
	  end_found = 1;
	}
      if (value && (start_found == 1) && (end_found == 1))
	{
	  s.mult = 2;		// 2 or more error zones
	}
    }

  if (start_found && (!end_found))
    s.end = 60;			// case where no 1->0 transition found

  s.center = (s.start + s.end) / 2;	// first error zone centroid
  s.length = s.end - s.start + 1;	// first error zone length

  return s;
}


/*
  Local Variables:
  compile-command: "make -B delay_scan_code"
  End:
 */
