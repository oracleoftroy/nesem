#pragma once

#include <array>
#include <span>

#include <cm/math.hpp>

namespace cm
{
	struct Vec3
	{
		float x = 0;
		float y = 0;
		float z = 0;

		constexpr auto operator<=>(const Vec3 &other) const noexcept = default;
	};

	struct Vec4
	{
		float x = 0;
		float y = 0;
		float z = 0;
		float w = 0;

		constexpr auto operator<=>(const Vec4 &other) const noexcept = default;
	};

	constexpr Vec3 vec3(Vec4 v) noexcept
	{
		return Vec3{v.x, v.y, v.z};
	}

	constexpr Vec4 vec4(Vec3 v, float w = 1.0f) noexcept
	{
		return Vec4{v.x, v.y, v.z, w};
	}

	struct Mat3
	{
		constexpr static size_t N = 3;
		float m[N][N] = {};

		constexpr auto operator<=>(const Mat3 &other) const noexcept = default;

		constexpr auto operator[](size_t index) noexcept
		{
			return std::span(m[index]);
		}

		constexpr auto operator[](size_t index) const noexcept
		{
			return std::span(m[index]);
		}

		static constexpr Mat3 identity() noexcept
		{
			Mat3 m{};
			m[0][0] = 1;
			m[1][1] = 1;
			m[2][2] = 1;

			return m;
		}

		static /*constexpr*/ Mat3 rotate_x(float theta) noexcept
		{
			Mat3 m;
			m[0][0] = 1.0f;
			m[1][1] = std::cos(theta);
			m[1][2] = std::sin(theta);
			m[2][1] = -std::sin(theta);
			m[2][2] = std::cos(theta);

			return m;
		}

		static /*constexpr*/ Mat3 rotate_y(float theta) noexcept
		{
			Mat3 m;
			m[0][0] = std::cos(theta);
			m[1][1] = 1.0f;
			m[2][0] = std::sin(theta);
			m[0][2] = -std::sin(theta);
			m[2][2] = std::cos(theta);

			return m;
		}

		static /*constexpr*/ Mat3 rotate_z(float theta) noexcept
		{
			Mat3 m;
			m[0][0] = std::cos(theta);
			m[0][1] = std::sin(theta);
			m[1][0] = -std::sin(theta);
			m[1][1] = std::cos(theta);
			m[2][2] = 1.0f;

			return m;
		}
	};

	struct Mat4
	{
		constexpr static size_t N = 4;
		float m[N][N] = {};

		constexpr auto operator<=>(const Mat4 &other) const noexcept = default;

		constexpr auto operator[](size_t index) noexcept
		{
			return std::span(m[index]);
		}

		constexpr auto operator[](size_t index) const noexcept
		{
			return std::span(m[index]);
		}

		static constexpr Mat4 identity() noexcept;
		static /*constexpr*/ Mat4 projection(float near, float far, float fov, float aspect_ratio) noexcept;
		static /*constexpr*/ Mat4 projection(float near, float far, float fov, float width, float height) noexcept;
		static /*constexpr*/ Mat4 projection(float near, float far, float fov, Sizef size) noexcept;
		static /*constexpr*/ Mat4 rotate_x(float theta) noexcept;
		static /*constexpr*/ Mat4 rotate_y(float theta) noexcept;
		static /*constexpr*/ Mat4 rotate_z(float theta) noexcept;
		static constexpr Mat4 translate(Vec3 pos) noexcept;

		// the object at `position` will be rotated to face `at` with the orientation specified in `up`
		static /*constexpr*/ Mat4 point_at(Vec3 position, Vec3 at, Vec3 up) noexcept;

		// the camera at `eye` will be rotated to face `at` with the orientation specified in `up`
		static /*constexpr*/ Mat4 look_at(Vec3 eye, Vec3 at, Vec3 up) noexcept;
	};

	// unary + does nothing
	constexpr Vec3 operator+(Vec3 v) noexcept
	{
		return v;
	}

	constexpr Vec3 operator-(Vec3 v) noexcept
	{
		v.x = -v.x;
		v.y = -v.y;
		v.z = -v.z;

		return v;
	}

	constexpr Vec3 &operator+=(Vec3 &v1, Vec3 v2) noexcept
	{
		v1.x += v2.x;
		v1.y += v2.y;
		v1.z += v2.z;

		return v1;
	}

	constexpr Vec3 operator+(Vec3 v1, Vec3 v2) noexcept
	{
		return v1 += v2;
	}

	constexpr Vec3 &operator+=(Vec3 &v, float scalar) noexcept
	{
		v.x += scalar;
		v.y += scalar;
		v.z += scalar;

		return v;
	}

