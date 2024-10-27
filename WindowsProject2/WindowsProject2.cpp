#include <windows.h>
#include <commdlg.h>
#include <fstream>
#include <string>
#include <locale>
#include <codecvt>
#include <iostream>
#include <tchar.h>
#include <cwctype> // Для использования iswlower

using namespace std;

// Глобальные переменные
HWND hEditOutput;
wstring loadedText;
wstring loadedKey;

// Прототипы функций
void loadText(const wstring& filePath);
void loadKey(const wstring& filePath);
wstring encrypt(const wstring& text, const wstring& key);
wstring decrypt(const wstring& encryptedText, const wstring& key);
void saveToFile(const wstring& filePath, const wstring& content);
void ShowMessage(const wstring& message);
void SelectFile(wstring& fileName);
void print_alphabet();
bool check_str(const wstring& str);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Главная функция программы
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd) {
    const TCHAR CLASS_NAME[] = _T("EncryptionWindowClass");

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        _T("Шифрование и Дешифрование"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nShowCmd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// Функция обработки сообщений окна
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        CreateWindow(
            _T("BUTTON"), _T("Загрузить текст"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            50, 20, 200, 30,
            hwnd, (HMENU)1,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        CreateWindow(
            _T("BUTTON"), _T("Загрузить ключ"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            300, 20, 200, 30,
            hwnd, (HMENU)2,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        CreateWindow(
            _T("BUTTON"), _T("Зашифровать"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            50, 70, 200, 50,
            hwnd, (HMENU)3,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        CreateWindow(
            _T("BUTTON"), _T("Дешифровать"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            300, 70, 200, 50,
            hwnd, (HMENU)4,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        hEditOutput = CreateWindow(
            _T("EDIT"), NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            50, 130, 500, 200,
            hwnd, NULL,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
    }
                  break;

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case 1: // Загрузить текст
            SelectFile(loadedText);
            loadText(loadedText);
            SetWindowText(hEditOutput, loadedText.c_str()); // Отображаем загруженный текст
            break;
        case 2: // Загрузить ключ
            SelectFile(loadedKey);
            loadKey(loadedKey);
            SetWindowText(hEditOutput, loadedKey.c_str()); // Отображаем загруженный ключ
            break;
        case 3: // Зашифровать
            if (!loadedText.empty() && !loadedKey.empty()) {
                if (check_str(loadedText) && check_str(loadedKey)) {
                    wstring encryptedText = encrypt(loadedText, loadedKey);
                    saveToFile(L"encrypted_text.txt", encryptedText);
                    SetWindowText(hEditOutput, encryptedText.c_str());
                    ShowMessage(L"Текст успешно зашифрован!");
                }
                else {
                    ShowMessage(L"Текст или ключ содержат недопустимые символы.");
                }
            }
            else {
                ShowMessage(L"Сначала загрузите текст и ключ.");
            }
            break;
        case 4: // Дешифровать
            if (!loadedText.empty() && !loadedKey.empty()) {
                if (check_str(loadedText) && check_str(loadedKey)) {
                    wstring decryptedText = decrypt(loadedText, loadedKey);
                    saveToFile(L"decrypted_text.txt", decryptedText);
                    SetWindowText(hEditOutput, decryptedText.c_str());
                    ShowMessage(L"Текст успешно дешифрован!");
                }
                else {
                    ShowMessage(L"Текст или ключ содержат недопустимые символы.");
                }
            }
            else {
                ShowMessage(L"Сначала загрузите текст и ключ.");
            }
            break;
        }
    }
                   break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
// Функция для выбора файла
void SelectFile(wstring& fileName) {
    OPENFILENAME ofn;       // структура для открытия файла
    wchar_t szFile[MAX_PATH] = { 0 }; // буфер для имени файла
    ZeroMemory(&ofn, sizeof(ofn)); // обнуляем структуру
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"Все файлы\0*.*\0Текстовые файлы\0*.txt\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Передаем адрес ofn в GetOpenFileName
    if (GetOpenFileName(&ofn)) {
        fileName = ofn.lpstrFile;
    }
}

// Функция для загрузки текста из файла
void loadText(const wstring& filePath) {
    wifstream file(filePath);
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
    if (!file.is_open()) {
        wcerr << L"Ошибка при открытии файла: " << filePath << endl;
        loadedText.clear();
        return;
    }
    wstring line;
    loadedText.clear();
    while (getline(file, line)) {
        loadedText += line + L"\n"; // Добавляем новую строку
    }
}

// Функция для загрузки ключа из файла
void loadKey(const wstring& filePath) {
    wifstream file(filePath);
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
    if (!file.is_open()) {
        wcerr << L"Ошибка при открытии файла: " << filePath << endl;
        loadedKey.clear();
        return;
    }
    getline(file, loadedKey);
}

// Функция для шифрования текста
wstring encrypt(const wstring& text, const wstring& key) {
    wstring alphabet = L"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ_";
    wstring encrypted;

    // Перебираем каждый символ в строке
    for (wchar_t c : text) {
        size_t index = alphabet.find(c);
        if (index != wstring::npos) {
            encrypted += key[index];
        }
        else {
            encrypted += c; // Оставляем символы, которые не находятся в алфавите
        }
    }
    return encrypted;
}

// Функция для дешифрования текста
wstring decrypt(const wstring& encryptedText, const wstring& key) {
    wstring alphabet = L"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ_";
    wstring decrypted;

    // Перебираем каждый символ в строке
    for (wchar_t c : encryptedText) {
        size_t index = key.find(c);
        if (index != wstring::npos) {
            decrypted += alphabet[index];
        }
        else {
            decrypted += c; // Оставляем символы, которые не находятся в алфавите
        }
    }
    return decrypted;
}

// Функция для сохранения текста в файл
void saveToFile(const wstring& filePath, const wstring& content) {
    wofstream file(filePath);
    if (!file.is_open()) {
        wcerr << L"Ошибка при открытии файла для записи: " << filePath << endl;
        return;
    }
    file << content;
}

// Функция для отображения сообщения
void ShowMessage(const wstring& message) {
    MessageBoxW(NULL, message.c_str(), L"Сообщение", MB_OK);
}


// Функция проверки, что все символы в строке принадлежат алфавиту
bool check_str(const wstring& str) {
    wstring alphabet = L"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ_"; // Используем строчные буквы русского алфавита
    for (auto& simbol : str) {
        if (alphabet.find(simbol) == wstring::npos && simbol !='\n') { // Проверяем, есть ли символ в алфавите
            wchar_t buffer[100];
            swprintf(buffer, sizeof(buffer) / sizeof(wchar_t), L"Символ %lc не принадлежит алфавиту", simbol);
            ShowMessage(buffer);
            return false;
        }
    }
    return true;
}
