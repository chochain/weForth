#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

SDL_Window *win;
SDL_Renderer *rndr;
SDL_Rect box = { 200, 200, 100, 100 };

void redraw() {
    SDL_SetRenderDrawColor(rndr, 0xe0, 0xe0, 0xff, 0xff);
    SDL_RenderClear(rndr);
    
    lineColor(rndr, 10, 10, 390, 390, 0xffff00ff);         // AGBR
    lineRGBA(rndr,  20, 10, 380, 390, 0xff, 0x0, 0, 0xff);
    
    thickLineColor(rndr, 100, 300, 300, 200, 20, 0x8000ffff);
    thickLineRGBA(rndr,  200, 300, 300, 100, 20, 0xff, 0x00, 0xff, 0x80);
    
    boxColor(rndr, 0,   0, 100, 100, 0x800000ff);
    boxRGBA(rndr,  100, 0, 200, 100, 0, 0, 0xff, 0x80);
    
    // check that the x2 > x1 case is handled correctly
    boxColor(rndr, 200, 100, 100, 200, 0x8000ff00);
    boxColor(rndr, 0,   200, 100, 100, 0x8000ffff);
    
    rectangleColor(rndr,  250, 20,  350, 80,  0xff00ff00);
    rectangleRGBA(rndr,   250, 120, 350, 180, 0xff, 0, 0, 0x80);
    rectangleRGBA(rndr,   280, 10,  320, 190, 0x0, 0, 0xff, 0x80);
    
    filledEllipseRGBA(rndr,      200, 200, 50, 100, 0xff, 0x00, 0x00, 0x80);
    ellipseRGBA(rndr,            200, 200, 50, 100, 0xff, 0xff, 0x00, 0x80);
    roundedRectangleColor(rndr,  150, 100, 250, 300, 20, 0x8000ff00);
    
    filledEllipseRGBA(rndr,      260, 260, 100, 50, 0, 0, 0xff, 0x80);
    roundedRectangleRGBA(rndr,   160, 210, 360, 310, 20, 0xff, 0, 0, 0x80);
    
    SDL_RenderPresent(rndr);
}

bool handle_events() {
    SDL_Event event;
    SDL_PollEvent(&event);
    if (event.type == SDL_QUIT) return false;
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_UP:    box.w += 1; break;
        case SDLK_DOWN:  box.w -= 1; break;
        case SDLK_RIGHT: box.h += 1; break;
        case SDLK_LEFT:  box.h -= 1; break;
        case SDLK_q:     return false;
        }
        redraw();
    }
    return true;
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(400, 400, 0, &win, &rndr);

    redraw();
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop([]() { handle_events(); }, 0, true);
#else
    while (handle_events());
#endif

    SDL_DestroyRenderer(rndr);
    SDL_DestroyWindow(win);

    SDL_Quit();
}
