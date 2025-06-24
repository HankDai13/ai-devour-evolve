#include <torch/torch.h>
#include <iostream>

int main() {
    std::cout << "LibTorch version: " << TORCH_VERSION_MAJOR << "." 
              << TORCH_VERSION_MINOR << "." << TORCH_VERSION_PATCH << std::endl;
    
    // 创建一个简单的张量
    torch::Tensor tensor = torch::rand({2, 3});
    std::cout << "Random tensor:\n" << tensor << std::endl;
    
    return 0;
}
