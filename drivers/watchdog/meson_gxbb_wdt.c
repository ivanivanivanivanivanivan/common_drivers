// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2016 BayLibre, SAS.
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 *
 */
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#ifdef CONFIG_AMLOGIC_MODIFY
#include <linux/panic_notifier.h>
#include <linux/of_device.h>
#include <watchdog_core.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/amlogic/gki_module.h>

#define DRIVER_NAME		"meson_gxbb_wdt"
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
#include <linux/amlogic/aml_iotrace.h>
#endif

#ifdef CONFIG_AMLOGIC_MODIFY
#define DEFAULT_TIMEOUT 60      /* seconds */
#define DEFAULT_PRETIMEOUT 0
#else
#define DEFAULT_TIMEOUT	30	/* seconds */
#endif

#define GXBB_WDT_CTRL_REG			0x0
#define GXBB_WDT_CTRL1_REG			0x4
#define GXBB_WDT_TCNT_REG			0x8
#define GXBB_WDT_RSET_REG			0xc

#define GXBB_WDT_CTRL_CLKDIV_EN			BIT(25)
#define GXBB_WDT_CTRL_CLK_EN			BIT(24)
#ifdef CONFIG_AMLOGIC_MODIFY
#define GXBB_WDT_CTRL_IRQ_EN			BIT(23)
#endif
#define GXBB_WDT_CTRL_EE_RESET			BIT(21)
#define GXBB_WDT_CTRL_EN			BIT(18)
#define GXBB_WDT_CTRL_DIV_MASK			(BIT(18) - 1)

#define GXBB_WDT_TCNT_SETUP_MASK		(BIT(16) - 1)
#define GXBB_WDT_TCNT_CNT_SHIFT			16
#define GXBB_WDT_RST_SIG_EN			BIT(17)

struct meson_gxbb_wdt {
	void __iomem *reg_base;
	struct watchdog_device wdt_dev;
	struct clk *clk;
#ifdef CONFIG_AMLOGIC_MODIFY
	unsigned int feed_watchdog_mode;
	struct notifier_block notifier;
	void __iomem *reg_offset;
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct dentry *debugfs_dir;
#endif
	u32 *shadow;
	int shadow_size;
#endif
};

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
static int wdt_debug;
module_param(wdt_debug, int, 0644);
#endif

#ifdef CONFIG_AMLOGIC_MODIFY
static unsigned int watchdog_enabled = 1;
static int get_watchdog_enabled_env(char *str)
{
	return kstrtouint(str, 0, &watchdog_enabled);
}
__setup("watchdog_enabled=", get_watchdog_enabled_env);

static int stop_after_panic;
module_param(stop_after_panic, int, 0644);
MODULE_PARM_DESC(stop_after_panic, "Stop watchdog after panic (0=keep watching, 1=stop)");
static int get_stop_after_panic_env(char *str)
{
	return kstrtoint(str, 0, &stop_after_panic);
}
__setup("wdt_stop_after_panic=", get_stop_after_panic_env);

static int wdt_panic_print_setup(char *str)
{
	stop_after_panic = 1;

	return 1;
}
__setup("panic_print=", wdt_panic_print_setup);
#endif

static int meson_gxbb_wdt_start(struct watchdog_device *wdt_dev)
{
	struct meson_gxbb_wdt *data = watchdog_get_drvdata(wdt_dev);

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	if (wdt_debug)
		pr_info("meson gxbb wdt start\n");
#endif
	writel(readl(data->reg_base + GXBB_WDT_CTRL_REG) | GXBB_WDT_CTRL_EN,
	       data->reg_base + GXBB_WDT_CTRL_REG);

	return 0;
}

static int meson_gxbb_wdt_stop(struct watchdog_device *wdt_dev)
{
	struct meson_gxbb_wdt *data = watchdog_get_drvdata(wdt_dev);

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	if (wdt_debug)
		pr_info("meson gxbb wdt stop\n");
#endif
	writel(readl(data->reg_base + GXBB_WDT_CTRL_REG) & ~GXBB_WDT_CTRL_EN,
	       data->reg_base + GXBB_WDT_CTRL_REG);

	return 0;
}

static int meson_gxbb_wdt_ping(struct watchdog_device *wdt_dev)
{
	struct meson_gxbb_wdt *data = watchdog_get_drvdata(wdt_dev);

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	if (wdt_debug)
		pr_info("meson gxbb wdt ping\n");
#endif
	writel(0, data->reg_base + GXBB_WDT_RSET_REG);

	return 0;
}

