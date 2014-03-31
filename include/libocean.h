/*
 * libocean.h - Libocean is a simple library to query data from
 *              Ocean-Optics Spectrometers.
 *
 * Copyright (C) 2014 Soeren Grunewald <soeren.grunewald@desy.de>
 * Copyright (C) 2014 DESY Hamburg
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef LIBOCEAN_H
#define LIBOCEAN_H 1

#ifdef __cplusplus
extern "C" {
#endif

struct ocean;
struct ocean_status;
struct ocean_spectra;

/**
 * Create a new spectra container, and query the current coefficients
 *
 * @param[out] spec A pointer that should keep the new context instance.
 * @param[in] ctx A pointer to a valid ocean context
 *
 * @return Zero on success, otherwise negative error number
 *
 * @see #ocean_spectra_free
 * @see #ocean_open
 */
int ocean_spectra_create(struct ocean_spectra **spec, struct ocean *ctx);
/**
 * Free all acquired resources in the given context.
 *
 * @param[in] spec The pointer to the context to freed
 *
 * @see #ocean_spectra_create
 */
void ocean_spectra_free(struct ocean_spectra *spec);

/* The unprocessed raw data */
size_t ocean_spectra_get_raw_size(struct ocean_spectra *spec);
uint8_t *ocean_spectra_get_raw_data(struct ocean_spectra *spec);

/**
 * Returns the size of the spectra. This is usually equal to the value
 * returned by {@link #ocean_get_num_of_pixel}. In some cases it can be less
 * because some of the sensor pixels are not valid for usage.
 *
 * @param[in] ctx A pointer to a valid ocean_spectra context
 *
 * @return The number of pixels in the spectra.
 *
 * @see #ocean_spectra_get_data
 * @see #ocean_request_spectra
 * @see #ocean_spectra_get_wavelength
 */
size_t ocean_spectra_get_size(struct ocean_spectra *spec);
/**
 * Returns the spectra data with applied correction coefficients. Each value
 * is a intensity value. To get the correct wavelength belonging to this value
 * see {@link #ocean_spectra_get_wavelength}.
 *
 * @param[in] ctx A pointer to a valid ocean_spectra context
 *
 * @return The spectra data, or NULL.
 *
 * @see #ocean_spectra_get_data
 * @see #ocean_spectra_get_wavelength
 */
double *ocean_spectra_get_data(struct ocean_spectra *spec);

/**
 * Returns the wavelength belonging to the given pixel.
 *
 * @param[in] ctx A pointer to a valid ocean_spectra context
 * @param[in] pixel The pixel in a range from 0 to spectra length.
 *
 * @return The wavelength, or undefined value if pixel is out of range.
 *
 * @see #ocean_spectra_get_size
 * @see #ocean_request_spectra
 */
double ocean_spectra_get_wavelength(struct ocean_spectra *spec, int pixel);


/**
 * Receive the version of libocean in major.minor.patch notation.
 *
 * @param[out] major The major version number
 * @param[out] minor The minor version number
 * @param[out] patch The patch level.
 *
 * @return Zero on success, otherwise negative error number
 */
int ocean_get_version(int *major, int *minor, int *patch);

/**
 * Create a new libocean context.
 *
 * @param[out] ctx A pointer that should keep the new context instance.
 *
 * @return Zero on success, otherwise negative error number
 *
 * @see #ocean_free
 */
int ocean_create(struct ocean **ctx);
/**
 * Free all acquired resources.
 *
 * @param[in] ctx The pointer to the context to freed
 *
 * @return Zero on success, otherwise negative error number
 *
 * @see #ocean_create
 */
void ocean_free(struct ocean *ctx);

/**
 * Initialize the specified spectrometer.
 *
 * @param[in] ctx A pointer to a valid context
 * @param[in] vendor The usb vendor code (usually 0x2457)
 * @param[in] product The usb product code
 *
 * @return Zero on success, otherwise negative error number
 *
 * @see #ocean_close
 * @see #ocean_create
 */
int ocean_open(struct ocean *ctx, uint16_t vendor, uint16_t product);
/**
 * Close a opened spectrometer and free all acquired resources.
 *
 * @param[in] ctx The pointer to the context to be closed
 *
 * @see #ocean_open
 * @see #ocean_free
 */
void ocean_close(struct ocean *ctx);

/**
 * Request and receive the spectrometer serial
 *
 * @param[in] ctx A pointer to a valid context
 * @param[out] buf A buffer to receive the serial
 * @param[out] len The maximal size of the given buffer
 *
 * @return Zero on success, otherwise negative error number
 *
 * @see #ocean_open
 */
int ocean_get_serial(struct ocean *ctx, char *buf, size_t len);
/**
 * Request and receive the current spectrometer temperature.
 *
 * @param[in] ctx A pointer to a valid context
 * @param[out] pcb The temperature of the spectrometer PCB
 * @param[out] sink The temperature of the spectrometer sink
 *
 * @return Zero on success, otherwise negative error number
 *
 * @see #ocean_open
 */
int ocean_get_temperature(struct ocean *ctx, float *pcb, float *sink);

/**
 * Set the integration time for the spectrometer.
 *
 * @param[in] ctx A pointer to a valid context
 * @param[in] time The new integration time in milli seconds
 *
 * @return Zero on success, otherwise negative error number
 *
 * @see #ocean_get_integration_time
 * @see #ocean_request_spectra
 */
int ocean_set_integration_time(struct ocean *ctx, uint32_t time);
/**
 * Query and receive the currently set integration time.
 *
 * @param[in] ctx A pointer to a valid context
 * @param[out] time Variable to receive the value.
 *
 * @return Zero on success, otherwise negative error number
 */
int ocean_get_integration_time(struct ocean *ctx, uint32_t *time);

int ocean_enable_strob(struct ocean *ctx, bool enable);
int ocean_enable_fan(struct ocean *ctx, bool enable);
int ocean_enable_external_trigger(struct ocean *ctx, bool enable);

/**
 * Cause the spectrometer to start data acquisition and store the
 * result in the given spectra structure. If you receive spectra faster
 * than the current integration time, it can happen that your spectra
 * is not completely filled.
 *
 * @param[in] ctx A pointer to a valid context
 * @param[out] spec A pointer to a valid spectra
 *
 * @return Zero on success, otherwise negative error number
 *
 * @see #ocean_open
 * @see #ocean_stop_spectral_acquisition
 * @see #ocean_set_integration_time
 */
int ocean_request_spectra(struct ocean *ctx, struct ocean_spectra *spec);
/**
 * Cause the spectrometer to stop the data acquisition.
 *
 * @param[in] ctx A pointer to a valid context
 *
 * @return Zero on success, otherwise negative error number
 *
 * @see #ocean_request_spectra
 */
int ocean_stop_spectral_acquisition(struct ocean *ctx);
/**
 * Query and receive the number of pixels the spectrometer provides.
 *
 * @param[in] ctx A pointer to a valid context
 * @param[out] num_of_pixel A pointer to store the value
 *
 * @return Zero on success, otherwise negative error number
 *
 * @see #ocean_open
 */
int ocean_get_num_of_pixel(struct ocean *ctx, uint32_t *num_of_pixel);


/* For testing */
int ocean_dump_status(struct ocean *self, FILE *out);

#ifdef __cplusplus
};
#endif

#endif /* LIBOCEAN_H */
