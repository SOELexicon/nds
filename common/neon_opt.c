// LGPL-2.1 License
// NEON SIMD Optimizations Implementation

#include <string.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

// Forward declaration of the scalar function
uint32_t rgb565_to_rgb888(uint16_t c);

// RGB565 to RGB888 conversion - vectorized version
void rgb565_to_rgb888_neon(const uint16_t *src, uint32_t *dst, size_t count)
{
#ifdef __ARM_NEON__
    size_t i;
    
    // Process 8 pixels at a time with NEON
    for (i = 0; i + 8 <= count; i += 8) {
        // Load 8 RGB565 pixels
        uint16x8_t v565 = vld1q_u16(src + i);
        
        // Extract R, G, B channels using masks
        // RGB565 format: RRRRRGGGGGGBBBBB
        uint16x8_t r = vandq_u16(v565, vdupq_n_u16(0xF800));  // Red: bits 11-15
        uint16x8_t g = vandq_u16(v565, vdupq_n_u16(0x07E0));  // Green: bits 5-10
        uint16x8_t b = vandq_u16(v565, vdupq_n_u16(0x001F));  // Blue: bits 0-4
        
        // Shift to align for 8-bit conversion
        r = vshrq_n_u16(r, 8);   // Shift right 8 (from bit 11 to bit 3)
        g = vshrq_n_u16(g, 3);   // Shift right 3 (from bit 5 to bit 2)
        b = vshlq_n_u16(b, 3);   // Shift left 3 (from bit 0 to bit 3)
        
        // Convert from 5/6-bit to 8-bit by replicating high bits
        // For 5-bit: xxx00 -> xxx000xx (replicate top 2 bits)
        // For 6-bit: xxxxxx00 -> xxxxxxxxxx (replicate top 2 bits)
        uint16x8_t r_expand = vshrq_n_u16(r, 5);  // Get top 2 bits
        uint16x8_t g_expand = vshrq_n_u16(g, 6);  // Get top 2 bits
        uint16x8_t b_expand = vshrq_n_u16(b, 5);  // Get top 2 bits
        
        r = vorrq_u16(r, r_expand);
        g = vorrq_u16(g, g_expand);
        b = vorrq_u16(b, b_expand);
        
        // Narrow to 8-bit
        uint8x8_t r8 = vmovn_u16(r);
        uint8x8_t g8 = vmovn_u16(g);
        uint8x8_t b8 = vmovn_u16(b);
        
        // Interleave as RGBA (with alpha = 0xFF)
        uint8x8x4_t rgba;
        rgba.val[0] = r8;
        rgba.val[1] = g8;
        rgba.val[2] = b8;
        rgba.val[3] = vdup_n_u8(0xFF);  // Alpha channel
        
        // Store interleaved RGBA pixels
        vst4_u8((uint8_t *)(dst + i), rgba);
    }
    
    // Handle remaining pixels with scalar code
    for (; i < count; i++) {
        dst[i] = rgb565_to_rgb888(src[i]);
    }
#else
    // Fallback for non-NEON platforms
    for (size_t i = 0; i < count; i++) {
        dst[i] = rgb565_to_rgb888(src[i]);
    }
#endif
}