static int meson_gxbb_wdt_set_timeout(struct watchdog_device *wdt_dev,
				      unsigned int timeout)
{
	struct meson_gxbb_wdt *data = watchdog_get_drvdata(wdt_dev);
	unsigned long tcnt = timeout * 1000;

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	if (wdt_debug)
		pr_info("%s() timeout=%u\n", __func__, timeout);
#endif
	if (tcnt > GXBB_WDT_TCNT_SETUP_MASK)
		tcnt = GXBB_WDT_TCNT_SETUP_MASK;

	wdt_dev->timeout = timeout;

	meson_gxbb_wdt_ping(wdt_dev);

	writel(tcnt, data->reg_base + GXBB_WDT_TCNT_REG);

	return 0;
}

static unsigned int meson_gxbb_wdt_get_timeleft(struct watchdog_device *wdt_dev)
{
	struct meson_gxbb_wdt *data = watchdog_get_drvdata(wdt_dev);
	unsigned long reg;

	reg = readl(data->reg_base + GXBB_WDT_TCNT_REG);

	return ((reg & GXBB_WDT_TCNT_SETUP_MASK) -
		(reg >> GXBB_WDT_TCNT_CNT_SHIFT)) / 1000;
}

#ifdef CONFIG_AMLOGIC_MODIFY
static int meson_gxbb_wdt_set_pretimeout(struct watchdog_device *wdt_dev,
					 unsigned int pretimeout)
{
	struct meson_gxbb_wdt *data = watchdog_get_drvdata(wdt_dev);
	unsigned long tcnt = pretimeout * 1000;

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	if (wdt_debug)
		pr_info("%s() pretimeout=%u\n", __func__, pretimeout);
#endif
	if (tcnt > GXBB_WDT_TCNT_SETUP_MASK)
		tcnt = GXBB_WDT_TCNT_SETUP_MASK;

	wdt_dev->pretimeout = pretimeout;

	writel(tcnt, data->reg_offset);

	return 0;
}

static irqreturn_t meson_gxbb_wdt_interrupt(int irq, void *p)
{
	struct meson_gxbb_wdt *data = (struct meson_gxbb_wdt *)p;

	/*
	 * If it's not 0, dual mode window will be enabled, the offset value
	 * is the window time before watchdog reset when interrupt comes.
	 *
	 * Two cases:
	 *   1. offset is disabled, the interrupt is normal WDT reset interrupt
	 *   2. offset is enabled, the interrupt is the window interrupt to analyse
	 */
	if (readl(data->reg_offset) & GXBB_WDT_TCNT_SETUP_MASK) {
		pr_warn("Watchdog has not been fed for %d seconds, and the system will be reset by it in %d seconds\n",
			data->wdt_dev.timeout - data->wdt_dev.pretimeout,
			data->wdt_dev.pretimeout);
	}

	return IRQ_HANDLED;
}
#endif

static const struct watchdog_ops meson_gxbb_wdt_ops = {
	.start = meson_gxbb_wdt_start,
	.stop = meson_gxbb_wdt_stop,
	.ping = meson_gxbb_wdt_ping,
	.set_timeout = meson_gxbb_wdt_set_timeout,
	.get_timeleft = meson_gxbb_wdt_get_timeleft,
#ifdef CONFIG_AMLOGIC_MODIFY
	.set_pretimeout = meson_gxbb_wdt_set_pretimeout,
#endif
};

