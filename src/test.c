// src/gui.c
#include "../include/gui.h"
#include "../include/client_logic.h"
#include "../include/resources.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <string.h>

// Window dimension constants
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 768
#define ICON_SIZE 32
#define CARD_WIDTH 300
#define CARD_HEIGHT 200
#define BUTTON_WIDTH 130
#define BUTTON_HEIGHT 40

// Main Board Buttons
#define BUTTON_O_X 650
#define BUTTON_O_Y 100
#define BUTTON_S_X 650
#define BUTTON_S_Y 160
#define BUTTON_G_X 650 
#define BUTTON_G_Y 220

// Popup Dimensions
#define POPUP_WIDTH          1000
#define POPUP_HEIGHT         560
#define CARD_START_X         220
#define CARD_START_Y         180
#define BG_CARD_WIDTH        160
#define BG_CARD_HEIGHT       100
#define COLUMNS_PER_ROW      5
#define COLUMN_SPACING       20
#define ROW_SPACING          40

// --- [修复核心] 统一按钮坐标宏定义 ---
// 1. O & S 弹窗按钮 (Observe / Speculate)
#define OS_BTN_Y      380  // Y坐标
#define OS_BTN_W      80   // 宽
#define OS_BTN_H      30   // 高
#define OS_OK_X       240  // OK按钮 X
#define OS_CANCEL_X   360  // Cancel按钮 X

// 2. G 弹窗按钮 (Guess)
#define G_BTN_Y       580  // Y坐标
#define G_BTN_W       80
#define G_BTN_H       30
#define G_OK_X        400
#define G_CANCEL_X    520

static int showEndDialog = 0;
static PopupState popupState = POPUP_NONE;

// Tracking Mouse State
static int mouseX = 0, mouseY = 0;
static int hoverO = 0, hoverS = 0, hoverG = 0;

// Selections
static int selectedObjectId = -1;
static int selectedPlayerId = -1;
static int selectedGuessCard = -1;

