/*
 * LZO 1x decompression
 * Copyright (c) 2006 Reimar Doeffinger
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string.h>
#include <limits.h>
#include "lzo.h"

/** @} */
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned long long uint64_tt;

typedef union {
    uint64_tt u64;
    uint32_t u32[2];
    uint16_t u16[4];
    uint8_t  u8 [8];
    double   f64;
    float    f32[2];
}  av_alias64;

typedef union {
    uint32_t u32;
    uint16_t u16[2];
    uint8_t  u8 [4];
    float    f32;
}  av_alias32;

typedef union {
    uint16_t u16;
    uint8_t  u8 [2];
}  av_alias16;

/*
 * Arch-specific headers can provide any combination of
 * AV_[RW][BLN](16|24|32|48|64) and AV_(COPY|SWAP|ZERO)(64|128) macros.
 * Preprocessor symbols must be defined, even if these are implemented
 * as inline functions.
 *
 * R/W means read/write, B/L/N means big/little/native endianness.
 * The following macros require aligned access, compared to their
 * unaligned variants: AV_(COPY|SWAP|ZERO)(64|128), AV_[RW]N[8-64]A.
 * Incorrect usage may range from abysmal performance to crash
 * depending on the platform.
 *
 * The unaligned variants are AV_[RW][BLN][8-64] and AV_COPY*U.
 */

#ifdef HAVE_AV_CONFIG_H

#include "config.h"

#if   ARCH_ARM
#   include "arm/intreadwrite.h"
#elif ARCH_AVR32
#   include "avr32/intreadwrite.h"
#elif ARCH_MIPS
#   include "mips/intreadwrite.h"
#elif ARCH_PPC
#   include "ppc/intreadwrite.h"
#elif ARCH_TOMI
#   include "tomi/intreadwrite.h"
#elif ARCH_X86
#   include "x86/intreadwrite.h"
#endif

#endif /* HAVE_AV_CONFIG_H */

/*
 * Map AV_RNXX <-> AV_R[BL]XX for all variants provided by per-arch headers.
 */

#if AV_HAVE_BIGENDIAN

#   if    defined(AV_RN16) && !defined(AV_RB16)
#       define AV_RB16(p) AV_RN16(p)
#   elif !defined(AV_RN16) &&  defined(AV_RB16)
#       define AV_RN16(p) AV_RB16(p)
#   endif

#   if    defined(AV_WN16) && !defined(AV_WB16)
#       define AV_WB16(p, v) AV_WN16(p, v)
#   elif !defined(AV_WN16) &&  defined(AV_WB16)
#       define AV_WN16(p, v) AV_WB16(p, v)
#   endif

#   if    defined(AV_RN24) && !defined(AV_RB24)
#       define AV_RB24(p) AV_RN24(p)
#   elif !defined(AV_RN24) &&  defined(AV_RB24)
#       define AV_RN24(p) AV_RB24(p)
#   endif

#   if    defined(AV_WN24) && !defined(AV_WB24)
#       define AV_WB24(p, v) AV_WN24(p, v)
#   elif !defined(AV_WN24) &&  defined(AV_WB24)
#       define AV_WN24(p, v) AV_WB24(p, v)
#   endif

#   if    defined(AV_RN32) && !defined(AV_RB32)
#       define AV_RB32(p) AV_RN32(p)
#   elif !defined(AV_RN32) &&  defined(AV_RB32)
#       define AV_RN32(p) AV_RB32(p)
#   endif

#   if    defined(AV_WN32) && !defined(AV_WB32)
#       define AV_WB32(p, v) AV_WN32(p, v)
#   elif !defined(AV_WN32) &&  defined(AV_WB32)
#       define AV_WN32(p, v) AV_WB32(p, v)
#   endif

#   if    defined(AV_RN48) && !defined(AV_RB48)
#       define AV_RB48(p) AV_RN48(p)
#   elif !defined(AV_RN48) &&  defined(AV_RB48)
#       define AV_RN48(p) AV_RB48(p)
#   endif

