// gui.h
#ifndef GUI_H
#define GUI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h> 

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 768

typedef enum {
    POPUP_NONE,
    POPUP_O,
    POPUP_S_SELECT_OBJ,
    POPUP_S_SELECT_PLAYER,
    POPUP_G
} PopupState;

void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color);
void render_icon(SDL_Renderer *renderer, SDL_Texture *tex, int x, int y, int size);
void render_cards(SDL_Renderer* renderer, SDL_Texture* cards[], int* mycards, int cardCount);
void draw_selection_popup(SDL_Renderer* renderer, TTF_Font* font);
void draw_guess_popup(SDL_Renderer* renderer, TTF_Font* font);
void show_end_popup(SDL_Renderer* renderer, TTF_Font* font, const char* result);
void setShowEndDialog(int value);
void send_action_request(char type, int objet, int cible);
void run_gui();
void draw_game_board(SDL_Renderer* renderer, TTF_Font* font);
void draw_role_table(SDL_Renderer* renderer, TTF_Font* font);
void render_osg_buttons(SDL_Renderer* renderer, TTF_Font* font);

#endif
