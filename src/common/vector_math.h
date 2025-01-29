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

#include <cmath>
#include <cstring>
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
            // Load xy components into lower half of XMM register
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 b = _mm_setr_ps(other.x, other.y, 0.0f, 0.0f);
            __m128 sum = _mm_add_ps(a, b);
            result.x = _mm_cvtss_f32(sum);
            result.y = _mm_cvtss_f32(_mm_shuffle_ps(sum, sum, _MM_SHUFFLE(1, 1, 1, 1)));
#elif defined(HAVE_NEON)
            float32x2_t a = {x, y};
            float32x2_t b = {other.x, other.y};
            float32x2_t sum = vadd_f32(a, b);
            result.x = vget_lane_f32(sum, 0);
            result.y = vget_lane_f32(sum, 1);
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
            x = _mm_cvtss_f32(sum);
            y = _mm_cvtss_f32(_mm_shuffle_ps(sum, sum, _MM_SHUFFLE(1, 1, 1, 1)));
#elif defined(HAVE_NEON)
            float32x2_t a = {x, y};
            float32x2_t b = {other.x, other.y};
            float32x2_t sum = vadd_f32(a, b);
            x = vget_lane_f32(sum, 0);
            y = vget_lane_f32(sum, 1);
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
            result.x = _mm_cvtss_f32(diff);
            result.y = _mm_cvtss_f32(_mm_shuffle_ps(diff, diff, _MM_SHUFFLE(1, 1, 1, 1)));
#elif defined(HAVE_NEON)
            float32x2_t a = {x, y};
            float32x2_t b = {other.x, other.y};
            float32x2_t diff = vsub_f32(a, b);
            result.x = vget_lane_f32(diff, 0);
            result.y = vget_lane_f32(diff, 1);
#endif
            return result;
        } else {
            return {x - other.x, y - other.y};
        }
    }
    constexpr Vec2& operator-=(const Vec2& other) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            __m128 a = _mm_setr_ps(x, y, 0.0f, 0.0f);
            __m128 b = _mm_setr_ps(other.x, other.y, 0.0f, 0.0f);
            __m128 diff = _mm_sub_ps(a, b);
            x = _mm_cvtss_f32(diff);
            y = _mm_cvtss_f32(_mm_shuffle_ps(diff, diff, _MM_SHUFFLE(1, 1, 1, 1)));
#elif defined(HAVE_NEON)
            float32x2_t a = {x, y};
            float32x2_t b = {other.x, other.y};
            float32x2_t diff = vsub_f32(a, b);
            x = vget_lane_f32(diff, 0);
            y = vget_lane_f32(diff, 1);
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
            result.x = _mm_cvtss_f32(neg);
            result.y = _mm_cvtss_f32(_mm_shuffle_ps(neg, neg, _MM_SHUFFLE(1, 1, 1, 1)));
#elif defined(HAVE_NEON)
            float32x2_t a = {x, y};
            float32x2_t neg = vneg_f32(a);
            result.x = vget_lane_f32(neg, 0);
            result.y = vget_lane_f32(neg, 1);
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
            result.x = _mm_cvtss_f32(prod);
            result.y = _mm_cvtss_f32(_mm_shuffle_ps(prod, prod, _MM_SHUFFLE(1, 1, 1, 1)));
#elif defined(HAVE_NEON)
            float32x2_t a = {x, y};
            float32x2_t b = {other.x, other.y};
            float32x2_t prod = vmul_f32(a, b);
            result.x = vget_lane_f32(prod, 0);
            result.y = vget_lane_f32(prod, 1);
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
            result.x = _mm_cvtss_f32(prod);
            result.y = _mm_cvtss_f32(_mm_shuffle_ps(prod, prod, _MM_SHUFFLE(1, 1, 1, 1)));
#elif defined(HAVE_NEON)
            float32x2_t a = {x, y};
            float32x2_t b = vdup_n_f32(f);
            float32x2_t prod = vmul_f32(a, b);
            result.x = vget_lane_f32(prod, 0);
            result.y = vget_lane_f32(prod, 1);
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
            x = _mm_cvtss_f32(prod);
            y = _mm_cvtss_f32(_mm_shuffle_ps(prod, prod, _MM_SHUFFLE(1, 1, 1, 1)));
#elif defined(HAVE_NEON)
            float32x2_t a = {x, y};
            float32x2_t b = vdup_n_f32(f);
            float32x2_t prod = vmul_f32(a, b);
            x = vget_lane_f32(prod, 0);
            y = vget_lane_f32(prod, 1);
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
            result.x = _mm_cvtss_f32(quot);
            result.y = _mm_cvtss_f32(_mm_shuffle_ps(quot, quot, _MM_SHUFFLE(1, 1, 1, 1)));
#elif defined(HAVE_NEON)
            float32x2_t a = {x, y};
            // NEON doesn't have direct division, use reciprocal multiplication
            float32x2_t recip = vdup_n_f32(1.0f / f);
            float32x2_t quot = vmul_f32(a, recip);
            result.x = vget_lane_f32(quot, 0);
            result.y = vget_lane_f32(quot, 1);
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
            x = _mm_cvtss_f32(quot);
            y = _mm_cvtss_f32(_mm_shuffle_ps(quot, quot, _MM_SHUFFLE(1, 1, 1, 1)));
