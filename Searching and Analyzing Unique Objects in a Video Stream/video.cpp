#include "video.h"
#include <windows.h>
#include <commdlg.h>

cv::VideoCapture cap;
cv::Mat currentFrame;
int currentFrameIndex = 0;
int totalFrames = 0;
int playDirection = 0;
std::vector<int> markedFrames;

int videoWidth = 0;
int videoHeight = 0;

std::string videoFullPath;
std::string videoName;

std::string ExtractName(const std::string& path) {
    size_t lastSlash = path.find_last_of("/\\");
    std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);
    size_t dot = filename.find_last_of('.');
    return (dot == std::string::npos) ? filename : filename.substr(0, dot);
}

std::string OpenVideoFile(HWND owner)
{
    OPENFILENAMEW ofn = { 0 };
    wchar_t szFile[MAX_PATH] = { 0 };

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Video Files (*.avi;*.mp4;*.mov)\0*.avi;*.mp4;*.mov\0All Files\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        std::wstring ws(ofn.lpstrFile);
        videoFullPath = std::string(ws.begin(), ws.end());
        videoName = ExtractName(videoFullPath);
        return videoFullPath;
    }
    return "";
}


bool LoadVideo(const std::string& path) {
    cap.open(path);
    if (!cap.isOpened()) return false;

    totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
    videoWidth = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    videoHeight = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    ShowFrame(nullptr, 0);
    return true;
}

int GetVideoWidth() {
    return videoWidth;
}

int GetVideoHeight() {
    return videoHeight;
}

void ShowFrame(HWND hWnd, int frameIndex) {
    if (frameIndex < 0 || frameIndex >= totalFrames) return;

    cap.set(cv::CAP_PROP_POS_FRAMES, frameIndex);
    if (!cap.read(currentFrame) || currentFrame.empty()) return;

    currentFrameIndex = frameIndex;

    if (hWnd) {
        RECT r = { 20, 120-50, 20 + frameWidth, 120 + frameHeight };
        InvalidateRect(hWnd, &r, FALSE);
    }
}

const cv::Mat& GetCurrentFrame() {
    return currentFrame;
}

void SetPlayDirection(int direction) {
    playDirection = direction;
}

bool UpdateFrame(HWND hWnd) {
    if (playDirection == 1 && currentFrameIndex + 1 < totalFrames) {
        ShowFrame(hWnd, currentFrameIndex + 1);
        return true;
    }
    else if (playDirection == -1 && currentFrameIndex - 1 >= 0) {
        ShowFrame(hWnd, currentFrameIndex - 1);
        return true;
    }
    return false;
}

int GetTotalFrames() {
    return totalFrames;
}

int GetCurrentFrameIndex() {
    return currentFrameIndex;
}

void ReleaseVideo() {
    if (cap.isOpened()) cap.release();
}

const std::vector<int>& GetMarkedFrames() {
    return markedFrames;
}

void MarkCurrentFrame() {
    if (std::find(markedFrames.begin(), markedFrames.end(), currentFrameIndex) == markedFrames.end())
        markedFrames.push_back(currentFrameIndex);
}

void DrawProgressBar(HDC hdc, RECT clientRect) {
    RECT barRect = { 20, 70, 640, 90 };

    HBRUSH bgBrush = CreateSolidBrush(RGB(230, 230, 230));
    FillRect(hdc, &barRect, bgBrush);
    DeleteObject(bgBrush);

    FrameRect(hdc, &barRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

    if (totalFrames > 0) {
        int progressWidth = (currentFrameIndex * (barRect.right - barRect.left)) / totalFrames;
        RECT progressRect = { barRect.left, barRect.top, barRect.left + progressWidth, barRect.bottom };
        HBRUSH progBrush = CreateSolidBrush(RGB(100, 149, 237)); //cornflower blue
        FillRect(hdc, &progressRect, progBrush);
        DeleteObject(progBrush);
    }

    for (int mark : markedFrames) {
        int x = barRect.left + (mark * (barRect.right - barRect.left)) / totalFrames;
        MoveToEx(hdc, x, barRect.top, NULL);
        LineTo(hdc, x, barRect.bottom);
    }
}
