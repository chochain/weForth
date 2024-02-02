#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <sstream>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#define run_main_loop(cb,ctx) emscripten_set_main_loop_arg(cb,ctx,-1,1)
#else
#define run_main_loop(cb,ctx) while(g_run) { cb(ctx); }
#endif

bool     g_run   = true;
TTF_Font *g_font = NULL;

struct Tile {
    SDL_Renderer *rndr;          // pointer to renderer
    SDL_Texture  *tex= NULL;     // Texture (stored in hardwared/GPU)
    SDL_Color    c4;             // set draw color if defined
    bool         c4_set = false; // color set
    SDL_Rect     rect;           // rectangle to be drawn upon
    double       ang = 0.0;
    
    Tile(SDL_Renderer *rndr, int x, int y, int w=0, int h=0) : rndr(rndr) {
        rect.x = x; rect.y = y, rect.w = w; rect.h = h;
    }
    
    void free() { if (tex) SDL_DestroyTexture(tex); }  // run before destructor is called
    Tile *load(const char *fname, SDL_Color *key=NULL) {
        SDL_Surface *img = IMG_Load(fname);
        if (!img) {
            printf("IMG_Load: %s\n", IMG_GetError());
            return NULL;
        }
        rect.w = img->w;                               // adjust image size
        rect.h = img->h;
        
        if (key) {
            SDL_Color &k = *key;                       // use reference
            SDL_SetColorKey(img, SDL_TRUE,             // key color => transparent 
                SDL_MapRGB(img->format, k.r, k.g, k.b));
        }
        tex = SDL_CreateTextureFromSurface(rndr, img); // convert img to GPU texture
        SDL_FreeSurface(img);
        
        return this;
    }
    Tile *set_color(SDL_Color c) {
        c4     = c;
        c4_set = true;
        return this;
    }
    Tile *render(SDL_Rect *clip=NULL) {
        if (c4_set) {
            SDL_SetRenderDrawColor(rndr, c4.r, c4.g, c4.b, c4.a);
        }
        if (tex) {                                            // display texture
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);// can use other blending mode
//          SDL_RenderCopy(rndr, tex, NULL, &rect);           // entire texture
//          SDL_RenderCopy(rndr, tex, NULL, NULL);            // stretch to entire viewport
            SDL_RenderCopyEx(rndr, tex, NULL, &rect,
                             ang, NULL /* center */, SDL_FLIP_NONE);
        }
        else SDL_RenderFillRect(rndr, &rect);                 // fill rectangle
        
        return this;
    }
};

struct Text : Tile {
    const char        *header;
    Uint32            t0;
    std::stringstream stime;
    
    Text(SDL_Renderer *rndr, int x, int y, int w=0, int h=0) : Tile(rndr, x, y, w, h) {
        SDL_Color black = {0,0,0,0xff};
        set_color(black);                                    // default to black
    }
    Text *load(const char *str, SDL_Color *c=NULL) {         // text string with color
        header = str;
        if (c) set_color(*c);                                // if color given
        return this;
    }
    Text *render(SDL_Rect *clip=NULL) {
        Uint32 t = SDL_GetTicks();
        stime.str("");
        stime << header << " : " << (t - t0) / 1000;         // timer ticks (in second)

        SDL_Surface *txt = TTF_RenderText_Solid(             // create surface (real-time text)
            g_font, stime.str().c_str(), c4);
        if (!txt) {
            printf("TTF_Load: %s\n", TTF_GetError());
            return NULL;
        }
        rect.w = txt->w;
        rect.h = txt->h;
        
        free();                                              // release previous allocated tex
        tex = SDL_CreateTextureFromSurface(rndr, txt);       // build new texture
        SDL_FreeSurface(txt);                                // release surface object
        
        if (!tex) {
            printf("SDL_Load: %s\n", SDL_GetError());
            return NULL;
        }
        Tile::render();
        
        return this;
    }
};

struct Context {
    const char   *title = "SDL2 works";
    int          x=50, y=30, w=640, h=480;   // default size
    Uint8        r = 0x80, a = 0x80;         // default colors
    Uint32       t0, t1;
    SDL_Window   *window;
    SDL_Renderer *rndr;
    Tile         *img, *sq;
    Text         *txt;
};