#elif defined(HAVE_NEON)
            float32x2_t a = {x, y};
            // NEON doesn't have direct division, use reciprocal multiplication
            float32x2_t recip = vdup_n_f32(1.0f / f);
            float32x2_t quot = vmul_f32(a, recip);
            x = vget_lane_f32(quot, 0);
            y = vget_lane_f32(quot, 1);
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
            // Add x+y components
            return _mm_cvtss_f32(_mm_add_ss(sq, _mm_shuffle_ps(sq, sq, _MM_SHUFFLE(1, 1, 1, 1))));
#elif defined(HAVE_NEON)
            float32x2_t v = {x, y};
            float32x2_t sq = vmul_f32(v, v);
            return vget_lane_f32(vpadd_f32(sq, sq), 0);
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
            return (_mm_movemask_ps(cmp) & 0x3) != 0x3; // Check only x,y (mask 0x3)
#elif defined(HAVE_NEON)
            float32x2_t a = {x, y};
            float32x2_t b = {other.x, other.y};
            uint32x2_t cmp = vceq_f32(a, b);
            return (vget_lane_u32(cmp, 0) & vget_lane_u32(cmp, 1)) != 0xFFFFFFFF;
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
            return (_mm_movemask_ps(cmp) & 0x3) == 0x3; // Check only x,y (mask 0x3)
#elif defined(HAVE_NEON)
            float32x2_t a = {x, y};
            float32x2_t b = {other.x, other.y};
            uint32x2_t cmp = vceq_f32(a, b);
            return (vget_lane_u32(cmp, 0) & vget_lane_u32(cmp, 1)) == 0xFFFFFFFF;
#endif
        } else {
            return std::memcmp(AsArray(), other.AsArray(), sizeof(Vec2)) == 0;
        }
    }

    // Only implemented for T=float
    [[nodiscard]] float Length() const;
    float Normalize(); // returns the previous length, which is often useful

    [[nodiscard]] constexpr T& operator[](std::size_t i) {
        return *((&x) + i);
    }
    [[nodiscard]] constexpr const T& operator[](std::size_t i) const {
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
[[nodiscard]] constexpr Vec2<T> operator*(const V& f, const Vec2<T>& vec) {
    return Vec2<T>(f * vec.x, f * vec.y);
}

using Vec2f = Vec2<float>;
using Vec2i = Vec2<int>;
using Vec2u = Vec2<unsigned int>;

template <>
inline float Vec2<float>::Length() const {
#if defined(HAVE_SSE4_1)
    // SSE4.1 has a dedicated dot product instruction
    __m128 v = _mm_setr_ps(x, y, 0.0f, 0.0f);
    return _mm_cvtss_f32(_mm_sqrt_ss(
        _mm_dp_ps(v, v, 0x31))); // 0x31: only multiply xy (0x3) and store in lowest (0x1)
#elif defined(HAVE_SSE2)
    __m128 v = _mm_setr_ps(x, y, 0.0f, 0.0f);
    __m128 sq = _mm_mul_ps(v, v); // Square components
    __m128 sum = _mm_add_ss(sq, _mm_shuffle_ps(sq, sq, _MM_SHUFFLE(1, 1, 1, 1))); // Add x^2 + y^2
    return _mm_cvtss_f32(_mm_sqrt_ss(sum));                                       // sqrt(x^2 + y^2)
#elif defined(HAVE_NEON)
    float32x2_t v = {x, y};
    float32x2_t sq = vmul_f32(v, v);     // Square components
    float32x2_t sum = vpadd_f32(sq, sq); // Horizontal add
    return sqrt(vget_lane_f32(sum, 0));  // sqrt(x^2 + y^2)
#else
    return std::sqrt(x * x + y * y);
#endif
}

template <>
inline float Vec2<float>::Normalize() {
    float length = Length();
    *this /= length;
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
    T x;
    T y;
    T z;
    T pad; // For SIMD alignment

    constexpr Vec3() = default;
    constexpr Vec3(const T& x_, const T& y_, const T& z_) : x(x_), y(y_), z(z_) {}

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
                _mm_store_ps(&result.x, _mm_load_ps(&x));
            } else {
                // Fallback to scalar for other type conversions
                result.x = static_cast<T2>(x);
                result.y = static_cast<T2>(y);
                result.z = static_cast<T2>(z);
            }
#elif defined(HAVE_NEON)
            if constexpr (std::is_same_v<T, float> && std::is_same_v<T2, float>) {
                vst1q_f32(&result.x, vld1q_f32(&x));
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
            _mm_store_ps(&result.x, _mm_set_ps1(f));
#elif defined(HAVE_NEON)
            vst1q_f32(&result.x, vdupq_n_f32(f));
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
            _mm_store_ps(&result.x, _mm_add_ps(_mm_load_ps(&x), _mm_load_ps(&other.x)));
#elif defined(HAVE_NEON)
            vst1q_f32(&result.x, vaddq_f32(vld1q_f32(&x), vld1q_f32(&other.x)));
#endif
            return result;
        } else {
            return {x + other.x, y + other.y, z + other.z};
        }
    }

    constexpr Vec3& operator+=(const Vec3& other) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            _mm_store_ps(&x, _mm_add_ps(_mm_load_ps(&x), _mm_load_ps(&other.x)));
