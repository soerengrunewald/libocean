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

#define EP_CMD_SEND 0
#define EP_CMD_RECV 1
#define EP_DATA_RECV 2
#define EP_DATA_RECV2 3

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

typedef int (*ReceiveFuncPtr)(struct ocean *, struct ocean_spectra *);

struct ocean {
	libusb_context *usb;
	libusb_device_handle *dev;
	uint8_t ep[4];
	int timeout;
	/* The function used to receive requested specrtra data */
	ReceiveFuncPtr receive;
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

struct ocean_status {
	uint16_t num_of_pixels;
	uint16_t integration_time;
	uint8_t lamp_enable;
	uint8_t trigger_mode;
	uint8_t request_spectrum;
	uint8_t reserved1;
	uint8_t specral_data_ready;
	uint8_t reserved2;
	uint8_t power_state;
	uint8_t spectral_data_counter;
	uint8_t detector_gain_select;
	uint8_t fan_and_tec_state;
	uint16_t reserved3;
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

#ifdef __cplusplus
};
#endif

#endif /* LIBOCEAN_PRIV_H */