#ifdef CONFIG_AMLOGIC_MODIFY
static struct watchdog_info meson_gxbb_wdt_info = {
#else
static const struct watchdog_info meson_gxbb_wdt_info = {
#endif
	.identity = "Meson GXBB Watchdog",
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
};

static int __maybe_unused meson_gxbb_wdt_resume(struct device *dev)
{
	struct meson_gxbb_wdt *data = dev_get_drvdata(dev);

#ifdef CONFIG_AMLOGIC_MODIFY
	if ((watchdog_active(&data->wdt_dev) ||
	    watchdog_hw_running(&data->wdt_dev)) &&
	    watchdog_enabled)
		meson_gxbb_wdt_start(&data->wdt_dev);
#else
	if (watchdog_active(&data->wdt_dev))
		meson_gxbb_wdt_start(&data->wdt_dev);
#endif

	return 0;
}

static int __maybe_unused meson_gxbb_wdt_suspend(struct device *dev)
{
	struct meson_gxbb_wdt *data = dev_get_drvdata(dev);

#ifdef CONFIG_AMLOGIC_MODIFY
	if (watchdog_active(&data->wdt_dev) ||
	    watchdog_hw_running(&data->wdt_dev))
		meson_gxbb_wdt_stop(&data->wdt_dev);
#else
	if (watchdog_active(&data->wdt_dev))
		meson_gxbb_wdt_stop(&data->wdt_dev);
#endif

	return 0;
}

#if defined(CONFIG_AMLOGIC_MODIFY)
static int meson_gxbb_wdt_restore(struct device *dev)
{
	struct meson_gxbb_wdt *data = dev_get_drvdata(dev);
	int index;
	int ret;

	ret = clk_prepare_enable(data->clk);
	if (ret)
		return ret;

	for (index = 0; index < data->shadow_size; index += 4)
		writel(data->shadow[index >> 2], data->reg_base + index);

	dev_dbg(dev, "restore register completed\n");

	ret = meson_gxbb_wdt_resume(dev);
	if (ret)
		return ret;

	return 0;
}

static int meson_gxbb_wdt_freeze(struct device *dev)
{
	struct meson_gxbb_wdt *data = dev_get_drvdata(dev);
	int index;
	int ret;

	ret = meson_gxbb_wdt_suspend(dev);
	if (ret)
		return ret;

	for (index = 0; index < data->shadow_size; index += 4)
		data->shadow[index >> 2] = readl(data->reg_base + index);

	dev_dbg(dev, "save register completed\n");

	clk_disable_unprepare(data->clk);

	return 0;
}
#endif /* CONFIG_AMLOGIC_MODIFY */

static const struct dev_pm_ops meson_gxbb_wdt_pm_ops = {
#if defined(CONFIG_AMLOGIC_MODIFY)
	.suspend = meson_gxbb_wdt_suspend,
	.resume = meson_gxbb_wdt_resume,
	.freeze = meson_gxbb_wdt_freeze,
	.thaw = meson_gxbb_wdt_resume,
	.poweroff = meson_gxbb_wdt_suspend,
	.restore = meson_gxbb_wdt_restore,
#else
	SET_SYSTEM_SLEEP_PM_OPS(meson_gxbb_wdt_suspend, meson_gxbb_wdt_resume)
#endif /* CONFIG_AMLOGIC_MODIFY */
};

#ifdef CONFIG_AMLOGIC_MODIFY
struct wdt_params {
	u8 rst_shift;
};

static const struct wdt_params sc2_params __initconst = {
	.rst_shift = 22,
};

static const struct wdt_params gxbb_params __initconst = {
	.rst_shift = 21,
};
#endif

static const struct of_device_id meson_gxbb_wdt_dt_ids[] = {
#ifndef CONFIG_AMLOGIC_MODIFY
	 { .compatible = "amlogic,meson-gxbb-wdt", },
#else
	 { .compatible = "amlogic,meson-gxbb-wdt", .data = &gxbb_params},
	 { .compatible = "amlogic,meson-sc2-wdt", .data = &sc2_params},
	 { .compatible = "amlogic,meson-a4-wdt", .data = &sc2_params},
#endif
	 { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, meson_gxbb_wdt_dt_ids);

static void meson_clk_disable_unprepare(void *data)
{
	clk_disable_unprepare(data);
}

#ifdef CONFIG_AMLOGIC_MODIFY
static void meson_gxbb_wdt_shutdown(struct platform_device *pdev)
{
	struct meson_gxbb_wdt *data = platform_get_drvdata(pdev);

	if (watchdog_active(&data->wdt_dev) ||
	    watchdog_hw_running(&data->wdt_dev))
		meson_gxbb_wdt_stop(&data->wdt_dev);
};

#if IS_ENABLED(CONFIG_DEBUG_FS)
static ssize_t debugfs_read(struct file *file, char __user *buffer,
			    size_t size, loff_t *pos)
{
	int ret;
	struct meson_gxbb_wdt *data = file->private_data;
	char *enabled = "false\n";

	if (readl(data->reg_base + GXBB_WDT_CTRL_REG) & GXBB_WDT_CTRL_EN)
		enabled = "true\n";

	ret = simple_read_from_buffer(buffer, size, pos, enabled, strlen(enabled));

	return ret;
}

static ssize_t debugfs_write(struct file *file, const char __user *buffer,
			     size_t size, loff_t *pos)
{
	struct meson_gxbb_wdt *data = file->private_data;
	struct watchdog_device *wdt_dev = &data->wdt_dev;
	char buff[5] = {0};
	ssize_t ret;

	ret = simple_write_to_buffer(buff, 5, pos, buffer, size);
	if (ret < 0)
		return ret;

	if (!memcmp(buff, "false", 5)) {
		meson_gxbb_wdt_stop(&data->wdt_dev);
		meson_gxbb_wdt_ping(&data->wdt_dev);
		watchdog_enabled = 0;
		dev_info(wdt_dev->parent, "watchdog stop\n");
	} else if (!memcmp(buff, "true", 4)) {
		meson_gxbb_wdt_ping(&data->wdt_dev);
		meson_gxbb_wdt_start(&data->wdt_dev);
		watchdog_enabled = 1;
		dev_info(wdt_dev->parent, "watchdog start\n");
	}

	return size;
}

static const struct file_operations debugfs_ops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = debugfs_read,
	.write = debugfs_write,
	.llseek = default_llseek,
};
#endif

static int meson_gxbb_wdt_notifier(struct notifier_block *self,
				   unsigned long v, void *p)
{
	struct meson_gxbb_wdt *data = container_of(self, struct meson_gxbb_wdt,
						   notifier);

	if (stop_after_panic) {
		meson_gxbb_wdt_stop(&data->wdt_dev);
		pr_info("watchdog has stopped after panic\n");
	}

	return NOTIFY_DONE;
}
#endif

static int meson_gxbb_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct meson_gxbb_wdt *data;
	int ret;
#ifdef CONFIG_AMLOGIC_MODIFY
	struct wdt_params *wdt_params;
	int reset_by_soc;
	struct watchdog_device *wdt_dev;
	int irq;
	struct resource *res;
	resource_size_t res_size;
#endif

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

#ifdef CONFIG_AMLOGIC_MODIFY
	data->reg_base = devm_platform_get_and_ioremap_resource(pdev, 0, &res);
#else
	data->reg_base = devm_platform_ioremap_resource(pdev, 0);
#endif
	if (IS_ERR(data->reg_base))
		return PTR_ERR(data->reg_base);