#elif defined(HAVE_NEON)
            vst1q_f32(&x, vaddq_f32(vld1q_f32(&x), vld1q_f32(&other.x)));
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
            _mm_store_ps(&result.x, _mm_sub_ps(_mm_load_ps(&x), _mm_load_ps(&other.x)));
#elif defined(HAVE_NEON)
            vst1q_f32(&result.x, vsubq_f32(vld1q_f32(&x), vld1q_f32(&other.x)));
#endif
            return result;
        } else {
            return {x - other.x, y - other.y, z - other.z};
        }
    }

    constexpr Vec3& operator-=(const Vec3& other) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            _mm_store_ps(&x, _mm_sub_ps(_mm_load_ps(&x), _mm_load_ps(&other.x)));
#elif defined(HAVE_NEON)
            vst1q_f32(&x, vsubq_f32(vld1q_f32(&x), vld1q_f32(&other.x)));
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
        return {-x, -y, -z};
    }

    [[nodiscard]] constexpr Vec3<decltype(T{} * T{})> operator*(const Vec3& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec3<T> result;
#if defined(HAVE_SSE2)
            _mm_store_ps(&result.x, _mm_mul_ps(_mm_load_ps(&x), _mm_load_ps(&other.x)));
#elif defined(HAVE_NEON)
            vst1q_f32(&result.x, vmulq_f32(vld1q_f32(&x), vld1q_f32(&other.x)));
#endif
            return result;
        } else {
            return {x * other.x, y * other.y, z * other.z};
        }
    }

    template <typename V>
    [[nodiscard]] constexpr Vec3<decltype(T{} * V{})> operator*(const V& f) const {
        if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
            Vec3<decltype(T{} * V{})> result;
#if defined(HAVE_SSE2)
            _mm_store_ps(&result.x, _mm_mul_ps(_mm_load_ps(&x), _mm_set1_ps(f)));
#elif defined(HAVE_NEON)
            vst1q_f32(&result.x, vmulq_f32(vld1q_f32(&x), vdupq_n_f32(f)));
#endif
            return result;
        } else {
            return {x * f, y * f, z * f};
        }
    }

    template <typename V>
    constexpr Vec3& operator*=(const V& f) {
        if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
#if defined(HAVE_SSE2)
            _mm_store_ps(&x, _mm_mul_ps(_mm_load_ps(&x), _mm_set1_ps(f)));
#elif defined(HAVE_NEON)
            vst1q_f32(&x, vmulq_f32(vld1q_f32(&x), vdupq_n_f32(f)));
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
        if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
            Vec3<decltype(T{} / V{})> result;
#if defined(HAVE_SSE2)
            _mm_store_ps(&result.x, _mm_div_ps(_mm_load_ps(&x), _mm_set1_ps(f)));
#elif defined(HAVE_NEON)
            // NEON doesn't have a direct divide instruction, so we multiply by reciprocal
            float32x4_t recip = vrecpeq_f32(vdupq_n_f32(f));
            // One Newton-Raphson iteration for better precision
            recip = vmulq_f32(vrecpsq_f32(vdupq_n_f32(f), recip), recip);
            vst1q_f32(&result.x, vmulq_f32(vld1q_f32(&x), recip));
#endif
            return result;
        } else {
            return {x / f, y / f, z / f};
        }
    }

    template <typename V>
    constexpr Vec3& operator/=(const V& f) {
        if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
#if defined(HAVE_SSE2)
            _mm_store_ps(&x, _mm_div_ps(_mm_load_ps(&x), _mm_set1_ps(f)));
#elif defined(HAVE_NEON)
            // NEON doesn't have a direct divide instruction, so we multiply by reciprocal
            float32x4_t recip = vrecpeq_f32(vdupq_n_f32(f));
            // One Newton-Raphson iteration for better precision
            recip = vmulq_f32(vrecpsq_f32(vdupq_n_f32(f), recip), recip);
            vst1q_f32(&x, vmulq_f32(vld1q_f32(&x), recip));
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
            __m128 cmp = _mm_cmpeq_ps(_mm_load_ps(&x), _mm_load_ps(&other.x));
            return (_mm_movemask_ps(cmp) & 0x7) == 0x7; // Check only x,y,z (mask 0x7)
#elif defined(HAVE_NEON)
            uint32x4_t cmp = vceqq_f32(vld1q_f32(&x), vld1q_f32(&other.x));
            return (vgetq_lane_u32(cmp, 0) & vgetq_lane_u32(cmp, 1) & vgetq_lane_u32(cmp, 2)) ==
                   0xFFFFFFFF;
#endif
        } else {
            return x == other.x && y == other.y && z == other.z;
        }
    }

    [[nodiscard]] constexpr T Length2() const {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            __m128 v = _mm_load_ps(&x);
            __m128 sq = _mm_mul_ps(v, v);
            // Add x+y+z components (we don't care about w)
            __m128 sum = _mm_add_ss(_mm_add_ss(sq, _mm_shuffle_ps(sq, sq, _MM_SHUFFLE(1, 1, 1, 1))),
                                    _mm_shuffle_ps(sq, sq, _MM_SHUFFLE(2, 2, 2, 2)));
            return _mm_cvtss_f32(sum);
#elif defined(HAVE_NEON)
            float32x4_t v = vld1q_f32(&x);
            float32x4_t sq = vmulq_f32(v, v);
            float32x2_t sum = vpadd_f32(vget_low_f32(sq), vget_high_f32(sq));
            return vget_lane_f32(vpadd_f32(sum, sum), 0); // Only need x+y+z
#endif
        } else {
            return x * x + y * y + z * z;
        }
    }

    // Only implemented for T=float
    [[nodiscard]] float Length() const;
    [[nodiscard]] Vec3 Normalized() const;
    float Normalize(); // returns the previous length, which is often useful

    [[nodiscard]] constexpr T& operator[](std::size_t i) {
        return *((&x) + i);
    }

    [[nodiscard]] constexpr const T& operator[](std::size_t i) const {
        return *((&x) + i);
    }

    constexpr void SetZero() {
        x = 0;
        y = 0;
        z = 0;
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
    if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
        Vec3<T> result;
#if defined(HAVE_SSE2)
        _mm_store_ps(&result.x, _mm_mul_ps(_mm_set1_ps(f), _mm_load_ps(&vec.x)));
#elif defined(HAVE_NEON)
        vst1q_f32(&result.x, vmulq_f32(vdupq_n_f32(f), vld1q_f32(&vec.x)));
#endif
        return result;
    } else {
        return Vec3<T>(f * vec.x, f * vec.y, f * vec.z);
    }
}

