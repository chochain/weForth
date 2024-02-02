#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#define run_main_loop(cb,ctx) emscripten_set_main_loop_arg(cb,ctx,-1,1)
#else
#define run_main_loop(cb,ctx) while(g_run) { cb(ctx); }
#endif

bool g_run = true;

struct C4 : SDL_Color {
    C4(Uint8 r, Uint8 g, Uint8 b, Uint8 a) { set(r, g, b, a);         }
    C4(SDL_Color &c)                       { set(c.r, c.g, c.b, c.a); }
    void set(Uint8 r0, Uint8 g0, Uint8 b0, Uint8 a0) { r=r0; g=g0; b=b0; a=a0; }
};

struct Tile {
    SDL_Renderer *rndr;         // pointer to renderer
    SDL_Texture  *tex= NULL;    // Texture (stored in hardwared/GPU)
    C4           *c4 = NULL;    // set draw color if defined
    SDL_Rect     rect;          // rectangle to be drawn upon
    double       ang = 0.0;
    
    Tile(SDL_Renderer *rndr, int x, int y, int w=0, int h=0) : rndr(rndr) {
        rect.x = x; rect.y = y, rect.w = w; rect.h = h;
    }
    ~Tile() { if (c4) delete c4; }
    
    void free() { if (tex) SDL_DestroyTexture(tex); }  // run before destructor is called
    Tile *load(const char *fname, C4 *key=NULL) {
        SDL_Surface *img = IMG_Load(fname);
        if (!img) {
            printf("IMG_Load: %s\n", IMG_GetError());
            return NULL;
        }
        rect.w = img->w;                               // adjust image size
        rect.h = img->h;
        
        if (key) {
            C4 &k = *key;                              // use reference
            SDL_SetColorKey(img, SDL_TRUE,             // key color => transparent 
                SDL_MapRGB(img->format, k.r, k.g, k.b));
        }
        tex = SDL_CreateTextureFromSurface(rndr, img); // convert img to GPU texture
        SDL_FreeSurface(img);
        
        return this;
    }
    Tile *set_color(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
        if (c4) c4->set(r,g,b,a);
        else    c4 = new C4(r,g,b,a);
        
        return this;
    }
    Tile *render(SDL_Rect *clip=NULL) {
        if (c4) {                                             // set draw color if given
            SDL_SetRenderDrawColor(rndr, c4->r, c4->g, c4->b, c4->a);
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
    Text(SDL_Renderer *rndr, int x, int y, int w=0, int h=0) : Tile(rndr, x, y, w, h) {}
    
    Text *load(TTF_Font *font, const char *str, C4 *c4=NULL) {
        SDL_Color c = { 0, 0, 0, 0xff };
        if (c4) { c.r=c4->r; c.g=c4->g; c.b=c4->b; c.a=c4->a; }
        
        SDL_Surface *txt = TTF_RenderText_Solid(font, str, c);
        if (!txt) {
            printf("TTF_Load: %s\n", TTF_GetError());
            return NULL;
        }
        c4     = new C4(c);
        rect.w = txt->w;
        rect.h = txt->h;
//        SDL_QueryTexture(tex, NULL, NULL, &rect.w, &rect.h);
        
        tex = SDL_CreateTextureFromSurface(rndr, txt);
        if (!tex) {
            printf("SDL_Load: %s\n", SDL_GetError());
            return NULL;
        }
        SDL_FreeSurface(txt);

        return this;
    }
};

struct Context {
    std::string    title = "SDL2 works";
    int            x=50, y=30, w=640, h=480;
    Uint8          r = 0x80, a = 0x80;
    SDL_Window     *window;
    SDL_Renderer   *rndr;
    Tile           *img, *sq;
    Text           *txt;
    TTF_Font       *font = NULL;
};

void callback(void *arg) {
    Context   &ctx = *static_cast<Context*>(arg);
    SDL_Event ev;
    SDL_Rect  &img = ctx.img->rect;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:            exit(0);         break;
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
        Tile &t = *ctx.sq;                                // display square
        t.set_color(ctx.r, 0xf0, 0xc0, ctx.a);            // with changing color
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
    
    ctx.window = SDL_CreateWindow(
        ctx.title.c_str(),
        ctx.x, ctx.y, ctx.w, ctx.h,
        SDL_WINDOW_SHOWN
        );
    
    ctx.rndr = SDL_CreateRenderer(ctx.window, -1, 0);
    ctx.font = TTF_OpenFont("tests/assets/FreeSans.ttf", 48);
    if (!ctx.font) {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        return 1;
    }
    SDL_SetRenderDrawBlendMode(ctx.rndr, SDL_BLENDMODE_BLEND);  // for alpha blending

    return 0;
}

int play(Context &ctx, const char *title, const char *fname) {
    C4 key(0xff, 0xff, 0xff, 0xff);
    
    ctx.sq  = new Tile(ctx.rndr, 400, 100, 200, 200);
    ctx.img = new Tile(ctx.rndr, 160, 160);
    if (!ctx.img->load(fname, &key)) return 1;

    ctx.txt = new Text(ctx.rndr, 100, 100);
    if (!ctx.txt->load(ctx.font, title)) return 1;
    
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
    const char *title = "Hello Owl!";
    const char *fname = "tests/assets/owl.png";
    Context ctx;
    
    if (setup(ctx)) return -1;
    if (play(ctx, title, fname)) return -1;
    
    run_main_loop(callback, &ctx);
    
    teardown(ctx);
  
    return 0;
}
