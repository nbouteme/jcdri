include(FindPkgConfig)

pkg_search_module(HIDAPI REQUIRED hidapi-hidraw)
pkg_search_module(LIBEVDEV REQUIRED libevdev)

add_executable(jcdri main.c joycon.c utils.c evdev_wrapper.c rumble.c)
target_compile_options(jcdri PUBLIC -Wall -Wextra -Werror -g)

target_include_directories(jcdri PUBLIC ${HIDAPI_INCLUDE_DIRS})
target_include_directories(jcdri PUBLIC ${LIBEVDEV_INCLUDE_DIRS})
target_link_libraries(jcdri PUBLIC ${HIDAPI_LIBRARIES} ${LIBEVDEV_LIBRARIES} m pthread)

