#include "ui/canvas.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <numbers>
#include <utility>

#include <util/logging.hpp>

namespace
{
	struct BresenhamState
	{
		// convience flag for whether we've reached the end
		bool done = false;

		// the point on the line at the current step
		cm::Point2i current;

		// INTERNAL VARIABLES
		cm::Point2i goal;

		int x_inc;
		int y_inc;

		int dx;
		int dy;
		int err;
	};

	// Bresenham's line algorithm
	constexpr BresenhamState bresenham_line_init(cm::Point2i p1, cm::Point2i p2) noexcept
	{
		if (p1.y > p2.y)
			std::swap(p1, p2);

		auto diffx = p2.x - p1.x;
		auto diffy = p2.y - p1.y;

		auto dx = cm::abs(diffx);
		auto dy = cm::abs(diffy);

		return {
			.current = p1,
			.goal = p2,

			.x_inc = cm::signum(diffx),
			.y_inc = cm::signum(diffy),

			.dx = dx,
			.dy = dy,
			.err = dx - dy,
		};
	}

	constexpr bool bresenham_line_next(BresenhamState &state) noexcept
	{
		if (state.current == state.goal)
		{
			state.done = true;
		}
		else
		{
			auto e2 = 2 * state.err;

			if (e2 >= -state.dy)
			{
				state.err -= state.dy;
				state.current.x += state.x_inc;
			}
			if (e2 <= state.dx)
			{
				state.err += state.dx;
				state.current.y += state.y_inc;
			}
		}

		return !state.done;
	}

	// Draw an anti-aliased line using Xiaolin Wu's line algorithm
	void wu_line(ui::Canvas &canvas, const cm::Color color, cm::Point2i p1, cm::Point2i p2) noexcept
	{
		if (p1.y > p2.y)
			std::swap(p1, p2);

		auto p = p1;
		const auto goal = p2;

		// Draw the initial pixel, which is always exactly intersected by the line and so needs no weighting
		canvas.draw_point(color, p);

		int delta_x = goal.x - p.x;
		int delta_y = goal.y - p.y;
		int x_inc = 1;

		if (delta_x < 0)
		{
			x_inc = -1;
			delta_x = -delta_x;
		}

		CHECK(delta_x >= 0, "delta_x should be positive at this point");
		CHECK(delta_y >= 0, "delta_y should be positive");

		// First, handle special cases for horizontal, vertical, and diagonal lines
		// These require no weighting because they go right through the center of every pixel

		// Horizontal line
		if (delta_y == 0)
		{
			while (delta_x-- != 0)
			{
				p.x += x_inc;
				canvas.draw_point(color, p);
			}
			return;
		}

		// Vertical line
		if (delta_x == 0)
		{
			do
			{
				++p.y;
				canvas.draw_point(color, p);
			} while (--delta_y != 0);
			return;
		}

		// Diagonal line
		if (delta_x == delta_y)
		{
			do
			{
				p.x += x_inc;
				++p.y;
				canvas.draw_point(color, p);
			} while (--delta_y != 0);
			return;
		}

		// At this point, the line is not horizontal, diagonal, or vertical

		// # of bits by which to shift ErrorAcc to get intensity level
		const uint32_t intensity_bits = 8;
		const uint32_t intensity_shift = 16 - intensity_bits;

		// Mask used to flip all bits in an intensity weighting, producing the result (1 - intensity weighting)
		uint32_t weighting_complement_mask = 0xFF;

		// major_len - the length in pixels of the major axis
		// error_adj - 16-bit fixed-point fractional part of a pixel, used to advance the minor axis when this overflows
		// adv_major - function to advance our major axis
		// adv_minor - function to advance our minor axis
		// paired_pt - make a point for the complementary pixel, one unit away in the proper direction
		auto do_draw = [&](auto major_len, auto error_adj, auto adv_major, auto adv_minor, auto paired_pt) {
			// 16-bit fixed-point number
			uint32_t error_acc = 0;

			while (--major_len)
			{
				// calculate error for next pixel
				error_acc += error_adj;

				// if the high-word is > 0, we have accumulated enough error to advance on the minor axis
				if (error_acc >> 16)
				{
					// keep only the fractional portion
					error_acc &= 0xFFFF;

					// The error accumulator turned over, so advance the minor axis
					adv_minor();
				}

				// always advance the major axis
				adv_major();

				// The intensity_bits most significant bits of error_acc give us the intensity
				// weighting for this pixel, and the complement of the weighting for the paired pixel
				// the range is 0 -> 255 from most to least intense for the current pixel, and least to most for paired pixel
				uint32_t weight = error_acc >> intensity_shift;

				// draw the paired point first, since weight is already inverted
				auto c = cm::Color{
					.r = color.r,
					.g = color.g,
					.b = color.b,
					.a = uint8_t((color.a * weight) >> intensity_shift),
				};

				canvas.draw_point(c, paired_pt(p));

				// invert weight and draw the current point
				c.a = uint8_t((color.a * (weight ^ weighting_complement_mask)) >> intensity_shift);
				canvas.draw_point(c, p);
			}

			// Draw the final pixel, which is always exactly intersected by the line and so needs no weighting
			canvas.draw_point(color, goal);
		};

		// Is this an X-major or Y-major line?
		if (delta_y > delta_x)
		{
			// Y-major line; calculate 16-bit fixed-point fractional part of a pixel that X advances each time Y
			// advances 1 pixel, truncating the result so that we won't overrun the endpoint along the X axis
			auto error_adj = uint32_t((delta_x << 16) / delta_y);

			auto adv_major = [&] {
				++p.y;
			};

			auto adv_minor = [&] {
				p.x += x_inc;
			};

			auto paired_pt = [&](cm::Point2i pt) {
				return cm::Point2i{pt.x + x_inc, pt.y};
			};

			do_draw(delta_y, error_adj, adv_major, adv_minor, paired_pt);
		}
		else
		{
			// X-major line; calculate 16-bit fixed-point fractional part of a pixel that Y advances each time X
			// advances 1 pixel, truncating the result to avoid overrunning the endpoint along the X axis
			auto error_adj = uint32_t((delta_y << 16) / delta_x);

			auto adv_major = [&] {
				p.x += x_inc;
			};

			auto adv_minor = [&] {
				++p.y;
			};

			auto paired_pt = [&](cm::Point2i pt) {
				return cm::Point2i{pt.x, pt.y + 1};
			};

			do_draw(delta_x, error_adj, adv_major, adv_minor, paired_pt);
		}
	}
}

