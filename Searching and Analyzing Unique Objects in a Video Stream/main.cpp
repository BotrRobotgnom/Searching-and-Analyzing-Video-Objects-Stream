#include "framework.h"
#include "main.h"
#include "video.h"
#include "segment_utils.h"
#include "neural_network.h"

#include <random> 
#include <windows.h>
#include <unordered_map>
#include <tuple>
#include <fstream>

#define MAX_LOADSTRING 100


WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
HINSTANCE hInst;
HWND hWndMain;

int frameWidth = 640;
int frameHeight = 360;
int targetPixels = 4;
int inputSize = targetPixels * targetPixels;

HWND hEditAddType;
HWND hButtonAddType;

int nextTypeID = 1900; //Унікальні ID для кнопок "Видалити"
std::vector<TypeEntry> types;

std::vector<COLORREF> typeColors;
std::map<int, COLORREF> buttonColors;
COLORREF currentPaintColor = RGB(0, 0, 0);

extern std::string videoFullPath;

bool isPainting = false;
bool isErasing = false;
POINT dragStart, dragEnd;

static std::unordered_map<std::wstring, int> classIndex;
static std::vector<std::wstring> classNames;

std::vector<int> layers = { 2, 2, 2};
double learning_rate = 0.23;
int epochs = 1500;
extern NeuralNetwork neuralNet;
NeuralNetwork neuralNet(layers, learning_rate);

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = szWindowClass;
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow,
    LPCWSTR szWindowClass, LPCWSTR szTitle,
    HWND& hWndMain)
{
    hWndMain = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 890, 580, nullptr, nullptr, hInstance, nullptr);

    if (!hWndMain) return FALSE;

    CreateWindowW(L"BUTTON", L"Prev Frame", WS_VISIBLE | WS_CHILD,
        20, 20, 100, 30, hWndMain, (HMENU)ID_BUTTON_PREV, hInstance, NULL);
    CreateWindowW(L"BUTTON", L"Next Frame", WS_VISIBLE | WS_CHILD,
        140, 20, 100, 30, hWndMain, (HMENU)ID_BUTTON_NEXT, hInstance, NULL);
    CreateWindowW(L"BUTTON", L"Play Forward", WS_VISIBLE | WS_CHILD,
        260, 20, 100, 30, hWndMain, (HMENU)ID_BUTTON_PLAY_FORWARD, hInstance, NULL);
    CreateWindowW(L"BUTTON", L"Play Backward", WS_VISIBLE | WS_CHILD,
        380, 20, 120, 30, hWndMain, (HMENU)ID_BUTTON_PLAY_BACKWARD, hInstance, NULL);
    CreateWindowW(L"BUTTON", L"Pause", WS_VISIBLE | WS_CHILD,
        520, 20, 100, 30, hWndMain, (HMENU)ID_BUTTON_PAUSE, hInstance, NULL);

    //Створення текстового поля для введення нового типу
    hEditAddType = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        680, 20, 180, 25, hWndMain, NULL, hInstance, NULL);

    //Кнопка "Додати"
    hButtonAddType = CreateWindowW(L"BUTTON", L"Add type", WS_VISIBLE | WS_CHILD,
        680, 55, 180, 30, hWndMain, (HMENU)9001, hInstance, NULL);

    CreateWindowW(L"BUTTON", L"Train NN", WS_VISIBLE | WS_CHILD,
        20, 500, 100, 30, hWndMain, (HMENU)ID_BUTTON_TRAIN, hInstance, NULL);

    //Кнопка Run NN
    CreateWindowW(L"BUTTON", L"Run NN", WS_VISIBLE | WS_CHILD,
        180, 500, 100, 30, hWndMain, (HMENU)ID_BUTTON_RUN, hInstance, NULL);

    CreateWindowW(L"BUTTON", L"Load NN", WS_VISIBLE | WS_CHILD,
        340, 500, 100, 30, hWndMain, (HMENU)ID_LOAD_BUTTON, hInstance, NULL);

    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);

    return TRUE;
}

// Вспомогательные функции (положите в сегмент_utils.cpp или рядом с OnTrainButton)

// Проверка существования файла
bool fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

