cmake_minimum_required(VERSION 3.5) # CMake install : https://cmake.org/download/
project(ai-devour-evolve LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 根据编译器选择Qt路径
if(MSVC)
    set(CMAKE_PREFIX_PATH "D:/qt/6.9.1/msvc2022_64;D:/tools/libtorch-cpu/libtorch") # Qt Kit Dir and LibTorch Dir
    message(STATUS "Using MSVC compiler with Qt MSVC build")
elseif(MINGW)
    set(CMAKE_PREFIX_PATH "D:/qt/6.9.1/mingw_64;D:/tools/libtorch-cpu/libtorch") # Qt Kit Dir and LibTorch Dir
    message(STATUS "Using MinGW compiler with Qt MinGW build")
else()
    message(WARNING "Unknown compiler, trying MSVC paths")
    set(CMAKE_PREFIX_PATH "D:/qt/6.9.1/msvc2022_64;D:/tools/libtorch-cpu/libtorch")
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 禁用CUDA以确保与MinGW兼容
set(USE_CUDA OFF)
set(CUDA_TOOLKIT_ROOT_DIR "")

find_package(Qt6 COMPONENTS Widgets REQUIRED) # Qt COMPONENTS
find_package(Torch REQUIRED)

# 过滤掉MSVC特定的编译标志，如果使用MinGW的话
if(WIN32 AND MINGW)
    foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        string(REPLACE "/EHsc" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/bigobj" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/MD" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/Z7" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/W3" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/Wall" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/wd4251" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/wd4244" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/wd4267" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/wd4996" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/wd4129" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/wd4458" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/wd4018" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/wd4190" "" ${flag_var} "${${flag_var}}")
    endforeach()
endif()

aux_source_directory(./src srcs)

# 明确指定源文件（推荐方式，更精确控制）
set(SOURCES
    src/main.cpp
    src/DemoQtVS.cpp
    src/GameView.cpp
    src/PlayerCell.cpp
    src/FoodItem.cpp  # 新增食物类
    # 未来添加更多源文件时在这里列出
)

set(HEADERS
    src/DemoQtVS.h
    src/GameView.h
    src/PlayerCell.h
    src/FoodItem.h    # 新增食物类
    # 未来添加更多头文件时在这里列出
)

set(UI_FILES
    src/DemoQtVS.ui
    # 未来添加更多UI文件时在这里列出
)

# Specify MSVC UTF-8 encoding   
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(${PROJECT_NAME}
    WIN32 # If you need a terminal for debug, please comment this statement 
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
)

# 设置头文件包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}  # 为自动生成的MOC文件
) 

# 将Qt6和Torch的库链接到我们的程序上
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    "${TORCH_LIBRARIES}"
)

# 对于Windows，解决LibTorch的一个常见链接问题
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -D_GLIBCXX_USE_CXX11_ABI=0)
    # 只在MinGW下禁用某些警告
    if(MINGW)
        target_compile_options(${PROJECT_NAME} PRIVATE -Wno-deprecated-declarations)
    endif()
    # MSVC特定设置
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W3)
        # 设置为多线程运行库
        set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

# 为将来的开发添加一些有用的编译定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060000  # 禁用Qt 6.0之前的废弃API
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# 安装规则（可选）
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 打印配置信息
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Qt found: ${Qt6_VERSION}")
message(STATUS "Torch found: ${Torch_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")