namespace ui
{
	Canvas::Canvas(cm::Sizei size, cm::ColorFormat format) noexcept
		: canvas_size(size), canvas_format(format), canvas_ptr(new uint32_t[size.w * size.h])
	{
	}

	Canvas::Canvas(cm::Sizei size, cm::ColorFormat format, uint32_t *ptr) noexcept
		: canvas_size(size), canvas_format(format), canvas_ptr(ptr), owns_canvas(false)
	{
	}

	Canvas::~Canvas()
	{
		cleanup();
	}

	void Canvas::cleanup() noexcept
	{
		if (owns_canvas)
			delete[] canvas_ptr;

		canvas_ptr = nullptr;
	}

	Canvas::Canvas(Canvas &&other) noexcept
		: canvas_size(std::exchange(other.canvas_size, {})),
		  canvas_format(other.canvas_format),
		  canvas_ptr(std::exchange(other.canvas_ptr, nullptr)),
		  owns_canvas(other.owns_canvas)
	{
	}

	Canvas &Canvas::operator=(Canvas &&other) noexcept
	{
		cleanup();

		canvas_size = std::exchange(other.canvas_size, {});
		canvas_format = other.canvas_format;
		canvas_ptr = std::exchange(other.canvas_ptr, nullptr);
		owns_canvas = other.owns_canvas;

		return *this;
	}

	Canvas::operator bool() const noexcept
	{
		return canvas_ptr != nullptr;
	}

	cm::Sizei Canvas::size() const noexcept
	{
		return canvas_size;
	}

	cm::ColorFormat Canvas::format() const noexcept
	{
		return canvas_format;
	}

