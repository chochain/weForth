#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <emscripten.h>
#include <iostream>

struct Context {
    std::string    title;
    int            w, h;
    SDL_Window     *window;
    SDL_Renderer   *rndr;
    SDL_Event      event;
    SDL_Rect       rect, rect2;
    SDL_Texture    *logo;
};

void callback(void * arg){
    Context *ctx  = static_cast<Context*>(arg);
    while(SDL_PollEvent(&ctx->event)) {
        switch (ctx->event.type) {
        case SDL_QUIT:            exit(0);            break;
        case SDL_MOUSEBUTTONDOWN: ctx->rect2.x -= 20; break;
        default: /* do nothing */                     break;
        }
    }
    
    SDL_Renderer *rn = ctx->rndr;
    SDL_RenderClear(rn);
    SDL_SetRenderDrawColor(rn, 0x40, 0x80, 0xc0, 0xa0);
    //SDL_RenderDrawRect(rn, &rect2);
    SDL_RenderFillRect(rn, &ctx->rect2);
    SDL_SetRenderDrawColor(rn, 0x8, 0x10, 0x20, 0x80);
    SDL_RenderCopy(rn, ctx->logo, NULL, &ctx->rect);
    SDL_RenderPresent(rn);
}

void setup(Context &ctx) {
    SDL_Init(SDL_INIT_EVERYTHING);
    
    ctx.title  = "SDL2 It Works!";
    ctx.w      = 640;
    ctx.h      = 480;
    ctx.window = SDL_CreateWindow(
        ctx.title.c_str(),
        50, 30, ctx.w, ctx.h,
        SDL_WINDOW_SHOWN
        );
    ctx.rndr = SDL_CreateRenderer(ctx.window, -1, 0);
}

void inline RECT(SDL_Rect &r, int x, int y, int w, int h) {
    r.x = x; r.y = y; r.w = w; r.h = h;
}

int play(Context &ctx) {
    SDL_Surface *image = IMG_Load("tests/assets/owl.png");
    if (!image) {
        printf("IMG_Load: %s\n", IMG_GetError());
        return 1;
    }
    ctx.logo = SDL_CreateTextureFromSurface(ctx.rndr, image);
    SDL_FreeSurface(image);

    RECT(ctx.rect,  260, 260, image->w, image->h);
    RECT(ctx.rect2, 400, 100, 200, 200);

    return 0;
}    

void teardown(Context &ctx) {
    SDL_DestroyTexture(ctx.logo);
    SDL_DestroyRenderer(ctx.rndr);
    SDL_DestroyWindow(ctx.window);
    SDL_Quit();
}

int main(int argc, char** argv) {
    Context ctx;
    
    setup(ctx);
    if (play(ctx)) return -1;

    emscripten_set_main_loop_arg(callback, &ctx, -1, 1);

    teardown(ctx);
  
    return 0;
}
