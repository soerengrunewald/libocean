#include "libocean.h"

#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof (x[0]))
#endif

static void hexdump(uint8_t *buf, size_t len)
{
	const size_t rowsize = 16;
	const uint8_t *ptr = buf;
	FILE *fp = stdout;
	unsigned i, j;

	for (j = 0; j < len; j += rowsize) {

		fprintf(fp, "0x%08x | ", j);

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

/**
 * Test basic query functions
 */
static int test_query(struct ocean *usb)
{
	float pcb_temp = 0.0f, sink_temp = 0.0f;
	char buf[512] = { 0 };
	uint32_t pixel = 0;
	size_t len = 16;
	int ret;

	ret = ocean_get_serial(usb, buf, len);
	if (ret < 0) {
		printf("ocean_get_serial: %d\n", ret);
		goto out;
	}
	printf("Serial [#%s]\n", buf);

	ret = ocean_get_temperature(usb, &pcb_temp, &sink_temp);
	if (ret < 0) {
		printf("ocean_get_temperature: %d\n", ret);
		goto out;
	}
	printf("Temperature: PCB=%.2f°C HeatSink=%.2f°C\n",
		pcb_temp, sink_temp);

	ret = ocean_get_num_of_pixel(usb, &pixel);
	if (ret < 0) {
		printf("ocean_get_num_of_pixel: %d\n", ret);
		goto out;
	}
	printf("Pixel: %u\n", pixel);

	ret = ocean_dump_status(usb, stdout);
	if (ret < 0) {
		printf("ocean_dump_status: %d\n", ret);
		goto out;
	}

out:
	return ret;
}

/**
 * Test enable and disable functions.
 */
static int test_enable(struct ocean *usb)
{
	int ret;

	ret = ocean_enable_fan(usb, false);
	if (ret < 0) {
		printf("ocean_enable_fan: %d\n", ret);
		goto out;
	}

	ret = ocean_enable_strob(usb, true);
	if (ret < 0) {
		printf("ocean_enable_strob: %d\n", ret);
		goto out;
	}

	ret = ocean_set_integration_time(usb, 1000);
	if (ret < 0) {
		printf("ocean_set_integration_time: %d\n", ret);
		goto out;
	}

	sleep(3);
	ocean_dump_status(usb, stdout);

	ret = ocean_enable_fan(usb, true);
	if (ret < 0) {
		printf("ocean_enable_fan: %d\n", ret);
		goto out;
	}

	ret = ocean_enable_strob(usb, false);
	if (ret < 0) {
		printf("ocean_enable_strob: %d\n", ret);
		goto out;
	}

	ret = ocean_set_integration_time(usb, 2000);
	if (ret < 0) {
		printf("ocean_set_integration_time: %d\n", ret);
		goto out;
	}

	sleep(3);
	ocean_dump_status(usb, stdout);

out:
	return ret;
}

/**
 * (Hex)Dump some spectras
 */
static int test_spectra_dump(struct ocean *usb)
{
	struct ocean_spectra *spec = NULL;
	int ret, i;

	ret = ocean_spectra_create(&spec, usb);
	if (ret < 0) {
		printf("ocean_spectra_create: %d\n", ret);
		goto out;
	}

	for (i=0; i<3; i++) {
		uint8_t *buf;
		size_t len;

		ret = ocean_request_spectra(usb, spec);
		if (ret < 0) {
			printf("ocean_request_spectra: %d\n", ret);
			goto cleanup;
		}

		buf = ocean_spectra_get_raw_data(spec);
		len = ocean_spectra_get_raw_size(spec);
		hexdump(buf, len);

		sleep(2); /* give it some time to integrate */
	}

cleanup:
	ocean_spectra_free(spec);
out:
	return ret;
}


/**
 * Create a csv file from the spectra data
 */
static int test_spectra_csv(struct ocean *usb)
{
	struct ocean_spectra *spec = NULL;
	double *buf;
	size_t len;
	int ret, i;
	FILE *f;

	ret = ocean_spectra_create(&spec, usb);
	if (ret < 0) {
		printf("ocean_spectra_create: %d\n", ret);
		goto out;
	}

	f = fopen("sample.csv", "w+");
	if (!f) {
		printf("failed to open output file: %d\n", errno);
		goto cleanup;
	}

	ret = ocean_request_spectra(usb, spec);
	if (ret < 0) {
		printf("ocean_request_spectra: %d\n", ret);
		goto cleanup;
	}

	buf = ocean_spectra_get_data(spec);
	len = ocean_spectra_get_size(spec);

	fprintf(f, "Wavelength (nm), Intensity (counts)\n");
	for (i = 0; i < len; i++) {
		fprintf(f, "%e, %e\n", ocean_spectra_get_wavelength(spec, i), *buf++);
	}

cleanup:
	ocean_spectra_free(spec);
	fclose(f);
out:
	return ret;
}

int main(int argc, char *argv[])
{
	struct ocean *usb = NULL;
	int ret;

	ret = ocean_create(&usb);
	if (ret < 0) {
		printf("ocean_create: %d\n", ret);
		goto out;
	}

	ret = ocean_open(usb, 0x2457, 0x1026);
	if (ret < 0) {
		printf("ocean_open: %d\n", ret);
		goto out;
	}

	test_query(usb);
//	test_enable(usb);
//	test_spectra_dump(usb);
	test_spectra_csv(usb);

out:
	ocean_free(usb);
	return 0;
}
