#ifndef COMMON_VECTOR_MATH_H
#define COMMON_VECTOR_MATH_H

// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

// Copyright 2014 Tony Wasserka
// Copyright 2025 Borked3DS Emulator Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the owner nor the names of its contributors may
//       be used to endorse or promote products derived from this software
//       without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <new>
#include <type_traits>
#include <boost/serialization/access.hpp>

// SIMD includes
#if defined(__x86_64__) || defined(_M_X64)
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#if defined(__SSE2__)
#define HAVE_SSE2
#endif
#if defined(__SSE4_1__)
#define HAVE_SSE4_1
#endif
#if defined(__AVX__)
#define HAVE_AVX
#endif
#elif defined(__aarch64__) || defined(_M_ARM64)
#ifdef _MSC_VER
#include <arm64_neon.h>
#else
#include <arm_neon.h>
#endif
#define HAVE_NEON
#endif
#if defined(__AVX2__)
#define HAVE_AVX2
#endif
#if !defined(__FMA__) && defined(__AVX2__)
#define HAVE_FMA
#endif

namespace Common {

template <typename T>
class Vec2;
template <typename T>
class Vec3;
template <typename T>
class Vec4;

namespace detail {
#if defined(HAVE_SSE2) || defined(HAVE_NEON)
constexpr bool has_simd_support = true;
#else
constexpr bool has_simd_support = false;
#endif

template <typename T, typename = void>
struct has_simd_member : std::false_type {};

template <typename T>
struct has_simd_member<T, std::void_t<decltype(std::remove_cv_t<T>::simd)>> : std::true_type {};

#if defined(HAVE_SSE2)
[[nodiscard]] inline __m128i vec2_to_epi16(int16_t x, int16_t y) {
    return _mm_setr_epi16(x, y, 0, 0, 0, 0, 0, 0);
}

[[nodiscard]] inline __m128i vec3_to_epi16(int16_t x, int16_t y, int16_t z) {
    return _mm_setr_epi16(x, y, z, 0, 0, 0, 0, 0);
}
#endif

template <typename T>
struct is_vectorizable : std::bool_constant<std::is_same_v<T, float> && has_simd_support> {};
} // namespace detail

template <typename T>
class Vec2 {
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int file_version) {
        ar & x;
        ar & y;
    }

public:
    T x;
    T y;

    constexpr Vec2() = default;
    constexpr Vec2(const T& x_, const T& y_) : x(x_), y(y_) {}
    static constexpr std::size_t dimension = 2;

    [[nodiscard]] T* AsArray() {
        return &x;
    }

    [[nodiscard]] const T* AsArray() const {
        return &x;
    }

    template <typename T2>
    [[nodiscard]] constexpr Vec2<T2> Cast() const {
        return Vec2<T2>(static_cast<T2>(x), static_cast<T2>(y));
    }

    [[nodiscard]] static constexpr Vec2 AssignToAll(const T& f) {
        return Vec2{f, f};
    }

    [[nodiscard]] constexpr Vec2<decltype(T{} + T{})> operator+(const Vec2& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec2<T> result;
#if defined(HAVE_SSE2)
            // Create SIMD vector with both components (x,y)
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 b = _mm_setr_ps(other.x, other.y, 0.0f, 0.0f);
            __m128 sum = _mm_add_ps(a, b);
            // Store directly to result using lower 64 bits
            _mm_store_sd(reinterpret_cast<double*>(&result.x), _mm_castps_pd(sum));
#elif defined(HAVE_NEON)
            // Create temporary storage for safe type punning
            uint64_t a_bits, b_bits;
            std::memcpy(&a_bits, &x, sizeof(uint64_t));
            std::memcpy(&b_bits, &other.x, sizeof(uint64_t));

            // Create NEON vectors and add
            float32x2_t a = vcreate_f32(a_bits);
            float32x2_t b = vcreate_f32(b_bits);
            float32x2_t sum = vadd_f32(a, b);

            // Store back the result
            float result_array[2];
            vst1_f32(result_array, sum);
            std::memcpy(&result.x, result_array, sizeof(float) * 2);
#endif
            return result;
        } else {
            return {x + other.x, y + other.y};
        }
    }

    [[nodiscard]] constexpr Vec2& operator+=(const Vec2& other) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 b = _mm_setr_ps(other.x, other.y, 0.0f, 0.0f);
            __m128 sum = _mm_add_ps(a, b);
            // Store result directly to x,y using 64-bit store
            _mm_store_sd(reinterpret_cast<double*>(&x), _mm_castps_pd(sum));
#elif defined(HAVE_NEON)
            // Create temporary storage for safe type punning
            uint64_t a_bits, b_bits;
            std::memcpy(&a_bits, &x, sizeof(uint64_t));
            std::memcpy(&b_bits, &other.x, sizeof(uint64_t));

            // Create NEON vectors and add
            float32x2_t a = vcreate_f32(a_bits);
            float32x2_t b = vcreate_f32(b_bits);
            float32x2_t sum = vadd_f32(a, b);

            // Store back the result directly to this object's x,y
            float result_array[2];
            vst1_f32(result_array, sum);
            std::memcpy(&x, result_array, sizeof(float) * 2);
#endif
        } else {
            x += other.x;
            y += other.y;
        }
        return *this;
    }

    [[nodiscard]] constexpr Vec2<decltype(T{} - T{})> operator-(const Vec2& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec2<T> result;
#if defined(HAVE_SSE2)
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 b = _mm_setr_ps(other.x, other.y, 0.0f, 0.0f);
            __m128 diff = _mm_sub_ps(a, b);
            // Store result directly using 64-bit store
            _mm_store_sd(reinterpret_cast<double*>(&result.x), _mm_castps_pd(diff));
#elif defined(HAVE_NEON)
            // Create temporary storage for safe type punning
            uint64_t a_bits, b_bits;
            std::memcpy(&a_bits, &x, sizeof(uint64_t));
            std::memcpy(&b_bits, &other.x, sizeof(uint64_t));

            // Create NEON vectors and subtract
            float32x2_t a = vcreate_f32(a_bits);
            float32x2_t b = vcreate_f32(b_bits);
            float32x2_t diff = vsub_f32(a, b);

            // Store back the result
            float result_array[2];
            vst1_f32(result_array, diff);
            std::memcpy(&result.x, result_array, sizeof(float) * 2);
#endif
            return result;
        } else {
            return {x - other.x, y - other.y};
        }
    }

    [[nodiscard]] constexpr Vec2& operator-=(const Vec2& other) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 b = _mm_setr_ps(other.x, other.y, 0.0f, 0.0f);
            __m128 diff = _mm_sub_ps(a, b);
            // Store directly to x,y using 64-bit store
            _mm_store_sd(reinterpret_cast<double*>(&x), _mm_castps_pd(diff));
#elif defined(HAVE_NEON)
            // Create temporary storage for safe type punning
            uint64_t a_bits, b_bits;
            std::memcpy(&a_bits, &x, sizeof(uint64_t));
            std::memcpy(&b_bits, &other.x, sizeof(uint64_t));

            // Create NEON vectors and subtract
            float32x2_t a = vcreate_f32(a_bits);
            float32x2_t b = vcreate_f32(b_bits);
            float32x2_t diff = vsub_f32(a, b);

            // Store back the result directly to this object's x,y
            float result_array[2];
            vst1_f32(result_array, diff);
            std::memcpy(&x, result_array, sizeof(float) * 2);
#endif
        } else {
            x -= other.x;
            y -= other.y;
        }
        return *this;
    }

    template <typename U = T>
    [[nodiscard]] constexpr Vec2<std::enable_if_t<std::is_signed_v<U>, U>> operator-() const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec2<T> result;
#if defined(HAVE_SSE2)
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 neg = _mm_sub_ps(_mm_setzero_ps(), a);
            // Store directly using 64-bit store
            _mm_store_sd(reinterpret_cast<double*>(&result.x), _mm_castps_pd(neg));
#elif defined(HAVE_NEON)
            // Create temporary storage for safe type punning
            uint64_t vec_bits;
            std::memcpy(&vec_bits, &x, sizeof(uint64_t));

            // Create NEON vector and negate it
            float32x2_t a = vcreate_f32(vec_bits);
            float32x2_t neg = vneg_f32(a);

            // Store back the result
            float result_array[2];
            vst1_f32(result_array, neg);
            std::memcpy(&result.x, result_array, sizeof(float) * 2);
#endif
            return result;
        } else {
            return {-x, -y};
        }
    }

    [[nodiscard]] constexpr Vec2<decltype(T{} * T{})> operator*(const Vec2& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec2<T> result;
#if defined(HAVE_SSE2)
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 b = _mm_setr_ps(other.x, other.y, 0.0f, 0.0f);
            __m128 prod = _mm_mul_ps(a, b);
            // Store directly using 64-bit store
            _mm_store_sd(reinterpret_cast<double*>(&result.x), _mm_castps_pd(prod));
#elif defined(HAVE_NEON)
            // Create temporary storage for safe type punning
            uint64_t a_bits, b_bits;
            std::memcpy(&a_bits, &x, sizeof(uint64_t));
            std::memcpy(&b_bits, &other.x, sizeof(uint64_t));

            // Create NEON vectors and multiply
            float32x2_t a = vcreate_f32(a_bits);
            float32x2_t b = vcreate_f32(b_bits);
            float32x2_t prod = vmul_f32(a, b);

            // Store back the result
            float result_array[2];
            vst1_f32(result_array, prod);
            std::memcpy(&result.x, result_array, sizeof(float) * 2);
#endif
            return result;
        } else {
            return {x * other.x, y * other.y};
        }
    }

    template <typename V>
    [[nodiscard]] constexpr Vec2<decltype(T{} * V{})> operator*(const V& f) const {
        if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
            Vec2<decltype(T{} * V{})> result;
#if defined(HAVE_SSE2)
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 b = _mm_set1_ps(f);
            __m128 prod = _mm_mul_ps(a, b);
            // Store directly using 64-bit store
            _mm_store_sd(reinterpret_cast<double*>(&result.x), _mm_castps_pd(prod));
#elif defined(HAVE_NEON)
            // Create temporary storage for safe type punning
            uint64_t vec_bits;
            std::memcpy(&vec_bits, &x, sizeof(uint64_t));

            // Create NEON vectors and multiply
            float32x2_t a = vcreate_f32(vec_bits);
            float32x2_t b = vdup_n_f32(f);
            float32x2_t prod = vmul_f32(a, b);

            // Store back the result
            float result_array[2];
            vst1_f32(result_array, prod);
            std::memcpy(&result.x, result_array, sizeof(float) * 2);
#endif
            return result;
        } else {
            return {x * f, y * f};
        }
    }

    template <typename V>
    constexpr Vec2& operator*=(const V& f) {
        if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
#if defined(HAVE_SSE2)
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 b = _mm_set1_ps(f);
            __m128 prod = _mm_mul_ps(a, b);
            // Store directly to x,y using 64-bit store
            _mm_store_sd(reinterpret_cast<double*>(&x), _mm_castps_pd(prod));
#elif defined(HAVE_NEON)
            // Create temporary storage for safe type punning
            uint64_t a_bits;
            std::memcpy(&a_bits, &x, sizeof(uint64_t));

            // Create NEON vectors and multiply
            float32x2_t a = vcreate_f32(a_bits);
            float32x2_t b = vdup_n_f32(f);
            float32x2_t prod = vmul_f32(a, b);

            // Store back the result directly to this object's x,y
            float result_array[2];
            vst1_f32(result_array, prod);
            std::memcpy(&x, result_array, sizeof(float) * 2);
#endif
        } else {
            *this = *this * f;
        }
        return *this;
    }

    template <typename V>
    [[nodiscard]] constexpr Vec2<decltype(T{} / V{})> operator/(const V& f) const {
        if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
            Vec2<decltype(T{} / V{})> result;
#if defined(HAVE_SSE2)
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 b = _mm_set1_ps(f);
            __m128 quot = _mm_div_ps(a, b);
            // Store directly using 64-bit store
            _mm_store_sd(reinterpret_cast<double*>(&result.x), _mm_castps_pd(quot));
#elif defined(HAVE_NEON)
            // Create temporary storage for safe type punning
            uint64_t a_bits;
            std::memcpy(&a_bits, &x, sizeof(uint64_t));

            // Create NEON vectors and divide (using reciprocal multiplication)
            float32x2_t a = vcreate_f32(a_bits);
            float32x2_t recip = vdup_n_f32(1.0f / f);
            float32x2_t quot = vmul_f32(a, recip);

            // Store back the result
            float result_array[2];
            vst1_f32(result_array, quot);
            std::memcpy(&result.x, result_array, sizeof(float) * 2);
#endif
            return result;
        } else {
            return {x / f, y / f};
        }
    }

    template <typename V>
    constexpr Vec2& operator/=(const V& f) {
        if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
#if defined(HAVE_SSE2)
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 b = _mm_set1_ps(f);
            __m128 quot = _mm_div_ps(a, b);
            // Store directly to x,y using 64-bit store
            _mm_store_sd(reinterpret_cast<double*>(&x), _mm_castps_pd(quot));
#elif defined(HAVE_NEON)
            // Create temporary storage for safe type punning
            uint64_t a_bits;
            std::memcpy(&a_bits, &x, sizeof(uint64_t));

            // Create NEON vectors and divide (using reciprocal multiplication)
            float32x2_t a = vcreate_f32(a_bits);
            float32x2_t recip = vdup_n_f32(1.0f / f);
            float32x2_t quot = vmul_f32(a, recip);

            // Store back the result directly to this object's x,y
            float result_array[2];
            vst1_f32(result_array, quot);
            std::memcpy(&x, result_array, sizeof(float) * 2);
#endif
        } else {
            *this = *this / f;
        }
        return *this;
    }

    [[nodiscard]] constexpr T Length2() const {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            __m128 v = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 sq = _mm_mul_ps(v, v);
            // Horizontal add using hadd instruction if available, otherwise use original method
#if defined(__SSE3__)
            return _mm_cvtss_f32(_mm_hadd_ps(sq, sq));
#else
            return _mm_cvtss_f32(_mm_add_ss(sq, _mm_shuffle_ps(sq, sq, _MM_SHUFFLE(1, 1, 1, 1))));
#endif
#elif defined(HAVE_NEON)
            // Create temporary storage for safe type punning
            uint64_t v_bits;
            std::memcpy(&v_bits, &x, sizeof(uint64_t));

            // Create NEON vector and compute squared length
            float32x2_t v = vcreate_f32(v_bits);
            float32x2_t sq = vmul_f32(v, v);

            // Horizontal add of the squares using vpadd
            float32x2_t sum = vpadd_f32(sq, sq);
            return vget_lane_f32(sum, 0);
#endif
        } else {
            return x * x + y * y;
        }
    }

    [[nodiscard]] constexpr bool operator!=(const Vec2& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 b = _mm_setr_ps(other.x, other.y, 0.0f, 0.0f);
            __m128 cmp = _mm_cmpeq_ps(a, b);
            // Simplified mask check using only lower 2 bits
            return (_mm_movemask_ps(cmp) & 3) != 3;
#elif defined(HAVE_NEON)
            // Create temporary storage for safe type punning
            uint64_t a_bits, b_bits;
            std::memcpy(&a_bits, &x, sizeof(uint64_t));
            std::memcpy(&b_bits, &other.x, sizeof(uint64_t));

            // Create NEON vectors and compare
            float32x2_t a = vcreate_f32(a_bits);
            float32x2_t b = vcreate_f32(b_bits);
            uint32x2_t cmp = vceq_f32(a, b);

            // Convert comparison result to 64-bit value and check
            uint64x1_t result = vreinterpret_u64_u32(cmp);
            return vget_lane_u64(result, 0) != UINT64_C(0xFFFFFFFFFFFFFFFF);
#endif
        } else {
            return std::memcmp(AsArray(), other.AsArray(), sizeof(Vec2)) != 0;
        }
    }

    [[nodiscard]] constexpr bool operator==(const Vec2& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 b = _mm_setr_ps(other.x, other.y, 0.0f, 0.0f);
            __m128 cmp = _mm_cmpeq_ps(a, b);
            // Simplified mask check using only lower 2 bits
            return (_mm_movemask_ps(cmp) & 3) == 3;
#elif defined(HAVE_NEON)
            // Create temporary storage for safe type punning
            uint64_t a_bits, b_bits;
            std::memcpy(&a_bits, &x, sizeof(uint64_t));
            std::memcpy(&b_bits, &other.x, sizeof(uint64_t));

            // Create NEON vectors and compare
            float32x2_t a = vcreate_f32(a_bits);
            float32x2_t b = vcreate_f32(b_bits);
            uint32x2_t cmp = vceq_f32(a, b);

            // Convert comparison result to 64-bit value and check
            uint64x1_t result = vreinterpret_u64_u32(cmp);
            return vget_lane_u64(result, 0) == UINT64_C(0xFFFFFFFFFFFFFFFF);
#endif
        } else {
            return std::memcmp(AsArray(), other.AsArray(), sizeof(Vec2)) == 0;
        }
    }

    // Only implemented for T=float
    [[nodiscard]] float Length() const;
    float Normalize(); // returns the previous length, which is often useful

    [[nodiscard]] constexpr T& operator[](std::size_t i) {
        assert(i < dimension && "Index out of bounds in Vec2");
        return *((&x) + i);
    }

    [[nodiscard]] constexpr const T& operator[](std::size_t i) const {
        assert(i < dimension && "Index out of bounds in Vec2");
        return *((&x) + i);
    }

    constexpr void SetZero() {
        x = 0;
        y = 0;
    }

    // Common aliases: UV (texel coordinates), ST (texture coordinates)
    [[nodiscard]] constexpr T& u() {
        return x;
    }
    [[nodiscard]] constexpr T& v() {
        return y;
    }
    [[nodiscard]] constexpr T& s() {
        return x;
    }
    [[nodiscard]] constexpr T& t() {
        return y;
    }

    [[nodiscard]] constexpr const T& u() const {
        return x;
    }
    [[nodiscard]] constexpr const T& v() const {
        return y;
    }
    [[nodiscard]] constexpr const T& s() const {
        return x;
    }
    [[nodiscard]] constexpr const T& t() const {
        return y;
    }

    // swizzlers - create a subvector of specific components
    [[nodiscard]] constexpr Vec2 yx() const {
        return Vec2(y, x);
    }
    [[nodiscard]] constexpr Vec2 vu() const {
        return Vec2(y, x);
    }
    [[nodiscard]] constexpr Vec2 ts() const {
        return Vec2(y, x);
    }
};

