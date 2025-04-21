// gui.c
#include "../include/gui.h"
#include "../include/client_logic.h"
#include "../include/resources.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <string.h>

// 窗口尺寸常量
#define WINDOW_WIDTH 1280    ///< 窗口宽度
#define WINDOW_HEIGHT 768    ///< 窗口高度
#define ICON_SIZE 32         ///< 图标尺寸
#define CARD_WIDTH 300       ///< 卡牌宽度
#define CARD_HEIGHT 200      ///< 卡牌高度
#define BUTTON_WIDTH 130     ///< 按钮宽度
#define BUTTON_HEIGHT 40     ///< 按钮高度
#define BUTTON_O_X 650
#define BUTTON_O_Y 100
#define BUTTON_S_X 650
#define BUTTON_S_Y 160
#define BUTTON_G_X 650 
#define BUTTON_G_Y 220

static int showEndDialog = 0;     ///< 是否显示结束对话框标志
static PopupState popupState = POPUP_NONE;  ///< 当前弹窗状态

// Tracking Mouse State
static int mouseX = 0, mouseY = 0;
static int hoverO = 0, hoverS = 0, hoverG = 0;

// 记录选择弹窗中的对象与玩家
static int selectedObjectId = -1;
static int selectedPlayerId = -1;
static int selectedGuessCard = -1;

/**
 * @brief 渲染文本到指定位置
 * 
 * @param renderer SDL渲染器
 * @param font 字体对象
 * @param text 要渲染的文本
 * @param x 文本左上角x坐标
 * @param y 文本左上角y坐标
 * @param color 文本颜色
 */
void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!text || strlen(text) == 0) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect rect = {x, y, surf->w, surf->h};
    SDL_RenderCopy(renderer, tex, NULL, &rect);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

/**
 * @brief 渲染图标到指定位置
 * 
 * @param renderer SDL渲染器
 * @param tex 图标纹理
 * @param x 图标左上角x坐标
 * @param y 图标左上角y坐标
 * @param size 图标尺寸
 */
void render_icon(SDL_Renderer *renderer, SDL_Texture *tex, int x, int y, int size) {
    if (!tex) return;
    SDL_Rect dst = {x, y, size, size};
    SDL_RenderCopy(renderer, tex, NULL, &dst);
}
 
/**
 * @brief 渲染玩家持有的卡牌
 * 
 * @param renderer SDL渲染器
 * @param cards 卡牌纹理数组
 * @param mycards 玩家持有的卡牌索引数组
 * @param cardCount 卡牌数量
 */
void render_cards(SDL_Renderer* renderer, SDL_Texture* cards[], int* mycards, int cardCount) {
    for (int i = 0; i < cardCount; ++i) {
        int idx = mycards[i];
        if (idx >= 0 && idx < 13) {
            SDL_Rect dst = {940, 50 + i * 230, CARD_WIDTH, CARD_HEIGHT};
            SDL_RenderCopy(renderer, cards[idx], NULL, &dst);
        }
    }
}
 
/**
 * @brief 绘制选择弹窗(用于观察/推测操作)
 * 
 * @param renderer SDL渲染器
 * @param font 字体对象
 */
