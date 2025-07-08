// single_camera.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
// 这是一个使用蓝芯相机SDK的单相机示例程序，演示如何获取深度、强度和RGB图像数据

#include <iostream>
#include <thread>
#include <sstream>
#include "lx_camera_api.h"  // 蓝芯相机API头文件

#include "opencv2/opencv.hpp"  // OpenCV库，用于图像处理和显示
using namespace cv;

using namespace std;

// 全局变量定义
static char wait_key = '0';    // 用于接收键盘输入的字符
static DcHandle handle = 0;    // 相机设备句柄

// 显示控制和图像参数
static bool is_show = true;    // 是否显示图像窗口
static int tof_width = 0;      // TOF深度图像宽度
static int tof_height = 0;     // TOF深度图像高度
static int tof_depth_type = LX_DATA_TYPE::LX_DATA_UNSIGNED_SHORT;  // 深度数据类型，默认为16位无符号整数
static int tof_amp_type = LX_DATA_TYPE::LX_DATA_UNSIGNED_SHORT;    // 强度数据类型，默认为16位无符号整数
static int rgb_width = 0;      // RGB图像宽度
static int rgb_height = 0;     // RGB图像高度
static int rgb_channles = 3;   // RGB图像通道数，默认为3通道
static int rgb_data_type = LX_DATA_TYPE::LX_DATA_UNSIGNED_CHAR;    // RGB数据类型，默认为8位无符号字符

// 错误检查宏定义
// 用于检查API调用返回状态，如果失败则打印错误信息并退出程序
#define checkTC(state) {LX_STATE val=state;                            \
    if(val != LX_SUCCESS){                                             \
        std::cout << DcGetErrorString(val)<<std::endl;                  \
        if(val != LX_E_RECONNECTING && val != LX_E_NOT_SUPPORT){        \
            DcCloseDevice(handle);                                     \
            wait_key = getchar();                                      \
            return -1;                                                 \
        }                                                              \
    }                                                                  \
}

// 函数声明
int TestDepth(bool is_enable, DcHandle handle);  // 测试深度数据获取和显示
int TestAmp(bool is_enable, DcHandle handle);    // 测试强度数据获取和显示
int TestRgb(bool is_enable, DcHandle handle);    // 测试RGB数据获取和显示

// 等待键盘输入的线程函数
void WaitKey()
{
    printf("**********press 'q' to exit********\n\n");
    wait_key = getchar();  // 阻塞等待键盘输入
}