#   if    defined(AV_WN48) && !defined(AV_WB48)
#       define AV_WB48(p, v) AV_WN48(p, v)
#   elif !defined(AV_WN48) &&  defined(AV_WB48)
#       define AV_WN48(p, v) AV_WB48(p, v)
#   endif

#   if    defined(AV_RN64) && !defined(AV_RB64)
#       define AV_RB64(p) AV_RN64(p)
#   elif !defined(AV_RN64) &&  defined(AV_RB64)
#       define AV_RN64(p) AV_RB64(p)
#   endif

#   if    defined(AV_WN64) && !defined(AV_WB64)
#       define AV_WB64(p, v) AV_WN64(p, v)
#   elif !defined(AV_WN64) &&  defined(AV_WB64)
#       define AV_WN64(p, v) AV_WB64(p, v)
#   endif

#else /* AV_HAVE_BIGENDIAN */

#   if    defined(AV_RN16) && !defined(AV_RL16)
#       define AV_RL16(p) AV_RN16(p)
#   elif !defined(AV_RN16) &&  defined(AV_RL16)
#       define AV_RN16(p) AV_RL16(p)
#   endif

#   if    defined(AV_WN16) && !defined(AV_WL16)
#       define AV_WL16(p, v) AV_WN16(p, v)
#   elif !defined(AV_WN16) &&  defined(AV_WL16)
#       define AV_WN16(p, v) AV_WL16(p, v)
#   endif

#   if    defined(AV_RN24) && !defined(AV_RL24)
#       define AV_RL24(p) AV_RN24(p)
#   elif !defined(AV_RN24) &&  defined(AV_RL24)
#       define AV_RN24(p) AV_RL24(p)
#   endif

#   if    defined(AV_WN24) && !defined(AV_WL24)
#       define AV_WL24(p, v) AV_WN24(p, v)
#   elif !defined(AV_WN24) &&  defined(AV_WL24)
#       define AV_WN24(p, v) AV_WL24(p, v)
#   endif

#   if    defined(AV_RN32) && !defined(AV_RL32)
#       define AV_RL32(p) AV_RN32(p)
#   elif !defined(AV_RN32) &&  defined(AV_RL32)
#       define AV_RN32(p) AV_RL32(p)
#   endif

#   if    defined(AV_WN32) && !defined(AV_WL32)
#       define AV_WL32(p, v) AV_WN32(p, v)
#   elif !defined(AV_WN32) &&  defined(AV_WL32)
#       define AV_WN32(p, v) AV_WL32(p, v)
#   endif

#   if    defined(AV_RN48) && !defined(AV_RL48)
#       define AV_RL48(p) AV_RN48(p)
#   elif !defined(AV_RN48) &&  defined(AV_RL48)
#       define AV_RN48(p) AV_RL48(p)
#   endif

#   if    defined(AV_WN48) && !defined(AV_WL48)
#       define AV_WL48(p, v) AV_WN48(p, v)
#   elif !defined(AV_WN48) &&  defined(AV_WL48)
#       define AV_WN48(p, v) AV_WL48(p, v)
#   endif

#   if    defined(AV_RN64) && !defined(AV_RL64)
#       define AV_RL64(p) AV_RN64(p)
#   elif !defined(AV_RN64) &&  defined(AV_RL64)
#       define AV_RN64(p) AV_RL64(p)
#   endif

#   if    defined(AV_WN64) && !defined(AV_WL64)
#       define AV_WL64(p, v) AV_WN64(p, v)
#   elif !defined(AV_WN64) &&  defined(AV_WL64)
#       define AV_WN64(p, v) AV_WL64(p, v)
#   endif

#endif /* !AV_HAVE_BIGENDIAN */

/*
 * Define AV_[RW]N helper macros to simplify definitions not provided
 * by per-arch headers.
 */

#if defined(__GNUC__) && !defined(__TI_COMPILER_VERSION__)

