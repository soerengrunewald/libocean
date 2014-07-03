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

#define MIN_RET_BUF_LEN 16

static void hexdump(uint8_t *buf, size_t len, const char *prefix)
{
	const size_t rowsize = 16;
	const uint8_t *ptr = buf;
	FILE *fp = stdout;
	size_t i, j;

	for (j = 0; j < len; j += rowsize) {
		if (prefix)
			fprintf(fp, "%s | ", prefix);

		for (i = 0; i < rowsize; i++) {
			if ((j + i) < len)
				fprintf(fp, "%s%02x", i ? " " : "", ptr[j + i]);
			else
				fprintf(fp, "%s  ", i ? " " : "");
		}

		fprintf(fp, " | ");

		for (i = 0; i < rowsize; i++) {
			if ((j + i) < len) {
				if (isprint(ptr[j + i]))
					fprintf(fp, "%c", ptr[j + i]);
				else
					fprintf(fp, ".");
			} else {
				fprintf(fp, " ");
			}
		}

		fprintf(fp, " |\n");
	}
}

static int ocean_query_dev_info(struct ocean *self, uint8_t what, uint8_t *buf, size_t len);

static int ocean_dump_all(struct ocean *self)
{
	char prefix[32];
	uint8_t buf[32];
	uint8_t i;
	int ret;

	for (i = OCEAN_DEVICE_SERIAL; i < OCEAN_LAST; i++) {
		memset(buf, 0, ARRAY_SIZE(buf));
		ret = ocean_query_dev_info(self, i, buf, ARRAY_SIZE(buf));
		if (ret < 0) {
			fprintf(stderr, "ERR:%s: failed to query: 0x%x -> %d\n",
				__func__, i, ret);
			continue;
		}
		snprintf(prefix, ARRAY_SIZE(prefix), "0pt %.2d", i);
		hexdump(buf, 16, prefix);
	}

	return 0;
}

static void ocean_spectra_dump_coefficents(struct ocean_spectra *spec)
{
	int order;

	for (order = 0; order < ARRAY_SIZE(spec->wl_cal_coef); order++) {
		fprintf(stdout, "DBG: wavelength calibration "
			"coefficent #%d: %E\n", order, spec->wl_cal_coef[order]);
	}

	fprintf(stdout, "DBG: polynomical order of non-linearity "
		"calibratrion: %d\n", spec->poly_order_non_lin);

	for (order = 0; order < ARRAY_SIZE(spec->non_lin_coef); order++) {
		fprintf(stdout, "DBG: non-linearity calibration "
			"coefficent #%d: %E\n", order, spec->non_lin_coef[order]);
	}

	fprintf(stdout, "DBG: saturation level %d\n", spec->saturation);
}