	void Canvas::enable_blending(bool enable) noexcept
	{
		blending_enabled = enable;
	}

	uint32_t *Canvas::ptr() const noexcept
	{
		return canvas_ptr;
	}

	ptrdiff_t Canvas::to_index(cm::Point2i point) const noexcept
	{
		return point.y * canvas_size.w + point.x;
	}

	cm::Point2i Canvas::from_index(ptrdiff_t index) const noexcept
	{
		auto [y, x] = std::lldiv(index, canvas_size.w);

		CHECK(std::in_range<int>(x), "screen width ought to fit in an int");
		CHECK(std::in_range<int>(y), "screen height ought to fit in an int");

		return {int(x), int(y)};
	}

	void Canvas::fill(cm::Color color) noexcept
	{
		if (!VERIFY(canvas_ptr, "Invalid canvas"))
			return;

		std::fill_n(canvas_ptr, canvas_size.w * canvas_size.h, to_pixel(canvas_format, color));
	}

	void Canvas::do_draw_point(uint32_t pixel, cm::Point2i point) noexcept
	{
		CHECK(contains(rect({0, 0}, canvas_size), point), "point should be on screen!");

		if (blending_enabled)
		{
			auto index = to_index(point);
			canvas_ptr[index] = cm::blend(canvas_format, canvas_ptr[index], pixel);
		}
		else
			canvas_ptr[to_index(point)] = pixel;
	}

	void Canvas::draw_point(cm::Color color, cm::Point2i point) noexcept
	{
		if (!VERIFY(canvas_ptr, "Invalid canvas"))
			return;

		if (!contains(rect({0, 0}, canvas_size), point))
			return;

		auto pixel = to_pixel(canvas_format, color);
		do_draw_point(pixel, point);
	}

	void Canvas::draw_line(cm::Color color, cm::Point2i p1, cm::Point2i p2, bool antialias) noexcept
	{
		if (!VERIFY(canvas_ptr, "Invalid canvas"))
			return;

		constexpr bool skip_clipping = true;

		if constexpr (skip_clipping)
		{
			if (antialias)
			{
				wu_line(*this, color, p1, p2);
			}
			else
			{
				for (auto state = bresenham_line_init(p1, p2); !state.done; bresenham_line_next(state))
					draw_point(color, state.current);
			}
		}
		else
		{
			auto clipped_line = clip_line(rect({0, 0}, canvas_size), p1, p2);
			if (!clipped_line)
				return;

			auto [c1, c2] = clipped_line.value();

			CHECK(contains(rect({0, 0}, canvas_size), c1), "clipped point should be on screen!");
			CHECK(contains(rect({0, 0}, canvas_size), c2), "clipped point should be on screen!");

			for (auto state = bresenham_line_init(c1, c2); !state.done; bresenham_line_next(state))
				draw_point(color, state.current);
		}
	}

	void Canvas::draw_triangle(cm::Color color, cm::Point2i p1, cm::Point2i p2, cm::Point2i p3, bool antialias) noexcept
	{
		if (!VERIFY(canvas_ptr, "Invalid canvas"))
			return;

		// sort points by y to match fill algorithm
		auto ps = std::array{p1, p2, p3};
		std::ranges::sort(ps, {}, &cm::Point2i::y);

		draw_line(color, ps[0], ps[1], antialias);

		// if the top or bottom is flat, just draw the line
		if (ps[0].y == ps[1].y || ps[1].y == ps[2].y)
			draw_line(color, ps[0], ps[2], antialias);
		else
		{
			// split the line and draw in two segments to match the fill algorithm
			// this reduces rounding artifacts when trying to outline a filled rectangle
			auto mid = cm::Point2i{
				.x = ps[0].x + (ps[1].y - ps[0].y) * (ps[2].x - ps[0].x) / (ps[2].y - ps[0].y),
				.y = ps[1].y,
			};

			draw_line(color, ps[0], mid, antialias);
			draw_line(color, mid, ps[2], antialias);
		}

		draw_line(color, ps[1], ps[2], antialias);
	}

