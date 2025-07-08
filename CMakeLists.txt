# CMake配置用于编译多智能体Python绑定
cmake_minimum_required(VERSION 3.16)
project(MultiAgentGoBigger)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Python和pybind11
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# 查找pybind11
find_package(pybind11 QUIET)
if(NOT pybind11_FOUND)
    # 如果系统没有pybind11，尝试使用pip安装的版本
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m pybind11 --cmake
        OUTPUT_VARIABLE pybind11_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(pybind11_DIR)
        find_package(pybind11 REQUIRED HINTS ${pybind11_DIR})
    else()
        message(FATAL_ERROR "pybind11 not found. Please install: pip install pybind11")
    endif()
endif()

# 设置Qt路径
set(CMAKE_PREFIX_PATH "D:/qt/6.9.1/msvc2022_64" CACHE PATH "Qt installation path")
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# 启用Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 多智能体模块源文件
set(MULTI_AGENT_SOURCES
    ../python/multi_agent_game_engine.cpp
    ../python/multi_agent_bindings.cpp
    
    # src_new中的核心文件
    ../src_new/GameManager.cpp
    ../src_new/MultiPlayerManager.cpp
    ../src_new/SimpleAIPlayer.cpp
    ../src_new/CloneBall.cpp
    ../src_new/BaseBall.cpp
    ../src_new/FoodBall.cpp
    ../src_new/SporeBall.cpp
    ../src_new/ThornsBall.cpp
    ../src_new/QuadTree.cpp
    ../src_new/PlayerCell.cpp
    ../src_new/ONNXInference.cpp
)

# 多智能体模块头文件
set(MULTI_AGENT_HEADERS
    ../python/multi_agent_game_engine.h
    ../python/pybind11_qt_casters.h
    
    # src_new中的头文件
    ../src_new/GameManager.h
    ../src_new/MultiPlayerManager.h
    ../src_new/SimpleAIPlayer.h
    ../src_new/CloneBall.h
    ../src_new/BaseBall.h
    ../src_new/FoodBall.h
    ../src_new/SporeBall.h
    ../src_new/ThornsBall.h
    ../src_new/QuadTree.h
    ../src_new/PlayerCell.h
    ../src_new/ONNXInference.h
    ../src_new/GoBiggerConfig.h
)

# 创建多智能体Python模块
pybind11_add_module(gobigger_multi_env ${MULTI_AGENT_SOURCES} ${MULTI_AGENT_HEADERS})

# 设置编译属性
target_compile_definitions(gobigger_multi_env PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})
target_compile_features(gobigger_multi_env PRIVATE cxx_std_17)

# 链接Qt库
target_link_libraries(gobigger_multi_env PRIVATE Qt6::Core Qt6::Widgets)

# 设置包含目录
target_include_directories(gobigger_multi_env PRIVATE 
    ../src_new
    ../python
)

# 设置输出目录
set_target_properties(gobigger_multi_env PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ../python
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ../python
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ../python
)

# Windows特定设置
if(WIN32)
    set_target_properties(gobigger_multi_env PROPERTIES
        SUFFIX ".pyd"
        LIBRARY_OUTPUT_NAME "gobigger_multi_env"
    )
endif()

# 编译器特定设置
if(MSVC)
    target_compile_options(gobigger_multi_env PRIVATE /W4)
else()
    target_compile_options(gobigger_multi_env PRIVATE -Wall -Wextra -pedantic)
endif()

# 自动复制Qt DLL到输出目录（Windows）
if(WIN32)
    add_custom_command(TARGET gobigger_multi_env POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Qt6::Core>
        ../python/
        
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Qt6::Widgets>
        ../python/
        
        COMMENT "Copying Qt DLLs to python directory"
    )
endif()

message(STATUS "🤖 多智能体Python绑定配置完成")
message(STATUS "   模块名: gobigger_multi_env")
message(STATUS "   输出目录: ../python")
message(STATUS "   源文件数: 16")
