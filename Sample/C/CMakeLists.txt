﻿cmake_minimum_required(VERSION 3.0)
project(lx_camera_demo)

set(LIBS LxCameraApi)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (WIN32)
    if(CMAKE_CL_64) 
        set(PLATFORM_PREFEX win_x64)
    else()
        set(PLATFORM_PREFEX win_x86)
    endif()
	set(CAMERA_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../)
	include_directories(${CAMERA_BASE_PATH}/SDK/include)
	link_directories(${CAMERA_BASE_PATH}/SDK/lib/${PLATFORM_PREFEX})
elseif (UNIX)
	include_directories(/opt/Lanxin-MRDVS/include)  #将指定目录添加到编译器的头文件搜索目录下
	link_directories(/opt/Lanxin-MRDVS/lib/)# 指定第三方库目录
	list(APPEND LIBS ${LIBS} pthread) #将${LIBS} pthread库添加到LIBS中
endif ()
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin) #设置输出路径
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)#设置编译开启C++11的支持


#opencv
find_package(OpenCV)
if(${OpenCV_FOUND})
	message("find opencv " ${OpenCV_VERSION})
	include_directories(${OpenCV_INCLUDE_DIRS}) #设置包含目录
	link_directories(${OpenCV_LIB_DIR}) #设置链接库目录
	list(APPEND LIBS ${LIBS} ${OpenCV_LIBS})
	add_definitions(-DHAS_OPENCV) #添加编译宏定义
endif()
#使用宏定义
macro(add_exe NAME)  
add_executable(demo_${NAME} ${NAME}/${NAME}.cpp)  #添加源文件
target_link_libraries(demo_${NAME} ${LIBS})
endmacro()

add_exe(single_camera)
add_exe(single_camera2)
add_exe(multi_cameras)
add_exe(application_obstacle)
add_exe(application_pallet)
add_exe(frame_callback)

