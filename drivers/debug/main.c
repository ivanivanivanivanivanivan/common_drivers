// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <linux/module.h>
#include "main.h"

static int __init debug_main_init(void)
{
	debug_lockup_init();
	cpu_mhz_init();
	meson_atrace_init();

	return 0;
}

static void __exit debug_main_exit(void)
{
}

subsys_initcall(debug_main_init);
module_exit(debug_main_exit);

MODULE_LICENSE("GPL v2");
