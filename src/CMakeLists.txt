include(FindPkgConfig)

pkg_search_module(HIDAPI REQUIRED hidapi-hidraw)
pkg_search_module(LIBEVDEV REQUIRED libevdev)
pkg_search_module(GLIB REQUIRED glib-2.0)

add_executable(jcdri
  main.cpp
  ipc_source.cpp
  event_loop.cpp
  joycon.cpp
  udev.cpp
  utils.cpp
  rumble.cpp
  devices/dual_dev.cpp
  devices/single_dev.cpp
  devices/jc_device.cpp
#devices/single_dev.c
#devices/keyboard_dual.c
)

add_executable(jccli
  cli.cpp
#devices/single_dev.c
#devices/keyboard_dual.c
)

target_compile_options(jcdri PUBLIC -Wall -Wextra -Werror -g)
target_compile_options(jccli PUBLIC -Wall -Wextra -Werror -g)

target_include_directories(jcdri PUBLIC ../includes)
target_include_directories(jcdri PUBLIC ${HIDAPI_INCLUDE_DIRS})
target_include_directories(jcdri PUBLIC ${GLIB_INCLUDE_DIRS})
target_include_directories(jcdri PUBLIC ${LIBEVDEV_INCLUDE_DIRS})
target_link_libraries(jcdri PUBLIC ${HIDAPI_LIBRARIES} ${LIBEVDEV_LIBRARIES} m pthread)
target_link_libraries(jcdri PUBLIC
  ${HIDAPI_LIBRARIES}
  ${LIBEVDEV_LIBRARIES}
  udev
  m)