// Преобразование COLORREF → cv::Vec3b
cv::Vec3b ColorRefToVec3b(COLORREF ref) {
    return cv::Vec3b(GetBValue(ref), GetGValue(ref), GetRValue(ref));
}

// Получаем список всех путей к файлам масок (*.png) в папке без расширения videoFullPath
std::vector<std::string> getExistingMaskPaths(const std::string& videoFullPath) {
    // Убираем расширение (".mp4", ".avi", ...)
    size_t pos = videoFullPath.find_last_of('.');
    std::string folder = (pos == std::string::npos)
        ? videoFullPath
        : videoFullPath.substr(0, pos);

    std::vector<std::string> masks;
    // Предполагаем, что всего totalFrames кадров
    extern int totalFrames;
    for (int i = 0; i < totalFrames; ++i) {
        std::ostringstream oss;
        oss << folder << "/frame_" << i << ".png";
        std::string path = oss.str();
        if (fileExists(path)) {
            masks.push_back(path);
        }
    }
    return masks;
}

// Извлекаем индекс кадра из имени "…/frame_<idx>.png"
int extractFrameIndex(const std::string& maskPath) {
    // находим последний '_'
    size_t u = maskPath.find_last_of('_');
    size_t d = maskPath.find_last_of('.');
    if (u == std::string::npos || d == std::string::npos || d <= u + 1)
        return -1;
    std::string num = maskPath.substr(u + 1, d - u - 1);
    return std::atoi(num.c_str());
}

// Строим путь до исходного кадра видео по тому же индексу
std::string buildFramePath(const std::string& videoFullPath, int frameIndex) {
    size_t pos = videoFullPath.find_last_of('.');
    std::string folder = (pos == std::string::npos)
        ? videoFullPath
        : videoFullPath.substr(0, pos);
    std::ostringstream oss;
    oss << folder << "/frame_" << frameIndex << ".png";
    return oss.str();
}