void draw_selection_popup(SDL_Renderer* renderer, TTF_Font* font) {
    // 绘制弹窗背景
    SDL_Rect popup = {180, 100, 440, 380};
    SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
    SDL_RenderFillRect(renderer, &popup);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &popup);

    SDL_Color black = {0, 0, 0};

    // 显示对象选择（O与S共用）
    render_text(renderer, font, "Objets:", 200, 120, black);
    for (int i = 0; i < 8; ++i) {
        SDL_Rect box = {200, 160 + i * 25, 200, 22};
        if (i == selectedObjectId) {
            SDL_SetRenderDrawColor(renderer, 0, 200, 0, 100);
            SDL_RenderFillRect(renderer, &box);
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &box);

        char label[32];
        snprintf(label, sizeof(label), "%d: %s", i + 1, nameobjets[i]);
        render_text(renderer, font, label, 205, 155 + i * 25, black);
    }

    // 仅当 S 操作需要显示玩家选择
    if (popupState == POPUP_S_SELECT_OBJ || popupState == POPUP_S_SELECT_PLAYER) {
        render_text(renderer, font, "Joueurs:", 420, 120, black);
        int pc = getPlayerCount();
        for (int i = 0; i < pc; ++i) {
            SDL_Rect box = {420, 160 + i * 25, 180, 22};
            if (i == selectedPlayerId) {
                SDL_SetRenderDrawColor(renderer, 200, 200, 0, 100);
                SDL_RenderFillRect(renderer, &box);
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &box);

            char label[32];
            snprintf(label, sizeof(label), "%d: %s", i + 1, getPlayerName(i));
            render_text(renderer, font, label, 425, 155 + i * 25, black);
        }
    }

    // 确认和取消按钮
    SDL_Rect confirm = {240, 380, 80, 30};
    SDL_Rect cancel  = {360, 380, 80, 30};
    SDL_SetRenderDrawColor(renderer, 200, 255, 200, 255);
    SDL_RenderFillRect(renderer, &confirm);
    SDL_RenderDrawRect(renderer, &confirm);
    render_text(renderer, font, "OK", confirm.x + 25, confirm.y + 5, black);

    SDL_SetRenderDrawColor(renderer, 255, 200, 200, 255);
    SDL_RenderFillRect(renderer, &cancel);
    SDL_RenderDrawRect(renderer, &cancel);
    render_text(renderer, font, "Cancel", cancel.x + 10, cancel.y + 5, black);
}
 
 /**
  * @brief 绘制猜测弹窗
  * 
  * @param renderer SDL渲染器
  * @param font 字体对象
  */
 void draw_guess_popup(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Rect popup = {180, 120, 900, 480};
    SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
    SDL_RenderFillRect(renderer, &popup);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &popup);

    SDL_Color black = {0, 0, 0};
    render_text(renderer, font, "Cliquez sur la carte que vous soupçonnez être le coupable :", 200, 130, black);

    for (int i = 0; i < 13; ++i) {
        int row = i / 7;
        int col = i % 7;
        int x = 200 + col * 120;
        int y = 180 + row * 160;

        SDL_Rect cardRect = {x, y, 100, 140};
        if (i == selectedGuessCard) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 150);
            SDL_RenderFillRect(renderer, &cardRect);
        }
        SDL_RenderCopy(renderer, cards[i], NULL, &cardRect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &cardRect);
    }

    // OK / Cancel buttons
    SDL_Rect okBtn = {320, 440, 80, 30};
    SDL_Rect cancelBtn = {440, 440, 80, 30};
    SDL_SetRenderDrawColor(renderer, 200, 255, 200, 255);
    SDL_RenderFillRect(renderer, &okBtn);
    SDL_RenderDrawRect(renderer, &okBtn);
    render_text(renderer, font, "OK", okBtn.x + 25, okBtn.y + 5, black);

    SDL_SetRenderDrawColor(renderer, 255, 200, 200, 255);
    SDL_RenderFillRect(renderer, &cancelBtn);
    SDL_RenderDrawRect(renderer, &cancelBtn);
    render_text(renderer, font, "Cancel", cancelBtn.x + 10, cancelBtn.y + 5, black);
}
 
 /**
  * @brief 显示游戏结束弹窗
  * 
  * @param renderer SDL渲染器
  * @param font 字体对象
  * @param result 游戏结果文本
  */
 void show_end_popup(SDL_Renderer* renderer, TTF_Font* font, const char* result) {
     if (!showEndDialog) return;
     
     // 绘制弹窗背景
     SDL_Rect popup = {180, 180, 420, 180};
     SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
     SDL_RenderFillRect(renderer, &popup);
     SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
     SDL_RenderDrawRect(renderer, &popup);
 
     SDL_Color red = {255, 0, 0};
     SDL_Color green = {0, 160, 0};
     SDL_Color black = {0, 0, 0};
     
     // 绘制标题和结果
     render_text(renderer, font, "FIN DE PARTIE", 250, 200, red);
     render_text(renderer, font, result, 250, 240, green);
 
     // 绘制退出按钮
     SDL_Rect quitBtn = {300, 300, 100, 30};
     SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
     SDL_RenderFillRect(renderer, &quitBtn);
     SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
     SDL_RenderDrawRect(renderer, &quitBtn);
     render_text(renderer, font, "QUITTER", 320, 305, black);
 }
 
 /**
  * @brief 设置是否显示结束对话框
  * 
  * @param value 1显示/0隐藏
  */
 void setShowEndDialog(int value) {
     showEndDialog = value;
 }
 
