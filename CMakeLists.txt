cmake_minimum_required(VERSION 3.5)
project(ai-devour-evolve LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

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

# 查找 Qt6 组件
find_package(Qt6 COMPONENTS Core Gui Widgets Network REQUIRED)
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

# 源文件与头文件列表（按你的完整实现文件名列表）
set(SOURCES
    src/main.cpp
    src/Physics.cpp
    src/Utils.cpp
    
    src/PlayerBall.cpp
    src/Player.cpp
    src/Food.cpp
    src/Spore.cpp
    src/Thorn.cpp
    src/ScoreManager.cpp
    src/GameController.cpp
    src/HumanController.cpp
    src/AIController.cpp
    src/GameWindow.cpp
    src/StartScreen.cpp
    src/GameOverScreen.cpp
)

set(HEADERS
    src/Physics.h
    src/Utils.h
    src/QuadTree.h
    src/QuadTree_impl.h
    src/PlayerBall.h
    src/Player.h
    src/Food.h
    src/Spore.h
    src/Thorn.h
    src/ScoreManager.h
    src/GameController.h
    src/HumanController.h
    src/AIController.h
    src/GameWindow.h
    src/StartScreen.h
    src/GameOverScreen.h
)

set(UI_FILES
    # 如果有Qt Designer生成的.ui文件，可以在这里添加
)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(${PROJECT_NAME}
    WIN32
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
 "src/Ball.h" "src/Ball.cpp")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    "${TORCH_LIBRARIES}"
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -D_GLIBCXX_USE_CXX11_ABI=0)
    if(MINGW)
        target_compile_options(${PROJECT_NAME} PRIVATE -Wno-deprecated-declarations)
    endif()
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W3)
        set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Qt found: ${Qt6_VERSION}")
message(STATUS "Torch found: ${Torch_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")