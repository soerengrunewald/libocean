#include "libocean_p.h"

#define MIN_RET_BUF_LEN 16

struct devices {
	uint16_t vendor;
	uint16_t product;
	uint8_t endpoint[4];
	/* function to receive data from the spectrometer */
	ReceiveFuncPtr receive;
};

extern int nirquest512_recv_spectra(struct ocean *, struct ocean_spectra *);
extern int usb4000_recv_spectra(struct ocean *, struct ocean_spectra *);

/* FIXME: This is device specific... */
static const struct devices SUPPORTED[] = {
	{ /* OceanOptics USB4000 */
		.vendor = 0x2457,
		.product = 0x1022,
		.endpoint = {
			(0x01 | LIBUSB_ENDPOINT_OUT),
			(0x81 | LIBUSB_ENDPOINT_IN),
			(0x86 | LIBUSB_ENDPOINT_IN),
			(0x82 | LIBUSB_ENDPOINT_IN),
		},
		.receive = usb4000_recv_spectra,
	}, { /* OceanOptics NirQuest512 */
		.vendor = 0x2457,
		.product = 0x1026,
		.endpoint = {
			(0x01 | LIBUSB_ENDPOINT_OUT),
			(0x81 | LIBUSB_ENDPOINT_IN),
			(0x82 | LIBUSB_ENDPOINT_IN),
			0
		},
		.receive = nirquest512_recv_spectra,
	}, {
		.vendor = 0x2457,
		.product = 0x1028,
		.endpoint = {
			(0x01 | LIBUSB_ENDPOINT_OUT),
			(0x81 | LIBUSB_ENDPOINT_IN),
			(0x82 | LIBUSB_ENDPOINT_IN),
			0
		},
		.receive = nirquest512_recv_spectra,
	}
};

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
static int ocean_query_status(struct ocean *self, struct ocean_status *status);

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
		ret = ocean_query_dev_info(ocean, OCEAN_WAVELEN_CAL_COEF_0 + order, buf, ARRAY_SIZE(buf));
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
	ret = ocean_query_dev_info(ocean, OCEAN_POLY_ORDER_NON_LIN_COR, buf, ARRAY_SIZE(buf));
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
		ret = ocean_query_dev_info(ocean, OCEAN_NON_LIN_COR_COEF_0 + order, buf, ARRAY_SIZE(buf));
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
	struct ocean_status status;
	int ret;

	if (!ocean)
		return -EINVAL;

	ret = ocean_query_status(ocean, &status);
	if (ret < 0)
		return -EIO;

	/* FIXME: The length needs to be larger because of the sync byte */
	ret = ocean_spectra_create_custom(spec, (status.num_of_pixels * 2) + 2);
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

static int ocean_send_command(struct ocean *self, uint8_t *cmd, size_t len)
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

//	libusb_set_debug(ctx->usb, LIBUSB_LOG_LEVEL_INFO);

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

static int ocean_supports(uint16_t vendor, uint16_t product)
{
	unsigned i;

	for (i = 0; i < ARRAY_SIZE(SUPPORTED); i++) {
		if (vendor == SUPPORTED[i].vendor &&
		    product == SUPPORTED[i].product)
			return true;
	}

	printf("WRN: %s: vendor=0x%x product=0x%x not supported\n",
		__func__, vendor, product);
	return false;
}

static void ocean_set_device_specifics_for(struct ocean *self,
					   uint16_t vendor,
					   uint16_t product)
{
	unsigned i;

	for (i = 0; i < ARRAY_SIZE(SUPPORTED); i++) {
		if (vendor == SUPPORTED[i].vendor &&
		    product == SUPPORTED[i].product) {
			memcpy(self->ep, &SUPPORTED[i].endpoint, ARRAY_SIZE(self->ep));
			self->receive = SUPPORTED[i].receive;
			break;
		}
	}
}

