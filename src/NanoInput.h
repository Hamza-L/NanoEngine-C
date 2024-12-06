#ifndef NANOINPUT_H_
#define NANOINPUT_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef enum {
    PRESSED,
    HELD,
    RELEASED,
    NONE,
} STATE;

typedef struct{
    int key_id;
    STATE key_state;
} NanoKey;

void InitNanoInput();
void ResetNanoInput();
void PopMostRecentInputKey();
void PeekMostRecentInputKey();
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow *window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

#endif // NANOINPUT_H_