template <>
inline float Vec3<float>::Length() const {
#if defined(HAVE_SSE2)
    __m128 v = _mm_load_ps(&x);
    __m128 sq = _mm_mul_ps(v, v);
    // Horizontal add x+y+z components
    __m128 sum = _mm_add_ss(_mm_add_ss(sq, _mm_shuffle_ps(sq, sq, _MM_SHUFFLE(1, 1, 1, 1))),
                            _mm_shuffle_ps(sq, sq, _MM_SHUFFLE(2, 2, 2, 2)));
    return _mm_cvtss_f32(_mm_sqrt_ss(sum));
#elif defined(HAVE_NEON)
    float32x4_t v = vld1q_f32(&x);
    float32x4_t sq = vmulq_f32(v, v);
    float32x2_t sum = vpadd_f32(vget_low_f32(sq), vget_high_f32(sq));
    return sqrt(vget_lane_f32(vpadd_f32(sum, sum), 0)); // Only need x+y+z
#else
    return std::sqrt(x * x + y * y + z * z);
#endif
}

template <>
inline Vec3<float> Vec3<float>::Normalized() const {
#if defined(HAVE_SSE2)
    __m128 v = _mm_load_ps(&x);
    __m128 sq = _mm_mul_ps(v, v);
    // Compute sum of squares
    __m128 sum = _mm_add_ss(_mm_add_ss(sq, _mm_shuffle_ps(sq, sq, _MM_SHUFFLE(1, 1, 1, 1))),
                            _mm_shuffle_ps(sq, sq, _MM_SHUFFLE(2, 2, 2, 2)));
    // Get reciprocal sqrt and broadcast to all elements
    __m128 len = _mm_sqrt_ss(sum);
    __m128 recip = _mm_div_ps(_mm_set1_ps(1.0f), _mm_shuffle_ps(len, len, _MM_SHUFFLE(0, 0, 0, 0)));

    Vec3<float> result;
    _mm_store_ps(&result.x, _mm_mul_ps(v, recip));
    return result;
#elif defined(HAVE_NEON)
    float32x4_t v = vld1q_f32(&x);
    float32x4_t sq = vmulq_f32(v, v);
    float32x2_t sum = vpadd_f32(vget_low_f32(sq), vget_high_f32(sq));
    float32x2_t len = vsqrt_f32(vpadd_f32(sum, sum));
    // Compute reciprocal and broadcast
    float32x4_t recip = vdupq_n_f32(1.0f / vget_lane_f32(len, 0));

    Vec3<float> result;
    vst1q_f32(&result.x, vmulq_f32(v, recip));
    return result;
#else
    return *this / Length();
#endif
}

