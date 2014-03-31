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

api_private
int nirquest512_recv_spectra(struct ocean *self, struct ocean_spectra *spec)
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
