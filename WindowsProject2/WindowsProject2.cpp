#include <windows.h>
#include <tchar.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <commdlg.h>
using namespace std;

// Константы для идентификаторов элементов интерфейса
#define MAX_SYMBOLS 256
#define ID_BUTTON_ENCODE 1
#define ID_BUTTON_DECODE 2
#define ID_BUTTON_EXIT 3
#define ID_EDIT_OUTPUT 4
#define ID_BUTTON_SYMBOLS_FILE 5
#define ID_BUTTON_PROBABILITIES_FILE 6

// Класс для представления символа с вероятностью и кодовым словом
class Symbol {
private:
    string name = ""; // Имя символа
    double P = 0; // Вероятность символа
    string codeName = ""; // Кодовое слово символа
public:
    Symbol() = default;
    Symbol(string _name, double _P) : name(_name), P(_P) {};
    ~Symbol() = default;

    // Геттеры и методы для работы с символом
    inline string get_name() { return name; }
    inline double get_P() { return P; }
    inline string get_code_name() { return codeName; }
    inline void append_to_code_name(char c) { codeName += c; }
};

// Класс для кодирования и декодирования методом Шеннона-Фано
class ShannonFano {
private:
    Symbol symbols[MAX_SYMBOLS]; // Массив символов
    int symbol_count = 0; // Количество символов
    bool load;
    // Метод для оценки математического выражения
    double evaluate_expression(const string& expression) {
        size_t pos = expression.find('^');
        if (pos != string::npos) {
            double base = stod(expression.substr(0, pos));
            double exponent = stod(expression.substr(pos + 1));
            double r = pow(base, exponent);
            return r;
        }

        pos = expression.find('/');
        if (pos != string::npos) {
            double numerator = stod(expression.substr(0, pos));
            double denominator = stod(expression.substr(pos + 1));
            double r = numerator / denominator;
            return r;
        }
        return stod(expression);
    }

    // Метод для заполнения массива символов из файлов
    bool filling_array(const string& fileName, const string& FileName) {
        ifstream iFile(fileName);
        ifstream cFile(FileName);
        string tmpName;
        stringstream ss;
        string probabilitiesLine;
        double f_p = 0.0;
        symbol_count = 0;
        if (!iFile || !cFile) {
            ss << "Не удалось открыть файл." << "\r\n";
            iFile.close();
            cFile.close();
            return false;
        }
        if (getline(cFile, probabilitiesLine)) {
            istringstream iss(probabilitiesLine);
            vector<string> probabilities;
            string token;
            while (iss >> token) {
                probabilities.push_back(token);
            }
            while (iFile >> tmpName && symbol_count < MAX_SYMBOLS) {
                if (symbol_count < probabilities.size()) {
                    try {
                        double P = evaluate_expression(probabilities[symbol_count]);
                        symbols[symbol_count++] = Symbol(tmpName, P);
                        f_p += P;
                    }
                    catch (const invalid_argument&) {
                        MessageBox(NULL, _T("Ошибка в выражении вероятности: "), _T(""), MB_OK);
                        iFile.close();
                        cFile.close();
                        return false;
                        break;
                    }
                }
                else {
                    MessageBox(NULL, _T("Недостаточно вероятностей для всех имен."), _T(""), MB_OK);
                    iFile.close();
                    cFile.close();
                    return false;
                    break;
                }

            }
        }
        if (f_p > 1.0) {
            ss << "Вероятность больше 1" << "\r\n";
            MessageBox(NULL, _T("Вероятность больше 1"), _T(""), MB_OK);
            return false;
        }
        else {
            return true;
        }
        iFile.close();
        cFile.close();
    }

    // Метод для сортировки символов по вероятности 
    void sort_symbols() {
        sort(symbols, symbols + symbol_count, [](Symbol& a, Symbol& b) {
            return a.get_P() > b.get_P();
            });
    }