static int ocean_spectra_query_coefficents(struct ocean_spectra *spec, struct ocean *ocean)
{
	uint8_t buf[18];
	int order;
	int ret;

	/* query the wavelength calibration coefficents */
	for (order = 0; order < ARRAY_SIZE(spec->wl_cal_coef); order++) {
		memset(buf, 0, ARRAY_SIZE(buf));
		ret = ocean_query_dev_info(ocean, OCEAN_WAVELEN_CAL_COEF_0 +
					   order, buf, ARRAY_SIZE(buf));
		if (ret < 0) {
			fprintf(stderr, "ERR: Unable to query the wavelength "
				"calibration coefficent #%d\n", order);
			continue;
		}
		/* the values are stored as asci strings */
		spec->wl_cal_coef[order] = strtod((const char *)&buf[2], NULL);
	}

	/* query the polynomical order of non-linearity calibratrion */
	memset(buf, 0, ARRAY_SIZE(buf));
	ret = ocean_query_dev_info(ocean, OCEAN_POLY_ORDER_NON_LIN_COR, buf,
				   ARRAY_SIZE(buf));
	if (ret < 0) {
		fprintf(stderr, "ERR: Unable to query polynomical order of "
			"non-linearity calibratrion\n");
		return ret;
	}
	/* the value is stored as asci strings */
	spec->poly_order_non_lin = atoi((const char *)&buf[2]);

	/* query the non-linerarity correction coefficents */
	for (order = 0; order < ARRAY_SIZE(spec->non_lin_coef); order++) {
		memset(buf, 0, ARRAY_SIZE(buf));
		ret = ocean_query_dev_info(ocean, OCEAN_NON_LIN_COR_COEF_0 +
					   order, buf, ARRAY_SIZE(buf));
		if (ret < 0) {
			fprintf(stderr, "ERR :Unable to query the wavelength "
				"calibration coefficent #%d\n", order);
			continue;
		}
		/* the values are stored as asci strings */
		spec->non_lin_coef[order] = strtod((const char *)&buf[2], NULL);
	}

	/* query the saturation level */
	memset(buf, 0, ARRAY_SIZE(buf));
	ret = ocean_query_dev_info(ocean, OCEAN_CONFIG_PARAM_RETURN, buf, ARRAY_SIZE(buf));
	if (ret < 0)
		fprintf(stderr, "ERR: ifailed to query saturation level, "
			"using default\n");
	else {
		const uint16_t val = (buf[7] << 8) | buf[6];
		spec->saturation = val;
//		fprintf(stdout, "Saturation: %d\nRaw: ", val);
//		hexdump(buf, ARRAY_SIZE(buf), NULL);
	}

	ocean_spectra_dump_coefficents(spec);
	return 0;
}

static int ocean_spectra_create_custom(struct ocean_spectra **spec, size_t size)
{
	struct ocean_spectra *s;

	if (!spec)
		return -EINVAL;

	s = malloc(sizeof(*s));
	if (!s)
		return -ENOMEM;
	memset(s, 0, sizeof(*s));

	s->raw = malloc(s->raw_size = size);
	if (!s->raw) {
		free(s);
		return -ENOMEM;
	}

	/* FIXME: We remove on value because, we need 2 extra bytes
	 *        to store the sync byte whitin the raw data */
	s->data_size = (size / 2) - 1;
	s->data = malloc(s->data_size * sizeof(double));
	if (!s->data) {
		free(s->data);
		free(s);
		return -ENOMEM;
	}

	*spec = s;
	return 0;
}

api_public
int ocean_spectra_create(struct ocean_spectra **spec, struct ocean *ocean)
{
	uint32_t pixels = 0;
	int ret;

	if (!ocean)
		return -EINVAL;

	ret = ocean->get_num_pixel(ocean, &pixels);
	if (ret < 0)
		return -EIO;

	/* FIXME: The length needs to be larger because of the sync byte */
	ret = ocean_spectra_create_custom(spec, (pixels * 2) + 2);
	if (ret < 0)
		return ret;

	ret = ocean_spectra_query_coefficents(*spec, ocean);
	if (ret < 0)
		return ret;

	return 0;
}

api_public
void ocean_spectra_free(struct ocean_spectra *spec)
{
	if (!spec)
		return;

	if (spec->raw) {
		free(spec->raw);
		spec->raw = NULL;
		spec->raw_size = 0;
	}

	if (spec->data) {
		free(spec->data);
		spec->data = NULL;
		spec->data_size = 0;
	}

	free(spec);
	spec = NULL;
}

api_public
size_t ocean_spectra_get_size(struct ocean_spectra *spec)
{
	return spec ? spec->data_size - 1 : (size_t)-EINVAL;
}

api_public
double *ocean_spectra_get_data(struct ocean_spectra *spec)
{
	return spec ? spec->data : NULL;
}

api_public
size_t ocean_spectra_get_raw_size(struct ocean_spectra *spec)
{
	return spec ? spec->raw_size : (size_t)-EINVAL;
}

api_public
uint8_t *ocean_spectra_get_raw_data(struct ocean_spectra *spec)
{
	return spec ? spec->raw : NULL;
}

static void ocean_spectra_clear(struct ocean_spectra *spec)
{
	memset(spec->raw, 0, spec->raw_size);
	memset(spec->data, 0, spec->data_size);
}