template <>
inline float Vec3<float>::Normalize() {
    float length = Length();
    *this /= length;
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
    constexpr Vec4(const T& x_, const T& y_, const T& z_, const T& w_) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            simd = _mm_set_ps(w_, z_, y_, x_);
#elif defined(HAVE_NEON)
            simd = {x_, y_, z_, w_};
#endif
        } else {
            x = x_;
            y = y_;
            z = z_;
            w = w_;
        }
    }

    template <typename T2>
    [[nodiscard]] constexpr Vec4<T2> Cast() const {
        return Vec4<T2>(static_cast<T2>(x), static_cast<T2>(y), static_cast<T2>(z),
                        static_cast<T2>(w));
    }

    [[nodiscard]] static constexpr Vec4 AssignToAll(const T& f) {
        return Vec4(f, f, f, f);
    }

    [[nodiscard]] constexpr Vec4<decltype(T{} + T{})> operator+(const Vec4& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec4<T> result;
#if defined(HAVE_SSE2)
            result.simd = _mm_add_ps(simd, other.simd);
#elif defined(HAVE_NEON)
            result.simd = vaddq_f32(simd, other.simd);
#endif
            return result;
        } else {
            return {x + other.x, y + other.y, z + other.z, w + other.w};
        }
    }

    constexpr Vec4& operator+=(const Vec4& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    [[nodiscard]] constexpr Vec4<decltype(T{} - T{})> operator-(const Vec4& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec4<T> result;
#if defined(HAVE_SSE2)
            result.simd = _mm_sub_ps(simd, other.simd);
#elif defined(HAVE_NEON)
            result.simd = vsubq_f32(simd, other.simd);
#endif
            return result;
        } else {
            return {x - other.x, y - other.y, z - other.z, w - other.w};
        }
    }

    constexpr Vec4& operator-=(const Vec4& other) {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            simd = _mm_sub_ps(simd, other.simd);
#elif defined(HAVE_NEON)
            simd = vsubq_f32(simd, other.simd);
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
        return {-x, -y, -z, -w};
    }

    [[nodiscard]] constexpr Vec4<decltype(T{} * T{})> operator*(const Vec4& other) const {
        if constexpr (detail::is_vectorizable<T>::value) {
            Vec4<T> result;
#if defined(HAVE_SSE2)
            result.simd = _mm_mul_ps(simd, other.simd);
#elif defined(HAVE_NEON)
            result.simd = vmulq_f32(simd, other.simd);
#endif
            return result;
        } else {
            return {x * other.x, y * other.y, z * other.z, w * other.w};
        }
    }

    template <typename V>
    [[nodiscard]] constexpr Vec4<decltype(T{} * V{})> operator*(const V& f) const {
        if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
            Vec4<decltype(T{} * V{})> result;
#if defined(HAVE_SSE2)
            result.simd = _mm_mul_ps(simd, _mm_set1_ps(f));
#elif defined(HAVE_NEON)
            result.simd = vmulq_f32(simd, vdupq_n_f32(f));
#endif
            return result;
        } else {
            return {x * f, y * f, z * f, w * f};
        }
    }

    template <typename V>
    constexpr Vec4& operator*=(const V& f) {
        if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
#if defined(HAVE_SSE2)
            simd = _mm_mul_ps(simd, _mm_set1_ps(f));
#elif defined(HAVE_NEON)
            simd = vmulq_f32(simd, vdupq_n_f32(f));
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
        if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
            Vec4<decltype(T{} / V{})> result;
#if defined(HAVE_SSE2)
            result.simd = _mm_div_ps(simd, _mm_set1_ps(f));
#elif defined(HAVE_NEON)
            // NEON doesn't have direct division, use reciprocal multiplication
            float32x4_t recip = vrecpeq_f32(vdupq_n_f32(f));
            // One Newton-Raphson iteration for better precision
            recip = vmulq_f32(vrecpsq_f32(vdupq_n_f32(f), recip), recip);
            result.simd = vmulq_f32(simd, recip);
#endif
            return result;
        } else {
            return {x / f, y / f, z / f, w / f};
        }
    }

    template <typename V>
    constexpr Vec4& operator/=(const V& f) {
        if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
#if defined(HAVE_SSE2)
            simd = _mm_div_ps(simd, _mm_set1_ps(f));
#elif defined(HAVE_NEON)
            // NEON doesn't have direct division, use reciprocal multiplication
            float32x4_t recip = vrecpeq_f32(vdupq_n_f32(f));
            // One Newton-Raphson iteration for better precision
            recip = vmulq_f32(vrecpsq_f32(vdupq_n_f32(f), recip), recip);
            simd = vmulq_f32(simd, recip);
#endif
        } else {
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
            __m128 cmp = _mm_cmpeq_ps(simd, other.simd);
            return _mm_movemask_ps(cmp) == 0xF; // All 4 components must match
#elif defined(HAVE_NEON)
            uint32x4_t cmp = vceqq_f32(simd, other.simd);
            uint32x2_t fold = vand_u32(vget_low_u32(cmp), vget_high_u32(cmp));
            return vget_lane_u32(vpmin_u32(fold, fold), 0) == 0xFFFFFFFF;
#endif
        } else {
            return x == other.x && y == other.y && z == other.z && w == other.w;
        }
    }

    [[nodiscard]] constexpr bool operator!=(const Vec4& other) const {
        return !(*this == other);
    }

    [[nodiscard]] constexpr T Length2() const {
        if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE2)
            __m128 sq = _mm_mul_ps(simd, simd);
            // Horizontal add all four components
            __m128 sum1 = _mm_add_ps(sq, _mm_shuffle_ps(sq, sq, _MM_SHUFFLE(2, 3, 0, 1)));
            __m128 sum2 = _mm_add_ps(sum1, _mm_shuffle_ps(sum1, sum1, _MM_SHUFFLE(1, 0, 3, 2)));
            return _mm_cvtss_f32(sum2);
