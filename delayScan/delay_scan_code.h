/*
 *  delay_scan_code.h  - Header file for delay_scan_code program
 *
 *  Authors: Ben Raydo, Ed Jastrzembski, Bryan Moffit
 *           Jefferson Lab Data Acquisition Group
 *           September 2019
 *
 *----------------------------------------------------------------------------*/

#define FAV3_ADC_CONFIG4_IDELAY_CHAN_MASK  0xF000	/* For delay scan  */
#define FAV3_ADC_CONFIG4_IDELAY_RESET      0x0100
#define FAV3_ADC_CONFIG4_IDELAY_INC_P      0x0200
#define FAV3_ADC_CONFIG4_IDELAY_INC_N      0x0400
#define FAV3_ADC_CONFIG4_CMP_RESET         0x0800
#define FAV3_ADC_STATUS4_CMP_ERR           0x0400
#define IDELAY_PN_OFFSET  3

#define FALOCK      if(pthread_mutex_lock(&faV3Mutex)<0) perror("pthread_mutex_lock");
#define FAUNLOCK    if(pthread_mutex_unlock(&faV3Mutex)<0) perror("pthread_mutex_unlock");

struct scan_struct
{
  int start;
  int end;
  int center;
  int length;
  int mult;
};

struct b_field_short
{
  uint8_t b_0:1;
  uint8_t b_1:1;
  uint8_t b_2:1;
  uint8_t b_3:1;
  uint8_t b_4:1;
  uint8_t b_5:1;
  uint8_t b_6:1;
  uint8_t b_7:1;
  uint8_t b_8:1;
  uint8_t b_9:1;
  uint8_t b_10:1;
  uint8_t b_11:1;
  uint8_t b_12:1;
  uint8_t b_13:1;
  uint8_t b_14:1;
  uint8_t b_15:1;
};

/* function prototypes */
void sig_handler(int signo);
int faV3SetPrbsMode(int id, int pflag);
int faV3SetIdelay(int id, int ch, int delay_p, int delay_n);
int faV3MeasureIdelayErrors(int id);
unsigned short faV3GetIdelayErrors(int id);
void faV3GRunDelayScan(int pflag);
struct scan_struct scan_analyze(uint64_t delay_scan);