api_private
int ocean_send_command(struct ocean *self, uint8_t *cmd, size_t len)
{
	int done = 0;
	int ret;

	ret = libusb_bulk_transfer(self->dev, self->ep[EP_CMD_SEND],
				   cmd, len, &done, self->timeout);
	if (ret < 0) {
		fprintf(stderr, "ERR: usb read failed: %d (done %d/%zu)\n",
			ret, done, len);
		return ret;
	}

	return 0;
}

/*
 * WORKS partialy, sometimes reading fails
 */
static int ocean_query_dev_info(struct ocean *self, uint8_t what, uint8_t *buf, size_t len)
{
	uint8_t cmd[] = { 0x05, what };
	int done = 0;
	int ret;

	ret = ocean_send_command(self, cmd, ARRAY_SIZE(cmd));
	if (ret < 0)
		return -EIO;

	ret = libusb_bulk_transfer(self->dev, self->ep[EP_CMD_RECV],
				   buf, len, &done, self->timeout);
	if (ret < 0) {
		fprintf(stderr, "ERR: usb read failed: %d (done %d/%zu)\n",
			ret, done, len);
		return ret;
	}
	return 0;
}

static int ocean_initialize(struct ocean *self)
{
	uint8_t cmd[] = { 0x01 };
	return ocean_send_command(self, cmd, ARRAY_SIZE(cmd));
}

api_public
int ocean_create(struct ocean **oceanp)
{
	struct ocean *ctx = NULL;
	int ret;

	ctx = malloc(sizeof(*ctx));
	if (!ctx)
		return -ENOMEM;

	memset(ctx, 0, sizeof(*ctx));
	ctx->timeout = 1000;

	ret = libusb_init(&ctx->usb);
	if (ret != 0)
		return -ENODEV;

	*oceanp = ctx;
	return 0;
}

api_public
void ocean_free(struct ocean *self)
{
	if (!self)
		return;

	ocean_close(self);

	libusb_exit(self->usb);
	self->usb = NULL;

	free(self);
	self = NULL;
}

extern int nirquest512_initialize(struct ocean *, uint16_t, uint16_t);
extern int usb4000_initialize(struct ocean *, uint16_t, uint16_t);

static int ocean_init_device_specific(struct ocean *self, uint16_t vendor,
				      uint16_t product)
{
	static const struct initialize {
		Initialize initfunc;
	} INIT[] = {
		{ .initfunc = nirquest512_initialize },
		{ .initfunc = usb4000_initialize },
	};
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(INIT); i++) {
		ret = INIT[i].initfunc(self, vendor, product);
		/* If vendor/product is not supported, try the next one */
		if (ret == -ENODEV)
			continue;
		/* When the device is supported, it should be initialized
		 * or a possible error should be reported */
		break;
	}
	return ret;
}