// Alpha blending for UI overlays
void alpha_blend_neon(uint32_t *dst, const uint32_t *src, uint8_t alpha, size_t count)
{
#ifdef __ARM_NEON__
    if (alpha == 0) {
        return;  // No blending needed
    }
    if (alpha == 255) {
        // Full opacity - just copy
        memcpy(dst, src, count * sizeof(uint32_t));
        return;
    }
    
    uint8x8_t va = vdup_n_u8(alpha);
    uint8x8_t va_inv = vdup_n_u8(255 - alpha);
    
    size_t i;
    for (i = 0; i + 8 <= count; i += 8) {
        // Load source and destination pixels in RGBA format
        uint8x8x4_t src_rgba = vld4_u8((const uint8_t *)(src + i));
        uint8x8x4_t dst_rgba = vld4_u8((const uint8_t *)(dst + i));
        
        // Blend each channel: result = (src * alpha + dst * (255-alpha)) / 255
        // Using multiply-accumulate for efficiency
        
        // Red channel
        uint16x8_t r = vmull_u8(src_rgba.val[0], va);
        r = vmlal_u8(r, dst_rgba.val[0], va_inv);
        dst_rgba.val[0] = vshrn_n_u16(r, 8);
        
        // Green channel
        uint16x8_t g = vmull_u8(src_rgba.val[1], va);
        g = vmlal_u8(g, dst_rgba.val[1], va_inv);
        dst_rgba.val[1] = vshrn_n_u16(g, 8);
        
        // Blue channel
        uint16x8_t b = vmull_u8(src_rgba.val[2], va);
        b = vmlal_u8(b, dst_rgba.val[2], va_inv);
        dst_rgba.val[2] = vshrn_n_u16(b, 8);
        
        // Alpha channel (blend alpha values too)
        uint16x8_t a = vmull_u8(src_rgba.val[3], va);
        a = vmlal_u8(a, dst_rgba.val[3], va_inv);
        dst_rgba.val[3] = vshrn_n_u16(a, 8);
        
        // Store blended pixels
        vst4_u8((uint8_t *)(dst + i), dst_rgba);
    }
    
    // Handle remaining pixels with scalar code
    for (; i < count; i++) {
        uint8_t *d = (uint8_t *)&dst[i];
        const uint8_t *s = (const uint8_t *)&src[i];
        
        for (int c = 0; c < 4; c++) {
            d[c] = (s[c] * alpha + d[c] * (255 - alpha)) / 255;
        }
    }
#else
    // Fallback for non-NEON platforms
    for (size_t i = 0; i < count; i++) {
        uint8_t *d = (uint8_t *)&dst[i];
        const uint8_t *s = (const uint8_t *)&src[i];
        
        for (int c = 0; c < 4; c++) {
            d[c] = (s[c] * alpha + d[c] * (255 - alpha)) / 255;
        }
    }
#endif
}

// Fast memory fill operation
void memset_neon(void *dst, uint8_t value, size_t size)
{
#ifdef __ARM_NEON__
    uint8_t *d = (uint8_t *)dst;
    
    // Create vector with the fill value
    uint8x16_t v = vdupq_n_u8(value);
    
    size_t i;
    // Process 64 bytes at a time for maximum throughput
    for (i = 0; i + 64 <= size; i += 64) {
        vst1q_u8(d + i, v);
        vst1q_u8(d + i + 16, v);
        vst1q_u8(d + i + 32, v);
        vst1q_u8(d + i + 48, v);
    }
    
    // Process remaining 16-byte chunks
    for (; i + 16 <= size; i += 16) {
        vst1q_u8(d + i, v);
    }
    
    // Handle remaining bytes with standard memset
    if (i < size) {
        memset(d + i, value, size - i);
    }
#else
    // Fallback for non-NEON platforms
    memset(dst, value, size);
#endif
}

// Audio mixing with saturation
void mix_audio_neon(int16_t *dst, const int16_t *src, size_t samples)
{
#ifdef __ARM_NEON__
    size_t i;
    
    // Process 8 samples at a time
    for (i = 0; i + 8 <= samples; i += 8) {
        // Load destination and source samples
        int16x8_t v_dst = vld1q_s16(dst + i);
        int16x8_t v_src = vld1q_s16(src + i);
        
        // Saturating add prevents overflow/underflow
        int16x8_t v_mix = vqaddq_s16(v_dst, v_src);
        
        // Store mixed samples
        vst1q_s16(dst + i, v_mix);
    }
    
    // Handle remaining samples with scalar code
    for (; i < samples; i++) {
        int32_t mixed = (int32_t)dst[i] + (int32_t)src[i];
        
        // Manual saturation
        if (mixed > 32767) {
            dst[i] = 32767;
        } else if (mixed < -32768) {
            dst[i] = -32768;
        } else {
            dst[i] = (int16_t)mixed;
        }
    }
#else
    // Fallback for non-NEON platforms
    for (size_t i = 0; i < samples; i++) {
        int32_t mixed = (int32_t)dst[i] + (int32_t)src[i];
        
        // Manual saturation
        if (mixed > 32767) {
            dst[i] = 32767;
        } else if (mixed < -32768) {
            dst[i] = -32768;
        } else {
            dst[i] = (int16_t)mixed;
        }
    }
#endif
}
