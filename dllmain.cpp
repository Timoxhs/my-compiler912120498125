#include <windows.h>
#include <string>
#include <vector>

// Координаты области экрана, где появляется объявление (задаются в пикселях)
// Для точной настройки под разрешение экрана их можно будет изменить
const int SCAN_X_START = 800;
const int SCAN_Y_START = 100;
const int SCAN_WIDTH   = 320;
const int SCAN_HEIGHT  = 50;

// Симуляция нажатия одного Unicode-символа
void SendUnicodeChar(wchar_t ch)
{
    INPUT input[2] = {};

    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = ch;
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.wScan = ch;
    input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    SendInput(2, input, sizeof(INPUT));
}

// Симуляция нажатия управляющих клавиш (Enter, Backspace и т.д.)
void SendVirtualKey(WORD vk)
{
    INPUT input[2] = {};

    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = vk;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = vk;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(2, input, sizeof(INPUT));
}

// Быстрый последовательный ввод строки и нажатие Enter
void TypeTextAndEnter(const std::wstring& text, DWORD delayMs = 2)
{
    for (wchar_t ch : text)
    {
        SendUnicodeChar(ch);
        if (delayMs > 0) Sleep(delayMs);
    }
    SendVirtualKey(VK_RETURN);
}

// Функция для чтения текстовых данных с экрана в заданной области
// Извлекает сырые текстовые блоки из контекста вывода процесса
std::wstring CaptureScreenText()
{
    std::wstring detectedText = L"";
    
    // Получаем контекст устройства текущего активного окна
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return detectedText;

    HDC hdc = GetDC(hwnd);
    if (!hdc) return detectedText;

    // Внутренний буфер для системного считывания шрифтовых глифов и строк
    // Примечание: В базовом WinAPI чтение отрисованного текста ограничено, 
    // поэтому здесь подготавливается структура под будущую интеграцию OCR-библиотеки.
    
    ReleaseDC(hwnd, hdc);
    return detectedText;
}

// Основной фоновый поток внутри процесса
DWORD WINAPI MainAutomationThread(LPVOID lpParam)
{
    std::wstring lastCapturedCode = L"";

    while (true)
    {
        // Минимальная задержка цикла для предотвращения перегрузки процессора (20 мс)
        Sleep(20); 

        // Считываем то, что сейчас отображается по центру экрана
        std::wstring currentText = CaptureScreenText();

        // Проверяем, что текст появился, он не пустой и это не старое объявление
        if (!currentText.empty() && currentText != lastCapturedCode)
        {
            lastCapturedCode = currentText;

            // Имитируем клик или нажатие горячей клавиши для открытия окна ввода кодов, 
            // если оно закрыто (например, отправка интерфейсного макроса)
            
            // Вводим перехваченный текст в активное поле
            TypeTextAndEnter(currentText);
        }
    }
    return 0;
}

// Точка входа для динамической библиотеки (DLL)
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        // Создаем изолированный поток, чтобы не подвешивать основной процесс игры
        HANDLE hThread = CreateThread(nullptr, 0, MainAutomationThread, nullptr, 0, nullptr);
        if (hThread != nullptr)
        {
            CloseHandle(hThread); // Закрываем дескриптор, поток продолжает жить автономно
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
