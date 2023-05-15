add_executable(
	nesem WIN32
	"nesem.manifest"
	"main.cpp"
	"bottom_bar.cpp"
	"bottom_bar.hpp"
	"color_palette.cpp"
	"color_palette.hpp"
	"config.cpp"
	"config.hpp"
	"controller_overlay.cpp"
	"controller_overlay.hpp"
	"nes_app.cpp"
	"nes_app.hpp"
	"nes_overlay.cpp"
	"nes_overlay.hpp"
	"side_bar.cpp"
	"side_bar.hpp"
	"text.cpp"
	"text.hpp"
)

find_package(tomlplusplus CONFIG REQUIRED)

target_link_libraries(nesem PRIVATE project_options)
target_link_libraries(nesem PRIVATE nesemlib appui appui_main)
target_link_libraries(nesem PRIVATE tomlplusplus::tomlplusplus)
