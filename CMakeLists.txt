cmake_minimum_required(VERSION 3.16)
project(fcitx5-accenthold VERSION 0.1.0 LANGUAGES CXX)

find_package(Fcitx5Core REQUIRED)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(libaccenthold MODULE accenthold.cpp)
target_link_libraries(libaccenthold Fcitx5::Core)
set_target_properties(libaccenthold PROPERTIES PREFIX "" OUTPUT_NAME libaccenthold)

# System install (used by packaging): the standard fcitx5 addon locations.
install(TARGETS libaccenthold LIBRARY DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES accenthold.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
