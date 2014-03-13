/*
 * libocean.h
 *
 * Copyright (C) 2014 Soeren Grunewald <soeren.grunewald@desy.de>
 * Copyright (C) 2014 DESY Hamburg
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

/* Create a new spectra container, and query the current coefficents */
int ocean_spectra_create(struct ocean_spectra **spec, struct ocean *ctx);
void ocean_spectra_free(struct ocean_spectra *spec);

/* The unprocessed raw data */
size_t ocean_spectra_get_raw_size(struct ocean_spectra *spec);
uint8_t *ocean_spectra_get_raw_data(struct ocean_spectra *spec);

/* The values with applied correction coefficents */
size_t ocean_spectra_get_size(struct ocean_spectra *spec);
double *ocean_spectra_get_data(struct ocean_spectra *spec);

/* Returns the wavelength belonging to a pixel */
double ocean_spectra_get_wavelength(struct ocean_spectra *spec, int pixel);


int ocean_create(struct ocean **ctx);
void ocean_free(struct ocean *ctx);

int ocean_open(struct ocean *ctx, uint16_t vendor, uint16_t product);
void ocean_close(struct ocean *ctx);

int ocean_get_serial(struct ocean *ctx, char *buf, size_t len);
int ocean_get_temperature(struct ocean *ctx, float *pcb, float *sink);

int ocean_set_integration_time(struct ocean *ctx, uint32_t time);
int ocean_get_integration_time(struct ocean *ctx, uint32_t *time);

int ocean_enable_strob(struct ocean *ctx, bool enable);
int ocean_enable_fan(struct ocean *ctx, bool enable);
int ocean_enable_external_trigger(struct ocean *ctx, bool enable);

int ocean_request_spectra(struct ocean *ctx, struct ocean_spectra *spec);
int ocean_stop_spectral_acquisition(struct ocean *ctx);
int ocean_get_num_of_pixel(struct ocean *ctx, uint32_t *num_of_pixel);


/* For testing */
int ocean_dump_status(struct ocean *self, FILE *out);

#ifdef __cplusplus
};
#endif

#endif /* LIBOCEAN_H */
