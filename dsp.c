/*
 * Copyright (c) 2014-2015, TAKAHASHI Tomohiro (TTRFTECH) edy555@gmail.com
 * All rights reserved.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * The software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef __USE_CMSIS
#include "LPC43xx.h"
#endif

#include <cr_section_macros.h>
#include <limits.h>
#include <arm_math.h>
#include "lpc43xx_i2s.h"
#include "receiver.h"

#define NCO_CYCLE 1024
#define NCO_SAMPLES 1024
#define NCO_COS_OFFSET (NCO_CYCLE/4)

__RAMFUNC(RAM)
void nco_set_frequency(float32_t freq)
{
	int16_t *costbl = NCO_SIN_TABLE;
	int16_t *sintbl = NCO_COS_TABLE;
	int f;
	int i;

	freq -= (int)(freq / ADC_RATE) * ADC_RATE;
	f = (int)(freq / ADC_RATE * NCO_CYCLE);
	for (i = 0; i < NCO_SAMPLES; i++) {
		float32_t phase = 2*PI*f*(i+0.5)/NCO_CYCLE;
		costbl[i] = (int16_t)(arm_cos_f32(phase) * NCO_AMPL);
		sintbl[i] = (int16_t)(arm_sin_f32(phase) * NCO_AMPL);
	}
}

#define FIR_NUM_TAPS			32

q15_t fir_coeff[FIR_NUM_TAPS] = {
#if 0
			 // bw=156kHz, flat
			 -59,    -5,    96,    24,  -208,   -82,   411,   213,  -732,
			-480,  1240,  1027, -2176, -2424,  5158, 14351, 14351,  5158,
		   -2424, -2176,  1027,  1240,  -480,  -732,   213,   411,   -82,
			-208,    24,    96,    -5,   -59
#else
			// fc=120kHz
			  10,   -60,   -73,    51,   213,    96,  -341,  -502,   175,
			1061,   640, -1323, -2434,   289,  6566, 11984, 11984,  6566,
			 289, -2434, -1323,   640,  1061,   175,  -502,  -341,    96,
			 213,    51,   -73,   -60,    10
#endif
};


__RAMFUNC(RAM)
static uint32_t compute_adc_dc_offset(uint8_t *buf, int len)
{
	uint16_t *const capture = (uint16_t*)buf;
	int count = len / sizeof(uint16_t);
	uint32_t acc = 0;
	uint16_t offset;
	int i = 0;
	for (i = 0; i < count; i++)
		acc += capture[i];
	offset = acc / count;
	// place same 2 offset values on uint32_t
	return __PKHBT(offset, offset, 16);
}


void
update_adc_dc_offset(void)
{
	uint32_t offset = compute_adc_dc_offset(CAPTUREBUFFER0, CAPTUREBUFFER_SIZEHALF);
	cic_i.dc_offset = offset;
	cic_q.dc_offset = offset;
}



struct {
	int32_t index;
} resample_state;

volatile struct {
	uint16_t write_current;
	uint16_t write_total;
	uint16_t read_total;
	uint16_t read_current;
	uint16_t rebuffer_count;
} audio_state;


void generate_test_tone(int freq)
{
	int i;
	int16_t *buf = (int16_t*)AUDIO_BUFFER;
	int samples = AUDIO_BUFFER_SIZE / 2;
	int n = freq * samples / 48000;
	for (i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
		float res = arm_sin_f32(((float)i * 2.0 * PI * n) / 4096);
		buf[i] = (int)(res * 1500.0) + 1500;
	}
}


__RAMFUNC(RAM)
void I2S0_IRQHandler()
{
#if 1
	uint32_t txLevel = I2S_GetLevel(LPC_I2S0, I2S_TX_MODE);
	//TESTPOINT_ON();
	if (txLevel < 8) {
		// Fill the remaining FIFO
		int cur = audio_state.read_current;
		int16_t *buffer = (int16_t*)AUDIO_BUFFER;
		int i;
		for (i = 0; i < (8 - txLevel); i++) {
			uint32_t x = *(uint32_t *)&buffer[cur]; // read TWO samples
			LPC_I2S0->TXFIFO = x;//__PKHTB(x, x, 0);
			cur += 2;
			cur %= AUDIO_BUFFER_SIZE / 2;
			audio_state.read_total += 2;
		}
		audio_state.read_current = cur;
	}
	//TESTPOINT_OFF();
#endif
}