int main(int argc, char** argv)
{
    // 打印API版本信息
    std::cout << " call api version:" << DcGetApiVersion() << std::endl;

    // 查找相机设备
    int device_num = 0;                // 设备数量
    LxDeviceInfo* p_device_list = NULL; // 设备列表指针
    checkTC(DcGetDeviceList(&p_device_list, &device_num));  // 获取设备列表
    if (device_num <= 0)
    {
        std::cout << "not found any device, press any key to exit" << std::endl;
        wait_key = getchar();
        return -1;
    }
    std::cout << "DcGetDeviceList success list: " << device_num << std::endl;

    // 打开相机设备
    std::string open_param;  // 打开参数
    int open_mode = OPEN_BY_INDEX;  // 打开模式，默认按索引打开
    switch (open_mode)
    {
        // 根据IP地址打开设备
    case OPEN_BY_IP:
        open_param = "192.168.100.82";
        break;
        // 根据设备ID打开设备
    case OPEN_BY_ID:
        open_param = "F13301122647";
        break;
        // 根据序列号打开设备
    case OPEN_BY_SN:
        open_param = "ccf8981cc50b66b6";
        break;
        // 根据搜索设备列表索引打开设备（默认方式）
    case OPEN_BY_INDEX:
    default:
        open_param = "0";  // 打开第一个设备
        break;
    }

    // 尝试打开设备
    LxDeviceInfo device_info;  // 设备信息结构体
    LX_STATE lx_state = DcOpenDevice((LX_OPEN_MODE)open_mode, open_param.c_str(), &handle, &device_info);
    if (LX_SUCCESS != lx_state) {
        std::cout << "open device failed, open_mode:" << open_mode << " open_param:" << open_param << " press any key to exit" << std::endl;
        wait_key = getchar();
        return -1;
    }

    // 打印设备信息
    std::cout << "device_info\n cameraid:" << device_info.id << "\n uniqueid:" << handle
        << "\n cameraip:" << device_info.ip << "\n firmware_ver:" << device_info.firmware_ver << "\n sn:" << device_info.sn
        << "\n name:" << device_info.name << "\n img_algor_ver:" << device_info.algor_ver << std::endl;

    // 设置数据流开关
    bool test_depth = false, test_amp = false, test_rgb = false;  // 数据流状态标志
    checkTC(DcSetBoolValue(handle, LX_BOOL_ENABLE_3D_DEPTH_STREAM, true));  // 开启深度数据流
    checkTC(DcSetBoolValue(handle, LX_BOOL_ENABLE_3D_AMP_STREAM, true));    // 开启强度数据流
    checkTC(DcSetBoolValue(handle, LX_BOOL_ENABLE_2D_STREAM, true));        // 开启2D RGB数据流

    // 获取数据流开启状态
    checkTC(DcGetBoolValue(handle, LX_BOOL_ENABLE_3D_DEPTH_STREAM, &test_depth));
    checkTC(DcGetBoolValue(handle, LX_BOOL_ENABLE_3D_AMP_STREAM, &test_amp));
    checkTC(DcGetBoolValue(handle, LX_BOOL_ENABLE_2D_STREAM, &test_rgb));
    std::cout << "test_depth:" << test_depth << " test_amp:" << test_amp << " test_rgb:" << test_rgb << std::endl;

    // RGBD对齐设置
    // TOF的图像尺寸和像素会扩展到与RGB一致，开启后建议关闭强度流以节省带宽
    checkTC(DcSetBoolValue(handle, LX_BOOL_ENABLE_2D_TO_DEPTH, true));      // 开启RGBD对齐
    checkTC(DcSetBoolValue(handle, LX_BOOL_ENABLE_3D_AMP_STREAM, false));   // 关闭强度流

    // 获取图像参数
    // 注意：设置ROI、BINNING、RGBD对齐之后需要重新获取图像尺寸
    LxIntValueInfo int_value;  // 整数值信息结构体
    
    // 获取3D深度图像参数
    checkTC(DcGetIntValue(handle, LX_INT_3D_IMAGE_WIDTH, &int_value));
    tof_width = int_value.cur_value;
    checkTC(DcGetIntValue(handle, LX_INT_3D_IMAGE_HEIGHT, &int_value));
    tof_height = int_value.cur_value;
    checkTC(DcGetIntValue(handle, LX_INT_3D_DEPTH_DATA_TYPE, &int_value));
    tof_depth_type = int_value.cur_value;
    checkTC(DcGetIntValue(handle, LX_INT_3D_AMPLITUDE_DATA_TYPE, &int_value));
    tof_amp_type = int_value.cur_value;
    
    // 获取2D RGB图像参数
    checkTC(DcGetIntValue(handle, LX_INT_2D_IMAGE_WIDTH, &int_value));
    rgb_width = int_value.cur_value;
    checkTC(DcGetIntValue(handle, LX_INT_2D_IMAGE_HEIGHT, &int_value));
    rgb_height = int_value.cur_value;
    checkTC(DcGetIntValue(handle, LX_INT_2D_IMAGE_CHANNEL, &int_value));
    rgb_channles = int_value.cur_value;
    checkTC(DcGetIntValue(handle, LX_INT_2D_IMAGE_DATA_TYPE, &int_value));
    rgb_data_type = int_value.cur_value;

    // 开启帧同步模式
    // 可以根据需要决定是否开启帧同步模式，开启该模式内部会对每一帧做同步处理后返回
    // 默认若不需要TOF与RGB数据同步，则不需要开启此功能，内部会优先保证数据实时性
    checkTC(DcSetBoolValue(handle, LX_BOOL_ENABLE_SYNC_FRAME, true));

    // 开启数据流
    checkTC(DcStartStream(handle));
   // checkTC(DcSetBoolValue(handle, LX_BOOL_ENABLE_3D_AMP_STREAM, false));  // 可选：再次确认关闭强度流

    // 创建键盘输入监听线程
    std::thread pthread = std::thread(WaitKey);
    pthread.detach();  // 分离线程，让其在后台运行

    // 主循环：持续获取和处理图像数据
    auto _time = std::chrono::system_clock::now();  // 用于计算帧率的时间戳
    while (true)
    {
        // 更新数据 - 主动获取最新一帧数据
        auto ret = DcSetCmd(handle, LX_CMD_GET_NEW_FRAME);
        if ((LX_SUCCESS != ret)
            && (LX_E_FRAME_ID_NOT_MATCH != ret)      // 帧ID不匹配（正常情况）
            && (LX_E_FRAME_MULTI_MACHINE != ret))    // 多机干扰（正常情况）
        {
            if (LX_E_RECONNECTING == ret) {
                std::cout << "device reconnecting" << std::endl;  // 设备重连中
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));  // 等待1秒后重试
            continue;
        }

        // 计算并显示帧率（每秒更新一次）
        auto _time2 = std::chrono::system_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(_time2 - _time).count() > 1)
        {
            _time = std::chrono::system_clock::now();

            LxFloatValueInfo dep_float_info = { 0 };  // 深度帧率信息
            LxFloatValueInfo rgb_float_info = { 0 };  // RGB帧率信息
            std::stringstream ss;
            
            // 获取深度流帧率
            if (test_depth)
            {
                checkTC(DcGetFloatValue(handle, LX_FLOAT_3D_DEPTH_FPS, &dep_float_info));
                ss << "depth fps:" << dep_float_info.cur_value;
            }

            // 获取RGB流帧率
            if (test_rgb)
            {
                checkTC(DcGetFloatValue(handle, LX_FLOAT_2D_IMAGE_FPS, &rgb_float_info));
                ss << " rgb fps:" << rgb_float_info.cur_value;
            }

            std::cout << ss.str() << std::endl;
        }

        // 处理各种数据流
        TestDepth(test_depth, handle);  // 处理深度数据
        TestAmp(test_amp, handle);      // 处理强度数据
        TestRgb(test_rgb, handle);      // 处理RGB数据

        // 检查退出条件
        if (wait_key == 'q' || wait_key == 'Q')
            break;
    }

    // 清理资源
    DcStopStream(handle);   // 停止数据流
    DcCloseDevice(handle);  // 关闭设备

    return 0;
}

