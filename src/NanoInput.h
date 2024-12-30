#ifndef NANOINPUT_H_
#define NANOINPUT_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Str.h"

typedef enum {
    KEY_STATE_PRESSED,
    KEY_STATE_HELD,
    KEY_STATE_RELEASED,
    KEY_STATE_NONE,
} STATE;

typedef enum {
    KEY_MOD_SHIFT = 0x0001,
    KEY_MOD_CTRL = 0x0002,
    KEY_MOD_COMMAND = 0x0004,
    KEY_MOD_ALT = 0x0004,
    KEY_MOD_CAPS = 0x0010,
    KEY_MOD_NONE = 0x0000,
} MODKEY;

typedef struct{
    int key_id;
    STATE key_state;
    MODKEY key_mod;
} NanoKey;

void InitNanoInput();
void ResetNanoInput();
NanoKey PopMostRecentInputKey();
NanoKey PeekMostRecentInputKey();
String GetDroppedInFile();
void SetDroppedInFile(const char* fileName);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow *window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

#endif // NANOINPUT_H_
