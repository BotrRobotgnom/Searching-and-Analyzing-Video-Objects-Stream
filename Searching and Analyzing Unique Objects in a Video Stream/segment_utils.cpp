#include "segment_utils.h"
#include "video.h"     //currentFrameIndex, GetVideoWidth/Height()
#include "main.h"      //hWndMain
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

std::pair<int, int> CalculateGridSize(int videoWidth, int videoHeight, int cubeSize) {
    if (cubeSize <= 0) cubeSize = 1;
    if (videoWidth <= 0) videoWidth = cubeSize;
    if (videoHeight <= 0) videoHeight = cubeSize;

    int gridWidth = (videoWidth + cubeSize - 1) / cubeSize;
    int gridHeight = (videoHeight + cubeSize - 1) / cubeSize;

    if (gridWidth == 0) gridWidth = 1;
    if (gridHeight == 0) gridHeight = 1;

    return { gridWidth, gridHeight };
}

//Глобальний контейнер масок
std::unordered_map<int, cv::Mat> frameMasks;

bool EnsureDirectoryExists(const std::string& folderPath) {
    DWORD ftyp = GetFileAttributesA(folderPath.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES) {
        return CreateDirectoryA(folderPath.c_str(), NULL);  //створити папку
    }
    return (ftyp & FILE_ATTRIBUTE_DIRECTORY);  //вже існує
}

static std::string maskFilename(int frameIndex) {
    EnsureDirectoryExists(videoName);
    return videoName + "/frame_" + std::to_string(frameIndex) + ".png";
}

cv::Mat& LoadMaskForFrame(int frameIndex, int videoWidth, int videoHeight) {
    auto it = frameMasks.find(frameIndex);
    if (it != frameMasks.end()) return it->second;

    //Спробуємо завантажити з диску
    std::string fname = maskFilename(frameIndex);
    cv::Mat mask = cv::imread(fname, cv::IMREAD_UNCHANGED);
    if (mask.empty()) {
        //Якщо немає файлу — створимо пусту (прозору/single-color)
        mask = cv::Mat::zeros(videoHeight, videoWidth, CV_8UC3);
    }
    else if (mask.cols != videoWidth || mask.rows != videoHeight) {
        //Якщо розміри не такі як відео — ресайзимо
        cv::resize(mask, mask, cv::Size(videoWidth, videoHeight));
    }
    auto inserted = frameMasks.emplace(frameIndex, mask);
    return inserted.first->second;
}

void SaveMaskForFrame(int frameIndex) {
    auto it = frameMasks.find(frameIndex);
    if (it == frameMasks.end()) return;
    cv::imwrite(maskFilename(frameIndex), it->second);
}

void DrawGridAndOverlayMask(HDC hdc, const RECT& gridRect,
    int videoWidth, int videoHeight,
    int targetPixels, int frameIndex)
{
    //1) Малюємо сітку (як у вас було)
    int width = gridRect.right - gridRect.left;
    int height = gridRect.bottom - gridRect.top;
    auto gridSize = CalculateGridSize(videoWidth, videoHeight, targetPixels);
    int cols = gridSize.first;
    int rows = gridSize.second;

    int cellW = width / cols;
    int cellH = height / rows;

    for (int x = 0; x <= cols; ++x) {
        int px = gridRect.left + x * cellW;
        MoveToEx(hdc, px, gridRect.top, NULL);
        LineTo(hdc, px, gridRect.top + height);
    }
    for (int y = 0; y <= rows; ++y) {
        int py = gridRect.top + y * cellH;
        MoveToEx(hdc, gridRect.left, py, NULL);
        LineTo(hdc, gridRect.left + width, py);
    }

    //2) Накладаємо маску поверх відео
    cv::Mat& mask = LoadMaskForFrame(frameIndex, videoWidth, videoHeight);
    //Конвертуємо cv::Mat (RGB) у DIB поверх HDC
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = videoWidth;
    bmi.bmiHeader.biHeight = -videoHeight; //зворотний порядок
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(hdc,
        gridRect.left, gridRect.top, width, height,
        0, 0, videoWidth, videoHeight,
        mask.data, &bmi, DIB_RGB_COLORS, SRCPAINT);
}

COLORREF GenerateUniqueColor(const std::vector<COLORREF>& existingColors) {
    COLORREF newColor;
    do {
        newColor = RGB(rand() % 256, rand() % 256, rand() % 256);
    } while (std::find(existingColors.begin(), existingColors.end(), newColor) != existingColors.end());
    return newColor;
}