#elif defined(HAVE_NEON)
            float32x4_t sq = vmulq_f32(simd, simd);
            float32x2_t sum = vpadd_f32(vget_low_f32(sq), vget_high_f32(sq));
            return vget_lane_f32(vpadd_f32(sum, sum), 0);
#endif
        } else {
            return x * x + y * y + z * z + w * w;
        }
    }

    [[nodiscard]] constexpr T& operator[](std::size_t i) {
        return *((&x) + i);
    }

    [[nodiscard]] constexpr const T& operator[](std::size_t i) const {
        return *((&x) + i);
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
    if constexpr (detail::is_vectorizable<T>::value && std::is_same_v<V, float>) {
        Vec4<decltype(V{} * T{})> result;
#if defined(HAVE_SSE2)
        result.simd = _mm_mul_ps(_mm_set1_ps(f), vec.simd);
#elif defined(HAVE_NEON)
        result.simd = vmulq_f32(vdupq_n_f32(f), vec.simd);
#endif
        return result;
    } else {
        return {f * vec.x, f * vec.y, f * vec.z, f * vec.w};
    }
}

using Vec4f = Vec4<float>;
using Vec4i = Vec4<int>;
using Vec4u = Vec4<unsigned int>;

template <typename T>
constexpr decltype(T{} * T{} + T{} * T{}) Dot(const Vec2<T>& a, const Vec2<T>& b) {
    if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE4_1)
        // Load 2D vectors into lower half of SSE register and use SSE4.1 dot product
        __m128 va = _mm_setr_ps(a.x, a.y, 0.0f, 0.0f);
        __m128 vb = _mm_setr_ps(b.x, b.y, 0.0f, 0.0f);
        return _mm_cvtss_f32(
            _mm_dp_ps(va, vb, 0x31)); // 0x31: only multiply xy (0x3) and store in lowest (0x1)
#elif defined(HAVE_SSE2)
        // Load just the two components we need
        __m128 va = _mm_setr_ps(a.x, a.y, 0.0f, 0.0f);
        __m128 vb = _mm_setr_ps(b.x, b.y, 0.0f, 0.0f);
        __m128 mul = _mm_mul_ps(va, vb);
        // Add first two elements (x and y)
        __m128 sum = _mm_add_ss(mul, _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(1, 1, 1, 1)));
        return _mm_cvtss_f32(sum);
#elif defined(HAVE_NEON)
        // Load just the two components into a 64-bit register
        float32x2_t va = {a.x, a.y};
        float32x2_t vb = {b.x, b.y};
        // Multiply and add horizontally in one go
        float32x2_t mul = vmul_f32(va, vb);
        return vget_lane_f32(vpadd_f32(mul, mul), 0);
#endif
    } else {
        return a.x * b.x + a.y * b.y;
    }
}

template <typename T>
[[nodiscard]] constexpr decltype(T{} * T{} + T{} * T{}) Dot(const Vec3<T>& a, const Vec3<T>& b) {
    if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE4_1)
        // SSE4.1 has a dedicated dot product instruction
        // 0xF1 mask: multiply all components (0xF) but only store result in lowest component (0x1)
        return _mm_cvtss_f32(_mm_dp_ps(_mm_load_ps(&a.x), _mm_load_ps(&b.x), 0x71));
#elif defined(HAVE_SSE2)
        __m128 va = _mm_load_ps(&a.x);
        __m128 vb = _mm_load_ps(&b.x);
        __m128 mul = _mm_mul_ps(va, vb);
        // Add x+y components
        __m128 sum = _mm_add_ss(mul, _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(1, 1, 1, 1)));
        // Add z component
        sum = _mm_add_ss(sum, _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 2, 2, 2)));
        return _mm_cvtss_f32(sum);
#elif defined(HAVE_NEON)
        float32x4_t va = vld1q_f32(&a.x);
        float32x4_t vb = vld1q_f32(&b.x);
        float32x4_t mul = vmulq_f32(va, vb);
        float32x2_t sum = vget_low_f32(mul);
        // Add first two elements
        sum = vpadd_f32(sum, sum);
        // Add third element
        return vget_lane_f32(sum, 0) + vgetq_lane_f32(mul, 2);
#endif
    } else {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
}

template <typename T>
[[nodiscard]] constexpr decltype(T{} * T{} + T{} * T{}) Dot(const Vec4<T>& a, const Vec4<T>& b) {
    if constexpr (detail::is_vectorizable<T>::value) {
#if defined(HAVE_SSE4_1)
        // SSE4.1 has a dedicated dot product instruction
        return _mm_cvtss_f32(_mm_dp_ps(a.simd, b.simd, 0xF1));
#elif defined(HAVE_SSE2)
        __m128 mul = _mm_mul_ps(a.simd, b.simd);
        // Add pairs
        __m128 sum1 = _mm_add_ps(mul, _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1)));
        // Add remaining pairs
        __m128 sum2 = _mm_add_ps(sum1, _mm_shuffle_ps(sum1, sum1, _MM_SHUFFLE(1, 0, 3, 2)));
        return _mm_cvtss_f32(sum2);