api_public
int ocean_open(struct ocean *self, uint16_t vendor, uint16_t product)
{
	uint8_t desc[32] = { 0 };
	int ret;
	int i;

	if (!self)
		return -EINVAL;

	ocean_close(self);

	/* try to find and open the device */
	self->dev = libusb_open_device_with_vid_pid(self->usb, vendor, product);
	if (self->dev == NULL) {
		fprintf(stderr, "ERR: libusb_open_device_with_vid_pid: 0x%x:0x%x "
			"not found\n", vendor, product);
		return -ENODEV;
	}

	/* apply the device specific settings */
	ret = ocean_init_device_specific(self, vendor, product);
	if (ret < 0) {
		fprintf(stderr, "ERR: libusb_set_auto_detach_kernel_driver: %s\n",
			libusb_strerror(ret));
		goto cleanup;
	}

#if HAVE_LIBUSB_EXTENDED
	ret = libusb_set_auto_detach_kernel_driver(self->dev, true);
	if (ret < 0) {
		fprintf(stderr, "ERR: libusb_set_auto_detach_kernel_driver: "
			"%d -> %s\n", ret, libusb_strerror(ret));
		ret = -EIO;
		goto cleanup;
	}
#endif

	/* select the right usb configuration, for now we leave the
	 * configuration default, for some unknown reason, setting the
	 * configuration to 1 will cause the device stop working. */
	ret = libusb_claim_interface(self->dev, 0);
	if (ret < 0) {
		fprintf(stderr, "ERR: libusb_claim_interface: %d -> %s\n", ret,
			libusb_strerror(ret));
		ret = -EIO;
		goto cleanup;
	}
	/* dump the device descriptor, just for debug purposes */
	ret = libusb_get_string_descriptor_ascii(self->dev, 1, desc, ARRAY_SIZE(desc));
	if (ret < 0) {
		fprintf(stderr, "ERR: libusb_get_string_descriptor_ascii: %d\n",
			ret);
	} else
		printf("Device is: %s\n", desc);

	/* clear the endpoints, otherwise we can't communicate */
	for (i = 0; i < ARRAY_SIZE(self->ep); i++) {
		if (self->ep[i] == 0)
			continue;

		ret = libusb_clear_halt(self->dev, self->ep[i]);
		if (ret < 0) {
			fprintf(stderr, "ERR: libusb_clear_halt(ep: 0x%x): %d\n",
				self->ep[i], ret);
		}
	}

	/* Send the init command to the spectrometer */
	ret = ocean_initialize(self);
	if (ret < 0) {
		fprintf(stderr, "ERR: %s: %s\n", __func__, strerror(-ret));
		goto cleanup;
	}

	return 0;

cleanup:
	ocean_close(self);
	return ret;
}

api_public
void ocean_close(struct ocean *self)
{
	if (!self || !self->dev)
		return;

	if (self->dev) {
		libusb_release_interface(self->dev, 0);
		libusb_close(self->dev);
		self->dev = NULL;
	}
}

api_public
int ocean_reset(struct ocean *self)
{
	if (!self && !self->dev)
		return -EINVAL;

	if (libusb_reset_device(self->dev) != 0)
		return -ECONNRESET;

	return 0;
}

api_private
int ocean_query_status(struct ocean *self)
{
	uint8_t cmd[] = { 0xFE };
	int done = 0;
	int ret;

	ret = ocean_send_command(self, cmd, ARRAY_SIZE(cmd));
	if (ret < 0)
		return -EIO;

	ret = libusb_bulk_transfer(self->dev, self->ep[EP_CMD_RECV],
				   (uint8_t *)self->status,
				   sizeof(*self->status),
				   &done, self->timeout);
	if (ret < 0) {
		fprintf(stderr, "ERR: usb read failed: %d (done %d/%zu)\n",
			ret, done, sizeof(*self->status));
		return ret;
	}

	return 0;
}

api_public
int ocean_get_serial(struct ocean *self, char *buf, size_t len)
{
#if 1
	uint8_t cmd[] = { 0x08 };
	int done = 0;
	int ret;

	if (!self || !buf || len < MIN_RET_BUF_LEN)
		return -EINVAL;

	ret = ocean_send_command(self, cmd, ARRAY_SIZE(cmd));
	if (ret < 0)
		return -EIO;

	ret = libusb_bulk_transfer(self->dev, self->ep[EP_CMD_RECV],
				   (uint8_t *)buf, len, &done, self->timeout);
	if (ret < 0) {
		fprintf(stderr, "ERR: usb read failed: %d (done %d/%zu)\n",
			ret, done, len);
		return ret;
	}
	return 0;
#else
	/* FIXME: ocean_dump_all is working this call not.
	 *        figure out why... */
	return ocean_query_dev_info(self, OCEAN_DEVICE_SERIAL, buf, len);
#endif
}

