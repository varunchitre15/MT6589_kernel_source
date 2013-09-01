/*
 *  linux/arch/arm/mach-versatile/clock.c
 *
 *  Copyright (C) 2004 ARM Limited.
 *  Written by Deep Blue Solutions Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/mutex.h>
//FIX-ME: marked for early porting
//#include <asm/clkdev.h>
#include <asm/hardware/icst.h>

#include "mach/clock.h"

int clk_enable(struct clk *clk)
{
	return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
    return 24000000;
}
EXPORT_SYMBOL(clk_get_rate);

void clk_put(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_put);

struct clk *clk_get(struct device *dev, const char *id)
{
    return NULL;
}

int clk_set_rate(struct clk *clk, unsigned long rate)
{
        int ret = -EIO;

        return ret;
}
EXPORT_SYMBOL(clk_set_rate);

