#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace cm
{
	constexpr auto abs(auto value) noexcept
	{
		if (std::is_constant_evaluated())
		{
			if (value < 0)
				return -value;
			return value;
		}
		return std::abs(value);
	}

	// the 'sign' of the number, 1 for positive, -1 for negative, and 0 for 0 (and -0.0 in the case of floating point)
	template <typename T>
	constexpr int signum(T value) noexcept
	{
		if constexpr (std::unsigned_integral<T>)
			return T(0) < value;
		else if constexpr (std::floating_point<T> || std::integral<T>)
			return (T(0) < value) - (value < T(0));
	}

	constexpr void t()
	{
		constexpr auto i0 = signum(0);
		constexpr auto i1 = signum(-10);
		constexpr auto i2 = signum(10);

		constexpr auto u0 = signum(0u);
		constexpr auto u1 = signum(10u);

		constexpr auto f0 = signum(0.0);
		constexpr auto f00 = signum(-0.0);
		constexpr auto f1 = signum(-10.0);
		constexpr auto f2 = signum(10.0);

		static_assert(i0 == 0);
		static_assert(i1 == -1);
		static_assert(i2 == 1);

		static_assert(u0 == 0);
		static_assert(u1 == 1);

		static_assert(f0 == 0);
		static_assert(f00 == 0);
		static_assert(f1 == -1);
		static_assert(f2 == 1);
	}

	template <typename T>
	struct Point2
	{
		T x = T{};
		T y = T{};

		constexpr auto operator<=>(const Point2 &other) const noexcept = default;
	};

	struct Color
	{
		uint8_t r = 0;
		uint8_t g = 0;
		uint8_t b = 0;
		uint8_t a = 255;

		constexpr auto operator<=>(const Color &other) const noexcept = default;
	};

	struct Colorf
	{
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		float a = 1.0f;

		constexpr auto operator<=>(const Colorf &other) const noexcept = default;
	};

	struct ColorHSL
	{
		float h = 0.0f;
		float s = 0.0f;
		float l = 0.0f;
		float a = 1.0f;

		constexpr auto operator<=>(const ColorHSL &other) const noexcept = default;
	};

	struct ColorFormat
	{
		uint32_t mask_r = 0x00FF0000;
		uint32_t mask_g = 0x0000FF00;
		uint32_t mask_b = 0x000000FF;
		uint32_t mask_a = 0xFF000000;

		int shift_r = std::countr_zero(mask_r);
		int shift_g = std::countr_zero(mask_g);
		int shift_b = std::countr_zero(mask_b);
		int shift_a = std::countr_zero(mask_a);

		constexpr auto operator<=>(const ColorFormat &other) const noexcept = default;
	};

	template <typename T>
	struct Rect
	{
		T x = T{};
		T y = T{};
		T w = T{};
		T h = T{};

		constexpr auto operator<=>(const Rect &other) const noexcept = default;
	};

	template <typename T>
	struct Circle
	{
		T radius;
		Point2<T> pos;

		constexpr auto operator<=>(const Circle &other) const noexcept = default;
	};

	template <typename T>
	struct Size
	{
		T w = T{};
		T h = T{};

		constexpr auto operator<=>(const Size &other) const noexcept = default;
	};

	template <typename T>
	constexpr Point2<T> &operator+=(Point2<T> &p1, Point2<T> p2) noexcept
	{
		p1.x += p2.x;
		p1.y += p2.y;

		return p1;
	}

	template <typename T>
	constexpr Point2<T> operator+(Point2<T> p1, Point2<T> p2) noexcept
	{
		return p1 += p2;
	}

	template <typename T>
	constexpr Point2<T> &operator+=(Point2<T> &point, T scalar) noexcept
	{
		point.x = T(point.x + scalar);
		point.y = T(point.y + scalar);

		return point;
	}

	template <typename T>
	constexpr Point2<T> operator+(Point2<T> point, T scalar) noexcept
	{
		return point += scalar;
	}

	template <typename T>
	constexpr Point2<T> operator+(T scalar, Point2<T> point) noexcept
	{
		point.x = scalar + point.x;
		point.y = scalar + point.y;

		return point;
	}

	template <typename T>
	constexpr Point2<T> &operator-=(Point2<T> &p1, Point2<T> p2) noexcept
	{
		p1.x -= p2.x;
		p1.y -= p2.y;

		return p1;
	}

	template <typename T>
	constexpr Point2<T> operator-(Point2<T> p1, Point2<T> p2) noexcept
	{
		return p1 -= p2;
	}

	template <typename T>
	constexpr Point2<T> &operator-=(Point2<T> &point, T scalar) noexcept
	{
		point.x = T(point.x - scalar);
		point.y = T(point.y - scalar);

		return point;
	}

	template <typename T>
	constexpr Point2<T> operator-(Point2<T> point, T scalar) noexcept
	{
		return point -= scalar;
	}

	template <typename T>
	constexpr Point2<T> operator-(T scalar, Point2<T> point) noexcept
	{
		point.x = scalar - point.x;
		point.y = scalar - point.y;

		return point;
	}

	template <typename T>
	constexpr Point2<T> &operator*=(Point2<T> &point, Size<T> size) noexcept
	{
		point.x = T(point.x * size.w);
		point.y = T(point.y * size.h);

		return point;
	}

	template <typename T>
	constexpr Point2<T> operator*(Point2<T> point, Size<T> size) noexcept
	{
		return point *= size;
	}

	template <typename T>
	constexpr Point2<T> &operator*=(Point2<T> &point, T scalar) noexcept
	{
		point.x = T(point.x * scalar);
		point.y = T(point.y * scalar);

		return point;
	}

	template <typename T>
	constexpr Point2<T> operator*(Point2<T> point, T scalar) noexcept
	{
		return point *= scalar;
	}

	template <typename T>
	constexpr Point2<T> operator*(T scalar, Point2<T> point) noexcept
	{
		point.x = scalar * point.x;
		point.y = scalar * point.y;

		return point;
	}

	template <typename T>
	constexpr Point2<T> &operator/=(Point2<T> &point, Size<T> size) noexcept
	{
		point.x = T(point.x / size.w);
		point.y = T(point.y / size.h);

		return point;
	}

	template <typename T>
	constexpr Point2<T> operator/(Point2<T> point, Size<T> size) noexcept
	{
		return point /= size;
	}

	template <typename T>
	constexpr Point2<T> &operator/=(Point2<T> &point, T scalar) noexcept
	{
		point.x = T(point.x / scalar);
		point.y = T(point.y / scalar);

		return point;
	}

	template <typename T>
	constexpr Point2<T> operator/(Point2<T> point, T scalar) noexcept
	{
		return point /= scalar;
	}

	template <typename T>
	constexpr Point2<T> operator/(T scalar, Point2<T> point) noexcept
	{
		point.x = scalar / point.x;
		point.y = scalar / point.y;

		return point;
	}

	template <std::floating_point T>
	constexpr Point2<T> curve(Point2<T> p1, Point2<T> c, Point2<T> p2, float t) noexcept
	{
		auto inv_t = 1.0f - t;
		auto t2 = t * t;
		auto inv_t2 = inv_t * inv_t;

		return inv_t2 * p1 + 2 * inv_t * t * c + t2 * p2;
	}

	template <std::integral T>
	constexpr Point2<T> curve(Point2<T> p1, Point2<T> c, Point2<T> p2, float t) noexcept
	{
		return to_integral(curve(to_floating(p1), to_floating(c), to_floating(p2), t));
	}

	// distance squared - use if you don't need the exact distance, like for sorting by distance
	template <typename T>
	constexpr T distance_sq(Point2<T> p1, Point2<T> p2) noexcept
	{
		auto v = p2 - p1;
		return v.x * v.x + v.y * v.y;
	}

	template <std::floating_point T>
	constexpr T distance(Point2<T> p1, Point2<T> p2) noexcept
	{
		auto v = p2 - p1;
		return std::hypot(v.x, v.y);
	}

	template <std::floating_point R = float, std::integral T>
	constexpr R distance(Point2<T> p1, Point2<T> p2) noexcept
	{
		auto v = p2 - p1;
		return R(std::hypot(v.x, v.y));
	}

	constexpr Color to_color(Colorf color) noexcept
	{
		return {
			.r = uint8_t(255.0f * color.r),
			.g = uint8_t(255.0f * color.g),
			.b = uint8_t(255.0f * color.b),
			.a = uint8_t(255.0f * color.a),
		};
	}

	constexpr Colorf to_color(Color color) noexcept
	{
		return {
			.r = float(color.r) / 255.0f,
			.g = float(color.g) / 255.0f,
			.b = float(color.b) / 255.0f,
			.a = float(color.a) / 255.0f,
		};
	}

	constexpr Color to_grayscale(Color color) noexcept
	{
		auto gray = uint8_t(color.r * 0.2162f + color.g * 0.7152f + color.b * 0.0722f);

		return {
			.r = gray,
			.g = gray,
			.b = gray,
			.a = color.a,
		};
	}

	constexpr ColorHSL to_hsl(Colorf c) noexcept
	{
		auto [min, max] = std::minmax({c.r, c.g, c.b});
		auto delta = max - min;
		auto result = ColorHSL{.a = c.a};

		result.l = (min + max) / 2.0f;
		if (result.l > 0.0f && result.l < 1.0f)
			result.s = delta / (result.l < 0.5f ? (2.0f * result.l) : (2.0f - 2.0f * result.l));

		if (delta > 0)
		{
			if (max == c.r && max != c.g)
				result.h += (c.g - c.b) / delta;

			if (max == c.g && max != c.b)
				result.h += (2 + (c.b - c.r) / delta);

			if (max == c.b && max != c.r)
				result.h += (4 + (c.r - c.g) / delta);

			result.h *= 60;
		}

		return result;
	}

	constexpr Colorf to_rgb(ColorHSL c)
	{
		while (c.h < 0.0f)
			c.h += 360.0f;
		while (c.h > 360.0f)
			c.h -= 360.0f;

		Colorf sat;
		if (c.h < 120.0f)
		{
			sat.r = (120.0f - c.h) / 60.0f;
			sat.g = c.h / 60.0f;
			sat.b = 0.0f;
		}
		else if (c.h < 240.0f)
		{
			sat.r = 0.0f;
			sat.g = (240.0f - c.h) / 60.0f;
			sat.b = (c.h - 120.0f) / 60.0f;
		}
		else
		{
			sat.r = (c.h - 240.0f) / 60.0f;
			sat.g = 0.0f;
			sat.b = (360.0f - c.h) / 60.0f;
		}
		sat.r = std::min(sat.r, 1.0f);
		sat.g = std::min(sat.g, 1.0f);
		sat.b = std::min(sat.b, 1.0f);

		auto tmp = Colorf{
			.r = 2 * c.s * sat.r + (1 - c.s),
			.g = 2 * c.s * sat.g + (1 - c.s),
			.b = 2 * c.s * sat.b + (1 - c.s),
		};

		auto result = Colorf{.a = c.a};

		if (c.l < 0.5f)
		{
			result.r = c.l * tmp.r;
			result.g = c.l * tmp.g;
			result.b = c.l * tmp.b;
		}
		else
		{
			result.r = (1 - c.l) * tmp.r + 2 * c.l - 1;
			result.g = (1 - c.l) * tmp.g + 2 * c.l - 1;
			result.b = (1 - c.l) * tmp.b + 2 * c.l - 1;
		}

		return result;
	}

	constexpr ColorHSL to_hsl(Color c) noexcept
	{
		return to_hsl(to_color(c));
	}

	constexpr Color lerp(Color c1, Color c2, std::floating_point auto t) noexcept
	{
		return {
			.r = uint8_t(std::lerp(c1.r, c2.r, t)),
			.g = uint8_t(std::lerp(c1.g, c2.g, t)),
			.b = uint8_t(std::lerp(c1.b, c2.b, t)),
			.a = uint8_t(std::lerp(c1.a, c2.a, t)),
		};
	}

	constexpr uint32_t to_pixel(const ColorFormat &format, Color color) noexcept
	{
		return uint32_t((color.r << format.shift_r) |
			(color.g << format.shift_g) |
			(color.b << format.shift_b) |
			(color.a << format.shift_a));
	}

	constexpr Color from_pixel(const ColorFormat &format, uint32_t pixel) noexcept
	{
		return {
			.r = uint8_t((pixel >> format.shift_r) & 0xFF),
			.g = uint8_t((pixel >> format.shift_g) & 0xFF),
			.b = uint8_t((pixel >> format.shift_b) & 0xFF),
			.a = uint8_t((pixel >> format.shift_a) & 0xFF),
		};
	}

	constexpr uint32_t to_pixel(const ColorFormat &format, Colorf color) noexcept
	{
		return to_pixel(format, to_color(color));
	}

	constexpr Color blend(Color dst, Color src) noexcept
	{
		auto one_minus_src_a = 255 - src.a;

		// fixed-point multiply for 8-bit fractional component
		constexpr auto fp_mul = [](auto a, auto b) {
			// fixed point multiply is just regular multiply, but the decimal place ends up being the sum
			// of the fixed point position for a and b. Since both a and b are 8-bit fractions, we need to
			// shift down by 8 to move it back into place
			return uint8_t((a * b) >> 8);
		};

		return Color{
			.r = uint8_t(fp_mul(dst.r, one_minus_src_a) + fp_mul(src.r, src.a)),
			.g = uint8_t(fp_mul(dst.g, one_minus_src_a) + fp_mul(src.g, src.a)),
			.b = uint8_t(fp_mul(dst.b, one_minus_src_a) + fp_mul(src.b, src.a)),
			.a = src.a,
		};
	}

	constexpr uint32_t blend(const ColorFormat &format, uint32_t dst, uint32_t src) noexcept
	{
		auto dst_r = (dst >> format.shift_r) & 0xFF;
		auto dst_g = (dst >> format.shift_g) & 0xFF;
		auto dst_b = (dst >> format.shift_b) & 0xFF;
		auto dst_a = (dst >> format.shift_a) & 0xFF;

		auto src_r = (src >> format.shift_r) & 0xFF;
		auto src_g = (src >> format.shift_g) & 0xFF;
		auto src_b = (src >> format.shift_b) & 0xFF;
		auto src_a = (src >> format.shift_a) & 0xFF;

		// floating point version
		// auto alpha = src_a / 255.0f;
		// auto one_minus_alpha = 1.0f - alpha;

		// Color c{
		// 	.r = uint8_t(dst_r * one_minus_alpha + src_r * alpha),
		// 	.g = uint8_t(dst_g * one_minus_alpha + src_g * alpha),
		// 	.b = uint8_t(dst_b * one_minus_alpha + src_b * alpha),
		// 	.a = uint8_t(src_a),
		// };

		// return to_pixel(format, c);

		// fixed point version
		auto one_minus_src_a = 255 - src_a;

		// fixed-point multiply for 8-bit fractional component
		constexpr auto fp_mul = [](auto a, auto b) {
			// fixed point multiply is just regular multiply, but the decimal place ends up being the sum
			// of the fixed point position for a and b. Since both a and b are 8-bit fractions, we need to
			// shift down by 8 to move it back into place
			return uint8_t((a * b) >> 8);
		};

		Color c{
			.r = uint8_t(fp_mul(dst_r, one_minus_src_a) + fp_mul(src_r, src_a)),
			.g = uint8_t(fp_mul(dst_g, one_minus_src_a) + fp_mul(src_g, src_a)),
			.b = uint8_t(fp_mul(dst_b, one_minus_src_a) + fp_mul(src_b, src_a)),
			.a = uint8_t(fp_mul(dst_a, one_minus_src_a) + src_a),
		};

		return to_pixel(format, c);
	}

	template <typename T>
	constexpr Size<T> &operator+=(Size<T> &s1, Size<T> s2) noexcept
	{
		s1.w += s2.w;
		s1.h += s2.h;

		return s1;
	}

	template <typename T>
	constexpr Size<T> operator+(Size<T> s1, Size<T> s2) noexcept
	{
		s1 /= s2;
		return s1;
	}

	template <typename T>
	constexpr Size<T> &operator+=(Size<T> &size, T scalar) noexcept
	{
		size.w += scalar;
		size.h += scalar;
		return size;
	}

	template <typename T>
	constexpr Size<T> operator+(Size<T> size, T scalar) noexcept
	{
		size += scalar;
		return size;
	}

	template <typename T>
	constexpr Size<T> operator+(T scalar, Size<T> size) noexcept
	{
		size.w = scalar + size.w;
		size.h = scalar + size.h;
		return size;
	}

	template <typename T>
	constexpr Size<T> &operator-=(Size<T> &s1, Size<T> s2) noexcept
	{
		s1.w -= s2.w;
		s1.h -= s2.h;

		return s1;
	}

	template <typename T>
	constexpr Size<T> operator-(Size<T> s1, Size<T> s2) noexcept
	{
		s1 -= s2;
		return s1;
	}

	template <typename T>
	constexpr Size<T> &operator-=(Size<T> &size, T scalar) noexcept
	{
		size.w -= scalar;
		size.h -= scalar;
		return size;
	}

	template <typename T>
	constexpr Size<T> operator-(Size<T> size, T scalar) noexcept
	{
		size -= scalar;
		return size;
	}

	template <typename T>
	constexpr Size<T> operator-(T scalar, Size<T> size) noexcept
	{
		size.w = scalar - size.w;
		size.h = scalar - size.h;
		return size;
	}

	template <typename T>
	constexpr Size<T> &operator*=(Size<T> &s1, Size<T> s2) noexcept
	{
		s1.w *= s2.w;
		s1.h *= s2.h;

		return s1;
	}

	template <typename T>
	constexpr Size<T> operator*(Size<T> s1, Size<T> s2) noexcept
	{
		s1 *= s2;
		return s1;
	}

	template <typename T>
	constexpr Size<T> &operator*=(Size<T> &size, T scalar) noexcept
	{
		size.w *= scalar;
		size.h *= scalar;
		return size;
	}

	template <typename T>
	constexpr Size<T> operator*(Size<T> size, T scalar) noexcept
	{
		size *= scalar;
		return size;
	}

	template <typename T>
	constexpr Size<T> operator*(T scalar, Size<T> size) noexcept
	{
		size.w = scalar * size.w;
		size.h = scalar * size.h;
		return size;
	}

	template <typename T>
	constexpr Size<T> &operator/=(Size<T> &s1, Size<T> s2) noexcept
	{
		s1.w /= s2.w;
		s1.h /= s2.h;

		return s1;
	}

	template <typename T>
	constexpr Size<T> operator/(Size<T> s1, Size<T> s2) noexcept
	{
		s1 /= s2;
		return s1;
	}

	template <typename T>
	constexpr Size<T> &operator/=(Size<T> &size, T scalar) noexcept
	{
		size.w /= scalar;
		size.h /= scalar;
		return size;
	}

	template <typename T>
	constexpr Size<T> operator/(Size<T> size, T scalar) noexcept
	{
		size /= scalar;
		return size;
	}

	template <typename T>
	constexpr Size<T> operator/(T scalar, Size<T> size) noexcept
	{
		size.w = scalar / size.w;
		size.h = scalar / size.h;
		return size;
	}

	template <typename T>
	constexpr Rect<T> rect(Point2<T> p1, Point2<T> p2) noexcept
	{
		return {
			.x = std::min(p1.x, p2.x),
			.y = std::min(p1.y, p2.y),
			.w = abs(p2.x - p1.x) + 1,
			.h = abs(p2.y - p1.y) + 1,
		};
	}

	template <typename T>
	constexpr Rect<T> rect(Point2<T> p, Size<T> s) noexcept
	{
		return {
			.x = p.x,
			.y = p.y,
			.w = s.w,
			.h = s.h,
		};
	}

	template <typename T>
	constexpr Rect<T> rect(T x, T y, T w, T h) noexcept
	{
		return {
			.x = x,
			.y = y,
			.w = w,
			.h = h,
		};
	}

	template <typename T>
	constexpr Point2<T> top_left(Rect<T> rect) noexcept
	{
		return {rect.x, rect.y};
	}

	template <typename T>
	constexpr Point2<T> top_right(Rect<T> rect) noexcept
	{
		return {rect.x + rect.w - 1, rect.y};
	}

	template <typename T>
	constexpr Point2<T> bottom_left(Rect<T> rect) noexcept
	{
		return {rect.x, rect.y + rect.h - 1};
	}

	template <typename T>
	constexpr Point2<T> bottom_right(Rect<T> rect) noexcept
	{
		return {rect.x + rect.w - 1, rect.y + rect.h - 1};
	}

	template <typename T>
	constexpr Point2<T> center(Rect<T> rect) noexcept
	{
		return {rect.x + rect.w / 2, rect.y + rect.h / 2};
	}

	template <typename T>
	constexpr Point2<T> center(Circle<T> circle) noexcept
	{
		return circle.pos;
	}

	template <typename T>
	constexpr Size<T> size(Rect<T> rect) noexcept
	{
		return {rect.w, rect.h};
	}

	template <typename T>
	constexpr Rect<T> widen(Rect<T> rect, T amount) noexcept
	{
		return {rect.x - amount, rect.y - amount, rect.w + amount * 2, rect.h + amount * 2};
	}

	template <typename T>
	constexpr Rect<T> widen(Rect<T> rect, Size<T> amount) noexcept
	{
		return {rect.x - amount.w, rect.y - amount.h, rect.w + amount.w * 2, rect.h + amount.h * 2};
	}

	template <typename T>
	constexpr Rect<T> &operator+=(Rect<T> &rect, Point2<T> pos)
	{
		rect.x += pos.x;
		rect.y += pos.y;
		return rect;
	}

	template <typename T>
	constexpr Rect<T> operator+(Rect<T> rect, Point2<T> pos)
	{
		rect += pos;
		return rect;
	}

	template <typename T>
	constexpr Rect<T> &operator-=(Rect<T> &rect, Point2<T> pos)
	{
		rect.x -= pos.x;
		rect.y -= pos.y;
		return rect;
	}

	template <typename T>
	constexpr Rect<T> operator-(Rect<T> rect, Point2<T> pos)
	{
		rect -= pos;
		return rect;
	}

	template <typename T>
	constexpr std::optional<Rect<T>> clip_rect(Rect<T> bounds, Rect<T> r) noexcept
	{
		auto bounds_br = bottom_right(bounds);
		auto br = bottom_right(r);

		if (r.x <= bounds_br.x && bounds.x <= br.x &&
			r.y <= bounds_br.y && bounds.y <= br.y)
		{
			auto p1 = Point2{
				std::clamp(r.x, bounds.x, bounds_br.x),
				std::clamp(r.y, bounds.y, bounds_br.y),
			};

			auto p2 = Point2{
				std::clamp(br.x, bounds.x, bounds_br.x),
				std::clamp(br.y, bounds.y, bounds_br.y),
			};

			return std::make_optional<Rect<T>>(rect(p1, p2));
		}

		return std::nullopt;
	}

	template <typename T>
	constexpr bool contains(Rect<T> rect, Point2<T> point) noexcept
	{
		return point.x >= rect.x &&
			point.y >= rect.y &&
			point.x < rect.x + rect.w &&
			point.y < rect.y + rect.h;
	}

	template <typename T>
	concept BoundedArea = requires(T t, T u)
	{
		// clang-format off
		{collides(t, u)} -> std::convertible_to<bool>;
		{collides(u, t)} -> std::convertible_to<bool>;
		{center(t)};
		// clang-format on
	};

	template <typename T>
	constexpr bool collides(Rect<T> r1, Rect<T> r2) noexcept
	{
		return r1.x < r2.x + r2.w &&
			r1.y < r2.y + r2.h &&
			r2.x < r1.x + r1.w &&
			r2.y < r1.y + r1.h;
	}

	template <typename T>
	constexpr bool collides(Circle<T> c1, Circle<T> c2) noexcept
	{
		auto p = c2.pos - c1.pos;
		auto dist_r = c1.radius + c2.radius;

		// we can skip the square root and just compare the magnitutes
		return (p.x * p.x + p.y * p.y) < (dist_r * dist_r);
	}

	template <typename T>
	constexpr bool collides(Circle<T> c, Rect<T> r) noexcept
	{
		// get a point within the rect nearest the circle's center
		auto test_x = std::clamp(c.pos.x, r.x, r.x + r.w);
		auto test_y = std::clamp(c.pos.y, r.y, r.y + r.h);

		// get distance from closest edges
		float dx = c.pos.x - test_x;
		float dy = c.pos.y - test_y;

		float dist_square = dx * dx + dy * dy;

		// we can skip the square root and just compare the magnitutes
		return dist_square < c.radius * c.radius;
	}

	template <typename T>
	constexpr bool collides(Rect<T> r, Circle<T> c) noexcept
	{
		return collides(std::forward<Circle<T>>(c), std::forward<Rect<T>>(r));
	}

	template <std::integral ResultT = int, std::floating_point T>
	constexpr Point2<ResultT> to_integral(Point2<T> point) noexcept
	{
		return {
			ResultT(point.x),
			ResultT(point.y),
		};
	}

	template <std::floating_point ResultT = float, std::integral T>
	constexpr Point2<ResultT> to_floating(Point2<T> point) noexcept
	{
		return {
			ResultT(point.x),
			ResultT(point.y),
		};
	}

	template <std::integral ResultT = int, std::floating_point T>
	constexpr Size<ResultT> to_integral(Size<T> size) noexcept
	{
		return {
			ResultT(size.w),
			ResultT(size.h),
		};
	}

	template <std::floating_point ResultT = float, std::integral T>
	constexpr Size<ResultT> to_floating(Size<T> size) noexcept
	{
		return {
			ResultT(size.w),
			ResultT(size.h),
		};
	}

	template <std::integral ResultT = int, std::floating_point T>
	constexpr Rect<ResultT> to_integral(Rect<T> rect) noexcept
	{
		return {
			ResultT(rect.x),
			ResultT(rect.y),
			ResultT(rect.w),
			ResultT(rect.h),
		};
	}

	template <std::floating_point ResultT = float, std::integral T>
	constexpr Rect<ResultT> to_floating(Rect<T> rect) noexcept
	{
		return {
			ResultT(rect.x),
			ResultT(rect.y),
			ResultT(rect.w),
			ResultT(rect.h),
		};
	}

	template <std::integral ResultT = int, std::floating_point T>
	constexpr Circle<ResultT> to_integral(Circle<T> circle) noexcept
	{
		return {
			.radius = ResultT(circle.radius),
			.pos = to_integral<ResultT>(circle.pos),
		};
	}

	template <std::floating_point ResultT = float, std::integral T>
	constexpr Circle<ResultT> to_floating(Circle<T> circle) noexcept
	{
		return {
			.pos = to_floating<ResultT>(circle.pos),
			.radius = ResultT(circle.radius),
		};
	}

	///////////
	// void clipMY(double x[], double y[], double minx, double miny, double maxx, double maxy)
	// {
	// 	// gradient and y-intercept of the line
	// 	double m, c;
	// 	int i;
	// 	// non vertical lines
	// 	if (x[0] != x[1])
	// 	{
	// 		// non vertical and non horizontal lines
	// 		if (y[0] != y[1])
	// 		{
	// 			// calculate the gradient
	// 			m = (y[0] - y[1]) / (x[0] - x[1]);
	// 			// calculate the y-intercept
	// 			c = (x[0] * y[1] - x[1] * y[0]) / (x[0] - x[1]);
	// 			for (i = 0; i < 2; i++)
	// 			{
	// 				if (x[i] < minx)
	// 				{
	// 					x[i] = minx;
	// 					y[i] = m * minx + c;
	// 				}
	// 				else if (x[i] > maxx)
	// 				{
	// 					x[i] = maxx;
	// 					y[i] = m * maxx + c;
	// 				}
	// 				if (y[i] < miny)
	// 				{
	// 					x[i] = (miny - c) / m;
	// 					y[i] = miny;
	// 				}
	// 				else if (y[i] > maxy)
	// 				{
	// 					x[i] = (maxy - c) / m;
	// 					y[i] = maxy;
	// 				}
	// 			}
	// 			// initial line is completely outside
	// 			if ((x[0] - x[1] < 1) && (x[1] - x[0] < 1))
	// 			{
	// 				// do nothing
	// 			}
	// 			// draw the clipped line
	// 			else
	// 			{
	// 				setcolor(15);
	// 				line(x[0], y[0], x[1], y[1]);
	// 			}
	// 		}
	// 		// horizontal lines
	// 		else
	// 		{
	// 			// initial line is completely outside
	// 			if ((y[0] <= miny) || (y[0] >= maxy))
	// 			{
	// 				// do nothing
	// 			}
	// 			else
	// 			{
	// 				for (i = 0; i < 2; i++)
	// 				{
	// 					if (x[i] < minx)
	// 					{
	// 						x[i] = minx;
	// 					}
	// 					else if (x[i] > maxx)
	// 					{
	// 						x[i] = maxx;
	// 					}
	// 				}
	// 				// initial line is completely outside
	// 				if ((x[0] - x[1] < 1) && (x[1] - x[0] < 1))
	// 				{
	// 					// do nothing
	// 				}
	// 				// draw the clipped line
	// 				else
	// 				{
	// 					setcolor(15);
	// 					line(x[0], y[0], x[1], y[1]);
	// 				}
	// 			}
	// 		}
	// 	}
	// 	// vertical lines
	// 	else
	// 	{
	// 		// initial line is just a point
	// 		if (y[0] == y[1])
	// 		{
	// 			// initial point is outside
	// 			if ((y[0] <= miny) || (y[0] >= maxy))
	// 			{
	// 				// do nothing
	// 			}
	// 			// initial point is outside
	// 			else if ((x[0] <= minx) || (x[0] >= maxx))
	// 			{
	// 				// do nothing
	// 			}
	// 			// initial point is inside
	// 			else
	// 			{
	// 				putpixel(x[0], y[0], 15);
	// 			}
	// 		}
	// 		// initial line is completely outside
	// 		else if ((x[0] <= minx) || (x[0] >= maxx))
	// 		{
	// 			// do nothing
	// 		}
	// 		else
	// 		{
	// 			for (i = 0; i < 2; i++)
	// 			{
	// 				if (y[i] < miny)
	// 				{
	// 					y[i] = miny;
	// 				}
	// 				else if (y[i] > maxy)
	// 				{
	// 					y[i] = maxy;
	// 				}
	// 			}
	// 			// initial line is completely outside
	// 			if ((y[0] - y[1] < 1) && (y[1] - y[0] < 1))
	// 			{
	// 				// do nothing
	// 			}
	// 			// draw the clipped line
	// 			else
	// 			{
	// 				setcolor(15);
	// 				line(x[0], y[0], x[1], y[1]);
	// 			}
	// 		}
	// 	}
	// }
	///////////

	// From: An Efficient Algorithm for Line Clipping in Computer Graphics Programming (2013)
	// By: S. R. Kodituwakku1, K. R. Wijeweera1, M. A. P. Chamikara
	// Department of Statistics & Computer Science, University of Peradeniya, Sri Lanka
	// Postgraduate Institute of Science, University of Peradeniya, Sri Lanka
	// https://www.pdn.ac.lk/research/journal/cjsps/pdf/paper%2017_1.pdf
	template <std::floating_point T>
	constexpr std::optional<std::pair<Point2<T>, Point2<T>>> clip_line_Kodituwakku_Wijeweere_Chamikara(const Rect<T> rect, const Point2<T> p1, const Point2<T> p2) noexcept
	{
		using ResultType = std::pair<Point2<T>, Point2<T>>;

		auto [minx, miny] = top_left(rect);
		auto [maxx, maxy] = bottom_right(rect);

		auto x = std::array{p1.x, p2.x};
		auto y = std::array{p1.y, p2.y};

		// non vertical lines
		if (x[0] != x[1])
		{
			// non vertical and non horizontal lines
			if (y[0] != y[1])
			{
				// gradient of the line
				T m = (y[0] - y[1]) / (x[0] - x[1]);
				// y-intercept of the line
				T c = (x[0] * y[1] - x[1] * y[0]) / (x[0] - x[1]);

				for (int i = 0; i < 2; i++)
				{
					if (x[i] < minx)
					{
						x[i] = minx;
						y[i] = m * minx + c;
					}
					else if (x[i] > maxx)
					{
						x[i] = maxx;
						y[i] = m * maxx + c;
					}
					if (y[i] < miny)
					{
						x[i] = (miny - c) / m;
						y[i] = miny;
					}
					else if (y[i] > maxy)
					{
						x[i] = (maxy - c) / m;
						y[i] = maxy;
					}
				}

				if (!(x[0] - x[1] < 1 && x[1] - x[0] < 1))
					return std::make_optional<ResultType>(Point2<T>(x[0], y[0]), Point2<T>(x[1], y[1]));
			}
			// horizontal lines
			else
			{
				if (!(y[0] < miny || y[0] > maxy))
				{
					for (int i = 0; i < 2; i++)
					{
						if (x[i] < minx)
							x[i] = minx;
						else if (x[i] > maxx)
							x[i] = maxx;
					}

					if (!(x[0] - x[1] < 1 && x[1] - x[0] < 1))
						return std::make_optional<ResultType>(Point2<T>(x[0], y[0]), Point2<T>(x[1], y[1]));
				}
			}
		}
		// vertical lines
		else
		{
			// initial line is just a point
			if (y[0] == y[1])
			{
				if (!(y[0] < miny || y[0] > maxy) &&
					!(x[0] < minx || x[0] > maxx))
					return std::make_optional<ResultType>(Point2<T>(x[0], y[0]), Point2<T>(x[1], y[1]));
			}
			else if (!(x[0] < minx || x[0] > maxx))
			{
				for (int i = 0; i < 2; i++)
				{
					if (y[i] < miny)
						y[i] = miny;
					else if (y[i] > maxy)
						y[i] = maxy;
				}

				if (!(y[0] - y[1] < 1 && y[1] - y[0] < 1))
					return std::make_optional<ResultType>(Point2<T>(x[0], y[0]), Point2<T>(x[1], y[1]));
			}
		}

		return std::nullopt;
	}

	// From: A Simple and Fast Line-Clipping Method as a Scratch Extension for Computer Graphics Education (2019)
	// By: Dimitrios Matthes, Vasileios Drakopoulos
	// Faculty of Sciences, Department of Computer Science and Biomedical Informatics, Lamia, 35131, Central Greece, Greece
	// https://www.researchgate.net/publication/333809404_A_Simple_and_Fast_Line-Clipping_Method_as_a_Scratch_Extension_for_Computer_Graphics_Education
	template <std::floating_point T>
	constexpr std::optional<std::pair<Point2<T>, Point2<T>>> clip_line_Matthes_Drakopoulos(const Rect<T> rect, const Point2<T> p1, const Point2<T> p2) noexcept
	{
		/* Pseudo-code:
		// x1 , y1 , x2 , y2 , minx ,ymayx , xmax , ymin //
		if not ( x1<xmin and x2xmixn  anyd not ( x1>xmax and x2>xmax ) then
		    if not ( y1<ymin and y2<ymin ) and not ( y1>ymax and y2>ymax ) then
		        x[1] = x1
		        y[1] = y1
		        x[2] = x2
		        y[2] = y2
		        i = 1
		        repeat
		            if x[i] < xmin then
		                x[i] = xmin
		                y[i] = ( ( y2−y1 ) / ( x2−x1 ) )∗( xmin−x1 )+ y1
		            else if x[i] > xmax then
		                x[i] = xmax
		                y[i] = ( ( y2−y1 ) / ( x2−x1 ) )∗( xmax−x1 )+ y1
		            end if
		            if y[i] < ymin then
		                y[i] = ymin
		                x[i] = ( ( x2−x1 ) / ( y2−y1 ) )∗( ymin−y1 )+ x1
		            else if y[i] > ymax then
		                y[i] = ymax
		                x[i] = ( ( x2−x1 ) / ( y2−y1 ) )∗( ymax−y1 )+ x1
		            end if
		            i = i + 1
		        until i>2
		        if not (x[1] < xmin and x[2] < xmin) then
		            if not (x[1] > xmax and x[2] > xmax) then
		                drawLine(x[1], y[1], x[2], y[2])
		            end if
		        end if
		    end if
		end if
		*/

		auto [xmin, ymin] = top_left(rect);
		auto [xmax, ymax] = bottom_right(rect);

		if (!(p1.x < xmin && p2.x < xmin) && !(p1.x > xmax && p2.x > xmax))
		{
			if (!(p1.y < ymin && p2.y < ymin) && !(p1.y > ymax && p2.y > ymax))
			{
				Point2 r1 = p1;

				if (r1.x < xmin)
				{
					r1.x = xmin;
					r1.y = ((p2.y - p1.y) / (p2.x - p1.x)) * (xmin - p1.x) + p1.y;
				}
				else if (r1.x > xmax)
				{
					r1.x = xmax;
					r1.y = ((p2.y - p1.y) / (p2.x - p1.x)) * (xmax - p1.x) + p1.y;
				}

				if (r1.y < ymin)
				{
					r1.y = ymin;
					r1.x = ((p2.x - p1.x) / (p2.y - p1.y)) * (ymin - p1.y) + p1.x;
				}
				else if (r1.y > ymax)
				{
					r1.y = ymax;
					r1.x = ((p2.x - p1.x) / (p2.y - p1.y)) * (ymax - p1.y) + p1.x;
				}

				Point2 r2 = p2;

				if (r2.x < xmin)
				{
					r2.x = xmin;
					r2.y = ((p2.y - p1.y) / (p2.x - p1.x)) * (xmin - p1.x) + p1.y;
				}
				else if (r2.x > xmax)
				{
					r2.x = xmax;
					r2.y = ((p2.y - p1.y) / (p2.x - p1.x)) * (xmax - p1.x) + p1.y;
				}

				if (r2.y < ymin)
				{
					r2.y = ymin;
					r2.x = ((p2.x - p1.x) / (p2.y - p1.y)) * (ymin - p1.y) + p1.x;
				}
				else if (r2.y > ymax)
				{
					r2.y = ymax;
					r2.x = ((p2.x - p1.x) / (p2.y - p1.y)) * (ymax - p1.y) + p1.x;
				}

				if (!(r1.x < xmin && r2.x < xmin))
				{
					if (!(r1.x > xmax && r2.x > xmax))
					{
						using ResultType = std::pair<Point2<T>, Point2<T>>;
						return std::make_optional<ResultType>(r1, r2);
					}
				}
			}
		}

		return std::nullopt;
	}

	enum class ClipAlgorithm
	{
		Default,
		Kodituwakku_Wijeweere_Chamikara,
		Matthes_Drakopoulos,
	};

	template <ClipAlgorithm clip_algorithm = ClipAlgorithm::Default, std::floating_point T>
	constexpr std::optional<std::pair<Point2<T>, Point2<T>>> clip_line(Rect<T> rect, Point2<T> p1, Point2<T> p2) noexcept
	{
		if constexpr (clip_algorithm == ClipAlgorithm::Kodituwakku_Wijeweere_Chamikara)
			return clip_line_Kodituwakku_Wijeweere_Chamikara(std::forward<Rect<T>>(rect), std::forward<Point2<T>>(p1), std::forward<Point2<T>>(p2));
		else if constexpr (clip_algorithm == ClipAlgorithm::Matthes_Drakopoulos)
			return clip_line_Matthes_Drakopoulos(std::forward<Rect<T>>(rect), std::forward<Point2<T>>(p1), std::forward<Point2<T>>(p2));
		else
			return clip_line_Kodituwakku_Wijeweere_Chamikara(std::forward<Rect<T>>(rect), std::forward<Point2<T>>(p1), std::forward<Point2<T>>(p2));
	}

	template <ClipAlgorithm clip_algorithm = ClipAlgorithm::Default, std::integral T>
	constexpr std::optional<std::pair<Point2<T>, Point2<T>>> clip_line(Rect<T> rect, Point2<T> p1, Point2<T> p2) noexcept
	{
		auto result = clip_line<clip_algorithm>(to_floating(rect), to_floating(p1), to_floating(p2));
		if (result)
		{
			using ResultType = std::pair<Point2<T>, Point2<T>>;
			return std::make_optional<ResultType>(to_integral(result->first), to_integral(result->second));
		}

		return std::nullopt;
	}

	using Point2f = Point2<float>;
	using Point2i = Point2<int>;
	using Sizef = Size<float>;
	using Sizei = Size<int>;
	using Rectf = Rect<float>;
	using Recti = Rect<int>;
	using Circlef = Circle<float>;
	using Circlei = Circle<int>;

	namespace detail
	{
		constexpr void math_constexpr_tests() noexcept
		{
			static_assert(BoundedArea<Recti>);
			static_assert(BoundedArea<Rectf>);
			static_assert(BoundedArea<Circlei>);
			static_assert(BoundedArea<Circlef>);

			constexpr auto p1 = Point2{4, 2};
			constexpr auto p2 = Point2{13, 13};

			static_assert(25 == distance_sq(Point2{0, 0}, Point2{3, 4}));
			// static_assert(5 == distance(Point2{0, 0}, Point2{3, 4}));

			static_assert(p1 == curve(p1, {-1, 5}, p2, 0.0f));
			static_assert(p2 == curve(p2, {-1, 5}, p2, 1.0f));

			constexpr auto s1 = Size{10, 12};
			constexpr auto s2 = s1 / 2;
			static_assert(s2 == Size{5, 6});

			constexpr auto r1 = Rect{4, 2, 10, 12};
			constexpr auto r2 = rect(p1, p2);
			constexpr auto r3 = rect(p1, s1);
			static_assert(r1 == r2);
			static_assert(r2 == r3);

			static_assert(top_left(r1) == p1);
			static_assert(top_right(r1) == Point2{13, 2});
			static_assert(bottom_left(r1) == Point2{4, 13});
			static_assert(bottom_right(r1) == p2);

			// landmark points should be contained in the rect
			static_assert(contains(r1, top_left(r1)));
			static_assert(contains(r1, top_right(r1)));
			static_assert(contains(r1, bottom_left(r1)));
			static_assert(contains(r1, bottom_right(r1)));

			constexpr auto format = ColorFormat{};

			constexpr auto c1 = Color(1, 2, 3, 4);

			// to/from _pixel round trip
			constexpr auto pixel1 = to_pixel(format, c1);
			constexpr auto c2 = from_pixel(format, pixel1);
			static_assert(c1 == c2);

			// to_color round trip
			static_assert(c1 == to_color(to_color(c1)));

			constexpr auto l1 = lerp(Color(0, 0, 0, 0), Color(255, 255, 255, 255), 0.0f);
			constexpr auto l2 = lerp(Color(0, 0, 0, 0), Color(255, 255, 255, 255), 0.5f);
			constexpr auto l3 = lerp(Color(0, 0, 0, 0), Color(255, 255, 255, 255), 1.0f);
			static_assert(l1 == Color(0, 0, 0, 0));
			static_assert(l2 == Color(127, 127, 127, 127));
			static_assert(l3 == Color(255, 255, 255, 255));

			{
				constexpr auto clip_rect = rect(0, 0, 10, 10);

				constexpr auto clip1 = clip_line(clip_rect, {1, 5}, {7, 5});
				static_assert(clip1.has_value());
				static_assert(clip1->first == Point2{1, 5});
				static_assert(clip1->second == Point2{7, 5});

				constexpr auto clip2 = clip_line(clip_rect, {-1, 5}, {20, 5});
				static_assert(clip2.has_value());
				static_assert(clip2->first == Point2{0, 5});
				static_assert(clip2->second == Point2{9, 5});

				constexpr auto clip3 = clip_line(clip_rect, {5, -1}, {5, 20});
				static_assert(clip3.has_value());
				static_assert(clip3->first == Point2{5, 0});
				static_assert(clip3->second == Point2{5, 9});

				constexpr auto clip4 = clip_line(clip_rect, {-5, -1}, {-5, -20});
				static_assert(!clip4.has_value());
			}
			{
				constexpr auto clip_rect = rect(0.0f, 0.0f, 10.0f, 10.0f);

				constexpr auto clip1 = clip_line(clip_rect, {1.0f, 5.0f}, {7.0f, 5.0f});
				static_assert(clip1.has_value());
				static_assert(contains(clip_rect, clip1->first));
				static_assert(contains(clip_rect, clip1->second));

				constexpr auto clip2 = clip_line(clip_rect, {-1.0f, 5.0f}, {20.0f, 5.0f});
				static_assert(clip2.has_value());
				static_assert(contains(clip_rect, clip2->first));
				static_assert(contains(clip_rect, clip2->second));

				constexpr auto clip3 = clip_line(clip_rect, {5.0f, -1.0f}, {5.0f, 20.0f});
				static_assert(clip3.has_value());
				static_assert(contains(clip_rect, clip3->first));
				static_assert(contains(clip_rect, clip3->second));

				constexpr auto clip4 = clip_line(clip_rect, {20.0f, -1.0f}, {15, 20.0f});
				static_assert(!clip4.has_value());
			}

			{
				constexpr auto size = Size{200, 150};
				constexpr auto bad1 = Point2{.x = 210, .y = -7};
				constexpr auto bad2 = Point2{.x = 73, .y = 71};

				constexpr auto bounds = rect({0, 0}, size);
				constexpr auto result = clip_line(bounds, bad1, bad2);

				static_assert(result.has_value());
				static_assert(contains(bounds, result->first), "The clipped line should be within our clipping boundary!");
				static_assert(contains(bounds, result->second), "The clipped line should be within our clipping boundary!");
			}

			{
				constexpr auto clip1 = clip_rect(r1, Rect{r1.x + 2, r1.y + 2, r1.w, r1.h});
				static_assert(clip1);
				static_assert(clip1->w == 8);
				static_assert(clip1->h == 10);

				constexpr auto clip2 = clip_rect(r1, Rect{r1.x - 2, r1.y - 2, r1.w, r1.h});
				static_assert(clip2);
				static_assert(clip2->w == 8);
				static_assert(clip2->h == 10);

				constexpr auto clip3 = clip_rect(r1, Rect{r1.x - 20, r1.y - 20, r1.w, r1.h});
				static_assert(!clip3);
			}

			{
				constexpr auto bounds = rect(Point2{0, 0}, Point2{200, 150});
				constexpr auto clip = clip_line(bounds, {-50, 114}, {136, 247});

				static_assert(!clip);
				// static_assert(clip);
				// static_assert(contains(bounds, clip->first));
				// static_assert(contains(bounds, clip->second));
			}
			{
				constexpr auto bounds = rect(Point2{0, 0}, Point2{200, 150});
				constexpr auto clip = clip_line(bounds, {-59, -97}, {200, 150});

				static_assert(clip);
				static_assert(contains(bounds, clip->first));
				static_assert(contains(bounds, clip->second));
			}
			{
				constexpr auto bounds = rect(Point2{0, 0}, Point2{200, 150});
				constexpr auto clip = clip_line(bounds, {0, 0}, {0, 150});

				static_assert(clip);
				static_assert(contains(bounds, clip->first));
				static_assert(contains(bounds, clip->second));
			}
			{
				constexpr auto bounds = rect(Point2{0, 0}, Point2{200, 150});
				constexpr auto clip = clip_line(bounds, {0, 0}, {200, 0});

				static_assert(clip);
				static_assert(contains(bounds, clip->first));
				static_assert(contains(bounds, clip->second));
			}

			{
				constexpr auto ball1 = Circlef{
					.radius = 31.604132f,
					.pos = Point2f{384.904907f, 232.534729f},
				};
				constexpr auto ball2 = Circlef{
					.radius = 39.012817f,
					.pos = Point2f{400.714386f, 271.606720f},
				};

				constexpr auto diff = ball1.pos - ball2.pos;
				static_assert((diff.x * diff.x + diff.y * diff.y) < (ball1.radius * ball1.radius + ball2.radius * ball2.radius));

				static_assert(collides(ball1, ball2));
			}

			{
				constexpr auto ball1 = cm::Circlef{
					.radius = 33.640793f,
					.pos = cm::Point2f{373.846802f, 377.605988f},
				};
				constexpr auto ball2 = cm::Circlef{
					.radius = 41.628155f,
					.pos = cm::Point2f{504.975586f, 281.952026f},
				};

				constexpr auto diff = ball1.pos - ball2.pos;
				// static_assert(std::sqrt(diff.x * diff.x + diff.y * diff.y) > (ball1.radius + ball2.radius));
				static_assert((diff.x * diff.x + diff.y * diff.y) > (ball1.radius * ball1.radius + ball2.radius * ball2.radius));
				static_assert(!collides(ball1, ball2));
			}
		}
	}
}
