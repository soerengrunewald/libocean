#include "libocean_p.h"

static inline unsigned flip(unsigned x, unsigned bit)
{
	return x ^ (1 << bit);
}

static inline double ocean_spectra_correct_intensity(struct ocean_spectra *spec, double intensity)
{
	double value = 0.0;
	int order;

	for (order = spec->poly_order_non_lin; order > 0; order--)
		value = intensity * (spec->non_lin_coef[order] + value);

	return intensity / (spec->non_lin_coef[0] + value);
}

api_private
void ocean_spectra_apply_coefficents(struct ocean_spectra *spec)
{
	const double saturation = (65535.0f / spec->saturation);
	size_t i = 0, j = 0;

	while ((j < spec->data_size) && (i+1 < spec->raw_size)) {
		const uint16_t val = flip((spec->raw[i+1] << 8) | spec->raw[i], 15);
		spec->data[j++] = ocean_spectra_correct_intensity(spec, val * saturation);
		i+=2;
		/* every 15th packets (each package has 512bytes),
		 * we have a sync byte, skip it */
		if ((j % 512) == 0) {
			printf("Skipping byte %zu/%zu = 0x%x\n",
				i, spec->raw_size, spec->raw[i]);
			i++;
		}
	}
}

api_private
int ocean_recv_spectra(struct ocean *self, struct ocean_spectra *spec)
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

	return 0;
}
