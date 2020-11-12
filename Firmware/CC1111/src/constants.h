
#define _INIT_INFO_JESUS_

#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

/***********************************************************************************
* CONSTANTS
*/
//#define GREEN_LED       P1_1
#define GREEN_LED       P2_0
// Bit masks to check CLKCON register
#define OSC_BIT           0x40  // bit mask used to select/check the system clock oscillator
#define TICKSPD_BITS      0x38  // bit mask used to check the timer ticks output setting
#define CLKSPD_BIT        0x03  // bit maks used to check the clock speed
#define MAIN_OSC_BITS     0x7F  // bit mask used to control the system clock oscillator
                                // e.g. ~MAIN_OSC_BITS can be used to start Crystal OSC

// Bit masks to check SLEEP register
#define XOSC_STABLE_BIT   0x40  // bit mask used to check the stability of XOSC
#define HFRC_STB_BIT      0x20  // bit maks used to check the stability of the High-frequency RC oscillator
#define OSC_PD_BIT        0x04  // bit maks used to power down system clock oscillators

// Macro for checking status of the crystal oscillator
#define XOSC_STABLE (SLEEP & XOSC_STABLE_BIT)

// RF States
#define RFST_SFSTXON      0x00
#define RFST_SCAL         0x01
#define RFST_SRX          0x02
#define RFST_STX          0x03
#define RFST_SIDLE        0x04

#define SIZE_OF_RF_BUFFER  128  // 71 bytes + 4b6b = 106.5 Bytes

// Size of allocated UART RX/TX buffer (just an example)
#define SIZE_OF_UART_RX_BUFFER   85
#define SIZE_OF_UART_TX_BUFFER   SIZE_OF_UART_RX_BUFFER

// Baudrate = 9600 bps (U0BAUD.BAUD_M = 163, U0GCR.BAUD_E = 8)
#define UART_BAUD_M  163
#define UART_BAUD_E  8

// Time Constants
#define TICKS_PER_SECOND      5

// RF Constants
//  Init Freqs: 868.0 MHz (EUR) and 915.0 MHz (US)
//  RF Channel spacing: 25 KHz
#define _RF_CHANNEL_SPACING_     0x00000044
#define _INIT_FREQ_EUR_          0x00242CCA
#define _INIT_FREQ_USA_          0x00262BC0

// Possible bandwidths :
//   750 KHz -> 0x00, 600 KHz -> 0x10, 500 KHz -> 0x20, 429 KHz -> 0x30,
//   375 KHz -> 0x40, 300 KHz -> 0x50, 250 KHz -> 0x60, 214 KHz -> 0x70,
//   188 KHz -> 0x80, 150 KHz -> 0x90, 125 KHz -> 0xA0, 107 KHz -> 0xB0,
//    94 KHz -> 0xC0,  75 KHz -> 0xD0,  63 KHz -> 0xE0,  54 KHz -> 0xF0
#define _WIDE_BANDWIDTH_      0x10 // 600 KHz
#define _NARROW_BANDWIDTH_    0xE0 //  63 KHz

#define _MAX_TIMECOUNTER_VAL_ ((unsigned long)((16+1)*(5*60+0)*TICKS_PER_SECOND))

#define _PUMP_AWAKE_TIME_ ((1*60 + 0)*TICKS_PER_SECOND)

// DATA SOURCES
#define DATA_SOURCE_MYSENTRY                    0x00
#define DATA_SOURCE_MINILINK_MANUAL             0x01
#define DATA_SOURCE_MINILINK_PUMP_CAL           0x02

// PUMP COMMAND FLAGS
#define PUMP_SUSPEND_CMD_FLAG                   0x01
#define PUMP_RESUME_CMD_FLAG                    0x02
#define PUMP_SET_TEMP_BASAL_CMD_FLAG            0x04
#define PUMP_CANCEL_TEMP_BASAL_CMD_FLAG         0x08
#define PUMP_SET_BOLUS_CMD_FLAG                 0x10
#define PUMP_READ_PUMP_CONFIG_CMD_FLAG          0x20
#define PUMP_READ_SENSOR_CAL_FACTOR_CMD_FLAG    0x40
#define PUMP_READ_STATUS_CMD_FLAG               0x80
#define PUMP_TIMEOUT                            0x00
#define PUMP_RESET_NOTIFICATION                 0xFF

// ACTION CONSTRAINTS
#define MAX_TEMP_BASAL_RATE        4.0  // Units
#define MAX_TEMP_BASAL_DURATION    2.0  // Hours
#define MAX_BOLUS_AMOUNT          14.0  // Units

#define MAX_NUM_TARGETS               20
#define MAX_NUM_BASALS                20
#define MAX_NUM_CARB_RATIOS           20
#define MAX_NUM_INSULIN_SENS          20

// UNITS
#define BG_UNITS_NONE               0x00
#define BG_UNITS_MGDL               0x01
#define BG_UNITS_MMOLL              0x02

#define CARB_UNITS_NONE             0x00
#define CARB_UNITS_GRAMS            0x01
#define CARB_UNITS_EXCH             0x02

// CONTROL LOOP CONSTANTS
#define NO_CONTROL_LOOP                         0x00
#define TEMPBASAL_CONTROL_LOOP                  0x01

#define CMD_PUMP_ACK                            0x06
#define CMD_BOLUS                               0x42
#define CMD_SET_TEMP_BASAL                      0x4C
#define CMD_PUMP_SUSPEND                        0x4D
#define CMD_SET_RF_STATUS                       0x5D
#define CMD_READ_PUMP_RTC                       0x70
#define CMD_READ_PUMP_BATTERY_STATUS            0x72
#define CMD_READ_PUMP_REMAINING_INSULIN         0x73
#define CMD_READ_PUMP_FW_VERSION                0x74
#define CMD_READ_PUMP_STATE                     0x80
#define CMD_READ_BOLUS_WIZARD_SETUP             0x87
#define CMD_READ_CARB_UNITS                     0x88
#define CMD_READ_BG_UNITS                       0x89
#define CMD_READ_CARB_RATIOS                    0x8A
#define CMD_READ_INSULIN_SENSITIVITIES          0x8B
#define CMD_READ_PUMP_MODEL                     0x8D
#define CMD_READ_CALIBRATION_FACTOR             0x9C
#define CMD_READ_STD_BASAL_PROFILE              0x92
#define CMD_READ_PUMP_STATUS                    0x98
#define CMD_READ_BG_TARGETS                     0x9F
#define CMD_READ_PUMP_PARAMS                    0xC0

#define MAX_NUM_IOB_REGISTERS                   96
#define MAX_NUM_FUTURE_IOBS                      6

#endif