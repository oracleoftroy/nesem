#include <catch2/catch_test_macros.hpp>
#include <nes.hpp>

std::filesystem::path find_path(const std::filesystem::path &path)
{
	namespace fs = std::filesystem;

	// remember the bit that varies from our current working directory
	const auto relative = fs::proximate(path);
	auto dir = fs::current_path();

	while (!fs::exists(dir / relative))
	{
		auto previous = std::exchange(dir, dir.parent_path());

		// we are at the root directory and can't go up anymore, so bail
		if (dir == previous)
			break;
	}

	if (auto p = dir / relative;
		fs::exists(p))
	{
		// we found a path, return it
		return p;
	}

	// could not find path, return original path
	return path;
}

TEST_CASE("nestest.nes should run", "[.skip][nestest.nes]")
{
	auto nes = nesem::Nes{nesem::NesSettings{.error = [](const auto &msg) { FAIL(msg); }}};

	if (!nes.load_rom(find_path("data/nestest.nes")))
		SKIP("Could not load nestest.nes");

	// put a hard limit on the number of iterations in case we hit an endless loop
	// nestest.nes should complete in 8991 instructions, so give a little room for failure
	size_t counter = 9000;

	// reset PC to $C000 for automated test running
	nes.cpu().reset(nesem::Addr{0xC000});

	SECTION("Official OP codes should work")
	{
		// PC $C6A9 marks the start into the unofficial OP code tests
		while (nes.cpu().state().PC != nesem::Addr{0xC6A9} && --counter > 0)
			nes.step(nesem::NesClockStep::OneCpuInstruction);

		CHECK(counter > 0);

		// address $0002 holds the result with 0 indicating success or an error code indicating which test failed
		auto result = nes.bus().read(nesem::Addr{0x0002}, nesem::NesBusOp::ready);
		CHECK(result == 0);
	}

	CHECK(counter > 0);

	// SECTION("Unofficial OP codes should work")
	// {
	// 	// PC $C5FF is the return address of the tests
	// 	// TODO: MAKE SURE THAT IS TRUE! Untested as unofficial op codes aren't supported at this time
	// 	while (nes.cpu().state().PC != nesem::Addr{0xC5FF} && --counter > 0)
	// 		nes.step(nesem::NesClockStep::OneCpuInstruction);

	// 	CHECK(counter > 0);

	// 	// address $0003 holds the result with 0 indicating success or an error code indicating which test failed
	// 	auto result = nes.bus().read(nesem::Addr{0x0003}, nesem::NesBusOp::ready);
	// 	CHECK(result == 0);
	// }

	CHECK(counter > 0);
}