// 深度数据处理函数
int TestDepth(bool is_enable, DcHandle handle)
{
    if (!is_enable)  // 如果深度流未开启，直接返回
        return 0;

    // 获取深度数据指针
    void* data_ptr = nullptr;
    if (LX_SUCCESS != DcGetPtrValue(handle, LX_PTR_3D_DEPTH_DATA, &data_ptr))
        return -1;

    // 访问特定位置的深度数据示例
    // TOF相机深度数据为unsigned short类型，结构光相机（LX_DEVICE_H3）为float类型
    // 点云为float类型，xyz依次存储
    // LX_DATA_TYPE的定义与opencv CV_8U CV_16U CV_32F一致
    int yRow = 100, xCol = 100;  // 示例：访问第100行第100列的像素
    int pose = yRow * tof_width + xCol;  // 计算在一维数组中的位置
    
    if (LX_DATA_UNSIGNED_SHORT == tof_depth_type)
    {
        unsigned short* data = (unsigned short*)data_ptr;
        unsigned short value = data[pose];  // 获取深度值（毫米单位）
    }
    else if (LX_DATA_FLOAT == tof_depth_type)
    {
        float* data = (float*)data_ptr;
        float value = data[pose];  // 获取深度值（毫米单位）
    }

    // 获取对应的点云数据（3D坐标）
    float* xyz_data = nullptr;
    if (LX_SUCCESS != DcGetPtrValue(handle, LX_PTR_XYZ_DATA, (void**)&xyz_data))
        return -1;
    
    // 提取XYZ坐标（单位：毫米）
    float x = xyz_data[pose * 3];      // X坐标
    float y = xyz_data[pose * 3 + 1];  // Y坐标
    float z = xyz_data[pose * 3 + 2];  // Z坐标

    // 创建OpenCV Mat对象用于显示
    cv::Mat depth_image = cv::Mat(tof_height, tof_width, CV_MAKETYPE(tof_depth_type, 1), data_ptr);
    
    if (is_show)
    {
        cv::Mat depth_show;
        // 将深度图转换为8位图像用于显示（除以16进行缩放）
        depth_image.convertTo(depth_show, CV_8U, 1.0 / 16);
        cout<<"depth_show shape:"<<depth_show.type()<<" "<<depth_show.channels()<<" "<<depth_show.size()<<endl;
        
        // 创建窗口并显示深度图
        cv::namedWindow("depth", 0);
        cv::resizeWindow("depth", 640, 480);
        cv::imshow("depth", depth_show);
        cv::waitKey(1);  // 非阻塞等待，用于刷新显示
    }

    return 0;
}

