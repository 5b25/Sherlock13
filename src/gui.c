// gui.c
#include "../include/gui.h"
#include "../include/client_logic.h"
#include "../include/resources.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <string.h>

// Window dimension constants
#define WINDOW_WIDTH 1280    ///< Window width
#define WINDOW_HEIGHT 768    ///< Window height
#define ICON_SIZE 32         ///< Icon size
#define CARD_WIDTH 300       ///< Card width
#define CARD_HEIGHT 200      ///< Card height
#define BUTTON_WIDTH 130     ///< Button width
#define BUTTON_HEIGHT 40     ///< Button height
#define BUTTON_O_X 650
#define BUTTON_O_Y 100
#define BUTTON_S_X 650
#define BUTTON_S_Y 160
#define BUTTON_G_X 650 
#define BUTTON_G_Y 220

#define POPUP_WIDTH          1000
#define POPUP_HEIGHT         560
#define CARD_START_X         220
#define CARD_START_Y         180
#define BG_CARD_WIDTH        160
#define BG_CARD_HEIGHT       100
#define COLUMNS_PER_ROW      5
#define COLUMN_SPACING       20
#define ROW_SPACING          40

static int showEndDialog = 0;     ///< Flag for showing end dialog
static PopupState popupState = POPUP_NONE;  ///< Current popup state

// Tracking Mouse State
static int mouseX = 0, mouseY = 0;
static int hoverO = 0, hoverS = 0, hoverG = 0;

// Record selected object and player in popup
static int selectedObjectId = -1;
static int selectedPlayerId = -1;
static int selectedGuessCard = -1;

int okBtnX = 400, okBtnY = 580;    // OK button coordinates
int cancelBtnX = 520, cancelBtnY = 580;  // Cancel button coordinates

// Message display area parameters
#define MESSAGE_BOX_HEIGHT    40     // Message box height
#define MESSAGE_FONT_SIZE     18     // Font size
#define MESSAGE_PADDING       10     // Padding
#define MESSAGE_MAX_LENGTH    256    // Max message length

/**
 * @brief Render text at specified position
 * 
 * @param renderer SDL renderer
 * @param font Font object
 * @param text Text to render
 * @param x Top-left x coordinate
 * @param y Top-left y coordinate
 * @param color Text color
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
 * @brief Render icon at specified position
 * 
 * @param renderer SDL renderer
 * @param tex Icon texture
 * @param x Top-left x coordinate
 * @param y Top-left y coordinate
 * @param size Icon size
 */
void render_icon(SDL_Renderer *renderer, SDL_Texture *tex, int x, int y, int size) {
    if (!tex) return;
    SDL_Rect dst = {x, y, size, size};
    SDL_RenderCopy(renderer, tex, NULL, &dst);
}
 
/**
 * @brief Render player's cards
 * 
 * @param renderer SDL renderer
 * @param cards Array of card textures
 * @param mycards Array of player's card indices
 * @param cardCount Number of cards
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
 * @brief Draw selection popup (for Observe/Speculate actions)
 * 
 * @param renderer SDL renderer
 * @param font Font object
 */
void draw_selection_popup(SDL_Renderer* renderer, TTF_Font* font) {
    // Draw popup background
    SDL_Rect popup = {180, 100, 440, 380};
    SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
    SDL_RenderFillRect(renderer, &popup);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &popup);

    SDL_Color black = {0, 0, 0};

    // Show object selection (shared by O and S)
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

    // Only show player selection for S action
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

    // OK / Cancel buttons
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
  * @brief Draw guess popup
  * 
  * @param renderer SDL renderer
  * @param font Font object
  */
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

    // OK / Cancel buttons
    SDL_Rect okBtn = {okBtnX, okBtnY, 80, 30};
    SDL_Rect cancelBtn = {cancelBtnX, cancelBtnY, 80, 30};
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
  * @brief Show end game popup
  * 
  * @param renderer SDL renderer
  * @param font Font object
  * @param result Game result text
  */
 void show_end_popup(SDL_Renderer* renderer, TTF_Font* font, const char* result) {
     if (!showEndDialog) return;
     
     // Draw popup background
     SDL_Rect popup = {180, 180, 420, 180};
     SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
     SDL_RenderFillRect(renderer, &popup);
     SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
     SDL_RenderDrawRect(renderer, &popup);
 
     SDL_Color red = {255, 0, 0};
     SDL_Color green = {0, 160, 0};
     SDL_Color black = {0, 0, 0};
     
     // Draw title and result
     render_text(renderer, font, "GAME OVER", 250, 200, red);
     render_text(renderer, font, result, 250, 240, green);
 
     // Draw quit button
     SDL_Rect quitBtn = {300, 300, 100, 30};
     SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
     SDL_RenderFillRect(renderer, &quitBtn);
     SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
     SDL_RenderDrawRect(renderer, &quitBtn);
     render_text(renderer, font, "QUITTER", 320, 305, black);
 }
 
 /**
  * @brief Set whether to show end dialog
  * 
  * @param value 1=show/0=hide
  */
 void setShowEndDialog(int value) {
     showEndDialog = value;
 }
 
