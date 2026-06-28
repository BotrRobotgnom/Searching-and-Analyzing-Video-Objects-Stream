#pragma once

#include <opencv2/opencv.hpp>
#include <windows.h>
#include <vector>
#include <string>

extern int currentFrameIndex;
extern int totalFrames;
extern int playDirection;
extern int frameWidth;
extern int frameHeight;


extern cv::Mat currentFrame;
extern cv::VideoCapture cap;
std::string OpenVideoFile(HWND owner);

//Ініціалізація відео (відкриття файлу)
bool LoadVideo(const std::string& path);

//Показ кадру
void ShowFrame(HWND hWnd, int frameIndex);

//Отримання поточного кадру
const cv::Mat& GetCurrentFrame();

//Встановлення напрямку програвання
void SetPlayDirection(int direction);

//Оновлення кадру під час таймера
bool UpdateFrame(HWND hWnd);

//Отримання загальної кількості кадрів
int GetTotalFrames();

//Отримання поточного індексу кадру
int GetCurrentFrameIndex();

//Очистка відеоресурсів
void ReleaseVideo();

//Отримання списку розмічених кадрів
const std::vector<int>& GetMarkedFrames();

//Додавання мітки
void MarkCurrentFrame();

//Побудова шкали прогресу
void DrawProgressBar(HDC hdc, RECT clientRect);

int GetVideoWidth();
int GetVideoHeight();