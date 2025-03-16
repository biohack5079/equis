#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <random>
#include <thread>
#include <chrono>
#include <memory>
#include <algorithm>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winmm.lib")

using namespace Gdiplus;

// Function declarations for MP3 playback
bool PlayMP3(const char* filename) {
    char command[256];
    sprintf_s(command, "open \"%s\" type mpegvideo alias bgm", filename);
    if (mciSendStringA(command, NULL, 0, NULL) != 0) {
        MessageBoxW(NULL, L"Failed to open the MP3 file.", L"Error", MB_ICONERROR);
        return false;
    }
    if (mciSendStringA("play bgm repeat", NULL, 0, NULL) != 0) {
        MessageBoxW(NULL, L"Failed to play the MP3 file.", L"Error", MB_ICONERROR);
        return false;
    }
    return true;
}

void StopMP3() {
    mciSendStringA("stop bgm", NULL, 0, NULL);
    mciSendStringA("close bgm", NULL, 0, NULL);
}

// Game constants
const int WINDOW_WIDTH = 1250;
const int WINDOW_HEIGHT = 690;
const int CONTRIBUTION_AMOUNT = 10000000; // 1000万円
const std::vector<long long> PRIZE_DISTRIBUTION = {200000000, 100000000, 100000000, 0, 0, 0};

// Game state
struct GameState {
    std::vector<std::wstring> horseNames;
    std::vector<long long> contributions;
    std::vector<long long> previousResults;
    bool isRacing;
    bool skipConfirmation;
    
    GameState() : 
        horseNames{L"クラウドナイト", L"ダンディオン", L"ルシフェルウィング", 
                  L"アレスフレア", L"レオンハート", L"ゼウスブレイド"},
        contributions(6, 0),
        previousResults(6, 0),
        isRacing(false),
        skipConfirmation(false) {}
};

// UI Resources
struct UIResources {
    std::unique_ptr<Image> bgImage;
    std::vector<std::unique_ptr<Image>> horseImages;
    std::unique_ptr<Image> girlImage;
    int bgX1, bgX2, bgX3;
    
    UIResources() : bgX1(0), bgX2(WINDOW_WIDTH), bgX3(WINDOW_WIDTH * 2) {}
};

class HorseRacingGame {
private:
    HWND hwnd;
    GameState gameState;
    UIResources resources;
    std::thread raceThread;
    std::thread backgroundThread;

public:
    HorseRacingGame(HWND window) : hwnd(window) {
        LoadResources();
        CreateUI();
    }

    void LoadResources() {
        // Load background image
        resources.bgImage = std::unique_ptr<Image>(Image::FromFile(L"0.png"));
        
        // Load horse images
        for (int i = 1; i <= 6; i++) {
            wchar_t filename[20];
            swprintf(filename, 20, L"%d.png", i);
            resources.horseImages.push_back(
                std::unique_ptr<Image>(Image::FromFile(filename)));
        }
        
        // Load girl image
        resources.girlImage = std::unique_ptr<Image>(Image::FromFile(L"7.png"));
    }

    void CreateUI() {
        // Create buttons for each horse
        for (size_t i = 0; i < gameState.horseNames.size(); i++) {
            CreateWindowW(L"BUTTON", gameState.horseNames[i].c_str(),
                         WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                         50, 120 + 50 * i, 200, 30,
                         hwnd, (HMENU)(1000 + i), GetModuleHandle(NULL), NULL);
        }

        // Create start button
        CreateWindowW(L"BUTTON", L"START",
                     WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                     380, 430, 100, 40,
                     hwnd, (HMENU)2000, GetModuleHandle(NULL), NULL);
    }