template <typename T, typename V>
[[nodiscard]] constexpr Vec2<decltype(V{} * T{})> operator*(const V& f, const Vec2<T>& vec) {
    if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
        if constexpr (std::is_same_v<V, float> && std::is_same_v<T, float>) {
            Vec2<float> result;
#if defined(HAVE_FMA)
            // Use FMA for better precision
            __m128 vf = _mm_set1_ps(f);
            __m128 vvec = _mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<const __m64*>(&vec.x));
            _mm_storel_pi(reinterpret_cast<__m64*>(&result.x), _mm_mul_ps(vf, vvec));
#else
            // Standard SSE2 path
            __m128 vf = _mm_set1_ps(f);
            __m128 vvec = _mm_setr_ps(vec.x, vec.y, 0.0f, 0.0f);
            __m128 vresult = _mm_mul_ps(vf, vvec);
            result.x = _mm_cvtss_f32(vresult);
            result.y = _mm_cvtss_f32(_mm_shuffle_ps(vresult, vresult, _MM_SHUFFLE(1, 1, 1, 1)));
#endif
            return result;
        } else if constexpr (std::is_integral_v<V> && std::is_integral_v<T>) {
            Vec2<decltype(V{} * T{})> result;
            if constexpr (sizeof(T) <= 2) {
                // For 16-bit integers
                __m128i vf = _mm_set1_epi16(static_cast<int16_t>(f));
                __m128i vvec =
                    detail::vec2_to_epi16(static_cast<int16_t>(vec.x), static_cast<int16_t>(vec.y));
                __m128i vresult = _mm_mullo_epi16(vf, vvec);
                result.x = static_cast<T>(_mm_extract_epi16(vresult, 0));
                result.y = static_cast<T>(_mm_extract_epi16(vresult, 1));
            } else {
                // For 32-bit integers
                __m128i vf = _mm_set1_epi32(static_cast<int32_t>(f));
                __m128i vvec =
                    _mm_setr_epi32(static_cast<int32_t>(vec.x), static_cast<int32_t>(vec.y), 0, 0);
                __m128i vresult = _mm_mullo_epi32(vf, vvec);
                result.x = static_cast<T>(_mm_cvtsi128_si32(vresult));
                result.y = static_cast<T>(_mm_extract_epi32(vresult, 1));
            }
            return result;
        }
#elif defined(HAVE_NEON)
        if constexpr (std::is_same_v<V, float> && std::is_same_v<T, float>) {
            Vec2<float> result;
#if defined(__ARM_FEATURE_FMA)
            // Use FMA on ARM if available
            // Create temporary storage and use memcpy for safe data access
            float vec_array[2];
            std::memcpy(vec_array, &vec.x, sizeof(float) * 2);

            float32x2_t vvec = vld1_f32(vec_array);
            float32x2_t vf = vdup_n_f32(f);
            float32x2_t vresult = vmul_f32(vf, vvec);

            // Store result safely using memcpy
            vst1_f32(vec_array, vresult);
            std::memcpy(&result.x, vec_array, sizeof(float) * 2);
#else
            // Standard NEON path
            float vec_array[2] = {vec.x, vec.y};
            float32x2_t vvec = vld1_f32(vec_array);
            float32x2_t vf = vdup_n_f32(f);
            float32x2_t vresult = vmul_f32(vf, vvec);

            result.x = vget_lane_f32(vresult, 0);
            result.y = vget_lane_f32(vresult, 1);
#endif
            return result;
        } else if constexpr (std::is_integral_v<V> && std::is_integral_v<T>) {
            Vec2<decltype(V{} * T{})> result;
            if constexpr (sizeof(T) <= 2) {
                // For 16-bit integers
                int16_t vec_array[4] = {static_cast<int16_t>(vec.x), static_cast<int16_t>(vec.y), 0,
                                        0};
                int16x4_t vvec = vld1_s16(vec_array);
                int16x4_t vf = vdup_n_s16(static_cast<int16_t>(f));
                int16x4_t vresult = vmul_s16(vf, vvec);
                result.x = static_cast<T>(vget_lane_s16(vresult, 0));
                result.y = static_cast<T>(vget_lane_s16(vresult, 1));
            } else {
                // For 32-bit integers
                int32_t vec_array[2] = {static_cast<int32_t>(vec.x), static_cast<int32_t>(vec.y)};
                int32x2_t vvec = vld1_s32(vec_array);
                int32x2_t vf = vdup_n_s32(static_cast<int32_t>(f));
                int32x2_t vresult = vmul_s32(vf, vvec);
                result.x = static_cast<T>(vget_lane_s32(vresult, 0));
                result.y = static_cast<T>(vget_lane_s32(vresult, 1));
            }
            return result;
        }
#endif
    }
    // Fallback for non-vectorizable types
    return Vec2<decltype(V{} * T{})>(f * vec.x, f * vec.y);
}

using Vec2f = Vec2<float>;
using Vec2i = Vec2<int>;
using Vec2u = Vec2<unsigned int>;

template <>
inline float Vec2<float>::Length() const {
#if defined(HAVE_SSE4_1)
    // SSE4.1 has a dedicated dot product instruction
    __m128 v = _mm_setr_ps(x, y, 0.0f, 0.0f);
    // Use dp_ps with mask 0x31 for xy multiply and lowest element store
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(v, v, 0x31)));
#elif defined(HAVE_SSE2)
    __m128 v = _mm_setr_ps(x, y, 0.0f, 0.0f);
    __m128 sq = _mm_mul_ps(v, v);
#if defined(__SSE3__)
    // Use hadd for more efficient horizontal addition if SSE3 is available
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_hadd_ps(sq, sq)));
#else
    // Fallback to SSE2 shuffle and add
    return _mm_cvtss_f32(
        _mm_sqrt_ss(_mm_add_ss(sq, _mm_shuffle_ps(sq, sq, _MM_SHUFFLE(1, 1, 1, 1)))));
#endif
#elif defined(HAVE_NEON)
    // Create temporary storage for safe type punning
    uint64_t v_bits;
    std::memcpy(&v_bits, &x, sizeof(uint64_t));

    // Create NEON vector and compute squared length
    float32x2_t v = vcreate_f32(v_bits);
    float32x2_t sq = vmul_f32(v, v);
    float32x2_t sum = vpadd_f32(sq, sq);

    // Extract scalar value, compute square root, and return
    float sum_scalar = vget_lane_f32(sum, 0);
    return vget_lane_f32(vsqrt_f32(vdup_n_f32(sum_scalar)), 0);
#else
    return std::sqrt(x * x + y * y);
#endif
}

template <>
inline float Vec2<float>::Normalize() {
    const float length = Length();
    if (length < std::numeric_limits<float>::epsilon()) {
        x = 0;
        y = 0;
        return 0;
    }

    const float inv_length = 1.0f / length;
    x *= inv_length;
    y *= inv_length;
    return length;
}

template <typename T>
class alignas(16) Vec3 {
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int file_version) {
        ar & x;
        ar & y;
        ar & z;
    }

public:
    union {
        struct {
            T x, y, z;
            T pad; // For SIMD alignment
        };
#if defined(HAVE_SSE2)
        __m128 simd;
#elif defined(HAVE_NEON)
        float32x4_t simd;
#endif
    };

    constexpr void SetZero() {
        x = 0;
        y = 0;
        z = 0;
        if constexpr (detail::is_vectorizable<T>::value) {
            // Initialize the padding to maintain a consistent state
            pad = 0;
        }
    }

    constexpr Vec3() = default;
    constexpr Vec3(const T& x_, const T& y_, const T& z_) : x(x_), y(y_), z(z_) {}
    static constexpr std::size_t dimension = 3;

    [[nodiscard]] T* AsArray() {
        return &x;
    }

    [[nodiscard]] const T* AsArray() const {
        return &x;
    }

    template <typename T2>
    [[nodiscard]] constexpr Vec3<T2> Cast() const {
        if constexpr (detail::is_vectorizable<T>::value && detail::is_vectorizable<T2>::value) {
            Vec3<T2> result;
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float> && std::is_same_v<T2, float>) {
                // Use unaligned load/store for better flexibility
                __m128 v = _mm_loadu_ps(&x);
                _mm_storeu_ps(&result.x, v);
            } else if constexpr (std::is_same_v<T, int32_t> && std::is_same_v<T2, float>) {
                // Integer to float conversion
                __m128i vi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&x));
                _mm_storeu_ps(&result.x, _mm_cvtepi32_ps(vi));
            } else if constexpr (std::is_same_v<T, float> && std::is_same_v<T2, int32_t>) {
                // Float to integer conversion
                __m128 v = _mm_loadu_ps(&x);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&result.x), _mm_cvtps_epi32(v));
            } else {
                // Fallback to scalar for other type conversions
                result.x = static_cast<T2>(x);
                result.y = static_cast<T2>(y);
                result.z = static_cast<T2>(z);
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float> && std::is_same_v<T2, float>) {
                // Use temporary arrays for safe memory operations
                float temp[4];
                std::memcpy(temp, &x, sizeof(float) * 3);

                float32x4_t v = vld1q_f32(temp);
                vst1q_f32(temp, v);

                std::memcpy(&result.x, temp, sizeof(float) * 3);
            } else if constexpr (std::is_same_v<T, int32_t> && std::is_same_v<T2, float>) {
                // Integer to float conversion using safe memory handling
                int32_t temp_in[4];
                float temp_out[4];
                std::memcpy(temp_in, &x, sizeof(int32_t) * 3);

                int32x4_t vi = vld1q_s32(temp_in);
                float32x4_t vf = vcvtq_f32_s32(vi);
                vst1q_f32(temp_out, vf);

                std::memcpy(&result.x, temp_out, sizeof(float) * 3);
            } else if constexpr (std::is_same_v<T, float> && std::is_same_v<T2, int32_t>) {
                // Float to integer conversion using safe memory handling
                float temp_in[4];
                int32_t temp_out[4];
                std::memcpy(temp_in, &x, sizeof(float) * 3);

                float32x4_t v = vld1q_f32(temp_in);
                int32x4_t vi = vcvtq_s32_f32(v);
                vst1q_s32(temp_out, vi);

                std::memcpy(&result.x, temp_out, sizeof(int32_t) * 3);
            } else {
                // Fallback to scalar for other type conversions
                result.x = static_cast<T2>(x);
                result.y = static_cast<T2>(y);
                result.z = static_cast<T2>(z);
            }
#endif
            return result;
        } else {
            return Vec3<T2>(static_cast<T2>(x), static_cast<T2>(y), static_cast<T2>(z));
        }
    }

    [[nodiscard]] static constexpr Vec3 AssignToAll(const T& f) {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec3<T> result;
#if defined(HAVE_SSE2)
            // Use unaligned store for better flexibility, and _mm_set1_ps is more commonly used
            // than _mm_set_ps1
            _mm_storeu_ps(&result.x, _mm_set1_ps(f));
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                // Optimal for float type using temporary array
                float temp[4];
                float32x4_t v = vdupq_n_f32(f);
                vst1q_f32(temp, v);
                std::memcpy(&result.x, temp, sizeof(float) * 3);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                // Support for integer type using temporary array
                int32_t temp[4];
                int32x4_t v = vdupq_n_s32(f);
                vst1q_s32(temp, v);
                std::memcpy(&result.x, temp, sizeof(int32_t) * 3);
            }
#endif
            return result;
        } else {
            return Vec3(f, f, f);
        }
    }

    [[nodiscard]] constexpr Vec3<decltype(T{} + T{})> operator+(const Vec3& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec3<T> result;
#if defined(HAVE_SSE2)
            // Use unaligned loads/stores for better flexibility
            __m128 a = _mm_loadu_ps(&x);
            __m128 b = _mm_loadu_ps(&other.x);
            _mm_storeu_ps(&result.x, _mm_add_ps(a, b));
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                // Optimal for float type using temporary arrays
                float temp_a[4], temp_b[4], temp_result[4];

                // Safely copy input vectors to temporary arrays
                std::memcpy(temp_a, &x, sizeof(float) * 3);
                std::memcpy(temp_b, &other.x, sizeof(float) * 3);

                // Perform NEON vector addition
                float32x4_t a = vld1q_f32(temp_a);
                float32x4_t b = vld1q_f32(temp_b);
                vst1q_f32(temp_result, vaddq_f32(a, b));

                // Safely copy result back
                std::memcpy(&result.x, temp_result, sizeof(float) * 3);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                // Support for integer type using temporary arrays
                int32_t temp_a[4], temp_b[4], temp_result[4];

                // Safely copy input vectors to temporary arrays
                std::memcpy(temp_a, &x, sizeof(int32_t) * 3);
                std::memcpy(temp_b, &other.x, sizeof(int32_t) * 3);

                // Perform NEON vector addition
                int32x4_t a = vld1q_s32(temp_a);
                int32x4_t b = vld1q_s32(temp_b);
                vst1q_s32(temp_result, vaddq_s32(a, b));

                // Safely copy result back
                std::memcpy(&result.x, temp_result, sizeof(int32_t) * 3);
            }
#endif
            return result;
        } else {
            return {x + other.x, y + other.y, z + other.z};
        }
    }

    [[nodiscard]] constexpr Vec3& operator+=(const Vec3& other) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            // Use unaligned loads/stores for better flexibility
            __m128 a = _mm_loadu_ps(&x);
            __m128 b = _mm_loadu_ps(&other.x);
            _mm_storeu_ps(&x, _mm_add_ps(a, b));
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                // Optimal for float type using temporary arrays
                float temp_a[4], temp_b[4], temp_result[4];

                // Safely copy input vectors to temporary arrays
                std::memcpy(temp_a, &x, sizeof(float) * 3);
                std::memcpy(temp_b, &other.x, sizeof(float) * 3);

                // Perform NEON vector addition
                float32x4_t a = vld1q_f32(temp_a);
                float32x4_t b = vld1q_f32(temp_b);
                vst1q_f32(temp_result, vaddq_f32(a, b));

                // Safely copy result back to this object
                std::memcpy(&x, temp_result, sizeof(float) * 3);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                // Support for integer type using temporary arrays
                int32_t temp_a[4], temp_b[4], temp_result[4];

                // Safely copy input vectors to temporary arrays
                std::memcpy(temp_a, &x, sizeof(int32_t) * 3);
                std::memcpy(temp_b, &other.x, sizeof(int32_t) * 3);

                // Perform NEON vector addition
                int32x4_t a = vld1q_s32(temp_a);
                int32x4_t b = vld1q_s32(temp_b);
                vst1q_s32(temp_result, vaddq_s32(a, b));

                // Safely copy result back to this object
                std::memcpy(&x, temp_result, sizeof(int32_t) * 3);
            }
#endif
        } else {
            x += other.x;
            y += other.y;
            z += other.z;
        }
        return *this;
    }

    [[nodiscard]] constexpr Vec3<decltype(T{} - T{})> operator-(const Vec3& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec3<T> result;
#if defined(HAVE_SSE2)
            // Use unaligned loads/stores for better flexibility
            __m128 a = _mm_loadu_ps(&x);
            __m128 b = _mm_loadu_ps(&other.x);
            _mm_storeu_ps(&result.x, _mm_sub_ps(a, b));
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                // Optimal for float type using temporary arrays
                float temp_a[4], temp_b[4], temp_result[4];

                // Safely copy input vectors to temporary arrays
                std::memcpy(temp_a, &x, sizeof(float) * 3);
                std::memcpy(temp_b, &other.x, sizeof(float) * 3);

                // Perform NEON vector subtraction
                float32x4_t a = vld1q_f32(temp_a);
                float32x4_t b = vld1q_f32(temp_b);
                vst1q_f32(temp_result, vsubq_f32(a, b));

                // Safely copy result back
                std::memcpy(&result.x, temp_result, sizeof(float) * 3);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                // Support for integer type using temporary arrays
                int32_t temp_a[4], temp_b[4], temp_result[4];

                // Safely copy input vectors to temporary arrays
                std::memcpy(temp_a, &x, sizeof(int32_t) * 3);
                std::memcpy(temp_b, &other.x, sizeof(int32_t) * 3);

                // Perform NEON vector subtraction
                int32x4_t a = vld1q_s32(temp_a);
                int32x4_t b = vld1q_s32(temp_b);
                vst1q_s32(temp_result, vsubq_s32(a, b));

                // Safely copy result back
                std::memcpy(&result.x, temp_result, sizeof(int32_t) * 3);
            }
