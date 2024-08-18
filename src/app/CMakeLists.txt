add_library(cmlib INTERFACE "include/cm/math.hpp" "include/cm/math3d.hpp")
target_link_libraries(cmlib INTERFACE project_options)
target_include_directories(cmlib INTERFACE include)

add_library(drawlib "src/canvas.cpp" "include/ui/canvas.hpp")
target_link_libraries(drawlib PRIVATE project_options)
target_link_libraries(drawlib PRIVATE cmlib util)
target_include_directories(drawlib PUBLIC include)

add_library(
	appui
	"src/app.cpp"
	"include/ui/app.hpp"
	"src/audio_device.cpp"
	"include/ui/audio_device.hpp"
	"src/texture.cpp"
	"include/ui/texture.hpp"
	"src/renderer.cpp"
	"include/ui/renderer.hpp"
)
target_link_libraries(appui PRIVATE project_options)
target_link_libraries(appui PUBLIC drawlib util)
target_link_libraries(appui PRIVATE $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
target_include_directories(appui PUBLIC include)

add_library(appui_main OBJECT "src/app-main.cpp")
target_link_libraries(appui_main PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>)