/**
 * @brief 发送动作请求到服务器
 * 
 * @param type 动作类型(O/S/G)
 * @param objet 对象ID
 * @param cible 目标ID(仅S动作需要)
 */
void send_action_request(char type, int objet, int cible) {
    char buffer[64];
    if (type == 'O') {
        snprintf(buffer, sizeof(buffer), "O %d", objet); // 格式：O obj
    } else if (type == 'S') {
        snprintf(buffer, sizeof(buffer), "S %d %d", cible, objet); // 格式：S target_id obj
    } else if (type == 'G') {
        snprintf(buffer, sizeof(buffer), "G %d %d", myClientId, objet); // 格式：G myClientId card
    }
    sendMessageToServer("", 0, buffer);
}
 
/**
 * @brief 主GUI循环
 * 
 * 初始化SDL窗口和渲染器，处理用户输入，渲染游戏界面
 */
void run_gui() {
    // 初始化SDL子系统
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
 
    // 创建窗口和渲染器
    SDL_Window *window = SDL_CreateWindow("Sherlock13", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("img/sans.ttf", 20);
 
    SDL_Color black = {0, 0, 0};
     
    // 加载所有纹理资源
    load_all_textures(renderer);
 
    // 用户名输入框相关变量
    char inputText[32] = "";
    SDL_Rect inputBox = {400, 300, 400, 40};
    SDL_Rect goButton = {550, 360, 140, 40};
    SDL_StartTextInput();
 
    // 主循环控制变量
    int running = 1;
    SDL_Event e;
 
    // 主事件循环
    while (running) {
        // 处理所有待处理事件
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            }
            // 处理文本输入
            else if (e.type == SDL_TEXTINPUT && strlen(inputText) < 31) {
                strcat(inputText, e.text.text);
            }
            // 处理键盘按键
            else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    if (popupState != POPUP_NONE) {
                        // 按 ESC 关闭弹窗
                        popupState = POPUP_NONE;
                        selectedObjectId = -1;
                        selectedPlayerId = -1;
                        selectedGuessCard = -1;
                    }
                } else if (e.key.keysym.sym == SDLK_BACKSPACE && strlen(inputText) > 0) {
                    // 删除输入文本
                    inputText[strlen(inputText) - 1] = '\0';
                }
            }
            // 处理鼠标点击
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int x = e.button.x, y = e.button.y;

                // 游戏未开始时处理用户名输入
                if (!getIsGameStarted() && x >= goButton.x && x <= goButton.x + goButton.w &&
                   y >= goButton.y && y <= goButton.y + goButton.h && inputText[0] != '\0' && !isUsernameSet()) {
                   setUsername(inputText);
                    if (getClientPort() == 0) {
                        char msg[128];
                        snprintf(msg, sizeof(msg), "C %s", getUsername());
                        sendMessageToServer("", 0, msg);
                    }
                }
 
                // 游戏进行中且是当前玩家回合时处理操作按钮
                if (getIsGameStarted() && isTurn()) {
                    if (hoverO) {
                        popupState = POPUP_O;
                        printf("Detect button O click\n");
                    } 
                    else if (hoverS) {
                        popupState = POPUP_S_SELECT_OBJ; 
                        printf("Detect button S click\n");
                    }
                    else if (hoverG) {
                        popupState = POPUP_G;
                        printf("Detect button G click\n");
                    }
                }

                if (popupState == POPUP_S_SELECT_OBJ || popupState == POPUP_S_SELECT_PLAYER || popupState == POPUP_O) {
                    int x = e.button.x, y = e.button.y;
                
                    // 选中对象
                    for (int i = 0; i < 8; ++i) {
                        SDL_Rect objBox = {200, 160 + i * 25, 200, 22};
                        if (x >= objBox.x && x <= objBox.x + objBox.w && y >= objBox.y && y <= objBox.y + objBox.h) {
                            selectedObjectId = i;
                        }
                    }
                
                    // 选中玩家（用于 S 操作）
                    int pc = getPlayerCount();
                    for (int i = 0; i < pc; ++i) {
                        SDL_Rect playerBox = {420, 160 + i * 25, 180, 22};
                        if (x >= playerBox.x && x <= playerBox.x + playerBox.w && y >= playerBox.y && y <= playerBox.y + playerBox.h) {
                            selectedPlayerId = i;
                        }
                    }
                
                    // OK / Cancel
                    SDL_Rect okBtn = {240, 380, 80, 30};
                    SDL_Rect cancelBtn = {360, 380, 80, 30};
                
                    if (x >= okBtn.x && x <= okBtn.x + okBtn.w && y >= okBtn.y && y <= okBtn.y + okBtn.h) {
                        // 根据操作类型发送请求
                        if (popupState == POPUP_O && selectedObjectId >= 0) {
                            send_action_request('O', selectedObjectId, 0);
                        } else if (popupState == POPUP_S_SELECT_OBJ && selectedObjectId >= 0 && selectedPlayerId >= 0) {
                            send_action_request('S', selectedObjectId, selectedPlayerId);
                        }
                        // 重置状态
                        popupState = POPUP_NONE;
                        selectedObjectId = -1;
                        selectedPlayerId = -1;
                    }
                
                    if (x >= cancelBtn.x && x <= cancelBtn.x + cancelBtn.w && y >= cancelBtn.y && y <= cancelBtn.y + cancelBtn.h) {
                        popupState = POPUP_NONE;
                        selectedObjectId = -1;
                        selectedPlayerId = -1;
                    }
                }
                if (popupState == POPUP_G) {
                    int x = e.button.x, y = e.button.y;
                
                    // 角色卡牌点击选择
                    for (int i = 0; i < 13; ++i) {
                        int row = i / 7;
                        int col = i % 7;
                        int cx = 200 + col * 120;
                        int cy = 180 + row * 160;
                        SDL_Rect cardRect = {cx, cy, 100, 140};
                
                        if (x >= cardRect.x && x <= cardRect.x + cardRect.w && y >= cardRect.y && y <= cardRect.y + cardRect.h) {
                            selectedGuessCard = i;
                        }
                    }
                
                    // OK / Cancel
                    SDL_Rect okBtn = {320, 440, 80, 30};
                    SDL_Rect cancelBtn = {440, 440, 80, 30};
                    if (x >= okBtn.x && x <= okBtn.x + okBtn.w && y >= okBtn.y && y <= okBtn.y + okBtn.h && selectedGuessCard >= 0) {
                        send_action_request('G', selectedGuessCard, 0);
                        popupState = POPUP_NONE;
                        selectedGuessCard = -1;
                    }
                
                    if (x >= cancelBtn.x && x <= cancelBtn.x + cancelBtn.w && y >= cancelBtn.y && y <= cancelBtn.y + cancelBtn.h) {
                        popupState = POPUP_NONE;
                        selectedGuessCard = -1;
                    }
                }
                
                // 游戏结束时处理退出按钮
                if (getIsGameEnded()) {
                    if (x >= 300 && x <= 400 && y >= 300 && y <= 330) {
                        running = 0;
                    }
                }
            }
            // 处理鼠标移动事件
            else if (e.type == SDL_MOUSEMOTION) {
                mouseX = e.motion.x;
                mouseY = e.motion.y;
                
                // 更新悬停状态
                hoverO = (mouseX >= BUTTON_O_X && mouseX <= BUTTON_O_X + BUTTON_WIDTH &&
                         mouseY >= BUTTON_O_Y && mouseY <= BUTTON_O_Y + BUTTON_HEIGHT);
                hoverS = (mouseX >= BUTTON_S_X && mouseX <= BUTTON_S_X + BUTTON_WIDTH &&
                         mouseY >= BUTTON_S_Y && mouseY <= BUTTON_S_Y + BUTTON_HEIGHT);
                hoverG = (mouseX >= BUTTON_G_X && mouseX <= BUTTON_G_X + BUTTON_WIDTH &&
                         mouseY >= BUTTON_G_Y && mouseY <= BUTTON_G_Y + BUTTON_HEIGHT);
            }
        }

        // 清屏
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // 根据游戏状态渲染不同界面
        if (!getIsGameStarted()) {
            // 渲染用户名输入界面
            render_text(renderer, font, "Entrez votre nom:", 400, 250, black);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderFillRect(renderer, &inputBox);
            render_text(renderer, font, inputText, inputBox.x + 10, inputBox.y + 5, black);
            SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
            SDL_RenderFillRect(renderer, &goButton);
            render_text(renderer, font, "Connect", goButton.x + 33, goButton.y + 5, black);

            // 显示已连接玩家列表
            for (int i = 0; i < getPlayerCount(); ++i) {
                char line[64];
                snprintf(line, sizeof(line), "Joueur %d: %s", i + 1, getPlayerName(i));
                render_text(renderer, font, line, 400, 460 + i * 30, black);
            }
        } else {
            // 游戏进行中，渲染游戏主界面
            draw_game_board(renderer, font);
            draw_role_table(renderer, font);

            // 根据弹窗状态渲染不同弹窗
            if (popupState == POPUP_O || popupState == POPUP_S_SELECT_OBJ || popupState == POPUP_S_SELECT_PLAYER) {
                draw_selection_popup(renderer, font);
            } else if (popupState == POPUP_G) {
                draw_guess_popup(renderer, font);
            }

            // 游戏结束时显示结束弹窗
            if (getIsGameEnded()) {
                show_end_popup(renderer, font, getLastResult());
            }
        }

        // 更新屏幕
        SDL_RenderPresent(renderer);
    }

    // 清理资源
    SDL_StopTextInput();
    TTF_CloseFont(font);
    unload_textures();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}
 