#endif
            return result;
        } else {
            return {x - other.x, y - other.y, z - other.z};
        }
    }

    [[nodiscard]] constexpr Vec3& operator-=(const Vec3& other) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            // Use unaligned loads/stores for better flexibility
            __m128 a = _mm_loadu_ps(&x);
            __m128 b = _mm_loadu_ps(&other.x);
            _mm_storeu_ps(&x, _mm_sub_ps(a, b));
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                // Optimal for float type using temporary arrays
                float temp_a[4], temp_b[4], temp_result[4];

                // Safely copy input vectors to temporary arrays
                std::memcpy(temp_a, &x, sizeof(float) * 3);
                std::memcpy(temp_b, &other.x, sizeof(float) * 3);

                // Perform NEON vector subtraction
                float32x4_t a = vld1q_f32(temp_a);
                float32x4_t b = vld1q_f32(temp_b);
                vst1q_f32(temp_result, vsubq_f32(a, b));

                // Safely copy result back to this object
                std::memcpy(&x, temp_result, sizeof(float) * 3);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                // Support for integer type using temporary arrays
                int32_t temp_a[4], temp_b[4], temp_result[4];

                // Safely copy input vectors to temporary arrays
                std::memcpy(temp_a, &x, sizeof(int32_t) * 3);
                std::memcpy(temp_b, &other.x, sizeof(int32_t) * 3);

                // Perform NEON vector subtraction
                int32x4_t a = vld1q_s32(temp_a);
                int32x4_t b = vld1q_s32(temp_b);
                vst1q_s32(temp_result, vsubq_s32(a, b));

                // Safely copy result back to this object
                std::memcpy(&x, temp_result, sizeof(int32_t) * 3);
            }
#endif
        } else {
            x -= other.x;
            y -= other.y;
            z -= other.z;
        }
        return *this;
    }

    template <typename U = T>
    [[nodiscard]] constexpr Vec3<std::enable_if_t<std::is_signed_v<U>, U>> operator-() const {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                Vec3<float> result;
                // Use bitwise XOR with sign bit to negate efficiently
                const __m128 sign_mask = _mm_set1_ps(-0.0f); // 0x80000000
                __m128 v = _mm_load_ps(&x);
                _mm_store_ps(&result.x, _mm_xor_ps(v, sign_mask));
                return result;
            } else if constexpr (std::is_integral_v<T>) {
                Vec3<T> result;
                if constexpr (sizeof(T) <= 2) {
                    // For 16-bit integers
                    __m128i v = detail::vec3_to_epi16(
                        static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(z));
                    __m128i neg = _mm_sub_epi16(_mm_setzero_si128(), v);
                    result.x = static_cast<T>(_mm_extract_epi16(neg, 0));
                    result.y = static_cast<T>(_mm_extract_epi16(neg, 1));
                    result.z = static_cast<T>(_mm_extract_epi16(neg, 2));
                } else {
                    // For 32-bit integers
                    __m128i v = _mm_setr_epi32(static_cast<int32_t>(x), static_cast<int32_t>(y),
                                               static_cast<int32_t>(z), 0);
                    __m128i neg = _mm_sub_epi32(_mm_setzero_si128(), v);
                    result.x = static_cast<T>(_mm_cvtsi128_si32(neg));
                    result.y = static_cast<T>(_mm_extract_epi32(neg, 1));
                    result.z = static_cast<T>(_mm_extract_epi32(neg, 2));
                }
                return result;
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                Vec3<float> result;
                float32x4_t v = vld1q_f32(&x);
                // Negate using NEON vector negation
                float32x4_t neg = vnegq_f32(v);
                vst1q_f32(&result.x, neg);
                return result;
            } else if constexpr (std::is_integral_v<T>) {
                Vec3<T> result;
                if constexpr (sizeof(T) <= 2) {
                    // For 16-bit integers
                    int16x4_t v = {static_cast<int16_t>(x), static_cast<int16_t>(y),
                                   static_cast<int16_t>(z), 0};
                    int16x4_t neg = vneg_s16(v);
                    result.x = static_cast<T>(vget_lane_s16(neg, 0));
                    result.y = static_cast<T>(vget_lane_s16(neg, 1));
                    result.z = static_cast<T>(vget_lane_s16(neg, 2));
                } else {
                    // For 32-bit integers
                    int32x4_t v = {static_cast<int32_t>(x), static_cast<int32_t>(y),
                                   static_cast<int32_t>(z), 0};
                    int32x4_t neg = vnegq_s32(v);
                    result.x = static_cast<T>(vgetq_lane_s32(neg, 0));
                    result.y = static_cast<T>(vgetq_lane_s32(neg, 1));
                    result.z = static_cast<T>(vgetq_lane_s32(neg, 2));
                }
                return result;
            }
#endif
        }
        // Fallback for non-vectorizable types
        return {-x, -y, -z};
    }

    [[nodiscard]] constexpr Vec3<decltype(T{} * T{})> operator*(const Vec3& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec3<T> result;
#if defined(HAVE_SSE2)
            // Use unaligned loads/stores for better flexibility
            __m128 a = _mm_loadu_ps(&x);
            __m128 b = _mm_loadu_ps(&other.x);
            _mm_storeu_ps(&result.x, _mm_mul_ps(a, b));
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                // Optimal for float type using temporary arrays
                float temp_a[4], temp_b[4], temp_result[4];

                // Safely copy input vectors to temporary arrays
                std::memcpy(temp_a, &x, sizeof(float) * 3);
                std::memcpy(temp_b, &other.x, sizeof(float) * 3);

                // Perform NEON vector multiplication
                float32x4_t a = vld1q_f32(temp_a);
                float32x4_t b = vld1q_f32(temp_b);
                vst1q_f32(temp_result, vmulq_f32(a, b));

                // Safely copy result back
                std::memcpy(&result.x, temp_result, sizeof(float) * 3);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                // Support for integer type using temporary arrays
                int32_t temp_a[4], temp_b[4], temp_result[4];

                // Safely copy input vectors to temporary arrays
                std::memcpy(temp_a, &x, sizeof(int32_t) * 3);
                std::memcpy(temp_b, &other.x, sizeof(int32_t) * 3);

                // Perform NEON vector multiplication
                int32x4_t a = vld1q_s32(temp_a);
                int32x4_t b = vld1q_s32(temp_b);
                vst1q_s32(temp_result, vmulq_s32(a, b));

                // Safely copy result back
                std::memcpy(&result.x, temp_result, sizeof(int32_t) * 3);
            }
#endif
            return result;
        } else {
            return {x * other.x, y * other.y, z * other.z};
        }
    }

    template <typename V>
    [[nodiscard]] constexpr Vec3<decltype(T{} * V{})> operator*(const V& f) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec3<decltype(T{} * V{})> result;
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<V, float>) {
                // Optimal for float scalar multiplication
                __m128 vec = _mm_loadu_ps(&x);
                __m128 scalar = _mm_set1_ps(f);
                _mm_storeu_ps(&result.x, _mm_mul_ps(vec, scalar));
            } else if constexpr (std::is_same_v<V, int32_t>) {
                // Support for integer scalar multiplication using float conversion
                __m128 vec = _mm_loadu_ps(&x);
                __m128 scalar = _mm_set1_ps(static_cast<float>(f));
                _mm_storeu_ps(&result.x, _mm_mul_ps(vec, scalar));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<V, float>) {
                // Optimal for float scalar multiplication using temporary arrays
                float temp_vec[4], temp_result[4];

                // Safely copy input vector to temporary array
                std::memcpy(temp_vec, &x, sizeof(float) * 3);

                // Perform NEON vector-scalar multiplication
                float32x4_t vec = vld1q_f32(temp_vec);
                float32x4_t scalar = vdupq_n_f32(f);
                vst1q_f32(temp_result, vmulq_f32(vec, scalar));

                // Safely copy result back
                std::memcpy(&result.x, temp_result, sizeof(float) * 3);
            } else if constexpr (std::is_same_v<V, int32_t>) {
                if constexpr (std::is_same_v<T, float>) {
                    // Float vector * int scalar using temporary arrays
                    float temp_vec[4], temp_result[4];

                    // Safely copy input vector to temporary array
                    std::memcpy(temp_vec, &x, sizeof(float) * 3);

                    // Perform NEON vector-scalar multiplication with float conversion
                    float32x4_t vec = vld1q_f32(temp_vec);
                    float32x4_t scalar = vdupq_n_f32(static_cast<float>(f));
                    vst1q_f32(temp_result, vmulq_f32(vec, scalar));

                    // Safely copy result back
                    std::memcpy(&result.x, temp_result, sizeof(float) * 3);
                } else if constexpr (std::is_same_v<T, int32_t>) {
                    // Int vector * int scalar using temporary arrays
                    int32_t temp_vec[4], temp_result[4];

                    // Safely copy input vector to temporary array
                    std::memcpy(temp_vec, &x, sizeof(int32_t) * 3);

                    // Perform NEON vector-scalar multiplication
                    int32x4_t vec = vld1q_s32(temp_vec);
                    int32x4_t scalar = vdupq_n_s32(f);
                    vst1q_s32(temp_result, vmulq_s32(vec, scalar));

                    // Safely copy result back
                    std::memcpy(&result.x, temp_result, sizeof(int32_t) * 3);
                }
            }
#endif
            return result;
        } else {
            return {x * f, y * f, z * f};
        }
    }

    template <typename V>
    [[nodiscard]] constexpr Vec3& operator*=(const V& f) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<V, float>) {
                // Optimal for float scalar multiplication
                __m128 vec = _mm_loadu_ps(&x);
                __m128 scalar = _mm_set1_ps(f);
                _mm_storeu_ps(&x, _mm_mul_ps(vec, scalar));
            } else if constexpr (std::is_same_v<V, int32_t>) {
                // Support for integer scalar multiplication using float conversion
                __m128 vec = _mm_loadu_ps(&x);
                __m128 scalar = _mm_set1_ps(static_cast<float>(f));
                _mm_storeu_ps(&x, _mm_mul_ps(vec, scalar));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<V, float>) {
                // Optimal for float scalar multiplication using temporary arrays
                float temp_vec[4], temp_result[4];

                // Safely copy input vector to temporary array
                std::memcpy(temp_vec, &x, sizeof(float) * 3);

                // Perform NEON vector-scalar multiplication
                float32x4_t vec = vld1q_f32(temp_vec);
                float32x4_t scalar = vdupq_n_f32(f);
                vst1q_f32(temp_result, vmulq_f32(vec, scalar));

                // Safely copy result back to this object
                std::memcpy(&x, temp_result, sizeof(float) * 3);
            } else if constexpr (std::is_same_v<V, int32_t>) {
                if constexpr (std::is_same_v<T, float>) {
                    // Float vector * int scalar using temporary arrays
                    float temp_vec[4], temp_result[4];

                    // Safely copy input vector to temporary array
                    std::memcpy(temp_vec, &x, sizeof(float) * 3);

                    // Perform NEON vector-scalar multiplication with float conversion
                    float32x4_t vec = vld1q_f32(temp_vec);
                    float32x4_t scalar = vdupq_n_f32(static_cast<float>(f));
                    vst1q_f32(temp_result, vmulq_f32(vec, scalar));

                    // Safely copy result back to this object
                    std::memcpy(&x, temp_result, sizeof(float) * 3);
                } else if constexpr (std::is_same_v<T, int32_t>) {
                    // Int vector * int scalar using temporary arrays
                    int32_t temp_vec[4], temp_result[4];

                    // Safely copy input vector to temporary array
                    std::memcpy(temp_vec, &x, sizeof(int32_t) * 3);

                    // Perform NEON vector-scalar multiplication
                    int32x4_t vec = vld1q_s32(temp_vec);
                    int32x4_t scalar = vdupq_n_s32(f);
                    vst1q_s32(temp_result, vmulq_s32(vec, scalar));

                    // Safely copy result back to this object
                    std::memcpy(&x, temp_result, sizeof(int32_t) * 3);
                }
            }
#endif
        } else {
            x *= f;
            y *= f;
            z *= f;
        }
        return *this;
    }

    template <typename V>
    [[nodiscard]] constexpr Vec3<decltype(T{} / V{})> operator/(const V& f) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec3<decltype(T{} / V{})> result;
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<V, float>) {
                // Optimal for float scalar division
                __m128 vec = _mm_loadu_ps(&x);
                __m128 scalar = _mm_set1_ps(f);
                _mm_storeu_ps(&result.x, _mm_div_ps(vec, scalar));
            } else if constexpr (std::is_same_v<V, int32_t>) {
                // Support for integer scalar division using float conversion
                __m128 vec = _mm_loadu_ps(&x);
                __m128 scalar = _mm_set1_ps(static_cast<float>(f));
                _mm_storeu_ps(&result.x, _mm_div_ps(vec, scalar));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<V, float>) {
                // NEON optimized float division using reciprocal approximation
                float32x4_t vec = vld1q_f32(&x);
                float32x4_t recip = vrecpeq_f32(vdupq_n_f32(f));
                // Two Newton-Raphson iterations for better precision
                recip = vmulq_f32(vrecpsq_f32(vdupq_n_f32(f), recip), recip);
                recip = vmulq_f32(vrecpsq_f32(vdupq_n_f32(f), recip), recip);
                vst1q_f32(&result.x, vmulq_f32(vec, recip));
            } else if constexpr (std::is_same_v<V, int32_t>) {
                if constexpr (std::is_same_v<T, float>) {
                    // Float vector / int scalar
                    float32x4_t vec = vld1q_f32(&x);
                    float32x4_t recip = vrecpeq_f32(vdupq_n_f32(static_cast<float>(f)));
                    // Two Newton-Raphson iterations
                    recip =
                        vmulq_f32(vrecpsq_f32(vdupq_n_f32(static_cast<float>(f)), recip), recip);
                    recip =
                        vmulq_f32(vrecpsq_f32(vdupq_n_f32(static_cast<float>(f)), recip), recip);
                    vst1q_f32(&result.x, vmulq_f32(vec, recip));
                }
                // Note: Integer division is not optimized with NEON as it's typically not
                // beneficial
            }
#endif
            return result;
        } else {
            return {x / f, y / f, z / f};
        }
    }

    template <typename V>
    [[nodiscard]] constexpr Vec3& operator/=(const V& f) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<V, float>) {
                // Optimal for float scalar division
                __m128 vec = _mm_loadu_ps(&x);
                __m128 scalar = _mm_set1_ps(f);
                _mm_storeu_ps(&x, _mm_div_ps(vec, scalar));
            } else if constexpr (std::is_same_v<V, int32_t>) {
                // Support for integer scalar division using float conversion
                __m128 vec = _mm_loadu_ps(&x);
                __m128 scalar = _mm_set1_ps(static_cast<float>(f));
                _mm_storeu_ps(&x, _mm_div_ps(vec, scalar));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<V, float>) {
                // NEON optimized float division using reciprocal approximation
                float32x4_t vec = vld1q_f32(&x);
                float32x4_t recip = vrecpeq_f32(vdupq_n_f32(f));
                // Two Newton-Raphson iterations for better precision
                recip = vmulq_f32(vrecpsq_f32(vdupq_n_f32(f), recip), recip);
                recip = vmulq_f32(vrecpsq_f32(vdupq_n_f32(f), recip), recip);
                vst1q_f32(&x, vmulq_f32(vec, recip));
            } else if constexpr (std::is_same_v<V, int32_t>) {
                if constexpr (std::is_same_v<T, float>) {
                    // Float vector / int scalar
                    float32x4_t vec = vld1q_f32(&x);
                    float32x4_t recip = vrecpeq_f32(vdupq_n_f32(static_cast<float>(f)));
                    // Two Newton-Raphson iterations
                    recip =
                        vmulq_f32(vrecpsq_f32(vdupq_n_f32(static_cast<float>(f)), recip), recip);
                    recip =
                        vmulq_f32(vrecpsq_f32(vdupq_n_f32(static_cast<float>(f)), recip), recip);
                    vst1q_f32(&x, vmulq_f32(vec, recip));
                }
                // Note: Integer division is not optimized with NEON as it's typically not
                // beneficial
            }
#endif
        } else {
            x /= f;
            y /= f;
            z /= f;
        }
        return *this;
    }

    [[nodiscard]] constexpr bool operator!=(const Vec3& other) const {
        return !(*this == other);
    }

    [[nodiscard]] constexpr bool operator==(const Vec3& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                // Load vectors using unaligned loads for better flexibility
                __m128 a = _mm_loadu_ps(&x);
                __m128 b = _mm_loadu_ps(&other.x);
                // Compare for equality
                __m128 cmp = _mm_cmpeq_ps(a, b);
                // Check only x,y,z components (mask 0x7)
                return (_mm_movemask_ps(cmp) & 0x7) == 0x7;
            } else if constexpr (std::is_same_v<T, int32_t>) {
                // Integer comparison
                __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&x));
                __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&other.x));
                __m128i cmp = _mm_cmpeq_epi32(a, b);
                // Check only x,y,z components (mask 0x7)
                return (_mm_movemask_ps(_mm_castsi128_ps(cmp)) & 0x7) == 0x7;
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                // Load vectors using temporary arrays
                float temp_a[4], temp_b[4];
                std::memcpy(temp_a, &x, sizeof(float) * 3);
                std::memcpy(temp_b, &other.x, sizeof(float) * 3);

                // Compare for equality
                float32x4_t a = vld1q_f32(temp_a);
                float32x4_t b = vld1q_f32(temp_b);
                uint32x4_t cmp = vceqq_f32(a, b);

                // Use AND reduction for first 3 components only
                return ((vgetq_lane_u32(cmp, 0) & vgetq_lane_u32(cmp, 1) &
                         vgetq_lane_u32(cmp, 2)) == 0xFFFFFFFF);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                // Integer comparison using temporary arrays
                int32_t temp_a[4], temp_b[4];
                std::memcpy(temp_a, &x, sizeof(int32_t) * 3);
                std::memcpy(temp_b, &other.x, sizeof(int32_t) * 3);

                // Compare for equality
                int32x4_t a = vld1q_s32(temp_a);
                int32x4_t b = vld1q_s32(temp_b);
                uint32x4_t cmp = vceqq_s32(a, b);

                // Use AND reduction for first 3 components only
                return ((vgetq_lane_u32(cmp, 0) & vgetq_lane_u32(cmp, 1) &
                         vgetq_lane_u32(cmp, 2)) == 0xFFFFFFFF);
            }
