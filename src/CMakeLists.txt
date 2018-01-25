include(FindPkgConfig)

pkg_search_module(HIDAPI REQUIRED hidapi-hidraw)
pkg_search_module(LIBEVDEV REQUIRED libevdev)
pkg_search_module(GLIB REQUIRED glib-2.0)

add_executable(jcdri
main.c
joycon.c
utils.c
devices/dual_dev.c
devices/common_dev.c
devices/single_dev.c
rumble.c)

target_compile_options(jcdri PUBLIC -Wall -Wextra -Werror -g)

target_include_directories(jcdri PUBLIC ../includes)
target_include_directories(jcdri PUBLIC ${HIDAPI_INCLUDE_DIRS})
target_include_directories(jcdri PUBLIC ${GLIB_INCLUDE_DIRS})
target_include_directories(jcdri PUBLIC ${LIBEVDEV_INCLUDE_DIRS})
target_link_libraries(jcdri PUBLIC ${HIDAPI_LIBRARIES} ${LIBEVDEV_LIBRARIES} m pthread)
target_link_libraries(jcdri PUBLIC
  ${HIDAPI_LIBRARIES}
  ${LIBEVDEV_LIBRARIES}
  ${GLIB_LIBRARIES}
  m)


