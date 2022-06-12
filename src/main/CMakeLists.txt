add_executable(
	nesem WIN32
	"nesem.manifest"
	"main.cpp"
	"nes_app.cpp"
	"nes_app.hpp"
	"nes_overlay.cpp"
	"nes_overlay.hpp"
	"bottom_bar.cpp"
	"bottom_bar.hpp"
	"side_bar.cpp"
	"side_bar.hpp"
	"text.cpp"
	"text.hpp"
)

target_link_libraries(nesem PRIVATE project_options)
target_link_libraries(nesem PRIVATE nesemlib)
target_link_libraries(nesem PRIVATE appui)
