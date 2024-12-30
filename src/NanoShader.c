#include "NanoShader.h"
#include "NanoConfig.h"
#include "NanoError.h"
#include "NanoRenderer.h"
#include "NanoUtility.h"
#include "NanoBuffers.h"
#include "Str.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN64
#include <windows.h>
#elif __APPLE__
#include <unistd.h>
#endif

void CleanUpShader(NanoRenderer* nanoRenderer, NanoShader* shaderToCleanUp){
    vkDestroyShaderModule(nanoRenderer->m_pNanoContext->device, shaderToCleanUp->m_shaderModule, NULL);
}

static VkShaderModule CreateShaderModule(VkDevice device, NanoShader* shader) {
  VkShaderModule shaderModule = {};
  if(!shader->m_isCompiled){
      LOG_MSG(stderr, "Attempting to create a shader module from uncompiled shader\n");
      return shaderModule; // failed the re-attempt at compiling
  }

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = shader->m_rawShaderCodeSize;
  createInfo.pCode = (const uint32_t*)shader->m_rawShaderCode;

  if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
    LOG_MSG(stderr, "failed to create shader module!\n");
  }

  return shaderModule;
}

#ifdef _WIN64
int RunGLSLCompiler(const char* lpApplicationName, char const* fileName, const char* outputFileName, const char* shaderName)
{
   LPCTSTR executable = lpApplicationName;
   // additional information
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   String command = CreateString("glslc ");
   AppendToString(&command, fileName);
   AppendToString(&command, " -o ");
   AppendToString(&command, outputFileName);
   /* command.append(fileName); */
   /* command.append(" -o "); */
   /* command.append(outputFileName); */

   // set the size of the structures
   ZeroMemory( &si, sizeof(si) );
   si.cb = sizeof(si);
   ZeroMemory( &pi, sizeof(pi) );

  // start the program up
  bool test = CreateProcess( executable,   // the path
    command.m_data,           // Command line
    NULL,           // Process handle not inheritable
    NULL,           // Thread handle not inheritable
    FALSE,          // Set handle inheritance to FALSE
    0,              // No creation flags
    NULL,           // Use parent's environment block
    NULL,           // Use parent's starting directory
    &si,            // Pointer to STARTUPINFO structure
    &pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
    );

  // Wait until child process exits.
  LOG_MSG(stderr, "compiling : %s\n", fileName);
  WaitForSingleObject( pi.hProcess, INFINITE );

  DWORD exit_code;
  GetExitCodeProcess(pi.hProcess, &exit_code);

  int compilerExitCode = (int)exit_code;
  LOG_MSG(stderr, "finished compiling: %s\t with exit code: %d\n", shaderName, compilerExitCode);

  // Close process and thread handles.
  CloseHandle( pi.hProcess );
  CloseHandle( pi.hThread );

  return compilerExitCode;
}

#elif __APPLE__
int RunGLSLCompiler(const char* lpApplicationName, char const* fileName, const char* outputFileName, const char* shaderName)
{
  int err = 0;
  int status;
  pid_t pid;

  LOG_MSG(stderr, "compiling : %s\n", shaderName);

  pid = fork();
  if(pid == -1){
    LOG_MSG(stderr, "error pid\n");
    exit(EXIT_FAILURE);
  }else if(pid == 0){
    err = execl(lpApplicationName, "", fileName, "-o", outputFileName, (char *)0);
    /* LOG_MSG(stderr, "lpApplicationName: %s\n", lpApplicationName); */
    /* LOG_MSG(stderr, "fileName: %s\n", fileName); */
    /* LOG_MSG(stderr, "outputFileName: %s\n", outputFileName); */
    LOG_MSG(stderr, "finished compiling: %s\t with exit code: %d\n", shaderName, err);
    exit(0);
  }else{
    if(waitpid(pid, &status, 0) > 0){
      if (WIFEXITED(status)){
        if(!WEXITSTATUS(status)){
          //LOG_MSG(ERRLevel::INFO, "glslc ran with no issues");
          LOG_MSG(stderr, "glslc exited with no error\n");
        } else if ( WEXITSTATUS(status) == 127){
          LOG_MSG(stderr, "glslc exited with error 127\n");
        }
      } else {
        LOG_MSG(stderr, "process did not exit normally\n");
      }
    } else {
        LOG_MSG(stderr, "error occured with waitpid\n");
    }
  }
    return err;
}
#endif

void InitShader(NanoRenderer* nanoRenderer, NanoShader* shaderToInitialize, NanoShaderConfig config){
    shaderToInitialize->config = config;
    shaderToInitialize->m_rawShaderCodeSize = 0;
    shaderToInitialize->m_isCompiled = false;
    shaderToInitialize->m_shaderModule = VK_NULL_HANDLE;

}

int CompileShader(NanoRenderer* nanoRenderer, NanoShader* shaderToCompile, bool forceCompile){

    FILE* file;
    bool compileNeeded = forceCompile;
    bool sourceFileExists = IsFileInPath(shaderToCompile->config.m_sourcefileFullPath.m_data, "source file not found");
    bool binaryFileExists = IsFileInPath(shaderToCompile->config.m_binaryfileFullPath.m_data, "binary file not found");

    // String op to get the resulting spv path from the input shader file
    int exitCode = 0;
    int startIndx = FindLastRawString(shaderToCompile->config.m_sourcefileFullPath.m_data, "/") + 1; //we don't want to include the "/"
    int endIndx = FindLastRawString(shaderToCompile->config.m_sourcefileFullPath.m_data, ".");

    String filename;
    InitString(&filename, shaderToCompile->config.m_sourcefileFullPath.m_data);
    SubString(&filename, startIndx, endIndx-startIndx);

    compileNeeded = (!binaryFileExists || forceCompile);

    if(compileNeeded){
          if(!sourceFileExists){
              LOG_MSG(stderr, "source file not found. cannot compile\n");
              exitCode = -1;
          } else {
              String cmdArgument = CreateString(" ");
              AppendToString(&cmdArgument, shaderToCompile->config.m_sourcefileFullPath.m_data);
              AppendToString(&cmdArgument, " -g ");
              AppendToString(&cmdArgument, " -o ");
              AppendToString(&cmdArgument, shaderToCompile->config.m_sourcefileFullPath.m_data);

              String glslcExe = CreateString("");

#ifdef __APPLE__
              AppendToString(&glslcExe, "./external/VULKAN/mac/glslc");
#elif _WIN32
              AppendToString(&glslcExe, "./external/VULKAN/win/glslc.exe");
#else
              AppendToString(&glslcExe, "./external/VULKAN/linux/glslc");
#endif
              exitCode = RunGLSLCompiler(glslcExe.m_data,
                                         shaderToCompile->config.m_sourcefileFullPath.m_data,
                                         shaderToCompile->config.m_binaryfileFullPath.m_data,
                                         &shaderToCompile->config.m_sourcefileFullPath.m_data[startIndx]);
          }
    }

    if(!exitCode){
        shaderToCompile->m_isCompiled = true;
        /* LOG_MSG(stderr, "reading raw shader code from: %s\n", shaderToCompile->config.m_binaryfileFullPath.m_data); */
        shaderToCompile->m_rawShaderCode = ReadBinaryFile(shaderToCompile->config.m_binaryfileFullPath.m_data, &shaderToCompile->m_rawShaderCodeSize);
        shaderToCompile->m_shaderModule = CreateShaderModule(nanoRenderer->m_pNanoContext->device, shaderToCompile);
    } else {
        shaderToCompile->m_isCompiled = false;
    }

    return exitCode;
}