/**
 * @brief 绘制游戏主面板
 * 
 * @param renderer SDL渲染器
 * @param font 字体对象
 */
void draw_game_board(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Color black = {0, 0, 0};
    SDL_Color red = {255, 0, 0};
    SDL_Color gray = {150, 150, 150};

    // 布局参数
    int marginLeft = 100, marginTop = 70;
    int cellSize = 35;
    int playerCount = getPlayerCount();
    int currentPlayer = getCurrentPlayer();

    // 顶部对象图标与数量显示
    for (int i = 0; i < 8; ++i) {
        render_icon(renderer, icons[i], marginLeft + i * 60, marginTop, 32);
        char countStr[4];
        snprintf(countStr, sizeof(countStr), "%d", getObjectCounts()[i]);
        render_text(renderer, font, countStr, marginLeft + i * 60 + 10, marginTop + 35, black);
    }

    // 在左上角显示当前玩家名称
    SDL_Color blue = {0, 0, 255};
    char currentUserText[64];
    snprintf(currentUserText, sizeof(currentUserText), "Player: %s", getUsername());
    render_text(renderer, font, currentUserText, 20, 20, blue);

    // 玩家对象矩阵
    int startY = marginTop + 70;
    for (int p = 0; p < playerCount; ++p) {
        // 根据玩家状态设置颜色
        SDL_Color color = black;
        if (!isPlayerAlive(p)) color = gray;
        else if (p == currentPlayer) color = red;

        // 绘制玩家名称
        render_text(renderer, font, getPlayerName(p), marginLeft - 80, startY + p * 40 + 5, color);

        // 绘制玩家对象矩阵
        for (int o = 0; o < 8; ++o) {
            SDL_Rect rect = {marginLeft + o * 60, startY + p * 40, cellSize, cellSize};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &rect);

            // 显示对象值
            char val[2];
            snprintf(val, sizeof(val), "%d", getTableValue(p, o));
            render_text(renderer, font, val, rect.x + 12, rect.y + 8, black);
        }
    }

    // 右侧显示玩家的卡牌
    render_cards(renderer, cards, getMyCards(), 3);
 
    // OSG操作按钮
    render_osg_buttons(renderer, font);
 
    // 底部显示
    render_text(renderer, font, getLastResult(), WINDOW_WIDTH / 2 - 200, WINDOW_HEIGHT - 60, red);
    // 在底部显示当前玩家姓名
    if (getIsGameStarted()) {
        SDL_Color turnColor = {255, 0, 0}; // 红色高亮当前玩家
        int currentPlayerId = -1;
        currentPlayerId = getCurrentTurnPlayer();

        // 显示带玩家名的回合信息
        char turnText[128];
        if (currentPlayerId >= 0) {
            snprintf(turnText, sizeof(turnText), "Current Player: %s", getPlayerName(currentPlayerId));
        } else {
            strcpy(turnText, "Waiting for game start...");
        }
        render_text(renderer, font, turnText, WINDOW_WIDTH / 2 - 200, WINDOW_HEIGHT - 90, turnColor);
    }
    // 在底部显示LOGO
    render_text(renderer, font, "SHERLOCK 13", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT - 30, black);
}
 