void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!text || strlen(text) == 0 || !font || !renderer) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex) {
        SDL_Rect rect = {x, y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void render_icon(SDL_Renderer *renderer, SDL_Texture *tex, int x, int y, int size) {
    if (!tex || !renderer) return;
    SDL_Rect dst = {x, y, size, size};
    SDL_RenderCopy(renderer, tex, NULL, &dst);
}
 
void render_cards(SDL_Renderer* renderer, SDL_Texture* cards[], int* mycards, int cardCount) {
    if (!mycards || !cards || !renderer) return;
    for (int i = 0; i < cardCount; ++i) {
        int idx = mycards[i];
        if (idx >= 0 && idx < 13 && cards[idx]) {
            SDL_Rect dst = {940, 50 + i * 230, CARD_WIDTH, CARD_HEIGHT};
            SDL_RenderCopy(renderer, cards[idx], NULL, &dst);
        }
    }
}
 
void draw_selection_popup(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Rect popup = {180, 100, 440, 380};
    SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
    SDL_RenderFillRect(renderer, &popup);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &popup);

    SDL_Color black = {0, 0, 0};

    // Objets
    render_text(renderer, font, "Objets:", 200, 120, black);
    for (int i = 0; i < 8; ++i) {
        SDL_Rect box = {200, 160 + i * 25, 200, 22};
        if (i == selectedObjectId) {
            SDL_SetRenderDrawColor(renderer, 0, 200, 0, 100);
            SDL_RenderFillRect(renderer, &box);
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &box);

        char label[64];
        snprintf(label, sizeof(label), "%d: %s", i + 1, nameobjets[i]);
        render_text(renderer, font, label, 205, 155 + i * 25, black);
    }

    // Players (Only for S)
    if (popupState == POPUP_S_SELECT_OBJ || popupState == POPUP_S_SELECT_PLAYER) {
        render_text(renderer, font, "Joueurs:", 420, 120, black);
        int pc = getPlayerCount();
        if (pc > 4) pc = 4;

        for (int i = 0; i < pc; ++i) {
            SDL_Rect box = {420, 160 + i * 25, 180, 22};
            if (i == selectedPlayerId) {
                SDL_SetRenderDrawColor(renderer, 200, 200, 0, 100);
                SDL_RenderFillRect(renderer, &box);
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &box);

            char label[64];
            snprintf(label, sizeof(label), "%d: %s", i + 1, getPlayerName(i));
            render_text(renderer, font, label, 425, 155 + i * 25, black);
        }
    }

    // --- 使用宏绘制按钮 ---
    SDL_Rect confirm = {OS_OK_X, OS_BTN_Y, OS_BTN_W, OS_BTN_H};
    SDL_Rect cancel  = {OS_CANCEL_X, OS_BTN_Y, OS_BTN_W, OS_BTN_H};

    SDL_SetRenderDrawColor(renderer, 200, 255, 200, 255);
    SDL_RenderFillRect(renderer, &confirm);
    SDL_RenderDrawRect(renderer, &confirm);
    render_text(renderer, font, "OK", confirm.x + 25, confirm.y + 5, black);

    SDL_SetRenderDrawColor(renderer, 255, 200, 200, 255);
    SDL_RenderFillRect(renderer, &cancel);
    SDL_RenderDrawRect(renderer, &cancel);
    render_text(renderer, font, "Cancel", cancel.x + 10, cancel.y + 5, black);
}
 
void draw_guess_popup(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Rect popup = {180, 100, POPUP_WIDTH, POPUP_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
    SDL_RenderFillRect(renderer, &popup);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &popup);

    SDL_Color black = {0, 0, 0};
    render_text(renderer, font, "Cliquez sur la carte que vous soupçonnez être le coupable :", 200, 130, black);

    for (int i = 0; i < 13; ++i) {
        int row = i / COLUMNS_PER_ROW;
        int col = i % COLUMNS_PER_ROW;
        int x = CARD_START_X + col * (BG_CARD_WIDTH + COLUMN_SPACING);
        int y = CARD_START_Y + row * (BG_CARD_HEIGHT + ROW_SPACING);

        SDL_Rect cardRect = {x, y, BG_CARD_WIDTH, BG_CARD_HEIGHT};
        if (i == selectedGuessCard) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 150);
            SDL_RenderFillRect(renderer, &cardRect);
        }
        SDL_RenderCopy(renderer, cards[i], NULL, &cardRect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &cardRect);
    }

    // --- 使用宏绘制按钮 ---
    SDL_Rect okBtn = {G_OK_X, G_BTN_Y, G_BTN_W, G_BTN_H};
    SDL_Rect cancelBtn = {G_CANCEL_X, G_BTN_Y, G_BTN_W, G_BTN_H};

    SDL_SetRenderDrawColor(renderer, 200, 255, 200, 255);
    SDL_RenderFillRect(renderer, &okBtn);
    SDL_RenderDrawRect(renderer, &okBtn);
    render_text(renderer, font, "OK", okBtn.x + 25, okBtn.y + 5, black);

    SDL_SetRenderDrawColor(renderer, 255, 200, 200, 255);
    SDL_RenderFillRect(renderer, &cancelBtn);
    SDL_RenderDrawRect(renderer, &cancelBtn);
    render_text(renderer, font, "Cancel", cancelBtn.x + 10, cancelBtn.y + 5, black);
}
 
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
     render_text(renderer, font, "GAME OVER", 250, 200, red);
     render_text(renderer, font, result, 250, 240, green);
     SDL_Rect quitBtn = {300, 300, 100, 30};
     SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
     SDL_RenderFillRect(renderer, &quitBtn);
     SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
     SDL_RenderDrawRect(renderer, &quitBtn);
     render_text(renderer, font, "QUITTER", 320, 305, black);
}
 
void setShowEndDialog(int value) { showEndDialog = value; }
 
