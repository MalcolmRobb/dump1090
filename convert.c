// Part of dump1090, a Mode S message decoder for RTLSDR devices.
//
// convert.c: support for various IQ -> magnitude conversions
//
// Copyright (c) 2015 Oliver Jowett <oliver@mutability.co.uk>
//
// This file is free software: you may copy, redistribute and/or modify it  
// under the terms of the GNU General Public License as published by the
// Free Software Foundation, either version 2 of the License, or (at your  
// option) any later version.  
//
// This file is distributed in the hope that it will be useful, but  
// WITHOUT ANY WARRANTY; without even the implied warranty of  
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License  
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "dump1090.h"

struct converter_state {
    float dc_a;
    float dc_b;
    float z1_I;
    float z1_Q;
};

static void convert_uc8_nodc_nopower(void *iq_data,
                                     uint16_t *mag_data,
                                     unsigned nsamples,
                                     struct converter_state *state,
                                     double *out_power)
{
    uint16_t *in = iq_data;
    unsigned i;

    MODES_NOTUSED(state);

    // unroll this a bit
    for (i = 0; i < (nsamples>>3); ++i) {
        *mag_data++ = Modes.maglut[*in++];
        *mag_data++ = Modes.maglut[*in++];
        *mag_data++ = Modes.maglut[*in++];
        *mag_data++ = Modes.maglut[*in++];
        *mag_data++ = Modes.maglut[*in++];
        *mag_data++ = Modes.maglut[*in++];
        *mag_data++ = Modes.maglut[*in++];
        *mag_data++ = Modes.maglut[*in++];
    }

    for (i = 0; i < (nsamples&7); ++i) {
        *mag_data++ = Modes.maglut[*in++];
    }

    if (out_power)
        *out_power = 0.0; // not measured
}

static void convert_uc8_nodc_power(void *iq_data,
                                   uint16_t *mag_data,
                                   unsigned nsamples,
                                   struct converter_state *state,
                                   double *out_power)
{
    uint16_t *in = iq_data;
    unsigned i;
    uint64_t power = 0;
    uint16_t mag;

    MODES_NOTUSED(state);

    // unroll this a bit
    for (i = 0; i < (nsamples>>3); ++i) {
        mag = Modes.maglut[*in++];
        *mag_data++ = mag;
        power += (uint32_t)mag * (uint32_t)mag;

        mag = Modes.maglut[*in++];
        *mag_data++ = mag;
        power += (uint32_t)mag * (uint32_t)mag;

        mag = Modes.maglut[*in++];
        *mag_data++ = mag;
        power += (uint32_t)mag * (uint32_t)mag;

        mag = Modes.maglut[*in++];
        *mag_data++ = mag;
        power += (uint32_t)mag * (uint32_t)mag;

        mag = Modes.maglut[*in++];
        *mag_data++ = mag;
        power += (uint32_t)mag * (uint32_t)mag;

        mag = Modes.maglut[*in++];
        *mag_data++ = mag;
        power += (uint32_t)mag * (uint32_t)mag;

        mag = Modes.maglut[*in++];
        *mag_data++ = mag;
        power += (uint32_t)mag * (uint32_t)mag;

        mag = Modes.maglut[*in++];
        *mag_data++ = mag;
        power += (uint32_t)mag * (uint32_t)mag;
    }

    for (i = 0; i < (nsamples&7); ++i) {
        mag = Modes.maglut[*in++];
        *mag_data++ = mag;
        power += (uint32_t)mag * (uint32_t)mag;
    }

    if (out_power) {
        *out_power = power / 65535.0 / 65535.0;
    }
}

static void convert_uc8_generic(void *iq_data,
                                uint16_t *mag_data,
                                unsigned nsamples,
                                struct converter_state *state,
                                double *out_power)
{
    uint8_t *in = iq_data;
    float power = 0.0;
    float z1_I = state->z1_I;
    float z1_Q = state->z1_Q;
    const float dc_a = state->dc_a;
    const float dc_b = state->dc_b;

    unsigned i;
    uint8_t I, Q;
    float fI, fQ, magsq;

    for (i = 0; i < nsamples; ++i) {
        I = *in++;
        Q = *in++;
        fI = (I - 127.5) / 127.5;
        fQ = (Q - 127.5) / 127.5;

        // DC block
        z1_I = fI * dc_a + z1_I * dc_b;
        z1_Q = fQ * dc_a + z1_Q * dc_b;
        fI -= z1_I;
        fQ -= z1_Q;

        magsq = fI * fI + fQ * fQ;
        if (magsq > 1)
            magsq = 1;

        power += magsq;
        *mag_data++ = (uint16_t)(sqrtf(magsq) * 65535.0 + 0.5);
    }

    state->z1_I = z1_I;
    state->z1_Q = z1_Q;

    if (out_power)
        *out_power = power;
}