void AddOrRemoveSegmentOnMask(int frameIndex,
    POINT start, POINT end,
    bool erase, COLORREF drawColor)
{
    // Отримуємо розміри відео
    int vw = GetVideoWidth();
    int vh = GetVideoHeight();

    // Графічний прямокутник, у якому відображається відео в основному вікні
    RECT gridRect = { 20, 120, 20 + frameWidth, 120 + frameHeight };

    // Масштабування координат з HDC → координати пікселів відео
    double scaleX = double(vw) / (gridRect.right - gridRect.left);
    double scaleY = double(vh) / (gridRect.bottom - gridRect.top);

    // Переводимо координати початку та кінця перетягування в пікселі відео
    int x1 = int((start.x - gridRect.left) * scaleX);
    int y1 = int((start.y - gridRect.top) * scaleY);
    int x2 = int((end.x - gridRect.left) * scaleX);
    int y2 = int((end.y - gridRect.top) * scaleY);

    // Обмежуємо координати, щоб вони не виходили за межі відео
    if (x1 < 0) x1 = 0;
    if (x1 > vw - 1) x1 = vw - 1;
    if (y1 < 0) y1 = 0;
    if (y1 > vh - 1) y1 = vh - 1;

    if (x2 < 0) x2 = 0;
    if (x2 > vw - 1) x2 = vw - 1;
    if (y2 < 0) y2 = 0;
    if (y2 > vh - 1) y2 = vh - 1;

    // Визначаємо область в пікселях відео, яку вибрав користувач
    int rx = (x1 < x2 ? x1 : x2);
    int ry = (y1 < y2 ? y1 : y2);
    int rw = abs(x2 - x1) + 1;
    int rh = abs(y2 - y1) + 1;

    // Підготуємо маску для поточного кадру
    cv::Mat& mask = LoadMaskForFrame(frameIndex, vw, vh);

    // Визначаємо діапазон grid-індексів по горизонталі/вертикалі
    int cellX1 = rx / targetPixels;
    int cellY1 = ry / targetPixels;
    int cellX2 = (rx + rw - 1) / targetPixels;
    int cellY2 = (ry + rh - 1) / targetPixels;

    // Обмежуємо ці індекси, щоб не виходили за межі сітки
    int maxCellX = (vw - 1) / targetPixels;
    int maxCellY = (vh - 1) / targetPixels;

    if (cellX1 < 0) cellX1 = 0;
    if (cellX1 > maxCellX) cellX1 = maxCellX;
    if (cellY1 < 0) cellY1 = 0;
    if (cellY1 > maxCellY) cellY1 = maxCellY;

    if (cellX2 < 0) cellX2 = 0;
    if (cellX2 > maxCellX) cellX2 = maxCellX;
    if (cellY2 < 0) cellY2 = 0;
    if (cellY2 > maxCellY) cellY2 = maxCellY;

    // Якщо режим "стирання" – закрашуємо клітинки чорною,
    // інакше – кольором drawColor
    if (erase) {
        for (int cy = cellY1; cy <= cellY2; ++cy) {
            for (int cx = cellX1; cx <= cellX2; ++cx) {
                int px = cx * targetPixels;
                int py = cy * targetPixels;
                int pw = ((px + targetPixels) <= vw) ? targetPixels : (vw - px);
                int ph = ((py + targetPixels) <= vh) ? targetPixels : (vh - py);
                cv::Rect cellRect(px, py, pw, ph);
                cv::rectangle(mask, cellRect, cv::Scalar(0, 0, 0), cv::FILLED);
            }
        }
    }
    else {
        int b = GetBValue(drawColor);
        int g = GetGValue(drawColor);
        int r = GetRValue(drawColor);
        for (int cy = cellY1; cy <= cellY2; ++cy) {
            for (int cx = cellX1; cx <= cellX2; ++cx) {
                int px = cx * targetPixels;
                int py = cy * targetPixels;
                int pw = ((px + targetPixels) <= vw) ? targetPixels : (vw - px);
                int ph = ((py + targetPixels) <= vh) ? targetPixels : (vh - py);
                cv::Rect cellRect(px, py, pw, ph);
                cv::rectangle(mask, cellRect, cv::Scalar(b, g, r), cv::FILLED);
            }
        }
    }

    // Зберігаємо змінену маску та оновлюємо вікно
    SaveMaskForFrame(frameIndex);
    InvalidateRect(hWndMain, NULL, TRUE);
}
