
#ifndef GUI_H
#define GUI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h> 

extern int gClientPort;             // 用户输入的端口
extern SDL_Texture* cards[13];      // 卡牌图像指针数组

void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color);
void run_gui();
void draw_selection_popup(SDL_Renderer* renderer, TTF_Font* font);
void draw_guess_popup(SDL_Renderer* renderer, TTF_Font* font);
void show_end_popup(SDL_Renderer* renderer, TTF_Font* font, const char* result);
void setShowEndDialog(int value);
void render_cards(SDL_Renderer* renderer, SDL_Texture* cards[], int* mycards, int cardCount);
void send_action_request(char type, int objet, int cible);
void render_cards(SDL_Renderer* renderer, SDL_Texture* cards[], int* mycards, int cardCount);
#endif
