cmake_minimum_required(VERSION 4.4)
project(jappeos_daemon_core)

set(CMAKE_CXX_STANDARD 20)

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(GLIB REQUIRED glib-2.0)

set(JAPPEOS_DAEMON_SOURCES
        src/daemon.cpp
        src/daemon.h
        src/daemon_state.def
        src/utils/jos_exception.h
        src/utils/dbus_utils.h
        src/utils/dbus_peer.cpp
        src/utils/dbus_peer.h
        src/utils/scope_guard.h
        src/utils/command_runner.cpp
        src/utils/command_runner.h
        src/globals.h
        src/utils/stack_trace.h
        src/utils/null_check_exception.h
        src/dbus/dbus_wrapper.cpp
        src/dbus/dbus_wrapper.h
        src/logging/daemon_logger.h
        src/logging/stdout_logger.cpp
        src/logging/stdout_logger.h
        src/logging/logger.h
        src/logging/logger.cpp
        src/system/os_utils.h
        src/system/init_boot_file.h
)

function(jappeos_configure_target target_name)
    target_include_directories(${target_name} PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${DBUS_INCLUDE_DIRS}
            ${DBUS_INCLUDE_DIRS}/../dbus-1.0
            ${DBUS_LIBDIR}/dbus-1.0/include
            ${GLIB_INCLUDE_DIRS}
    )

    target_compile_options(${target_name} PRIVATE
            -march=x86-64
            -mtune=generic
            -fno-lto
            -mno-avx
            -mno-avx2
            -mno-fma
            -mno-bmi
            -mno-bmi2
            -mno-sse4.1
            -mno-sse4.2
            $<$<CONFIG:Debug>:-O0 -g>
            $<$<CONFIG:Release>:-O2 -g -fno-omit-frame-pointer>
            ${DBUS_CFLAGS_OTHER}
            ${GLIB_CFLAGS_OTHER}
    )

    target_link_options(${target_name} PRIVATE
            $<$<CONFIG:Debug>:-rdynamic>
    )

    target_link_libraries(${target_name} PUBLIC
            ${DBUS_LIBRARIES}
            ${GLIB_LIBRARIES}
    )

    set_target_properties(${target_name} PROPERTIES
            INSTALL_RPATH "$ORIGIN"
            BUILD_WITH_INSTALL_RPATH TRUE
    )
endfunction()

add_library(jappeos_daemon_core STATIC
        ${JAPPEOS_DAEMON_SOURCES}
)
add_library(jappeos_daemon_core::jappeos_daemon_core
        ALIAS jappeos_daemon_core
)
jappeos_configure_target(jappeos_daemon_core)