void callback(void *arg) {
    Context   &ctx = *static_cast<Context*>(arg);
    SDL_Event ev;
    SDL_Rect  &img = ctx.img->rect;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT: exit(0); /* force exit */   break;
        case SDL_MOUSEBUTTONDOWN: img.w <<= 1;     break;
        case SDL_MOUSEBUTTONUP:   img.w >>= 1;     break;
        case SDL_KEYDOWN: {
            SDL_Rect &sq = ctx.sq->rect;
            switch(ev.key.keysym.sym) {
            case SDLK_UP:    sq.y -= 20;           break;
            case SDLK_DOWN:  sq.y += 20;           break;
            case SDLK_LEFT:  sq.x -= 20;           break;
            case SDLK_RIGHT: sq.x += 20;           break;
            case SDLK_a:     ctx.img->ang -= 30.0; break;
            case SDLK_f:     ctx.img->ang += 30.0; break;
            case SDLK_e:     ctx.a -= 0x20;        break;
            case SDLK_x:     ctx.a += 0x20;        break;
            case SDLK_s:     ctx.r -= 0x20;        break;
            case SDLK_d:     ctx.r += 0x20;        break;
            case SDLK_q:     g_run = false;        break;
            default: /* do nothing */ break;
            }
        } break;
        default: /* do nothing */ break;
        }
    }

    SDL_Renderer *rn = ctx.rndr;
    SDL_SetRenderDrawColor(rn, 0xf0, 0xff, 0xe0, 0x80);   // shade the background
    SDL_RenderClear(rn);
    {
        ctx.img->render();                                // display image
        ctx.txt->render();                                // display text
        Tile      &t = *ctx.sq;                           // display square
        SDL_Color c  = {ctx.r, 0xf0, 0xc0, ctx.a};        // update color
        t.set_color(c);                                   // with changing color
        t.render();
    }
    SDL_RenderPresent(rn);                                // update screen
}

int setup(Context &ctx) {
    SDL_Init(SDL_INIT_VIDEO);
    if (IMG_Init(IMG_INIT_PNG)==-1) {
        printf("IMG_Init: %s\n", IMG_GetError()); return 1;
    }
    if (TTF_Init()==-1) {
        printf("TTF_Init: %s\n", TTF_GetError()); return 1;
    }
    g_font = TTF_OpenFont("tests/assets/FreeSans.ttf", 48);
    if (!g_font) {
        printf("TTF_OpenFont: %s\n", TTF_GetError()); return 1;
    }
    
    ctx.window = SDL_CreateWindow(
        ctx.title, ctx.x, ctx.y, ctx.w, ctx.h,
        SDL_WINDOW_SHOWN
        );
    
    ctx.rndr = SDL_CreateRenderer(ctx.window, -1, 0);
    SDL_SetRenderDrawBlendMode(ctx.rndr, SDL_BLENDMODE_BLEND);  // for alpha blending

    ctx.t0 = SDL_GetTicks();       // get start up time

    return 0;
}

int play(Context &ctx, const char *text, const char *fname) {
    SDL_Color key = {0xff, 0xff, 0xff, 0xff};          // key on white (as transparent)
    SDL_Color red = {0xff, 0x0,  0x0,  0xff};
    
    ctx.sq  = new Tile(ctx.rndr, 400, 100, 200, 200);  // initialize square
    ctx.img = new Tile(ctx.rndr, 160, 160);            // initialize image
    if (!ctx.img->load(fname, &key)) return 1;

    ctx.txt = new Text(ctx.rndr, 100, 100);            // initialize text
    if (!ctx.txt->load(text, &red)) return 1;
    
    return 0;
}    

void teardown(Context &ctx) {
#ifndef EMSCRIPTEN
    printf("SDL shutting down...\n");
    SDL_DestroyRenderer(ctx.rndr);
    ctx.txt->free();
    ctx.img->free();
    SDL_DestroyWindow(ctx.window);
    SDL_Quit();
    printf("%s done.\n", __FILE__);
#endif
}

int main(int argc, char** argv) {
    const char *text  = "Hello Owl!";
    const char *fname = "tests/assets/owl.png";
    Context ctx;
    
    if (setup(ctx)) return -1;
    if (play(ctx, text, fname)) return -1;
    
    run_main_loop(callback, &ctx);
    
    teardown(ctx);
  
    return 0;
}
