#include <bits/stdc++.h>
#include <sys/ioctl.h>
#include <unistd.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace std;

// 获取终端尺寸
pair<unsigned, unsigned> get_terminal_size() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0 && w.ws_row > 0) {
        return {w.ws_col, w.ws_row};
    }
    // 默认值（80x24）
    return {80, 24};
}

// 双线性插值函数
tuple<unsigned char, unsigned char, unsigned char, unsigned char> 
bilinear_interpolation(const unsigned char* image, 
                      unsigned width, unsigned height,
                      double x, double y, int channels) {
    // 计算整数坐标
    unsigned x0 = static_cast<unsigned>(floor(x));
    unsigned y0 = static_cast<unsigned>(floor(y));
    unsigned x1 = min(x0 + 1, width - 1);
    unsigned y1 = min(y0 + 1, height - 1);
    
    double dx = x - x0;
    double dy = y - y0;
    double dx_inv = 1.0 - dx;
    double dy_inv = 1.0 - dy;
    
    // 获取四个角点的索引
    unsigned idx00 = channels * (y0 * width + x0);
    unsigned idx01 = channels * (y1 * width + x0);
    unsigned idx10 = channels * (y0 * width + x1);
    unsigned idx11 = channels * (y1 * width + x1);
    
    // 确保索引在范围内
    unsigned max_index = channels * width * height;
    if (idx00 >= max_index || idx11 + channels - 1 >= max_index) {
        return {0, 0, 0, 255};
    }
    
    // 对每个通道进行插值
    auto interpolate = [&](unsigned idx0, unsigned idx1, unsigned idx2, unsigned idx3) {
        double v00 = image[idx0];
        double v01 = image[idx1];
        double v10 = image[idx2];
        double v11 = image[idx3];
        
        return static_cast<unsigned char>(
            v00 * dx_inv * dy_inv + 
            v10 * dx * dy_inv + 
            v01 * dx_inv * dy + 
            v11 * dx * dy
        );
    };
    
    // 插值计算每个通道
    unsigned char r = interpolate(idx00,   idx01,   idx10,   idx11);
    unsigned char g = interpolate(idx00+1, idx01+1, idx10+1, idx11+1);
    unsigned char b = interpolate(idx00+2, idx01+2, idx10+2, idx11+2);
    
    // 处理Alpha通道（如果有）
    unsigned char a = 255;
    if (channels == 4) {
        a = interpolate(idx00+3, idx01+3, idx10+3, idx11+3);
    }
    
    return {r, g, b, a};
}

// 支持的格式列表
const vector<string> SUPPORTED_FORMATS = {
    "png", "jpg", "jpeg", "bmp", "tga", "psd", "gif", "hdr", "pic", "pnm"
};

// 检查文件扩展名是否支持
bool is_supported_format(const string& filename) {
    // 提取文件扩展名
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == string::npos) return false;
    
    string ext = filename.substr(dot_pos + 1);
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // 检查是否在支持的格式列表中
    return find(SUPPORTED_FORMATS.begin(), SUPPORTED_FORMATS.end(), ext) != SUPPORTED_FORMATS.end();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "用法: " << argv[0] << " <图像文件>" << endl;
        cerr << "示例: " << argv[0] << " image.png" << endl;
        cerr << "支持格式: ";
        for (const auto& fmt : SUPPORTED_FORMATS) cerr << fmt << " ";
        cerr << endl;
        return 1;
    }

    const char* filename = argv[1];
    
    // 检查文件格式支持
    if (!is_supported_format(filename)) {
        cerr << "错误: 不支持的图像格式!" << endl;
        cerr << "支持的格式: ";
        for (const auto& fmt : SUPPORTED_FORMATS) cerr << fmt << " ";
        cerr << endl;
        return 1;
    }

    // 加载图像
    int width, height, channels;
    unsigned char* image_data = stbi_load(filename, &width, &height, &channels, 0);
    
    if (!image_data) {
        cerr << "错误: 无法加载图像 (" << stbi_failure_reason() << ")" << endl;
        return 1;
    }

    // 获取终端尺寸
    auto [term_cols, term_rows] = get_terminal_size();
    
    // 计算终端可用高度（保留4行用于信息显示）
    unsigned available_rows = term_rows > 4 ? term_rows - 4 : term_rows;
    
    // 计算缩放比例 - 考虑字符宽高比（1:2）
    double aspect_ratio = static_cast<double>(width) / height;
    
    // 计算目标高度和宽度
    unsigned target_height = min(available_rows, static_cast<unsigned>(height / 2));
    unsigned target_width = static_cast<unsigned>(target_height * aspect_ratio * 2);
    
    // 如果计算出的宽度超过终端宽度，则重新计算
    if (target_width > term_cols) {
        target_width = term_cols;
        target_height = static_cast<unsigned>(target_width / (aspect_ratio * 2));
    }
    
    // 确保最小尺寸
    if (target_width < 1) target_width = 1;
    if (target_height < 1) target_height = 1;
    
    // 清屏并隐藏光标
    printf("\033[2J\033[H\033[?25l");
    
    // 显示图像信息
    cout << "图像: " << filename << "\n";
    cout << "尺寸: " << width << "×" << height << " 像素\n";
    cout << "通道: " << channels << " (";
    if (channels == 1) cout << "灰度";
    else if (channels == 2) cout << "灰度+Alpha";
    else if (channels == 3) cout << "RGB";
    else if (channels == 4) cout << "RGBA";
    cout << ")\n";
    cout << "终端: " << term_cols << "×" << term_rows << " 字符\n";
    cout << "目标: " << target_width << "×" << target_height << " 字符\n";
    cout << "比例: " << fixed << setprecision(2) << aspect_ratio << "\n\n";

    // 使用双线性插值生成缩放后的图像
    for (unsigned y = 0; y < target_height; y++) {
        for (unsigned x = 0; x < target_width; x++) {
            // 计算原始图像中的对应位置（浮点坐标）
            double src_x = (x + 0.5) * (width / static_cast<double>(target_width));
            double src_y = (y + 0.5) * (height / static_cast<double>(target_height));
            
            // 使用双线性插值获取像素值
            auto [r, g, b, a] = bilinear_interpolation(image_data, width, height, src_x, src_y, channels);
            
            // 透明像素处理（与黑色背景混合）
            if (a < 255) {
                // 假设背景是黑色
                double alpha = a / 255.0;
                r = static_cast<unsigned char>(r * alpha);
                g = static_cast<unsigned char>(g * alpha);
                b = static_cast<unsigned char>(b * alpha);
            }
            
            // 使用单个空格输出彩色块
            printf("\x1b[48;2;%d;%d;%dm \x1b[0m", r, g, b);
        }
        printf("\n");
    }
    
    // 恢复光标显示
    printf("\033[?25h");
    
    // 释放图像内存
    stbi_image_free(image_data);
    
    return 0;
}