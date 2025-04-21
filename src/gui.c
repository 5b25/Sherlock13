// gui.c
#include "../include/gui.h"
#include "../include/client_logic.h"
#include "../include/resources.h" 
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

typedef enum {
    POPUP_NONE,
    POPUP_O,
    POPUP_S_SELECT_OBJ,
    POPUP_S_SELECT_PLAYER,
    POPUP_G
} PopupState;

static int showEndDialog = 0;

// 渲染文字函数
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

// 渲染小图标
void render_icon(SDL_Renderer *renderer, SDL_Texture *icon, int x, int y, int size) {
    SDL_Rect rect = {x, y, size, size};
    SDL_RenderCopy(renderer, icon, NULL, &rect);
}

void render_cards(SDL_Renderer* renderer, SDL_Texture* cards[], int* mycards, int cardCount) {
    for (int i = 0; i < cardCount; ++i) {
        int idx = mycards[i];
        if (idx >= 0 && idx < 13) {
            SDL_Rect dst = {1020, 400 + i * 150, 100, 140};
            SDL_RenderCopy(renderer, cards[idx], NULL, &dst);
        }
    }
}

// 主GUI入口
void run_gui() {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

    SDL_Window *window = SDL_CreateWindow("Sherlock13", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("img/sans.ttf", 20);
    SDL_Color black = {0, 0, 0};
    SDL_Color red = {255, 0, 0};
    SDL_Color gray = {150, 150, 150};

    // 加载资源
    load_all_textures(renderer);

    char inputText[32] = "";
    SDL_Rect inputBox = {400, 300, 400, 40};
    SDL_Rect goButton = {550, 360, 100, 40};
    SDL_StartTextInput();

    int running = 1;
    SDL_Event e;
    PopupState popupState = POPUP_NONE;
    //int selectedObject = -1;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_TEXTINPUT && strlen(inputText) < 31) {
                strcat(inputText, e.text.text);
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_BACKSPACE && strlen(inputText) > 0) {
                inputText[strlen(inputText) - 1] = '\0';
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int x = e.button.x, y = e.button.y;

                // 加入阶段点击 GO 按钮提交名字
                if (!getIsGameStarted() && x >= goButton.x && x <= goButton.x + goButton.w &&
                    y >= goButton.y && y <= goButton.y + goButton.h &&
                    inputText[0] != '\0' && !isUsernameSet()) {

                    setUsername(inputText);
                    if (getClientPort() == 0) {
                        char msg[128];
                        snprintf(msg, sizeof(msg), "C %s", getUsername());
                        sendMessageToServer("", 0, msg);
                    }

                    printf("[GUI] Username '%s' submitted to server.\n", getUsername());
                }

                // 游戏阶段按钮交互
                if (getIsGameStarted() && isTurn()) {
                    if (x >= 1150 && x <= 1220) {
                        if (y >= 100 && y <= 140) popupState = POPUP_O;
                        else if (y >= 150 && y <= 190) popupState = POPUP_S_SELECT_OBJ;
                        else if (y >= 200 && y <= 240) popupState = POPUP_G;
                    }
                }

                // 结束退出按钮
                if (getIsGameEnded()) {
                    if (x >= 300 && x <= 400 && y >= 300 && y <= 330) {
                        running = 0;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // ==== 加入阶段 ====
        if (!getIsGameStarted()) {
            render_text(renderer, font, "Entrez votre nom:", 400, 250, black);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderFillRect(renderer, &inputBox);
            render_text(renderer, font, inputText, inputBox.x + 10, inputBox.y + 5, black);
            SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
            SDL_RenderFillRect(renderer, &goButton);
            render_text(renderer, font, "GO", goButton.x + 30, goButton.y + 5, black);
            for (int i = 0; i < getPlayerCount(); i++) {
                char line[64];
                snprintf(line, sizeof(line), "Joueur %d: %s", i + 1, getPlayerName(i));
                render_text(renderer, font, line, 400, 460 + i * 30, black);
            }
        } else {
            // 顶部对象图标 + 数量
            for (int i = 0; i < 8; ++i) {
                render_icon(renderer, icons[i], 100 + i * 60, 60, 40);
                char countStr[8];
                snprintf(countStr, sizeof(countStr), "%d", getObjectCounts()[i]);
                render_text(renderer, font, countStr, 115 + i * 60, 105, black);
            }

            // 玩家对象矩阵（左侧名字 + 行内圆圈标记）
            for (int i = 0; i < getPlayerCount(); ++i) {
                const char* name = getPlayerName(i);
                SDL_Color color = isPlayerAlive(i) ? (i == getCurrentPlayer() ? red : black) : gray;
                render_text(renderer, font, name, 20, 140 + i * 40, color);

                for (int j = 0; j < 8; ++j) {
                    int hasObj = getTableValue(i, j);
                    if (hasObj == 1) {
                        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
                        SDL_Rect rect = {150 + j * 60, 145 + i * 40, 20, 20};
                        SDL_RenderFillRect(renderer, &rect);
                    }
                }
            }

            // 右侧按钮
            SDL_Rect buttonO = {1150, 100, 70, 40};
            SDL_Rect buttonS = {1150, 150, 70, 40};
            SDL_Rect buttonG = {1150, 200, 70, 40};
            SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
            SDL_RenderFillRect(renderer, &buttonO);
            SDL_RenderFillRect(renderer, &buttonS);
            SDL_RenderFillRect(renderer, &buttonG);
            render_text(renderer, font, "O", 1170, 110, black);
            render_text(renderer, font, "S", 1170, 160, black);
            render_text(renderer, font, "G", 1170, 210, black);

            // 底部我的卡牌
            render_cards(renderer, cards, getMyCards(), 3);

            // 弹窗交互
            if (popupState == POPUP_O || popupState == POPUP_S_SELECT_OBJ || popupState == POPUP_S_SELECT_PLAYER) {
                draw_selection_popup(renderer, font);
            } else if (popupState == POPUP_G) {
                draw_guess_popup(renderer, font);
            }

            if (getIsGameEnded()) {
                show_end_popup(renderer, font, getLastResult());
            }

            render_text(renderer, font, getLastResult(), 600, 20, red);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput();
    TTF_CloseFont(font);
    unload_textures(); 
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

// 弹窗：选择对象与玩家
void draw_selection_popup(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Rect popup = {180, 100, 440, 380};
    SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
    SDL_RenderFillRect(renderer, &popup);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &popup);

    SDL_Color black = {0, 0, 0};

    render_text(renderer, font, "Objets:", 200, 120, black);
    for (int i = 0; i < 8; ++i) {
        char label[32];
        snprintf(label, sizeof(label), "%d: %s", i + 1, nameobjets[i]);
        SDL_Rect box = {200, 160 + i * 25, 200, 22};
        SDL_RenderDrawRect(renderer, &box);
        render_text(renderer, font, label, 205, 162 + i * 25, black);
    }

    render_text(renderer, font, "Joueurs:", 420, 120, black);
    int pc = getPlayerCount();
    for (int i = 0; i < pc; ++i) {
        char label[32];
        snprintf(label, sizeof(label), "%d: %s", i + 1, getPlayerName(i));
        SDL_Rect box = {420, 160 + i * 25, 180, 22};
        SDL_RenderDrawRect(renderer, &box);
        render_text(renderer, font, label, 425, 162 + i * 25, black);
    }
}

// 弹窗：猜测角色
void draw_guess_popup(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Rect popup = {220, 180, 360, 240};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &popup);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &popup);

    SDL_Color black = {0, 0, 0};
    render_text(renderer, font, "Deviner le coupable:", 240, 200, black);
    render_text(renderer, font, "← Cliquez sur un nom de carte →", 240, 240, black);
}

// 游戏结束弹窗
void show_end_popup(SDL_Renderer* renderer, TTF_Font* font, const char* result) {
    if (!showEndDialog) return;

    SDL_Rect popup = {180, 180, 420, 180};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &popup);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &popup);

    SDL_Color red = {255, 0, 0};
    SDL_Color green = {0, 160, 0};
    SDL_Color black = {0, 0, 0};

    render_text(renderer, font, "FIN DE PARTIE", 250, 200, red);
    render_text(renderer, font, result, 250, 240, green);

    SDL_Rect quitBtn = {300, 300, 100, 30};
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderFillRect(renderer, &quitBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &quitBtn);
    render_text(renderer, font, "QUITTER", 320, 305, black);
}

// 设置游戏结束弹窗的显示
void setShowEndDialog(int value) {
    showEndDialog = value;
}

// 发送玩家操作请求（O / S / G）
void send_action_request(char type, int objet, int cible) {
    char buffer[64];
    if (type == 'O') {
        snprintf(buffer, sizeof(buffer), "O %d", objet);
    } else if (type == 'S') {
        snprintf(buffer, sizeof(buffer), "S %d %d", objet, cible);
    } else if (type == 'G') {
        snprintf(buffer, sizeof(buffer), "G %d", objet);
    }
    sendMessageToServer("", 0, buffer); 
}