    void StartRace() {
        if (gameState.isRacing) {
            MessageBoxW(hwnd, L"レース中です！途中で止めると無効になります。", 
                       L"警告", MB_ICONWARNING);
            return;
        }

        if (MessageBoxW(hwnd, L"レースが始まります！最後まで推しを信じて貢ぎましょう！", 
                       L"レース開始", MB_OK) == IDOK) {
            gameState.isRacing = true;
            PlayMP3("race_bgm.mp3");
            
            // Start background scroll thread
            backgroundThread = std::thread([this]() {
                while (gameState.isRacing) {
                    ScrollBackground();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });

            // Start race simulation thread
            raceThread = std::thread([this]() {
                SimulateRace();
            });
        }
    }

    void StopRace() {
        gameState.isRacing = false;
        if (backgroundThread.joinable()) backgroundThread.join();
        if (raceThread.joinable()) raceThread.join();
        StopMP3();
        CalculatePrize();
    }

    void Contribute(int horseIndex) {
        if (horseIndex >= 0 && horseIndex < (int)gameState.horseNames.size()) {
            gameState.contributions[horseIndex] += CONTRIBUTION_AMOUNT;
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }

    void DrawUI(HDC hdc) {
        Graphics graphics(hdc);
        
        // Draw background
        graphics.DrawImage(resources.bgImage.get(), 
                         resources.bgX1, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        graphics.DrawImage(resources.bgImage.get(), 
                         resources.bgX2, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        graphics.DrawImage(resources.bgImage.get(), 
                         resources.bgX3, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        // Draw horse contributions
        DrawContributions(hdc);
        
        // Draw horse images
        DrawHorses(graphics);
        
        // Draw girl image
        graphics.DrawImage(resources.girlImage.get(), 
                         1020, 510, 150, 150);
    }

private:
    void ScrollBackground() {
        resources.bgX1 -= 2;
        resources.bgX2 -= 2;
        resources.bgX3 -= 2;

        if (resources.bgX1 <= -WINDOW_WIDTH) resources.bgX1 = WINDOW_WIDTH * 2;
        if (resources.bgX2 <= -WINDOW_WIDTH) resources.bgX2 = WINDOW_WIDTH * 2;
        if (resources.bgX3 <= -WINDOW_WIDTH) resources.bgX3 = WINDOW_WIDTH * 2;

        InvalidateRect(hwnd, NULL, FALSE);
    }

    void SimulateRace() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, gameState.horseNames.size() - 1);
        
        int delay = 10;
        while (gameState.isRacing) {
            int horseIndex = dis(gen);
            gameState.contributions[horseIndex] += CONTRIBUTION_AMOUNT;
            InvalidateRect(hwnd, NULL, FALSE);
            
            delay = std::max(1, delay - 1);
            std::this_thread::sleep_for(std::chrono::seconds(delay));
        }
    }

    void CalculatePrize() {
        // Sort horses by contribution
        std::vector<size_t> indices(gameState.horseNames.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::sort(indices.begin(), indices.end(),
                 [this](size_t a, size_t b) {
                     return gameState.contributions[a] > gameState.contributions[b];
                 });

        // Calculate total prize money (max 3億円)
        long long totalPrize = 0;
        for (size_t i = 0; i < 3; i++) {
            totalPrize += gameState.contributions[indices[i]];
        }
        totalPrize = std::min(totalPrize, 300000000LL);

        // Display results
        std::wstring result = L"🏆レース結果🏆\n";
        for (size_t i = 0; i < 3; i++) {
            result += gameState.horseNames[indices[i]] + 
                     FormatMoney(gameState.contributions[indices[i]]) + L"\n";
        }
        result += L"\n✨獲得賞金: " + FormatMoney(totalPrize) + L"✨";

        MessageBoxW(hwnd, result.c_str(), L"レース結果", MB_OK);
    }

    std::wstring FormatMoney(long long amount) {
        if (amount >= 100000000) {
            return std::to_wstring(amount / 100000000) + L"億円";
        } else if (amount >= 10000) {
            return std::to_wstring(amount / 10000) + L"万円";
        }
        return std::to_wstring(amount) + L"円";
    }

    void DrawContributions(HDC hdc) {
        SetTextAlign(hdc, TA_LEFT | TA_TOP);
        HFONT hFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                DEFAULT_PITCH | FF_DONTCARE, L"MS Gothic");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        for (size_t i = 0; i < gameState.horseNames.size(); i++) {
            std::wstring text = gameState.horseNames[i] + L": " + 
                              FormatMoney(gameState.contributions[i]);
            TextOutW(hdc, 50, 120 + 50 * i, text.c_str(), text.length());
        }

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }

    void DrawHorses(Graphics& graphics) {
        for (size_t i = 0; i < resources.horseImages.size(); i++) {
            int x = 550 + (i % 3) * 220;
            int y = 90 + (i / 3) * 210;
            graphics.DrawImage(resources.horseImages[i].get(), x, y, 200, 200);
        }
    }
};

// Global variables
ULONG_PTR gdiplusToken;
std::unique_ptr<HorseRacingGame> game;

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            GdiplusStartupInput gdiplusStartupInput;
            GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
            game = std::make_unique<HorseRacingGame>(hwnd);
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id >= 1000 && id < 1006) {
                game->Contribute(id - 1000);
            } else if (id == 2000) {
                game->StartRace();
            }
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            game->DrawUI(hdc);
            EndPaint(hwnd, &ps);
            break;
        }

        case WM_DESTROY: {
            game->StopRace();
            game.reset();
            GdiplusShutdown(gdiplusToken);
            PostQuitMessage(0);
            break;
        }

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// WinMain function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"EquisGame";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Window class registration failed.", L"Error", MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowW(L"EquisGame", L"Equis - Handsome Guys Derby", WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
                             NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBoxW(NULL, L"Window creation failed.", L"Error", MB_ICONERROR);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Start game logic by setting a timer
    SetTimer(hwnd, 1, 30, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}