#endif
        }
        // Fallback to scalar comparison
        return x == other.x && y == other.y && z == other.z;
    }

    [[nodiscard]] constexpr T Length2() const {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                // Load vector using unaligned load for better flexibility
                __m128 v = _mm_loadu_ps(&x);
                // Square all components
                __m128 sq = _mm_mul_ps(v, v);

                // Horizontal add for x+y+z components (ignoring w)
                // Using more efficient shuffling pattern
                __m128 sum = sq;
                sum = _mm_add_ps(sum, _mm_movehl_ps(sum, sum));     // Add z to x
                sum = _mm_add_ss(sum, _mm_shuffle_ps(sum, sum, 1)); // Add y

                return _mm_cvtss_f32(sum);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                // Integer version using SSE2
                __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&x));
                __m128i sq = _mm_mullo_epi32(v, v); // SSE4.1, fallback to scalar if not available

                // Extract and sum components
                int32_t sum = _mm_cvtsi128_si32(sq) +                       // x
                              _mm_cvtsi128_si32(_mm_shuffle_epi32(sq, 1)) + // y
                              _mm_cvtsi128_si32(_mm_shuffle_epi32(sq, 2));  // z

                return sum;
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                // Load and square components using temporary array
                float temp[4];
                std::memcpy(temp, &x, sizeof(float) * 3);

                float32x4_t v = vld1q_f32(temp);
                float32x4_t sq = vmulq_f32(v, v);

                // Efficient horizontal add using NEON intrinsics
                float32x2_t sum = vget_low_f32(sq);     // Get x,y
                sum = vadd_f32(sum, vget_high_f32(sq)); // Add z,w (only need z)
                sum = vpadd_f32(sum, vdup_n_f32(0.0f)); // Horizontal add x+y

                return vget_lane_f32(sum, 0) + vget_lane_f32(vget_high_f32(sq), 0); // Add z
            } else if constexpr (std::is_same_v<T, int32_t>) {
                // Integer version using NEON with safe memory handling
                int32_t temp[4];
                std::memcpy(temp, &x, sizeof(int32_t) * 3);

                int32x4_t v = vld1q_s32(temp);
                int32x4_t sq = vmulq_s32(v, v);

                // Horizontal add for integers
                int32x2_t sum = vget_low_s32(sq);       // Get x,y
                sum = vadd_s32(sum, vget_high_s32(sq)); // Add z
                sum = vpadd_s32(sum, vdup_n_s32(0));    // Horizontal add

                return vget_lane_s32(sum, 0);
            }
#endif
        }
        // Fallback to scalar calculation
        return x * x + y * y + z * z;
    }

    // Only implemented for T=float
    [[nodiscard]] float Length() const;
    [[nodiscard]] Vec3 Normalized() const;
    float Normalize(); // returns the previous length, which is often useful

    [[nodiscard]] constexpr T& operator[](std::size_t i) {
        assert(i < dimension && "Vector index out of bounds");
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            // For Vec4 with SIMD member
            if constexpr (detail::has_simd_member<decltype(*this)>::value) {
                // Use std::array as a type-safe container for aligned storage
                alignas(16) std::array<T, 4>& elements =
                    *std::launder(reinterpret_cast<std::array<T, 4>*>(&simd));
                return elements[i];
            }
#elif defined(HAVE_NEON)
            // For Vec4 with SIMD member
            if constexpr (detail::has_simd_member<decltype(*this)>::value) {
                // Use std::array as a type-safe container
                std::array<T, 4>& elements =
                    *std::launder(reinterpret_cast<std::array<T, 4>*>(&simd));
                return elements[i];
            }
#endif
        }

        // Use a union for safe access to the components
        union Components {
            struct {
                T x, y, z;
            };
            std::array<T, 3> arr;
        };
        Components& c = *std::launder(reinterpret_cast<Components*>(this));
        return c.arr[i];
    }

    [[nodiscard]] constexpr const T& operator[](std::size_t i) const {
        assert(i < 4 && "Index out of bounds in Vec4");
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            // For Vec4 with SIMD member
            if constexpr (std::is_same_v<T, float>) {
                // Use aligned array to handle SIMD data
                struct alignas(16) AlignedArray {
                    T data[4];
                };
                const auto& elements = *std::launder(reinterpret_cast<const AlignedArray*>(&simd));
                return elements.data[i];
            } else if constexpr (std::is_integral_v<T>) {
                if constexpr (sizeof(T) <= 2) {
                    // 16-bit integers
                    struct alignas(16) AlignedArray {
                        T data[4];
                    };
                    const auto& elements =
                        *std::launder(reinterpret_cast<const AlignedArray*>(&simd));
                    return elements.data[i];
                } else {
                    // 32-bit integers
                    struct alignas(16) AlignedArray {
                        T data[4];
                    };
                    const auto& elements =
                        *std::launder(reinterpret_cast<const AlignedArray*>(&simd));
                    return elements.data[i];
                }
            }
#elif defined(HAVE_NEON)
            // For Vec4 with SIMD member
            if constexpr (detail::has_simd_member<decltype(*this)>::value) {
                // Use union to handle SIMD data safely
                union AlignedAccess {
                    float32x4_t simd;
                    T data[4];
                };
                const auto& elements = *std::launder(reinterpret_cast<const AlignedAccess*>(this));
                return elements.data[i];
            }
#endif
        }

        // Use a union for safe access to components
        union Components {
            struct {
                T x, y, z, w;
            };
            T data[4];
        };
        const auto& c = *std::launder(reinterpret_cast<const Components*>(this));
        return c.data[i];
    }

    // Common aliases: UVW (texel coordinates), RGB (colors), STQ (texture coordinates)
    [[nodiscard]] constexpr T& u() {
        return x;
    }
    [[nodiscard]] constexpr T& v() {
        return y;
    }
    [[nodiscard]] constexpr T& w() {
        return z;
    }

    [[nodiscard]] constexpr T& r() {
        return x;
    }
    [[nodiscard]] constexpr T& g() {
        return y;
    }
    [[nodiscard]] constexpr T& b() {
        return z;
    }

    [[nodiscard]] constexpr T& s() {
        return x;
    }
    [[nodiscard]] constexpr T& t() {
        return y;
    }
    [[nodiscard]] constexpr T& q() {
        return z;
    }

    [[nodiscard]] constexpr const T& u() const {
        return x;
    }
    [[nodiscard]] constexpr const T& v() const {
        return y;
    }
    [[nodiscard]] constexpr const T& w() const {
        return z;
    }

    [[nodiscard]] constexpr const T& r() const {
        return x;
    }
    [[nodiscard]] constexpr const T& g() const {
        return y;
    }
    [[nodiscard]] constexpr const T& b() const {
        return z;
    }

    [[nodiscard]] constexpr const T& s() const {
        return x;
    }
    [[nodiscard]] constexpr const T& t() const {
        return y;
    }
    [[nodiscard]] constexpr const T& q() const {
        return z;
    }

// swizzlers - create a subvector of specific components
// e.g. Vec2 uv() { return Vec2(x,y); }
// _DEFINE_SWIZZLER2 defines a single such function, DEFINE_SWIZZLER2 defines all of them for all
// component names (x<->r) and permutations (xy<->yx)
#define _DEFINE_SWIZZLER2(a, b, name)                                                              \
    [[nodiscard]] constexpr Vec2<T> name() const {                                                 \
        return Vec2<T>(a, b);                                                                      \
    }
#define DEFINE_SWIZZLER2(a, b, a2, b2, a3, b3, a4, b4)                                             \
    _DEFINE_SWIZZLER2(a, b, a##b);                                                                 \
    _DEFINE_SWIZZLER2(a, b, a2##b2);                                                               \
    _DEFINE_SWIZZLER2(a, b, a3##b3);                                                               \
    _DEFINE_SWIZZLER2(a, b, a4##b4);                                                               \
    _DEFINE_SWIZZLER2(b, a, b##a);                                                                 \
    _DEFINE_SWIZZLER2(b, a, b2##a2);                                                               \
    _DEFINE_SWIZZLER2(b, a, b3##a3);                                                               \
    _DEFINE_SWIZZLER2(b, a, b4##a4)

    DEFINE_SWIZZLER2(x, y, r, g, u, v, s, t);
    DEFINE_SWIZZLER2(x, z, r, b, u, w, s, q);
    DEFINE_SWIZZLER2(y, z, g, b, v, w, t, q);
#undef DEFINE_SWIZZLER2
#undef _DEFINE_SWIZZLER2
};

template <typename T, typename V>
[[nodiscard]] constexpr Vec3<T> operator*(const V& f, const Vec3<T>& vec) {
    if constexpr (detail::is_vectorizable<T>::value) {
        Vec3<T> result;
#if defined(HAVE_SSE2)
        if constexpr (std::is_same_v<V, float>) {
            // Optimal for float scalar multiplication
            __m128 scalar = _mm_set1_ps(f);
            __m128 vector = _mm_loadu_ps(&vec.x);
            _mm_storeu_ps(&result.x, _mm_mul_ps(scalar, vector));
        } else if constexpr (std::is_same_v<V, int32_t>) {
            if constexpr (std::is_same_v<T, float>) {
                // Integer scalar * float vector
                __m128 scalar = _mm_set1_ps(static_cast<float>(f));
                __m128 vector = _mm_loadu_ps(&vec.x);
                _mm_storeu_ps(&result.x, _mm_mul_ps(scalar, vector));
            } else if constexpr (std::is_same_v<T, int32_t>) {
                // Integer scalar * integer vector
                __m128i scalar = _mm_set1_epi32(f);
                __m128i vector = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&vec.x));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&result.x),
                                 _mm_mullo_epi32(scalar, vector));
            }
        }
#elif defined(HAVE_NEON)
        if constexpr (std::is_same_v<V, float>) {
            // Optimal for float scalar multiplication using temporary arrays
            float temp_vec[4], temp_result[4];
            std::memcpy(temp_vec, &vec.x, sizeof(float) * 3);

            float32x4_t scalar = vdupq_n_f32(f);
            float32x4_t vector = vld1q_f32(temp_vec);
            vst1q_f32(temp_result, vmulq_f32(scalar, vector));

            std::memcpy(&result.x, temp_result, sizeof(float) * 3);
        } else if constexpr (std::is_same_v<V, int32_t>) {
            if constexpr (std::is_same_v<T, float>) {
                // Integer scalar * float vector using temporary arrays
                float temp_vec[4], temp_result[4];
                std::memcpy(temp_vec, &vec.x, sizeof(float) * 3);

                float32x4_t scalar = vdupq_n_f32(static_cast<float>(f));
                float32x4_t vector = vld1q_f32(temp_vec);
                vst1q_f32(temp_result, vmulq_f32(scalar, vector));

                std::memcpy(&result.x, temp_result, sizeof(float) * 3);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                // Integer scalar * integer vector using temporary arrays
                int32_t temp_vec[4], temp_result[4];
                std::memcpy(temp_vec, &vec.x, sizeof(int32_t) * 3);

                int32x4_t scalar = vdupq_n_s32(f);
                int32x4_t vector = vld1q_s32(temp_vec);
                vst1q_s32(temp_result, vmulq_s32(scalar, vector));

                std::memcpy(&result.x, temp_result, sizeof(int32_t) * 3);
            }
        }
#endif
        return result;
    } else {
        return Vec3<T>(f * vec.x, f * vec.y, f * vec.z);
    }
}

template <>
inline float Vec3<float>::Length() const {
#if defined(HAVE_AVX)
    __m256 v = _mm256_setr_ps(x, y, z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    __m256 sq = _mm256_mul_ps(v, v);
    __m256 sum = _mm256_hadd_ps(sq, sq);
    sum = _mm256_hadd_ps(sum, sum);
    return _mm_cvtss_f32(_mm256_extractf128_ps(sum, 0));
#elif defined(HAVE_SSE2)
    // Load vector using unaligned load for better flexibility
    __m128 v = _mm_loadu_ps(&x);
    // Square all components
    __m128 sq = _mm_mul_ps(v, v);

    // More efficient horizontal add for x+y+z components
    __m128 sum = sq;
    sum = _mm_add_ps(sum, _mm_movehl_ps(sum, sum));     // Add z to x
    sum = _mm_add_ss(sum, _mm_shuffle_ps(sum, sum, 1)); // Add y

    // Calculate square root of sum
    return _mm_cvtss_f32(_mm_sqrt_ss(sum));
#elif defined(HAVE_NEON)
    // Load and square components
    float32x4_t v = vld1q_f32(&x);
    float32x4_t sq = vmulq_f32(v, v);

    // Efficient horizontal add using NEON intrinsics
    float32x2_t sum = vget_low_f32(sq);     // Get x,y
    sum = vadd_f32(sum, vget_high_f32(sq)); // Add z,w (only need z)
    sum = vpadd_f32(sum, vdup_n_f32(0.0f)); // Horizontal add x+y
    float32x2_t result = vrsqrte_f32(sum);  // Approximate 1/sqrt

    // One Newton-Raphson iteration for better precision
    result = vmul_f32(vrsqrts_f32(vmul_f32(sum, result), result), result);

    // Final multiplication to get sqrt
    return vget_lane_f32(vmul_f32(sum, result), 0);
#else
    return std::sqrt(x * x + y * y + z * z);
#endif
}

template <>
inline Vec3<float> Vec3<float>::Normalized() const {
#if defined(HAVE_SSE2)
    // Load vector using unaligned load for better flexibility
    __m128 v = _mm_loadu_ps(&x);
    // Square all components
    __m128 sq = _mm_mul_ps(v, v);

    // Efficient horizontal add for x+y+z components
    __m128 sum = sq;
    sum = _mm_add_ps(sum, _mm_movehl_ps(sum, sum));     // Add z to x
    sum = _mm_add_ss(sum, _mm_shuffle_ps(sum, sum, 1)); // Add y

    // Compute reciprocal sqrt directly (faster than sqrt + div)
    __m128 rsqrt = _mm_rsqrt_ps(_mm_shuffle_ps(sum, sum, 0));

    // One Newton-Raphson iteration for better precision
    // y = rsqrt * (1.5f - 0.5f * x * rsqrt * rsqrt)
    __m128 three_halves = _mm_set1_ps(1.5f);
    __m128 half = _mm_set1_ps(0.5f);
    __m128 rsqrt_sq = _mm_mul_ps(rsqrt, rsqrt);
    __m128 half_x_rsqrt_sq = _mm_mul_ps(_mm_mul_ps(half, _mm_shuffle_ps(sum, sum, 0)), rsqrt_sq);
    rsqrt = _mm_mul_ps(rsqrt, _mm_sub_ps(three_halves, half_x_rsqrt_sq));

    Vec3<float> result;
    _mm_storeu_ps(&result.x, _mm_mul_ps(v, rsqrt));
    return result;

#elif defined(HAVE_NEON)
    // Load and square components
    float32x4_t v = vld1q_f32(&x);
    float32x4_t sq = vmulq_f32(v, v);

    // Efficient horizontal add
    float32x2_t sum = vget_low_f32(sq);     // Get x,y
    sum = vadd_f32(sum, vget_high_f32(sq)); // Add z,w (only need z)
    sum = vpadd_f32(sum, vdup_n_f32(0.0f)); // Horizontal add x+y+z

    // Use NEON reciprocal square root approximation
    float32x2_t rsqrt = vrsqrte_f32(sum);

    // Two Newton-Raphson iterations for better precision
    rsqrt = vmul_f32(vrsqrts_f32(vmul_f32(sum, rsqrt), rsqrt), rsqrt);
    rsqrt = vmul_f32(vrsqrts_f32(vmul_f32(sum, rsqrt), rsqrt), rsqrt);

    // Broadcast reciprocal sqrt to all lanes and multiply
    float32x4_t normalized = vmulq_f32(v, vdupq_lane_f32(rsqrt, 0));

    Vec3<float> result;
    vst1q_f32(&result.x, normalized);
    return result;

#else
    float length = Length();
    if (length > 0.0f) {
        float recip = 1.0f / length;
        return Vec3<float>(x * recip, y * recip, z * recip);
    }
    return *this;
#endif
}

template <>
inline float Vec3<float>::Normalize() {
    const float length = Length();
    if (length < std::numeric_limits<float>::epsilon()) {
        SetZero();
        return 0.0f;
    }

    const float inv_length = 1.0f / length;
    x *= inv_length;
    y *= inv_length;
    z *= inv_length;
    return length;
}

using Vec3f = Vec3<float>;
using Vec3i = Vec3<int>;
using Vec3u = Vec3<unsigned int>;

template <typename T>
class alignas(16) Vec4 {
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int file_version) {
        ar & x;
        ar & y;
        ar & z;
        ar & w;
    }

public:
    union {
        struct {
            T x, y, z, w;
        };
#if defined(HAVE_SSE2)
        __m128 simd;
#elif defined(HAVE_NEON)
        float32x4_t simd;
#endif
    };

    T* AsArray() {
        return &x;
    }

    const T* AsArray() const {
        return &x;
    }

    constexpr Vec4() = default;
    constexpr Vec4(const T& x_, const T& y_, const T& z_, const T& w_)
        : x(x_), y(y_), z(z_), w(w_) {}
    static constexpr std::size_t dimension = 4;

    template <typename T2>
    [[nodiscard]] constexpr Vec4<T2> Cast() const {
        return Vec4<T2>(static_cast<T2>(x), static_cast<T2>(y), static_cast<T2>(z),
                        static_cast<T2>(w));
    }

    [[nodiscard]] static constexpr Vec4 AssignToAll(const T& f) {
        return Vec4(f, f, f, f);
    }

    [[nodiscard]] Vec4 Normalized() const {
        const float length = Length();
        if (length < std::numeric_limits<float>::epsilon()) {
            return Vec4{0, 0, 0, 0};
        }
        return *this / length;
    }

    float Normalize() {
        const float length = Length();
        if (length < std::numeric_limits<float>::epsilon()) {
            SetZero();
            return 0.0f;
        }

        const float inv_length = 1.0f / length;
        x *= inv_length;
        y *= inv_length;
        z *= inv_length;
        w *= inv_length;
        return length;
    }

    [[nodiscard]] constexpr Vec4<decltype(T{} + T{})> operator+(const Vec4& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec4<T> result;
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                result.simd = _mm_add_ps(simd, other.simd);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                result.simd = _mm_castsi128_ps(
                    _mm_add_epi32(_mm_castps_si128(simd), _mm_castps_si128(other.simd)));
            } else if constexpr (std::is_same_v<T, int16_t>) {
                result.simd = _mm_castsi128_ps(_mm_packs_epi32(
                    _mm_add_epi16(_mm_castps_si128(simd), _mm_castps_si128(other.simd)),
                    _mm_setzero_si128()));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                result.simd = vaddq_f32(simd, other.simd);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                result.simd = vreinterpretq_f32_s32(
                    vaddq_s32(vreinterpretq_s32_f32(simd), vreinterpretq_s32_f32(other.simd)));
            } else if constexpr (std::is_same_v<T, int16_t>) {
                result.simd = vreinterpretq_f32_s16(
                    vaddq_s16(vreinterpretq_s16_f32(simd), vreinterpretq_s16_f32(other.simd)));
            }
#endif
            return result;
        } else {
            return {x + other.x, y + other.y, z + other.z, w + other.w};
        }
    }

    [[nodiscard]] constexpr Vec4& operator+=(const Vec4& other) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                simd = _mm_add_ps(simd, other.simd);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                simd = _mm_castsi128_ps(
                    _mm_add_epi32(_mm_castps_si128(simd), _mm_castps_si128(other.simd)));
            } else if constexpr (std::is_same_v<T, int16_t>) {
                simd = _mm_castsi128_ps(_mm_packs_epi32(
                    _mm_add_epi16(_mm_castps_si128(simd), _mm_castps_si128(other.simd)),
                    _mm_setzero_si128()));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                simd = vaddq_f32(simd, other.simd);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                simd = vreinterpretq_f32_s32(
                    vaddq_s32(vreinterpretq_s32_f32(simd), vreinterpretq_s32_f32(other.simd)));
            } else if constexpr (std::is_same_v<T, int16_t>) {
                simd = vreinterpretq_f32_s16(
                    vaddq_s16(vreinterpretq_s16_f32(simd), vreinterpretq_s16_f32(other.simd)));
            }
#else
            x += other.x;
            y += other.y;
            z += other.z;
            w += other.w;
#endif
        } else {
            x += other.x;
            y += other.y;
            z += other.z;
            w += other.w;
        }
        return *this;
    }

    [[nodiscard]] constexpr Vec4<decltype(T{} - T{})> operator-(const Vec4& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec4<T> result;
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                result.simd = _mm_sub_ps(simd, other.simd);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                result.simd = _mm_castsi128_ps(
                    _mm_sub_epi32(_mm_castps_si128(simd), _mm_castps_si128(other.simd)));
            } else if constexpr (std::is_same_v<T, int16_t>) {
                result.simd = _mm_castsi128_ps(_mm_packs_epi32(
                    _mm_sub_epi16(_mm_castps_si128(simd), _mm_castps_si128(other.simd)),
                    _mm_setzero_si128()));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                result.simd = vsubq_f32(simd, other.simd);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                result.simd = vreinterpretq_f32_s32(
                    vsubq_s32(vreinterpretq_s32_f32(simd), vreinterpretq_s32_f32(other.simd)));
            } else if constexpr (std::is_same_v<T, int16_t>) {
                result.simd = vreinterpretq_f32_s16(
                    vsubq_s16(vreinterpretq_s16_f32(simd), vreinterpretq_s16_f32(other.simd)));
            }
