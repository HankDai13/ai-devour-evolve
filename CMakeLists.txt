cmake_minimum_required(VERSION 3.5) # CMake install : https://cmake.org/download/
project(ai-devour-evolve LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_PREFIX_PATH "D:/qt/6.9.1/msvc2022_64;D:/tools/libtorch-cpu/libtorch") # Qt Kit Dir and LibTorch Dir
# set(CMAKE_PREFIX_PATH "D:/qt/6.9.1/mingw_64;D:/tools/libtorch") # Qt Kit Dir and LibTorch Dir
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

aux_source_directory(./src srcs)

# Specify MSVC UTF-8 encoding   
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(${PROJECT_NAME}
    WIN32 # If you need a terminal for debug, please comment this statement 
    ${srcs} 
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
endif()