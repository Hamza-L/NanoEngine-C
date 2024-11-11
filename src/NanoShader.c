#include "NanoShader.h"
#include "NanoGraphics.h"
#include "NanoError.h"
#include "NanoUtility.h"
#include <StrUtil.h>

#include <stdio.h>
#include <string.h>

#ifdef _WIN64
#include <windows.h>
#elif __APPLE__
#include <unistd.h>
#endif


void CleanUpShader(NanoGraphics* nanoGraphics, NanoShader* shaderToCleanUp){
    vkDestroyShaderModule(nanoGraphics->m_pNanoContext->device, shaderToCleanUp->m_shaderModule, NULL);
}

static VkShaderModule CreateShaderModule(VkDevice device, NanoShader* shader) {
  VkShaderModule shaderModule = {};
  if(!shader->m_isCompiled){
      fprintf(stderr, "Attempting to create a shader module from uncompiled shader");
      return shaderModule; // failed the re-attempt at compiling
  }

  VkShaderModuleCreateInfo createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = shader->m_rawShaderCodeSize;
  createInfo.pCode = (const uint32_t*)shader->m_rawShaderCode;

  if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
    fprintf(stderr, "failed to create shader module!");
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

   std::string command = "glslc ";
   command.append(fileName);
   command.append(" -o ");
   command.append(outputFileName);

   // set the size of the structures
   ZeroMemory( &si, sizeof(si) );
   si.cb = sizeof(si);
   ZeroMemory( &pi, sizeof(pi) );

  // start the program up
  bool test = CreateProcess( executable,   // the path
    const_cast<char*>(command.c_str()),           // Command line
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
  LOG_MSG(INFO, "compiling : %s", fileName);
  WaitForSingleObject( pi.hProcess, INFINITE );

  DWORD exit_code;
  GetExitCodeProcess(pi.hProcess, &exit_code);

  int compilerExitCode = (int)exit_code;
  LOG_MSG(ERRLevel::INFO, "finished compiling: %s\t with exit code: %d", shaderName, compilerExitCode);

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

  fprintf(stderr, "compiling : %s\n", shaderName);

  pid = fork();
  if(pid == -1){
    fprintf(stderr, "error pid\n");
    exit(EXIT_FAILURE);
  }else if(pid == 0){
    err = execl(lpApplicationName, "", fileName, "-o", outputFileName, (char *)0);
    /* fprintf(stderr, "lpApplicationName: %s\n", lpApplicationName); */
    /* fprintf(stderr, "fileName: %s\n", fileName); */
    /* fprintf(stderr, "outputFileName: %s\n", outputFileName); */
    fprintf(stderr, "finished compiling: %s\t with exit code: %d\n", shaderName, err);
    exit(0);
  }else{
    if(waitpid(pid, &status, 0) > 0){
      if (WIFEXITED(status) && !WEXITSTATUS(status) && WEXITSTATUS(status) != 127){
        //LOG_MSG(ERRLevel::INFO, "glslc ran with no issues");
      } else{
        fprintf(stderr, "glslc exit with error\n");
      }
    } else {
        fprintf(stderr, "error occured with waitpid");
    }
  }
  return err;
}
#endif


void InitShader(NanoShader* shaderToInitialize, const char* shaderCodeFile){
    AppendString(shaderToInitialize->m_fileFullPath, shaderCodeFile);
}

int CompileShader(NanoGraphics* nanoGraphics, NanoShader* shaderToCompile, bool forceCompile){
    int exitCode = 1;
    char outputFile[MAX_FILEPATH_LENGTH] = "./src/shader/";

    if(FindString(shaderToCompile->m_fileFullPath, ".vert") >= 0){
        AppendString(outputFile, "vert");
    } else if (FindString(shaderToCompile->m_fileFullPath, ".frag") >= 0){
        AppendString(outputFile, "frag");
    } else if (FindString(shaderToCompile->m_fileFullPath, ".comp") >= 0){
        AppendString(outputFile, "comp");
    }
    AppendString(outputFile, "_");

    int startIndx = FindLastString(shaderToCompile->m_fileFullPath, "/") + 1; //we don't want to include the "/"
    int endIndx = FindLastString(shaderToCompile->m_fileFullPath, ".");

    char filename[256] = {};
    strcpy(filename, shaderToCompile->m_fileFullPath);
    SubString(filename, startIndx, endIndx-startIndx);

    AppendString(outputFile, filename);
    AppendString(outputFile, ".spv");

    char cmdArgument[256] = " ";
    AppendString(cmdArgument, filename);
    AppendString(cmdArgument, shaderToCompile->m_fileFullPath);
    AppendString(cmdArgument, " -o ");
    AppendString(cmdArgument, outputFile);

    char const *argv[] = { shaderToCompile->m_fileFullPath, "-o", outputFile};
#ifdef __APPLE__
    const char* executable = "./external/VULKAN/mac/glslc";
#elif _WIN32
    const char* executable = "./external/VULKAN/win/glslc.exe";
#else
    const char* executable = "./external/VULKAN/linux/glslc";
#endif
    exitCode = RunGLSLCompiler(executable, shaderToCompile->m_fileFullPath, outputFile, &shaderToCompile->m_fileFullPath[startIndx]);

    if(!exitCode){
      fprintf(stdout, "Successfully compiled\n");
      shaderToCompile->m_isCompiled = true;
      fprintf(stdout, "reading raw shader code from: %s\n", outputFile);
      shaderToCompile->m_rawShaderCode = ReadBinaryFile(outputFile);
      //shaderToCompile->m_shaderModule = CreateShaderModule(nanoGraphics->m_pNanoContext->device, shaderToCompile);
    } else {
      shaderToCompile->m_isCompiled = false;
    }

    return exitCode;
}