	constexpr Vec3 operator+(Vec3 v, float scalar) noexcept
	{
		return v += scalar;
	}

	constexpr Vec3 operator+(float scalar, Vec3 v) noexcept
	{
		v.x = scalar + v.x;
		v.y = scalar + v.y;
		v.z = scalar + v.z;

		return v;
	}

	constexpr Vec3 &operator-=(Vec3 &v1, Vec3 v2) noexcept
	{
		v1.x -= v2.x;
		v1.y -= v2.y;
		v1.z -= v2.z;

		return v1;
	}

	constexpr Vec3 operator-(Vec3 v1, Vec3 v2) noexcept
	{
		return v1 -= v2;
	}

	constexpr Vec3 &operator-=(Vec3 &v, float scalar) noexcept
	{
		v.x -= scalar;
		v.y -= scalar;
		v.z -= scalar;

		return v;
	}

	constexpr Vec3 operator-(Vec3 v, float scalar) noexcept
	{
		return v -= scalar;
	}

	constexpr Vec3 operator-(float scalar, Vec3 v) noexcept
	{
		v.x = scalar - v.x;
		v.y = scalar - v.y;
		v.z = scalar - v.z;

		return v;
	}

	constexpr Vec3 &operator*=(Vec3 &v1, Vec3 v2) noexcept
	{
		v1.x *= v2.x;
		v1.y *= v2.y;
		v1.z *= v2.z;

		return v1;
	}

	constexpr Vec3 operator*(Vec3 v1, Vec3 v2) noexcept
	{
		return v1 *= v2;
	}

	constexpr Vec3 &operator*=(Vec3 &v, float scalar) noexcept
	{
		v.x *= scalar;
		v.y *= scalar;
		v.z *= scalar;

		return v;
	}

	constexpr Vec3 operator*(Vec3 v, float scalar) noexcept
	{
		return v *= scalar;
	}

	constexpr Vec3 operator*(float scalar, Vec3 v) noexcept
	{
		v.x = scalar * v.x;
		v.y = scalar * v.y;
		v.z = scalar * v.z;

		return v;
	}

	constexpr Vec3 &operator/=(Vec3 &v1, Vec3 v2) noexcept
	{
		v1.x /= v2.x;
		v1.y /= v2.y;
		v1.z /= v2.z;

		return v1;
	}

	constexpr Vec3 operator/(Vec3 v1, Vec3 v2) noexcept
	{
		return v1 /= v2;
	}

	constexpr Vec3 &operator/=(Vec3 &v, float scalar) noexcept
	{
		v.x /= scalar;
		v.y /= scalar;
		v.z /= scalar;

		return v;
	}

	constexpr Vec3 operator/(Vec3 v, float scalar) noexcept
	{
		return v /= scalar;
	}

	constexpr Vec3 operator/(float scalar, Vec3 v) noexcept
	{
		v.x = scalar / v.x;
		v.y = scalar / v.y;
		v.z = scalar / v.z;

		return v;
	}

	// unary + does nothing
	constexpr Vec4 operator+(Vec4 v) noexcept
	{
		return v;
	}

	constexpr Vec4 operator-(Vec4 v) noexcept
	{
		v.x = -v.x;
		v.y = -v.y;
		v.z = -v.z;

		return v;
	}

	constexpr Vec4 &operator+=(Vec4 &v1, Vec4 v2) noexcept
	{
		v1.x += v2.x;
		v1.y += v2.y;
		v1.z += v2.z;
		v1.w += v2.w;

		return v1;
	}

	constexpr Vec4 operator+(Vec4 v1, Vec4 v2) noexcept
	{
		return v1 += v2;
	}

	constexpr Vec4 &operator+=(Vec4 &v, float scalar) noexcept
	{
		v.x += scalar;
		v.y += scalar;
		v.z += scalar;
		v.w += scalar;

		return v;
	}

	constexpr Vec4 operator+(Vec4 v, float scalar) noexcept
	{
		return v += scalar;
	}

	constexpr Vec4 operator+(float scalar, Vec4 v) noexcept
	{
		v.x = scalar + v.x;
		v.y = scalar + v.y;
		v.z = scalar + v.z;
		v.w = scalar + v.w;

		return v;
	}

	constexpr Vec4 &operator-=(Vec4 &v1, Vec4 v2) noexcept
	{
		v1.x -= v2.x;
		v1.y -= v2.y;
		v1.z -= v2.z;
		v1.w -= v2.w;

		return v1;
	}

