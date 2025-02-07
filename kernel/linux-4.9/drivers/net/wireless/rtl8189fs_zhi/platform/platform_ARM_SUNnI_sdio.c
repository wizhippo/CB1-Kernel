/******************************************************************************
 *
 * Copyright(c) 2013 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#include <drv_types.h>
#ifdef CONFIG_GPIO_WAKEUP
#include <linux/gpio.h>
#endif

#include <linux/mmc/host.h>
#include <linux/sunxi-gpio.h>
#include <linux/power/aw_pm.h>

#ifdef CONFIG_GPIO_WAKEUP
extern unsigned int oob_irq;
#endif
extern void sunxi_mmc_rescan_card(unsigned ids);
extern void sunxi_wlan_set_power(bool on);
extern int sunxi_wlan_get_bus_index(void);
extern int sunxi_wlan_get_oob_irq(void);
extern int sunxi_wlan_get_oob_irq_flags(void);

int platform_wifi_power_on(void)
{
	int wlan_bus_index = 0;
	sunxi_wlan_set_power(1);
	mdelay(100);

	wlan_bus_index = sunxi_wlan_get_bus_index();
	if(wlan_bus_index < 0){
		printk("get wifi_sdc_id failed\n");
		return -1;
	} else {
		printk("----- %s sdc_id: %d\n", __FUNCTION__, wlan_bus_index);
		sunxi_mmc_rescan_card(wlan_bus_index);
	}

#ifdef CONFIG_GPIO_WAKEUP
	oob_irq = sunxi_wlan_get_oob_irq();
#endif
	return 0;
}

void platform_wifi_power_off(void)
{
	sunxi_wlan_set_power(0);
	mdelay(100);
	printk("%s: remove card, power off.\n", __FUNCTION__);
}
