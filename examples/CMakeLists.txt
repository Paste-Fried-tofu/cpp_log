add_executable(cpp_log_example example.cpp)
target_link_libraries(cpp_log_example PRIVATE cpp_log)

# 添加Boost依赖
find_package(Boost REQUIRED)
target_include_directories(cpp_log_example PRIVATE ${Boost_INCLUDE_DIRS})

# 设置C++20标准
set_target_properties(cpp_log_example PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON)