void send_action_request(char type, int objet, int cible) {
    if (type == 'O') sendActionO(objet);
    else if (type == 'S') sendActionS(cible, objet);
    else if (type == 'G') sendActionG(objet);
}
 
void run_gui() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return;
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) return;
    if (TTF_Init() == -1) return;

    SDL_Window *window = SDL_CreateWindow("Sherlock 13 - Client", 
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return;
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    TTF_Font *font = TTF_OpenFont("img/sans.ttf", 20);

    SDL_Color black = {0, 0, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray  = {200, 200, 200, 255};
    SDL_Color blue  = {0, 0, 255, 255};
    SDL_Color red   = {255, 0, 0, 255};

    load_all_textures(renderer);

    char inputText[32] = "";
    SDL_Rect inputBox = {400, 300, 400, 40};
    SDL_Rect goButton = {550, 360, 140, 40};
    SDL_StartTextInput();

    int running = 1;
    SDL_Event e;
    static int connectClicked = 0;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;

            // Login Logic
            else if (!getIsConnected()) {
                if (e.type == SDL_TEXTINPUT && strlen(inputText) < 31) strcat(inputText, e.text.text);
                else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_BACKSPACE && strlen(inputText) > 0) inputText[strlen(inputText)-1] = '\0';
                else if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int x = e.button.x, y = e.button.y;
                    if (x >= goButton.x && x <= goButton.x + goButton.w && y >= goButton.y && y <= goButton.y + goButton.h) {
                        if (strlen(inputText) > 0 && !connectClicked) {
                            connectClicked = 1;
                            setUsername(inputText);
                            sendConnect(getUsername(), getClientPort());
                        }
                    }
                }
            }
            // Game Logic
            else {
                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                    if (popupState != POPUP_NONE) popupState = POPUP_NONE;
                }

                if (getIsGameStarted()) {
                    if (e.type == SDL_MOUSEBUTTONDOWN) {
                        int x = e.button.x, y = e.button.y;
                        
                        // 1. Main Buttons
                        if (isTurn() && popupState == POPUP_NONE) {
                            if (x >= BUTTON_O_X && x <= BUTTON_O_X + BUTTON_WIDTH && y >= BUTTON_O_Y && y <= BUTTON_O_Y + BUTTON_HEIGHT) {
                                popupState = POPUP_O;
                                selectedObjectId = -1; 
                            }
                            else if (x >= BUTTON_S_X && x <= BUTTON_S_X + BUTTON_WIDTH && y >= BUTTON_S_Y && y <= BUTTON_S_Y + BUTTON_HEIGHT) {
                                popupState = POPUP_S_SELECT_OBJ;
                                selectedObjectId = -1; selectedPlayerId = -1;
                            }
                            else if (x >= BUTTON_G_X && x <= BUTTON_G_X + BUTTON_WIDTH && y >= BUTTON_G_Y && y <= BUTTON_G_Y + BUTTON_HEIGHT) {
                                popupState = POPUP_G;
                                selectedGuessCard = -1;
                            }
                        }

                        // 2. Popup Interaction
                        if (popupState != POPUP_NONE) {
                            // --- O & S Popup ---
                            if (popupState == POPUP_O || popupState == POPUP_S_SELECT_OBJ || popupState == POPUP_S_SELECT_PLAYER) {
                                // Objects
                                for (int i = 0; i < 8; ++i) {
                                    SDL_Rect b = {200, 160 + i * 25, 200, 22};
                                    if (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h) selectedObjectId = i;
                                }
                                // Players
                                int pc = getPlayerCount();
                                if(pc > 4) pc = 4;
                                for (int i = 0; i < pc; ++i) {
                                    SDL_Rect b = {420, 160 + i * 25, 180, 22};
                                    if (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h) selectedPlayerId = i;
                                }

                                // [修复] 使用宏检测点击，确保与绘制一致
                                // OK Button
                                if (x >= OS_OK_X && x <= OS_OK_X + OS_BTN_W && y >= OS_BTN_Y && y <= OS_BTN_Y + OS_BTN_H) {
                                    if (popupState == POPUP_O && selectedObjectId >= 0) {
                                        send_action_request('O', selectedObjectId, 0);
                                        popupState = POPUP_NONE;
                                    }
                                    else if (popupState == POPUP_S_SELECT_OBJ && selectedObjectId >= 0 && selectedPlayerId >= 0) {
                                        send_action_request('S', selectedObjectId, selectedPlayerId);
                                        popupState = POPUP_NONE;
                                    } else {
                                        printf("[GUI] Missing selection for OK.\n");
                                    }
                                }
                                // Cancel Button
                                if (x >= OS_CANCEL_X && x <= OS_CANCEL_X + OS_BTN_W && y >= OS_BTN_Y && y <= OS_BTN_Y + OS_BTN_H) {
                                    popupState = POPUP_NONE;
                                }
                            }
                            // --- G Popup ---
                            else if (popupState == POPUP_G) {
                                // Cards
                                for (int i = 0; i < 13; ++i) {
                                    int row = i / COLUMNS_PER_ROW, col = i % COLUMNS_PER_ROW;
                                    int cx = CARD_START_X + col * (BG_CARD_WIDTH + COLUMN_SPACING);
                                    int cy = CARD_START_Y + row * (BG_CARD_HEIGHT + ROW_SPACING);
                                    if (x >= cx && x <= cx + BG_CARD_WIDTH && y >= cy && y <= cy + BG_CARD_HEIGHT) selectedGuessCard = i;
                                }
                                // [修复] 使用宏检测点击
                                // OK Button
                                if (x >= G_OK_X && x <= G_OK_X + G_BTN_W && y >= G_BTN_Y && y <= G_BTN_Y + G_BTN_H) {
                                    if (selectedGuessCard >= 0) {
                                        send_action_request('G', selectedGuessCard, 0);
                                        popupState = POPUP_NONE;
                                    }
                                }
                                // Cancel Button
                                if (x >= G_CANCEL_X && x <= G_CANCEL_X + G_BTN_W && y >= G_BTN_Y && y <= G_BTN_Y + G_BTN_H) {
                                    popupState = POPUP_NONE;
                                }
                            }
                        }

                        // 3. Game Over
                        if (getIsGameEnded()) {
                            if (x >= 300 && x <= 400 && y >= 300 && y <= 330) running = 0;
                        }
                    }

                    // Hover
                    if (e.type == SDL_MOUSEMOTION) {
                        mouseX = e.motion.x; mouseY = e.motion.y;
                        if (popupState == POPUP_NONE) {
                            hoverO = (mouseX >= BUTTON_O_X && mouseX <= BUTTON_O_X + BUTTON_WIDTH && mouseY >= BUTTON_O_Y && mouseY <= BUTTON_O_Y + BUTTON_HEIGHT);
                            hoverS = (mouseX >= BUTTON_S_X && mouseX <= BUTTON_S_X + BUTTON_WIDTH && mouseY >= BUTTON_S_Y && mouseY <= BUTTON_S_Y + BUTTON_HEIGHT);
                            hoverG = (mouseX >= BUTTON_G_X && mouseX <= BUTTON_G_X + BUTTON_WIDTH && mouseY >= BUTTON_G_Y && mouseY <= BUTTON_G_Y + BUTTON_HEIGHT);
                        } else {
                            hoverO = hoverS = hoverG = 0;
                        }
                    }
                }
            }
        } 

        // --- Render ---
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        if (getIsGameStarted()) {
            draw_game_board(renderer, font);
            draw_role_table(renderer, font);
            if (popupState != POPUP_NONE && popupState != POPUP_G) draw_selection_popup(renderer, font);
            else if (popupState == POPUP_G) draw_guess_popup(renderer, font);
            if (getIsGameEnded()) show_end_popup(renderer, font, getLastResult());
        }
        else if (getIsConnected()) {
            render_text(renderer, font, "Sherlock 13 - Lobby", 520, 100, black);
            render_text(renderer, font, "Connected to Server!", 530, 150, blue);
            char countMsg[64];
            snprintf(countMsg, sizeof(countMsg), "Players Joined: %d / 4", getPlayerCount());
            render_text(renderer, font, countMsg, 540, 200, black);
            int startY = 260;
            for (int i = 0; i < getPlayerCount(); i++) {
                char playerRow[128];
                const char* pname = getPlayerName(i);
                if (i == getMyClientId()) snprintf(playerRow, sizeof(playerRow), "Player %d: %s (YOU)", i, pname);
                else snprintf(playerRow, sizeof(playerRow), "Player %d: %s", i, pname);
                render_text(renderer, font, playerRow, 500, startY + (i * 40), (i == getMyClientId()) ? blue : black);
            }
            if (getPlayerCount() < 4) render_text(renderer, font, "Waiting for more players...", 510, 500, gray);
            else render_text(renderer, font, "Launching Game...", 550, 500, red);
        }
        else {
            render_text(renderer, font, "Login:", 400, 250, black);
            SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
            SDL_RenderFillRect(renderer, &inputBox);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &inputBox);
            render_text(renderer, font, inputText, inputBox.x + 10, inputBox.y + 5, black);
            SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
            SDL_RenderFillRect(renderer, &goButton);
            render_text(renderer, font, "Connect", goButton.x + 35, goButton.y + 10, white);
            char serverInfo[300];
            snprintf(serverInfo, sizeof(serverInfo), "Server: %s", serverIP);
            render_text(renderer, font, serverInfo, 20, WINDOW_HEIGHT - 30, gray);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_StopTextInput();
    if (font) TTF_CloseFont(font);
    unload_textures();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void draw_game_board(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Color black = {0, 0, 0};
    SDL_Color red = {255, 0, 0};
    SDL_Color gray = {150, 150, 150};
    int marginLeft = 100, marginTop = 70;
    int cellSize = 35;
    int playerCount = getPlayerCount();
    if (playerCount > 4) playerCount = 4;
    int currentPlayer = getCurrentPlayer();

    for (int i = 0; i < 8; ++i) {
        render_icon(renderer, icons[i], marginLeft + i * 60, marginTop, 32);
        char countStr[16];
        int* counts = getObjectCounts();
        if (counts) {
            snprintf(countStr, sizeof(countStr), "%d", counts[i]);
            render_text(renderer, font, countStr, marginLeft + i * 60 + 10, marginTop + 35, black);
        }
    }

    SDL_Color blue = {0, 0, 255};
    char currentUserText[64];
    snprintf(currentUserText, sizeof(currentUserText), "Player: %s", getUsername());
    render_text(renderer, font, currentUserText, 20, 20, blue);

    int startY = marginTop + 70;
    for (int p = 0; p < playerCount; ++p) {
        SDL_Color color = black;
        if (!isPlayerAlive(p)) color = gray;
        else if (p == currentPlayer) color = red;
        render_text(renderer, font, getPlayerName(p), marginLeft - 80, startY + p * 40 + 5, color);
        for (int o = 0; o < 8; ++o) {
            SDL_Rect rect = {marginLeft + o * 60, startY + p * 40, cellSize, cellSize};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &rect);
            char val[16];
            snprintf(val, sizeof(val), "%d", getTableValue(p, o));
            render_text(renderer, font, val, rect.x + 12, rect.y + 8, black);
        }
    }

    render_cards(renderer, cards, getMyCards(), 3);
    render_osg_buttons(renderer, font);
 
    char currentUserid[128];
    snprintf(currentUserid, sizeof(currentUserid), "Current ID: %s", getLastResult());
    render_text(renderer, font, currentUserid, WINDOW_WIDTH / 2 - 200, WINDOW_HEIGHT - 60, red);
    if (getIsGameStarted()) {
        SDL_Color turnColor = {255, 0, 0};
        int currentPlayerId = getCurrentTurnPlayer();
        char turnText[128];
        if (currentPlayerId >= 0) snprintf(turnText, sizeof(turnText), "Current Player: %s", getPlayerName(currentPlayerId));
        else strcpy(turnText, "Waiting for game start...");
        render_text(renderer, font, turnText, WINDOW_WIDTH / 2 - 200, WINDOW_HEIGHT - 90, turnColor);
    }
    render_text(renderer, font, "SHERLOCK 13", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT - 30, black);
}
 
