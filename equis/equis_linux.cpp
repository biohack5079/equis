#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h> // Include SDL_ttf
#include <string>
#include <vector>
#include <random>
#include <thread>
#include <chrono>
#include <memory>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>

// Game constants
const int WINDOW_WIDTH = 1250;
const int WINDOW_HEIGHT = 690;
const int CONTRIBUTION_AMOUNT = 10000000; // 1000万円
const std::vector<long long> PRIZE_DISTRIBUTION = {200000000, 100000000, 100000000, 0, 0, 0};

// Game state
struct GameState {
    std::vector<std::string> horseNames;
    std::vector<long long> contributions;
    std::vector<long long> previousResults;
    bool isRacing;
    bool skipConfirmation;

    GameState() :
        horseNames{"クラウドナイト", "ダンディオン", "ルシフェルウィング",
                  "アレスフレア", "レオンハート", "ゼウスブレイド"},
        contributions(6, 0),
        previousResults(6, 0),
        isRacing(false),
        skipConfirmation(false) {}
};

// UI Resources
struct UIResources {
    SDL_Texture* bgImage;
    std::vector<SDL_Texture*> horseImages;
    SDL_Texture* girlImage;
    int bgX1, bgX2, bgX3;
    TTF_Font* font; // Add font

    UIResources() : bgImage(nullptr), girlImage(nullptr), bgX1(0), bgX2(WINDOW_WIDTH), bgX3(WINDOW_WIDTH * 2), font(nullptr) {}
};

class HorseRacingGame {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    GameState gameState;
    UIResources resources;
    std::thread raceThread;
    std::thread backgroundThread;
    Mix_Music* bgm;
    bool resourcesLoaded;