/**
 * @brief 绘制角色信息表
 * 
 * @param renderer SDL渲染器
 * @param font 字体对象
 */
void draw_role_table(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Color black = {0, 0, 0};
     
    // 角色名称数组
    const char* roleNames[13] = {
        "Sebastian Moran", "Irene Adler", "Inspector Lestrade", "Inspector Gregson",
        "Inspector Baynes", "Inspector Bradstreet", "Inspector Hopkins",
        "Sherlock Holmes", "John Watson", "Mycroft Holmes", "Mrs. Hudson",
        "Mary Morstan", "James Moriarty"
    };
     
    // 角色对象矩阵
    int roleObjects[13][8] = {
        {0,0,0,1,0,0,0,1}, {1,0,0,0,1,0,0,0}, {0,1,0,0,1,0,1,0},
        {0,1,1,0,0,0,1,0}, {0,0,1,1,0,0,0,1}, {1,0,0,0,0,1,1,0},
        {1,1,0,0,0,1,0,0}, {0,0,1,0,1,0,1,0}, {1,1,0,0,0,0,1,0},
        {0,0,0,0,1,1,0,1}, {1,0,1,0,0,0,0,1}, {0,1,1,0,0,1,0,0},
        {0,0,0,1,1,0,1,0}
    };
 
    // 绘制角色表格
    int startX = 20, startY = 400;
    for (int i = 0; i < 13; ++i) {
        // 绘制角色名称(分两列显示)
        render_text(renderer, font, roleNames[i], startX + ((i < 7) ? 0 : 450), startY + (i % 7) * 35, black);
         
        // 绘制角色拥有的对象图标
        for (int j = 0; j < 8; ++j) {
            if (roleObjects[i][j]) {
                int x = startX + ((i < 7) ? 200 : 630) + j * 26;
                render_icon(renderer, icons[j], x, startY + (i % 7) * 35, 30);
            }
        }
    }
}

 // 按钮渲染函数
