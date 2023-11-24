#include <windows.h>
#include <psapi.h>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <sstream> 

std::mutex logMutex; // :))))))

std::string GetCurrentProcessName()
{
    char processName[MAX_PATH] = "";
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());

    if (processHandle != NULL)
    {
        GetModuleFileNameExA(processHandle, NULL, processName, sizeof(processName));
        CloseHandle(processHandle);
    }

    // Extract process name from full path
    std::string fullPath(processName);
    size_t pos = fullPath.find_last_of("\\/");
    if (pos != std::string::npos)
    {
        return fullPath.substr(pos + 1);
    }
    return fullPath;
}

std::string logFilePath;

void Log(const char* message)
{
    std::lock_guard<std::mutex> lock(logMutex); // idk

    std::ofstream logFile(logFilePath, std::ios_base::app);
    if (logFile.is_open())
    {
        logFile << message << std::endl;
        logFile.flush();
        logFile.close();
    }
}

void ClearLog()
{
    std::ofstream logFile(logFilePath, std::ios_base::out | std::ios_base::trunc);
    if (logFile.is_open())
    {
        logFile.close();
    }
}



std::string GetDllDirectoryPath()
{
    char dllPath[MAX_PATH] = { 0 };
    HMODULE hModule = NULL;

    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)GetDllDirectoryPath, &hModule) == 0)
    {
        int error = GetLastError();
        char logMessage[512];
        snprintf(logMessage, sizeof(logMessage), "Failed to get DLL path. Error code: %d", error);
        Log(logMessage);
        return "";
    }

    GetModuleFileNameA(hModule, dllPath, sizeof(dllPath));

    std::string fullPath(dllPath);
    size_t lastSlash = fullPath.find_last_of("\\/");
    return fullPath.substr(0, lastSlash);
}


std::string GetIniFilePath()
{
    return GetDllDirectoryPath() + "\\memoryFreer.ini";
}


bool FileExists(const std::string& filename)
{
    std::ifstream file(filename);
    return file.good();
}

void GenerateDefaultIniFile()
{
    if (!FileExists(".\\memoryFreer.ini"))
    {
        std::ofstream iniFile(".\\memoryFreer.ini");
        if (iniFile.is_open())
        {
            iniFile << "[Settings]" << std::endl;
            iniFile << "WorkingSetHotkey=120; Virtual key code. This is F9, use this as a last resort if you hit E12, as it allows the game to save despite it. Will crash if memory hits limit while saving." << std::endl;
            iniFile << "ModifyMemoryHotkey=119; Virtual key code for NOP/Forcing High detail lots to 0. This is F8." << std::endl;
            iniFile << "ModifyMemory=0; Auto NOP. Does nothing and I need to delete this and below" << std::endl;
            iniFile << "DisableWorkingSetClear=0; Disables memory 'clearing'/trim working set and HeapCompact feature. Does nothing because I removed it opting for hotkey instead :)" << std::endl;
            iniFile << "Threshold=2800; Memory in MB to triger warning. I recommend 2.7gb but you can probably get away with 2.8. This is not total usage, assume there's an extra .6gb. Saving adds an extra 100-700mb(!) sometimes.." << std::endl;

            iniFile.close();

            Log("Generated default memoryFreer.ini file.");
        }
        else
        {
            Log("Failed to generate memoryFreer.ini file.");
        }
    }
}


void TriggerLowMemoryNotification()
{
    SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1); //neither of these fix the issue, HeapCompact seems to buy more time tho?
    HeapCompact(GetProcessHeap(), 0);
}

int GetIniValue(const char* section, const char* key, int defaultValue)
{
    int value = GetPrivateProfileIntA(section, key, defaultValue, GetIniFilePath().c_str());

    char logMessage[512];
    snprintf(logMessage, sizeof(logMessage), "Reading INI: [%s] %s = %d", section, key, value); // ini wasn't working but I'm just gunna leave this here
    Log(logMessage);

    return value;
}

int GetHotkey(const char* section, const char* key, int defaultValue)
{
    return GetIniValue(section, key, defaultValue);
}

bool GetBooleanIniValue(const char* section, const char* key, int defaultValue)
{
    return GetIniValue(section, key, defaultValue) != 0;
}