/**
 * @brief Send action request to server
 * 
 * @param type Action type (O/S/G)
 * @param objet Object ID
 * @param cible Target ID (only needed for S action)
 */
void send_action_request(char type, int objet, int cible) {
    if (type == 'O') {
        sendActionO(objet);// Format: O obj
    } else if (type == 'S') {
        sendActionS(cible, objet); // Format: S myClientId target_id obj
    } else if (type == 'G') {
        sendActionG(objet); // Format: G myClientId card
    }
}
 
/**
 * @brief Main GUI loop
 * 
 * Initialize SDL window and renderer, handle user input, render game interface
 */
void run_gui() {
    // Initialize SDL subsystems
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
 
    // Create window and renderer
    SDL_Window *window = SDL_CreateWindow("Sherlock13", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Ensure the font file exists. If not, the program will exit with an error message.
    TTF_Font *font = TTF_OpenFont("img/sans.ttf", 20);
    if (!font) {
        printf("Font error: %s\n", TTF_GetError());
        return;
    }
 
    SDL_Color black = {0, 0, 0};
     
    // Load all texture resources
    load_all_textures(renderer);
 
    // Username input box variables
    char inputText[32] = "";
    SDL_Rect inputBox = {400, 300, 400, 40};
    SDL_Rect goButton = {550, 360, 140, 40};
    SDL_StartTextInput();
 
    // Main loop control variable
    int running = 1;
    SDL_Event e;
 
    // Main event loop
    while (running) {
        // Process all pending events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            }
            // Handle text input
            else if (e.type == SDL_TEXTINPUT && strlen(inputText) < 31) {
                strcat(inputText, e.text.text);
            }
            // Handle keyboard presses
            else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    if (popupState != POPUP_NONE) {
                        // Press ESC to close popup
                        popupState = POPUP_NONE;
                        selectedObjectId = -1;
                        selectedPlayerId = -1;
                        selectedGuessCard = -1;
                    }
                } else if (e.key.keysym.sym == SDLK_BACKSPACE && strlen(inputText) > 0) {
                    // Delete input text
                    inputText[strlen(inputText) - 1] = '\0';
                }
            }
            // Handle mouse clicks
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int x = e.button.x, y = e.button.y;

                // Handle username input when game not started
                if (!getIsGameStarted() && x >= goButton.x && x <= goButton.x + goButton.w &&
                   y >= goButton.y && y <= goButton.y + goButton.h && inputText[0] != '\0' && !isUsernameSet()) {
                    setUsername(inputText);
                    sendConnect(getUsername(), getClientPort());
                }
 
                // Handle action buttons when game is in progress and it's current player's turn
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
                
                    // Select object
                    for (int i = 0; i < 8; ++i) {
                        SDL_Rect objBox = {200, 160 + i * 25, 200, 22};
                        if (x >= objBox.x && x <= objBox.x + objBox.w && y >= objBox.y && y <= objBox.y + objBox.h) {
                            selectedObjectId = i;
                        }
                    }
                
                    // Select player (for S action)
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
                        // Send request based on action type
                        if (popupState == POPUP_O && selectedObjectId >= 0) {
                            send_action_request('O', selectedObjectId, 0);
                        } else if (popupState == POPUP_S_SELECT_OBJ && selectedObjectId >= 0 && selectedPlayerId >= 0) {
                            send_action_request('S', selectedObjectId, selectedPlayerId);
                        }
                        // Reset state
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
                
                    // Character card selection
                    for (int i = 0; i < 13; ++i) {
                        int row = i / COLUMNS_PER_ROW;
                        int col = i % COLUMNS_PER_ROW;
                        int cx = CARD_START_X + col * (BG_CARD_WIDTH + COLUMN_SPACING);
                        int cy = CARD_START_Y + row * (BG_CARD_HEIGHT + ROW_SPACING);
                        SDL_Rect cardRect = {cx, cy, BG_CARD_WIDTH, BG_CARD_HEIGHT};
                
                        if (x >= cardRect.x && x <= cardRect.x + cardRect.w && y >= cardRect.y && y <= cardRect.y + cardRect.h) {
                            selectedGuessCard = i;
                        }
                    }
                
                    // OK / Cancel
                    SDL_Rect okBtn = {okBtnX, okBtnY, 80, 30};
                    SDL_Rect cancelBtn = {cancelBtnX, cancelBtnY, 80, 30};
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
                
                // Handle quit button when game ends
                if (getIsGameEnded()) {
                    if (x >= 300 && x <= 400 && y >= 300 && y <= 330) {
                        running = 0;
                    }
                }
            }
            // Handle mouse motion events
            else if (e.type == SDL_MOUSEMOTION) {
                mouseX = e.motion.x;
                mouseY = e.motion.y;
                
                // Update hover state
                hoverO = (mouseX >= BUTTON_O_X && mouseX <= BUTTON_O_X + BUTTON_WIDTH &&
                         mouseY >= BUTTON_O_Y && mouseY <= BUTTON_O_Y + BUTTON_HEIGHT);
                hoverS = (mouseX >= BUTTON_S_X && mouseX <= BUTTON_S_X + BUTTON_WIDTH &&
                         mouseY >= BUTTON_S_Y && mouseY <= BUTTON_S_Y + BUTTON_HEIGHT);
                hoverG = (mouseX >= BUTTON_G_X && mouseX <= BUTTON_G_X + BUTTON_WIDTH &&
                         mouseY >= BUTTON_G_Y && mouseY <= BUTTON_G_Y + BUTTON_HEIGHT);
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Render different interfaces based on game state
        if (!getIsGameStarted()) {
            // Render username input interface
            render_text(renderer, font, "Entrez votre nom:", 400, 250, black);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderFillRect(renderer, &inputBox);
            render_text(renderer, font, inputText, inputBox.x + 10, inputBox.y + 5, black);
            SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
            SDL_RenderFillRect(renderer, &goButton);
            render_text(renderer, font, "Connect", goButton.x + 33, goButton.y + 5, black);

            // Show list of connected players
            for (int i = 0; i < getPlayerCount(); ++i) {
                char line[64];
                snprintf(line, sizeof(line), "Joueur %d: %s", i + 1, getPlayerName(i));
                render_text(renderer, font, line, 400, 460 + i * 30, black);
            }
        } else {
            // Game in progress, render main game interface
            draw_game_board(renderer, font);
            draw_role_table(renderer, font);

            // Render different popups based on state
            if (popupState == POPUP_O || popupState == POPUP_S_SELECT_OBJ || popupState == POPUP_S_SELECT_PLAYER) {
                draw_selection_popup(renderer, font);
            } else if (popupState == POPUP_G) {
                draw_guess_popup(renderer, font);
            }

            // Show end popup when game ends
            if (getIsGameEnded()) {
                show_end_popup(renderer, font, getLastResult());
            }
        }

        // Update screen
        SDL_RenderPresent(renderer);
    }

    // Clean up resources
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
 * @brief Draw main game board
 * 
 * @param renderer SDL renderer
 * @param font Font object
 */