	void Canvas::fill_triangle(cm::Color color, cm::Point2i p1, cm::Point2i p2, cm::Point2i p3) noexcept
	{
		// TODO: consider clipping the triangle against the view to skip clipping versions of draw_line/draw_point

		if (!VERIFY(canvas_ptr, "Invalid canvas"))
			return;

		auto ps = std::array{p1, p2, p3};
		std::ranges::sort(ps, {}, &cm::Point2i::y);

		auto fill = [this, color](BresenhamState state_1, BresenhamState state_2) {
			while (!state_1.done && !state_2.done)
			{
				CHECK(state_1.current.y == state_2.current.y, "Should always be on the same scanline");

				auto scanline = state_1.current.y;

				draw_line(color, state_1.current, state_2.current);

				bresenham_line_next(state_1);
				bresenham_line_next(state_2);

				// if we have more points on this scanline, draw them individually
				while (!state_1.done && state_1.current.y == scanline)
				{
					draw_point(color, state_1.current);
					bresenham_line_next(state_1);
				}

				// if we have more points on this scanline, draw them individually
				while (!state_2.done && state_2.current.y == scanline)
				{
					draw_point(color, state_2.current);
					bresenham_line_next(state_2);
				}
			}
		};

		auto fill_flat_bottom = [&fill](auto pt1, auto pt2, auto pt3) {
			CHECK(pt2.y == pt3.y, "bottom not flat");
			fill(bresenham_line_init(pt1, pt2), bresenham_line_init(pt1, pt3));
		};

		auto fill_flat_top = [&fill](auto pt1, auto pt2, auto pt3) {
			CHECK(pt1.y == pt2.y, "top not flat");
			fill(bresenham_line_init(pt1, pt3), bresenham_line_init(pt2, pt3));
		};

		if (ps[1].y == ps[2].y)
			fill_flat_bottom(ps[0], ps[1], ps[2]);

		else if (ps[0].y == ps[1].y)
			fill_flat_top(ps[0], ps[1], ps[2]);

		else
		{
			// split the triangle into two flat triangles
			auto mid = cm::Point2i{
				.x = ps[0].x + (ps[1].y - ps[0].y) * (ps[2].x - ps[0].x) / (ps[2].y - ps[0].y),
				.y = ps[1].y,
			};

			fill_flat_bottom(ps[0], ps[1], mid);
			fill_flat_top(ps[1], mid, ps[2]);
		}
	}

	void Canvas::draw_rect(cm::Color color, cm::Recti rect) noexcept
	{
		if (!VERIFY(canvas_ptr, "Invalid canvas"))
			return;

		auto tl = top_left(rect);
		auto tr = top_right(rect);
		auto bl = bottom_left(rect);
		auto br = bottom_right(rect);

		if (tr.y >= 0 && tr.y < canvas_size.h)
			draw_line(color, tl, tr);

		if (tr.x >= 0 && tr.x < canvas_size.w)
			draw_line(color, tr, br);

		if (bl.y >= 0 && bl.y < canvas_size.h)
			draw_line(color, br, bl);

		if (bl.x >= 0 && bl.x < canvas_size.w)
			draw_line(color, bl, tl);
	}

	void Canvas::fill_rect(cm::Color color, cm::Recti r) noexcept
	{
		if (!VERIFY(canvas_ptr, "Invalid canvas"))
			return;

		auto clip = clip_rect(rect({0, 0}, canvas_size), r);
		if (!clip)
			return;

		auto x1 = clip->x;
		auto x2 = clip->x + clip->w - 1;
		auto y = clip->y;

		for (int offset = 0; offset < clip->h; ++offset)
			draw_line(color, {x1, y + offset}, {x2, y + offset});
	}

