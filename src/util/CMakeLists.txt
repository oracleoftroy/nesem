add_library(
	util
	INTERFACE
	"include/util/enum.hpp"
	"include/util/logging.hpp"
	"include/util/preprocessor.hpp"
	"include/util/ptr.hpp"
	"include/util/rng.hpp"
)
target_include_directories(util INTERFACE ${SCOTTT_DEBUGBREAK_INCLUDE_DIRS})
target_include_directories(util INTERFACE include)
target_link_libraries(util INTERFACE spdlog::spdlog)
target_link_libraries(util INTERFACE project_options)
