#ifndef _USER_CONFIG_OVERRIDE_H_
#define _USER_CONFIG_OVERRIDE_H_

// Remove as much stuff to get the image in at under 512k to allow over the air updates
// The curtain is a pain in the ass to flash, so I only want to do this once!

#undef USE_DOMOTICZ
#undef USE_TIMERS
#undef USE_TIMERS_WEB
#undef USE_SUNRISE
#undef USE_RULES

#undef ROTARY_V1
#undef USE_SONOFF_RF
#undef USE_SONOFF_SC
#undef USE_TUYA_MCU
#undef USE_ARMTRONIX_DIMMERS
#undef USE_PS_16_DZ
#undef USE_SONOFF_IFAN
#undef USE_BUZZER
#undef USE_ARILUX_RF
#undef USE_SHUTTER
#undef USE_DEEPSLEEP
#undef USE_EXS_DIMMER
#undef USE_DEVICE_GROUPS
#undef USE_PWM_DIMMER
#undef USE_SONOFF_D1
#undef USE_SHELLY_DIMMER

#undef USE_WS2812
#undef USE_MY92X1
#undef USE_SM16716
#undef USE_SM2135
#undef USE_SM2335
#undef USE_BP1658CJ
#undef USE_BP5758D
#undef USE_SONOFF_L1
#undef USE_ELECTRIQ_MOODL
#undef USE_LIGHT_PALETTE
#undef USE_LIGHT_VIRTUAL_CT
#undef USE_DGR_LIGHT_SEQUENCE

#undef USE_COUNTER

#undef USE_DS18x20

#undef USE_I2C

#undef USE_SPI

#undef USE_SERIAL_BRIDGE

#undef USE_ENERGY_SENSOR

#undef USE_ENERGY_MARGIN_DETECTION
#undef USE_ENERGY_DUMMY
#undef USE_HLW8012
#undef USE_CSE7766
#undef USE_PZEM004T
#undef USE_PZEM_AC
#undef USE_PZEM_DC
#undef USE_MCP39F501
#undef USE_BL09XX

#undef USE_DHT
#undef USE_IR_REMOTE

// ---

// The important one - add in our driver!
#define USE_EWELINK_GATEWAY

#endif  // _USER_CONFIG_OVERRIDE_H_
