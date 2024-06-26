/*
 * File:
 *    faV3GFirmwareUpdate
 *
 * Description:
 *    JLab Flash250 V3 firmware updating
 *     Multiple boards, using slot number addressing convention to ID faV3 in crate
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "jvme.h"
#include "faV3Lib.h"
#include "faV3FirmwareTools.h"

#define FADC_ADDR (3<<19)
#define NFADC     16
#define SKIPSS

extern int nfaV3;
char *progName;

void Usage();

int
main(int argc, char *argv[])
{

  int32_t status;
  int fpga_choice, firmware_choice = 0;
  char *mcs_filename;
  char inputchar[16];
  int ifa = 0;
  uint32_t cfw = 0;

  printf("\nJLAB fadc firmware update\n");
  printf("----------------------------\n");

  progName = argv[0];

  if(argc < 2)
    {
      printf(" ERROR: Must specify one argument\n");
      Usage();
      exit(-1);
    }
  else
    {
      mcs_filename = argv[1];
    }

  if(faV3FirmwareReadMcsFile(mcs_filename) != OK)
    {
      exit(-1);
    }


  fpga_choice = faV3FirmwareChipFromFpgaID(0);
  if(fpga_choice == ERROR)
    {
      printf(" ERROR: Did not obtain FPGA type from firmware file.\n");
      printf("        Please specific FPGA type\n");
      printf("          1 for FX70T (Control FPGA)\n");
      printf("          2 for LX110 (Processing FPGA)\n");
      printf("    or q and <ENTER> to quit without update\n");
      printf("\n");

    REPEAT:
      printf(" (y/n): ");
      scanf("%s", (char *) &inputchar);

      if((strcmp(inputchar, "q") == 0) || (strcmp(inputchar, "Q") == 0))
	{
	  printf("--- Exiting without update ---\n");
	  exit(0);
	}
      else if(strcmp(inputchar, "1") == 0)
	{
	  fpga_choice = 1;
	}
      else if(strcmp(inputchar, "2") == 0)
	{
	  fpga_choice = 2;
	}
      else
	{
	  goto REPEAT;
	}

    }

  vmeSetQuietFlag(1);
  status = vmeOpenDefaultWindows();

  vmeCheckMutexHealth(10);
  vmeBusLock();
  int iFlag = (1 << 18);	// Skip firmware check

#ifdef SKIPSS
  faV3Init((uint32_t) (FADC_ADDR), (1 << 19), NFADC + 2, iFlag);
#else
  faV3Init((uint32_t) (FADC_ADDR), (1 << 19), NFADC, iFlag);
#endif

  if(nfaV3 == 0)
    {
      printf(" Unable to initialize any FADCs.\n");
      goto CLOSE;
    }

  for(ifa = 0; ifa < nfaV3; ifa++)
    {
      cfw = faV3GetFirmwareVersions(faV3Slot(ifa), 0);
      printf("%2d: Control Firmware Version: 0x%04x   Proc Firmware Version: 0x%04x\n",
	     faV3Slot(ifa), cfw & 0xFFFF, (cfw >> 16) & 0xFFFF);
    }

  printf(" Will update firmware for ");
  if(fpga_choice == 1)
    {
      firmware_choice = FAV3_FIRMWARE_FX70T;
      printf("FX70T (Control FPGA) ");
    }
  else if((fpga_choice == 2) || (fpga_choice == 0))
    {
      firmware_choice = FAV3_FIRMWARE_LX110;
      printf("LX110 (Processing FPGA) ");
    }

  printf(" with file: \n   %s", mcs_filename);
  if(faV3FirmwareRevFromFpgaID(0))
    {
      printf(" (rev = 0x%x)\n", faV3FirmwareRevFromFpgaID(0));
    }
  else
    {
      printf("\n");
    }

 REPEAT2:
  printf(" Press y and <ENTER> to continue... n or q and <ENTER> to quit without update\n");

  scanf("%s", (char *) inputchar);

  if((strcmp(inputchar, "q") == 0) || (strcmp(inputchar, "Q") == 0) ||
     (strcmp(inputchar, "n") == 0) || (strcmp(inputchar, "N") == 0))
    {
      printf(" Exiting without update\n");
      goto CLOSE;
    }
  else if((strcmp(inputchar, "y") == 0) || (strcmp(inputchar, "Y") == 0))
    {
    }
  else
    goto REPEAT2;

  faV3FirmwareGLoad(firmware_choice, 0);

  goto CLOSE;

 CLOSE:

  vmeBusUnlock();
  status = vmeCloseDefaultWindows();
  if(status != OK)
    {
      printf("vmeCloseDefaultWindows failed: code 0x%08x\n", status);
      return -1;
    }

  exit(0);
}


void
Usage()
{
  printf("\n");
  printf("%s <firmware MCS file>\n", progName);
  printf("\n");

}