#endif
            return result;
        } else {
            return {x - other.x, y - other.y, z - other.z, w - other.w};
        }
    }

    [[nodiscard]] constexpr Vec4& operator-=(const Vec4& other) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                simd = _mm_sub_ps(simd, other.simd);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                simd = _mm_castsi128_ps(
                    _mm_sub_epi32(_mm_castps_si128(simd), _mm_castps_si128(other.simd)));
            } else if constexpr (std::is_same_v<T, int16_t>) {
                simd = _mm_castsi128_ps(_mm_packs_epi32(
                    _mm_sub_epi16(_mm_castps_si128(simd), _mm_castps_si128(other.simd)),
                    _mm_setzero_si128()));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                simd = vsubq_f32(simd, other.simd);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                simd = vreinterpretq_f32_s32(
                    vsubq_s32(vreinterpretq_s32_f32(simd), vreinterpretq_s32_f32(other.simd)));
            } else if constexpr (std::is_same_v<T, int16_t>) {
                simd = vreinterpretq_f32_s16(
                    vsubq_s16(vreinterpretq_s16_f32(simd), vreinterpretq_s16_f32(other.simd)));
            }
#else
            x -= other.x;
            y -= other.y;
            z -= other.z;
            w -= other.w;
#endif
        } else {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            w -= other.w;
        }
        return *this;
    }

    template <typename U = T>
    [[nodiscard]] constexpr Vec4<std::enable_if_t<std::is_signed_v<U>, U>> operator-() const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec4<T> result;
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                // Negate by XORing with sign bit mask
                result.simd = _mm_xor_ps(simd, _mm_set1_ps(-0.0f));
            } else if constexpr (std::is_same_v<T, int32_t>) {
                // Negate integers using subtraction from zero
                result.simd =
                    _mm_castsi128_ps(_mm_sub_epi32(_mm_setzero_si128(), _mm_castps_si128(simd)));
            } else if constexpr (std::is_same_v<T, int16_t>) {
                result.simd = _mm_castsi128_ps(
                    _mm_packs_epi32(_mm_sub_epi16(_mm_setzero_si128(), _mm_castps_si128(simd)),
                                    _mm_setzero_si128()));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                // Negate using multiplication by -1
                result.simd = vmulq_n_f32(simd, -1.0f);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                result.simd = vreinterpretq_f32_s32(vnegq_s32(vreinterpretq_s32_f32(simd)));
            } else if constexpr (std::is_same_v<T, int16_t>) {
                result.simd = vreinterpretq_f32_s16(vnegq_s16(vreinterpretq_s16_f32(simd)));
            }
#endif
            return result;
        } else {
            return {-x, -y, -z, -w};
        }
    }

    [[nodiscard]] constexpr Vec4<decltype(T{} * T{})> operator*(const Vec4& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec4<T> result;
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                result.simd = _mm_mul_ps(simd, other.simd);
            } else if constexpr (std::is_same_v<T, int32_t>) {
#if defined(__SSE4_1__)
                // Use SSE4.1's mullo instruction for full 32-bit multiplication
                result.simd = _mm_castsi128_ps(
                    _mm_mullo_epi32(_mm_castps_si128(simd), _mm_castps_si128(other.simd)));
#else
                // Fallback for SSE2: manual multiplication
                __m128i a = _mm_castps_si128(simd);
                __m128i b = _mm_castps_si128(other.simd);

                // Multiply lower 32 bits
                __m128i tmp = _mm_mul_epu32(a, b);
                // Shift and multiply upper 32 bits
                __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(a, 4), _mm_srli_si128(b, 4));
                // Combine results
                result.simd = _mm_castsi128_ps(
                    _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp, _MM_SHUFFLE(0, 0, 2, 0)),
                                       _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0))));
#endif
            } else if constexpr (std::is_same_v<T, int16_t>) {
                result.simd = _mm_castsi128_ps(_mm_packs_epi32(
                    _mm_mullo_epi16(_mm_castps_si128(simd), _mm_castps_si128(other.simd)),
                    _mm_setzero_si128()));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                result.simd = vmulq_f32(simd, other.simd);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                result.simd = vreinterpretq_f32_s32(
                    vmulq_s32(vreinterpretq_s32_f32(simd), vreinterpretq_s32_f32(other.simd)));
            } else if constexpr (std::is_same_v<T, int16_t>) {
                result.simd = vreinterpretq_f32_s16(
                    vmulq_s16(vreinterpretq_s16_f32(simd), vreinterpretq_s16_f32(other.simd)));
            }
#endif
            return result;
        } else {
            return {x * other.x, y * other.y, z * other.z, w * other.w};
        }
    }

    template <typename V>
    [[nodiscard]] constexpr Vec4<decltype(T{} * V{})> operator*(const V& f) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec4<decltype(T{} * V{})> result;
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<V, float>) {
                result.simd = _mm_mul_ps(simd, _mm_set1_ps(f));
            } else if constexpr (std::is_same_v<V, int32_t>) {
#if defined(__SSE4_1__)
                // Use SSE4.1's mullo instruction for full 32-bit multiplication
                result.simd =
                    _mm_castsi128_ps(_mm_mullo_epi32(_mm_castps_si128(simd), _mm_set1_epi32(f)));
#else
                // SSE2 fallback for integer multiplication
                __m128i vec = _mm_castps_si128(simd);
                __m128i scalar = _mm_set1_epi32(f);

                // Multiply lower 32 bits
                __m128i tmp = _mm_mul_epu32(vec, scalar);
                // Shift and multiply upper 32 bits
                __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(vec, 4), _mm_srli_si128(scalar, 4));
                // Combine results
                result.simd = _mm_castsi128_ps(
                    _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp, _MM_SHUFFLE(0, 0, 2, 0)),
                                       _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0))));
#endif
            } else if constexpr (std::is_same_v<V, int16_t>) {
                result.simd = _mm_castsi128_ps(
                    _mm_packs_epi32(_mm_mullo_epi16(_mm_castps_si128(simd), _mm_set1_epi16(f)),
                                    _mm_setzero_si128()));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<V, float>) {
                result.simd = vmulq_n_f32(simd, f); // Use scalar multiply variant
            } else if constexpr (std::is_same_v<V, int32_t>) {
                result.simd = vreinterpretq_f32_s32(vmulq_n_s32(vreinterpretq_s32_f32(simd), f));
            } else if constexpr (std::is_same_v<V, int16_t>) {
                result.simd = vreinterpretq_f32_s16(vmulq_n_s16(vreinterpretq_s16_f32(simd), f));
            }
#endif
            return result;
        } else {
            return {x * f, y * f, z * f, w * f};
        }
    }

    template <typename V>
    constexpr Vec4& operator*=(const V& f) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<V, float>) {
                simd = _mm_mul_ps(simd, _mm_set1_ps(f));
            } else if constexpr (std::is_same_v<V, int32_t>) {
#if defined(__SSE4_1__)
                // Use SSE4.1's mullo instruction for full 32-bit multiplication
                simd = _mm_castsi128_ps(_mm_mullo_epi32(_mm_castps_si128(simd), _mm_set1_epi32(f)));
#else
                // SSE2 fallback for integer multiplication
                __m128i vec = _mm_castps_si128(simd);
                __m128i scalar = _mm_set1_epi32(f);

                // Multiply lower 32 bits
                __m128i tmp = _mm_mul_epu32(vec, scalar);
                // Shift and multiply upper 32 bits
                __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(vec, 4), _mm_srli_si128(scalar, 4));
                // Combine results
                simd = _mm_castsi128_ps(
                    _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp, _MM_SHUFFLE(0, 0, 2, 0)),
                                       _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0))));
#endif
            } else if constexpr (std::is_same_v<V, int16_t>) {
                simd = _mm_castsi128_ps(
                    _mm_packs_epi32(_mm_mullo_epi16(_mm_castps_si128(simd), _mm_set1_epi16(f)),
                                    _mm_setzero_si128()));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<V, float>) {
                simd = vmulq_n_f32(simd, f); // Use scalar multiply variant
            } else if constexpr (std::is_same_v<V, int32_t>) {
                simd = vreinterpretq_f32_s32(vmulq_n_s32(vreinterpretq_s32_f32(simd), f));
            } else if constexpr (std::is_same_v<V, int16_t>) {
                simd = vreinterpretq_f32_s16(vmulq_n_s16(vreinterpretq_s16_f32(simd), f));
            }
#else
            x *= f;
            y *= f;
            z *= f;
            w *= f;
#endif
        } else {
            x *= f;
            y *= f;
            z *= f;
            w *= f;
        }
        return *this;
    }

    template <typename V>
    [[nodiscard]] constexpr Vec4<decltype(T{} / V{})> operator/(const V& f) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec4<decltype(T{} / V{})> result;
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<V, float>) {
                result.simd = _mm_div_ps(simd, _mm_set1_ps(f));
            } else if constexpr (std::is_same_v<V, int32_t>) {
                // For integer division, convert to float, divide, and convert back
                __m128 float_vec = _mm_cvtepi32_ps(_mm_castps_si128(simd));
                __m128 float_scalar = _mm_set1_ps(static_cast<float>(f));
                __m128 div_result = _mm_div_ps(float_vec, float_scalar);
                result.simd = _mm_castsi128_ps(_mm_cvtps_epi32(div_result));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<V, float>) {
                // NEON doesn't have direct division, use reciprocal multiplication
                float32x4_t divisor = vdupq_n_f32(f);
                // Initial estimate
                float32x4_t recip = vrecpeq_f32(divisor);

                // Two Newton-Raphson iterations for better precision
                // Each iteration approximately doubles the number of correct bits
                recip = vmulq_f32(vrecpsq_f32(divisor, recip), recip);
                recip = vmulq_f32(vrecpsq_f32(divisor, recip), recip);

                // Final multiplication
                result.simd = vmulq_f32(simd, recip);
            } else if constexpr (std::is_same_v<V, int32_t>) {
                // For integer division, convert to float, divide, and convert back
                float32x4_t float_vec = vcvtq_f32_s32(vreinterpretq_s32_f32(simd));
                float32x4_t divisor = vdupq_n_f32(static_cast<float>(f));
                float32x4_t recip = vrecpeq_f32(divisor);

                // Two Newton-Raphson iterations
                recip = vmulq_f32(vrecpsq_f32(divisor, recip), recip);
                recip = vmulq_f32(vrecpsq_f32(divisor, recip), recip);

                float32x4_t result_f = vmulq_f32(float_vec, recip);
                result.simd = vreinterpretq_f32_s32(vcvtq_s32_f32(result_f));
            }
#else
            return {x / f, y / f, z / f, w / f};
#endif
            return result;
        } else {
            // Check for division by zero in debug builds
#ifdef _DEBUG
            if (f == V{}) {
                throw std::domain_error("Division by zero in Vec4");
            }
#endif
            return {x / f, y / f, z / f, w / f};
        }
    }

    template <typename V>
    [[nodiscard]] constexpr Vec4& operator/=(const V& f) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<V, float>) {
                simd = _mm_div_ps(simd, _mm_set1_ps(f));
            } else if constexpr (std::is_same_v<V, int32_t>) {
                // For integer division, convert to float, divide, and convert back
                __m128 float_vec = _mm_cvtepi32_ps(_mm_castps_si128(simd));
                __m128 float_scalar = _mm_set1_ps(static_cast<float>(f));
                __m128 div_result = _mm_div_ps(float_vec, float_scalar);
                simd = _mm_castsi128_ps(_mm_cvtps_epi32(div_result));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<V, float>) {
                // NEON doesn't have direct division, use reciprocal multiplication
                float32x4_t divisor = vdupq_n_f32(f);
                // Initial estimate
                float32x4_t recip = vrecpeq_f32(divisor);

                // Two Newton-Raphson iterations for better precision
                // Each iteration approximately doubles the number of correct bits
                recip = vmulq_f32(vrecpsq_f32(divisor, recip), recip);
                recip = vmulq_f32(vrecpsq_f32(divisor, recip), recip);

                // Final multiplication
                simd = vmulq_f32(simd, recip);
            } else if constexpr (std::is_same_v<V, int32_t>) {
                // For integer division, convert to float, divide, and convert back
                float32x4_t float_vec = vcvtq_f32_s32(vreinterpretq_s32_f32(simd));
                float32x4_t divisor = vdupq_n_f32(static_cast<float>(f));
                float32x4_t recip = vrecpeq_f32(divisor);

                // Two Newton-Raphson iterations
                recip = vmulq_f32(vrecpsq_f32(divisor, recip), recip);
                recip = vmulq_f32(vrecpsq_f32(divisor, recip), recip);

                float32x4_t result_f = vmulq_f32(float_vec, recip);
                simd = vreinterpretq_f32_s32(vcvtq_s32_f32(result_f));
            }
#else
            x /= f;
            y /= f;
            z /= f;
            w /= f;