api_public
int ocean_open(struct ocean *self, uint16_t vendor, uint16_t product)
{
	uint8_t desc[32] = { 0 };
	int ret;
	int i;

	if (!self || !ocean_supports(vendor, product))
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
	ocean_set_device_specifics_for(self, vendor, product);

/*
	ret = libusb_set_auto_detach_kernel_driver(self->dev, true);
	if (ret < 0) {
		fprintf(stderr, "ERR: libusb_set_auto_detach_kernel_driver: %s\n",
			libusb_strerror(ret));
		return -EIO;
	}

*/
	/* select the right usb configuration, for now we leave the
	 * configuration default, for some unknown reason, setting the
	 * configuration to 1 will cause the device stop working. */
	ret = libusb_claim_interface(self->dev, 0);
	if (ret < 0) {
		fprintf(stderr, "ERR: libusb_claim_interface: %d\n", ret);
		return -EIO;
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
		return -EIO;
	}

	return 0;
}

api_public
void ocean_close(struct ocean *self)
{
	if (!self || !self->dev)
		return;

	libusb_release_interface(self->dev, 0);
	libusb_close(self->dev);
	self->dev = NULL;
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

static int ocean_query_status(struct ocean *self, struct ocean_status *status)
{
	uint8_t cmd[] = { 0xFE };
	int done = 0;
	int ret;

	ret = ocean_send_command(self, cmd, ARRAY_SIZE(cmd));
	if (ret < 0)
		return -EIO;

	ret = libusb_bulk_transfer(self->dev, self->ep[EP_CMD_RECV],
				   (uint8_t *)status, sizeof(*status),
				   &done, self->timeout);
	if (ret < 0) {
		fprintf(stderr, "ERR: usb read failed: %d (done %d/%zu)\n",
			ret, done, sizeof(status));
		return ret;
	}

	memcpy(&self->status, status, sizeof(*status));
	return 0;
}

api_public
int ocean_dump_status(struct ocean *self, FILE *out)
{
	struct ocean_status status;
	int ret;

	ret = ocean_query_status(self, &status);
	if (ret < 0)
		return -EIO;

	fprintf(out, "query status:\n");
	fprintf(out, "  +- num_of_pixels:........... 0x%x\n", status.num_of_pixels);
	fprintf(out, "  +- integration_time:........ 0x%x\n", status.integration_time);
	fprintf(out, "  +- lamp_enable:............. 0x%x\n", status.lamp_enable);
	fprintf(out, "  +- trigger_mode:............ 0x%x\n", status.trigger_mode);
	fprintf(out, "  +- request_spectrum:........ 0x%x\n", status.request_spectrum);
	fprintf(out, "  +- specral_data_ready:...... 0x%x\n", status.specral_data_ready);
	fprintf(out, "  +- power_state:............. 0x%x\n", status.power_state);
	fprintf(out, "  +- spectral_data_counter:... 0x%x\n", status.spectral_data_counter);
	fprintf(out, "  +- detector_gain_select:.... 0x%x\n", status.detector_gain_select);
	fprintf(out, "  `- fan_and_tec_state:....... 0x%x\n", status.fan_and_tec_state);

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
	uint8_t cmd[] = { 0x02, 0x00, 0x00, 0x00, 0x00 };
	int ret;

	/* Depending on device, the integration time is given
	 * in µs or ms. So we need to apply a correction factor */
	if (self->receive == usb4000_recv_spectra) {
		printf("DBG: integration time given in ms, but need µm\n");
		time *= 1000;
	}

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

	self->status.integration_time = time;
	return 0;
}

api_public
int ocean_get_integration_time(struct ocean *self, uint32_t *time)
{
	struct ocean_status status;
	int ret;

	if (!time)
		return -EINVAL;

	ret = ocean_query_status(self, &status);
	if (ret < 0)
		return ret;

	*time = (uint32_t)status.integration_time;
	return 0;
}

api_public
int ocean_get_num_of_pixel(struct ocean *self, uint32_t *num_of_pixel)
{
	struct ocean_status status;
	int ret;

	if (!num_of_pixel)
		return -EINVAL;

	ret = ocean_query_status(self, &status);
	if (ret < 0)
		return ret;

	*num_of_pixel = (uint32_t)status.num_of_pixels;
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
double ocean_spectra_get_wavelength(struct ocean_spectra *spec, int pixel_number)
{
	double value = 0.0;
	int order;

	for (order = ARRAY_SIZE(spec->wl_cal_coef) - 1; order > 0; order--)
		value = pixel_number * (spec->wl_cal_coef[order] + value);

	return spec->wl_cal_coef[0] + value;
}