    // Рекурсивный метод для кодирования методом Шеннона-Фано
    void shannon_fano(int start, int end) {
        if (start >= end) return;

        double total = 0;
        for (int i = start; i <= end; ++i) {
            total += symbols[i].get_P();
        }

        double half = total / 2;
        double accum = 0;
        int split = start;
        for (int i = start; i <= end; ++i) {
            accum += symbols[i].get_P();
            if (accum >= half) {
                split = i;
                break;
            }
        }

        for (int i = start; i <= split; ++i) {
            symbols[i].append_to_code_name('1');
        }
        for (int i = split + 1; i <= end; ++i) {
            symbols[i].append_to_code_name('0');
        }

        shannon_fano(start, split);
        shannon_fano(split + 1, end);
    }
public:
    // Конструктор, который инициализирует и запускает кодирование
    ShannonFano(const string symbolsFileName, const string probabilitiesFileName) {
        load = filling_array(symbolsFileName, probabilitiesFileName);
        if (load)
        {
            sort_symbols();
            shannon_fano(0, symbol_count - 1);
        }

    }

    // Метод для вывода кодовых слов символов 
    void output_code_words(stringstream& ss) {
        for (int i = 0; i < symbol_count; ++i) {
            ss << "Символ " << symbols[i].get_name() << " имеет кодовое слово " << symbols[i].get_code_name() << "\r\n";
        }
    }

    // Метод для расчета средней длины кодовых слов
    double calculate_average_length() {
        double avg_length = 0.0;
        for (int i = 0; i < symbol_count; ++i) {
            avg_length += symbols[i].get_P() * symbols[i].get_code_name().length();
        }
        return avg_length;
    }

    // Метод для расчета энтропии
    double calculate_entropy() {
        double entropy = 0.0;
        for (int i = 0; i < symbol_count; ++i) {
            double p = symbols[i].get_P();
            if (p > 0) {
                entropy -= p * log2(p);
            }
        }
        return entropy;
    }

    // Метод для расчета избыточности
    double calculate_redundancy() {
        return calculate_average_length() - calculate_entropy();
    }

    // Метод для проверки неравенства Крафта
    double check_kraft_inequality() {
        double sum = 0.0;
        for (int i = 0; i < symbol_count; ++i) {
            sum += 1.0 / pow(2, (symbols[i].get_code_name().length()));
        }
        return sum;
    }