union unaligned_64 { uint64_tt l; };
union unaligned_32 { uint32_t l; };
union unaligned_16 { uint16_t l; };

#   define AV_RN(s, p) (((const union unaligned_##s *) (p))->l)
#   define AV_WN(s, p, v) ((((union unaligned_##s *) (p))->l) = (v))

#elif defined(__DECC)

#   define AV_RN(s, p) (*((const __unaligned uint##s##_t*)(p)))
#   define AV_WN(s, p, v) (*((__unaligned uint##s##_t*)(p)) = (v))

#elif AV_HAVE_FAST_UNALIGNED

#   define AV_RN(s, p) (((const av_alias##s*)(p))->u##s)
#   define AV_WN(s, p, v) (((av_alias##s*)(p))->u##s = (v))

#else

#ifndef AV_RB16
#   define AV_RB16(x)                           \
    ((((const uint8_t*)(x))[0] << 8) |          \
      ((const uint8_t*)(x))[1])
#endif
#ifndef AV_WB16
#   define AV_WB16(p, darg) do {                \
        unsigned d = (darg);                    \
        ((uint8_t*)(p))[1] = (d);               \
        ((uint8_t*)(p))[0] = (d)>>8;            \
    } while(0)
#endif

#ifndef AV_RL16
#   define AV_RL16(x)                           \
    ((((const uint8_t*)(x))[1] << 8) |          \
      ((const uint8_t*)(x))[0])
#endif
#ifndef AV_WL16
#   define AV_WL16(p, darg) do {                \
        unsigned d = (darg);                    \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
    } while(0)
#endif

#ifndef AV_RB32
#   define AV_RB32(x)                                \
    (((uint32_t)((const uint8_t*)(x))[0] << 24) |    \
               (((const uint8_t*)(x))[1] << 16) |    \
               (((const uint8_t*)(x))[2] <<  8) |    \
                ((const uint8_t*)(x))[3])
#endif
#ifndef AV_WB32
#   define AV_WB32(p, darg) do {                \
        unsigned d = (darg);                    \
        ((uint8_t*)(p))[3] = (d);               \
        ((uint8_t*)(p))[2] = (d)>>8;            \
        ((uint8_t*)(p))[1] = (d)>>16;           \
        ((uint8_t*)(p))[0] = (d)>>24;           \
    } while(0)
#endif

#ifndef AV_RL32
#   define AV_RL32(x)                                \
    (((uint32_t)((const uint8_t*)(x))[3] << 24) |    \
               (((const uint8_t*)(x))[2] << 16) |    \
               (((const uint8_t*)(x))[1] <<  8) |    \
                ((const uint8_t*)(x))[0])
#endif
#ifndef AV_WL32
#   define AV_WL32(p, darg) do {                \
        unsigned d = (darg);                    \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
        ((uint8_t*)(p))[3] = (d)>>24;           \
    } while(0)
#endif

#ifndef AV_RB64
#   define AV_RB64(x)                                   \
    (((uint64_tt)((const uint8_t*)(x))[0] << 56) |       \
     ((uint64_tt)((const uint8_t*)(x))[1] << 48) |       \
     ((uint64_tt)((const uint8_t*)(x))[2] << 40) |       \
     ((uint64_tt)((const uint8_t*)(x))[3] << 32) |       \
     ((uint64_tt)((const uint8_t*)(x))[4] << 24) |       \
     ((uint64_tt)((const uint8_t*)(x))[5] << 16) |       \
     ((uint64_tt)((const uint8_t*)(x))[6] <<  8) |       \
      (uint64_tt)((const uint8_t*)(x))[7])
#endif
#ifndef AV_WB64
#   define AV_WB64(p, darg) do {                \
        uint64_tt d = (darg);                    \
        ((uint8_t*)(p))[7] = (d);               \
        ((uint8_t*)(p))[6] = (d)>>8;            \
        ((uint8_t*)(p))[5] = (d)>>16;           \
        ((uint8_t*)(p))[4] = (d)>>24;           \
        ((uint8_t*)(p))[3] = (d)>>32;           \
        ((uint8_t*)(p))[2] = (d)>>40;           \
        ((uint8_t*)(p))[1] = (d)>>48;           \
        ((uint8_t*)(p))[0] = (d)>>56;           \
    } while(0)