#elif defined(HAVE_NEON)
        float32x4_t mul = vmulq_f32(a.simd, b.simd);
        float32x2_t sum = vpadd_f32(vget_low_f32(mul), vget_high_f32(mul));
        return vget_lane_f32(vpadd_f32(sum, sum), 0);
#endif
    } else {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }
}

template <typename T>
[[nodiscard]] constexpr Vec3<decltype(T{} * T{} - T{} * T{})> Cross(const Vec3<T>& a,
                                                                    const Vec3<T>& b) {
    if constexpr (detail::is_vectorizable<T>::value) {
        Vec3<T> result;
#if defined(HAVE_SSE2)
        __m128 a_yzx =
            _mm_shuffle_ps(_mm_load_ps(&a.x), _mm_load_ps(&a.x), _MM_SHUFFLE(3, 0, 2, 1));
        __m128 b_yzx =
            _mm_shuffle_ps(_mm_load_ps(&b.x), _mm_load_ps(&b.x), _MM_SHUFFLE(3, 0, 2, 1));
        __m128 c =
            _mm_sub_ps(_mm_mul_ps(_mm_load_ps(&a.x), b_yzx), _mm_mul_ps(_mm_load_ps(&b.x), a_yzx));
        _mm_store_ps(&result.x, _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1)));
#elif defined(HAVE_NEON)
        float32x4_t a_vec = vld1q_f32(&a.x);
        float32x4_t b_vec = vld1q_f32(&b.x);
        float32x4_t a_yzx = vextq_f32(a_vec, a_vec, 1);
        float32x4_t b_yzx = vextq_f32(b_vec, b_vec, 1);
        float32x4_t c = vsubq_f32(vmulq_f32(a_vec, b_yzx), vmulq_f32(b_vec, a_yzx));
        vst1q_f32(&result.x, vextq_f32(c, c, 3));
#endif
        return result;
    } else {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }
}

// linear interpolation via float: 0.0=begin, 1.0=end
template <typename X>
[[nodiscard]] constexpr decltype(X{} * float{} + X{} * float{}) Lerp(const X& begin, const X& end,
                                                                     const float t) {
    if constexpr (detail::is_vectorizable<X>::value) {
#if defined(HAVE_SSE2)
        // For Vec2
        if constexpr (std::is_same_v<X, Vec2<float>>) {
            __m128 vbegin = _mm_setr_ps(begin.x, begin.y, 0.0f, 0.0f);
            __m128 vend = _mm_setr_ps(end.x, end.y, 0.0f, 0.0f);
            __m128 vt = _mm_set1_ps(t);
            __m128 invt = _mm_sub_ps(_mm_set1_ps(1.0f), vt);
            __m128 result = _mm_add_ps(_mm_mul_ps(vbegin, invt), _mm_mul_ps(vend, vt));
            return Vec2<float>(
                _mm_cvtss_f32(result),
                _mm_cvtss_f32(_mm_shuffle_ps(result, result, _MM_SHUFFLE(1, 1, 1, 1))));
        }
        // For Vec3
        else if constexpr (std::is_same_v<X, Vec3<float>>) {
            __m128 vbegin = _mm_load_ps(&begin.x);
            __m128 vend = _mm_load_ps(&end.x);
            __m128 vt = _mm_set1_ps(t);
            __m128 invt = _mm_sub_ps(_mm_set1_ps(1.0f), vt);
            __m128 result = _mm_add_ps(_mm_mul_ps(vbegin, invt), _mm_mul_ps(vend, vt));
            Vec3<float> ret;
            _mm_store_ps(&ret.x, result);
            return ret;
        }
        // For Vec4
        else if constexpr (std::is_same_v<X, Vec4<float>>) {
            __m128 invt = _mm_sub_ps(_mm_set1_ps(1.0f), _mm_set1_ps(t));
            __m128 result =
                _mm_add_ps(_mm_mul_ps(begin.simd, invt), _mm_mul_ps(end.simd, _mm_set1_ps(t)));
            Vec4<float> ret;
            ret.simd = result;
            return ret;
        }
#elif defined(HAVE_NEON)
        // For Vec2
        if constexpr (std::is_same_v<X, Vec2<float>>) {
            float32x2_t vbegin = {begin.x, begin.y};
            float32x2_t vend = {end.x, end.y};
            float32x2_t vt = vdup_n_f32(t);
            float32x2_t invt = vdup_n_f32(1.0f - t);
            float32x2_t result = vadd_f32(vmul_f32(vbegin, invt), vmul_f32(vend, vt));
            return Vec2<float>(vget_lane_f32(result, 0), vget_lane_f32(result, 1));
        }
        // For Vec3
        else if constexpr (std::is_same_v<X, Vec3<float>>) {
            float32x4_t vbegin = vld1q_f32(&begin.x);
            float32x4_t vend = vld1q_f32(&end.x);
            float32x4_t vt = vdupq_n_f32(t);
            float32x4_t invt = vdupq_n_f32(1.0f - t);
            float32x4_t result = vaddq_f32(vmulq_f32(vbegin, invt), vmulq_f32(vend, vt));
            Vec3<float> ret;
            vst1q_f32(&ret.x, result);
            return ret;
        }
        // For Vec4
        else if constexpr (std::is_same_v<X, Vec4<float>>) {
            float32x4_t invt = vdupq_n_f32(1.0f - t);
            float32x4_t vt = vdupq_n_f32(t);
            float32x4_t result = vaddq_f32(vmulq_f32(begin.simd, invt), vmulq_f32(end.simd, vt));
            Vec4<float> ret;
            ret.simd = result;
            return ret;
        }
#endif
    }
    // Fallback for non-vectorizable types
    return begin * (1.f - t) + end * t;
}