// Основная функция обучения
void OnTrainButton() {
    int outputSize = static_cast<int>(typeColors.size());
    if (outputSize == 0) {
        MessageBoxW(nullptr, L"No types defined for training", L"Training", MB_OK);
        return;
    }

    extern std::string videoFullPath;
    std::vector<std::string> maskPaths = getExistingMaskPaths(videoFullPath);
    {
        // DEBUG: сколько масок найдено
        std::wstring dbg = L"Found " + std::to_wstring(maskPaths.size()) + L" masks.";
        MessageBoxW(nullptr, dbg.c_str(), L"Debug", MB_OK);
    }
    if (maskPaths.empty()) {
        MessageBoxW(nullptr, L"No masks found for training", L"Training", MB_OK);
        return;
    }

    int inputSize = targetPixels * targetPixels;
    std::vector<int> layers = { inputSize, inputSize, outputSize };
    neuralNet = NeuralNetwork(layers, learning_rate);

    std::vector<std::vector<double>> inputs;
    std::vector<std::vector<double>> outputs;

    for (size_t mi = 0; mi < maskPaths.size(); ++mi) {
        const std::string& maskPath = maskPaths[mi];
        int frameIdx = extractFrameIndex(maskPath);
        if (frameIdx < 0) continue;

        std::string framePath = buildFramePath(videoFullPath, frameIdx);
        cv::Mat frame = cv::imread(framePath, cv::IMREAD_COLOR);
        cv::Mat mask = cv::imread(maskPath, cv::IMREAD_COLOR);
        if (frame.empty() || mask.empty()) {
            std::wstring msg = L"Cannot open frame or mask at index "
                + std::to_wstring(frameIdx);
            MessageBoxW(nullptr, msg.c_str(), L"Error", MB_OK);
            continue;
        }

        int validBlocks = 0;
        for (int y = 0; y < frame.rows; y += targetPixels) {
            for (int x = 0; x < frame.cols; x += targetPixels) {
                int w = std::min(targetPixels, frame.cols - x);
                int h = std::min(targetPixels, frame.rows - y);
                if (w <= 0 || h <= 0) continue;

                cv::Rect roi(x, y, w, h);
                cv::Mat patchColor = frame(roi);
                cv::Mat patchMask = mask(roi);
                if (patchColor.empty() || patchMask.empty()) continue;

                // вход: grayscale block
                cv::Mat gray;
                cv::cvtColor(patchColor, gray, cv::COLOR_BGR2GRAY);
                gray.convertTo(gray, CV_64F, 1.0 / 255.0);
                std::vector<double> inVec;
                inVec.reserve(inputSize);
                for (int py = 0; py < gray.rows; ++py)
                    for (int px = 0; px < gray.cols; ++px)
                        inVec.push_back(gray.at<double>(py, px));
                while ((int)inVec.size() < inputSize)
                    inVec.push_back(0.0);

                // выход: процент каждого типа в mask-block
                std::map<int, int> counts;
                int totalColored = 0;
                for (int py = 0; py < patchMask.rows; ++py) {
                    for (int px = 0; px < patchMask.cols; ++px) {
                        cv::Vec3b c = patchMask.at<cv::Vec3b>(py, px);
                        if (c == cv::Vec3b(0, 0, 0)) continue;
                        // ищем совпадение
                        for (int ci = 0; ci < outputSize; ++ci) {
                            cv::Vec3b tc = ColorRefToVec3b(typeColors[ci]);
                            if (c == tc) {
                                counts[ci]++;
                                break;
                            }
                        }
                        totalColored++;
                    }
                }

                std::vector<double> outVec(outputSize, 0.0);
                if (totalColored > 0) {
                    // отладка: покажем, какие цвета встретились
                    std::wstring dbg = L"Block at (" + std::to_wstring(x) + L"," +
                        std::to_wstring(y) + L") counts:";
                    for (std::map<int, int>::iterator it = counts.begin(); it != counts.end(); ++it) {
                        dbg += L" [" + std::to_wstring(it->first) + L":" +
                            std::to_wstring(it->second) + L"]";
                    }
                    //MessageBoxW(nullptr, dbg.c_str(), L"Debug Colors", MB_OK);

                    for (std::map<int, int>::iterator it = counts.begin(); it != counts.end(); ++it) {
                        outVec[it->first] = double(it->second) / totalColored;
                    }
                }

                inputs.push_back(inVec);
                outputs.push_back(outVec);
                ++validBlocks;
            }
        }
        if (validBlocks == 0) {
            std::wstring dbg = L"No valid blocks in mask " +
                std::wstring(maskPath.begin(), maskPath.end());
            MessageBoxW(nullptr, dbg.c_str(), L"Debug", MB_OK);
        }
    }

    if (inputs.empty()) {
        MessageBoxW(nullptr, L"No valid training data found.", L"Training", MB_OK);
        return;
    }

    neuralNet.train_batch(inputs, outputs, epochs);
}