	data->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(data->clk))
		return PTR_ERR(data->clk);

	ret = clk_prepare_enable(data->clk);
	if (ret)
		return ret;
	ret = devm_add_action_or_reset(dev, meson_clk_disable_unprepare,
				       data->clk);
	if (ret)
		return ret;

#ifdef CONFIG_AMLOGIC_MODIFY
	/*
	 * There are two conditions that need to be met to enable a pre-timeout
	 *   1. 'interrupts' properties exist in the watchdog node
	 *      of the device tree
	 *   2. 'offset' register exists in the watchdog node of the device tree
	 *
	 * NOTE: The A4 and all subsequent chips support this feature
	 */
	irq = platform_get_irq(pdev, 0);
	if (irq > 0) {
		ret = devm_request_irq(&pdev->dev, irq, meson_gxbb_wdt_interrupt,
				       IRQF_SHARED, DRIVER_NAME, data);
		if (ret < 0) {
			dev_err(&pdev->dev, "irq request failed (%d)\n", ret);
			return ret;
		}

		data->reg_offset = devm_platform_ioremap_resource(pdev, 1);
		if (IS_ERR(data->reg_offset))
			return PTR_ERR(data->reg_offset);

		data->wdt_dev.pretimeout = DEFAULT_PRETIMEOUT;
		meson_gxbb_wdt_info.options |= WDIOF_PRETIMEOUT;

		dev_info(&pdev->dev, "pretimeout is enabled\n");
	}
#endif

	platform_set_drvdata(pdev, data);

	data->wdt_dev.parent = dev;
	data->wdt_dev.info = &meson_gxbb_wdt_info;
	data->wdt_dev.ops = &meson_gxbb_wdt_ops;
	data->wdt_dev.max_hw_heartbeat_ms = GXBB_WDT_TCNT_SETUP_MASK;
	data->wdt_dev.min_timeout = 1;
	data->wdt_dev.timeout = DEFAULT_TIMEOUT;
	watchdog_set_drvdata(&data->wdt_dev, data);

#ifndef CONFIG_AMLOGIC_MODIFY
	/* Setup with 1ms timebase */
	writel(((clk_get_rate(data->clk) / 1000) & GXBB_WDT_CTRL_DIV_MASK) |
		GXBB_WDT_CTRL_EE_RESET |
		GXBB_WDT_CTRL_CLK_EN |
		GXBB_WDT_CTRL_CLKDIV_EN,
		data->reg_base + GXBB_WDT_CTRL_REG);
