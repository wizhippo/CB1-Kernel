/*
 * Allwinner sun50iw9p1 SoCs R_PIO pinctrl driver.
 *
 * Copyright(c) 2012-2016 Allwinnertech Co., Ltd.
 * Author: WimHuang <huangwei@allwinnertech.com>
 *
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin sun50iw9p1_r_pins[] = {
	//Register Name: PL_CFG0
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_rsb0"),		/* SCK */
		SUNXI_FUNCTION(0x3, "s_twi0"),		/* SCK */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "s_rsb0"),		/* SDA */
		SUNXI_FUNCTION(0x3, "s_twi0"),		/* SDA */
		SUNXI_FUNCTION(0x7, "io_disabled")),
};

static const unsigned sun50iw9p1_r_bank_base[] = {
	SUNXI_R_PIO_BANK_BASE(PL_BASE, 0),
};

static const struct sunxi_pinctrl_desc sun50iw9p1_r_pinctrl_data = {
	.pins = sun50iw9p1_r_pins,
	.npins = ARRAY_SIZE(sun50iw9p1_r_pins),
	.pin_base = PL_BASE,
	.banks = ARRAY_SIZE(sun50iw9p1_r_bank_base),
	.bank_base = sun50iw9p1_r_bank_base,
	.irq_banks = 0,
	.irq_bank_base = NULL,
};

static int sun50iw9p1_r_pinctrl_probe(struct platform_device *pdev)
{
	return sunxi_pinctrl_init(pdev, &sun50iw9p1_r_pinctrl_data);
}

static struct of_device_id sun50iw9p1_r_pinctrl_match[] = {
	{ .compatible = "allwinner,sun50iw9p1-r-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun50iw9p1_r_pinctrl_match);

static struct platform_driver sun50iw9p1_r_pinctrl_driver = {
	.probe	= sun50iw9p1_r_pinctrl_probe,
	.driver	= {
		.name		= "sun50iw9p1-r-pinctrl",
		.owner		= THIS_MODULE,
		.pm		= &sunxi_pinctrl_pm_ops,
		.of_match_table	= sun50iw9p1_r_pinctrl_match,
	},
};

static int __init sun50iw9p1_r_pio_init(void)
{
	int ret;
    
    sunxi_pinctrl_pio_register();
    
	ret = platform_driver_register(&sun50iw9p1_r_pinctrl_driver);
	if (ret) {
		pr_debug("register sun50i r-pio controller failed\n");
		return -EINVAL;
	}
	return 0;
}
postcore_initcall(sun50iw9p1_r_pio_init);

MODULE_AUTHOR("WimHuang<huangwei@allwinnertech.com>");
MODULE_DESCRIPTION("Allwinner sun50iw9p1 R_PIO pinctrl driver");
MODULE_LICENSE("GPL");