    // Helper function to render text
    SDL_Texture* RenderText(const std::string& text, SDL_Color color) {
        if (!resources.font) {
            std::cerr << "Error: Font not loaded!" << std::endl;
            return nullptr;
        }
        SDL_Surface* surface = TTF_RenderUTF8_Blended(resources.font, text.c_str(), color);
        if (!surface) {
            std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
            return nullptr;
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        return texture;
    }

public:
    HorseRacingGame(SDL_Window* window, SDL_Renderer* renderer) :
        window(window),
        renderer(renderer),
        bgm(nullptr),
        resourcesLoaded(false) {

        // Check for required files before loading
        CheckRequiredFiles();

        // Try to load resources
        resourcesLoaded = LoadResources();
        if (!resourcesLoaded) {
            std::cerr << "Failed to load all required resources. Game may not work properly." << std::endl;
        } else {
            std::cout << "All resources loaded successfully!" << std::endl;
        }
    }

    ~HorseRacingGame() {
        // Free resources
        if (resources.bgImage) SDL_DestroyTexture(resources.bgImage);
        for (auto& horseImage : resources.horseImages) {
            if (horseImage) SDL_DestroyTexture(horseImage);
        }
        if (resources.girlImage) SDL_DestroyTexture(resources.girlImage);
        if (bgm != nullptr) {
            Mix_FreeMusic(bgm);
        }
        if (resources.font) {
            TTF_CloseFont(resources.font);
        }
    }

    void CheckRequiredFiles() {
        std::vector<std::string> requiredFiles = {
            "0.png", "1.png", "2.png", "3.png", "4.png", "5.png", "6.png", "7.png", "race_bgm.mp3"
        };

        std::cout << "Checking for required files:" << std::endl;
        for (const auto& file : requiredFiles) {
            bool exists = std::filesystem::exists(file);
            std::cout << "  " << file << ": " << (exists ? "Found" : "MISSING") << std::endl;
        }
    }

    bool LoadResources() {
        bool success = true;

        // Load background image
        SDL_Surface* surface = IMG_Load("0.png");
        if (!surface) {
            std::cerr << "Failed to load image: 0.png, Error: " << IMG_GetError() << std::endl;
            success = false;
        } else {
            resources.bgImage = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
            if (!resources.bgImage) {
                std::cerr << "Failed to create texture from surface (0.png), Error: " << SDL_GetError() << std::endl;
                success = false;
            }
        }

        // Load horse images
        for (int i = 1; i <= 6; i++) {
            char filename[20];
            sprintf(filename, "%d.png", i);
            surface = IMG_Load(filename);
            if (!surface) {
                std::cerr << "Failed to load image: " << filename << ", Error: " << IMG_GetError() << std::endl;
                success = false;
                resources.horseImages.push_back(nullptr); // Add placeholder to maintain index integrity
            } else {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
                if (!texture) {
                    std::cerr << "Failed to create texture from surface (" << filename << "), Error: " << SDL_GetError() << std::endl;
                    success = false;
                    resources.horseImages.push_back(nullptr);
                } else {
                    resources.horseImages.push_back(texture);
                }
            }
        }

        // Load girl image
        surface = IMG_Load("7.png");
        if (!surface) {
            std::cerr << "Failed to load image: 7.png, Error: " << IMG_GetError() << std::endl;
            success = false;
        } else {
            resources.girlImage = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
            if (!resources.girlImage) {
                std::cerr << "Failed to create texture from surface (7.png), Error: " << SDL_GetError() << std::endl;
                success = false;
            }
        }
        
        // Load font
        resources.font = TTF_OpenFont("KaiseiTokumin-Bold.ttf", 24); // Replace with your font file
        if (!resources.font) {
            std::cerr << "Failed to load font: KaiseiTokumin-Bold.ttf, Error: " << TTF_GetError() << std::endl;
            success = false;
        }

        return success;
    }

    void StartRace() {
        if (!resourcesLoaded) {
            std::cout << "リソースの読み込みに失敗しているため、レースを開始できません。" << std::endl;
            return;
        }

        if (gameState.isRacing) {
            std::cout << "レース中です！途中で止めると無効になります。" << std::endl;
            return;
        }

        std::cout << "レースが始まります！最後まで推しを信じて貢ぎましょう！" << std::endl;
        gameState.isRacing = true;

        // Try to load and play BGM
        bgm = Mix_LoadMUS("race_bgm.mp3");
        if (bgm == NULL) {
            std::cerr << "Failed to load music: race_bgm.mp3, Error: " << Mix_GetError() << std::endl;
            std::cout << "音楽なしでレースを続行します。" << std::endl;
        } else {
            if (Mix_PlayMusic(bgm, -1) == -1) {
                std::cerr << "Failed to play music, Error: " << Mix_GetError() << std::endl;
            }
        }

        // Start background scroll thread
        try {
            backgroundThread = std::thread([this]() {
                while (gameState.isRacing) {
                    ScrollBackground();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        } catch (const std::exception& e) {
            std::cerr << "Failed to start background thread: " << e.what() << std::endl;
            gameState.isRacing = false;
        }

        // Start race simulation thread
        try {
            raceThread = std::thread([this]() {
                SimulateRace();
            });
        } catch (const std::exception& e) {
            std::cerr << "Failed to start race thread: " << e.what() << std::endl;
            gameState.isRacing = false;
        }
    }

    void StopRace() {
        gameState.isRacing = false;

        try {
            if (backgroundThread.joinable()) backgroundThread.join();
            if (raceThread.joinable()) raceThread.join();
        } catch (const std::exception& e) {
            std::cerr << "Error joining threads: " << e.what() << std::endl;
        }

        if (bgm != NULL) {
            Mix_HaltMusic();
            Mix_FreeMusic(bgm);
            bgm = nullptr;
        }

        CalculatePrize();
    }

    void Contribute(int horseIndex) {
        if (horseIndex >= 0 && horseIndex < (int)gameState.horseNames.size()) {
            gameState.contributions[horseIndex] += CONTRIBUTION_AMOUNT;
        }
    }

    void DrawUI() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw background if texture exists
        if (resources.bgImage) {
            SDL_Rect bgRect = {resources.bgX1, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
            SDL_RenderCopy(renderer, resources.bgImage, NULL, &bgRect);
            bgRect = {resources.bgX2, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
            SDL_RenderCopy(renderer, resources.bgImage, NULL, &bgRect);
            bgRect = {resources.bgX3, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
            SDL_RenderCopy(renderer, resources.bgImage, NULL, &bgRect);
        } else {
            // Draw a fallback background if texture is missing
            SDL_SetRenderDrawColor(renderer, 100, 149, 237, 255); // Cornflower blue
            SDL_Rect bgRect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
            SDL_RenderFillRect(renderer, &bgRect);
        }

        // Draw horse contributions
        DrawContributions();

        // Draw horse images
        DrawHorses();

        // Draw girl image if texture exists
        if (resources.girlImage) {
            SDL_Rect girlRect = {1020, 510, 150, 150};
            SDL_RenderCopy(renderer, resources.girlImage, NULL, &girlRect);
        }

        // Draw simple debug text
        DrawDebugInfo();
        
        // Draw race result
        if (!gameState.isRacing) {
            DrawRaceResult();
        }

        SDL_RenderPresent(renderer);
    }

    void ShowContributionDialog() {
        std::cout << "どの馬に貢ぎますか？" << std::endl;
        for (size_t i = 0; i < gameState.horseNames.size(); ++i) {
            std::cout << i + 1 << ". " << gameState.horseNames[i] << std::endl;
        }

        int choice;
        std::cout << "番号を入力してください (1-6): ";
        std::cin >> choice;

        if (choice >= 1 && choice <= (int)gameState.horseNames.size()) {
            Contribute(choice - 1);
        } else {
            std::cout << "無効な選択です。1から6の数字を入力してください。" << std::endl;
        }
    }

private:
    void ScrollBackground() {
        resources.bgX1 -= 2;
        resources.bgX2 -= 2;
        resources.bgX3 -= 2;

        if (resources.bgX1 <= -WINDOW_WIDTH) resources.bgX1 = WINDOW_WIDTH * 2;
        if (resources.bgX2 <= -WINDOW_WIDTH) resources.bgX2 = WINDOW_WIDTH * 2;
        if (resources.bgX3 <= -WINDOW_WIDTH) resources.bgX3 = WINDOW_WIDTH * 2;
    }

    void SimulateRace() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, gameState.horseNames.size() - 1);

        int delay = 10;
        while (gameState.isRacing) {
            int horseIndex = dis(gen);
            gameState.contributions[horseIndex] += CONTRIBUTION_AMOUNT;

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
        
        // Store results
        for (size_t i = 0; i < gameState.horseNames.size(); ++i) {
            gameState.previousResults[i] = gameState.contributions[i];
        }
    }

    std::string FormatMoney(long long amount) {
        std::string result;
        if (amount >= 100000000) {
            long long oku = amount / 100000000;
            long long man = (amount % 100000000) / 10000;
            if (man == 0) {
                result = std::to_string(oku) + "億円";
            } else {
                result = std::to_string(oku) + "億" + std::to_string(man) + "万円";
            }
        } else if (amount >= 10000) {
            result = std::to_string(amount / 10000) + "万円";
        } else {
            result = std::to_string(amount) + "円";
        }
        return result;
    }

    void DrawContributions() {
        // Draw contributions on screen
        if (!gameState.isRacing) {
            int y = 10;
            int x = 10;
            SDL_Color color = {255, 255, 255, 255};
            std::string contributionsText = "現在の貢ぎ額:";
            for (size_t i = 0; i < gameState.horseNames.size(); i++) {
                contributionsText += gameState.horseNames[i] + ": " + FormatMoney(gameState.contributions[i]);
                if (i < gameState.horseNames.size() - 1) {
                    contributionsText += ", ";
                }
            }
            SDL_Texture* textTexture = RenderText(contributionsText, color);
            if (textTexture) {
                SDL_Rect textRect = {x, y, 0, 0};
                SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h);
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
        }
    }
    
    void DrawRaceResult() {
        // Draw race result on screen
        int y = 400;
        int x = 10;
        SDL_Color color = {255, 255, 255, 255};
        
        // Sort horses by contribution
        std::vector<size_t> indices(gameState.horseNames.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::sort(indices.begin(), indices.end(),
                 [this](size_t a, size_t b) {
                     return gameState.previousResults[a] > gameState.previousResults[b];
                 });
        
        // Display results
        std::string resultText = "🏆レース結果🏆";
        SDL_Texture* textTexture = RenderText(resultText, color);
        if (textTexture) {
            SDL_Rect textRect = {x, y, 0, 0};
            SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h);
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
        }
        y += 30;
        for (size_t i = 0; i < 3; i++) {
            resultText = std::to_string(i+1) + "位: " + gameState.horseNames[indices[i]] + " - " + FormatMoney(gameState.previousResults[indices[i]]);
            textTexture = RenderText(resultText, color);
            if (textTexture) {
                SDL_Rect textRect = {x, y, 0, 0};
                SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h);
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            y += 30;
        }
        
        long long totalPrize = 0;
        for (size_t i = 0; i < 3; i++) {
            totalPrize += gameState.previousResults[indices[i]];
        }
        totalPrize = std::min(totalPrize, 300000000LL);
        
        resultText = "✨獲得賞金: " + FormatMoney(totalPrize) + "✨";
        textTexture = RenderText(resultText, color);
        if (textTexture) {
            SDL_Rect textRect = {x, y, 0, 0};
            SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h);
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
        }
    }

    void DrawHorses() {
        for (size_t i = 0; i < resources.horseImages.size(); i++) {
            int x = 550 + (i % 3) * 220;
            int y = 90 + (i / 3) * 210;
            SDL_Rect horseRect = {x, y, 200, 200};

            if (resources.horseImages[i]) {
                SDL_RenderCopy(renderer, resources.horseImages[i], NULL, &horseRect);
            } else {
                // Draw placeholder if texture is missing
                SDL_SetRenderDrawColor(renderer, 200, 100, 100, 255);
                SDL_RenderFillRect(renderer, &horseRect);
            }
        }
    }

    void DrawDebugInfo() {
        // Draw debug info on screen
        int y = WINDOW_HEIGHT - 30;
        int x = 10;
        SDL_Color color = {255, 255, 255, 255};
        std::string debugText;
        if (!gameState.isRacing) {
            debugText = "ゲーム状態: 待機中 | スペースキーでレース開始 | Cキーで馬に貢ぐ | ESCで終了";
        } else {
            debugText = "ゲーム状態: レース中...";
        }
        SDL_Texture* textTexture = RenderText(debugText, color);
        if (textTexture) {
            SDL_Rect textRect = {x, y, 0, 0};
            SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h);
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
        }
    }
};

int main(int argc, char* argv[]) {
    // Print SDL versions for debugging
    SDL_version compiled;
    SDL_VERSION(&compiled);
    std::cout << "SDL Compiled version: " << (int)compiled.major << "." << (int)compiled.minor << "." << (int)compiled.patch << std::endl;

    SDL_version linked;
    SDL_GetVersion(&linked);
    std::cout << "SDL Linked version: " << (int)linked.major << "." << (int)linked.minor << "." << (int)linked.patch << std::endl;

    // Initialize SDL with error checking
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    std::cout << "SDL initialized successfully." << std::endl;

    // Initialize SDL_mixer with error checking
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        std::cout << "Game will continue without sound." << std::endl;
    } else {
        std::cout << "SDL_mixer initialized successfully." << std::endl;
    }

    // Initialize SDL_image with error checking
    int imgFlags = IMG_INIT_PNG;
    int imgInitResult = IMG_Init(imgFlags);
    if (!(imgInitResult & imgFlags)) {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        std::cerr << "Requested: " << imgFlags << ", Got: " << imgInitResult << std::endl;
        return 1;
    }
    std::cout << "SDL_image initialized successfully." << std::endl;
    
    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return 1;
    }
    std::cout << "SDL_ttf initialized successfully." << std::endl;

    // Create window with error checking
    SDL_Window* window = SDL_CreateWindow("Equis - Handsome Guys Derby",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          WINDOW_WIDTH,
                                          WINDOW_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (window == NULL) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    std::cout << "Window created successfully." << std::endl;

    // Create renderer with error checking
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    std::cout << "Renderer created successfully." << std::endl;

    // Print current working directory
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;

    // Create and run game
    std::cout << "Creating game instance..." << std::endl;
    HorseRacingGame game(window, renderer);

    std::cout << "\nGame Controls:" << std::endl;
    std::cout << "  Space - Start race" << std::endl;
    std::cout << "  C     - Contribute to a horse" << std::endl;
    std::cout << "  ESC   - Quit game" << std::endl;
    std::cout << "\nPress any key to continue..." << std::endl;

    // 初期画面を表示
    game.DrawUI();

    std::cout << "Starting game loop..." << std::endl;
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        bool needRedraw = false;
        while (SDL_PollEvent(&e) != 0) {
            needRedraw = true;
            if (e.type == SDL_QUIT) {
                quit = true;
                game.StopRace(); // ゲーム終了時にスレッドを停止
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_SPACE) {
                    game.StartRace();
                } else if (e.key.keysym.sym == SDLK_c) {
                    game.ShowContributionDialog();
                } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                    game.StopRace(); // ゲーム終了時にスレッドを停止
                }
            }
        }
        if(needRedraw){
            game.DrawUI();
        }
        SDL_Delay(16); // Approx 60 FPS
    }

    std::cout << "Game loop ended." << std::endl;

    std::cout << "Cleaning up resources..." << std::endl;
    //game.StopRace(); // ゲームループ内で停止しているので、ここでは不要
    //SDL_DestroyRenderer(renderer); // ~HorseRacingGame()で解放しているので、ここでは不要
    //SDL_DestroyWindow(window); // ~HorseRacingGame()で解放しているので、ここでは不要
    TTF_Quit();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
    std::cout << "Cleanup complete. Exiting game." << std::endl;

    return 0;
}
