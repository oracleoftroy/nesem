#include <catch2/catch_test_macros.hpp>
#include <nes_apu.hpp>

TEST_CASE("Channel", "[nes_apu]")
{
	SECTION("Volume")
	{
		nesem::Channel c;
		c.set(0, 0x0F);
		CHECK(c.volume() == 0x0F);
		c.set(0, 0xFF);
		CHECK(c.volume() == 0x0F);
		c.set(0, 0xF1);
		c.set(1, 0xFF);
		c.set(2, 0xFF);
		c.set(3, 0xFF);
		CHECK(c.volume() == 0x01);
	}

	SECTION("Volume flag")
	{
		nesem::Channel c;
		c.set(0, 0x10);
		CHECK(c.use_constant_volume());
		c.set(0, 0xFF);
		CHECK(c.use_constant_volume());
		c.set(0, 0);
		CHECK(!c.use_constant_volume());

		c.set(0, 0xEF);
		c.set(1, 0xFF);
		c.set(2, 0xFF);
		c.set(3, 0xFF);
		CHECK(!c.use_constant_volume());
	}

	SECTION("halt flag")
	{
		nesem::Channel c;
		c.set(0, 0x20);
		CHECK(c.halt());
		c.set(0, 0xFF);
		CHECK(c.halt());
		c.set(0, 0);
		CHECK(!c.halt());

		c.set(0, 0xDF);
		c.set(1, 0xFF);
		c.set(2, 0xFF);
		c.set(3, 0xFF);
		CHECK(!c.halt());
	}

	SECTION("Duty")
	{
		nesem::Channel c;
		c.set(0, 0xC0);
		CHECK(c.duty() == 3);
		c.set(0, 0xFF);
		CHECK(c.duty() == 3);
		c.set(0, 0x3F);
		CHECK(c.duty() == 0);
		c.set(0, 0x7F);
		c.set(1, 0xFF);
		c.set(2, 0xFF);
		c.set(3, 0xFF);
		CHECK(c.duty() == 0x01);
	}

	SECTION("sweep enabled flag")
	{
		nesem::Channel c;
		c.set(1, 0x80);
		CHECK(c.sweep_enabled());
		c.set(1, 0xFF);
		CHECK(c.sweep_enabled());
		c.set(1, 0);
		CHECK(!c.sweep_enabled());

		c.set(1, 0x7F);
		c.set(0, 0xFF);
		c.set(2, 0xFF);
		c.set(3, 0xFF);
		CHECK(!c.sweep_enabled());
	}

	SECTION("sweep")
	{
		nesem::Channel c;
		c.set(1, 0x70);
		CHECK(c.sweep_period() == 7);
		c.set(1, 0xFF);
		CHECK(c.sweep_period() == 7);
		c.set(1, 0);
		CHECK(c.sweep_period() == 0);

		c.set(1, 0x1F);
		c.set(0, 0xFF);
		c.set(2, 0xFF);
		c.set(3, 0xFF);
		CHECK(c.sweep_period() == 1);
	}

	SECTION("negate flag")
	{
		nesem::Channel c;
		c.set(1, 0x08);
		CHECK(c.sweep_negate());
		c.set(1, 0xFF);
		CHECK(c.sweep_negate());
		c.set(1, 0);
		CHECK(!c.sweep_negate());

		c.set(1, 0xF7);
		c.set(0, 0xFF);
		c.set(2, 0xFF);
		c.set(3, 0xFF);
		CHECK(!c.sweep_negate());
	}

	SECTION("shift")
	{
		nesem::Channel c;
		c.set(1, 0x07);
		CHECK(c.sweep_shift() == 7);
		c.set(1, 0xFF);
		CHECK(c.sweep_shift() == 7);
		c.set(1, 0);
		CHECK(c.sweep_shift() == 0);

		c.set(1, 0xF1);
		c.set(0, 0xFF);
		c.set(2, 0xFF);
		c.set(3, 0xFF);
		CHECK(c.sweep_shift() == 1);
	}

	SECTION("timer")
	{
		nesem::Channel c;
		c.set(2, 0xFF);
		CHECK(c.timer() == 0xFF);
		c.set(3, 0x07);
		CHECK(c.timer() == 0x7FF);
		c.set(3, 0xFF);
		CHECK(c.timer() == 0x7FF);
		c.set(2, 0x55);
		CHECK(c.timer() == 0x755);

		c.set(3, 0xFA);
		c.set(2, 0xAA);
		c.set(1, 0xFF);
		c.set(0, 0xFF);
		CHECK(c.timer() == 0x2AA);
	}

	SECTION("length")
	{
		nesem::Channel c;
		c.set(3, 0xF8);
		CHECK(c.length() == 0x1F);
		c.set(3, 0xFF);
		CHECK(c.length() == 0x1F);
		c.set(3, 0);
		CHECK(c.length() == 0);

		c.set(3, 0xAF);
		c.set(2, 0xFF);
		c.set(1, 0xFF);
		c.set(0, 0xFF);
		CHECK(c.length() == 0x15);
	}
}
