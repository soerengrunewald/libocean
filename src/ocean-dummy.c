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

#include "config.h"
#include <libocean.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(WIN32)
#  if __GNUC__ >= 4
#    define api_public __attribute__((visibility("default")))
#    define api_private __attribute__((visibility("hidden")))
#  else
#    define api_public
#    define api_private
#  endif
#else
#  define api_public
#  define api_private
#endif

static const uint8_t raw1[] = {
	0
};

static const uint8_t raw2[] = {
	0
};

/* SUN at a sunny spring day ;) */
static const double spectrum1[] = {
	2866.02, 2883.02, 2902.14, 2912.76, 2891.52,
	2868.15, 2885.14, 2881.96, 2907.45, 2891.52,
	2928.70, 2952.07, 3000.93, 3033.86, 3046.61,
	3052.98, 3038.11, 3003.05, 2910.64, 2835.21,
	2766.17, 2737.49, 2739.61, 2777.85, 2821.41,
	2856.46, 2812.91, 2816.09, 2807.60, 2816.09,
	2822.47, 2836.28, 2838.40, 2832.03, 2849.02,
	2867.08, 2870.27, 2881.96, 2894.70, 2941.44,
	2956.31, 3004.12, 3029.61, 3048.73, 3038.11,
	3031.74, 3018.99, 3027.49, 3041.30, 3055.11,
	3069.98, 3096.53, 3098.66, 3116.72, 3102.91,
	3115.66, 3125.22, 3125.22, 3113.53, 3143.28,
	3112.47, 3132.65, 3115.66, 3120.97, 3126.28,
	3115.66, 3105.03, 3105.03, 3127.34, 3098.66,
	3115.66, 3113.53, 3110.34, 3088.04, 3091.22,
	3108.22, 3086.97, 3108.22, 3077.41, 3083.79,
	3094.41, 3092.29, 3067.85, 3083.79, 3046.61,
	3062.54, 3048.73, 3074.23, 3041.30, 3054.04,
	3047.67, 3050.86, 3051.92, 3057.23, 3040.23,
	3057.23, 3040.23, 3031.74, 3022.18, 3037.05,
	3014.74, 3020.05, 3001.99, 3020.05, 3007.30,
	3013.68, 3023.24, 3027.49, 3013.68, 3040.23,
	3039.17, 3033.86, 3020.05, 3024.30, 2998.81,
	3035.99, 3001.99, 3030.67, 3013.68, 3035.99,
	2993.49, 2981.81, 2981.81, 2993.49, 2976.50,
	2982.87, 2972.25, 2980.75, 2962.69, 2956.31,
	2934.01, 2892.58, 2869.21, 2825.65, 2796.97,
	2762.98, 2754.48, 2755.54, 2749.17, 2756.61,
	2742.80, 2770.42, 2762.98, 2795.91, 2803.35,
	2778.91, 2767.23, 2824.59, 2864.96, 2840.53,
	2818.22, 2816.09, 2789.54, 2787.41, 2806.53,
	2835.21, 2826.72, 2852.21, 2857.52, 2893.64,
	2894.70, 2920.20, 2938.26, 2952.07, 2958.44,
	2962.69, 2976.50, 2991.37, 2990.31, 2982.87,
	2973.31, 2986.06, 2986.06, 2974.37, 2961.63,
	2965.87, 2960.56, 2986.06, 2976.50, 2988.18,
	2973.31, 3025.36, 2985.00, 3023.24, 2977.56,
	2994.56, 2978.62, 2982.87, 2978.62, 2994.56,
	2989.24, 3020.05, 3001.99, 2999.87, 2985.00,
	2993.49, 2958.44, 2995.62, 2977.56, 3006.24,
	2974.37, 3003.05, 2988.18, 2999.87, 2986.06,
	3005.18, 2989.24, 3001.99, 2991.37, 3003.05,
	2986.06, 2974.37, 3005.18, 2972.25, 2998.81,
	2982.87, 2961.63, 2982.87, 2959.50, 2964.81,
	2966.94, 2953.13, 2934.01, 2946.75, 2939.32,
	2928.70, 2894.70, 2906.39, 2879.83, 2901.08,
	2854.34, 2903.20, 2903.20, 2946.75, 2947.82,
	2947.82, 2960.56, 2949.94, 2935.07, 2958.44,
	2970.12, 2978.62, 2962.69, 2977.56, 2961.63,
	2939.32, 2968.00, 2938.26, 2943.57, 2934.01,
	2940.38, 2945.69, 2924.45, 2927.63, 2923.38,
	2913.82, 2894.70, 2898.95, 2864.96, 2881.96,
	2884.08, 2908.51, 2866.02, 2909.57, 2879.83,
	2888.33, 2862.83, 2846.90, 2817.16, 2830.97,
	2817.16, 2822.47, 2822.47, 2821.41, 2826.72,
	2803.35, 2776.79, 2774.67, 2756.61, 2722.61,
	2717.30, 2691.81, 2690.75, 2687.56, 2683.31,
	2683.31, 2671.62, 2675.87, 2674.81, 2674.81,
	2675.87, 2675.87, 2674.81, 2665.25, 2646.13,
	2656.75, 2638.69, 2604.70, 2616.39, 2634.44,
	2624.88, 2639.76, 2638.69, 2657.81, 2634.44,
	2645.07, 2656.75, 2658.88, 2647.19, 2649.32,
	2661.00, 2634.44, 2649.32, 2642.94, 2664.19,
	2650.38, 2641.88, 2638.69, 2629.13, 2657.81,
	2647.19, 2656.75, 2650.38, 2642.94, 2649.32,
	2673.75, 2633.38, 2652.50, 2648.25, 2652.50,
	2655.69, 2653.57, 2653.57, 2652.50, 2614.26,
	2621.70, 2608.95, 2635.51, 2622.76, 2638.69,
	2628.07, 2648.25, 2630.20, 2647.19, 2622.76,
	2641.88, 2622.76, 2663.13, 2645.07, 2680.12,
	2666.31, 2672.69, 2670.56, 2666.31, 2667.38,
	2667.38, 2647.19, 2655.69, 2635.51, 2674.81,
	2639.76, 2692.87, 2657.81, 2668.44, 2663.13,
	2685.43, 2674.81, 2704.55, 2673.75, 2683.31,
	2688.62, 2716.24, 2697.12, 2721.55, 2702.43,
	2726.86, 2714.12, 2730.05, 2721.55, 2721.55,
	2704.55, 2734.30, 2723.68, 2725.80, 2704.55,
	2738.55, 2726.86, 2721.55, 2673.75, 2655.69,
	2645.07, 2632.32, 2646.13, 2646.13, 2619.57,
	2657.81, 2647.19, 2694.99, 2711.99, 2724.74,
	2704.55, 2730.05, 2707.74, 2738.55, 2718.36,
	2732.17, 2703.49, 2750.23, 2698.18, 2738.55,
	2704.55, 2721.55, 2693.93, 2721.55, 2696.06,
	2717.30, 2700.31, 2710.93, 2698.18, 2714.12,
	2693.93, 2703.49, 2666.31, 2685.43, 2645.07,
	2673.75, 2669.50, 2675.87, 2673.75, 2687.56,
	2659.94, 2692.87, 2675.87, 2686.50, 2659.94,
	2676.94, 2659.94, 2686.50, 2675.87, 2667.38,
	2665.25, 2652.50, 2650.38, 2656.75, 2634.44,
	2652.50, 2625.95, 2648.25, 2631.26, 2653.57,
	2636.57, 2652.50, 2647.19, 2653.57, 2636.57,
	2629.13, 2607.89, 2632.32, 2625.95, 2623.82,
	2611.07, 2615.32, 2602.58, 2620.64, 2578.14,
	2601.51, 2569.65, 2596.20, 2554.77, 2587.70,
	2550.52, 2563.27, 2525.03, 2571.77, 2530.34,
	2547.34, 2525.03, 2564.33, 2523.97, 2554.77,
	2514.41, 2549.46, 2508.03, 2527.15, 2513.35,
	2526.09, 2498.47, 2515.47, 2519.72, 2520.78,
	2505.91, 2517.59, 2496.35, 2527.15, 2491.04,
	2530.34, 2488.91, 2517.59, 2491.04, 2518.66,
	2483.60, 2519.72, 2486.79, 2501.66, 2478.29,
	2494.22, 2479.35, 2505.91, 2468.73, 2495.29,
	2476.17, 2503.78
};