	constexpr Vec4 operator-(Vec4 v1, Vec4 v2) noexcept
	{
		return v1 -= v2;
	}

	constexpr Vec4 &operator-=(Vec4 &v, float scalar) noexcept
	{
		v.x -= scalar;
		v.y -= scalar;
		v.z -= scalar;
		v.w -= scalar;

		return v;
	}

	constexpr Vec4 operator-(Vec4 v, float scalar) noexcept
	{
		return v -= scalar;
	}

	constexpr Vec4 operator-(float scalar, Vec4 v) noexcept
	{
		v.x = scalar - v.x;
		v.y = scalar - v.y;
		v.z = scalar - v.z;
		v.w = scalar - v.w;

		return v;
	}

	constexpr Vec4 &operator*=(Vec4 &v1, Vec4 v2) noexcept
	{
		v1.x *= v2.x;
		v1.y *= v2.y;
		v1.z *= v2.z;
		v1.w *= v2.w;

		return v1;
	}

	constexpr Vec4 operator*(Vec4 v1, Vec4 v2) noexcept
	{
		return v1 *= v2;
	}

	constexpr Vec4 &operator*=(Vec4 &v, float scalar) noexcept
	{
		v.x *= scalar;
		v.y *= scalar;
		v.z *= scalar;
		v.w *= scalar;

		return v;
	}

	constexpr Vec4 operator*(Vec4 v, float scalar) noexcept
	{
		return v *= scalar;
	}

	constexpr Vec4 operator*(float scalar, Vec4 v) noexcept
	{
		v.x = scalar * v.x;
		v.y = scalar * v.y;
		v.z = scalar * v.z;
		v.w = scalar * v.w;

		return v;
	}

	constexpr Vec4 &operator/=(Vec4 &v1, Vec4 v2) noexcept
	{
		v1.x /= v2.x;
		v1.y /= v2.y;
		v1.z /= v2.z;
		v1.w /= v2.w;

		return v1;
	}

	constexpr Vec4 operator/(Vec4 v1, Vec4 v2) noexcept
	{
		return v1 /= v2;
	}

	constexpr Vec4 &operator/=(Vec4 &v, float scalar) noexcept
	{
		v.x /= scalar;
		v.y /= scalar;
		v.z /= scalar;
		v.w /= scalar;

		return v;
	}

	constexpr Vec4 operator/(Vec4 v, float scalar) noexcept
	{
		return v /= scalar;
	}

	constexpr Vec4 operator/(float scalar, Vec4 v) noexcept
	{
		v.x = scalar / v.x;
		v.y = scalar / v.y;
		v.z = scalar / v.z;
		v.w = scalar / v.w;

		return v;
	}

	constexpr float dot(Vec3 v1, Vec3 v2) noexcept
	{
		v1 *= v2;
		return v1.x + v1.y + v1.z;
	}

	constexpr float length_sq(Vec3 v) noexcept
	{
		return dot(v, v);
	}

	/*constexpr*/ float length(Vec3 v) noexcept
	{
		return std::hypot(v.x, v.y, v.z);
	}

	/*constexpr*/ Vec3 normalize(Vec3 v) noexcept
	{
		return v / length(v);
	}

	constexpr Vec3 cross(Vec3 v1, Vec3 v2) noexcept
	{
		return {
			.x = v1.y * v2.z - v1.z * v2.y,
			.y = v1.z * v2.x - v1.x * v2.z,
			.z = v1.x * v2.y - v1.y * v2.x,
		};
	}