static void convert_sc16_generic(void *iq_data,
                                 uint16_t *mag_data,
                                 unsigned nsamples,
                                 struct converter_state *state,
                                 double *out_power)
{
    uint16_t *in = iq_data;
    float power = 0.0;
    float z1_I = state->z1_I;
    float z1_Q = state->z1_Q;
    const float dc_a = state->dc_a;
    const float dc_b = state->dc_b;

    unsigned i;
    int16_t I, Q;
    float fI, fQ, magsq;

    for (i = 0; i < nsamples; ++i) {
        I = (int16_t)le16toh(*in++);
        Q = (int16_t)le16toh(*in++);
        fI = I / 32768.0;
        fQ = Q / 32768.0;

        // DC block
        z1_I = fI * dc_a + z1_I * dc_b;
        z1_Q = fQ * dc_a + z1_Q * dc_b;
        fI -= z1_I;
        fQ -= z1_Q;

        magsq = fI * fI + fQ * fQ;
        if (magsq > 1)
            magsq = 1;

        power += magsq;
        *mag_data++ = (uint16_t)(sqrtf(magsq) * 65535.0 + 0.5);
    }

    state->z1_I = z1_I;
    state->z1_Q = z1_Q;

    if (out_power)
        *out_power = power;
}

static void convert_sc16q11_generic(void *iq_data,
                                    uint16_t *mag_data,
                                    unsigned nsamples,
                                    struct converter_state *state,
                                    double *out_power)
{
    uint16_t *in = iq_data;
    float power = 0.0;
    float z1_I = state->z1_I;
    float z1_Q = state->z1_Q;
    const float dc_a = state->dc_a;
    const float dc_b = state->dc_b;

    unsigned i;
    int16_t I, Q;
    float fI, fQ, magsq;

    for (i = 0; i < nsamples; ++i) {
        I = (int16_t)le16toh(*in++);
        Q = (int16_t)le16toh(*in++);
        fI = I / 2048.0;
        fQ = Q / 2048.0;

        // DC block
        z1_I = fI * dc_a + z1_I * dc_b;
        z1_Q = fQ * dc_a + z1_Q * dc_b;
        fI -= z1_I;
        fQ -= z1_Q;

        magsq = fI * fI + fQ * fQ;
        if (magsq > 1)
            magsq = 1;

        power += magsq;
        *mag_data++ = (uint16_t)(sqrtf(magsq) * 65535.0 + 0.5);
    }

    state->z1_I = z1_I;
    state->z1_Q = z1_Q;

    if (out_power)
        *out_power = power;
}

static struct {
    input_format_t format;
    int can_filter_dc;
    int can_compute_power;
    iq_convert_fn fn;
    const char *description;
} converters_table[] = {
    // In order of preference
    { INPUT_UC8,     0, 0, convert_uc8_nodc_nopower, "UC8, integer/table path" },
    { INPUT_UC8,     0, 1, convert_uc8_nodc_power,   "UC8, integer/table path, with power measurement" },
    { INPUT_UC8,     1, 1, convert_uc8_generic,      "UC8, float path" },
    { INPUT_SC16,    1, 1, convert_sc16_generic,     "SC16, float path" },
    { INPUT_SC16Q11, 1, 1, convert_sc16q11_generic,  "SC16Q11, float path" },
    { 0, 0, 0, NULL, NULL }
};

iq_convert_fn init_converter(input_format_t format,
                             double sample_rate,
                             int filter_dc,
                             int compute_power,
                             struct converter_state **out_state)
{
    int i;

    for (i = 0; converters_table[i].fn; ++i) {
        if (converters_table[i].format != format)
            continue;
        if (filter_dc && !converters_table[i].can_filter_dc)
            continue;
        if (compute_power && !converters_table[i].can_compute_power)
            continue;
        break;
    }

    if (!converters_table[i].fn) {
        fprintf(stderr, "no suitable converter for format=%d power=%d dc=%d\n",
                format, compute_power, filter_dc);
        return NULL;
    }

    fprintf(stderr, "Using sample converter: %s\n", converters_table[i].description);

    *out_state = malloc(sizeof(struct converter_state));
    if (! *out_state) {
        fprintf(stderr, "can't allocate converter state\n");
        return NULL;
    }

    (*out_state)->z1_I = 0;
    (*out_state)->z1_Q = 0;

    if (filter_dc) {
        // init DC block @ 1Hz
        (*out_state)->dc_b = exp(-2.0 * M_PI * 1.0 / sample_rate);
        (*out_state)->dc_a = 1.0 - (*out_state)->dc_b;
    } else {
        // if the converter does filtering, make sure it has no effect
        (*out_state)->dc_b = 1.0;
        (*out_state)->dc_a = 0.0;
    }

    return converters_table[i].fn;
}

void cleanup_converter(struct converter_state *state)
{
    free(state);
}