	void Canvas::draw_curve(cm::Color color, cm::Point2i p1, cm::Point2i p2, cm::Point2i p3) noexcept
	{
		if (!VERIFY(canvas_ptr, "Invalid canvas"))
			return;

		auto f1 = to_floating(p1);
		auto f2 = to_floating(p2);
		auto f3 = to_floating(p3);

		/*
		// dumb, but works well enough
		// assume we need to draw at most distance points between the point and control
		// use the longer distance to calculate an increment
		// TODO: reduce the number of increments by subdividing until we are moving <= 1 pixel
		// TODO: reduce the number of increments by subdividing until we are moving <= 1 pixel
		auto dist = std::max(distance(f1, f2), distance(f2, f3));
		auto inc = 0.5f / dist;

		for (auto t = 0.0f; t <= 1.0f; t += inc)
		{
		    auto p = curve(f1, f2, f3, t);
		    draw_point(color, to_integral(p));
		}
		*/

		auto last_p = f1;
		draw_point(color, p1);

		const auto inc = 0.1f;
		float t = 0.0f;

		while (t <= 1.0f)
		{
			auto current_inc = inc;
			auto p = curve(f1, f2, f3, t + current_inc);

			while (distance_sq(last_p, p) >= 1.5f)
			{
				current_inc *= 0.5f;
				p = curve(f1, f2, f3, t + current_inc);
			}

			draw_point(color, to_integral(p));
			last_p = p;
			t += current_inc;
		}
	}

	void Canvas::draw_circle(cm::Color color, cm::Circlei circle) noexcept
	{
		auto off = cm::Point2{0, circle.radius};
		int p = (5 - circle.radius * 4) / 4;

		auto draw = [&]() {
			if (off.x == 0)
			{
				draw_point(color, cm::Point2{circle.pos.x, circle.pos.y + off.y});
				draw_point(color, cm::Point2{circle.pos.x, circle.pos.y - off.y});
				draw_point(color, cm::Point2{circle.pos.x + off.y, circle.pos.y});
				draw_point(color, cm::Point2{circle.pos.x - off.y, circle.pos.y});
			}
			else if (off.x == off.y)
			{
				draw_point(color, cm::Point2{circle.pos.x + off.x, circle.pos.y + off.y});
				draw_point(color, cm::Point2{circle.pos.x - off.x, circle.pos.y + off.y});
				draw_point(color, cm::Point2{circle.pos.x + off.x, circle.pos.y - off.y});
				draw_point(color, cm::Point2{circle.pos.x - off.x, circle.pos.y - off.y});
			}
			else if (off.x < off.y)
			{
				draw_point(color, cm::Point2{circle.pos.x + off.x, circle.pos.y + off.y});
				draw_point(color, cm::Point2{circle.pos.x - off.x, circle.pos.y + off.y});
				draw_point(color, cm::Point2{circle.pos.x + off.x, circle.pos.y - off.y});
				draw_point(color, cm::Point2{circle.pos.x - off.x, circle.pos.y - off.y});
				draw_point(color, cm::Point2{circle.pos.x + off.y, circle.pos.y + off.x});
				draw_point(color, cm::Point2{circle.pos.x - off.y, circle.pos.y + off.x});
				draw_point(color, cm::Point2{circle.pos.x + off.y, circle.pos.y - off.x});
				draw_point(color, cm::Point2{circle.pos.x - off.y, circle.pos.y - off.x});
			}
		};

		draw();
		while (off.x < off.y)
		{
			++off.x;
			if (p < 0)
				p += 2 * off.x + 1;
			else
			{
				--off.y;
				p += 2 * (off.x - off.y) + 1;
			}
			draw();
		}
	}

