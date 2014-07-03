/*
 * Copyright (C) 2014 Soeren Grunewald <soeren.grunewald@desy.de>
 * Copyright (C) 2014 DESY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-*
 */

#include "config.h"
#include <libocean.h>

#include <libusb.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifndef LIBOCEAN_PRIV_H
#define LIBOCEAN_PRIV_H 1

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof (x[0]))
#endif

#if !defined(WIN32)
#  if __GNUC__ >= 4
#    define api_public __attribute__((visibility("default")))
#    define api_private __attribute__((visibility("hidden")))
#  else
#    define api_public
#    define api_private
#  endif
#else
#  define api_public
#  define api_private
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {
	EP_CMD_SEND = 0,
	EP_CMD_RECV,
	EP_DATA_RECV,
	EP_DATA_RECV2
};

enum {
	OCEAN_DEVICE_SERIAL = 0,
	OCEAN_WAVELEN_CAL_COEF_0,
	OCEAN_WAVELEN_CAL_COEF_1,
	OCEAN_WAVELEN_CAL_COEF_2,
	OCEAN_WAVELEN_CAL_COEF_3,
	OCEAN_STRAY_LIGHT_CONST,
	OCEAN_NON_LIN_COR_COEF_0,
	OCEAN_NON_LIN_COR_COEF_1,
	OCEAN_NON_LIN_COR_COEF_2,
	OCEAN_NON_LIN_COR_COEF_3,
	OCEAN_NON_LIN_COR_COEF_4,
	OCEAN_NON_LIN_COR_COEF_5,
	OCEAN_NON_LIN_COR_COEF_6,
	OCEAN_NON_LIN_COR_COEF_7,
	OCEAN_POLY_ORDER_NON_LIN_COR,
	OCEAN_OPTICAL_BENCH_CFG,
	OCEAN_DETECT_SERIAL,
	OCEAN_CONFIG_PARAM_RETURN,
	OCEAN_LAST
};

struct ocean_spectra {
	uint8_t *raw;
	double *data;
	size_t raw_size;
	size_t data_size;
	/* spectrometer wl_cal_coef values */
	double wl_cal_coef[4];
	double non_lin_coef[8];
	int poly_order_non_lin;
	uint16_t saturation;
};

typedef int (*Initialize)(struct ocean *, uint16_t, uint16_t);
typedef int (*ReceiveData)(struct ocean *, struct ocean_spectra *);
typedef int (*GetIntegrationTime)(struct ocean *, uint32_t *);
typedef int (*SetIntegrationTime)(struct ocean *, uint32_t);
typedef int (*GetNumberOfPixels)(struct ocean *, uint32_t *);

struct ocean_status {
	size_t status_size;
};

struct ocean {
	libusb_context *usb;
	libusb_device_handle *dev;
	struct ocean_status *status;

	uint8_t ep[4];
	int timeout;

	Initialize init;
	/* The function used to receive requested specrtra data */
	ReceiveData receive;
	GetIntegrationTime get_int_time;
	SetIntegrationTime set_int_time;
	GetNumberOfPixels get_num_pixel;
};

int ocean_send_command(struct ocean *, uint8_t *, size_t);
int ocean_query_status(struct ocean *);

#ifdef __cplusplus
};
#endif

#endif /* LIBOCEAN_PRIV_H */
