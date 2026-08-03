#include <windows.h>
#include <string>

// Функция симуляции нажатия Unicode-символа
void SendUnicodeChar(wchar_t ch) {
    INPUT inputDown = { 0 };
    inputDown.type = INPUT_KEYBOARD;
    inputDown.ki.wVk = 0;
    inputDown.ki.wScan = ch;
    inputDown.ki.dwFlags = KEYEVENTF_UNICODE;
    SendInput(1, &inputDown, sizeof(INPUT));

    INPUT inputUp = { 0 };
    inputUp.type = INPUT_KEYBOARD;
    inputUp.ki.wVk = 0;
    inputUp.ki.wScan = ch;
    inputUp.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    SendInput(1, &inputUp, sizeof(INPUT));
}

// Функция нажатия управляющей клавиши (Enter)
void SendVirtualKey(WORD vk) {
    INPUT inputDown = { 0 };
    inputDown.type = INPUT_KEYBOARD;
    inputDown.ki.wVk = vk;
    inputDown.ki.wScan = 0;
    inputDown.ki.dwFlags = 0;
    SendInput(1, &inputDown, sizeof(INPUT));

    INPUT inputUp = { 0 };
    inputUp.type = INPUT_KEYBOARD;
    inputUp.ki.wVk = vk;
    inputUp.ki.wScan = 0;
    inputUp.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &inputUp, sizeof(INPUT));
}

// Посимвольная печать строки текста
void TypeTextAndEnter(const std::wstring& text) {
    for (size_t i = 0; i < text.length(); ++i) {
        SendUnicodeChar(text[i]);
        Sleep(5);
    }
    SendVirtualKey(VK_RETURN);
}

// Получение текста из буфера обмена Windows
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
