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

#include "libocean_p.h"

struct usb4000_status {
	uint16_t num_of_pixels;
	uint32_t integration_time;
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

static inline double correct_intensity(struct ocean_spectra *spec, double intensity)
{
	double value = 0.0;
	int order;

	for (order = spec->poly_order_non_lin; order > 0; order--)
		value = intensity * (spec->non_lin_coef[order] + value);

	return intensity / (spec->non_lin_coef[0] + value);
}

static int convert_raw_to_intensity(struct ocean_spectra *spec)
{
	const double saturation = (65535.0f / spec->saturation);
	uint8_t *raw = &spec->raw[36]; /* pixel 1-19 are not usable */
	double *data = spec->data;

	/* the actualy reported number of pixels is 3840, but usable
	 * are only 3648, the rest can be ignored */
	spec->data_size = 3648;

	/* convert the raw data into actual values */
	while ((data - spec->data) < spec->data_size) {
		const uint8_t lsb = *raw++;
		const uint8_t msb = *raw++;
		const uint16_t val = ((msb << 8) & 0xFF00) | lsb;
		/* apply the non-linearity correction coefficents */
		*data++ = correct_intensity(spec, val * saturation);
	}

	return 0;
}

static int usb4000_set_integration_time(struct ocean *self, uint32_t time)
{
	struct usb4000_status *status = (struct usb4000_status *)self->status;
	uint8_t cmd[] = { 0x02, 0x00, 0x00, 0x00, 0x00 };
	int ret;

	/* Depending on device, the integration time is given
	 * in µs or ms. So we need to apply a correction factor */
	time *= 1000;

	/* FIXME: convert this properly depending on cpu */
	/* TODO: Check if libusb_le16_to_cpu or libusb_cpu_to_le16
	 *       can be used here */
	cmd[1] = (time >>  0) & 0xFF;
	cmd[2] = (time >>  8) & 0xFF;
	cmd[3] = (time >> 16) & 0xFF;
	cmd[4] = (time >> 24) & 0xFF;

	ret = ocean_send_command(self, cmd, ARRAY_SIZE(cmd));
	if (ret < 0)
		return -EIO;

	status->integration_time = time;
	return 0;
}

static int usb4000_get_integration_time(struct ocean *self, uint32_t *time)
{
	struct usb4000_status *status = (struct usb4000_status *)self->status;
	int ret;

	ret = ocean_query_status(self);
	if (ret < 0)
		return ret;

	*time = (uint32_t)status->integration_time;
	return 0;
}

static int usb4000_get_number_of_pixel(struct ocean *self, uint32_t *pixel)
{
	struct usb4000_status *status = (struct usb4000_status *)self->status;
	int ret;

	ret = ocean_query_status(self);
	if (ret < 0)
		return ret;

	*pixel = (uint32_t)status->num_of_pixels;
	return 0;
}

static int usb4000_recv_spectra(struct ocean *self, struct ocean_spectra *spec)
{
	/* the timeout is set in ms, but the integration time is given in µm */
	struct usb4000_status *status = (struct usb4000_status *)self->status;
	unsigned timeout = self->timeout + status->integration_time / 1000;
	int first_block = 4*512;
	int second_block = (int)spec->raw_size - first_block + 1 /*1 sync packet*/;
	int done = 0;
	int overall;
	int ret;

	ret = libusb_bulk_transfer(self->dev, self->ep[EP_DATA_RECV],
				   spec->raw, first_block, &done,
				   timeout);
	if (ret < 0) {
		fprintf(stderr, "ERR: libusb_bulk_transfer read failed: %d "
			"(done %d/%zu)\n", ret, done, spec->raw_size);
		return ret;
	}
	overall = done;

	ret = libusb_bulk_transfer(self->dev, self->ep[EP_DATA_RECV2],
				   &spec->raw[overall], second_block,
				   &done, timeout);
	if (ret < 0) {
		fprintf(stderr, "ERR: libusb_bulk_transfer read failed: %d "
			"(done %d/%zu)\n", ret, done, spec->raw_size);
		return ret;
	}
	overall += done;

	return convert_raw_to_intensity(spec);
}

api_private
int usb4000_initialize(struct ocean *self, uint16_t vendor, uint16_t device)
{
	if (vendor != 0x2457)
		return -ENODEV;
	if (device != 0x1022)
		return -ENODEV;

	self->ep[0] = (0x01 | LIBUSB_ENDPOINT_OUT);
	self->ep[1] = (0x81 | LIBUSB_ENDPOINT_IN);
	self->ep[2] = (0x86 | LIBUSB_ENDPOINT_IN);
	self->ep[3] = (0x82 | LIBUSB_ENDPOINT_IN);

	self->status = malloc(sizeof(struct usb4000_status));
	self->receive = usb4000_recv_spectra;
	self->get_int_time = usb4000_get_integration_time;
	self->set_int_time = usb4000_set_integration_time;
	self->get_num_pixel = usb4000_get_number_of_pixel;

	return 0;
}
