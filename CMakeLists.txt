cmake_minimum_required(VERSION 3.15)
project(cpp_log VERSION 1.0.0
    DESCRIPTION "A modern C++ logging library"
    HOMEPAGE_URL "https://github.com/yourusername/cpp_log"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 查找Boost
find_package(Boost REQUIRED)

# 创建接口库
add_library(cpp_log INTERFACE)
target_include_directories(cpp_log INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>${Boost_INCLUDE_DIRS}
)

# 生成版本文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cpp_log-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 创建安装配置文件
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpp_log-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cpp_log-config.cmake"
    INSTALL_DESTINATION lib/cmake/cpp_log
)

# 安装目标
install(TARGETS cpp_log
    EXPORT cpp_log-targets
    INCLUDES DESTINATION include
)

# 安装头文件
install(DIRECTORY src/cpp_log
    DESTINATION includeFILES_MATCHING PATTERN "*.hpp"
)

# 安装CMake配置文件
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cpp_log-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cpp_log-config-version.cmake"
    DESTINATION lib/cmake/cpp_log
)

# 安装导出目标
install(EXPORT cpp_log-targets
    FILE cpp_log-targets.cmake
    NAMESPACE cpp_log::
    DESTINATION lib/cmake/cpp_log
)

# 生成并安装pkg-config文件
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpp_log.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cpp_log.pc"
    @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cpp_log.pc"
    DESTINATION lib/pkgconfig
)

# 添加示例（可选）
option(CPP_LOG_BUILD_EXAMPLES "Build cpp_log examples" ON)
if(CPP_LOG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
