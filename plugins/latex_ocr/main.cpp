#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

std::string getExeDir()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string s(buffer);
    size_t pos = s.find_last_of("\\/");
    return (pos == std::string::npos) ? "" : s.substr(0, pos);
}

bool fileExists(const std::string& path)
{
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

int main(int argc, char* argv[])
{
    std::string exeDir = getExeDir();
    std::string runnerPy = exeDir + "\\runner.py";

    std::vector<std::string> pythonCandidates = {
        "E:\\miniconda3\\envs\\py311\\python.exe",
        "E:\\miniconda3\\envs\\py312\\python.exe",
        "E:\\miniconda3\\python.exe",
        "python.exe"
    };

    std::string pythonExe;
    for (size_t i = 0; i < pythonCandidates.size(); ++i) {
        if (fileExists(pythonCandidates[i]) || pythonCandidates[i] == "python.exe") {
            pythonExe = pythonCandidates[i];
            break;
        }
    }

    if (pythonExe.empty() || !fileExists(runnerPy)) {
        std::cerr << "{\"code\": 500, \"error\": \"Runner script or Python environment missing\"}" << std::endl;
        return 1;
    }

    std::string cmd = "\"" + pythonExe + "\" \"" + runnerPy + "\"";
    for (int i = 1; i < argc; ++i) {
        cmd += " \"";
        cmd += argv[i];
        cmd += "\"";
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "{\"code\": 500, \"error\": \"Failed to execute runner\"}" << std::endl;
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (int)exitCode;
}
