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

// Button Dimensions
// O & S Popup Dimensions (Observe / Speculate)
#define OS_OK_X       240  // OK button X
#define OS_CANCEL_X   360  // Cancel button X
#define OS_BTN_Y      380  // Y coordinate for OK/Cancel buttons in O/S popup
#define OS_BTN_W      80   // Button width
#define OS_BTN_H      30   // Button height

// G Popup Dimensions (Guess)
#define G_OK_X        400  // OK button X
#define G_CANCEL_X    520  // Cancel button X
#define G_BTN_Y       580  // Y coordinate for OK/Cancel buttons in G popup
#define G_BTN_W       80   // Button width
#define G_BTN_H       30   // Button height


static int showEndDialog = 0;     ///< Flag for showing end dialog
static PopupState popupState = POPUP_NONE;  ///< Current popup state

// Tracking Mouse State
static int mouseX = 0, mouseY = 0;
static int hoverO = 0, hoverS = 0, hoverG = 0;

// Record selected object and player in popup
static int selectedObjectId = -1;
static int selectedPlayerId = -1;
static int selectedGuessCard = -1;

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
    if (!tex || !renderer) return;
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
    if (!mycards || !cards || !renderer) return;
    if (!mycards) return;
    for (int i = 0; i < cardCount; ++i) {
        int idx = mycards[i];
        if (idx >= 0 && idx < 13 && cards[idx]) {
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

        char label[64]; // Increased buffer size safety
        snprintf(label, sizeof(label), "%d: %s", i + 1, nameobjets[i]);
        render_text(renderer, font, label, 205, 155 + i * 25, black);
    }

    // Only show player selection for S action
    if (popupState == POPUP_S_SELECT_OBJ || popupState == POPUP_S_SELECT_PLAYER) {
        render_text(renderer, font, "Joueurs:", 420, 120, black);
        int pc = getPlayerCount();
        if (pc > 4) pc = 4; // Safety clamp

        for (int i = 0; i < pc; ++i) {
            SDL_Rect box = {420, 160 + i * 25, 180, 22};
            if (i == selectedPlayerId) {
                SDL_SetRenderDrawColor(renderer, 200, 200, 0, 100);
                SDL_RenderFillRect(renderer, &box);
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &box);

            char label[64]; // Increased buffer size safety
            snprintf(label, sizeof(label), "%d: %s", i + 1, getPlayerName(i));
            render_text(renderer, font, label, 425, 155 + i * 25, black);
        }
    }

    // OK / Cancel buttons rendering
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

    // Draw OK / Cancel buttons
    SDL_Rect okBtn = {G_OK_X, G_BTN_Y, G_BTN_W, G_BTN_H};
    SDL_SetRenderDrawColor(renderer, 200, 255, 200, 255);
    SDL_RenderFillRect(renderer, &okBtn);
    SDL_RenderDrawRect(renderer, &okBtn);
    render_text(renderer, font, "OK", okBtn.x + 20, okBtn.y + 2, black);

    SDL_Rect cancelBtn = {G_CANCEL_X, G_BTN_Y, G_BTN_W, G_BTN_H};
    SDL_SetRenderDrawColor(renderer, 255, 200, 200, 255);
    SDL_RenderFillRect(renderer, &cancelBtn);
    SDL_RenderDrawRect(renderer, &cancelBtn);
    render_text(renderer, font, "Cancel", cancelBtn.x + 10, cancelBtn.y + 2, black);
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
/**
 * @brief Main GUI loop
 * * This function handles the entire lifecycle of the graphical interface:
 * 1. Initialization of SDL and resources.
 * 2. Event polling (Keyboard, Mouse, Text Input).
 * 3. State management (Login -> Lobby -> Game -> End).
 * 4. Rendering.
 * 5. Cleanup.
 */
void run_gui() {
    // --- 1. Initialization Phase ---

    // Initialize SDL Video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "[GUI Error] SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return; // Exit immediately if SDL fails
    }

    // Initialize PNG loading
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        fprintf(stderr, "[GUI Error] SDL_image could not initialize! IMG_Error: %s\n", IMG_GetError());
        SDL_Quit();
        return;
    }

    // Initialize TrueType Fonts
    if (TTF_Init() == -1) {
        fprintf(stderr, "[GUI Error] SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return;
    }

    // Create the main window
    SDL_Window *window = SDL_CreateWindow("Sherlock 13 - Client", 
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT, 
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "[GUI Error] Window could not be created! SDL_Error: %s\n", SDL_GetError());
        // Cleanup and exit
        TTF_Quit(); IMG_Quit(); SDL_Quit();
        return;
    }

    // Create the renderer (Hardware accelerated)
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "[GUI Error] Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit(); IMG_Quit(); SDL_Quit();
        return;
    }

    // Load the font (Safety check included)
    TTF_Font *font = TTF_OpenFont("img/sans.ttf", 20);
    if (!font) {
        fprintf(stderr, "[GUI Error] Failed to load font! TTF_Error: %s\n", TTF_GetError());
        // Handle missing resource gracefully
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit(); IMG_Quit(); SDL_Quit();
        return;
    }

    // Define colors for UI
    SDL_Color black = {0, 0, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    // SDL_Color green = {100, 200, 100, 255};
    SDL_Color gray  = {200, 200, 200, 255};
    SDL_Color blue  = {0, 0, 255, 255};
    SDL_Color red   = {255, 0, 0, 255};

    // Load texture resources (Icons and Cards)
    load_all_textures(renderer);

    // --- 2. UI Element Definitions ---

    // Input Box variables (Login Screen)
    char inputText[32] = ""; // Buffer for username
    SDL_Rect inputBox = {400, 300, 400, 40};
    SDL_Rect goButton = {550, 360, 140, 40};
    
    // Popup Button Definitions (Game Screen)
    // These match the popup logic coordinates
    SDL_Rect okBtn = {G_OK_X, G_BTN_Y, G_BTN_W, G_BTN_H};
    SDL_Rect cancelBtn = {G_CANCEL_X, G_BTN_Y, G_BTN_W, G_BTN_H};

    // Enable Text Input for the login screen
    SDL_StartTextInput();

    // Main loop control flag
    int running = 1;
    SDL_Event e;
    static int connectClicked = 0; // Prevent repeated clicks on the flag

    // --- 3. Main Event Loop ---
    while (running) {
        
        // Handle all events in the queue
        while (SDL_PollEvent(&e)) {
            
            // Global Quit Event (Clicking "X" on window)
            if (e.type == SDL_QUIT) {
                running = 0;
            }

            // ---------------------------------------------------------
            // SCENARIO A: User is NOT connected (Login Screen)
            // ---------------------------------------------------------
            else if (!getIsConnected()) {
                
                // Handle text input (Safe buffering)
                if (e.type == SDL_TEXTINPUT) {
                    // Check buffer size to prevent overflow (Security)
                    if (strlen(inputText) + strlen(e.text.text) < sizeof(inputText) - 1) {
                        strcat(inputText, e.text.text);
                    }
                }
                // Handle Backspace
                else if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_BACKSPACE && strlen(inputText) > 0) {
                        inputText[strlen(inputText) - 1] = '\0';
                    }
                }
                // Handle "Connect" Button Click
                else if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int x = e.button.x;
                    int y = e.button.y;

                    // Check if click is inside the "Connect" button
                    if (x >= goButton.x && x <= goButton.x + goButton.w &&
                        y >= goButton.y && y <= goButton.y + goButton.h) {
                        
                        // Validation: Prevent multiple clicks and ensure username is not empty
                        if (connectClicked) {
                            // Already clicked,  print debug info
                            printf("[GUI] Wait for server response.\n");
                        } 
                        else if (strlen(inputText) == 0) {
                            // Username is empty
                            printf("[GUI Warning] Attempted to connect with empty username.\n");
                        } 
                        else {
                            // Valid click, proceed with connection logic
                            connectClicked = 1; // Set flag to prevent repeated clicks
                            setUsername(inputText);
                            sendConnect(getUsername(), getClientPort());
                            printf("[GUI] Connect request sent for user: %s\n", inputText);
                        }
                    }
                }
            }

            // ---------------------------------------------------------
            // SCENARIO B: User IS Connected (Lobby or Game)
            // ---------------------------------------------------------
            else {
                // Global Key Events for Connected State
                if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_ESCAPE) {
                        // Close any active popup
                        if (popupState != POPUP_NONE) {
                            popupState = POPUP_NONE;
                            // Reset selection states for safety
                            selectedObjectId = -1;
                            selectedPlayerId = -1;
                            selectedGuessCard = -1;
                        }
                    }
                }

                // Only process game logic clicks if the game has actively started
                if (getIsGameStarted()) {
                    
                    if (e.type == SDL_MOUSEBUTTONDOWN) {
                        int x = e.button.x;
                        int y = e.button.y;

                        // [DEBUG] Print click coordinates and popup state for troubleshooting
                        // printf("[GUI Debug] Click at x=%d, y=%d. PopupState=%d\n", x, y, popupState);
                        // printf("[GUI Debug] x_Left=%d, x_Right=%d, y_Top=%d, y_Bottom=%d\n", okBtn.x, okBtn.x + okBtn.w, okBtn.y, okBtn.y + okBtn.h);

                        // 1. Handle Main Game Board Buttons (O/S/G)
                        // Safety Check: Only allow actions if it is this client's turn AND no popup is open
                        if (isTurn() && popupState == POPUP_NONE) {
                            
                            // Check Action O (Observe)
                            if (x >= BUTTON_O_X && x <= BUTTON_O_X + BUTTON_WIDTH &&
                                y >= BUTTON_O_Y && y <= BUTTON_O_Y + BUTTON_HEIGHT) {
                                popupState = POPUP_O;
                            }
                            // Check Action S (Speculate)
                            else if (x >= BUTTON_S_X && x <= BUTTON_S_X + BUTTON_WIDTH &&
                                     y >= BUTTON_S_Y && y <= BUTTON_S_Y + BUTTON_HEIGHT) {
                                popupState = POPUP_S_SELECT_OBJ;
                            }
                            // Check Action G (Guess)
                            else if (x >= BUTTON_G_X && x <= BUTTON_G_X + BUTTON_WIDTH &&
                                     y >= BUTTON_G_Y && y <= BUTTON_G_Y + BUTTON_HEIGHT) {
                                popupState = POPUP_G;
                            }
                        }

                        // 2. Handle Logic inside Popups (Selection & Confirmation)
                        if (popupState != POPUP_NONE) {
                            
                            // --- Handling Selection Logic for O and S ---
                            if (popupState == POPUP_O || popupState == POPUP_S_SELECT_OBJ || popupState == POPUP_S_SELECT_PLAYER) {
                                
                                // Detect Object Selection (Icons list)
                                for (int i = 0; i < 8; ++i) {
                                    SDL_Rect objBox = {200, 160 + i * 25, 200, 22};
                                    if (x >= objBox.x && x <= objBox.x + objBox.w &&
                                        y >= objBox.y && y <= objBox.y + objBox.h) {
                                        selectedObjectId = i;
                                    }
                                }

                                // Detect Player Selection (Only for S action)
                                int pc = getPlayerCount();
                                for (int i = 0; i < pc; ++i) {
                                    SDL_Rect playerBox = {420, 160 + i * 25, 180, 22};
                                    if (x >= playerBox.x && x <= playerBox.x + playerBox.w &&
                                        y >= playerBox.y && y <= playerBox.y + playerBox.h) {
                                        selectedPlayerId = i;
                                    }
                                }

                                // Handle OK Button of action O and S
                                if (x >= OS_OK_X && x <= OS_OK_X + OS_BTN_W && 
                                    y >= OS_BTN_Y && y <= OS_BTN_Y + OS_BTN_H) {
                                    
                                    // Validation: Ensure required selections are made before sending
                                    if (popupState == POPUP_O) {
                                        if (selectedObjectId >= 0 && selectedObjectId < 8) {
                                            send_action_request('O', selectedObjectId, 0);
                                            popupState = POPUP_NONE; // Close popup
                                        }
                                    } else if (popupState == POPUP_S_SELECT_OBJ) {
                                        // User logic: First select object, then ensure player is selected
                                        if (selectedObjectId >= 0 && selectedPlayerId >= 0) {
                                            send_action_request('S', selectedObjectId, selectedPlayerId);
                                            popupState = POPUP_NONE;
                                        }
                                    }
                                    
                                    // Reset selections after action
                                    selectedObjectId = -1;
                                    selectedPlayerId = -1;
                                }

                                // Handle Cancel Button
                                if (x >= cancelBtn.x && x <= cancelBtn.x + cancelBtn.w &&
                                    y >= cancelBtn.y && y <= cancelBtn.y + cancelBtn.h) {
                                    popupState = POPUP_NONE;
                                    selectedObjectId = -1;
                                    selectedPlayerId = -1;
                                }
                            }

                            // --- Handling Selection Logic for G (Guessing) ---
                            else if (popupState == POPUP_G) {
                                // Detect Card Selection
                                for (int i = 0; i < 13; ++i) {
                                    // Calculate grid position (must match draw_guess_popup logic)
                                    int row = i / COLUMNS_PER_ROW;
                                    int col = i % COLUMNS_PER_ROW;
                                    int cx = CARD_START_X + col * (BG_CARD_WIDTH + COLUMN_SPACING);
                                    int cy = CARD_START_Y + row * (BG_CARD_HEIGHT + ROW_SPACING);
                                    
                                    if (x >= cx && x <= cx + BG_CARD_WIDTH &&
                                        y >= cy && y <= cy + BG_CARD_HEIGHT) {
                                        selectedGuessCard = i;
                                    }
                                }

                                // Handle OK Button of action G
                                if (x >= okBtn.x && x <= okBtn.x + okBtn.w &&
                                    y >= okBtn.y && y <= okBtn.y + okBtn.h) {
                                    // Validation: Must have selected a card
                                    if (selectedGuessCard >= 0 && selectedGuessCard < 13) {
                                        send_action_request('G', selectedGuessCard, 0);
                                        popupState = POPUP_NONE;
                                    }
                                    selectedGuessCard = -1;
                                }

                                // Handle Cancel Button
                                if (x >= cancelBtn.x && x <= cancelBtn.x + cancelBtn.w &&
                                    y >= cancelBtn.y && y <= cancelBtn.y + cancelBtn.h) {
                                    popupState = POPUP_NONE;
                                    selectedGuessCard = -1;
                                }
                            }
                        } // End of Popup handling

                        // 3. Handle Game Over "Quit" Button
                        if (getIsGameEnded()) {
                             // Coordinates for the quit button inside show_end_popup
                             if (x >= 300 && x <= 400 && y >= 300 && y <= 330) {
                                 running = 0; // Exit Loop
                             }
                        }
                    }

                    // Mouse Motion Handling (Hover Effects)
                    if (e.type == SDL_MOUSEMOTION) {
                        mouseX = e.motion.x;
                        mouseY = e.motion.y;
                        
                        // Update hover flags only if no popup is blocking interaction
                        if (popupState == POPUP_NONE) {
                            hoverO = (mouseX >= BUTTON_O_X && mouseX <= BUTTON_O_X + BUTTON_WIDTH &&
                                      mouseY >= BUTTON_O_Y && mouseY <= BUTTON_O_Y + BUTTON_HEIGHT);
                            hoverS = (mouseX >= BUTTON_S_X && mouseX <= BUTTON_S_X + BUTTON_WIDTH &&
                                      mouseY >= BUTTON_S_Y && mouseY <= BUTTON_S_Y + BUTTON_HEIGHT);
                            hoverG = (mouseX >= BUTTON_G_X && mouseX <= BUTTON_G_X + BUTTON_WIDTH &&
                                      mouseY >= BUTTON_G_Y && mouseY <= BUTTON_G_Y + BUTTON_HEIGHT);
                        } else {
                            // Reset hover if popup is open
                            hoverO = 0; hoverS = 0; hoverG = 0;
                        }
                    }
                }
            }
        } // --- End of Event Polling ---

        // --- 4. Rendering Phase ---

        // Clear the screen with white background
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // --- RENDER STATE 1: GAME IN PROGRESS ---
        if (getIsGameStarted()) {
            
            // Draw the main board (Players, Objects, Hand)
            draw_game_board(renderer, font);
            
            // Draw the helper table (Characters and their items)
            draw_role_table(renderer, font);

            // Draw Popups on top if active
            if (popupState == POPUP_O || popupState == POPUP_S_SELECT_OBJ || popupState == POPUP_S_SELECT_PLAYER) {
                draw_selection_popup(renderer, font);
            } else if (popupState == POPUP_G) {
                draw_guess_popup(renderer, font);
            }

            // Draw Game Over Dialog if applicable
            if (getIsGameEnded()) {
                show_end_popup(renderer, font, getLastResult());
            }
        } 
        
        // --- RENDER STATE 2: CONNECTED BUT WAITING (LOBBY) ---
        else if (getIsConnected()) {
            // Draw Lobby Title
            render_text(renderer, font, "Sherlock 13 - Lobby", 520, 100, black);
            render_text(renderer, font, "Connected to Server!", 530, 150, blue);
            
            // Draw Player Count
            char countMsg[64];
            int currentCount = getPlayerCount();
            snprintf(countMsg, sizeof(countMsg), "Players Joined: %d / 4", currentCount);
            render_text(renderer, font, countMsg, 540, 200, black);

            // Draw Player List (Dynamic)
            int startY = 260;
            for (int i = 0; i < currentCount; i++) {
                char playerRow[128];
                const char* pname = getPlayerName(i);
                
                // Highlight myself
                if (i == getMyClientId()) {
                    snprintf(playerRow, sizeof(playerRow), "Player %d: %s (YOU)", i, pname);
                    render_text(renderer, font, playerRow, 500, startY + (i * 40), blue);
                } else {
                    snprintf(playerRow, sizeof(playerRow), "Player %d: %s", i, pname);
                    render_text(renderer, font, playerRow, 500, startY + (i * 40), black);
                }
            }

            // Draw Waiting Animation Text
            if (currentCount < 4) {
                 render_text(renderer, font, "Waiting for more players...", 510, 500, gray);
            } else {
                 render_text(renderer, font, "Launching Game...", 550, 500, red);
            }
        }
        
        // --- RENDER STATE 3: NOT CONNECTED (LOGIN) ---
        else {
            // Draw Login Interface
            render_text(renderer, font, "Sherlock 13 - Login", 550, 150, black);
            render_text(renderer, font, "Enter your nickname:", 400, 250, black);
            
            // Draw Input Box Background
            SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
            SDL_RenderFillRect(renderer, &inputBox);
            // Draw Input Box Border
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &inputBox);
            
            // Draw current text input
            render_text(renderer, font, inputText, inputBox.x + 10, inputBox.y + 5, black);
            
            // Draw Connect Button
            SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
            SDL_RenderFillRect(renderer, &goButton);
            render_text(renderer, font, "Connect", goButton.x + 35, goButton.y + 10, white);
            
            // Draw server info
            char serverInfo[300];
            snprintf(serverInfo, sizeof(serverInfo), "Server: %s", serverIP); // serverIP is global in client_logic
            render_text(renderer, font, serverInfo, 20, WINDOW_HEIGHT - 30, gray);
        }

        // Swap buffers (Display frame)
        SDL_RenderPresent(renderer);
        
        // Small delay to reduce CPU usage
        SDL_Delay(16); 
    }

    // --- 5. Cleanup Phase ---
    SDL_StopTextInput();
    if (font) TTF_CloseFont(font);
    
    // Unload textures (Helper function)
    unload_textures();
    
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    
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
    if (playerCount > 4) playerCount = 4; // Safety Clamp

    int currentPlayer = getCurrentPlayer();

    // Top object icons and counts
    for (int i = 0; i < 8; ++i) {
        render_icon(renderer, icons[i], marginLeft + i * 60, marginTop, 32);
        char countStr[16]; // Increased from 4 to 16 to prevent overflow
        // Safety check for pointer
        int* counts = getObjectCounts();
        if (counts) {
            snprintf(countStr, sizeof(countStr), "%d", counts[i]);
            render_text(renderer, font, countStr, marginLeft + i * 60 + 10, marginTop + 35, black);
        }
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
            char val[16]; // Increased buffer size for safety
            snprintf(val, sizeof(val), "%d", getTableValue(p, o));
            render_text(renderer, font, val, rect.x + 12, rect.y + 8, black);
        }
    }

    // Show player's cards on the right
    render_cards(renderer, cards, getMyCards(), 3);
 
    // OSG action buttons
    render_osg_buttons(renderer, font);
 
    // Bottom display
    char currentUserid[128];    // Safe buffer
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