// 强度数据处理函数
int TestAmp(bool is_enable, DcHandle handle)
{
    if (!is_enable)  // 如果强度流未开启，直接返回
        return 0;

    // 获取强度数据指针
    void* data_ptr = nullptr;
    if (LX_SUCCESS != DcGetPtrValue(handle, LX_PTR_3D_AMP_DATA, &data_ptr))
        return -1;
    
    std::cout << "get TestAmp data_ptr:"  << std::endl;
    
    // 访问特定位置的强度数据示例
    // TOF相机强度数据为unsigned short类型，结构光相机（LX_DEVICE_WK）为unsigned char类型
    // LX_DATA_TYPE的定义与opencv CV_8U CV_16U CV_32F一致
    int yRow = 100, xCol = 100;  // 示例：访问第100行第100列的像素
    int pose = yRow * tof_width + xCol;  // 计算在一维数组中的位置
    
    if (LX_DATA_UNSIGNED_SHORT == tof_amp_type)
    {
        unsigned short value = ((unsigned short*)data_ptr)[pose];  // 获取强度值
    }
    else if (LX_DATA_UNSIGNED_CHAR == tof_amp_type)
    {
        unsigned char value = ((unsigned char*)data_ptr)[pose];  // 获取强度值
    }

    // 创建OpenCV Mat对象用于显示
    cv::Mat amp_image = cv::Mat(tof_height, tof_width, CV_MAKETYPE(tof_amp_type, 1), data_ptr);
   
    cv::Mat amp_show;
    // 将强度图转换为8位图像用于显示（乘以0.25进行缩放）
    amp_image.convertTo(amp_show, CV_8U, 0.25);
    
    // 创建窗口并显示强度图
    cv::namedWindow("amptitude", 0);
    cv::resizeWindow("amptitude", 640, 480);
    cv::imshow("amptitude", amp_show);
    cv::waitKey(1);  // 非阻塞等待，用于刷新显示

    return 0;
}

// RGB数据处理函数
int TestRgb(bool is_enable, DcHandle handle)
{
    if (!is_enable)  // 如果RGB流未开启，直接返回
        return 0;

    // 获取RGB数据指针
    unsigned char* data_ptr = nullptr;
    if (LX_SUCCESS != DcGetPtrValue(handle, LX_PTR_2D_IMAGE_DATA, (void**)&data_ptr))
        return -1;

    // 访问特定位置的RGB数据示例
    // 2D图像目前只有unsigned char格式
    // LX_DATA_TYPE的定义与opencv CV_8U CV_16U CV_32F一致
    int yRow = 100, xCol = 100;  // 示例：访问第100行第100列的像素
    int pose = yRow * rgb_width + xCol;  // 计算在一维数组中的位置
    
    if (1 == rgb_channles)  // 灰度图像
    {
        unsigned char value = data_ptr[pose];  // 获取灰度值
    }
    else  // 彩色图像（RGB）
    {
        unsigned char r = data_ptr[pose * 3];      // 红色分量
        unsigned char g = data_ptr[pose * 3 + 1];  // 绿色分量
        unsigned char b = data_ptr[pose * 3 + 2];  // 蓝色分量
    }

    // 创建OpenCV Mat对象用于显示
    cv::Mat rgb_show = cv::Mat(rgb_height, rgb_width, CV_MAKETYPE(rgb_data_type, rgb_channles), data_ptr);
    
    // 可选：颜色空间转换（如果需要从RGB转换为BGR）
    // cv::Mat rgbImg;
    // cv::cvtColor(rgb_show, rgbImg, cv::COLOR_RGB2BGR);
    // cv::imshow("rgb1", rgbImg);

    // 显示RGB图像
    if (is_show) {
        cv::namedWindow("rgb", 0);
        cv::resizeWindow("rgb", 640, 480);
        cv::imshow("rgb", rgb_show);
        cv::waitKey(1);  // 非阻塞等待，用于刷新显示
    }

    return 0;
}