#include "NanoInput.h"
#include <stdio.h>

static float scroll = 0;
enum {max_key_buffer_size = 4};

static NanoKey key_stack[max_key_buffer_size];
static short int currentIndx;

void InitNanoInput(){
    for(int i = 0; i < max_key_buffer_size; i++){
        key_stack[i].key_id = -1;
        key_stack[i].key_state = -1;
        key_stack[i].key_mod = -1;
    }
    currentIndx = 0;
}

void ResetNanoInput(){
    InitNanoInput();
};

NanoKey PopMostRecentInputKey(){
    NanoKey keyToReturn = key_stack[currentIndx];
    if(keyToReturn.key_id == -1) {
        return keyToReturn;
    }
    key_stack[currentIndx].key_id = -1;
    key_stack[currentIndx].key_mod = -1;
    key_stack[currentIndx].key_state = -1;
    currentIndx--;
    currentIndx += currentIndx < 0 ? max_key_buffer_size : 0; // add max_key_buffer_size if the currentIndx is negative
    return keyToReturn;
}

NanoKey PeekMostRecentInputKey(){
    NanoKey keyToReturn = {};
    keyToReturn = key_stack[currentIndx];
    return keyToReturn;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods){
    if(action == GLFW_PRESS){
        printf("key pressed: %d\n", key);
        printf("key pressed: %d\n", mods);

        if(key_stack[currentIndx].key_id != key){
            currentIndx = (currentIndx + 1) % max_key_buffer_size;
        }

        key_stack[currentIndx].key_mod = mods;
        key_stack[currentIndx].key_id = key;
        key_stack[currentIndx].key_state = KEY_STATE_PRESSED;
    }
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
