/*
 * linux/sunxi-sid.h
 *
 * Copyright(c) 2014-2016 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: sunny <sunny@allwinnertech.com>
 *
 * allwinner sunxi soc chip version and chip id manager.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SUNXI_MACH_SUNXI_CHIP_H
#define __SUNXI_MACH_SUNXI_CHIP_H

#include <linux/types.h>
/* About ChipID of version */

//extern int ephy_config_init(struct phy_device *phydev);

#define SUNXI_CHIP_REV(p, v)  (p + v)

#define SUNXI_CHIP_SUN8IW6   (0x16730000)
#define SUN8IW6P1_REV_A SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW6, 0x0000)
#define SUN8IW6P1_REV_B SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW6, 0x0001)

#define SUNXI_CHIP_SUN8IW7   (0x16800000)
#define SUN8IW7P1_REV_A SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW7, 0x0000)
#define SUN8IW7P1_REV_B SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW7, 0x0001)
#define SUN8IW7P2_REV_A SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW7, 0x0100)
#define SUN8IW7P2_REV_B SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW7, 0x0101)

#define SUNXI_CHIP_SUN8IW11   (0x17010000)
#define SUN8IW11P1_REV_A SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW11, 0x0000)
#define SUN8IW11P2_REV_A SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW11, 0x0001)
#define SUN8IW11P3_REV_A SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW11, 0x0011)
#define SUN8IW11P4_REV_A SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW11, 0x0101)

#define SUNXI_CHIP_SUN8IW12   (0x17210000)
#define SUN8IW12P1_REV_A SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW12, 0x0000)

#define SUNXI_CHIP_SUN8IW15   (0x17550000)
#define SUN8IW15P1_REV_A SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW15, 0x0000)

#define SUNXI_CHIP_SUN8IW16   (0x18160000)
#define SUN8IW16P1_REV_A SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW16, 0x0000)
#define SUN8IW16P1_REV_B SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW16, 0x0001)

#define SUNXI_CHIP_SUN8IW19   (0x18170000)
#define SUN8IW19P1_REV_A SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW19, 0x0000)

#define SUNXI_CHIP_SUN8IW17   (0x17080000)
#define SUN8IW17P1_REV_A SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW17, 0x0000)

#define SUNXI_CHIP_SUN8IW18   (0x18210000)
#define SUN8IW18P1_REV_A SUNXI_CHIP_REV(SUNXI_CHIP_SUN8IW18, 0x0000)

#define SUNXI_CHIP_SUN50IW1   (0x16890000)
#define SUN50IW1P1_REV_A	SUNXI_CHIP_REV(SUNXI_CHIP_SUN50IW1, 0x0)

#define SUNXI_CHIP_SUN50IW2   (0x17180000)
#define SUN50IW2P1_REV_A	SUNXI_CHIP_REV(SUNXI_CHIP_SUN50IW2, 0x0)

#define SUNXI_CHIP_SUN50IW3   (0x17190000)
#define SUN50IW3P1_REV_A	SUNXI_CHIP_REV(SUNXI_CHIP_SUN50IW3, 0x0)

#define SUNXI_CHIP_SUN50IW6   (0x17280000)
#define SUN50IW6P1_REV_A	SUNXI_CHIP_REV(SUNXI_CHIP_SUN50IW6, 0x0)

#define SUNXI_CHIP_SUN50IW9   (0x18230000)
#define SUN50IW9P1_REV_A	SUNXI_CHIP_REV(SUNXI_CHIP_SUN50IW9, 0x0)
#define SUN50IW9P1_REV_B	SUNXI_CHIP_REV(SUNXI_CHIP_SUN50IW9, 0x1)

#define SUNXI_CHIP_SUN50IW10  (0x18550000)
#define SUN50IW10P1_REV_A	SUNXI_CHIP_REV(SUNXI_CHIP_SUN50IW10, 0x0)

/* The key info in Efuse */

