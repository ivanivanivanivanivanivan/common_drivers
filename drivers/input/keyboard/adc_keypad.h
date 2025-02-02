/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __LINUX_ADC_KEYPAD_H
#define __LINUX_ADC_KEYPAD_H
#include <linux/list.h>
#include <linux/input.h>
#include <linux/kobject.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/iio/consumer.h>
#include <dt-bindings/iio/adc/amlogic-saradc.h>
#include <linux/leds.h>

#define DRIVE_NAME "adc_keypad"
#define MAX_NAME_LEN 20

enum TOLERANCE_RANGE {
	TOL_MIN = 0,
	TOL_MAX = 255
};

enum SAMPLE_VALUE_RANGE {
	SAM_MIN = 0,
	SAM_MAX = 4095 /*12bit adc*/
};

struct adc_key {
	char name[MAX_NAME_LEN];
	unsigned int chan;
	unsigned int code;  /* input key code */
	int value; /* voltage/3.3v * 1023 */
	int tolerance;
	struct list_head list;
};

struct meson_adc_kp {
	unsigned char chan[8];
	unsigned char chan_num;   /*number of channel exclude duplicate*/
	unsigned char count;
	unsigned int report_code;
	unsigned int prev_code;
	unsigned int poll_period; /*key scan period*/
	struct mutex kp_lock;
	struct class kp_class;
	struct list_head adckey_head;
	struct delayed_work work;
	struct input_dev *input;
	struct iio_channel *pchan[SARADC_CH_NUM];
	struct led_trigger *led_blink;
	unsigned long led_delay_on;
	unsigned long led_delay_off;
	const char *led_trigger_name;
};

#endif