    // Метод для кодирования файла 
    void coding(const string iFileName, const string outFileName) {
        ifstream iFile(iFileName);
        ofstream oFile(outFileName);

        string tmp;
        if (iFile && oFile) {
            while (iFile >> tmp) {
                bool found = false;
                for (int i = 0; i < symbol_count; ++i) {
                    if (symbols[i].get_name() == tmp) {
                        oFile << symbols[i].get_code_name() << " ";
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    throw string{ "Слово \"" + tmp + "\" не найдено в алфавите" };
                }
            }
        }
        else {
            if (!iFile) {
                throw string{ "Не удалось открыть файл " + iFileName };
            }
            else {
                throw string{ "Не удалось открыть файл " + outFileName };
            }
        }
        iFile.close();
        oFile.close();
    }
    bool load_successful() {
        return load;
    }
    // Метод для декодирования файла
    void decoding(const string iFileName, const string outFileName) {
        ifstream iFile(iFileName);
        ofstream oFile(outFileName);

        string str;
        if (iFile && oFile) {
            iFile >> str;
            string currentCode;
            for (char c : str) {
                currentCode += c;
                for (int i = 0; i < symbol_count; ++i) {
                    if (symbols[i].get_code_name() == currentCode) {
                        oFile << symbols[i].get_name() << ' ';
                        currentCode.clear();
                        break;
                    }
                }
            }
            if (!currentCode.empty()) {
                throw string{ "Слово не найдено в алфавите" };
            }
        }
        else {
            if (!iFile) {
                throw string{ "Не удалось открыть файл " + iFileName };
            }
            else {
                throw string{ "Не удалось открыть файл " + outFileName };
            }
        }
        iFile.close();
        oFile.close();
    }
};

// Глобальные переменные для хранения имен файлов
string symbolsFileName;
string probabilitiesFileName;
string encodFileName;
string probabilities1FileName = "out1.txt";
string decodFileName;
string probabilities2FileName = "out2.txt";
HWND hEditOutput;

// Прототипы функций
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OnEncode();
void OnDecode();
void polych();
void SelectFile(string& fileName);

// Главная функция программы
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd) {
    const TCHAR CLASS_NAME[] = _T("ShannonFanoWindowClass");

    // Регистрация класса окна
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Создание окна
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        _T("Шеннон-Фано Кодировщик"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 500,
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
            _T("BUTTON"), _T("Выбрать файл символов"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            50, 20, 200, 30,
            hwnd, (HMENU)ID_BUTTON_SYMBOLS_FILE,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        CreateWindow(
            _T("BUTTON"), _T("Выбрать файл вероятностей"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            300, 20, 200, 30,
            hwnd, (HMENU)ID_BUTTON_PROBABILITIES_FILE,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        CreateWindow(
            _T("BUTTON"), _T("Закодировать"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            150, 60, 200, 50,
            hwnd, (HMENU)ID_BUTTON_ENCODE,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        CreateWindow(
            _T("BUTTON"), _T("Декодировать"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            150, 120, 200, 50,
            hwnd, (HMENU)ID_BUTTON_DECODE,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        CreateWindow(
            _T("BUTTON"), _T("Выход"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            150, 180, 200, 50,
            hwnd, (HMENU)ID_BUTTON_EXIT,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        // Создание текстового поля для вывода
        hEditOutput = CreateWindow(
            _T("EDIT"), NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            50, 250, 500, 200,
            hwnd, (HMENU)ID_EDIT_OUTPUT,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
    }
                  break;

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case ID_BUTTON_ENCODE:
            SelectFile(encodFileName);
            OnEncode();
            break;
        case ID_BUTTON_DECODE:
            SelectFile(decodFileName);
            OnDecode();
            break;
        case ID_BUTTON_EXIT:
            PostQuitMessage(0);
            break;
        case ID_BUTTON_SYMBOLS_FILE:
            SelectFile(symbolsFileName);
            if (!probabilitiesFileName.empty()) {
                polych();
            }
            break;
        case ID_BUTTON_PROBABILITIES_FILE:
            SelectFile(probabilitiesFileName);
            if (!symbolsFileName.empty()) {
                polych();
            }
            break;
        }
    }break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Функция для выбора файла
void SelectFile(string& fileName) {
    OPENFILENAMEA ofn; char szFile[MAX_PATH] = { 0 }; stringstream ss;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Все файлы\0*.*\0Текстовые файлы\0*.TXT\0";

    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        fileName = ofn.lpstrFile;
        ss << "Выбран файл: " << fileName << "\r\n";
    }
    else {
        ss << "Выбор файла отменен или не удался." << "\r\n";
    }
}

// Функция для кодирования
void OnEncode() {
    try {
        ShannonFano sh(symbolsFileName, probabilitiesFileName);
        if (sh.load_successful()) {
            sh.coding(encodFileName, probabilities1FileName);
            MessageBox(NULL, _T("Файл успешно закодирован!"), _T("Успех"), MB_OK);
        }
    }
    catch (const string& error_message) {
        MessageBoxA(NULL, error_message.c_str(), "Ошибка", MB_OK | MB_ICONERROR);
    }
}

// Функция для вывода результатов кодирования
void polych() {
    ShannonFano sh(symbolsFileName, probabilitiesFileName);
    stringstream ss;
    if (sh.load_successful()) {
        sh.output_code_words(ss);
        ss << "Средняя длина: " << sh.calculate_average_length() << "\r\n";
        ss << "Избыточность: " << sh.calculate_redundancy() << "\r\n";
        ss << "Неравенство Крафта выполнено: " << sh.check_kraft_inequality() << (sh.check_kraft_inequality() <= 1.0 ? " Да" : " Нет") << "\r\n";

    }SetWindowTextA(hEditOutput, ss.str().c_str());
}

// Функция для декодирования
void OnDecode() {
    try {
        ShannonFano sh(symbolsFileName, probabilitiesFileName);
        if (sh.load_successful()) {
            sh.decoding(decodFileName, probabilities2FileName);
            MessageBox(NULL, _T("Файл успешно декодирован!"), _T("Успех"), MB_OK);
        }
    }
    catch (const string& error_message) {
        MessageBoxA(NULL, error_message.c_str(), "Ошибка", MB_OK | MB_ICONERROR);
    }
}