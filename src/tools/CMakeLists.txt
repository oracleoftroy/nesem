add_executable(embed embed.cpp)

target_link_libraries(embed PRIVATE project_options util)
target_link_libraries(embed PRIVATE cryptopp::cryptopp)
target_link_libraries(embed PRIVATE fmt::fmt)
target_link_libraries(embed PRIVATE mio::mio)
