#ifndef NANOWINDOW_H_
#define NANOWINDOW_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include "NanoError.hpp"

class NanoWindow {
  public:
    ERR Init(const int32_t width, const int32_t height, bool forceReInit);
    ERR Init();
    void PollEvents();
    bool ShouldWindowClose();
    ERR CleanUp();
    GLFWwindow* getGLFWwindow();

  private:
    bool m_isInit = false;
};

#endif // NANOWINDOW_H_
