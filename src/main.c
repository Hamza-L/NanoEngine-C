#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "NanoEngine.h"
#include "NanoShader.h"
#include "Str.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string.h>

#include "NanoConfig.h"

void getRequiredInstanceExtensions(InstanceExtension* requiredInstanceExtensions, uint32_t* numOfInstanceExtensions){
    const char** glfwExtensions;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, APP_NAME, NULL, NULL);

    glfwExtensions = glfwGetRequiredInstanceExtensions(numOfInstanceExtensions);
    /* printf("num of instance extensions: %d", *numOfInstanceExtensions); */

    // additional instance extension we may want to add
    // 64 additional extensions that we can potentially add
    int extIdx = 0;
    while (desiredInstanceExtensions[extIdx]){
        strcpy(requiredInstanceExtensions[extIdx].extensionName, desiredInstanceExtensions[extIdx]);
        extIdx++;
    }

    for (uint32_t i = 0; i < *numOfInstanceExtensions; i++) {
        strcpy(requiredInstanceExtensions[extIdx].extensionName, glfwExtensions[i]);
        extIdx++;
    }

    if (enableValidationLayers) {
        strcpy(requiredInstanceExtensions[extIdx].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        extIdx++;
    }

    *numOfInstanceExtensions = extIdx;

    // ----------------------------------------
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main(int argc, char *argv[]) {
    /* NanoShader nanoShader; */
    /* InitShader(&nanoShader, "./src/shader/shader.vert"); */
    /* CompileShader(nullptr, &nanoShader, true); */

    String testString;
    HeapString testHeapString;
    InitString(&testString, "Hello");
    InitHeapString(&testHeapString, "Hello");

    AppendToString(&testString, " World!\n");
    AppendToHeapString(&testHeapString, " World!\n");
    return EXIT_SUCCESS;
}