#endif
        } else {
            // Check for division by zero in debug builds
#ifdef _DEBUG
            if (f == V{}) {
                throw std::domain_error("Division by zero in Vec4");
            }
#endif
            x /= f;
            y /= f;
            z /= f;
            w /= f;
        }
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const Vec4& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                __m128 cmp = _mm_cmpeq_ps(simd, other.simd);
                return _mm_movemask_ps(cmp) == 0xF; // All 4 components must match
            } else if constexpr (std::is_integral_v<T>) {
                __m128i a = _mm_castps_si128(simd);
                __m128i b = _mm_castps_si128(other.simd);
                __m128i cmp = _mm_cmpeq_epi32(a, b);
                return _mm_movemask_ps(_mm_castsi128_ps(cmp)) == 0xF;
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                uint32x4_t cmp = vceqq_f32(simd, other.simd);
#if defined(__aarch64__)
                // AArch64 has a more efficient implementation
                return vminvq_u32(cmp) == 0xFFFFFFFF;
#else
                // ARM32 implementation
                uint32x2_t and_hl = vand_u32(vget_high_u32(cmp), vget_low_u32(cmp));
                return vget_lane_u32(vpmin_u32(and_hl, and_hl), 0) == 0xFFFFFFFF;
#endif
            } else if constexpr (std::is_same_v<T, int32_t>) {
                uint32x4_t cmp =
                    vceqq_s32(vreinterpretq_s32_f32(simd), vreinterpretq_s32_f32(other.simd));
#if defined(__aarch64__)
                return vminvq_u32(cmp) == 0xFFFFFFFF;
#else
                uint32x2_t and_hl = vand_u32(vget_high_u32(cmp), vget_low_u32(cmp));
                return vget_lane_u32(vpmin_u32(and_hl, and_hl), 0) == 0xFFFFFFFF;
#endif
            } else if constexpr (std::is_same_v<T, int16_t>) {
                uint16x8_t cmp =
                    vceqq_s16(vreinterpretq_s16_f32(simd), vreinterpretq_s16_f32(other.simd));
#if defined(__aarch64__)
                return vminvq_u16(cmp) == 0xFFFF;
#else
                uint16x4_t and_hl = vand_u16(vget_high_u16(cmp), vget_low_u16(cmp));
                return vget_lane_u16(vpmin_u16(and_hl, and_hl), 0) == 0xFFFF;
#endif
            }
#endif
        }
        // Fallback to scalar comparison
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    [[nodiscard]] constexpr bool operator!=(const Vec4& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                // Use SSE comparison for floating-point
                __m128 eq = _mm_cmpeq_ps(simd, other.simd);
                return (_mm_movemask_ps(eq) & 0xF) != 0xF;
            } else if constexpr (std::is_integral_v<T>) {
                if constexpr (sizeof(T) <= 2) {
                    // For 16-bit integers
                    __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&x));
                    __m128i v2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&other.x));
                    __m128i eq = _mm_cmpeq_epi16(v1, v2);
                    return (_mm_movemask_epi8(eq) & 0xFF) != 0xFF;
                } else {
                    // For 32-bit integers
                    __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&x));
                    __m128i v2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&other.x));
                    __m128i eq = _mm_cmpeq_epi32(v1, v2);
                    return (_mm_movemask_ps(_mm_castsi128_ps(eq)) & 0xF) != 0xF;
                }
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                // Use NEON comparison for floating-point with SIMD member
                uint32x4_t eq = vceqq_f32(simd, other.simd);
#if defined(__aarch64__)
                // AArch64 has efficient all-true testing
                return !vminvq_u32(eq);
#else
                uint32x2_t hi = vget_high_u32(eq);
                uint32x2_t lo = vget_low_u32(eq);
                uint32x2_t and_res = vand_u32(hi, lo);
                return (vget_lane_u32(and_res, 0) & vget_lane_u32(and_res, 1)) != 0xFFFFFFFF;
#endif
            } else if constexpr (std::is_integral_v<T>) {
                if constexpr (sizeof(T) <= 2) {
                    // For 16-bit integers using temporary arrays
                    int16_t temp1[4], temp2[4];
                    std::memcpy(temp1, &x, sizeof(T) * 4);
                    std::memcpy(temp2, &other.x, sizeof(T) * 4);

                    int16x4_t v1 = vld1_s16(temp1);
                    int16x4_t v2 = vld1_s16(temp2);
                    uint16x4_t eq = vceq_s16(v1, v2);
                    return (vget_lane_u16(eq, 0) & vget_lane_u16(eq, 1) & vget_lane_u16(eq, 2) &
                            vget_lane_u16(eq, 3)) != 0xFFFF;
                } else {
                    // For 32-bit integers using temporary arrays
                    int32_t temp1[4], temp2[4];
                    std::memcpy(temp1, &x, sizeof(T) * 4);
                    std::memcpy(temp2, &other.x, sizeof(T) * 4);

                    int32x4_t v1 = vld1q_s32(temp1);
                    int32x4_t v2 = vld1q_s32(temp2);
                    uint32x4_t eq = vceqq_s32(v1, v2);
#if defined(__aarch64__)
                    return !vminvq_u32(eq);
#else
                    uint32x2_t hi = vget_high_u32(eq);
                    uint32x2_t lo = vget_low_u32(eq);
                    uint32x2_t and_res = vand_u32(hi, lo);
                    return (vget_lane_u32(and_res, 0) & vget_lane_u32(and_res, 1)) != 0xFFFFFFFF;
#endif
                }
            }
#endif
        }
        // Fallback scalar implementation
        return x != other.x || y != other.y || z != other.z || w != other.w;
    }

    [[nodiscard]] constexpr T Length2() const {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                __m128 sq = _mm_mul_ps(simd, simd);
                // Horizontal add all four components using optimized shuffles
                __m128 sum1 = _mm_add_ps(sq, _mm_movehl_ps(sq, sq)); // Add upper and lower halves
                __m128 sum2 =
                    _mm_add_ss(sum1, _mm_shuffle_ps(sum1, sum1, 1)); // Add remaining elements
                return _mm_cvtss_f32(sum2);
            } else if constexpr (std::is_integral_v<T>) {
                // Handle integer types - prevent overflow by converting to wider type
                __m128i vec = _mm_castps_si128(simd);
                __m128i squares;

                if constexpr (sizeof(T) <= 2) {
                    // For 16-bit integers, use 32-bit multiplication
                    squares = _mm_madd_epi16(vec, vec); // Multiply and add adjacent pairs
                } else {
                    // For 32-bit integers, multiply and accumulate carefully
                    __m128i lo = _mm_srli_epi64(_mm_mul_epu32(vec, vec), 32);
                    __m128i hi = _mm_mul_epu32(_mm_srli_epi64(vec, 32), _mm_srli_epi64(vec, 32));
                    squares = _mm_or_si128(lo, _mm_slli_epi64(hi, 32));
                }

                // Sum the components
                __m128i sum1 = _mm_add_epi64(_mm_unpacklo_epi32(squares, _mm_setzero_si128()),
                                             _mm_unpackhi_epi32(squares, _mm_setzero_si128()));
                return static_cast<T>(_mm_cvtsi128_si64(sum1) +
                                      _mm_cvtsi128_si64(_mm_shuffle_epi32(sum1, 0x4E)));
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                float32x4_t sq = vmulq_f32(simd, simd);
#if defined(__aarch64__)
                // Use dedicated instruction on AArch64
                return vaddvq_f32(sq);
#else
                // Optimized horizontal add for ARM32
                float32x2_t sum = vpadd_f32(vget_low_f32(sq), vget_high_f32(sq));
                return vget_lane_f32(vpadd_f32(sum, sum), 0);
#endif
            } else if constexpr (std::is_integral_v<T>) {
                if constexpr (sizeof(T) <= 2) {
                    // For 16-bit integers
                    int32x4_t squares = vmull_s16(vget_low_s16(vreinterpretq_s16_f32(simd)),
                                                  vget_low_s16(vreinterpretq_s16_f32(simd)));
#if defined(__aarch64__)
                    return static_cast<T>(vaddvq_s32(squares));
#else
                    int32x2_t sum = vadd_s32(vget_low_s32(squares), vget_high_s32(squares));
                    return static_cast<T>(vget_lane_s32(vpadd_s32(sum, sum), 0));
#endif
                } else {
                    // For 32-bit integers
                    int64x2_t squares = vmull_s32(vget_low_s32(vreinterpretq_s32_f32(simd)),
                                                  vget_low_s32(vreinterpretq_s32_f32(simd)));
                    return static_cast<T>(vgetq_lane_s64(squares, 0) + vgetq_lane_s64(squares, 1));
                }
            }
#endif
        }
        // Fallback scalar implementation
        return x * x + y * y + z * z + w * w;
    }

    [[nodiscard]] float Length() const {
        const float length2 = Length2();
        if (length2 < std::numeric_limits<float>::epsilon()) {
            return 0.0f;
        }
#if defined(HAVE_SSE2)
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(length2)));
#elif defined(HAVE_NEON)
        float32x2_t len = vsqrt_f32(vdup_n_f32(length2));
        return vget_lane_f32(len, 0);
#else
        return std::sqrt(length2);
#endif
    }

    [[nodiscard]] constexpr T& operator[](std::size_t i) {
        assert(i < 4 && "Index out of bounds in Vec4");
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                // Direct SIMD access for floats
                alignas(16) float* const elements = reinterpret_cast<float*>(&simd);
                return elements[i];
            } else if constexpr (std::is_integral_v<T>) {
                // Integer SIMD access with proper alignment
                if constexpr (sizeof(T) <= 2) {
                    // 16-bit integers
                    alignas(16) int16_t* const elements = reinterpret_cast<int16_t*>(&simd);
                    return *reinterpret_cast<T*>(&elements[i]);
                } else {
                    // 32-bit integers
                    alignas(16) int32_t* const elements = reinterpret_cast<int32_t*>(&simd);
                    return *reinterpret_cast<T*>(&elements[i]);
                }
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                // Use std::array for type-safe access to NEON data
                std::array<float, 4>& elements =
                    *std::launder(reinterpret_cast<std::array<float, 4>*>(&simd));
                return elements[i];
            } else if constexpr (std::is_integral_v<T>) {
                // Use std::array for both 16-bit and 32-bit integers
                std::array<T, 4>& elements =
                    *std::launder(reinterpret_cast<std::array<T, 4>*>(&simd));
                return elements[i];
            }
#endif
        }

        // Use a union for safe component access
        union Components {
            struct {
                T x, y, z, w;
            };
            std::array<T, 4> arr;
        };
        Components& c = *std::launder(reinterpret_cast<Components*>(this));
        return c.arr[i];
    }

    [[nodiscard]] constexpr const T& operator[](std::size_t i) const {
        assert(i < 4 && "Index out of bounds in Vec4");
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            if constexpr (std::is_same_v<T, float>) {
                // Direct const SIMD access for floats
                alignas(16) const float* const elements = reinterpret_cast<const float*>(&simd);
                return elements[i];
            } else if constexpr (std::is_integral_v<T>) {
                // Integer const SIMD access with proper alignment
                if constexpr (sizeof(T) <= 2) {
                    // 16-bit integers
                    alignas(16) const int16_t* const elements =
                        reinterpret_cast<const int16_t*>(&simd);
                    return *reinterpret_cast<const T*>(&elements[i]);
                } else {
                    // 32-bit integers
                    alignas(16) const int32_t* const elements =
                        reinterpret_cast<const int32_t*>(&simd);
                    return *reinterpret_cast<const T*>(&elements[i]);
                }
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float>) {
                // Use const std::array for type-safe access to NEON data
                const std::array<float, 4>& elements =
                    *std::launder(reinterpret_cast<const std::array<float, 4>*>(&simd));
                return elements[i];
            } else if constexpr (std::is_integral_v<T>) {
                // Use const std::array for both 16-bit and 32-bit integers
                const std::array<T, 4>& elements =
                    *std::launder(reinterpret_cast<const std::array<T, 4>*>(&simd));
                return elements[i];
            }
#endif
        }

        // Use a const union for safe component access
        union Components {
            struct {
                T x, y, z, w;
            };
            std::array<T, 4> arr;
        };
        const Components& c = *std::launder(reinterpret_cast<const Components*>(this));
        return c.arr[i];
    }

    constexpr void SetZero() {
        x = 0;
        y = 0;
        z = 0;
        w = 0;
    }

    // Common alias: RGBA (colors)
    [[nodiscard]] constexpr T& r() {
        return x;
    }
    [[nodiscard]] constexpr T& g() {
        return y;
    }
    [[nodiscard]] constexpr T& b() {
        return z;
    }
    [[nodiscard]] constexpr T& a() {
        return w;
    }

    [[nodiscard]] constexpr const T& r() const {
        return x;
    }
    [[nodiscard]] constexpr const T& g() const {
        return y;
    }
    [[nodiscard]] constexpr const T& b() const {
        return z;
    }
    [[nodiscard]] constexpr const T& a() const {
        return w;
    }

// Swizzlers - Create a subvector of specific components
// e.g. Vec2 uv() { return Vec2(x,y); }

// _DEFINE_SWIZZLER2 defines a single such function
// DEFINE_SWIZZLER2_COMP1 defines one-component functions for all component names (x<->r)
// DEFINE_SWIZZLER2_COMP2 defines two component functions for all component names (x<->r) and
// permutations (xy<->yx)
#define _DEFINE_SWIZZLER2(a, b, name)                                                              \
    [[nodiscard]] constexpr Vec2<T> name() const {                                                 \
        return Vec2<T>(a, b);                                                                      \
    }
