//
// Created by Putcho on 24.04.2025.
//

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <fmt/format.h>
#include <chrono>


namespace ufox::numerics {



    struct Vector2 {
        float x, y;
    };

    struct Vector3 {
        // Type definitions and constants
        using value_type = float;
        static constexpr value_type kEpsilon = std::numeric_limits<value_type>::epsilon();

        // Component values
        value_type x = 0.0f;
        value_type y = 0.0f;
        value_type z = 0.0f;

        // Constructors
        constexpr Vector3() noexcept = default;
        explicit constexpr Vector3(value_type value) noexcept : x{value}, y{value}, z{value} {}
        explicit constexpr Vector3(value_type xValue, value_type yValue, value_type zValue) noexcept
            : x{xValue}, y{yValue}, z{zValue} {}

        // Predefined vectors
        [[nodiscard]] static constexpr Vector3 zero() noexcept { return Vector3(0.0f); }
        [[nodiscard]] static constexpr Vector3 one() noexcept { return Vector3(1.0f); }

        // Basic arithmetic operators
        [[nodiscard]] constexpr Vector3 operator+(const Vector3& other) const noexcept {
            return Vector3(x + other.x, y + other.y, z + other.z);
        }
        [[nodiscard]] constexpr Vector3 operator-(const Vector3& other) const noexcept {
            return Vector3(x - other.x, y - other.y, z - other.z);
        }
        [[nodiscard]] constexpr Vector3 operator*(value_type scalar) const noexcept {
            return Vector3(x * scalar, y * scalar, z * scalar);
        }
        friend constexpr Vector3 operator*(value_type scalar, const Vector3& vec) noexcept {
            return vec * scalar;
        }
        [[nodiscard]] constexpr Vector3 operator/(value_type scalar) const noexcept {
            return Vector3(x / scalar, y / scalar, z / scalar);
        }

        // Vector operations
        [[nodiscard]] static constexpr value_type dot(const Vector3& a, const Vector3& b) noexcept {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }
        [[nodiscard]] static constexpr Vector3 cross(const Vector3& a, const Vector3& b) noexcept {
            return Vector3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }

        // Component-wise operations
        /// Clamps each component of a vector to the range [min, max].
        /// @param v Vector to clamp.
        /// @param min Minimum bounds for each component.
        /// @param max Maximum bounds for each component.
        /// @return A new vector with clamped components.
        [[nodiscard]] static constexpr Vector3 clamp(const Vector3& v, const Vector3& min, const Vector3& max) noexcept {
            return Vector3(
                std::clamp(v.x, min.x, max.x),
                std::clamp(v.y, min.y, max.y),
                std::clamp(v.z, min.z, max.z)
            );
        }

        /// Clamps each component of this vector to the range [min, max].
        /// @param min Minimum bounds for each component.
        /// @param max Maximum bounds for each component.
        /// @return A new vector with clamped components.
        [[nodiscard]] constexpr Vector3 clamp(const Vector3& min, const Vector3& max) const noexcept {
            return clamp(*this, min, max);
        }

        [[nodiscard]] static constexpr Vector3 min(const Vector3& a, const Vector3& b) noexcept {
            return Vector3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
        }
        [[nodiscard]] static constexpr Vector3 max(const Vector3& a, const Vector3& b) noexcept {
            return Vector3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
        }
        [[nodiscard]] static constexpr Vector3 abs(const Vector3& a) noexcept {
            return Vector3(std::abs(a.x), std::abs(a.y), std::abs(a.z));
        }
        [[nodiscard]] constexpr Vector3 abs() const noexcept { return abs(*this); }

        // Geometric operations
        [[nodiscard]] constexpr value_type lengthSquared() const noexcept { return dot(*this, *this); }
        [[nodiscard]] value_type length() const noexcept { return std::sqrt(lengthSquared()); }
        [[nodiscard]] static value_type distance(const Vector3& a, const Vector3& b) noexcept {
            return (a - b).length();
        }
        [[nodiscard]] static value_type distanceSquared(const Vector3& a, const Vector3& b) noexcept {
            return (a - b).lengthSquared();
        }
        [[nodiscard]] static constexpr Vector3 normalize(const Vector3& v) noexcept {
            const value_type length = v.length();
            return length < kEpsilon ? zero() : v / length;
        }
        [[nodiscard]] constexpr Vector3 normalize() const noexcept { return normalize(*this); }

        // Interpolation and reflection
        [[nodiscard]] static constexpr Vector3 lerp(const Vector3& a, const Vector3& b, value_type t) noexcept {
            return a + (b - a) * t;
        }
        [[nodiscard]] constexpr Vector3 lerp(const Vector3& b, value_type t) const noexcept {
            return lerp(*this, b, t);
        }
        [[nodiscard]] static constexpr Vector3 reflect(const Vector3& vec, const Vector3& normal) noexcept {
            return vec - normal * (2.0f * dot(vec, normal));
        }
        [[nodiscard]] constexpr Vector3 reflect(const Vector3& normal) const noexcept {
            return reflect(*this, normal);
        }

        // String conversion
        [[nodiscard]] std::string toString() const {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << "(" << x << ", " << y << ", " << z << ")";
            return ss.str();
        }
    };



    struct Vector4 {
        float x, y, z, w;
    };



    struct Matrix4x4 {
        float m11;
        float m12;
        float m13;
        float m14;
        float m21;
        float m22;
        float m23;
        float m24;
        float m31;
        float m32;
        float m33;
        float m34;
        float m41;
        float m42;
        float m43;
        float m44;

        static Matrix4x4 CreateLookAt(Vector3 cameraPosition, Vector3 cameraTarget, Vector3 cameraUpVector) {


            Vector3 zAxis = Vector3::normalize(cameraPosition - cameraTarget);
            Vector3 xAxis = Vector3::normalize(Vector3::cross(cameraUpVector, zAxis));
            Vector3 yAxis = Vector3::cross(zAxis, xAxis);

            Matrix4x4 result{};

            result.m11 = xAxis.x;
            result.m12 = yAxis.x;
            result.m13 = zAxis.x;
            result.m14 = 0.0f;
            result.m21 = xAxis.y;
            result.m22 = yAxis.y;
            result.m23 = zAxis.y;
            result.m24 = 0.0f;
            result.m31 = xAxis.z;
            result.m32 = yAxis.z;
            result.m33 = zAxis.z;
            result.m34 = 0.0f;
            result.m41 = -Vector3::dot(xAxis, cameraPosition);
            result.m42 = -Vector3::dot(yAxis, cameraPosition);
            result.m43 = -Vector3::dot(zAxis, cameraPosition);
            result.m44 = 1.0f;

            return result;
        }
    };

}

// Specialize fmt::formatter for Vector3
template <>
struct fmt::formatter<ufox::numerics::Vector3> {
    // Parse format specifications (e.g., "{:.2f}"), if needed
    static constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        // For simplicity, ignore format specs or assume default
        return ctx.begin();
    }

    // Format the Vector3 object
    template <typename FormatContext>
    auto format(const ufox::numerics::Vector3& v, FormatContext& ctx) const -> decltype(ctx.out()) {
        // Use ToString() for consistent formatting
        return fmt::format_to(ctx.out(), "{}", v.toString());
    }
};