#endif

#ifndef AV_RL64
#   define AV_RL64(x)                                   \
    (((uint64_tt)((const uint8_t*)(x))[7] << 56) |       \
     ((uint64_tt)((const uint8_t*)(x))[6] << 48) |       \
     ((uint64_tt)((const uint8_t*)(x))[5] << 40) |       \
     ((uint64_tt)((const uint8_t*)(x))[4] << 32) |       \
     ((uint64_tt)((const uint8_t*)(x))[3] << 24) |       \
     ((uint64_tt)((const uint8_t*)(x))[2] << 16) |       \
     ((uint64_tt)((const uint8_t*)(x))[1] <<  8) |       \
      (uint64_tt)((const uint8_t*)(x))[0])
#endif
#ifndef AV_WL64
#   define AV_WL64(p, darg) do {                \
        uint64_tt d = (darg);                    \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
        ((uint8_t*)(p))[3] = (d)>>24;           \
        ((uint8_t*)(p))[4] = (d)>>32;           \
        ((uint8_t*)(p))[5] = (d)>>40;           \
        ((uint8_t*)(p))[6] = (d)>>48;           \
        ((uint8_t*)(p))[7] = (d)>>56;           \
    } while(0)
#endif

#if AV_HAVE_BIGENDIAN
#   define AV_RN(s, p)    AV_RB##s(p)
#   define AV_WN(s, p, v) AV_WB##s(p, v)
#else
#   define AV_RN(s, p)    AV_RL##s(p)
#   define AV_WN(s, p, v) AV_WL##s(p, v)
#endif

#endif /* HAVE_FAST_UNALIGNED */

#ifndef AV_RN16
#   define AV_RN16(p) AV_RN(16, p)
#endif

#ifndef AV_RN32
#   define AV_RN32(p) AV_RN(32, p)
#endif

#ifndef AV_RN64
#   define AV_RN64(p) AV_RN(64, p)
#endif

#ifndef AV_WN16
#   define AV_WN16(p, v) AV_WN(16, p, v)
#endif

#ifndef AV_WN32
#   define AV_WN32(p, v) AV_WN(32, p, v)
#endif

#ifndef AV_WN64
#   define AV_WN64(p, v) AV_WN(64, p, v)
#endif

#if AV_HAVE_BIGENDIAN
#   define AV_RB(s, p)    AV_RN##s(p)
#   define AV_WB(s, p, v) AV_WN##s(p, v)
#   define AV_RL(s, p)    av_bswap##s(AV_RN##s(p))
#   define AV_WL(s, p, v) AV_WN##s(p, av_bswap##s(v))
#else
#   define AV_RB(s, p)    av_bswap##s(AV_RN##s(p))
#   define AV_WB(s, p, v) AV_WN##s(p, av_bswap##s(v))
#   define AV_RL(s, p)    AV_RN##s(p)
#   define AV_WL(s, p, v) AV_WN##s(p, v)
#endif

#define AV_RB8(x)     (((const uint8_t*)(x))[0])
#define AV_WB8(p, d)  do { ((uint8_t*)(p))[0] = (d); } while(0)

#define AV_RL8(x)     AV_RB8(x)
#define AV_WL8(p, d)  AV_WB8(p, d)

#ifndef AV_RB16
#   define AV_RB16(p)    AV_RB(16, p)
#endif
#ifndef AV_WB16
#   define AV_WB16(p, v) AV_WB(16, p, v)
#endif

#ifndef AV_RL16
#   define AV_RL16(p)    AV_RL(16, p)
#endif
#ifndef AV_WL16
#   define AV_WL16(p, v) AV_WL(16, p, v)
#endif

#ifndef AV_RB32
#   define AV_RB32(p)    AV_RB(32, p)
#endif
#ifndef AV_WB32
#   define AV_WB32(p, v) AV_WB(32, p, v)
#endif

