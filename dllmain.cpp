#include <windows.h>
#include <string>

// Функция симуляции нажатия символа
void SendUnicodeChar(wchar_t ch) {
    INPUT input[2] = {};
    
    // Нажатие клавиши
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = ch;
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;

    // Отпускание клавиши
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.wScan = ch;
    input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    SendInput(2, input, sizeof(INPUT));
}

// Функция нажатия клавиши Enter
void SendVirtualKey(WORD vk) {
    INPUT input[2] = {};

    // Нажатие
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = vk;

    // Отпускание
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = vk;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(2, input, sizeof(INPUT));
}

// Посимвольная печать текста
void TypeTextAndEnter(const std::wstring& text) {
    for (wchar_t ch : text) {
        SendUnicodeChar(ch);
        Sleep(5);
    }
    SendVirtualKey(VK_RETURN);
}

// Чтение буфера обмена
std::wstring GetClipboardTextW() {
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT) || !OpenClipboard(nullptr)) {
        return L"";
    }
    std::wstring result = L"";
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData != nullptr) {
        wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
        if (pszText != nullptr) {
            result = pszText;
            GlobalUnlock(hData);
        }
    }
    CloseClipboard();
    return result;
}

// Основной фоновый поток внутри процесса
DWORD WINAPI ThreadProc(LPVOID lpParam) {
    std::wstring lastText = GetClipboardTextW();
    while (true) {
        Sleep(50);
        std::wstring currentText = GetClipboardTextW();
        if (!currentText.empty() && currentText != lastText) {
            lastText = currentText;
            Sleep(100);
            TypeTextAndEnter(currentText);
        }
    }
    return 0;
}

// Точка входа DLL
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        HANDLE hThread = CreateThread(nullptr, 0, ThreadProc, nullptr, 0, nullptr);
        if (hThread != nullptr) {
            CloseHandle(hThread);
        }
    }
    return TRUE;
}