	constexpr Vec3 &operator*=(Vec3 &v, const Mat3 &m) noexcept
	{
		v = Vec3{
			.x = v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0],
			.y = v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1],
			.z = v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2],
		};

		return v;
	}

	constexpr Vec3 operator*(Vec3 v, const Mat3 &m) noexcept
	{
		v *= m;
		return v;
	}

	constexpr Vec4 &operator*=(Vec4 &v, const Mat4 &m) noexcept
	{
		v = Vec4{
			.x = v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + v.w * m[3][0],
			.y = v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + v.w * m[3][1],
			.z = v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + v.w * m[3][2],
			.w = v.x * m[0][3] + v.y * m[1][3] + v.z * m[2][3] + v.w * m[3][3],
		};

		return v;
	}

	constexpr Vec4 operator*(Vec4 v, const Mat4 &m) noexcept
	{
		v *= m;
		return v;
	}

	constexpr Mat3 operator*(float scalar, Mat3 m) noexcept
	{
		for (int row = 0; row < 3; ++row)
		{
			m[row][0] = scalar * m[row][0];
			m[row][1] = scalar * m[row][1];
			m[row][2] = scalar * m[row][2];
		}

		return m;
	}

	constexpr Mat3 &operator*=(Mat3 &m, float scalar) noexcept
	{
		for (int row = 0; row < 3; ++row)
		{
			m[row][0] *= scalar;
			m[row][1] *= scalar;
			m[row][2] *= scalar;
		}

		return m;
	}

	constexpr Mat3 operator*(Mat3 m, float scalar) noexcept
	{
		return m *= scalar;
	}

	constexpr Mat3 &operator*=(Mat3 &m1, const Mat3 &m2) noexcept
	{
		Mat3 result;
		for (int col = 0; col < 3; ++col)
		{
			for (int row = 0; row < 3; ++row)
			{
				result[row][col] =
					m1[row][0] * m2[0][col] +
					m1[row][1] * m2[1][col] +
					m1[row][2] * m2[2][col];
			}
		}

		return m1 = result;
	}

	constexpr Mat3 operator*(Mat3 m1, const Mat3 &m2) noexcept
	{
		return m1 *= m2;
	}

	constexpr Mat4 operator*(float scalar, Mat4 m) noexcept
	{
		for (int row = 0; row < 4; ++row)
		{
			m[row][0] = scalar * m[row][0];
			m[row][1] = scalar * m[row][1];
			m[row][2] = scalar * m[row][2];
			m[row][3] = scalar * m[row][3];
		}

		return m;
	}

	constexpr Mat4 &operator*=(Mat4 &m, float scalar) noexcept
	{
		for (int row = 0; row < 4; ++row)
		{
			m[row][0] *= scalar;
			m[row][1] *= scalar;
			m[row][2] *= scalar;
			m[row][3] *= scalar;
		}

		return m;
	}

	constexpr Mat4 operator*(Mat4 m, float scalar) noexcept
	{
		return m *= scalar;
	}

	constexpr Mat4 &operator*=(Mat4 &m1, const Mat4 &m2) noexcept
	{
		Mat4 result;
		for (int col = 0; col < 4; ++col)
		{
			for (int row = 0; row < 4; ++row)
			{
				result[row][col] =
					m1[row][0] * m2[0][col] +
					m1[row][1] * m2[1][col] +
					m1[row][2] * m2[2][col] +
					m1[row][3] * m2[3][col];
			}
		}

		return m1 = result;
	}

	constexpr Mat4 operator*(Mat4 m1, const Mat4 &m2) noexcept
	{
		return m1 *= m2;
	}

	constexpr Mat4 Mat4::identity() noexcept
	{
		Mat4 m{};
		m[0][0] = 1;
		m[1][1] = 1;
		m[2][2] = 1;
		m[3][3] = 1;

		return m;
	}

	inline /*constexpr*/ Mat4 Mat4::projection(float near, float far, float fov, float aspect_ratio) noexcept
	{
		const auto half_fov = fov * 0.5f;
		const auto f = std::cos(half_fov) / std::sin(half_fov);
		const auto q = far / (far - near);

		Mat4 m;
		m[0][0] = aspect_ratio * f;
		m[1][1] = f;
		m[2][2] = q;
		m[3][2] = -near * q;
		m[2][3] = 1.0f;
		m[3][3] = 0.0f;

		return m;
	}

	inline /*constexpr*/ Mat4 Mat4::projection(float near, float far, float fov, float width, float height) noexcept
	{
		return projection(near, far, fov, height / width);
	}

	inline /*constexpr*/ Mat4 Mat4::projection(float near, float far, float fov, Sizef size) noexcept
	{
		return projection(near, far, fov, size.h / size.w);
	}

	inline /*constexpr*/ Mat4 Mat4::rotate_x(float theta) noexcept
	{
		Mat4 m;
		m[0][0] = 1.0f;
		m[1][1] = std::cos(theta);
		m[1][2] = std::sin(theta);
		m[2][1] = -std::sin(theta);
		m[2][2] = std::cos(theta);
		m[3][3] = 1.0f;

		return m;
	}

	inline /*constexpr*/ Mat4 Mat4::rotate_y(float theta) noexcept
	{
		Mat4 m;
		m[0][0] = std::cos(theta);
		m[1][1] = 1.0f;
		m[2][0] = std::sin(theta);
		m[0][2] = -std::sin(theta);
		m[2][2] = std::cos(theta);
		m[3][3] = 1.0f;

		return m;
	}

	inline /*constexpr*/ Mat4 Mat4::rotate_z(float theta) noexcept
	{
		Mat4 m;
		m[0][0] = std::cos(theta);
		m[0][1] = std::sin(theta);
		m[1][0] = -std::sin(theta);
		m[1][1] = std::cos(theta);
		m[2][2] = 1.0f;
		m[3][3] = 1.0f;

		return m;
	}

	inline constexpr Mat4 Mat4::translate(Vec3 pos) noexcept
	{
		auto m = Mat4::identity();

		m[3][0] = pos.x;
		m[3][1] = pos.y;
		m[3][2] = pos.z;

		return m;
	}

	// the object at `position` will be rotated to face `at` with the orientation specified in `up`
	inline /*constexpr*/ Mat4 Mat4::point_at(Vec3 position, Vec3 at, Vec3 up) noexcept
	{
		auto dir = normalize(at - position);
		auto new_up = normalize(up - dir * dot(up, dir));
		auto new_right = cross(up, dir);

		Mat4 m;
		m[0][0] = new_right.x;
		m[0][1] = new_right.y;
		m[0][2] = new_right.z;
		m[1][0] = new_up.x;
		m[1][1] = new_up.y;
		m[1][2] = new_up.z;
		m[2][0] = dir.x;
		m[2][1] = dir.y;
		m[2][2] = dir.z;
		m[3][0] = position.x;
		m[3][1] = position.y;
		m[3][2] = position.z;
		m[3][3] = 1;

		return m;
	}

	// the camera at `eye` will be rotated to face `at` with the orientation specified in `up`
	inline /*constexpr*/ Mat4 Mat4::look_at(Vec3 eye, Vec3 at, Vec3 up) noexcept
	{
		auto zaxis = normalize(at - eye);
		auto xaxis = normalize(cross(up, zaxis));
		auto yaxis = cross(zaxis, xaxis);

		Mat4 m{};
		m[0][0] = xaxis.x;
		m[0][1] = yaxis.x;
		m[0][2] = zaxis.x;
		m[1][0] = xaxis.y;
		m[1][1] = yaxis.y;
		m[1][2] = zaxis.y;
		m[2][0] = xaxis.z;
		m[2][1] = yaxis.z;
		m[2][2] = zaxis.z;
		m[3][0] = -dot(xaxis, eye);
		m[3][1] = -dot(yaxis, eye);
		m[3][2] = -dot(zaxis, eye);
		m[3][3] = 1;

		return m;
	}

	namespace
	{
		constexpr void math3d_constexpr_tests() noexcept
		{
			constexpr auto v1 = Vec3{1, 2, 3};
			constexpr auto v2 = Vec3{2, 4, 6};
			{
				constexpr auto a1 = v1 + v2;
				constexpr auto a2 = v1 - v2;
				constexpr auto a3 = v1 * v2;
				constexpr auto a4 = v1 / v2;

				static_assert(a1 == Vec3{3, 6, 9});
				static_assert(a2 == Vec3{-1, -2, -3});
				static_assert(a3 == Vec3{2, 8, 18});
				static_assert(a4 == Vec3{.5f, .5f, .5f});
			}
			{
				constexpr auto a1 = v1 + 3.14f;
				constexpr auto a2 = v1 - 3.14f;
				constexpr auto a3 = v1 * 3.14f;
				constexpr auto a4 = v1 / 3.14f;

				static_assert(a1 == Vec3{1 + 3.14f, 2 + 3.14f, 3 + 3.14f});
				static_assert(a2 == Vec3{1 - 3.14f, 2 - 3.14f, 3 - 3.14f});
				static_assert(a3 == Vec3{1 * 3.14f, 2 * 3.14f, 3 * 3.14f});
				static_assert(a4 == Vec3{1 / 3.14f, 2 / 3.14f, 3 / 3.14f});
			}
			{
				constexpr auto a1 = 3.14f + v1;
				constexpr auto a2 = 3.14f - v1;
				constexpr auto a3 = 3.14f * v1;
				constexpr auto a4 = 3.14f / v1;

				static_assert(a1 == Vec3{3.14f + 1, 3.14f + 2, 3.14f + 3});
				static_assert(a2 == Vec3{3.14f - 1, 3.14f - 2, 3.14f - 3});
				static_assert(a3 == Vec3{3.14f * 1, 3.14f * 2, 3.14f * 3});
				static_assert(a4 == Vec3{3.14f / 1, 3.14f / 2, 3.14f / 3});
			}

			constexpr auto m = Mat4::identity();

			static_assert(m[0][0] == 1);
			static_assert(m[0][1] == 0);
			static_assert(m[1][1] == 1);
			static_assert(m[2][2] == 1);
			static_assert(m[3][3] == 1);
		}
	}
}
