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

struct nirquest_status {
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

static inline unsigned flip(unsigned x, unsigned bit)
{
	return x ^ (1 << bit);
}

static inline double correct_intensity(struct ocean_spectra *spec, double intensity)
{
	double value = 0.0;
	int order;

	for (order = spec->poly_order_non_lin; order > 0; order--)
		value = intensity * (spec->non_lin_coef[order] + value);

	return intensity / (spec->non_lin_coef[0] + value);
}

static int apply_spectral_coefficents(struct ocean_spectra *spec)
{
	const double saturation = (65535.0f / spec->saturation);
	size_t i = 0, j = 0;

	while ((j < spec->data_size) && (i+1 < spec->raw_size)) {
		const uint16_t val = flip((spec->raw[i+1] << 8) | spec->raw[i], 15);
		spec->data[j++] = correct_intensity(spec, val * saturation);
		i+=2;
		/* every 15th packets (each package has 512bytes),
		 * we have a sync byte, skip it */
		if ((j % 512) == 0) {
			printf("Skipping byte %zu/%zu = 0x%x\n",
				i, spec->raw_size, spec->raw[i]);
			i++;
		}
	}

	return 0;
}

static int nirquest512_recv_spectra(struct ocean *self, struct ocean_spectra *spec)
{
	int done = 0;
	int ret;

	ret = libusb_bulk_transfer(self->dev, self->ep[EP_DATA_RECV],
				   spec->raw, spec->raw_size, &done,
				   self->timeout);
	if (ret < 0) {
		printf("ERR: libusb_bulk_transfer read failed: %d (done %d/%zu)\n",
			ret, done, spec->raw_size);
		return ret;
	}

	return apply_spectral_coefficents(spec);
}

static int nirquest512_get_integration_time(struct ocean *self, uint32_t *time)
{
	struct nirquest_status *status = (struct nirquest_status *)self->status;
	int ret;

	ret = ocean_query_status(self);
	if (ret < 0)
		return ret;

	*time = (uint32_t)status->integration_time;
	return 0;
}

static int nirquest512_set_integration_time(struct ocean *self, uint32_t time)
{
	struct nirquest_status *status = (struct nirquest_status *)self->status;
	uint8_t cmd[] = { 0x02, 0x00, 0x00, 0x00, 0x00 };
	int ret;

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

static int nirquest512_get_number_of_pixel(struct ocean *self, uint32_t *pixel)
{
	struct nirquest_status *status = (struct nirquest_status *)self->status;
	int ret;

	ret = ocean_query_status(self);
	if (ret < 0)
		return ret;

	*pixel = (uint32_t)status->num_of_pixels;
	return 0;
}

api_private
int nirquest512_initialize(struct ocean *self, uint16_t vendor, uint16_t device)
{
	if (vendor != 0x2457)
		return -ENODEV;
	if (device != 0x1026 && device != 0x1028)
		return -ENODEV;

	self->ep[0] = (0x01 | LIBUSB_ENDPOINT_OUT);
	self->ep[1] = (0x81 | LIBUSB_ENDPOINT_IN);
	self->ep[2] = (0x82 | LIBUSB_ENDPOINT_IN);
	self->ep[3] = 0;

	self->status = malloc(sizeof(struct nirquest_status));
	self->receive = nirquest512_recv_spectra;
	self->get_int_time = nirquest512_get_integration_time;
	self->set_int_time = nirquest512_set_integration_time;
	self->get_num_pixel = nirquest512_get_number_of_pixel;

	return 0;
}
