
#ifndef RESOURCES_H
#define RESOURCES_H

#include <SDL.h>

extern SDL_Texture* icons[8];
extern SDL_Texture* cards[13];

void load_icons(SDL_Renderer* renderer);
void load_cards(SDL_Renderer* renderer);
void load_all_textures(SDL_Renderer* renderer);
void unload_textures();

#endif