void draw_game_board(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Color black = {0, 0, 0};
    SDL_Color red = {255, 0, 0};
    SDL_Color gray = {150, 150, 150};

    // Layout parameters
    int marginLeft = 100, marginTop = 70;
    int cellSize = 35;
    int playerCount = getPlayerCount();
    int currentPlayer = getCurrentPlayer();

    // Top object icons and counts
    for (int i = 0; i < 8; ++i) {
        render_icon(renderer, icons[i], marginLeft + i * 60, marginTop, 32);
        char countStr[4];
        snprintf(countStr, sizeof(countStr), "%d", getObjectCounts()[i]);
        render_text(renderer, font, countStr, marginLeft + i * 60 + 10, marginTop + 35, black);
    }

    // Show current player name in top-left corner
    SDL_Color blue = {0, 0, 255};
    char currentUserText[64];
    snprintf(currentUserText, sizeof(currentUserText), "Player: %s", getUsername());
    render_text(renderer, font, currentUserText, 20, 20, blue);

    // Player object matrix
    int startY = marginTop + 70;
    for (int p = 0; p < playerCount; ++p) {
        // Set color based on player state
        SDL_Color color = black;
        if (!isPlayerAlive(p)) color = gray;
        else if (p == currentPlayer) color = red;

        // Draw player name
        render_text(renderer, font, getPlayerName(p), marginLeft - 80, startY + p * 40 + 5, color);

        // Draw player object matrix
        for (int o = 0; o < 8; ++o) {
            SDL_Rect rect = {marginLeft + o * 60, startY + p * 40, cellSize, cellSize};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &rect);

            // Show object value
            char val[2];
            snprintf(val, sizeof(val), "%d", getTableValue(p, o));
            render_text(renderer, font, val, rect.x + 12, rect.y + 8, black);
        }
    }

    // Show player's cards on the right
    render_cards(renderer, cards, getMyCards(), 3);
 
    // OSG action buttons
    render_osg_buttons(renderer, font);
 
    // Bottom display
    char currentUserid[30];
    snprintf(currentUserid, sizeof(currentUserid), "Current ID: %s", getLastResult());
    render_text(renderer, font, currentUserid, WINDOW_WIDTH / 2 - 200, WINDOW_HEIGHT - 60, red);
    // Show current player name at bottom
    if (getIsGameStarted()) {
        SDL_Color turnColor = {255, 0, 0}; // Highlight current player in red
        int currentPlayerId = -1;
        currentPlayerId = getCurrentTurnPlayer();

        // Show turn information with player name
        char turnText[128];
        if (currentPlayerId >= 0) {
            snprintf(turnText, sizeof(turnText), "Current Player: %s", getPlayerName(currentPlayerId));
        } else {
            strcpy(turnText, "Waiting for game start...");
        }
        render_text(renderer, font, turnText, WINDOW_WIDTH / 2 - 200, WINDOW_HEIGHT - 90, turnColor);
    }
    // Show logo at bottom
    render_text(renderer, font, "SHERLOCK 13", WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT - 30, black);
}
 