void NOPAddress(uintptr_t address, size_t size)
{
    DWORD oldProtect;
    VirtualProtect((void*)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memset((void*)address, 0x90, size);  // 0x90 is the NOP :)
    VirtualProtect((void*)address, size, oldProtect, &oldProtect);
}

void RestoreOriginalInstruction(uintptr_t address, const unsigned char* originalBytes, size_t size)
{
    DWORD oldProtect;
    VirtualProtect((void*)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy((void*)address, originalBytes, size);
    VirtualProtect((void*)address, size, oldProtect, &oldProtect);
}



//Should probably change this to just write to the address but idk how
const size_t NOP_SIZE = 2; // Should be 2 bytes...h-haha
unsigned char originalBytes[NOP_SIZE];

void ModifyMemoryValue()
{
    uintptr_t baseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleA("TS3W.exe"));
    if (!baseAddress)
    {
        Log("Failed to get base address of TS3W.exe.");
        return;
    }

    uintptr_t targetAddress = baseAddress + 0x86C61B;

    // Backup the original bytes before NOP-ification
    memcpy(originalBytes, (void*)targetAddress, NOP_SIZE);

    NOPAddress(targetAddress, NOP_SIZE);
    Log("NOP-ed memory at target address.");
}

bool BackupOriginalBytes(uintptr_t address, size_t size)
{
    DWORD oldProtect;
    if (VirtualProtect((void*)address, size, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        // Backup the original bytes
        memcpy(originalBytes, (void*)address, size);
        VirtualProtect((void*)address, size, oldProtect, &oldProtect);
        return true;
    }
    return false;
}


bool isNOPed = false;

void ToggleNOP()
{
    uintptr_t baseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleA("TS3W.exe"));
    if (!baseAddress)
    {
        Log("Failed to get base address of TS3W.exe.");
        return;
    }

    uintptr_t targetAddress = baseAddress + 0x86C651; //0x86C61B - 7 bytes oops wrong one hehe

    if (isNOPed)
    {
        RestoreOriginalInstruction(targetAddress, originalBytes, NOP_SIZE);
        isNOPed = false;
        Log("Restored original instruction.");
    }
    else
    {
        if (BackupOriginalBytes(targetAddress, NOP_SIZE))
        {
            NOPAddress(targetAddress, NOP_SIZE);
            isNOPed = true;
            Log("NOP-ed memory at target address.");
        }
        else
        {
            Log("Failed to backup original bytes. NOP operation canceled.");
        }
    }
}





void HotkeyMonitor()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // honkshoo mimimi
    Log("HotkeyMonitor started.");
    int WorkingSetHotkey = GetHotkey("Settings", "WorkingSetHotkey", 120); // Default is F9.
    int ModifyMemoryHotkey = GetHotkey("Settings", "ModifyMemoryHotkey", 119);
    bool modifyMemory = GetBooleanIniValue("Settings", "ModifyMemory", 0);
    bool disableLowMemoryNotification = GetBooleanIniValue("Settings", "DisableWorkingSetClear", 0);

    char logMessage[512];
    snprintf(logMessage, sizeof(logMessage), "Starting HotkeyMonitor: Hotkey=%d, ModifyMemory=%d, DisableWorkingSetClear=%d", WorkingSetHotkey, modifyMemory, disableLowMemoryNotification);
    Log(logMessage); // Log initial settings

    while (true)
    {
        if (GetAsyncKeyState(WorkingSetHotkey) & 0x8000)
        {
            Log("WorkingSetHotkey Pressed!");

            if (!disableLowMemoryNotification)
            {
                Log("Hotkey pressed! Triggering low memory notification.");
                TriggerLowMemoryNotification();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1 sec delay, no spam pls, might need to increase idk
        }
        else if (GetAsyncKeyState(ModifyMemoryHotkey) & 0x8000)
        {
            Log("ModifyMemoryHotkey Hotkey Pressed!");
            ToggleNOP();

            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Polling delay, apparently this is good
    }
}



void MonitorMemory() {
    bool disableLowMemoryNotification = GetBooleanIniValue("Settings", "DisableWorkingSetClear", 0);
    SIZE_T threshold = static_cast<SIZE_T>(GetIniValue("Settings", "Threshold", 2900)) * 1024 * 1024;
    bool warningFired = false; 

    while (true) {
        PROCESS_MEMORY_COUNTERS_EX memInfo;
        DWORD gdiObjects = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
        DWORD userObjects = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);

        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&memInfo, sizeof(memInfo))) {
            SIZE_T totalMemoryUsed = memInfo.PrivateUsage;

            if (!warningFired && threshold != 0 && totalMemoryUsed > threshold) {
                std::stringstream logMessage;
                //could probably just not log any of this
                logMessage << "Memory usage exceeded threshold (" << threshold / 1024 / 1024 << " MB)! "
                    << "Current Memory Usage: " << totalMemoryUsed / 1024 / 1024 << " MB. "
                    << "Private Usage: " << memInfo.PrivateUsage / 1024 / 1024 << " MB, "
                    << "Pagefile Usage: " << memInfo.PagefileUsage / 1024 / 1024 << " MB, "
                    << "Working Set Size: " << memInfo.WorkingSetSize / 1024 / 1024 << " MB, "
                    << "Peak Working Set Size: " << memInfo.PeakWorkingSetSize / 1024 / 1024 << " MB, "
                    << "GDI Objects: " << gdiObjects << ", "
                    << "USER Objects: " << userObjects;

                Log(logMessage.str().c_str());

                MessageBox(NULL,
                    L"Memory threshold hit!! Please save right now or risk losing your progress!",
                    L"Memory Warning",
                    MB_OK | MB_ICONWARNING);

                if (!disableLowMemoryNotification) {
                    TriggerLowMemoryNotification();
                }

                warningFired = true; 
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        logFilePath = GetDllDirectoryPath() + "\\memoryFreer.log";
        ClearLog();
        GenerateDefaultIniFile();
        std::string processName = GetCurrentProcessName();
        char logMessage[512];
        snprintf(logMessage, sizeof(logMessage), "DLL attached to process: %s", processName.c_str());
        Log(logMessage); // Log process name
        Log("Plugin loaded successfully!??!?!?!?!");

        std::thread memoryMonitor(MonitorMemory);
        memoryMonitor.detach();

        std::thread hotkeyThread(HotkeyMonitor);
        hotkeyThread.detach();
    }
    break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}