#ifndef AV_RL32
#   define AV_RL32(p)    AV_RL(32, p)
#endif
#ifndef AV_WL32
#   define AV_WL32(p, v) AV_WL(32, p, v)
#endif

#ifndef AV_RB64
#   define AV_RB64(p)    AV_RB(64, p)
#endif
#ifndef AV_WB64
#   define AV_WB64(p, v) AV_WB(64, p, v)
#endif

#ifndef AV_RL64
#   define AV_RL64(p)    AV_RL(64, p)
#endif
#ifndef AV_WL64
#   define AV_WL64(p, v) AV_WL(64, p, v)
#endif

#ifndef AV_RB24
#   define AV_RB24(x)                           \
    ((((const uint8_t*)(x))[0] << 16) |         \
     (((const uint8_t*)(x))[1] <<  8) |         \
      ((const uint8_t*)(x))[2])
#endif
#ifndef AV_WB24
#   define AV_WB24(p, d) do {                   \
        ((uint8_t*)(p))[2] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[0] = (d)>>16;           \
    } while(0)
#endif

#ifndef AV_RL24
#   define AV_RL24(x)                           \
    ((((const uint8_t*)(x))[2] << 16) |         \
     (((const uint8_t*)(x))[1] <<  8) |         \
      ((const uint8_t*)(x))[0])
#endif
#ifndef AV_WL24
#   define AV_WL24(p, d) do {                   \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
    } while(0)
#endif

#ifndef AV_RB48
#   define AV_RB48(x)                                     \
    (((uint64_tt)((const uint8_t*)(x))[0] << 40) |         \
     ((uint64_tt)((const uint8_t*)(x))[1] << 32) |         \
     ((uint64_tt)((const uint8_t*)(x))[2] << 24) |         \
     ((uint64_tt)((const uint8_t*)(x))[3] << 16) |         \
     ((uint64_tt)((const uint8_t*)(x))[4] <<  8) |         \
      (uint64_tt)((const uint8_t*)(x))[5])
#endif
#ifndef AV_WB48
#   define AV_WB48(p, darg) do {                \
        uint64_tt d = (darg);                    \
        ((uint8_t*)(p))[5] = (d);               \
        ((uint8_t*)(p))[4] = (d)>>8;            \
        ((uint8_t*)(p))[3] = (d)>>16;           \
        ((uint8_t*)(p))[2] = (d)>>24;           \
        ((uint8_t*)(p))[1] = (d)>>32;           \
        ((uint8_t*)(p))[0] = (d)>>40;           \
    } while(0)
#endif

#ifndef AV_RL48
#   define AV_RL48(x)                                     \
    (((uint64_tt)((const uint8_t*)(x))[5] << 40) |         \
     ((uint64_tt)((const uint8_t*)(x))[4] << 32) |         \
     ((uint64_tt)((const uint8_t*)(x))[3] << 24) |         \
     ((uint64_tt)((const uint8_t*)(x))[2] << 16) |         \
     ((uint64_tt)((const uint8_t*)(x))[1] <<  8) |         \
      (uint64_tt)((const uint8_t*)(x))[0])
#endif
#ifndef AV_WL48
#   define AV_WL48(p, darg) do {                \
        uint64_tt d = (darg);                    \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
        ((uint8_t*)(p))[3] = (d)>>24;           \
        ((uint8_t*)(p))[4] = (d)>>32;           \
        ((uint8_t*)(p))[5] = (d)>>40;           \
    } while(0)
#endif

/*
 * The AV_[RW]NA macros access naturally aligned data
 * in a type-safe way.
 */

#define AV_RNA(s, p)    (((const av_alias##s*)(p))->u##s)
#define AV_WNA(s, p, v) (((av_alias##s*)(p))->u##s = (v))

#ifndef AV_RN16A
#   define AV_RN16A(p) AV_RNA(16, p)
#endif

