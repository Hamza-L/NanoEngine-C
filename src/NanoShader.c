#include "NanoShader.h"
#include "NanoConfig.h"
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

    // String op to get the resulting spv path from the input shader file
    int exitCode = 1;
    String outputFile = CreateString("./src/shader/");

    if(FindRawString(shaderToCompile->config.m_fileFullPath, ".vert") >= 0){
        AppendToString(&outputFile, "vert");
    } else if (FindRawString(shaderToCompile->config.m_fileFullPath, ".frag") >= 0){
        AppendToString(&outputFile, "frag");
    } else if (FindRawString(shaderToCompile->config.m_fileFullPath, ".comp") >= 0){
        AppendToString(&outputFile, "comp");
    }
    AppendToString(&outputFile, "_");

    int startIndx = FindLastRawString(shaderToCompile->config.m_fileFullPath, "/") + 1; //we don't want to include the "/"
    int endIndx = FindLastRawString(shaderToCompile->config.m_fileFullPath, ".");

    String filename;
    InitString(&filename, shaderToCompile->config.m_fileFullPath);
    SubString(&filename, startIndx, endIndx-startIndx);

    AppendToString(&outputFile, filename.m_data);
    AppendToString(&outputFile, ".spv");

    if(!forceCompile){
      if ((file = fopen(outputFile.m_data, "rb")) != NULL) {
        LOG_MSG(stderr, "compiled shader found! ForceCompile is not enabled so no need to compile\n");
        if(fclose(file) == EOF){
          LOG_MSG(stderr, "failed to close the file!\n");
        }
        compileNeeded = false;
        exitCode = 0;
      }
    }

    if(compileNeeded){
      String cmdArgument = CreateString(" ");
      AppendToString(&cmdArgument, shaderToCompile->config.m_fileFullPath);
      AppendToString(&cmdArgument, " -g ");
      AppendToString(&cmdArgument, " -o ");
      AppendToString(&cmdArgument, outputFile.m_data);

#ifdef __APPLE__
      const char* executable = "./external/VULKAN/mac/glslc";
#elif _WIN32
      const char* executable = "./external/VULKAN/win/glslc.exe";
#else
      const char* executable = "./external/VULKAN/linux/glslc";
#endif
      exitCode = RunGLSLCompiler(executable,
                                 shaderToCompile->config.m_fileFullPath,
                                 outputFile.m_data,
                                 &shaderToCompile->config.m_fileFullPath[startIndx]);
    }

    if(!exitCode){
      shaderToCompile->m_isCompiled = true;
      /* LOG_MSG(stderr, "reading raw shader code from: %s\n", outputFile.m_data); */
      shaderToCompile->m_rawShaderCode = ReadBinaryFile(outputFile.m_data, &shaderToCompile->m_rawShaderCodeSize);
      shaderToCompile->m_shaderModule = CreateShaderModule(nanoRenderer->m_pNanoContext->device, shaderToCompile);
    } else {
      shaderToCompile->m_isCompiled = false;
    }

    return exitCode;
}
