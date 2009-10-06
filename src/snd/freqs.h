#ifndef FREQS_H
#define FREQS_H

/*
Copyright (c) 2009 Tero Lindeman (kometbomb)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/


const Sint8 vibrato_table[] =
{
      0,
      6,
      12,
      18,
      24,
      31,
      37,
      43,
      48,
      54,
      60,
      65,
      71,
      76,
      81,
      85,
      90,
      94,
      98,
      102,
      106,
      109,
      112,
      115,
      118,
      120,
      122,
      124,
      125,
      126,
      127,
      127,
      127,
      127,
      127,
      126,
      125,
      124,
      122,
      120,
      118,
      115,
      112,
      109,
      106,
      102,
      98,
      94,
      90,
      85,
      81,
      76,
      71,
      65,
      60,
      54,
      48,
      43,
      37,
      31,
      24,
      18,
      12,
      6,
      0,
      -6,
      -12,
      -18,
      -24,
      -31,
      -37,
      -43,
      -48,
      -54,
      -60,
      -65,
      -71,
      -76,
      -81,
      -85,
      -90,
      -94,
      -98,
      -102,
      -106,
      -109,
      -112,
      -115,
      -118,
      -120,
      -122,
      -124,
      -125,
      -126,
      -127,
      -127,
      -128,
      -127,
      -127,
      -126,
      -125,
      -124,
      -122,
      -120,
      -118,
      -115,
      -112,
      -109,
      -106,
      -102,
      -98,
      -94,
      -90,
      -85,
      -81,
      -76,
      -71,
      -65,
      -60,
      -54,
      -48,
      -43,
      -37,
      -31,
      -24,
      -18,
      -12,
      -6
};

const Uint16 frequency_table[] = 
{
  (Uint16)(16.35 * 16),
  (Uint16)(17.32 * 16),
  (Uint16)(18.35 * 16),
  (Uint16)(19.45 * 16),
  (Uint16)(20.60 * 16),
  (Uint16)(21.83 * 16),
  (Uint16)(23.12 * 16),
  (Uint16)(24.50 * 16),
  (Uint16)(25.96 * 16),
  (Uint16)(27.50 * 16),
  (Uint16)(29.14 * 16),
  (Uint16)(30.87 * 16),
  (Uint16)(32.70 * 16),
  (Uint16)(34.65 * 16),
  (Uint16)(36.71 * 16),
  (Uint16)(38.89 * 16),
  (Uint16)(41.20 * 16),
  (Uint16)(43.65 * 16),
  (Uint16)(46.25 * 16),
  (Uint16)(49.00 * 16),
  (Uint16)(51.91 * 16),
  (Uint16)(55.00 * 16),
  (Uint16)(58.27 * 16),
  (Uint16)(61.74 * 16),
  (Uint16)(65.41 * 16),
  (Uint16)(69.30 * 16),
  (Uint16)(73.42 * 16),
  (Uint16)(77.78 * 16),
  (Uint16)(82.41 * 16),
  (Uint16)(87.31 * 16),
  (Uint16)(92.50 * 16),
  (Uint16)(98.00 * 16),
  (Uint16)(103.83 * 16),
  (Uint16)(110.00 * 16),
  (Uint16)(116.54 * 16),
  (Uint16)(123.47 * 16),
  (Uint16)(130.81 * 16),
  (Uint16)(138.59 * 16),
  (Uint16)(146.83 * 16),
  (Uint16)(155.56 * 16),
  (Uint16)(164.81 * 16),
  (Uint16)(174.61 * 16),
  (Uint16)(185.00 * 16),
  (Uint16)(196.00 * 16),
  (Uint16)(207.65 * 16),
  (Uint16)(220.00 * 16),
  (Uint16)(233.08 * 16),
  (Uint16)(246.94 * 16),
  (Uint16)(261.63 * 16),
  (Uint16)(277.18 * 16),
  (Uint16)(293.66 * 16),
  (Uint16)(311.13 * 16),
  (Uint16)(329.63 * 16),
  (Uint16)(349.23 * 16),
  (Uint16)(369.99 * 16),
  (Uint16)(392.00 * 16),
  (Uint16)(415.30 * 16),
  (Uint16)(440.00 * 16),
  (Uint16)(466.16 * 16),
  (Uint16)(493.88 * 16),
  (Uint16)(523.25 * 16),
  (Uint16)(554.37 * 16),
  (Uint16)(587.33 * 16),
  (Uint16)(622.25 * 16),
  (Uint16)(659.26 * 16),
  (Uint16)(698.46 * 16),
  (Uint16)(739.99 * 16),
  (Uint16)(783.99 * 16),
  (Uint16)(830.61 * 16),
  (Uint16)(880.00 * 16),
  (Uint16)(932.33 * 16),
  (Uint16)(987.77 * 16),
  (Uint16)(1046.50 * 16),
  (Uint16)(1108.73 * 16),
  (Uint16)(1174.66 * 16),
  (Uint16)(1244.51 * 16),
  (Uint16)(1318.51 * 16),
  (Uint16)(1396.91 * 16),
  (Uint16)(1479.98 * 16),
  (Uint16)(1567.98 * 16),
  (Uint16)(1661.22 * 16),
  (Uint16)(1760.00 * 16),
  (Uint16)(1864.66 * 16),
  (Uint16)(1975.53 * 16),
  (Uint16)(2093.00 * 16),
  (Uint16)(2217.46 * 16),
  (Uint16)(2349.32 * 16),
  (Uint16)(2489.02 * 16),
  (Uint16)(2637.02 * 16),
  (Uint16)(2793.83 * 16),
  (Uint16)(2959.96 * 16),
  (Uint16)(3135.96 * 16),
  (Uint16)(3322.44 * 16),
  (Uint16)(3520.00 * 16),
  (Uint16)(3729.31 * 16),
  (Uint16)(3951.07 * 16)
};

#define VIB_TAB_SIZE (sizeof(vibrato_table) / sizeof(vibrato_table[0]))
#define FREQ_TAB_SIZE (sizeof(frequency_table) / sizeof(frequency_table[0]))


#endif