#define EFUSE_CHIPID_NAME            "chipid"
#define EFUSE_BROM_CONF_NAME         "brom_conf"
#define EFUSE_BROM_TRY_NAME          "brom_try"
#define EFUSE_THM_SENSOR_NAME        "thermal_sensor"
#define EFUSE_FT_ZONE_NAME           "ft_zone"
#define EFUSE_TV_OUT_NAME            "tvout"
#define EFUSE_TVE_NAME               "tve"
#define EFUSE_OEM_NAME               "oem"
#define EFUSE_ANTI_BLUSH_NAME        "anti_blushing"

#define EFUSE_PSENSOR_NAME           "psensor"
#define EFUSE_DDR_CFG_NAME           "ddr_cfg"
#define EFUSE_LDOA_NAME              "ldoa"
#define EFUSE_LDOB_NAME              "ldob"
#define EFUSE_AUDIO_BIAS_NAME        "audio_bias"
#define EFUSE_GAMMA_NAME             "gamma"
#define EFUSE_WR_PROTECT_NAME        "write_protect"
#define EFUSE_RD_PROTECT_NAME        "read_protect"
#define EFUSE_IN_NAME                "in"
#define EFUSE_ID_NAME                "id"
#define EFUSE_ROTPK_NAME             "rotpk"
#define EFUSE_SSK_NAME               "ssk"
#define EFUSE_RSSK_NAME              "rssk"
#define EFUSE_HDCP_HASH_NAME         "hdcp_hash"
#define EFUSE_HDCP_PKF_NAME          "hdcp_pkf"
#define EFUSE_HDCP_DUK_NAME          "hdcp_duk"
#define EFUSE_EK_HASH_NAME           "ek_hash"
#define EFUSE_SN_NAME                "sn"
#define EFUSE_NV1_NAME               "nv1"
#define EFUSE_NV2_NAME               "nv2"
#define EFUSE_BACKUP_KEY_NAME        "backup_key"
#define EFUSE_BACKUP_KEY2_NAME       "backup_key2"
#define EFUSE_TCON_PARM_NAME         "tcon_parm"
#define EFUSE_RSAKEY_HASH_NAME       "rsakey_hash"
#define EFUSE_RENEW_NAME             "renewability"
#define EFUSE_OPT_ID_NAME            "operator_id"
#define EFUSE_LIFE_CYCLE_NAME        "life_cycle"
#define EFUSE_JTAG_SECU_NAME         "jtag_security"
#define EFUSE_JTAG_ATTR_NAME         "jtag_attr"
#define EFUSE_CHIP_CONF_NAME         "chip_config"
#define EFUSE_RESERVED_NAME          "reserved"
#define EFUSE_RESERVED2_NAME         "reserved2"

#define SUNXI_KEY_NAME_LEN	32

#define SID_PRCTL			0x40
#define SID_RDKEY			0x60
#define SID_OP_LOCK			0xAC

#define EFUSE_CHIPID_BASE	"allwinner,sunxi-chipid"
#define EFUSE_SID_BASE		"allwinner,sunxi-sid"

#define sunxi_efuse_read(key_name, read_buf) \
		sunxi_efuse_readn(key_name, read_buf, 1024)

/* The interface functions */

unsigned int sunxi_get_soc_ver(void);
int sunxi_get_soc_chipid(unsigned char *chipid);
int sunxi_get_soc_chipid_str(char *chipid);
int sunxi_get_soc_ft_zone_str(char *serial);
int sunxi_get_soc_rotpk_status_str(char *status);
int sunxi_get_pmu_chipid(unsigned char *chipid);
int sunxi_get_serial(unsigned char *serial);
unsigned int sunxi_get_soc_bin(void);
int sunxi_soc_is_secure(void);
s32 sunxi_get_platform(s8 *buf, s32 size);
s32 sunxi_efuse_readn(void *key_name, void *buf, u32 n);

#endif  /* __SUNXI_MACH_SUNXI_CHIP_H */
