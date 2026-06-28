#pragma once

#include "resource.h"
#include <windows.h>
#include <string>
#include <vector>

//Ідентифікатори елементів керування
#define ID_BUTTON_NEXT         1001
#define ID_BUTTON_PREV         1002
#define ID_BUTTON_PLAY_FORWARD 1003
#define ID_BUTTON_PLAY_BACKWARD 1004
#define ID_BUTTON_PAUSE        1005
#define ID_BUTTON_SET_FRAME_TYPE 1006

#define ID_BUTTON_TRAIN         1007
#define ID_BUTTON_RUN           1008
#define ID_LOAD_BUTTON          1009

#define TIMER_ID               1

//Глобальні змінні
extern HINSTANCE hInst;
extern HWND hWndMain;

//Функції з main.cpp
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int, LPCWSTR, LPCWSTR, HWND& hWnd);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

enum FrameType {
    TRAINING = 0,
    TEST = 1
};

struct TypeEntry {
    std::wstring name;
    HWND hStatic;
    HWND hDeleteButton;
    HWND hPaintButton;
    COLORREF color;
    int buttonID;
};

extern std::vector<TypeEntry> addedTypes;
extern FrameType currentFrameType;