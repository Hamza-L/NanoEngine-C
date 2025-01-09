cmake_minimum_required(VERSION 3.17)

# must be set before project()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_BUILD_TYPE "Debug")

if (WIN32)
    set(CMAKE_VS_PLATFORM_NAME "x64")
elseif (APPLE)
    set (MACOSX_DEPLOYMENT_TARGET "10.15" CACHE INTERNAL "")

    if (UniversalBinary)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")
        #set(CMAKE_OSX_ARCHITECTURES arm64 x86_64)
    endif()
endif()

project(NanoEngine)
set(PROJECT_NAME NanoEngine)
set(ROOT_NAMESPACE NanoEngine)
set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")

################################################################################zZ
# Source groups
################################################################################
set(Headers
    "src/NanoEngine.h"
    "src/MemManager.h"
    "src/NanoBuffers.h"
    "src/NanoImage.h"
    "src/NanoWindow.h"
    "src/NanoCamera.h"
    "src/NanoUi.h"
    "src/NanoInput.h"
    "src/NanoRenderer.h"
    "src/NanoGraphicsPipeline.h"
    "src/NanoScene.h"
    "src/NanoError.h"
    "src/NanoUtility.h"
    "src/NanoVkUtility.h"
    "src/NanoConfig.h"
    "src/NanoShader.h"
    "src/Str.h"
)

source_group("Headers" FILES ${Headers})

set(Sources
    "src/NanoEngine.c"
    "src/MemManager.c"
    "src/NanoBuffers.c"
    "src/NanoImage.c"
    "src/NanoWindow.c"
    "src/NanoCamera.c"
    "src/NanoUi.c"
    "src/NanoInput.c"
    "src/NanoRenderer.c"
    "src/NanoGraphicsPipeline.c"
    "src/NanoScene.c"
    "src/NanoError.c"
    "src/NanoUtility.c"
    "src/NanoVkUtility.c"
    "src/NanoConfig.c"
    "src/NanoShader.c"
    "src/Str.c"
    "src/main.c"
)

source_group("Sources" FILES ${Sources})

set(ALL_FILES
    ${Headers}
    ${Sources})

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

################################################################################
# Include directories and Dependencies
################################################################################

if(WIN32)
    message("Starting the windows Build")

target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/CGLM/win/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/GFLW/win/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/FREETYPE/win/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/JSMN"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/STB_IMAGE"
        "$ENV{VULKAN_SDK}/Include")

    target_link_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/GFLW/win/lib-vc2022"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/CGLM/win/lib/${CMAKE_BUILD_TYPE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/FREETYPE/win/lib/${CMAKE_BUILD_TYPE}"
        "$ENV{VULKAN_SDK}/Lib")

    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "glfw3.lib"
        "vulkan-1.lib"
        "freetype.lib"
        "cglm.lib")

    #only remove deprecation warning for NanoRenderer (no warning for using strcpy_s)
    set_source_files_properties("src/NanoRenderer.c" PROPERTIES COMPILE_FLAGS -Wno-deprecated-declarations)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

elseif(APPLE)
    message("Starting the Macos Build")

    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/CGLM/mac/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/GFLW/mac/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/FREETYPE/mac/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/JSMN"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/STB_IMAGE"
        "~/VulkanSDK/1.3.290.0/macOS/include")

    target_link_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/GFLW/mac/lib-arm64"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/CGLM/mac/lib/${CMAKE_BUILD_TYPE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/FREETYPE/mac/lib/${CMAKE_BUILD_TYPE}"
        "~/VulkanSDK/1.3.290.0/macOS/lib")

    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "libcglm.a"
        "libfreetype.a"
        "libglfw3.a"
        "libvulkan.dylib"
        "-framework Cocoa" # From statically build glfw. need to manually specify the dependency
        "-framework IOKit" # From statically build glfw. need to manually specify the dependency
    )
elseif(LINUX)
    message("Starting the Linux Build")

target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/GLM"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/GFLW/linux/include")

    target_link_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/GFLW/linux/lib")

    set(ADDITIONAL_LIBRARY_DEPENDENCIES
            "libglfw3.a")
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/external/windows/assimp/dll/assimp-vc143-mt.dll
#         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# file(COPY ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
#         DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})