#else

	res_size = resource_size(res);
	dev_dbg(&pdev->dev, "resource size: %d\n", (int)res_size);
	if (res_size & 0x3)
		dev_warn(&pdev->dev, "resource size is not a multiple of 4byte\n");

	data->shadow = devm_kmalloc(&pdev->dev, res_size, GFP_KERNEL);
	if (!data->shadow)
		return -ENOMEM;
	data->shadow_size = (int)res_size;

	wdt_params = (struct wdt_params *)of_device_get_match_data(dev);

	reset_by_soc = !(readl(data->reg_base + GXBB_WDT_CTRL1_REG) &
			 GXBB_WDT_RST_SIG_EN);

	/* Setup with 1ms timebase */
	writel(((clk_get_rate(data->clk) / 1000) & GXBB_WDT_CTRL_DIV_MASK) |
		(reset_by_soc << wdt_params->rst_shift) |
		GXBB_WDT_CTRL_CLK_EN |
		GXBB_WDT_CTRL_CLKDIV_EN,
		data->reg_base + GXBB_WDT_CTRL_REG);
#endif
	meson_gxbb_wdt_set_timeout(&data->wdt_dev, data->wdt_dev.timeout);

#ifdef CONFIG_AMLOGIC_MODIFY
	wdt_dev = &data->wdt_dev;

	if (meson_gxbb_wdt_info.options & WDIOF_PRETIMEOUT) {
		/* Configuration pretimeout */
		meson_gxbb_wdt_set_pretimeout(wdt_dev, wdt_dev->pretimeout);
		/* Watchdog interrupt enable */
		writel(readl(data->reg_base + GXBB_WDT_CTRL_REG) |
		       GXBB_WDT_CTRL_IRQ_EN, data->reg_base + GXBB_WDT_CTRL_REG);
	}

	ret = of_property_read_u32(pdev->dev.of_node,
				   "amlogic,feed_watchdog_mode",
				   &data->feed_watchdog_mode);
	if (ret)
		data->feed_watchdog_mode = 1;
	if (data->feed_watchdog_mode == 1) {
		set_bit(WDOG_HW_RUNNING, &wdt_dev->status);
		/*
		 * For the convenience of debugging, you can disable
		 * the watchdog in the boot parameters
		 */
		if (watchdog_enabled)
			meson_gxbb_wdt_start(&data->wdt_dev);
		else
			dev_info(&pdev->dev, "disabled watchdog in boot parameters\n");
	}

	dev_info(&pdev->dev, "feeding watchdog mode: [%s]\n",
		 data->feed_watchdog_mode ? "kernel" : "userspace");

	watchdog_stop_on_reboot(wdt_dev);
	ret = devm_watchdog_register_device(dev, wdt_dev);
	if (ret)
		return ret;
	/* 1. must set after watchdog  cdev register to prevent kernel
	 * & userspace use wdt at the same time
	 * 2. watchdog_cdev_register will check WDOG_HW_RUNNING to start hrtimer
	 * so, WDOG_HW_RUNNING should be set first on above
	 */
	if (data->feed_watchdog_mode == 1)
		set_bit(_WDOG_DEV_OPEN, &wdt_dev->wd_data->status);

#if IS_ENABLED(CONFIG_DEBUG_FS)
	/* Provided for debugging, can dynamically disable the watchdog */
	data->debugfs_dir = debugfs_create_dir("watchdog", NULL);
	debugfs_create_file("enabled", 0644,
			    data->debugfs_dir, data, &debugfs_ops);
#endif

	/*
	 * When debugging, there will be a lot of printing to be output after
	 * the panic, so it is very time-consuming. In order to prevent
	 * the watchdog from causing a reset, we stop it after receiving
	 * the notification.
	 */
	data->notifier.notifier_call = meson_gxbb_wdt_notifier;
	data->notifier.priority = 200; /* ftrace_dump_on_oops panic notifier priority is 150 */
	atomic_notifier_chain_register(&panic_notifier_list,
				       &data->notifier);

	return ret;
#else
	watchdog_stop_on_reboot(&data->wdt_dev);
	return devm_watchdog_register_device(dev, &data->wdt_dev);
#endif
}

static struct platform_driver meson_gxbb_wdt_driver = {
	.probe	= meson_gxbb_wdt_probe,
	.driver = {
		.name = "meson-gxbb-wdt",
		.pm = &meson_gxbb_wdt_pm_ops,
		.of_match_table	= meson_gxbb_wdt_dt_ids,
	},
#ifdef CONFIG_AMLOGIC_MODIFY
	.shutdown = meson_gxbb_wdt_shutdown,
#endif
};

module_platform_driver(meson_gxbb_wdt_driver);

MODULE_AUTHOR("Neil Armstrong <narmstrong@baylibre.com>");
MODULE_DESCRIPTION("Amlogic Meson GXBB Watchdog timer driver");
MODULE_LICENSE("Dual BSD/GPL");