void draw_role_table(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Color black = {0, 0, 0};
    const char* roleNames[13] = {
        "Sebastian Moran", "Irene Adler", "Inspector Lestrade", "Inspector Gregson",
        "Inspector Baynes", "Inspector Bradstreet", "Inspector Hopkins",
        "Sherlock Holmes", "John Watson", "Mycroft Holmes", "Mrs. Hudson",
        "Mary Morstan", "James Moriarty"
    };
    int roleObjects[13][8] = {
        {0,0,0,1,0,0,0,1}, {1,0,0,0,1,0,0,0}, {0,1,0,0,1,0,1,0},
        {0,1,1,0,0,0,1,0}, {0,0,1,1,0,0,0,1}, {1,0,0,0,0,1,1,0},
        {1,1,0,0,0,1,0,0}, {0,0,1,0,1,0,1,0}, {1,1,0,0,0,0,1,0},
        {0,0,0,0,1,1,0,1}, {1,0,1,0,0,0,0,1}, {0,1,1,0,0,1,0,0},
        {0,0,0,1,1,0,1,0}
    };
    int startX = 20, startY = 400;
    for (int i = 0; i < 13; ++i) {
        render_text(renderer, font, roleNames[i], startX + ((i < 7) ? 0 : 450), startY + (i % 7) * 35, black);
        for (int j = 0; j < 8; ++j) {
            if (roleObjects[i][j]) {
                int x = startX + ((i < 7) ? 200 : 630) + j * 26;
                render_icon(renderer, icons[j], x, startY + (i % 7) * 35, 30);
            }
        }
    }
}

