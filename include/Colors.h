#include <SDL3/SDL.h>

class Colors {
public:
    static void White(SDL_Renderer* r) {
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    }

    static void Black(SDL_Renderer* r) {
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    }

    static void Red(SDL_Renderer* r) {
        SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
    }

    static void Green(SDL_Renderer* r) {
        SDL_SetRenderDrawColor(r, 0, 255, 0, 255);
    }

    static void Blue(SDL_Renderer* r) {
        SDL_SetRenderDrawColor(r, 0, 0, 255, 255);
    }

    static void Yellow(SDL_Renderer* r) {
        SDL_SetRenderDrawColor(r, 255, 255, 0, 255);
    }

    static void Cyan(SDL_Renderer* r) {
        SDL_SetRenderDrawColor(r, 0, 255, 255, 255);
    }

    static void Purple(SDL_Renderer* r) {
        SDL_SetRenderDrawColor(r, 180, 0, 255, 255);
    }

    static void Orange(SDL_Renderer* r) {
        SDL_SetRenderDrawColor(r, 255, 165, 0, 255);
    }

    static void Gray(SDL_Renderer* r) {
        SDL_SetRenderDrawColor(r, 128, 128, 128, 255);
    }

    

    static SDL_FColor WhiteF() {
        return {1.0f, 1.0f, 1.0f, 1.0f};
    }

    static SDL_FColor BlackF() {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }

    static SDL_FColor RedF() {
        return {1.0f, 0.0f, 0.0f, 1.0f};
    }

    static SDL_FColor GreenF() {
        return {0.0f, 1.0f, 0.0f, 1.0f};
    }

    static SDL_FColor BlueF() {
        return {0.0f, 0.0f, 1.0f, 1.0f};
    }

    static SDL_FColor YellowF() {
        return {1.0f, 1.0f, 0.0f, 1.0f};
    }

    static SDL_FColor CyanF() {
        return {0.0f, 1.0f, 1.0f, 1.0f};
    }

    static SDL_FColor PurpleF() {
        return {180.0f / 255.0f, 0.0f, 1.0f, 1.0f};
    }

    static SDL_FColor OrangeF() {
        return {1.0f, 165.0f / 255.0f, 0.0f, 1.0f};
    }

    static SDL_FColor GrayF() {
        return {128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 1.0f};
    }
};