/**
 * @brief Draw character information table
 * 
 * @param renderer SDL renderer
 * @param font Font object
 */
void draw_role_table(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Color black = {0, 0, 0};
     
    // Character name array
    const char* roleNames[13] = {
        "Sebastian Moran", "Irene Adler", "Inspector Lestrade", "Inspector Gregson",
        "Inspector Baynes", "Inspector Bradstreet", "Inspector Hopkins",
        "Sherlock Holmes", "John Watson", "Mycroft Holmes", "Mrs. Hudson",
        "Mary Morstan", "James Moriarty"
    };
     
    // Character object matrix
    int roleObjects[13][8] = {
        {0,0,0,1,0,0,0,1}, {1,0,0,0,1,0,0,0}, {0,1,0,0,1,0,1,0},
        {0,1,1,0,0,0,1,0}, {0,0,1,1,0,0,0,1}, {1,0,0,0,0,1,1,0},
        {1,1,0,0,0,1,0,0}, {0,0,1,0,1,0,1,0}, {1,1,0,0,0,0,1,0},
        {0,0,0,0,1,1,0,1}, {1,0,1,0,0,0,0,1}, {0,1,1,0,0,1,0,0},
        {0,0,0,1,1,0,1,0}
    };
 
    // Draw character table
    int startX = 20, startY = 400;
    for (int i = 0; i < 13; ++i) {
        // Draw character name (display in two columns)
        render_text(renderer, font, roleNames[i], startX + ((i < 7) ? 0 : 450), startY + (i % 7) * 35, black);
         
        // Draw objects owned by character
        for (int j = 0; j < 8; ++j) {
            if (roleObjects[i][j]) {
                int x = startX + ((i < 7) ? 200 : 630) + j * 26;
                render_icon(renderer, icons[j], x, startY + (i % 7) * 35, 30);
            }
        }
    }
}

 // Button rendering function
void render_osg_buttons(SDL_Renderer* renderer, TTF_Font* font) {
    const char *labels[3] = {"Observe (O)", "Speculate (S)", "Guess (G)"};
    SDL_Color black = {0, 0, 0, 255};

    // O button
    SDL_Rect btnO = {BUTTON_O_X, BUTTON_O_Y, BUTTON_WIDTH, BUTTON_HEIGHT};
    SDL_SetRenderDrawColor(renderer, hoverO ? 255 : 200, hoverO ? 220 : 200, hoverO ? 0 : 200, 255);
    SDL_RenderFillRect(renderer, &btnO);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &btnO);
    render_text(renderer, font, labels[0], btnO.x + 13, btnO.y + 10, black);

    // S button 
    SDL_Rect btnS = {BUTTON_S_X, BUTTON_S_Y, BUTTON_WIDTH, BUTTON_HEIGHT};
    SDL_SetRenderDrawColor(renderer, hoverS ? 0 : 200, hoverS ? 220 : 200, hoverS ? 255 : 200, 255);
    SDL_RenderFillRect(renderer, &btnS);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
    SDL_RenderDrawRect(renderer, &btnS);
    render_text(renderer, font, labels[1], btnS.x + 9, btnS.y + 10, black);

    // G button
    SDL_Rect btnG = {BUTTON_G_X, BUTTON_G_Y, BUTTON_WIDTH, BUTTON_HEIGHT};
    SDL_SetRenderDrawColor(renderer, hoverG ? 255 : 200, hoverG ? 0 : 200, hoverG ? 0 : 200, 255);
    SDL_RenderFillRect(renderer, &btnG);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &btnG);
    render_text(renderer, font, labels[2], btnG.x + 22, btnG.y + 10, black);
}