void LoadNeuralNetwork() {
    std::ifstream in("weights.txt");
    if (!in) {
        MessageBox(nullptr, L"Cannot open weights.txt", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    int outputSize = static_cast<int>(typeColors.size());
    if (outputSize == 0) {
        MessageBoxW(nullptr, L"No types defined for training", L"Training", MB_OK);
        return;
    }
    std::vector<int> layers = { inputSize, inputSize, outputSize };
    neuralNet = NeuralNetwork(layers, learning_rate);
    std::vector<std::vector<std::vector<double>>> loaded;
    std::string line;
    int currentLayer = -1;

    while (std::getline(in, line)) {
        //Якщо рядок починається на "Layer "
        if (line.rfind("Layer", 0) == 0) {
            //Створюємо новий шар
            loaded.emplace_back();
            ++currentLayer;
        }
        else if (currentLayer >= 0 && !line.empty()) {
            //Парсимо рядок чисел
            std::istringstream iss(line);
            std::vector<double> row;
            double w;
            while (iss >> w) {
                row.push_back(w);
            }
            loaded[currentLayer].push_back(row);
        }
    }
    in.close();

    if (loaded.empty()) {
        MessageBox(nullptr, L"No weights found in file", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    //Передаємо в мережу
    neuralNet.set_weights(loaded);
    MessageBox(nullptr, L"Модель успішно завантажена.", L"Загрузка", MB_OK);
}

void OnRunButton() {
    // 1) Вибір відеофайлу
    std::string path = OpenVideoFile(hWndMain);
    if (path.empty()) return;

    // 2) Завантаження відео
    if (!LoadVideo(path)) {
        MessageBox(hWndMain, L"Не вдалося відкрити відеофайл.", L"Помилка", MB_OK | MB_ICONERROR);
        return;
    }
    // 3) Реєстрація вікна для відтворення
    static const wchar_t* runClass = L"NN_RUN_WINDOW";
    static bool isRegistered = false;
    if (!isRegistered) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc = [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
            switch (msg) {
            case WM_CREATE:
                SetTimer(hWnd, 1, 40, NULL);
                return 0;
            case WM_TIMER:
                if (!UpdateFrame(hWnd)) {
                    KillTimer(hWnd, 1);
                    ReleaseVideo();
                    MessageBox(hWnd, L"Відео завершено.", L"Інформація", MB_OK | MB_ICONINFORMATION);
                    DestroyWindow(hWnd);
                }
                else {
                    InvalidateRect(hWnd, nullptr, FALSE);
                }
                return 0;

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                cv::Mat frame = GetCurrentFrame();
                if (!frame.empty()) {
                    cv::Mat processed = frame.clone();
                    const int blockSize = targetPixels;
                    int cols = processed.cols, rows = processed.rows;
                    int outputCount = static_cast<int>(typeColors.size());

                    // Готовим цвета так же, как при тренировке
                    std::vector<cv::Scalar> overlayCols(outputCount);
                    for (int i = 0; i < outputCount; ++i) {
                        overlayCols[i] = cv::Scalar(
                            GetBValue(typeColors[i]),
                            GetGValue(typeColors[i]),
                            GetRValue(typeColors[i])
                        );
                    }

                    // Создаем наложение
                    cv::Mat overlay(processed.size(), CV_8UC3, cv::Scalar(0, 0, 0));

                    // Счетчики блоков по типам
                    std::vector<int> typeBlocks(outputCount, 0);
                    int totalBlocks = 0;

                    for (int y = 0; y < rows; y += blockSize) {
                        for (int x = 0; x < cols; x += blockSize) {
                            int w = std::min(blockSize, cols - x);
                            int h = std::min(blockSize, rows - y);
                            cv::Rect r(x, y, w, h);
                            cv::Mat patch = processed(r);
                            cv::Mat gray;
                            cv::cvtColor(patch, gray, cv::COLOR_BGR2GRAY);

                            // формируем вход
                            std::vector<double> in;
                            in.reserve(blockSize * blockSize);
                            for (int yy = 0; yy < gray.rows; ++yy)
                                for (int xx = 0; xx < gray.cols; ++xx)
                                    in.push_back(gray.at<uchar>(yy, xx) / 255.0);
                            while ((int)in.size() < blockSize * blockSize)
                                in.push_back(0.0);

                            // forward и выбор типа
                            auto out = neuralNet.forward(in);
                            int best = 0;
                            for (int i = 1; i < (int)out.size(); ++i)
                                if (out[i] > out[best]) best = i;

                            // отмечаем
                            typeBlocks[best]++;
                            totalBlocks++;

                            // красим блок
                            cv::rectangle(overlay, r, overlayCols[best], cv::FILLED);
                        }
                    }

                    // смешиваем
                    cv::addWeighted(overlay, 0.4, processed, 0.6, 0, processed);

                    /*
                    // рисуем проценты
                    int baseY = 30;
                    for (int i = 0; i < outputCount; ++i) {
                        double pct = totalBlocks > 0
                            ? (100.0 * typeBlocks[i] / totalBlocks)
                            : 0.0;
                        std::wstring text = std::to_wstring(i) + L": " +
                            std::to_wstring((int)pct) + L"%";
                        cv::putText(
                            processed,
                            std::string(text.begin(), text.end()),
                            cv::Point(10, baseY + 30 * i),
                            cv::FONT_HERSHEY_SIMPLEX,
                            0.8,
                            cv::Scalar(255, 255, 255),
                            2
                        );
                    }*/

                    // показываем
                    cv::Mat rgb;
                    cv::cvtColor(processed, rgb, cv::COLOR_BGR2RGB);
                    BITMAPINFO bmi = {};
                    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
                    bmi.bmiHeader.biWidth = rgb.cols;
                    bmi.bmiHeader.biHeight = -rgb.rows;
                    bmi.bmiHeader.biPlanes = 1;
                    bmi.bmiHeader.biBitCount = 24;
                    bmi.bmiHeader.biCompression = BI_RGB;
                    StretchDIBits(
                        hdc, 0, 0, rgb.cols, rgb.rows,
                        0, 0, rgb.cols, rgb.rows,
                        rgb.data, &bmi, DIB_RGB_COLORS, SRCCOPY
                    );
                }
                EndPaint(hWnd, &ps);
                return 0;
            }
            case WM_DESTROY:
                ReleaseVideo();
                return 0;
            }
            return DefWindowProc(hWnd, msg, wParam, lParam);
            };
        wc.hInstance = hInst;
        wc.lpszClassName = runClass;
        RegisterClassExW(&wc);
        isRegistered = true;
    }

    // 4) Створити і показати вікно
    HWND w = CreateWindowW(runClass, L"NN Run", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, GetVideoWidth(), GetVideoHeight(),
        nullptr, nullptr, hInst, nullptr);
    ShowWindow(w, SW_SHOW);
    UpdateWindow(w);

    // 5) Запустити таймер
    SetPlayDirection(1);
}



LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_LBUTTONDOWN:
        isPainting = true;
        dragStart.x = LOWORD(lParam);
        dragStart.y = HIWORD(lParam);
        break;
    case WM_LBUTTONUP:
        if (isPainting) {
            dragEnd.x = LOWORD(lParam);
            dragEnd.y = HIWORD(lParam);
            AddOrRemoveSegmentOnMask(currentFrameIndex, dragStart, dragEnd, false, currentPaintColor);
            isPainting = false;
        }
        break;
    case WM_RBUTTONDOWN:
        isErasing = true;
        dragStart.x = LOWORD(lParam);
        dragStart.y = HIWORD(lParam);
        break;
    case WM_RBUTTONUP:
        if (isErasing) {
            dragEnd.x = LOWORD(lParam);
            dragEnd.y = HIWORD(lParam);
            AddOrRemoveSegmentOnMask(currentFrameIndex, dragStart, dragEnd, true, 0);
            isErasing = false;
        }
        break;
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
        int controlID = lpdis->CtlID;

        if (buttonColors.count(controlID)) {
            COLORREF color = buttonColors[controlID];
            HBRUSH hBrush = CreateSolidBrush(color);

            FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
            FrameRect(lpdis->hDC, &lpdis->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH)); //рамка

            DeleteObject(hBrush);
            return TRUE;
        }
        break;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case ID_BUTTON_NEXT:
            if (currentFrameIndex + 1 < totalFrames)
                ShowFrame(hWnd, currentFrameIndex + 1);
            break;
        case ID_BUTTON_PREV:
            if (currentFrameIndex - 1 >= 0)
                ShowFrame(hWnd, currentFrameIndex - 1);
            break;
        case ID_BUTTON_PLAY_FORWARD:
            playDirection = 1;
            SetTimer(hWnd, TIMER_ID, 40, NULL);
            break;
        case ID_BUTTON_PLAY_BACKWARD:
            playDirection = -1;
            SetTimer(hWnd, TIMER_ID, 40, NULL);
            break;
        case ID_BUTTON_PAUSE:
            playDirection = 0;
            KillTimer(hWnd, TIMER_ID);
            break;
        case ID_BUTTON_SET_FRAME_TYPE:
            //Відображаємо діалогове вікно для вибору типу кадру
            if (MessageBox(hWnd, L"Set frame type to TEST?", L"Set Frame Type", MB_YESNO) == IDYES) {
                FrameType currentFrameType = TEST;
            }
            else {
                FrameType currentFrameType = TRAINING;
            }
            break;
        case ID_BUTTON_TRAIN:
            OnTrainButton();
            break;
        case ID_BUTTON_RUN:
            OnRunButton();
            break;
        case ID_LOAD_BUTTON:
            LoadNeuralNetwork();
            break;
        case 9001:
            WCHAR buffer[100];
            GetWindowTextW(hEditAddType, buffer, 100);
            std::wstring typeName(buffer);
            if (!typeName.empty()) {
                COLORREF typeColor = GenerateUniqueColor(typeColors);
                typeColors.push_back(typeColor);

                int uniqueID = nextTypeID++;
                int yOffset = 100 + types.size() * 35;
                HWND hStatic = CreateWindowW(L"STATIC", typeName.c_str(), WS_VISIBLE | WS_CHILD,
                    680, yOffset, 120, 25, hWnd, NULL, hInst, NULL);

                HWND hDeleteButton = CreateWindowW(L"BUTTON", L"X", WS_VISIBLE | WS_CHILD,
                    830, yOffset, 30, 25, hWnd, (HMENU)uniqueID, hInst, NULL);

                HWND hPaintButton = CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                    800, yOffset, 30, 25, hWnd, (HMENU)(uniqueID + 1000), hInst, NULL);
                buttonColors[uniqueID + 1000] = typeColor;

                types.push_back({ typeName, hStatic, hDeleteButton, hPaintButton, typeColor , uniqueID });

            }
            break;
        }
        int buttonID = LOWORD(wParam);
        if (buttonID >= 1900 and buttonID < 2900) {
            int buttonID = LOWORD(wParam);
            for (size_t i = 0; i < types.size(); ++i) {
                if (types[i].buttonID == buttonID) {
                    DestroyWindow(types[i].hStatic);
                    DestroyWindow(types[i].hDeleteButton);
                    DestroyWindow(types[i].hPaintButton);
                    types.erase(types.begin() + i);

                    //Перемещаем остальные
                    for (size_t j = 0; j < types.size(); ++j) {
                        int newY = 100 + j * 35;
                        SetWindowPos(types[j].hStatic, NULL, 660, newY, 120, 25, SWP_NOZORDER);
                        SetWindowPos(types[j].hDeleteButton, NULL, 810, newY, 30, 25, SWP_NOZORDER);
                        SetWindowPos(types[j].hPaintButton, NULL, 785, newY, 30, 25, SWP_NOZORDER);
                    }
                    break;
                }
            }
        }
        else if (buttonID >= 2900) { //Paint buttons (ID >= 1900 + 1000)
            for (auto& type : types) {
                if ((type.buttonID + 1000) == buttonID) {
                    currentPaintColor = type.color;
                    break;
                }
            }
        }
    }
    break;

    case WM_TIMER:
        if (playDirection == 1 && currentFrameIndex + 1 < totalFrames)
            ShowFrame(hWnd, currentFrameIndex + 1);
        else if (playDirection == -1 && currentFrameIndex - 1 >= 0)
            ShowFrame(hWnd, currentFrameIndex - 1);
        else {
            KillTimer(hWnd, TIMER_ID);
            playDirection = 0;
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT clientRect;
        GetClientRect(hWnd, &clientRect);

        //малюємо шкалу
        DrawProgressBar(hdc, clientRect);

        
        //малюємо кадр
        if (!currentFrame.empty()) {
            BITMAPINFO bmi{};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = currentFrame.cols;
            bmi.bmiHeader.biHeight = -currentFrame.rows;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 24;
            bmi.bmiHeader.biCompression = BI_RGB;

            StretchDIBits(hdc, 20, 120, frameWidth, frameHeight,
                0, 0, currentFrame.cols, currentFrame.rows,
                currentFrame.data, &bmi, DIB_RGB_COLORS, SRCCOPY);
        }

        RECT gridRect = { 20, 120, 20 + frameWidth, 120 + frameHeight };
        DrawGridAndOverlayMask(hdc, gridRect, GetVideoWidth(), GetVideoHeight(), targetPixels, currentFrameIndex);

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        if (cap.isOpened()) cap.release();
        KillTimer(hWnd, TIMER_ID);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SEARCHINGANDANALYZINGUNIQUEOBJECTSINAVIDEOSTREAM, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow, szWindowClass, szTitle, hWndMain)) return FALSE;

    std::string videoPath = OpenVideoFile(hWndMain);
    if (videoPath.empty()) {
        MessageBox(hWndMain, L"No file selected", L"Error", MB_OK);
        PostQuitMessage(0);
        return FALSE;
    }

    if (!LoadVideo(videoPath)) {
        MessageBox(hWndMain, L"Cannot open selected video", L"Error", MB_OK);
        PostQuitMessage(0);
        return FALSE;
    }

    ShowFrame(hWndMain, 0);

    MSG msg;
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SEARCHINGANDANALYZINGUNIQUEOBJECTSINAVIDEOSTREAM));

    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}