#ifndef AV_RN32A
#   define AV_RN32A(p) AV_RNA(32, p)
#endif

#ifndef AV_RN64A
#   define AV_RN64A(p) AV_RNA(64, p)
#endif

#ifndef AV_WN16A
#   define AV_WN16A(p, v) AV_WNA(16, p, v)
#endif

#ifndef AV_WN32A
#   define AV_WN32A(p, v) AV_WNA(32, p, v)
#endif

#ifndef AV_WN64A
#   define AV_WN64A(p, v) AV_WNA(64, p, v)
#endif

/*
 * The AV_COPYxxU macros are suitable for copying data to/from unaligned
 * memory locations.
 */

#define AV_COPYU(n, d, s) AV_WN##n(d, AV_RN##n(s));

#ifndef AV_COPY16U
#   define AV_COPY16U(d, s) AV_COPYU(16, d, s)
#endif

#ifndef AV_COPY32U
#   define AV_COPY32U(d, s) AV_COPYU(32, d, s)
#endif

#ifndef AV_COPY64U
#   define AV_COPY64U(d, s) AV_COPYU(64, d, s)
#endif

#ifndef AV_COPY128U
#   define AV_COPY128U(d, s)                                    \
    do {                                                        \
        AV_COPY64U(d, s);                                       \
        AV_COPY64U((char *)(d) + 8, (const char *)(s) + 8);     \
    } while(0)
#endif

/* Parameters for AV_COPY*, AV_SWAP*, AV_ZERO* must be
 * naturally aligned. They may be implemented using MMX,
 * so emms_c() must be called before using any float code
 * afterwards.
 */