void render_osg_buttons(SDL_Renderer* renderer, TTF_Font* font) {
    const char *labels[3] = {"Observe (O)", "Speculate (S)", "Guess (G)"};
    SDL_Color black = {0, 0, 0, 255};

    // O按钮
    SDL_Rect btnO = {BUTTON_O_X, BUTTON_O_Y, BUTTON_WIDTH, BUTTON_HEIGHT};
    SDL_SetRenderDrawColor(renderer, hoverO ? 255 : 200, hoverO ? 220 : 200, hoverO ? 0 : 200, 255);
    SDL_RenderFillRect(renderer, &btnO);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &btnO);
    render_text(renderer, font, labels[0], btnO.x + 13, btnO.y + 10, black);

    // S按钮 
    SDL_Rect btnS = {BUTTON_S_X, BUTTON_S_Y, BUTTON_WIDTH, BUTTON_HEIGHT};
    SDL_SetRenderDrawColor(renderer, hoverS ? 0 : 200, hoverS ? 220 : 200, hoverS ? 255 : 200, 255);
    SDL_RenderFillRect(renderer, &btnS);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
    SDL_RenderDrawRect(renderer, &btnS);
    render_text(renderer, font, labels[1], btnS.x + 9, btnS.y + 10, black);

    // G按钮
    SDL_Rect btnG = {BUTTON_G_X, BUTTON_G_Y, BUTTON_WIDTH, BUTTON_HEIGHT};
    SDL_SetRenderDrawColor(renderer, hoverG ? 255 : 200, hoverG ? 0 : 200, hoverG ? 0 : 200, 255);
    SDL_RenderFillRect(renderer, &btnG);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &btnG);
    render_text(renderer, font, labels[2], btnG.x + 22, btnG.y + 10, black);
}
