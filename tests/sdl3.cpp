#include <SDL3/SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

struct AppContext {
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
};

void main_loop(void* arg) {
    AppContext* ctx = static_cast<AppContext*>(arg);
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            ctx->running = false;
        }
    }

    // Rendering logic
    SDL_SetRenderDrawColor(ctx->renderer, 33, 33, 33, 255);
    SDL_RenderClear(ctx->renderer);
    
    // Draw game objects here...

    SDL_RenderPresent(ctx->renderer);

    #ifdef __EMSCRIPTEN__
    if (!ctx->running) {
        emscripten_cancel_main_loop();
    }
    #endif
}

int main(int argc, char* argv[]) {
    AppContext ctx = {nullptr, nullptr, true};

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return -1;
    }

    ctx.window = SDL_CreateWindow("SDL3 WASM", 800, 600, 0);
    ctx.renderer = SDL_CreateRenderer(ctx.window, nullptr);

    #ifdef __EMSCRIPTEN__
    // 0 fps lets the browser handle frame timing via requestAnimationFrame
    emscripten_set_main_loop_arg(main_loop, &ctx, 0, 1);
    #else
    while (ctx.running) {
        main_loop(&ctx);
        SDL_Delay(16); // Primitive frame limiter for native testing
    }
    #endif

    SDL_DestroyRenderer(ctx.renderer);
    SDL_DestroyWindow(ctx.window);
    SDL_Quit();
    return 0;
}
