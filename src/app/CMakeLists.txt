find_package(spdlog CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

add_library(cmlib INTERFACE "include/cm/math.hpp" "include/cm/math3d.hpp")
target_link_libraries(cmlib INTERFACE project_options)
target_include_directories(cmlib INTERFACE include)

add_library(drawlib "src/canvas.cpp" "include/ui/canvas.hpp")
target_link_libraries(drawlib PRIVATE project_options)
target_link_libraries(drawlib PRIVATE cmlib util)
target_include_directories(drawlib PUBLIC include)

add_library(appui "src/app.cpp" "include/ui/app.hpp" "src/audio_device.cpp" "include/ui/audio_device.hpp" "src/app-main.cpp")
target_link_libraries(appui PRIVATE project_options)
target_link_libraries(appui PUBLIC drawlib util)
target_link_libraries(appui PRIVATE SDL2::SDL2 SDL2::SDL2main)
target_include_directories(appui PUBLIC include)