#define AV_COPY(n, d, s) \
    (((av_alias##n*)(d))->u##n = ((const av_alias##n*)(s))->u##n)

#ifndef AV_COPY16
#   define AV_COPY16(d, s) AV_COPY(16, d, s)
#endif

#ifndef AV_COPY32
#   define AV_COPY32(d, s) AV_COPY(32, d, s)
#endif

#ifndef AV_COPY64
#   define AV_COPY64(d, s) AV_COPY(64, d, s)
#endif

#ifndef AV_COPY128
#   define AV_COPY128(d, s)                    \
    do {                                       \
        AV_COPY64(d, s);                       \
        AV_COPY64((char*)(d)+8, (char*)(s)+8); \
    } while(0)
#endif

#define AV_SWAP(n, a, b) FFSWAP(av_alias##n, *(av_alias##n*)(a), *(av_alias##n*)(b))

#ifndef AV_SWAP64
#   define AV_SWAP64(a, b) AV_SWAP(64, a, b)
#endif

#define AV_ZERO(n, d) (((av_alias##n*)(d))->u##n = 0)

#ifndef AV_ZERO16
#   define AV_ZERO16(d) AV_ZERO(16, d)
#endif

#ifndef AV_ZERO32
#   define AV_ZERO32(d) AV_ZERO(32, d)
#endif

#ifndef AV_ZERO64
#   define AV_ZERO64(d) AV_ZERO(64, d)
#endif

#ifndef AV_ZERO128
#   define AV_ZERO128(d)         \
    do {                         \
        AV_ZERO64(d);            \
        AV_ZERO64((char*)(d)+8); \
    } while(0)
#endif


#define AV_LZO_INPUT_PADDING   8
#define AV_LZO_OUTPUT_PADDING 12

#define 	FFMAX(a, b)   ((a) > (b) ? (a) : (b))




/// Define if we may write up to 12 bytes beyond the output buffer.
//#define OUTBUF_PADDED 1
/// Define if we may read up to 8 bytes beyond the input buffer.
//#define INBUF_PADDED 1



typedef struct LZOContext {
    const uint8_t *in, *in_end;
    uint8_t *out_start, *out, *out_end;
    int error;
} LZOContext;

/**
 * @brief Reads one byte from the input buffer, avoiding an overrun.
 * @return byte read
 */
static __inline int get_byte(LZOContext *c)
{
    if (c->in < c->in_end)
        return *c->in++;
    c->error |= AV_LZO_INPUT_DEPLETED;
    return 1;
}

#ifdef INBUF_PADDED
#define GETB(c) (*(c).in++)
#else
#define GETB(c) get_byte(&(c))
#endif

/**
 * @brief Decodes a length value in the coding used by lzo.
 * @param x previous byte value
 * @param mask bits used from x
 * @return decoded length value
 */
static __inline int get_len(LZOContext *c, int x, int mask)
{
    int cnt = x & mask;
    if (!cnt) {
        while (!(x = get_byte(c))) {
            if (cnt >= INT_MAX - 1000) {
                c->error |= AV_LZO_ERROR;
                break;
            }
            cnt += 255;
        }
        cnt += mask + x;
    }
    return cnt;
}

/**
 * @brief Copies bytes from input to output buffer with checking.
 * @param cnt number of bytes to copy, must be >= 0
 */
static __inline void copy(LZOContext *c, int cnt)
{
    register const uint8_t *src = c->in;
    register uint8_t *dst       = c->out;
  //  av_assert0(cnt >= 0);
    if (cnt > c->in_end - src) {
        cnt       = FFMAX(c->in_end - src, 0);
        c->error |= AV_LZO_INPUT_DEPLETED;
    }
    if (cnt > c->out_end - dst) {
        cnt       = FFMAX(c->out_end - dst, 0);
        c->error |= AV_LZO_OUTPUT_FULL;
    }
#if defined(INBUF_PADDED) && defined(OUTBUF_PADDED)
    AV_COPY32U(dst, src);
    src += 4;
    dst += 4;
    cnt -= 4;
    if (cnt > 0)
#endif
    memcpy(dst, src, cnt);
    c->in  = src + cnt;
    c->out = dst + cnt;
}

static void fill16(uint8_t *dst, int len)
{
    uint32_t v = AV_RN16(dst - 2);

    v |= v << 16;

    while (len >= 4) {
        AV_WN32(dst, v);
        dst += 4;
        len -= 4;
    }

    while (len--) {
        *dst = dst[-2];
        dst++;
    }
}

static void fill24(uint8_t *dst, int len)
{
#if HAVE_BIGENDIAN
    uint32_t v = AV_RB24(dst - 3);
    uint32_t a = v << 8  | v >> 16;
    uint32_t b = v << 16 | v >> 8;
    uint32_t c = v << 24 | v;
#else
    uint32_t v = AV_RL24(dst - 3);
    uint32_t a = v       | v << 24;
    uint32_t b = v >> 8  | v << 16;
    uint32_t c = v >> 16 | v << 8;
#endif

    while (len >= 12) {
        AV_WN32(dst,     a);
        AV_WN32(dst + 4, b);
        AV_WN32(dst + 8, c);
        dst += 12;
        len -= 12;
    }

    if (len >= 4) {
        AV_WN32(dst, a);
        dst += 4;
        len -= 4;
    }

    if (len >= 4) {
        AV_WN32(dst, b);
        dst += 4;
        len -= 4;
    }

    while (len--) {
        *dst = dst[-3];
        dst++;
    }
}

static void fill32(uint8_t *dst, int len)
{
    uint32_t v = AV_RN32(dst - 4);

    while (len >= 4) {
        AV_WN32(dst, v);
        dst += 4;
        len -= 4;
    }

    while (len--) {
        *dst = dst[-4];
        dst++;
    }
}

void av_memcpy_backptr(uint8_t *dst, int back, int cnt)
   {
       const uint8_t *src = &dst[-back];
     if (!back)
         return;

    if (back == 1) {
       memset(dst, *src, cnt);
     } else if (back == 2) {
         fill16(dst, cnt);
      } else if (back == 3) {
          fill24(dst, cnt);
     } else if (back == 4) {
          fill32(dst, cnt);
    } else {
        if (cnt >= 16) {
            int blocklen = back;
               while (cnt > blocklen) {
                   memcpy(dst, src, blocklen);
                   dst       += blocklen;
                   cnt       -= blocklen;
                   blocklen <<= 1;
               }
               memcpy(dst, src, cnt);
               return;
           }
           if (cnt >= 8) {
               AV_COPY32U(dst,     src);
               AV_COPY32U(dst + 4, src + 4);
               src += 8;
               dst += 8;
               cnt -= 8;
           }
           if (cnt >= 4) {
               AV_COPY32U(dst, src);
               src += 4;
               dst += 4;
               cnt -= 4;
           }
           if (cnt >= 2) {
               AV_COPY16U(dst, src);
              src += 2;
               dst += 2;
               cnt -= 2;
           }
           if (cnt)
               *dst = *src;
       }
   }

/**
 * @brief Copies previously decoded bytes to current position.
 * @param back how many bytes back we start, must be > 0
 * @param cnt number of bytes to copy, must be > 0
 *
 * cnt > back is valid, this will copy the bytes we just copied,
 * thus creating a repeating pattern with a period length of back.
 */
static __inline void copy_backptr(LZOContext *c, int back, int cnt)
{
    register uint8_t *dst       = c->out;
  //  av_assert0(cnt > 0);
    if (dst - c->out_start < back) {
        c->error |= AV_LZO_INVALID_BACKPTR;
        return;
    }
    if (cnt > c->out_end - dst) {
        cnt       = FFMAX(c->out_end - dst, 0);
        c->error |= AV_LZO_OUTPUT_FULL;
    }
    av_memcpy_backptr(dst, back, cnt);
    c->out = dst + cnt;
}

int av_lzo1x_decode(void *out, int *outlen, const void *in, int *inlen)
{
    int state = 0;
    int x;
    LZOContext c;
    if (*outlen <= 0 || *inlen <= 0) {
        int res = 0;
        if (*outlen <= 0)
            res |= AV_LZO_OUTPUT_FULL;
        if (*inlen <= 0)
            res |= AV_LZO_INPUT_DEPLETED;
        return res;
    }
    c.in      = in;
    c.in_end  = (const uint8_t *)in + *inlen;
    c.out     = c.out_start = out;
    c.out_end = (uint8_t *)out + *outlen;
    c.error   = 0;
    x         = GETB(c);
    if (x > 17) {
        copy(&c, x - 17);
        x = GETB(c);
        if (x < 16)
            c.error |= AV_LZO_ERROR;
    }
    if (c.in > c.in_end)
        c.error |= AV_LZO_INPUT_DEPLETED;
    while (!c.error) {
        int cnt, back;
        if (x > 15) {
            if (x > 63) {
                cnt  = (x >> 5) - 1;
                back = (GETB(c) << 3) + ((x >> 2) & 7) + 1;
            } else if (x > 31) {
                cnt  = get_len(&c, x, 31);
                x    = GETB(c);
                back = (GETB(c) << 6) + (x >> 2) + 1;
            } else {
                cnt   = get_len(&c, x, 7);
                back  = (1 << 14) + ((x & 8) << 11);
                x     = GETB(c);
                back += (GETB(c) << 6) + (x >> 2);
                if (back == (1 << 14)) {
                    if (cnt != 1)
                        c.error |= AV_LZO_ERROR;
                    break;
                }
            }
        } else if (!state) {
            cnt = get_len(&c, x, 15);
            copy(&c, cnt + 3);
            x = GETB(c);
            if (x > 15)
                continue;
            cnt  = 1;
            back = (1 << 11) + (GETB(c) << 2) + (x >> 2) + 1;
        } else {
            cnt  = 0;
            back = (GETB(c) << 2) + (x >> 2) + 1;
        }
        copy_backptr(&c, back, cnt + 2);
        state =
        cnt   = x & 3;
        copy(&c, cnt);
        x = GETB(c);
    }
    *inlen = c.in_end - c.in;
    if (c.in > c.in_end)
        *inlen = 0;
    *outlen = c.out_end - c.out;
    return c.error;
}
