
#include "resources.h"
#include <SDL_image.h>
#include <stdio.h>

SDL_Texture* icons[8];
SDL_Texture* cards[13];

void load_icons(SDL_Renderer* renderer) {
    const char* iconFiles[8] = {
        "img/SH13_pipe_120x120.png",
        "img/SH13_ampoule_120x120.png",
        "img/SH13_poing_120x120.png",
        "img/SH13_couronne_120x120.png",
        "img/SH13_carnet_120x120.png",
        "img/SH13_collier_120x120.png",
        "img/SH13_oeil_120x120.png",
        "img/SH13_crane_120x120.png"
    };

    for (int i = 0; i < 8; ++i) {
        SDL_Surface* temp = IMG_Load(iconFiles[i]);
        if (!temp) {
            fprintf(stderr, "Erreur de chargement (icon): %s\n", iconFiles[i]);
            icons[i] = NULL;
            continue;
        }
        icons[i] = SDL_CreateTextureFromSurface(renderer, temp);
        SDL_FreeSurface(temp);
    }
}

void load_cards(SDL_Renderer* renderer) {
    for (int i = 0; i < 13; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "img/SH13_%d.png", i);
        SDL_Surface* temp = IMG_Load(path);
        if (!temp) {
            fprintf(stderr, "Erreur de chargement (carte): %s\n", path);
            cards[i] = NULL;
            continue;
        }
        cards[i] = SDL_CreateTextureFromSurface(renderer, temp);
        SDL_FreeSurface(temp);
    }
}

void load_all_textures(SDL_Renderer* renderer) {
    load_icons(renderer);
    load_cards(renderer);
}

void unload_textures() {
    for (int i = 0; i < 8; ++i) {
        if (icons[i]) {
            SDL_DestroyTexture(icons[i]);
            icons[i] = NULL;
        }
    }
    for (int i = 0; i < 13; ++i) {
        if (cards[i]) {
            SDL_DestroyTexture(cards[i]);
            cards[i] = NULL;
        }
    }
}