/* officelamp */
static const double spectrum2[] = {
	18001.33, 18505.91, 18538.84, 19037.05,
	19066.79, 19610.68, 19619.18, 20262.92,
	20416.95, 21309.26, 21905.20, 23388.13,
	24343.12, 25895.10, 26573.90, 27818.89,
	28090.83, 29001.20, 28979.95, 29740.54,
	29608.82, 30333.29, 30143.15, 30732.71,
	30513.88, 31028.02, 30733.77, 31308.46,
	31032.27, 31610.15, 31319.09, 31872.53,
	31622.90, 32175.28, 31926.71, 32537.52,
	32263.45, 32906.13, 32693.67, 33321.48,
	33140.89, 33828.18, 33555.18, 34257.34,
	34022.58, 34742.80, 34545.22, 35270.75,
	35047.68, 35744.53, 35572.44, 36323.47,
	36092.96, 36840.80, 36568.86, 37282.70,
	37095.74, 37839.34, 37588.64, 38360.91,
	38047.54, 38792.20, 38463.96, 39234.11,
	38933.48, 39653.70, 39233.04, 39923.52,
	39496.49, 40209.28, 39764.18, 40440.85,
	39971.33, 40594.88, 40103.05, 40770.16,
	40248.58, 40915.69, 40295.32, 40854.08,
	40354.81, 40987.92, 40430.23, 41068.66,
	40486.53, 41098.40, 40605.50, 41232.25,
	40702.17, 41318.29, 40753.16, 41334.23,
	40846.64, 41495.69, 40973.05, 41611.48,
	41081.40, 41693.28, 41183.38, 41846.24,
	41395.84, 42045.95, 41492.50, 42168.11,
	41672.03, 42348.70, 41892.98, 42588.77,
	42135.18, 42863.90, 42441.12, 43176.21,
	42708.81, 43491.71, 43014.75, 43764.71,
	43361.05, 44101.45, 43728.60, 44521.05,
	44077.02, 44819.55, 44425.45, 45224.28,
	44831.24, 45695.93, 45315.64, 46154.83,
	45721.43, 46486.26, 46082.60, 46909.05,
	46467.14, 47312.72, 46855.94, 47704.70,
	47240.48, 48077.55, 47535.79, 48452.54,
	47994.70, 48929.50, 48420.67, 49315.11,
	48827.52, 49683.72, 49082.47, 49976.90,
	49448.95, 50236.10, 49653.97, 50482.55,
	49908.92, 50701.38, 50117.12, 50942.51,
	50264.78, 51013.69, 50347.64, 51123.10,
	50395.44, 51252.70, 50574.97, 51340.87,
	50645.08, 51415.23, 50670.57, 51450.28,
	50657.82, 51444.97, 50745.99, 51507.64,
	50663.13, 51430.10, 50615.33, 51357.86,
	50646.14, 51414.16, 50650.39, 51419.48,
	50697.13, 51440.72, 50734.31, 51487.46,
	50757.68, 51555.45, 50820.35, 51597.94,
	50888.34, 51682.92, 50934.01, 51678.67,
	50874.53, 51529.95, 50679.07, 51325.99,
	50529.29, 51202.77, 50449.62, 51190.02,
	50475.11, 51201.71, 50448.55, 51181.53,
	50395.44, 51167.72, 50395.44, 51177.28,
	50458.12, 51197.46, 50475.11, 51241.01,
	50611.08, 51381.23, 50708.81, 51481.09,
	50792.73, 51592.63, 50849.03, 51593.69,
	50947.82, 51759.40, 51022.18, 51835.89,
	51149.66, 51967.61, 51249.51, 52047.28,
	51320.68, 52133.32, 51385.48, 52174.75,
	51502.33, 52353.22, 51644.68, 52497.69,
	51818.89, 52751.57, 52000.54, 52925.78,
	52222.56, 53109.56, 52344.72, 53271.02,
	52527.43, 53377.25, 52625.16, 53571.65,
	52831.24, 53796.85, 53025.64, 53930.70,
	53144.61, 54061.36, 53310.33, 54245.13,
	53454.80, 54435.28, 53595.02, 54587.18,
	53769.23, 54670.04, 53888.20, 54833.63,
	53930.70, 54940.92, 54025.24, 55000.41,
	54160.15, 55110.88, 54219.63, 55205.43,
	54279.12, 55225.61, 54380.04, 55320.15,
	54402.35, 55443.38, 54503.26, 55434.88,
	54494.76, 55420.01, 54159.09, 54762.46,
	53569.52, 54341.80, 53390.00, 54211.14,
	52909.85, 53510.03, 52932.16, 53875.46,
	53199.85, 53939.19, 53114.87, 54279.12,
	53728.86, 54755.02, 53747.98, 54540.44,
	53191.35, 53713.99, 52677.21, 53982.75,
	53205.16, 54062.42, 53106.37, 54058.17,
	52749.44, 53637.51, 52447.76, 53448.42,
	52728.20, 53317.76, 52449.88, 53530.22,
	52226.80, 53188.16, 52137.57, 52937.47,
	52112.08, 52964.02, 51859.26, 52834.43,
	51852.88, 52670.84, 51719.04, 52575.23,
	51600.06, 52446.70, 51447.09, 52276.73,
	51271.82, 52082.34, 51047.68, 51953.80,
	50909.58, 51801.89, 50773.61, 51621.31,
	50585.59, 51280.32, 50161.74, 51057.24,
	50055.51, 50871.34, 49884.49, 50767.24,
	49751.70, 50667.38, 49678.40, 50510.17,
	49551.99, 50344.45, 49436.21, 50293.46,
	49320.42, 50204.23, 49158.95, 50018.33,
	49069.72, 49945.04, 48904.01, 49795.25,
	48932.69, 49942.91, 49207.82, 50391.19,
	49677.34, 50658.89, 49679.47, 50382.69,
	49211.00, 49908.92, 48867.89, 49551.99,
	48543.89, 49337.41, 48339.94, 49204.63,
	48223.09, 49078.22, 48164.66, 49034.67,
	48025.50, 48959.24, 47986.20, 48853.02,
	47858.73, 48724.48, 47718.51, 48600.19,
	47552.79, 48408.98, 47386.01, 48231.58,
	47160.81, 48095.61, 47088.58, 47899.09,
	46941.98, 47799.24, 46779.45, 47637.77,
	46640.29, 47512.42, 46441.65, 47318.03,
	46278.06, 47135.32, 46188.83, 46994.03,
	45997.62, 46835.75, 45742.67, 46622.24,
	45602.45, 46451.21, 45428.24, 46290.81,
	45233.84, 46074.10, 45003.33, 45801.10,
	44763.25, 45576.96, 44522.12, 45357.07,
	44293.73, 45116.99, 43975.04, 44770.69,
	43639.36, 44391.46, 43311.12, 44085.52,
	43067.86, 43834.82, 42753.43, 43478.96,
	42432.62, 43139.03, 42079.94, 42768.30,
	41659.28, 42351.89, 41207.81, 41918.48,
	40809.46, 41538.18, 40348.43, 41064.41,
	39917.15, 40539.64, 39386.01, 39929.90,
	38688.10, 39241.54, 37888.20, 38361.98,
	36876.92, 37063.88, 35442.84, 35390.79,
	33448.95, 33008.11, 30798.57, 29995.49,
	27625.55, 26532.47, 24160.41, 22975.97,
	20766.44, 19665.92, 17760.19, 16915.68,
	15421.06, 14761.39, 13513.21, 12972.52,
	11924.05, 11528.88, 10648.26, 10308.33,
	 9608.29,  9361.84,  8724.47,  8522.64,
	 8053.12,  7842.78,  7431.68,  7288.28,
	 6927.10,  6826.19,  6489.44,  6436.33,
	 6130.40,  6063.47,  5801.09,  5761.79,
	 5524.90,  5498.34,  5294.38,  5264.64,
	 5062.81,  5047.94,  4899.22,  4886.47,
	 4737.75,  4746.25,  4617.71,  4632.59,
};

