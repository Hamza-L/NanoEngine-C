#include "NanoInput.h"

static float scroll = 0;
enum {max_key_buffer_size = 4};

static NanoKey key_stack[max_key_buffer_size];
static short int currentIndx;

void InitNanoInput(){
    for(int i = 0; i < max_key_buffer_size; i++){
        key_stack[i].key_id = -1;
        key_stack[i].key_state = -1;
    }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods){
    /* if (key == GLFW_KEY_W && action == GLFW_PRESS){ */
    /*     UP_PRESS = true; */
    /* } else if(key == GLFW_KEY_W && action == GLFW_RELEASE){ */
    /*     UP_PRESS = false; */
    /* } */
}

void mouse_callback(GLFWwindow *window, int button, int action, int mods){
    /* if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE){ */
    /*     MFLAG_R = true; */
    /* } */

    /* if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)){ */
    /*     MPRESS_M = true; */
    /* } else { */
    /*     MPRESS_M = false; */
    /* } */
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    scroll += 2.0f*yoffset;
}