// linear interpolation via int: 0=begin, base=end
template <typename X, int base>
[[nodiscard]] constexpr decltype((X{} * int{} + X{} * int{}) / base) LerpInt(const X& begin,
                                                                             const X& end,
                                                                             const int t) {
    if constexpr (detail::is_vectorizable<X>::value) {
#if defined(HAVE_SSE2)
        // For Vec2
        if constexpr (std::is_same_v<X, Vec2<float>>) {
            __m128 vbegin = _mm_setr_ps(begin.x, begin.y, 0.0f, 0.0f);
            __m128 vend = _mm_setr_ps(end.x, end.y, 0.0f, 0.0f);
            __m128 vt = _mm_set1_ps(static_cast<float>(t));
            __m128 vbase_minus_t = _mm_set1_ps(static_cast<float>(base - t));
            __m128 vbase = _mm_set1_ps(static_cast<float>(base));
            __m128 result = _mm_div_ps(
                _mm_add_ps(_mm_mul_ps(vbegin, vbase_minus_t), _mm_mul_ps(vend, vt)), vbase);
            return Vec2<float>(
                _mm_cvtss_f32(result),
                _mm_cvtss_f32(_mm_shuffle_ps(result, result, _MM_SHUFFLE(1, 1, 1, 1))));
        }
        // For Vec3
        else if constexpr (std::is_same_v<X, Vec3<float>>) {
            __m128 vbegin = _mm_load_ps(&begin.x);
            __m128 vend = _mm_load_ps(&end.x);
            __m128 vt = _mm_set1_ps(static_cast<float>(t));
            __m128 vbase_minus_t = _mm_set1_ps(static_cast<float>(base - t));
            __m128 vbase = _mm_set1_ps(static_cast<float>(base));
            __m128 result = _mm_div_ps(
                _mm_add_ps(_mm_mul_ps(vbegin, vbase_minus_t), _mm_mul_ps(vend, vt)), vbase);
            Vec3<float> ret;
            _mm_store_ps(&ret.x, result);
            return ret;
        }
        // For Vec4
        else if constexpr (std::is_same_v<X, Vec4<float>>) {
            __m128 vt = _mm_set1_ps(static_cast<float>(t));
            __m128 vbase_minus_t = _mm_set1_ps(static_cast<float>(base - t));
            __m128 vbase = _mm_set1_ps(static_cast<float>(base));
            __m128 result = _mm_div_ps(
                _mm_add_ps(_mm_mul_ps(begin.simd, vbase_minus_t), _mm_mul_ps(end.simd, vt)), vbase);
            Vec4<float> ret;
            ret.simd = result;
            return ret;
        }
#elif defined(HAVE_NEON)
        // For Vec2
        if constexpr (std::is_same_v<X, Vec2<float>>) {
            float32x2_t vbegin = {begin.x, begin.y};
            float32x2_t vend = {end.x, end.y};
            float32x2_t vt = vdup_n_f32(static_cast<float>(t));
            float32x2_t vbase_minus_t = vdup_n_f32(static_cast<float>(base - t));
            float32x2_t vbase = vdup_n_f32(static_cast<float>(base));
            float32x2_t result =
                vdiv_f32(vadd_f32(vmul_f32(vbegin, vbase_minus_t), vmul_f32(vend, vt)), vbase);
            return Vec2<float>(vget_lane_f32(result, 0), vget_lane_f32(result, 1));
        }
        // For Vec3
        else if constexpr (std::is_same_v<X, Vec3<float>>) {
            float32x4_t vbegin = vld1q_f32(&begin.x);
            float32x4_t vend = vld1q_f32(&end.x);
            float32x4_t vt = vdupq_n_f32(static_cast<float>(t));
            float32x4_t vbase_minus_t = vdupq_n_f32(static_cast<float>(base - t));
            float32x4_t vbase = vdupq_n_f32(static_cast<float>(base));
            float32x4_t result =
                vdivq_f32(vaddq_f32(vmulq_f32(vbegin, vbase_minus_t), vmulq_f32(vend, vt)), vbase);
            Vec3<float> ret;
            vst1q_f32(&ret.x, result);
            return ret;
        }
        // For Vec4
        else if constexpr (std::is_same_v<X, Vec4<float>>) {
            float32x4_t vt = vdupq_n_f32(static_cast<float>(t));
            float32x4_t vbase_minus_t = vdupq_n_f32(static_cast<float>(base - t));
            float32x4_t vbase = vdupq_n_f32(static_cast<float>(base));
            float32x4_t result = vdivq_f32(
                vaddq_f32(vmulq_f32(begin.simd, vbase_minus_t), vmulq_f32(end.simd, vt)), vbase);
            Vec4<float> ret;
            ret.simd = result;
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
