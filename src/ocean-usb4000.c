#include "libocean_p.h"

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

api_private
int usb4000_recv_spectra(struct ocean *self, struct ocean_spectra *spec)
{
	/* the timeout is set in ms, but the integration time is given in µm */
	unsigned timeout = self->timeout + self->status.integration_time / 1000;
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
