/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* F75303 temperature sensor module for Chrome EC */

#ifndef __CROS_EC_F75303_H
#define __CROS_EC_F75303_H

#define F75303_I2C_ADDR		0x9A /* 7-bit address is 0x4C */

enum f75303_index {
	F75303_IDX_LOCAL = 0,
	F75303_IDX_REMOTE,
};

/* F75303 register */
#define F75303_TEMP_LOCAL		0x00
#define F75303_TEMP_REMOTE		0x01

/**
 * Get the last polled value of a sensor.
 *
 * @param idx		Index to read. Idx indicates whether to read die
 *			temperature or external temperature.
 * @param temp_ptr	Destination for temperature in K.
 *
 * @return EC_SUCCESS if successful, non-zero if error.
 */
int f75303_get_val(int idx, int *temp_ptr);

#endif  /* __CROS_EC_F75303_H */
