// LGPL-2.1 License
// NEON SIMD Optimizations Header

#ifndef NEON_OPT_H
#define NEON_OPT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __ARM_NEON__
    #define USE_NEON 1
    #include <arm_neon.h>
#else
    #define USE_NEON 0
#endif

// RGB565 to RGB888 conversion - vectorized version
// Converts multiple RGB565 pixels to RGB888 format at once
// Parameters:
//   src: source array of RGB565 pixels
//   dst: destination array for RGB888 pixels (32-bit with alpha)
//   count: number of pixels to convert
void rgb565_to_rgb888_neon(const uint16_t *src, uint32_t *dst, size_t count);

// Alpha blending for UI overlays
// Blends source pixels over destination with specified alpha value
// Parameters:
//   dst: destination pixel array (modified in-place)
//   src: source pixel array
//   alpha: alpha value (0-255)
//   count: number of pixels to blend
void alpha_blend_neon(uint32_t *dst, const uint32_t *src, uint8_t alpha, size_t count);

// Fast memory fill operation
// Parameters:
//   dst: destination memory
//   value: byte value to fill
//   size: number of bytes to fill
void memset_neon(void *dst, uint8_t value, size_t size);

// Audio mixing with saturation
// Mixes two audio streams with saturation to prevent overflow
// Parameters:
//   dst: destination audio buffer (modified in-place)
//   src: source audio buffer to mix in
//   samples: number of samples to mix
void mix_audio_neon(int16_t *dst, const int16_t *src, size_t samples);

#endif // NEON_OPT_H