	void Canvas::fill_circle(cm::Color color, cm::Circlei circle) noexcept
	{
		auto off = cm::Point2{0, circle.radius};
		int p = (5 - circle.radius * 4) / 4;

		auto draw = [&]() {
			if (off.x == 0)
			{
				draw_point(color, cm::Point2{circle.pos.x, circle.pos.y + off.y});
				draw_point(color, cm::Point2{circle.pos.x, circle.pos.y - off.y});

				draw_line(color,
					cm::Point2{circle.pos.x - off.y, circle.pos.y},
					cm::Point2{circle.pos.x + off.y, circle.pos.y});
			}
			else if (off.x == off.y)
			{
				draw_line(color,
					cm::Point2{circle.pos.x - off.x, circle.pos.y + off.y},
					cm::Point2{circle.pos.x + off.x, circle.pos.y + off.y});

				draw_line(color,
					cm::Point2{circle.pos.x - off.x, circle.pos.y - off.y},
					cm::Point2{circle.pos.x + off.x, circle.pos.y - off.y});
			}
			else if (off.x < off.y)
			{
				draw_line(color,
					cm::Point2{circle.pos.x - off.x, circle.pos.y + off.y},
					cm::Point2{circle.pos.x + off.x, circle.pos.y + off.y});

				draw_line(color,
					cm::Point2{circle.pos.x + off.x, circle.pos.y - off.y},
					cm::Point2{circle.pos.x - off.x, circle.pos.y - off.y});

				draw_line(color,
					cm::Point2{circle.pos.x + off.y, circle.pos.y + off.x},
					cm::Point2{circle.pos.x - off.y, circle.pos.y + off.x});

				draw_line(color,
					cm::Point2{circle.pos.x + off.y, circle.pos.y - off.x},
					cm::Point2{circle.pos.x - off.y, circle.pos.y - off.x});
			}
		};

		draw();
		while (off.x < off.y)
		{
			++off.x;
			if (p < 0)
				p += 2 * off.x + 1;
			else
			{
				--off.y;
				p += 2 * (off.x - off.y) + 1;
			}
			draw();
		}
	}

	void Canvas::blit(cm::Point2i dst, const Canvas &src, const std::optional<cm::Recti> &src_rect, cm::Sizei scale) noexcept
	{
		// start point is too big to be visible, so we are done
		if (dst.x >= canvas_size.w || dst.y >= canvas_size.h)
			return;

		auto src_area = src_rect.value_or(rect({}, src.canvas_size));
		auto dst_area = rect(dst, cm::size(src_area) * scale);

		// the dst rect is completely offscreen, we are done
		if (auto dst_br = bottom_right(dst_area);
			dst_br.x < 0 || dst_br.y < 0)
			return;

		// clip dest rect to the visible portion, and apply the equivalent clip to the src area as well

		auto start_col_index = 0;
		auto start_line_index = 0;

		auto stop_col_index = dst_area.w;
		auto stop_line_index = dst_area.h;

		if (dst_area.x < start_col_index)
		{
			// shift the start and stop indexes over by the amount we are skipping
			start_col_index -= dst_area.x;
			stop_col_index += start_col_index;
		}

		if (dst_area.y < start_line_index)
		{
			// shift the start and stop indexes over by the amount we are skipping
			start_line_index -= dst_area.y;
			stop_line_index += start_line_index;
		}

		// reduce our stop index by the number of extra lines
		if (auto x = canvas_size.w - dst_area.w;
			x < 0)
			stop_col_index += x;

		// reduce our stop index by the number of extra lines
		if (auto y = canvas_size.h - dst_area.h;
			y < 0)
			stop_line_index += y;

		// we should now have clipped src and dst areas, time to draw!
		if (!blending_enabled && scale.w == 1)
		{
			// no blending and no scaling of the width means we can just memcpy each line to the dst
			for (int line = start_line_index; line < stop_line_index; ++line)
			{
				int dst_x = dst_area.x + start_col_index;
				int dst_y = dst_area.y + line;
				int src_y = (line / scale.h) + src_area.y;
				std::copy_n(src.canvas_ptr + src.to_index({src_area.x, src_y}), src_area.w, canvas_ptr + to_index({dst_x, dst_y}));
			}
		}
		else
		{
			// we need to blend or we need to scale up pixels in the horizontal direction; either way, draw pixel by pixel
			for (int line = start_line_index; line < stop_line_index; ++line)
			{
				int dst_y = dst_area.y + line;
				int src_y = (line / scale.h) + src_area.y;

				for (int i = start_col_index; i < stop_col_index; ++i)
				{
					int dst_x = dst_area.x + i;
					int src_x = (i / scale.w) + src_area.x;

					do_draw_point(src.canvas_ptr[src.to_index({src_x, src_y})], {dst_x, dst_y});
				}
			}
		}
	}
}
