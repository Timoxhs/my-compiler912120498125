#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <thread>
#include <chrono>

// ==================== ВАШИ НАСТРОЕННЫЕ КООРДИНАТЫ ====================
// Координаты кнопки меню "Codes" из Paint (190, 657)
constexpr int CODES_BUTTON_X = 190;
constexpr int CODES_BUTTON_Y = 657;

// Координаты поля ввода "Code Here..." из Paint (950, 584)
constexpr int CODE_INPUT_X = 950;
constexpr int CODE_INPUT_Y = 584;

// Координаты кнопки подтверждения "Submit" из Paint (957, 675)
constexpr int SUBMIT_BUTTON_X = 957;
constexpr int SUBMIT_BUTTON_Y = 675;
// ==============================================================================

// Список триггерных слов/фраз (в верхнем регистре для сравнения)
static const std::vector<std::wstring> kTriggers = {
    L"THE CODE IS",
    L"THE CODE",
    L"CODE IS",
    L"HELLO",
    L"YO"
};

// Переводит строку в верхний регистр
std::wstring ToUpper(const std::wstring& s) {
    std::wstring result = s;
    std::transform(result.begin(), result.end(), result.begin(),
        [](wchar_t c) { return std::towupper(c); });
    return result;
}

// Проверяет, является ли символ "мусорным" разделителем
bool IsSeparator(wchar_t c) {
    return c == L':' || c == L'/' || c == L'\\' || c == L'-' ||
           c == L',' || c == L'.' || c == L'!' || c == L'?' ||
           c == L'"' || c == L'\'' || c == L'(' || c == L')' ||
           c == L'[' || c == L']' || c == L'*' || c == L'|';
}

// Разбивает строку на "слова" по пробелам/разделителям
std::vector<std::wstring> SplitToTokens(const std::wstring& s) {
    std::vector<std::wstring> tokens;
    std::wstring current;

    for (wchar_t c : s) {
        if (std::iswspace(c) || IsSeparator(c)) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

// Проверяет, что токен — валидный промокод
bool IsValidPromoCode(const std::wstring& token) {
    if (token.length() <= 3) return false;

    bool hasAlnum = false;
    for (wchar_t c : token) {
        bool isUpperLetter = (c >= L'A' && c <= L'Z');
        bool isDigit = (c >= L'0' && c <= L'9');
        if (!isUpperLetter && !isDigit) {
            return false;
        }
        if (isUpperLetter || isDigit) hasAlnum = true;
    }
    return hasAlnum;
}

// Ищет триггер и извлекает промокод
std::wstring ExtractPromoCode(const std::wstring& input) {
    std::wstring upperInput = ToUpper(input);

    bool triggerFound = false;
    for (const auto& trigger : kTriggers) {
        if (upperInput.find(trigger) != std::wstring::npos) {
            triggerFound = true;
            break;
        }
    }

    if (!triggerFound) return L"";

    std::vector<std::wstring> tokens = SplitToTokens(upperInput);

    static const std::vector<std::wstring> kBlacklist = {
        L"THE", L"CODE", L"IS", L"HELLO", L"YO"
    };

    for (const auto& token : tokens) {
        bool isBlacklisted = std::find(kBlacklist.begin(), kBlacklist.end(), token) != kBlacklist.end();
        if (isBlacklisted) continue;

        if (IsValidPromoCode(token)) {
            return token;
        }
    }
    return L"";
}

// Чтение буфера обмена Windows
std::wstring GetSourceText() {
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

// Клик мышью по абсолютным координатам экрана
void SimulateClick(int x, int y) {
    SetCursorPos(x, y);
    INPUT inputs = {};
    inputs.type = INPUT_MOUSE;
    inputs.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs.type = INPUT_MOUSE;
    inputs.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, inputs, sizeof(INPUT));
}

// Ввод одного unicode-символа
void SimulateChar(wchar_t ch) {
    INPUT inputs = {};
    inputs.type = INPUT_KEYBOARD;
    inputs.ki.wScan = ch;
    inputs.ki.dwFlags = KEYEVENTF_UNICODE;
    inputs.type = INPUT_KEYBOARD;
    inputs.ki.wScan = ch;
    inputs.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
}

// Логика автоматического открытия меню и ввода
void OpenInterfaceAndInput(const std::wstring& code) {
    // 1. Кликаем по кнопке Codes
    SimulateClick(CODES_BUTTON_X, CODES_BUTTON_Y);
    Sleep(50);

    // 2. Кликаем по текстовому полю ввода
    SimulateClick(CODE_INPUT_X, CODE_INPUT_Y);
    Sleep(30);

    // 3. Печатаем промокод
    for (size_t i = 0; i < code.length(); ++i) {
        SimulateChar(code[i]);
        Sleep(15);
    }
    Sleep(30);

    // 4. Кликаем по кнопке Submit для отправки
    SimulateClick(SUBMIT_BUTTON_X, SUBMIT_BUTTON_Y);
}

// Главная логика фонового потока
DWORD WINAPI WorkerThread(LPVOID lpParam) {
    std::wstring lastCode = L"";
    
    // Считываем буфер при старте, чтобы пропустить старые логи
    std::wstring initialText = GetSourceText();
    lastCode = ExtractPromoCode(initialText);

    while (true) {
        Sleep(30); // Проверка каждые 30 миллисекунд
        
        std::wstring sourceText = GetSourceText();
        if (sourceText.empty()) continue;

        std::wstring extractedCode = ExtractPromoCode(sourceText);

        // Если нашли в тексте новый промокод, который отличается от старого
        if (!extractedCode.empty() && extractedCode != lastCode) {
            lastCode = extractedCode; // Запоминаем, чтобы не спамить
            OpenInterfaceAndInput(extractedCode); // Запускаем авто-ввод
        }
    }
    return 0;
}

// Точка входа для инжектора
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        HANDLE hThread = CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
        if (hThread != nullptr) {
            CloseHandle(hThread);
        }
    }
    return TRUE;
}