struct ocean_spectra {
	uint8_t *raw;
	double *data;
	size_t raw_size;
	size_t data_size;
};

struct ocean_status {
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

struct ocean {
	struct ocean_status status;
};

api_public
int ocean_spectra_create(struct ocean_spectra **spec, struct ocean *ctx)
{
	struct ocean_spectra *s;

	if (!spec || !ctx)
		return -EINVAL;

	s = malloc(sizeof(*s));
	if (!s)
		return -ENOMEM;
	memset(s, 0, sizeof(*s));

	s->raw_size = ctx->status.num_of_pixels * 2;
	s->raw = malloc(s->raw_size);
	if (!s->raw) {
		free(s);
		return -ENOMEM;
	}

	s->data_size = ctx->status.num_of_pixels;
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
void ocean_spectra_free(struct ocean_spectra *spec)
{
	if (!spec)
		return;

	if (spec->data) {
		free(spec->data);
		spec->data = NULL;
	}

	if (spec->raw) {
		free(spec->raw);
		spec->raw = NULL;
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

api_public
double ocean_spectra_get_wavelength(struct ocean_spectra *spec, int pixel)
{
	static const double coef[] = { 8.994393E+02, 1.624139E+00, -9.097670E-05, 3.679440E-08 };
	double value = 0.0;
	int order;

	for (order = 3; order > 0; order--)
		value = pixel * (coef[order] + value);

	return coef[0] + value;
}

api_public
int ocean_create(struct ocean **oceanp)
{
	struct ocean *ctx = NULL;

	ctx = malloc(sizeof(*ctx));
	if (!ctx)
		return -ENOMEM;

	memset(ctx, 0, sizeof(*ctx));

	ctx->status.num_of_pixels     = 0x200;
	ctx->status.integration_time  = 0x64;
	ctx->status.fan_and_tec_state = 0x18;

	*oceanp = ctx;
	return 0;
}

api_public
void ocean_free(struct ocean *ctx)
{
	if (!ctx)
		return;

	free(ctx);
	ctx = NULL;
}

api_public
int ocean_open(struct ocean *ctx, uint16_t vendor, uint16_t product)
{
	if (!ctx)
		return -EINVAL;

	// TODO: implement read-out from CSV files
	return 0;
}

api_public
void ocean_close(struct ocean *ctx)
{
	if (!ctx)
		return;
}

int ocean_query_status(struct ocean *ctx, struct ocean_status *status)
{
	if (!ctx || !status)
		return -EINVAL;

	memcpy(status, &ctx->status, sizeof(*status));
	return 0;
}

api_public
int ocean_dump_status(struct ocean *ctx, FILE *out)
{
	if (!ctx || !out)
		return -EINVAL;

	fprintf(out, "query status:\n");
	fprintf(out, "  +- num_of_pixels:........... 0x%x\n", ctx->status.num_of_pixels);
	fprintf(out, "  +- integration_time:........ 0x%x\n", ctx->status.integration_time);
	fprintf(out, "  +- lamp_enable:............. 0x%x\n", ctx->status.lamp_enable);
	fprintf(out, "  +- trigger_mode:............ 0x%x\n", ctx->status.trigger_mode);
	fprintf(out, "  +- request_spectrum:........ 0x%x\n", ctx->status.request_spectrum);
	fprintf(out, "  +- specral_data_ready:...... 0x%x\n", ctx->status.specral_data_ready);
	fprintf(out, "  +- power_state:............. 0x%x\n", ctx->status.power_state);
	fprintf(out, "  +- spectral_data_counter:... 0x%x\n", ctx->status.spectral_data_counter);
	fprintf(out, "  +- detector_gain_select:.... 0x%x\n", ctx->status.detector_gain_select);
	fprintf(out, "  `- fan_and_tec_state:....... 0x%x\n", ctx->status.fan_and_tec_state);

	return 0;
}

api_public
int ocean_get_serial(struct ocean *ctx, char *buf, size_t len)
{
	if (!ctx || !buf)
		return -EINVAL;

	snprintf(buf, len, "NQ51DUMMY");
	return 0;
}

api_public
int ocean_get_temperature(struct ocean *ctx, float *pcb, float *sink)
{
	if (!ctx || !pcb || !sink)
		return -EINVAL;

	srand(time(NULL));

	*pcb = (float) (rand() % 40);
	*sink = (float) (rand() % 40);

	return 0;
}

api_public
int ocean_set_integration_time(struct ocean *ctx, uint32_t time)
{
	if (!ctx)
		return -EINVAL;

	ctx->status.integration_time = (uint16_t)time;
	return 0;
}

api_public
int ocean_get_integration_time(struct ocean *ctx, uint32_t *time)
{
	if (!ctx || !time)
		return -EINVAL;

	*time = (uint32_t)ctx->status.integration_time;
	return 0;
}

api_public
int ocean_enable_strob(struct ocean *ctx, bool enable)
{
	if (!ctx)
		return -EINVAL;

	ctx->status.lamp_enable = enable ? 0x1 : 0x0;
	return 0;
}

api_public
int ocean_enable_fan(struct ocean *ctx, bool enable)
{
	if (!ctx)
		return -EINVAL;

	if (enable)
		ctx->status.fan_and_tec_state |= 0x01;
	else
		ctx->status.fan_and_tec_state &= ~0x01;
	return 0;
}

api_public
int ocean_enable_external_trigger(struct ocean *ctx, bool enable)
{
	if (!ctx)
		return -EINVAL;

	if (enable)
		ctx->status.trigger_mode = 0x03;
	else
		ctx->status.trigger_mode = 0x00;
	return 0;
}

api_public
int ocean_request_spectra(struct ocean *ctx, struct ocean_spectra *spec)
{
	const double *data;
	const uint8_t *raw;

	if (!ctx || !spec)
		return -EINVAL;

	if (ctx->status.spectral_data_counter % 2) {
		raw = raw1;
		data = spectrum1;
	} else {
		raw = raw2;
		data = spectrum2;
	}

	memcpy(spec->raw, raw, spec->raw_size * sizeof(*raw));
	memcpy(spec->data, data, spec->data_size * sizeof(*data));

	ctx->status.spectral_data_counter++;
	return 0;
}

api_public
int ocean_stop_spectral_acquisition(struct ocean *ctx)
{
	if (!ctx)
		return -EINVAL;

	ctx->status.spectral_data_counter = 0;
	return 0;
}

api_public
int ocean_get_num_of_pixel(struct ocean *ctx, uint32_t *num_of_pixel)
{
	if (!ctx || !num_of_pixel)
		return -EINVAL;

	*num_of_pixel = (uint32_t)ctx->status.num_of_pixels;
	return 0;
}

api_public
int ocean_get_version(int *major, int *minor, int *patch)
{
	if (!major || !minor || !patch)
		return -EINVAL;

	return sscanf(PACKAGE_VERSION, "%d.%d.%d", major, minor, patch);
}
