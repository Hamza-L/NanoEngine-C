#include "NanoShader.h"
#include "NanoConfig.h"
#include "NanoError.h"
#include <string.h>

#ifdef _WIN64
#include <windows.h>
#elif __APPLE__
#include <unistd.h>
#endif

#include <fstream>
#include <StrUtil.h>

void CleanUp(NanoShader* shaderToInitialize){
    vkDestroyShaderModule(shaderToInitialize->_device, shaderToInitialize->m_shaderModule, NULL);
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
    throw std::runtime_error("failed to create shader module!");
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
  LOG_MSG(ERRLevel::INFO, "compiling : %s", fileName);
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

  LOG_MSG(ERRLevel::INFO, "compiling : %s", shaderName);

  pid = fork();
  if(pid == -1){
    fprintf(stderr, "error pid\n");
    exit(EXIT_FAILURE);
  }else if(pid == 0){
    err = execl(lpApplicationName, "", fileName, "-o", outputFileName, (char *)0);
    LOG_MSG(ERRLevel::INFO, "finished compiling: %s\t with exit code: %d", shaderName, err);
    exit(0);
  }else{
    if(waitpid(pid, &status, 0) > 0){
      if (WIFEXITED(status) && !WEXITSTATUS(status) && WEXITSTATUS(status) != 127){
        //LOG_MSG(ERRLevel::INFO, "glslc ran with no issues");
      } else{
        LOG_MSG(ERRLevel::INFO, "glslc exit with error");
      }
    } else {
        LOG_MSG(ERRLevel::FATAL, "error occured with waitpid");
    }
  }
  return err;
}
#endif

static std::vector<char> ReadBinaryFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

void InitShader(NanoShader* shaderToInitialize, const char* shaderCodeFile){
    AppendString(shaderToInitialize->m_fileFullPath, shaderCodeFile);
}

int CompileShader(NanoShader* shaderToInitialize, _Bool forceCompile){
    int exitCode = 1;
    char outputFile[MAX_FILEPATH_LENGTH] = "./src/shader/";

    if(FindString(shaderToInitialize->m_fileFullPath, ".vert") >= 0){
        AppendString(outputFile, "vert");
    } else if (FindString(shaderToInitialize->m_fileFullPath, ".frag") >= 0){
        AppendString(outputFile, "frag");
    } else if (FindString(shaderToInitialize->m_fileFullPath, ".comp") >= 0){
        AppendString(outputFile, "comp");
    }
    AppendString(outputFile, "_");

    int startIndx = FindLastString(shaderToInitialize->m_fileFullPath, "/");
    int endIndx = FindLastString(shaderToInitialize->m_fileFullPath, ".");

    char filename[256] = {};
    strcpy(filename, shaderToInitialize->m_fileFullPath);
    SubString(filename, startIndx, endIndx-startIndx);

    AppendString(outputFile, filename);
    AppendString(outputFile, ".spv");

    char cmdArgument[256] = " ";
    AppendString(cmdArgument, filename);
    AppendString(cmdArgument, shaderToInitialize->m_fileFullPath);
    AppendString(cmdArgument, " -o ");
    AppendString(cmdArgument, outputFile);

    char const *argv[] = { shaderToInitialize->m_fileFullPath, "-o", outputFile};
#ifdef __APPLE__
    const char* executable = "./external/VULKAN/mac/glslc";
#elif _WIN32
    const char* executable = "./external/VULKAN/win/glslc.exe";
#else
    const char* executable = "./external/VULKAN/linux/glslc";
#endif
    exitCode = RunGLSLCompiler(executable, shaderToInitialize->m_fileFullPath, outputFile, &shaderToInitialize->m_fileFullPath[startIndx]);

    if(!exitCode){
      fprintf(stdout, "Successfully compiled");
      shaderToInitialize->m_isCompiled = true;
      fprintf(stdout, "reading raw shader code from: %s", outputFile);
      shaderToInitialize->m_rawShaderCode = ReadBinaryFile(outputFile);
      shaderToInitialize->m_shaderModule = CreateShaderModule(_device, *this);
    } else {
      shaderToInitialize->m_isCompiled = false;
    }

    return exitCode;
}
