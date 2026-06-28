#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <unordered_map>

#include <opencv2/core.hpp>

extern int targetPixels;
extern std::string videoName;

///Маска для одного кадру: CV_8UC3 з тими ж розмірами, що й відео.
extern std::unordered_map<int, cv::Mat> frameMasks;

///Малює сітку + накладає маску з frameMasks[frameIndex]
void DrawGridAndOverlayMask(HDC hdc, const RECT& gridRect, int videoWidth, int videoHeight,
    int targetPixels, int frameIndex);

///Завантажити (або створити порожню) маску для кадру frameIndex
cv::Mat& LoadMaskForFrame(int frameIndex, int videoWidth, int videoHeight);

///Зберегти маску для кадру frameIndex у PNG
void SaveMaskForFrame(int frameIndex);

///Додати/стерти прямокутник на масці поточного кадру
void AddOrRemoveSegmentOnMask(int frameIndex, POINT start, POINT end,
    bool erase, COLORREF drawColor);

COLORREF GenerateUniqueColor(const std::vector<COLORREF>& existingColors);