/*
 * Copyright (C) 2016 Allwinner.
 * wangwei <wangwei@allwinnertech.com>
 *
 * SUNXI AXP21  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __AXP2202_H__
#define __AXP2202_H__

#include <arch/axp.h>

/*PMIC chip id reg03:bit7-6  bit3-*/

#define AXP2202_DEVICE_ADDR			(0x3A3)
#ifdef CFG_SUNXI_TWI
#define AXP2202_RUNTIME_ADDR			(0x34)
#else
#define AXP2202_RUNTIME_ADDR                      (0x2d)
#endif
/* define AXP21 REGISTER */
#define   AXP2202_MODE_CHGSTATUS      	(0x01)
#define   AXP2202_CHIP_ID        	 		(0x03)
#define   AXP2202_CHIP_VER        	 	(0x04)

#define   AXP2202_OUTPUT_CTL0    	   		(0x80)
#define   AXP2202_OUTPUT_CTL1     	 	(0x81)
#define   AXP2202_OUTPUT_CTL2     		(0x90)
#define   AXP2202_OUTPUT_CTL3     		(0x91)

#define   AXP2202_DC1OUT_VOL				(0x83)
#define   AXP2202_DC2OUT_VOL          	(0x84)
#define   AXP2202_DC3OUT_VOL          	(0x85)
#define   AXP2202_DC4OUT_VOL          	(0x86)

#define   AXP2202_ALDO1OUT_VOL			(0x93)
#define   AXP2202_ALDO2OUT_VOL			(0x94)
#define   AXP2202_ALDO3OUT_VOL			(0x95)
#define   AXP2202_ALDO4OUT_VOL			(0x96)

#define   AXP2202_BLDO1OUT_VOL			(0x97)
#define   AXP2202_BLDO2OUT_VOL			(0x98)
#define   AXP2202_BLDO3OUT_VOL			(0x99)
#define   AXP2202_BLDO4OUT_VOL			(0x9A)

#define   AXP2202_CLDO1OUT_VOL			(0x9B)
#define   AXP2202_CLDO2OUT_VOL			(0x9C)
#define   AXP2202_CLDO3OUT_VOL			(0x9D)
#define   AXP2202_CLDO4OUT_VOL			(0x9E)

#define   AXP2202_CPUSLDO_VOL				(0x9F)


#define   AXP2202_PWRON_STATUS           	(0x20)
#define   AXP2202_PWEON_PWEOFF_EN			(0x22)
#define   AXP2202_DCDC_PWEOFF_EN			(0x23)
#define   AXP2202_VSYS_MIN				(0x15)
#define   AXP2202_VBUS_VOL_SET         	(0x16)
#define   AXP2202_VBUS_CUR_SET          	(0x17)
#define   AXP2202_SELLP_CFG				(0x25)
#define   AXP2202_OFF_CTL             	(0x27)
#define   AXP2202_CHARGE1             	(0x62)
#define   AXP2202_CHGLED_SET             	(0x70)

#define   AXP2202_INTEN0              	(0x40)
#define   AXP2202_INTEN1              	(0x41)
#define   AXP2202_INTEN2              	(0x42)
#define   AXP2202_INTEN3              	(0x43)
#define   AXP2202_INTEN4              	(0x44)


#define   AXP2202_INTSTS0             	(0x48)
#define   AXP2202_INTSTS1             	(0x49)
#define   AXP2202_INTSTS2             	(0x4a)
#define   AXP2202_INTSTS3             	(0x4b)
#define   AXP2202_INTSTS4             	(0x4c)

#define   AXP2202_ADC_CH0         		 (0xC0)
#define   AXP2202_BAT_AVERVOL_H6          (0xC4)
#define   AXP2202_BAT_AVERVOL_L8          (0xC5)

#define   AXP2202_TWI_ADDR_EXT			(0xFF)
#define   AXP2202_EFUS_OP_CFG			(0xF0)
#define   AXP2202_EFREQ_CTRL			(0xF1)

int axp2202_probe_power_key(void);
int axp2202_set_ddr_voltage(int set_vol);
int axp2202_set_pll_voltage(int set_vol);
int axp2202_set_efuse_voltage(int set_vol);
int axp2202_set_sys_voltage(int set_vol, int onoff);
int axp2202_set_sys_voltage_ext(char *name, int set_vol, int onoff);
int axp2202_axp_init(u8 power_mode);


#endif /* __AXP2202_REGS_H__ */

