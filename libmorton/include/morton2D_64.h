// Libmorton - Methods to encode/decode 64-bit morton codes from/to 32-bit (x,y,z) coordinates
#ifndef MORTON2D_64_H_
#define MORTON2D_64_H_

#include <stdint.h>
#include "morton2D_LUTs.h"

#if _MSC_VER
#include <intrin.h>
#endif

template<typename morton, typename coord> inline morton morton2D_Encode_for(const coord x, const coord y);
template<typename morton, typename coord> inline morton morton2D_Encode_magicbits(const coord x, const coord y);

// ENCODE 2D 64-bit morton code : For Loop
template<typename morton, typename coord>
inline morton morton2D_Encode_for(const coord x, const coord y){
	morton answer = 0;
	unsigned long checkbits = sizeof(coord)*8;
	for (morton i = 0; i < checkbits; ++i) {
		answer |= (x & ((morton)0x1 << i)) << (2 * i) | (y & ((morton)0x1 << i)) << ((2 * i) + 1);
	}
	return answer;
}

// ENCODE 3D 64-bit morton code : Magic bits (helper method)
static uint_fast32_t encode2D_masks32[6] = { 0, 0x0000FFFF, 0x00FF00FF, 0x0F0F0F0F, 0x33333333, 0x55555555};
static uint_fast64_t encode2D_masks64[6] = { 0x00000000FFFFFFFF, 0x0000FFFF0000FFFF, 0x00FF00FF00FF00FF, 
											0x0F0F0F0F0F0F0F0F, 0x3333333333333333, 0x5555555555555555};

// ENCODE 2D 64-bit morton code : Magic bits (helper method)
template<typename morton, typename coord>
inline morton morton2D_splitby2(const coord a){
	morton* masks = (sizeof(morton) <= 4) ? reinterpret_cast<morton*>(encode2D_masks32) : reinterpret_cast<morton*>(encode2D_masks64);
	morton x = a;
	if (sizeof(morton) > 4) { x = (x | x << 32) & masks[0]; }
	x = (x | x << 16) & masks[1];
	x = (x | x << 8) & masks[2];
	x = (x | x << 4) & masks[3];
	x = (x | x << 2) & masks[4];
	x = (x | x << 1) & masks[5];
	return x;
}

// ENCODE 2D 64-bit morton code : Magic bits
template<typename morton, typename coord>
inline morton morton2D_Encode_magicbits(const coord x, const coord y){
	return morton2D_splitby2<morton, coord>(x) | (morton2D_splitby2<morton, coord>(y) << 1);
}

// ENCODE 2D 64-bit morton code: LUT preshifted
inline uint_fast64_t morton2D_64_Encode_LUT256_shifted(const uint_fast32_t x, const uint_fast32_t y){
	uint_fast64_t answer =
		Morton2D_64_encode_y_256[(y >> 24) & 0x000000FF] | // select 8 upper bits
		Morton2D_64_encode_x_256[(x >> 24) & 0x000000FF];
	answer = answer << 16 | // shift by 16 = 2 * 8bits
		Morton2D_64_encode_y_256[(y >> 16) & 0x000000FF] | // select next 8 bits
		Morton2D_64_encode_x_256[(x >> 16) & 0x000000FF];
	answer = answer << 16 | // shift by 16 = 2 * 8bits
		Morton2D_64_encode_y_256[(y >> 8) & 0x000000FF] | // select next 8 bits
		Morton2D_64_encode_x_256[(x >> 8) & 0x000000FF];
	answer = answer << 16 | // shift by 16 = 2 * 8bits
		Morton2D_64_encode_y_256[(y) & 0x000000FF] | // select next 8 bits
		Morton2D_64_encode_x_256[(x) & 0x000000FF];
	return answer;
}

// ENCODE 2D 64-bit morton code: LUT preshifted
inline uint_fast64_t morton2D_64_Encode_LUT256(const uint_fast32_t x, const uint_fast32_t y) {
	uint_fast64_t answer =
		(Morton2D_64_encode_x_256[(y >> 24) & 0x000000FF]) << 1 | // select 8 upper bits
		Morton2D_64_encode_x_256[(x >> 24) & 0x000000FF];
	answer = answer << 16 | // shift by 16 = 2 * 8bits
		(Morton2D_64_encode_x_256[(y >> 16) & 0x000000FF]) << 1 | // select next 8 bits
		Morton2D_64_encode_x_256[(x >> 16) & 0x000000FF];
	answer = answer << 16 | // shift by 16 = 2 * 8bits
		(Morton2D_64_encode_x_256[(y >> 8) & 0x000000FF]) << 1 | // select next 8 bits
		Morton2D_64_encode_x_256[(x >> 8) & 0x000000FF];
	answer = answer << 16 | // shift by 16 = 2 * 8bits
		(Morton2D_64_encode_x_256[(y)& 0x000000FF]) << 1 | // select next 8 bits
		Morton2D_64_encode_x_256[(x)& 0x000000FF];
	return answer;
}

// DECODE 2D morton code : For loop
template<typename morton, typename coord>
inline void morton2D_Decode_for(const morton morton, coord& x, coord& y) {
	x = 0; y = 0;
	for (uint_fast64_t i = 0; i < (sizeof(morton)*8); i++) {
		x |= (morton & (1ull << 2 * i)) >> i;
		y |= (morton & (1ull << ((2 * i) + 1))) >> (i + 1);
	}
}

#endif // MORTON2D_64_H_