#define DEFINE_SWIZZLER2_COMP1(a, a2)                                                              \
    _DEFINE_SWIZZLER2(a, a, a##a);                                                                 \
    _DEFINE_SWIZZLER2(a, a, a2##a2)
#define DEFINE_SWIZZLER2_COMP2(a, b, a2, b2)                                                       \
    _DEFINE_SWIZZLER2(a, b, a##b);                                                                 \
    _DEFINE_SWIZZLER2(a, b, a2##b2);                                                               \
    _DEFINE_SWIZZLER2(b, a, b##a);                                                                 \
    _DEFINE_SWIZZLER2(b, a, b2##a2)

    DEFINE_SWIZZLER2_COMP2(x, y, r, g);
    DEFINE_SWIZZLER2_COMP2(x, z, r, b);
    DEFINE_SWIZZLER2_COMP2(x, w, r, a);
    DEFINE_SWIZZLER2_COMP2(y, z, g, b);
    DEFINE_SWIZZLER2_COMP2(y, w, g, a);
    DEFINE_SWIZZLER2_COMP2(z, w, b, a);
    DEFINE_SWIZZLER2_COMP1(x, r);
    DEFINE_SWIZZLER2_COMP1(y, g);
    DEFINE_SWIZZLER2_COMP1(z, b);
    DEFINE_SWIZZLER2_COMP1(w, a);
#undef DEFINE_SWIZZLER2_COMP1
#undef DEFINE_SWIZZLER2_COMP2
#undef _DEFINE_SWIZZLER2

#define _DEFINE_SWIZZLER3(a, b, c, name)                                                           \
    [[nodiscard]] constexpr Vec3<T> name() const {                                                 \
        return Vec3<T>(a, b, c);                                                                   \
    }
#define DEFINE_SWIZZLER3_COMP1(a, a2)                                                              \
    _DEFINE_SWIZZLER3(a, a, a, a##a##a);                                                           \
    _DEFINE_SWIZZLER3(a, a, a, a2##a2##a2)
#define DEFINE_SWIZZLER3_COMP3(a, b, c, a2, b2, c2)                                                \
    _DEFINE_SWIZZLER3(a, b, c, a##b##c);                                                           \
    _DEFINE_SWIZZLER3(a, c, b, a##c##b);                                                           \
    _DEFINE_SWIZZLER3(b, a, c, b##a##c);                                                           \
    _DEFINE_SWIZZLER3(b, c, a, b##c##a);                                                           \
    _DEFINE_SWIZZLER3(c, a, b, c##a##b);                                                           \
    _DEFINE_SWIZZLER3(c, b, a, c##b##a);                                                           \
    _DEFINE_SWIZZLER3(a, b, c, a2##b2##c2);                                                        \
    _DEFINE_SWIZZLER3(a, c, b, a2##c2##b2);                                                        \
    _DEFINE_SWIZZLER3(b, a, c, b2##a2##c2);                                                        \
    _DEFINE_SWIZZLER3(b, c, a, b2##c2##a2);                                                        \
    _DEFINE_SWIZZLER3(c, a, b, c2##a2##b2);                                                        \
    _DEFINE_SWIZZLER3(c, b, a, c2##b2##a2)

    DEFINE_SWIZZLER3_COMP3(x, y, z, r, g, b);
    DEFINE_SWIZZLER3_COMP3(x, y, w, r, g, a);
    DEFINE_SWIZZLER3_COMP3(x, z, w, r, b, a);
    DEFINE_SWIZZLER3_COMP3(y, z, w, g, b, a);
    DEFINE_SWIZZLER3_COMP1(x, r);
    DEFINE_SWIZZLER3_COMP1(y, g);
    DEFINE_SWIZZLER3_COMP1(z, b);
    DEFINE_SWIZZLER3_COMP1(w, a);
#undef DEFINE_SWIZZLER3_COMP1
#undef DEFINE_SWIZZLER3_COMP3
#undef _DEFINE_SWIZZLER3
};

template <typename T, typename V>
[[nodiscard]] constexpr Vec4<decltype(V{} * T{})> operator*(const V& f, const Vec4<T>& vec) {
    if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
        if constexpr (std::is_same_v<V, float>) {
            Vec4<decltype(V{} * T{})> result;
            result.simd = _mm_mul_ps(_mm_set1_ps(f), vec.simd);
            return result;
        } else if constexpr (std::is_integral_v<V>) {
            Vec4<decltype(V{} * T{})> result;
            if constexpr (sizeof(V) <= 4) {
                // For 32-bit and smaller integers
                result.simd = _mm_castsi128_ps(_mm_mullo_epi32(
                    _mm_set1_epi32(static_cast<int32_t>(f)), _mm_castps_si128(vec.simd)));
            }
            return result;
        }
#elif defined(HAVE_NEON)
        if constexpr (std::is_same_v<V, float>) {
            Vec4<decltype(V{} * T{})> result;
            result.simd = vmulq_n_f32(vec.simd, f); // Use scalar multiply variant
            return result;
        } else if constexpr (std::is_same_v<V, int32_t>) {
            Vec4<decltype(V{} * T{})> result;
            result.simd = vreinterpretq_f32_s32(vmulq_n_s32(vreinterpretq_s32_f32(vec.simd), f));
            return result;
        } else if constexpr (std::is_same_v<V, int16_t>) {
            Vec4<decltype(V{} * T{})> result;
            result.simd = vreinterpretq_f32_s16(vmulq_n_s16(vreinterpretq_s16_f32(vec.simd), f));
            return result;
        }
#endif
    }
    // Fallback to scalar multiplication
    return {f * vec.x, f * vec.y, f * vec.z, f * vec.w};
}

using Vec4f = Vec4<float>;
using Vec4i = Vec4<int>;
using Vec4u = Vec4<unsigned int>;

template <typename T>
constexpr decltype(T{} * T{} + T{} * T{}) Dot(const Vec2<T>& a, const Vec2<T>& b) {
    if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE4_1)
        if constexpr (std::is_same_v<T, float>) {
            // Use SSE4.1 dot product instruction
            __m128 va = _mm_setr_ps(a.x, a.y, 0.0f, 0.0f);
            __m128 vb = _mm_setr_ps(b.x, b.y, 0.0f, 0.0f);
            return _mm_cvtss_f32(
                _mm_dp_ps(va, vb, 0x31)); // 0x31: multiply xy (0x3) and store in lowest (0x1)
        } else if constexpr (std::is_integral_v<T>) {
            // For integer types, use wider type to prevent overflow
            __m128i va = _mm_setr_epi32(static_cast<int32_t>(a.x), static_cast<int32_t>(a.y), 0, 0);
            __m128i vb = _mm_setr_epi32(static_cast<int32_t>(b.x), static_cast<int32_t>(b.y), 0, 0);
            __m128i mul = _mm_mullo_epi32(va, vb);
            return static_cast<T>(_mm_extract_epi32(mul, 0) + _mm_extract_epi32(mul, 1));
        }
#elif defined(HAVE_SSE2)
        if constexpr (std::is_same_v<T, float>) {
            // Optimized SSE2 path for floats
            __m128 va =
                _mm_set_ps(0.0f, 0.0f, a.y, a.x); // Reverse order for better shuffle performance
            __m128 vb = _mm_set_ps(0.0f, 0.0f, b.y, b.x);
            __m128 mul = _mm_mul_ps(va, vb);
            return _mm_cvtss_f32(_mm_add_ss(mul, _mm_shuffle_ps(mul, mul, 1)));
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (sizeof(T) <= 2) {
                // For 16-bit integers, use 32-bit multiplication
                __m128i va =
                    _mm_set_epi32(0, 0, static_cast<int32_t>(a.y), static_cast<int32_t>(a.x));
                __m128i vb =
                    _mm_set_epi32(0, 0, static_cast<int32_t>(b.y), static_cast<int32_t>(b.x));
                __m128i mul = _mm_madd_epi16(va, vb); // Multiply and add adjacent pairs
                return static_cast<T>(_mm_cvtsi128_si32(mul));
            } else {
                // For 32-bit integers
                __m128i va =
                    _mm_set_epi32(0, 0, static_cast<int32_t>(a.y), static_cast<int32_t>(a.x));
                __m128i vb =
                    _mm_set_epi32(0, 0, static_cast<int32_t>(b.y), static_cast<int32_t>(b.x));
                __m128i mul = _mm_mul_epu32(va, vb);
                return static_cast<T>(_mm_cvtsi128_si64(mul));
            }
        }
#elif defined(HAVE_NEON)
        if constexpr (std::is_same_v<T, float>) {
            // Optimized NEON path using dot product instruction
            float32x2_t va = {a.x, a.y};
            float32x2_t vb = {b.x, b.y};
#if defined(__aarch64__)
            return vaddv_f32(vmul_f32(va, vb)); // More efficient on AArch64
#else
            float32x2_t mul = vmul_f32(va, vb);
            return vget_lane_f32(vpadd_f32(mul, mul), 0);
#endif
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (sizeof(T) <= 2) {
                // For 16-bit integers
                int16x4_t va = {static_cast<int16_t>(a.x), static_cast<int16_t>(a.y), 0, 0};
                int16x4_t vb = {static_cast<int16_t>(b.x), static_cast<int16_t>(b.y), 0, 0};
                int32x4_t mul = vmull_s16(va, vb);
                return static_cast<T>(vgetq_lane_s32(mul, 0) + vgetq_lane_s32(mul, 1));
            } else {
                // For 32-bit integers
                int32x2_t va = {static_cast<int32_t>(a.x), static_cast<int32_t>(a.y)};
                int32x2_t vb = {static_cast<int32_t>(b.x), static_cast<int32_t>(b.y)};
                int64x2_t mul = vmull_s32(va, vb);
                return static_cast<T>(vgetq_lane_s64(mul, 0) + vgetq_lane_s64(mul, 1));
            }
        }
#endif
    }
    // Fallback scalar implementation
    return a.x * b.x + a.y * b.y;
}

template <typename T>
[[nodiscard]] constexpr decltype(T{} * T{} + T{} * T{}) Dot(const Vec3<T>& a, const Vec3<T>& b) {
    if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_AVX)
        __m256 va = _mm256_setr_ps(a.x, a.y, a.z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        __m256 vb = _mm256_setr_ps(b.x, b.y, b.z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        __m256 mul = _mm256_mul_ps(va, vb);
        __m256 sum = _mm256_hadd_ps(mul, mul);
        sum = _mm256_hadd_ps(sum, sum);
        return _mm_cvtss_f32(_mm256_extractf128_ps(sum, 0));
#elif defined(HAVE_SSE4_1)
        if constexpr (std::is_same_v<T, float>) {
            // SSE4.1 dot product instruction
            // 0x71 mask: multiply xyz components (0x7) and store in lowest component (0x1)
            return _mm_cvtss_f32(_mm_dp_ps(_mm_load_ps(&a.x), _mm_load_ps(&b.x), 0x71));
        } else if constexpr (std::is_integral_v<T>) {
            // For integer types, use wider type to prevent overflow
            __m128i va = _mm_setr_epi32(static_cast<int32_t>(a.x), static_cast<int32_t>(a.y),
                                        static_cast<int32_t>(a.z), 0);
            __m128i vb = _mm_setr_epi32(static_cast<int32_t>(b.x), static_cast<int32_t>(b.y),
                                        static_cast<int32_t>(b.z), 0);
            __m128i mul = _mm_mullo_epi32(va, vb);
            // Horizontal add of three components
            __m128i sum = _mm_add_epi32(mul, _mm_srli_si128(mul, 4));
            sum = _mm_add_epi32(sum, _mm_srli_si128(mul, 8));
            return static_cast<T>(_mm_cvtsi128_si32(sum));
        }
#elif defined(HAVE_SSE2)
        if constexpr (std::is_same_v<T, float>) {
            __m128 va = _mm_load_ps(&a.x);
            __m128 vb = _mm_load_ps(&b.x);
            __m128 mul = _mm_mul_ps(va, vb);
            // Optimized horizontal add using shuffles
            __m128 sum = _mm_add_ps(mul, _mm_movehl_ps(mul, mul)); // Add upper and lower halves
            sum = _mm_add_ss(sum, _mm_shuffle_ps(sum, sum, 1));    // Add remaining element
            return _mm_cvtss_f32(sum);
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (sizeof(T) <= 2) {
                // For 16-bit integers
                __m128i va =
                    detail::vec3_to_epi16(static_cast<int16_t>(a.x), static_cast<int16_t>(a.y),
                                          static_cast<int16_t>(a.z));
                __m128i vb =
                    detail::vec3_to_epi16(static_cast<int16_t>(b.x), static_cast<int16_t>(b.y),
                                          static_cast<int16_t>(b.z));
                __m128i mul = _mm_madd_epi16(va, vb); // Multiply and add adjacent pairs
                // Horizontal add of two 32-bit results
                __m128i sum = _mm_add_epi32(mul, _mm_srli_si128(mul, 4));
                return static_cast<T>(_mm_cvtsi128_si32(sum));
            } else {
                // For 32-bit integers
                __m128i va = _mm_setr_epi32(static_cast<int32_t>(a.x), static_cast<int32_t>(a.y),
                                            static_cast<int32_t>(a.z), 0);
                __m128i vb = _mm_setr_epi32(static_cast<int32_t>(b.x), static_cast<int32_t>(b.y),
                                            static_cast<int32_t>(b.z), 0);
                __m128i mul = _mm_mul_epu32(va, vb);
                // Horizontal add of results
                __m128i sum = _mm_add_epi64(mul, _mm_srli_si128(mul, 8));
                return static_cast<T>(_mm_cvtsi128_si64(sum));
            }
        }
#elif defined(HAVE_NEON)
        if constexpr (std::is_same_v<T, float>) {
            float32x4_t va = vld1q_f32(&a.x);
            float32x4_t vb = vld1q_f32(&b.x);
            float32x4_t mul = vmulq_f32(va, vb);
#if defined(__aarch64__)
            // More efficient on AArch64 using dedicated instruction
            return vaddvq_f32(vsetq_lane_f32(0.f, mul, 3));
#else
            float32x2_t sum = vget_low_f32(mul);
            sum = vpadd_f32(sum, sum);                             // Add x+y
            return vget_lane_f32(sum, 0) + vgetq_lane_f32(mul, 2); // Add z
#endif
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (sizeof(T) <= 2) {
                // For 16-bit integers
                int16x4_t va = {static_cast<int16_t>(a.x), static_cast<int16_t>(a.y),
                                static_cast<int16_t>(a.z), 0};
                int16x4_t vb = {static_cast<int16_t>(b.x), static_cast<int16_t>(b.y),
                                static_cast<int16_t>(b.z), 0};
                int32x4_t mul = vmull_s16(va, vb);
#if defined(__aarch64__)
                return static_cast<T>(vaddvq_s32(mul));
#else
                int32x2_t sum = vadd_s32(vget_low_s32(mul), vget_high_s32(mul));
                return static_cast<T>(vget_lane_s32(vpadd_s32(sum, sum), 0));
#endif
            } else {
                // For 32-bit integers
                int32x2_t va = {static_cast<int32_t>(a.x), static_cast<int32_t>(a.y)};
                int32x2_t vb = {static_cast<int32_t>(b.x), static_cast<int32_t>(b.y)};
                int64x2_t mul = vmull_s32(va, vb);
                int64x2_t sum = vaddq_s64(mul, vmull_s32(vcreate_s32(static_cast<int32_t>(a.z)),
                                                         vcreate_s32(static_cast<int32_t>(b.z))));
                return static_cast<T>(vgetq_lane_s64(sum, 0) + vgetq_lane_s64(sum, 1));
            }
        }
#endif
    }
    // Fallback scalar implementation
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <typename T>
[[nodiscard]] constexpr decltype(T{} * T{} + T{} * T{}) Dot(const Vec4<T>& a, const Vec4<T>& b) {
    if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_AVX)
        __m256 va = _mm256_setr_ps(a.x, a.y, a.z, a.w, 0.0f, 0.0f, 0.0f, 0.0f);
        __m256 vb = _mm256_setr_ps(b.x, b.y, b.z, b.w, 0.0f, 0.0f, 0.0f, 0.0f);
        __m256 mul = _mm256_mul_ps(va, vb);
        __m256 sum = _mm256_hadd_ps(mul, mul);
        sum = _mm256_hadd_ps(sum, sum);
        return _mm_cvtss_f32(_mm256_extractf128_ps(sum, 0));
#elif defined(HAVE_SSE4_1)
        if constexpr (std::is_same_v<T, float>) {
            // SSE4.1 dot product instruction
            // 0xF1 mask: multiply xyzw components (0xF) and store in lowest component (0x1)
            return _mm_cvtss_f32(_mm_dp_ps(a.simd, b.simd, 0xF1));
        } else if constexpr (std::is_integral_v<T>) {
            // For integer types, use wider type to prevent overflow
            __m128i mul = _mm_mullo_epi32(_mm_castps_si128(a.simd), _mm_castps_si128(b.simd));
            // Horizontal add of all components
            __m128i sum = _mm_add_epi32(mul, _mm_srli_si128(mul, 8));
            sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
            return static_cast<T>(_mm_cvtsi128_si32(sum));
        }
#elif defined(HAVE_SSE2)
        if constexpr (std::is_same_v<T, float>) {
            __m128 mul = _mm_mul_ps(a.simd, b.simd);
            // Optimized horizontal add
            __m128 sum = _mm_add_ps(mul, _mm_movehl_ps(mul, mul)); // Add upper and lower halves
            sum = _mm_add_ss(sum, _mm_shuffle_ps(sum, sum, 1));    // Add remaining elements
            return _mm_cvtss_f32(sum);
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (sizeof(T) <= 2) {
                // For 16-bit integers
                __m128i mul = _mm_madd_epi16(_mm_castps_si128(a.simd), _mm_castps_si128(b.simd));
                // Horizontal add
                __m128i sum = _mm_add_epi32(mul, _mm_srli_si128(mul, 8));
                sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
                return static_cast<T>(_mm_cvtsi128_si32(sum));
            } else {
                // For 32-bit integers
                __m128i va = _mm_castps_si128(a.simd);
                __m128i vb = _mm_castps_si128(b.simd);
                // Multiply low and high parts separately to handle potential overflow
                __m128i lo = _mm_mul_epu32(va, vb);
                __m128i hi = _mm_mul_epu32(_mm_srli_si128(va, 4), _mm_srli_si128(vb, 4));
                __m128i sum = _mm_add_epi64(lo, hi);
                return static_cast<T>(_mm_cvtsi128_si64(sum) +
                                      _mm_cvtsi128_si64(_mm_srli_si128(sum, 8)));
            }
        }
#elif defined(HAVE_NEON)
        if constexpr (std::is_same_v<T, float>) {
            float32x4_t mul = vmulq_f32(a.simd, b.simd);
#if defined(__aarch64__)
            // More efficient on AArch64
            return vaddvq_f32(mul);
#else
            float32x2_t sum = vpadd_f32(vget_low_f32(mul), vget_high_f32(mul));
            return vget_lane_f32(vpadd_f32(sum, sum), 0);
#endif
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (sizeof(T) <= 2) {
                // For 16-bit integers
                int16x8_t mul =
                    vmulq_s16(vreinterpretq_s16_f32(a.simd), vreinterpretq_s16_f32(b.simd));
#if defined(__aarch64__)
                return static_cast<T>(vaddvq_s16(mul));
#else
                int16x4_t sum = vadd_s16(vget_low_s16(mul), vget_high_s16(mul));
                return static_cast<T>(vget_lane_s16(vpadd_s16(sum, sum), 0));
#endif
            } else {
                // For 32-bit integers
                int32x4_t mul =
                    vmulq_s32(vreinterpretq_s32_f32(a.simd), vreinterpretq_s32_f32(b.simd));
#if defined(__aarch64__)
                return static_cast<T>(vaddvq_s32(mul));
#else
                int32x2_t sum = vadd_s32(vget_low_s32(mul), vget_high_s32(mul));
                return static_cast<T>(vget_lane_s32(vpadd_s32(sum, sum), 0));
#endif
            }
        }
#endif
    }
    // Fallback scalar implementation
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

template <typename T>
[[nodiscard]] constexpr Vec3<decltype(T{} * T{} - T{} * T{})> Cross(const Vec3<T>& a,
                                                                    const Vec3<T>& b) {
    if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_AVX)
        Vec3<T> result;
        __m256 va = _mm256_setr_ps(a.x, a.y, a.z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        __m256 vb = _mm256_setr_ps(b.x, b.y, b.z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

        // Shuffle components for cross product
        __m256 va_yzx = _mm256_permute_ps(va, _MM_SHUFFLE(3, 0, 2, 1));
        __m256 vb_yzx = _mm256_permute_ps(vb, _MM_SHUFFLE(3, 0, 2, 1));

        // Compute cross product
        __m256 mul1 = _mm256_mul_ps(va, vb_yzx);
        __m256 mul2 = _mm256_mul_ps(vb, va_yzx);
        __m256 res = _mm256_sub_ps(mul1, mul2);

        // Extract results
        result.x = _mm_cvtss_f32(_mm256_extractf128_ps(res, 0));
        result.y = _mm_cvtss_f32(
            _mm256_extractf128_ps(_mm256_permute_ps(res, _MM_SHUFFLE(1, 1, 1, 1)), 0));
        result.z = _mm_cvtss_f32(
            _mm256_extractf128_ps(_mm256_permute_ps(res, _MM_SHUFFLE(2, 2, 2, 2)), 0));

        return result;
#elif defined(HAVE_SSE2)
        if constexpr (std::is_same_v<T, float>) {
            Vec3<T> result;
            // Load vectors and create shuffled copies
            __m128 va = _mm_load_ps(&a.x);
            __m128 vb = _mm_load_ps(&b.x);
            __m128 a_yzx = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 0, 2, 1));
            __m128 b_yzx = _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 0, 2, 1));
            // Compute cross product with FMA if available
#if defined(HAVE_FMA)
            __m128 c = _mm_fmsub_ps(va, b_yzx, _mm_mul_ps(vb, a_yzx));
#else
            __m128 c = _mm_sub_ps(_mm_mul_ps(va, b_yzx), _mm_mul_ps(vb, a_yzx));
#endif
            // Store result with correct component ordering
            _mm_store_ps(&result.x, _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1)));
            return result;
        } else if constexpr (std::is_integral_v<T>) {
            Vec3<T> result;
            // Handle integer cross product with overflow protection

            __m128i va = _mm_setr_epi32(static_cast<int32_t>(a.x), static_cast<int32_t>(a.y),
                                        static_cast<int32_t>(a.z), 0);
            __m128i vb = _mm_setr_epi32(static_cast<int32_t>(b.x), static_cast<int32_t>(b.y),
                                        static_cast<int32_t>(b.z), 0);

            __m128i a_yzx = _mm_shuffle_epi32(va, _MM_SHUFFLE(3, 0, 2, 1));
            __m128i b_yzx = _mm_shuffle_epi32(vb, _MM_SHUFFLE(3, 0, 2, 1));

            // Compute products with wider type
            __m128i mul1 = _mm_mullo_epi32(va, b_yzx);
            __m128i mul2 = _mm_mullo_epi32(vb, a_yzx);
            __m128i diff = _mm_sub_epi32(mul1, mul2);

            // Store with correct ordering
            result.x = static_cast<T>(_mm_extract_epi32(diff, 1));
            result.y = static_cast<T>(_mm_extract_epi32(diff, 2));
            result.z = static_cast<T>(_mm_extract_epi32(diff, 0));
            return result;
        }
#elif defined(HAVE_NEON)
        if constexpr (std::is_same_v<T, float>) {
            float32x4_t va = vld1q_f32(&a.x);
            float32x4_t vb = vld1q_f32(&b.x);

// Optimize component shuffling
#if defined(__aarch64__)
            // Use dedicated instructions on AArch64
            float32x4_t a_yzx = vextq_f32(va, va, 1);
            float32x4_t b_yzx = vextq_f32(vb, vb, 1);

// Use fused multiply-subtract if available
#if defined(__ARM_FEATURE_FMA)
            float32x4_t result = vfmsq_f32(vmulq_f32(va, b_yzx), vb, a_yzx);
#else
            float32x4_t result = vsubq_f32(vmulq_f32(va, b_yzx), vmulq_f32(vb, a_yzx));
#endif
#else
            // Optimized for ARM32
            float32x2_t a_low = vget_low_f32(va);
            float32x2_t a_high = vget_high_f32(va);
            float32x2_t b_low = vget_low_f32(vb);
            float32x2_t b_high = vget_high_f32(vb);

            float32x2x2_t a_crossed = vzip_f32(a_high, a_low);
            float32x2x2_t b_crossed = vzip_f32(b_high, b_low);

            float32x4_t result =
                vsubq_f32(vmulq_f32(vcombine_f32(a_crossed.val[0], a_crossed.val[1]),
                                    vcombine_f32(b_crossed.val[0], b_crossed.val[1])),
                          vmulq_f32(vcombine_f32(b_crossed.val[1], b_crossed.val[0]),
                                    vcombine_f32(a_crossed.val[1], a_crossed.val[0])));
#endif

            Vec3<T> ret;
            vst1q_f32(&ret.x, result);
            return ret;
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (sizeof(T) <= 2) {
                // For 16-bit integers
                int16x4_t va = {static_cast<int16_t>(a.x), static_cast<int16_t>(a.y),
                                static_cast<int16_t>(a.z), 0};
                int16x4_t vb = {static_cast<int16_t>(b.x), static_cast<int16_t>(b.y),
                                static_cast<int16_t>(b.z), 0};

                int32x4_t mul1 = vmull_s16(va, vext_s16(vb, vb, 1));
                int32x4_t mul2 = vmull_s16(vb, vext_s16(va, va, 1));
                int32x4_t diff = vsubq_s32(mul1, mul2);

                Vec3<T> result;
                result.x = static_cast<T>(vgetq_lane_s32(diff, 1));
                result.y = static_cast<T>(vgetq_lane_s32(diff, 2));
                result.z = static_cast<T>(vgetq_lane_s32(diff, 0));
                return result;
            } else {
                // For 32-bit integers
                int32x4_t va = vld1q_s32(reinterpret_cast<const int32_t*>(&a.x));
                int32x4_t vb = vld1q_s32(reinterpret_cast<const int32_t*>(&b.x));

                int32x4_t a_yzx = vextq_s32(va, va, 1);
                int32x4_t b_yzx = vextq_s32(vb, vb, 1);

                int32x4_t result = vsubq_s32(vmulq_s32(va, b_yzx), vmulq_s32(vb, a_yzx));

                Vec3<T> ret;
                vst1q_s32(reinterpret_cast<int32_t*>(&ret.x), result);
                return ret;
            }
        }
#endif
    }
    // Fallback scalar implementation
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// linear interpolation via float: 0.0=begin, 1.0=end
template <typename X>
[[nodiscard]] constexpr decltype(X{} * float{} + X{} * float{}) Lerp(const X& begin, const X& end,
                                                                     const float t) {
    if constexpr (detail::is_vectorizable<X>::value) {
#if defined(HAVE_SSE2)
        const __m128 vt = _mm_set1_ps(t);
        const __m128 invt = _mm_sub_ps(_mm_set1_ps(1.0f), vt);

        if constexpr (std::is_same_v<X, Vec2<float>>) {
            // Optimized Vec2 implementation
#if defined(HAVE_FMA)
            // Use FMA for better precision
            const __m128 vbegin = _mm_setr_ps(begin.x, begin.y, 0.0f, 0.0f);
            const __m128 vend = _mm_setr_ps(end.x, end.y, 0.0f, 0.0f);
            const __m128 result = _mm_fmadd_ps(vend, vt, _mm_mul_ps(vbegin, invt));
#else
            const __m128 vbegin =
                _mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<const __m64*>(&begin.x));
            const __m128 vend =
                _mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<const __m64*>(&end.x));
            const __m128 result = _mm_add_ps(_mm_mul_ps(vbegin, invt), _mm_mul_ps(vend, vt));
#endif
            Vec2<float> ret;
            _mm_storel_pi(reinterpret_cast<__m64*>(&ret.x), result);
            return ret;
        } else if constexpr (std::is_same_v<X, Vec3<float>>) {
            // Optimized Vec3 implementation
#if defined(HAVE_FMA)
            const __m128 vbegin = _mm_load_ps(&begin.x);
            const __m128 vend = _mm_load_ps(&end.x);
            const __m128 result = _mm_fmadd_ps(vend, vt, _mm_mul_ps(vbegin, invt));
#else
            const __m128 vbegin = _mm_load_ps(&begin.x);
            const __m128 vend = _mm_load_ps(&end.x);
            const __m128 result = _mm_add_ps(_mm_mul_ps(vbegin, invt), _mm_mul_ps(vend, vt));
#endif
            Vec3<float> ret;
            _mm_store_ps(&ret.x, result);
            return ret;
        } else if constexpr (std::is_same_v<X, Vec4<float>>) {
            // Optimized Vec4 implementation
#if defined(HAVE_FMA)
            Vec4<float> ret;
            ret.simd = _mm_fmadd_ps(end.simd, vt, _mm_mul_ps(begin.simd, invt));
#else
            Vec4<float> ret;
            ret.simd = _mm_add_ps(_mm_mul_ps(begin.simd, invt), _mm_mul_ps(end.simd, vt));
#endif
            return ret;
        }

#elif defined(HAVE_NEON)
        if constexpr (std::is_same_v<X, Vec2<float>>) {
            // Optimized Vec2 implementation
            const float32x2_t vbegin = vld1_f32(&begin.x);
            const float32x2_t vend = vld1_f32(&end.x);
            const float32x2_t vt = vdup_n_f32(t);
#if defined(__ARM_FEATURE_FMA)
            const float32x2_t invt = vdup_n_f32(1.0f);
            const float32x2_t result = vfma_f32(vmul_f32(vbegin, invt), vend, vt);
#else
            const float32x2_t invt = vdup_n_f32(1.0f - t);
            const float32x2_t result = vadd_f32(vmul_f32(vbegin, invt), vmul_f32(vend, vt));
#endif
            Vec2<float> ret;
            vst1_f32(&ret.x, result);
            return ret;
        } else if constexpr (std::is_same_v<X, Vec3<float>>) {
            // Optimized Vec3 implementation
            const float32x4_t vbegin = vld1q_f32(&begin.x);
            const float32x4_t vend = vld1q_f32(&end.x);
            const float32x4_t vt = vdupq_n_f32(t);
#if defined(__ARM_FEATURE_FMA)
            const float32x4_t invt = vdupq_n_f32(1.0f);
            const float32x4_t result = vfmaq_f32(vmulq_f32(vbegin, invt), vend, vt);
#else
            const float32x4_t invt = vdupq_n_f32(1.0f - t);
            const float32x4_t result = vaddq_f32(vmulq_f32(vbegin, invt), vmulq_f32(vend, vt));
#endif
            Vec3<float> ret;
            vst1q_f32(&ret.x, result);
            return ret;
        } else if constexpr (std::is_same_v<X, Vec4<float>>) {
            // Optimized Vec4 implementation
            const float32x4_t vt = vdupq_n_f32(t);
#if defined(__ARM_FEATURE_FMA)
            const float32x4_t invt = vdupq_n_f32(1.0f);
            Vec4<float> ret;
            ret.simd = vfmaq_f32(vmulq_f32(begin.simd, invt), end.simd, vt);
#else
            const float32x4_t invt = vdupq_n_f32(1.0f - t);
            Vec4<float> ret;
            ret.simd = vaddq_f32(vmulq_f32(begin.simd, invt), vmulq_f32(end.simd, vt));
#endif
            return ret;
        }
#endif
    }
    // Fallback for non-vectorizable types
    return begin * (1.0f - t) + end * t;
}

// linear interpolation via int: 0=begin, base=end
template <typename X, int base>
[[nodiscard]] constexpr decltype((X{} * int{} + X{} * int{}) / base) LerpInt(const X& begin,
                                                                             const X& end,
                                                                             const int t) {
    static_assert(base > 0, "Base must be positive");
    static_assert(base <= std::numeric_limits<int>::max(), "Base too large");

    if constexpr (detail::is_vectorizable<X>::value) {
#if defined(HAVE_SSE2)
        // Precompute common values
        const __m128 vt = _mm_set1_ps(static_cast<float>(t));
        const __m128 vbase_minus_t = _mm_set1_ps(static_cast<float>(base - t));
        const __m128 inv_base = _mm_set1_ps(1.0f / static_cast<float>(base));

        if constexpr (std::is_same_v<X, Vec2<float>>) {
#if defined(HAVE_FMA)
            // Use FMA for better precision
            const __m128 vbegin =
                _mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<const __m64*>(&begin.x));
            const __m128 vend =
                _mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<const __m64*>(&end.x));
            const __m128 result =
                _mm_mul_ps(_mm_fmadd_ps(vend, vt, _mm_mul_ps(vbegin, vbase_minus_t)), inv_base);
#else
            const __m128 vbegin =
                _mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<const __m64*>(&begin.x));
            const __m128 vend =
                _mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<const __m64*>(&end.x));
            const __m128 result = _mm_mul_ps(
                _mm_add_ps(_mm_mul_ps(vbegin, vbase_minus_t), _mm_mul_ps(vend, vt)), inv_base);
#endif
            Vec2<float> ret;
            _mm_storel_pi(reinterpret_cast<__m64*>(&ret.x), result);
            return ret;
        } else if constexpr (std::is_same_v<X, Vec3<float>>) {
#if defined(HAVE_FMA)
            const __m128 vbegin = _mm_load_ps(&begin.x);
            const __m128 vend = _mm_load_ps(&end.x);
            const __m128 result =
                _mm_mul_ps(_mm_fmadd_ps(vend, vt, _mm_mul_ps(vbegin, vbase_minus_t)), inv_base);
#else
            const __m128 vbegin = _mm_load_ps(&begin.x);
            const __m128 vend = _mm_load_ps(&end.x);
            const __m128 result = _mm_mul_ps(
                _mm_add_ps(_mm_mul_ps(vbegin, vbase_minus_t), _mm_mul_ps(vend, vt)), inv_base);
#endif
            Vec3<float> ret;
            _mm_store_ps(&ret.x, result);
            return ret;
        } else if constexpr (std::is_same_v<X, Vec4<float>>) {
#if defined(HAVE_FMA)
            Vec4<float> ret;
            ret.simd = _mm_mul_ps(_mm_fmadd_ps(end.simd, vt, _mm_mul_ps(begin.simd, vbase_minus_t)),
                                  inv_base);
#else
            Vec4<float> ret;
            ret.simd = _mm_mul_ps(
                _mm_add_ps(_mm_mul_ps(begin.simd, vbase_minus_t), _mm_mul_ps(end.simd, vt)),
                inv_base);
#endif
            return ret;
        }

#elif defined(HAVE_NEON)
        const float inv_base = 1.0f / static_cast<float>(base);

        if constexpr (std::is_same_v<X, Vec2<float>>) {
            const float32x2_t vbegin = vld1_f32(&begin.x);
            const float32x2_t vend = vld1_f32(&end.x);
            const float32x2_t vt = vdup_n_f32(static_cast<float>(t));
            const float32x2_t vbase_minus_t = vdup_n_f32(static_cast<float>(base - t));
            const float32x2_t vinv_base = vdup_n_f32(inv_base);

#if defined(__ARM_FEATURE_FMA)
            const float32x2_t result =
                vmul_f32(vfma_f32(vmul_f32(vbegin, vbase_minus_t), vend, vt), vinv_base);
#else
            const float32x2_t result =
                vmul_f32(vadd_f32(vmul_f32(vbegin, vbase_minus_t), vmul_f32(vend, vt)), vinv_base);
#endif
            Vec2<float> ret;
            vst1_f32(&ret.x, result);
            return ret;
        } else if constexpr (std::is_same_v<X, Vec3<float>>) {
            const float32x4_t vbegin = vld1q_f32(&begin.x);
            const float32x4_t vend = vld1q_f32(&end.x);
            const float32x4_t vt = vdupq_n_f32(static_cast<float>(t));
            const float32x4_t vbase_minus_t = vdupq_n_f32(static_cast<float>(base - t));
            const float32x4_t vinv_base = vdupq_n_f32(inv_base);

#if defined(__ARM_FEATURE_FMA)
            const float32x4_t result =
                vmulq_f32(vfmaq_f32(vmulq_f32(vbegin, vbase_minus_t), vend, vt), vinv_base);
#else
            const float32x4_t result = vmulq_f32(
                vaddq_f32(vmulq_f32(vbegin, vbase_minus_t), vmulq_f32(vend, vt)), vinv_base);
#endif
            Vec3<float> ret;
            vst1q_f32(&ret.x, result);
            return ret;
        } else if constexpr (std::is_same_v<X, Vec4<float>>) {
            const float32x4_t vt = vdupq_n_f32(static_cast<float>(t));
            const float32x4_t vbase_minus_t = vdupq_n_f32(static_cast<float>(base - t));
            const float32x4_t vinv_base = vdupq_n_f32(inv_base);

#if defined(__ARM_FEATURE_FMA)
            Vec4<float> ret;
            ret.simd =
                vmulq_f32(vfmaq_f32(vmulq_f32(begin.simd, vbase_minus_t), end.simd, vt), vinv_base);
#else
            Vec4<float> ret;
            ret.simd =
                vmulq_f32(vaddq_f32(vmulq_f32(begin.simd, vbase_minus_t), vmulq_f32(end.simd, vt)),
                          vinv_base);
#endif
            return ret;
        }
#endif
    }
    // Fallback for non-vectorizable types
    return (begin * (base - t) + end * t) / base;
}

// bilinear interpolation. s is for interpolating x00-x01 and x10-x11, and t is for the second
// interpolation.
template <typename X>
[[nodiscard]] constexpr auto BilinearInterp(const X& x00, const X& x01, const X& x10, const X& x11,
                                            const float s, const float t) {
    auto y0 = Lerp(x00, x01, s);
    auto y1 = Lerp(x10, x11, s);
    return Lerp(y0, y1, t);
}

// Utility vector factories
template <typename T>
[[nodiscard]] constexpr Vec2<T> MakeVec(const T& x, const T& y) {
    return Vec2<T>{x, y};
}

template <typename T>
[[nodiscard]] constexpr Vec3<T> MakeVec(const T& x, const T& y, const T& z) {
    return Vec3<T>{x, y, z};
}

template <typename T>
[[nodiscard]] constexpr Vec4<T> MakeVec(const T& x, const T& y, const Vec2<T>& zw) {
    return MakeVec(x, y, zw[0], zw[1]);
}

template <typename T>
[[nodiscard]] constexpr Vec3<T> MakeVec(const Vec2<T>& xy, const T& z) {
    return MakeVec(xy[0], xy[1], z);
}

template <typename T>
[[nodiscard]] constexpr Vec3<T> MakeVec(const T& x, const Vec2<T>& yz) {
    return MakeVec(x, yz[0], yz[1]);
}

template <typename T>
[[nodiscard]] constexpr Vec4<T> MakeVec(const T& x, const T& y, const T& z, const T& w) {
    return Vec4<T>{x, y, z, w};
}

template <typename T>
[[nodiscard]] constexpr Vec4<T> MakeVec(const Vec2<T>& xy, const T& z, const T& w) {
    return MakeVec(xy[0], xy[1], z, w);
}

template <typename T>
[[nodiscard]] constexpr Vec4<T> MakeVec(const T& x, const Vec2<T>& yz, const T& w) {
    return MakeVec(x, yz[0], yz[1], w);
}

// NOTE: This has priority over "Vec2<Vec2<T>> MakeVec(const Vec2<T>& x, const Vec2<T>& y)".
//       Even if someone wanted to use an odd object like Vec2<Vec2<T>>, the compiler would error
//       out soon enough due to misuse of the returned structure.
template <typename T>
[[nodiscard]] constexpr Vec4<T> MakeVec(const Vec2<T>& xy, const Vec2<T>& zw) {
    return MakeVec(xy[0], xy[1], zw[0], zw[1]);
}

template <typename T>
[[nodiscard]] constexpr Vec4<T> MakeVec(const Vec3<T>& xyz, const T& w) {
    return MakeVec(xyz[0], xyz[1], xyz[2], w);
}

template <typename T>
[[nodiscard]] constexpr Vec4<T> MakeVec(const T& x, const Vec3<T>& yzw) {
    return MakeVec(x, yzw[0], yzw[1], yzw[2]);
}

} // namespace Common

#endif