api_public
int ocean_get_temperature(struct ocean *self, float *pcb, float *sink)
{
	uint8_t cmd[] = { 0x6c };
	uint8_t buf[6];
	int done = 0;
	int adc;
	int ret;

	if (!self || !pcb || !sink)
		return -EINVAL;

	ret = ocean_send_command(self, cmd, ARRAY_SIZE(cmd));
	if (ret < 0)
		return -EIO;

	ret = libusb_bulk_transfer(self->dev, self->ep[EP_CMD_RECV],
				   buf, ARRAY_SIZE(buf), &done, self->timeout);
	if (ret < 0) {
		fprintf(stderr, "ERR: usb read failed: %d (done %d/%lu)\n",
			ret, done, ARRAY_SIZE(buf));
		return ret;
	}

	/* FIXME: convert this properly depending on cpu */
	adc = ((buf[2] << 8) | buf[1]);
	*pcb = 0.003906f * adc;
	/* FIXME: convert this properly depending on cpu */
	adc = ((buf[5] << 8) | buf[4]);
	*sink = 0.003906f * adc;

	return 0;
}

api_public
int ocean_set_integration_time(struct ocean *self, uint32_t time)
{
	if (!self)
		return -EINVAL;

	return self->set_int_time(self, time);
}

api_public
int ocean_get_integration_time(struct ocean *self, uint32_t *time)
{
	if (!self || !time)
		return -EINVAL;

	return self->get_int_time(self, time);
}

api_public
int ocean_get_num_of_pixel(struct ocean *self, uint32_t *num_of_pixel)
{
	int ret;

	if (!num_of_pixel)
		return -EINVAL;

	ret = self->get_num_pixel(self, num_of_pixel);
	if (ret < 0)
		return ret;

	return 0;
}

api_public
int ocean_enable_strob(struct ocean *self, bool enable)
{
	uint8_t cmd[] = { 0x03, 0x00, 0x00 };

	if (!self)
		return -EINVAL;

	if (enable)
		cmd[1] = 0x01;

	return ocean_send_command(self, cmd, ARRAY_SIZE(cmd));
}

api_public
int ocean_enable_fan(struct ocean *self, bool enable)
{
	uint8_t cmd[] = { 0x70, 0x00, 0x00 };

	if (!self)
		return -EINVAL;

	if (enable)
		cmd[1] = 0x01;

	return ocean_send_command(self, cmd, ARRAY_SIZE(cmd));
}

api_public
int ocean_enable_external_trigger(struct ocean *self, bool enable)
{
	uint8_t cmd[] = { 0x0A, 0x00, 0x00 };

	if (!self)
		return -EINVAL;

	if (enable)
		cmd[1] = 0x03;

	return ocean_send_command(self, cmd, ARRAY_SIZE(cmd));
}

api_public
int ocean_request_spectra(struct ocean *self, struct ocean_spectra *spec)
{
	uint8_t cmd[] = { 0x09 };
	int ret;

	if (!self || !spec)
		return -EINVAL;

	ocean_spectra_clear(spec);

	ret = ocean_send_command(self, cmd, ARRAY_SIZE(cmd));
	if (ret < 0)
		return -EIO;

	ret = self->receive(self, spec);
	if (ret < 0)
		goto cleanup;

	return 0;

cleanup:
	ret = ocean_stop_spectral_acquisition(self);
	if (ret < 0)
		return ret;

	return -ENODATA;
}

api_public
int ocean_stop_spectral_acquisition(struct ocean *self)
{
	uint8_t cmd[] = { 0x1e };

	if (!self)
		return -EINVAL;

	return ocean_send_command(self, cmd, ARRAY_SIZE(cmd));
}

api_public
double ocean_spectra_get_wavelength(struct ocean_spectra *spec, uint16_t pixel_number)
{
	double value = 0.0;
	int order;

	for (order = ARRAY_SIZE(spec->wl_cal_coef) - 1; order > 0; order--)
		value = pixel_number * (spec->wl_cal_coef[order] + value);

	return spec->wl_cal_coef[0] + value;
}

api_public
int ocean_get_version(int *major, int *minor, int *patch)
{
	if (!major || !minor || !patch)
		return -EINVAL;

	return sscanf(PACKAGE_VERSION, "%d.%d.%d", major, minor, patch);
}
