cmake_minimum_required(VERSION 3.5)
project(ai-devour-evolve LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 构建选项
option(ENABLE_GUI "Enable Qt GUI" ON)
option(ENABLE_PYTHON_BINDING "Enable Python binding with pybind11" OFF)

# 根据编译器选择Qt路径
if(MSVC)
    set(CMAKE_PREFIX_PATH "D:/qt/6.9.1/msvc2022_64;D:/tools/libtorch-cpu/libtorch") 
    message(STATUS "Using MSVC compiler with Qt MSVC build")
elseif(MINGW)
    set(CMAKE_PREFIX_PATH "D:/qt/6.9.1/mingw_64;D:/tools/libtorch-cpu/libtorch") 
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

# 查找依赖
find_package(Qt6 COMPONENTS Core Gui REQUIRED) # 核心库需要Qt6::Core和Qt6::Gui

if(ENABLE_GUI)
    find_package(Qt6 COMPONENTS Widgets REQUIRED) # GUI应用需要Qt6::Widgets
endif()

find_package(Torch REQUIRED)

# 如果启用Python绑定，查找pybind11
if(ENABLE_PYTHON_BINDING)
    find_package(pybind11 REQUIRED)
endif()

# 过滤掉MSVC特定的编译标志（MinGW兼容性）
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

# ============ 核心逻辑库 (gobigger_core) ============
set(CORE_SOURCES
    src/core/GameEngine.cpp
    src/core/CoreUtils.cpp
    src/core/data/BaseBallData.cpp
    src/core/data/CloneBallData.cpp
    src/core/data/FoodBallData.cpp
    src/core/data/SporeBallData.cpp
    src/core/data/ThornsBallData.cpp
)

set(CORE_HEADERS
    src/GoBiggerConfig.h
    src/core/GameEngine.h
    src/core/CoreUtils.h
    src/core/data/BaseBallData.h
    src/core/data/CloneBallData.h
    src/core/data/FoodBallData.h
    src/core/data/SporeBallData.h
    src/core/data/ThornsBallData.h
)

# 创建核心逻辑共享库
add_library(gobigger_core SHARED
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

# 核心库的编译设置
target_include_directories(gobigger_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# 核心库只链接Qt6::Core和Qt6::Gui（QVector2D等几何类型需要）
target_link_libraries(gobigger_core PUBLIC
    Qt6::Core
    Qt6::Gui
)

# 设置核心库的编译定义
target_compile_definitions(gobigger_core PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# ============ GUI应用 (ai-devour-evolve) ============
if(ENABLE_GUI)
    set(GUI_SOURCES
        src/main.cpp
        src/DemoQtVS.cpp
        src/GameView.cpp
        src/PlayerCell.cpp
        src/FoodItem.cpp
        # GUI渲染代理类
        src/BaseBall.cpp
        src/CloneBall.cpp
        src/FoodBall.cpp
        src/SporeBall.cpp
        src/ThornsBall.cpp
        src/GameManager.cpp
        src/QuadTree.cpp
    )

    set(GUI_HEADERS
        src/DemoQtVS.h
        src/GameView.h
        src/PlayerCell.h
        src/FoodItem.h
        # GUI渲染代理类
        src/BaseBall.h
        src/CloneBall.h
        src/FoodBall.h
        src/SporeBall.h
        src/ThornsBall.h
        src/GameManager.h
        src/QuadTree.h
    )

    set(UI_FILES
        src/DemoQtVS.ui
    )

    # 创建GUI应用
    add_executable(ai-devour-evolve
        WIN32
        ${GUI_SOURCES}
        ${GUI_HEADERS}
        ${UI_FILES}
    )

    # GUI应用的编译设置
    target_include_directories(ai-devour-evolve PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
    )

    # GUI应用链接到核心库和Qt Widgets
    target_link_libraries(ai-devour-evolve PRIVATE
        gobigger_core
        Qt6::Widgets
        "${TORCH_LIBRARIES}"
    )

    # GUI应用的编译定义
    target_compile_definitions(ai-devour-evolve PRIVATE
        QT_DISABLE_DEPRECATED_BEFORE=0x060000
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )
endif()

# ============ Python绑定模块 ============
if(ENABLE_PYTHON_BINDING)
    set(PYTHON_BINDING_SOURCES
        python/bindings.cpp
    )

    # 创建Python模块
    pybind11_add_module(gobigger_env ${PYTHON_BINDING_SOURCES})

    # Python模块链接到核心库
    target_link_libraries(gobigger_env PRIVATE
        gobigger_core
    )

    # Python模块的编译定义
    target_compile_definitions(gobigger_env PRIVATE
        VERSION_INFO="${PROJECT_VERSION}"
    )
endif()

# ============ 通用编译设置 ============
# Specify MSVC UTF-8 encoding   
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# Windows特定设置
if(WIN32)
    # 对所有目标应用的编译定义
    if(TARGET gobigger_core)
        target_compile_definitions(gobigger_core PRIVATE -D_GLIBCXX_USE_CXX11_ABI=0)
    endif()
    
    if(TARGET ai-devour-evolve)
        target_compile_definitions(ai-devour-evolve PRIVATE -D_GLIBCXX_USE_CXX11_ABI=0)
    endif()
    
    if(TARGET gobigger_env)
        target_compile_definitions(gobigger_env PRIVATE -D_GLIBCXX_USE_CXX11_ABI=0)
    endif()
    
    # MinGW特定设置
    if(MINGW)
        if(TARGET gobigger_core)
            target_compile_options(gobigger_core PRIVATE -Wno-deprecated-declarations)
        endif()
        if(TARGET ai-devour-evolve)
            target_compile_options(ai-devour-evolve PRIVATE -Wno-deprecated-declarations)
        endif()
    endif()
    
    # MSVC特定设置
    if(MSVC)
        if(TARGET gobigger_core)
            target_compile_options(gobigger_core PRIVATE /W3)
            set_property(TARGET gobigger_core PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif()
        if(TARGET ai-devour-evolve)
            target_compile_options(ai-devour-evolve PRIVATE /W3)
            set_property(TARGET ai-devour-evolve PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif()
    endif()
endif()

# ============ 安装规则 ============
if(TARGET gobigger_core)
    install(TARGETS gobigger_core
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(TARGET ai-devour-evolve)
    install(TARGETS ai-devour-evolve
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(TARGET gobigger_env)
    install(TARGETS gobigger_env
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

# ============ 配置信息 ============
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Qt found: ${Qt6_VERSION}")
message(STATUS "Torch found: ${Torch_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Enable GUI: ${ENABLE_GUI}")
message(STATUS "Enable Python Binding: ${ENABLE_PYTHON_BINDING}")

if(TARGET gobigger_core)
    message(STATUS "Target: gobigger_core (Core Logic Library)")
endif()
if(TARGET ai-devour-evolve)
    message(STATUS "Target: ai-devour-evolve (GUI Application)")
endif()
if(TARGET gobigger_env)
    message(STATUS "Target: gobigger_env (Python Module)")
endif()