void render_osg_buttons(SDL_Renderer* renderer, TTF_Font* font) {
    const char *labels[3] = {"Observe (O)", "Speculate (S)", "Guess (G)"};
    SDL_Color black = {0, 0, 0, 255};
    SDL_Rect btnO = {BUTTON_O_X, BUTTON_O_Y, BUTTON_WIDTH, BUTTON_HEIGHT};
    SDL_SetRenderDrawColor(renderer, hoverO ? 255 : 200, hoverO ? 220 : 200, hoverO ? 0 : 200, 255);
    SDL_RenderFillRect(renderer, &btnO);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &btnO);
    render_text(renderer, font, labels[0], btnO.x + 13, btnO.y + 10, black);

    SDL_Rect btnS = {BUTTON_S_X, BUTTON_S_Y, BUTTON_WIDTH, BUTTON_HEIGHT};
    SDL_SetRenderDrawColor(renderer, hoverS ? 0 : 200, hoverS ? 220 : 200, hoverS ? 255 : 200, 255);
    SDL_RenderFillRect(renderer, &btnS);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
    SDL_RenderDrawRect(renderer, &btnS);
    render_text(renderer, font, labels[1], btnS.x + 9, btnS.y + 10, black);

    SDL_Rect btnG = {BUTTON_G_X, BUTTON_G_Y, BUTTON_WIDTH, BUTTON_HEIGHT};
    SDL_SetRenderDrawColor(renderer, hoverG ? 255 : 200, hoverG ? 0 : 200, hoverG ? 0 : 200, 255);
    SDL_RenderFillRect(renderer, &btnG);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &btnG);
    render_text(renderer, font, labels[2], btnG.x + 22, btnG.y + 10, black);
}