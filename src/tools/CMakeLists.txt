add_executable(embed embed.cpp)

target_link_libraries(embed PRIVATE project_options util)

find_package(cryptopp CONFIG REQUIRED)
target_link_libraries(embed PRIVATE cryptopp::cryptopp)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(embed PRIVATE fmt::fmt)

find_package(mio CONFIG REQUIRED)
target_link_libraries(embed PRIVATE mio::mio)
