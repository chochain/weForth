#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <sstream>
#include <vector>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#define CHK(t, mod) if (t) {                                \
        printf("%s Error: %s\n", #mod, mod ## _GetError()); \
        return 1;                                           \
    }
///
///> global control variables
///
bool     g_run   = true;
TTF_Font *g_font = NULL;
///
///> Rectangle textured tile class (for image or solid color)
///
struct Tile {
    SDL_Renderer *rndr;          // pointer to renderer
    SDL_Color    c4;             // set draw color if defined
    bool         c4_set = false; // color set
    SDL_Rect     rect;           // rectangle to be drawn upon
    double       ang = 0.0;
    
    Tile(SDL_Renderer *rndr, int x, int y, int w, int h) : rndr(rndr) {
        rect.x = x; rect.y = y, rect.w = w; rect.h = h;
    }
    Tile *set_color(SDL_Color c) {
        c4     = c;
        c4_set = true;
        return this;
    }
    virtual void free() {}
    virtual int  load(const char *str=NULL, SDL_Color *c=NULL) { return 0; }
    virtual int  render(SDL_Rect *clip=NULL) {
        if (c4_set) {
            SDL_SetRenderDrawColor(rndr, c4.r, c4.g, c4.b, c4.a);
        }
        SDL_RenderFillRect(rndr, &rect);    // fill rectangle
        return 0;
    }
};

struct Image : Tile {
    SDL_Texture *tex= NULL;       // Texture stored in hardwared/GPU

    Image(SDL_Renderer *rndr, int x, int y, int w=0, int h=0) : Tile(rndr, x, y, w, h) {}
    void free() override { if (tex) SDL_DestroyTexture(tex); }  // run before destructor is called

    int replace_tex_then_free(SDL_Surface *img) {      // boiler plate
        free();
        tex = SDL_CreateTextureFromSurface(rndr, img); // convert img to GPU texture
        SDL_FreeSurface(img);
        CHK(!tex, SDL);
        return 0;
    }
    virtual int load(const char *fname=NULL, SDL_Color *key=NULL) override {
        if (!fname) { printf("IMG_Load: filename not given\n"); return 1; }
        
        SDL_Surface *img = IMG_Load(fname);
        CHK(!img, IMG);
        
        rect.w = img->w;                               // adjust image size
        rect.h = img->h;
        
        if (key) {
            SDL_Color &k = *key;                       // use reference
            SDL_SetColorKey(img, SDL_TRUE,             // key color => transparent 
                SDL_MapRGB(img->format, k.r, k.g, k.b));
        }
        return replace_tex_then_free(img);
    }
    virtual int render(SDL_Rect *clip=NULL) override {
        if (c4_set) {
            SDL_SetRenderDrawColor(rndr, c4.r, c4.g, c4.b, c4.a);
        }
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);    // can use other blending mode
//          SDL_RenderCopy(rndr, tex, NULL, &rect);           // entire texture
//          SDL_RenderCopy(rndr, tex, NULL, NULL);            // stretch to entire viewport
        SDL_RenderCopyEx(rndr, tex, NULL, &rect,
                         ang, NULL /* center */, SDL_FLIP_NONE);
        return 0;
    }
};
///
///> Canvas class (to be drawn on)
///
struct Canvas : Image {
    Canvas(SDL_Renderer *rndr, int x, int y, int w, int h) : Image(rndr, x, y, w, h) {}

    virtual int  load(const char *dummy=NULL, SDL_Color *key=NULL) override {
        SDL_Surface *rgb =
            SDL_CreateRGBSurface(0, rect.w, rect.h, 32, 0, 0, 0, 0);
        CHK(!rgb, SDL);
        
        if (SDL_MUSTLOCK(rgb)) SDL_LockSurface(rgb);
        Uint32 *px = (Uint32*)rgb->pixels;
        for (int y=0; y < rect.h; y++) {
            for (int x=0; x < rect.w; x++) {
                *px++ = y | (x << 8);
            }
        }
        if (SDL_MUSTLOCK(rgb)) SDL_UnlockSurface(rgb);

        return replace_tex_then_free(rgb);
    }
};
///
///> Text string tile
///
struct Text : Image {
    const char        *header;
    std::stringstream stime;
    
    Text(SDL_Renderer *rndr, int x, int y, int w=0, int h=0) : Image(rndr, x, y, w, h) {
        SDL_Color black = {0,0,0,0xff};
        set_color(black);                                    // default to black
    }
    int load(const char *str, SDL_Color *c=NULL) override {  // text string with color
        header = str;
        if (c) set_color(*c);                                // if color given
        return 0;
    }
    int render(SDL_Rect *clip=NULL) override {
        static Uint32 t0  = SDL_GetTicks();
        static Uint32 dt0 = 0, cnt = 0, fps = 0;
        
        Uint32 t  = SDL_GetTicks();
        Uint32 dt = (t - t0) / 1000;
        
        if (dt <= dt0) cnt++;
        else { fps = cnt; dt0 = dt; cnt=0; }
        
        stime.str("");
        stime << header << " : " << dt << ", fps=" << fps;   // timer ticks (in second)
        
        SDL_Surface *txt = TTF_RenderText_Solid(             // create surface (real-time text)
            g_font, stime.str().c_str(), c4);
        CHK(!txt, TTF);
        
        rect.w = txt->w;
        rect.h = txt->h;

        return replace_tex_then_free(txt) || Image::render();
    }
};
///====================================================================================
///
///> Context class
///
struct Context {
    SDL_Window   *win;
    SDL_Renderer *rndr;
    SDL_Rect     vport;
    SDL_Color    bg;
    
    Uint8        r = 0x80, a = 0x80;         // default colors
    std::vector<Tile*> tile;                 // polymorphic pointers
    Tile         *img, *sq;                  // specialized

    void init(SDL_Window *wn, SDL_Renderer *rn, SDL_Color *c=NULL) {
        SDL_Color white = { 0xff, 0xff, 0xff, 0xff };
        win = wn; rndr = rn; bg = c ? *c : white;
        
        SDL_SetRenderDrawBlendMode(rndr, SDL_BLENDMODE_BLEND);  // for alpha blending
    }
    void render() {
        SDL_SetRenderDrawColor(rndr, bg.r, bg.g, bg.b, bg.a);   // shade the background
        SDL_RenderClear(rndr);
        for (int i=0; i < tile.size(); i++) {
            tile[i]->render();
        }
        SDL_RenderPresent(rndr);                                // update screen
    }
    void free() {
        SDL_DestroyRenderer(rndr);
        for (int i=0; i < tile.size(); i++) {
            tile[i]->free();
        }
        tile.clear();
        SDL_DestroyWindow(win);
    }
};
///
///> global main loop callback handler
///
void run(void *arg) {
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
    SDL_Color c = {ctx.r, 0xf0, 0xc0, ctx.a};   // update color
    ctx.sq->set_color(c);                       // with changing color
    
    ctx.render();                               // shade the background
}
///
///> SDL setup
///
int setup(Context &ctx, const char *title, SDL_Rect vp) {
    CHK(IMG_Init(IMG_INIT_PNG)==-1, IMG);
    CHK(TTF_Init()==-1, TTF);
    
    g_font = TTF_OpenFont("tests/assets/FreeSans.ttf", 48);
    CHK(!g_font, TTF);

    SDL_Window *win = SDL_CreateWindow(
        title, vp.x, vp.y, vp.w, vp.h,
        SDL_WINDOW_SHOWN
        );
    SDL_Renderer *rndr = SDL_CreateRenderer(win, -1, 0);
    SDL_Color    bg    = { 0xf0, 0xff, 0xe0, 0x80 };
    
    ctx.init(win, rndr, &bg);

    return 0;
}
///
///> SDL core
///
int test_sdl2(Context &ctx, const char *text, const char *fname) {
    SDL_Color key = {0xff, 0xff, 0xff, 0xff};          // key on white (as transparent)
    SDL_Color red = {0xff, 0x0,  0x0,  0xff};

    ctx.sq  = new Tile(ctx.rndr, 400, 80, 200, 200);   // initialize square
    ctx.img = new Image(ctx.rndr, 160, 160);           // initialize image
    if (ctx.img->load(fname, &key)) return 1;

    Tile *txt = new Text(ctx.rndr, 60, 120);           // initialize text
    if (txt->load(text, &red)) return 1;               // text default background transparent
    
    Tile *cnv = new Canvas(ctx.rndr, 240, 100, 256, 256);
    if (cnv->load()) return 1;

    ctx.tile.push_back(cnv);
    ctx.tile.push_back(ctx.img);
    ctx.tile.push_back(txt);
    ctx.tile.push_back(ctx.sq);

    return 0;
}    
///
/// SDL teardown (not called by WASM)
///
void teardown(Context &ctx) {
#ifndef EMSCRIPTEN
    printf("SDL shutting down...\n");
    ctx.free();
    printf("%s done.\n", __FILE__);
#endif
}

int main(int argc, char** argv) {
    const char *title = "SDL2 works";
    const char *text  = "Hello Owl!";
    const char *fname = "tests/assets/owl.png";
    SDL_Rect    vport = {50, 30, 640, 480};
    Context ctx;

    SDL_Init(SDL_INIT_VIDEO);
    if (setup(ctx, title, vport)) return -1;
    
    if (test_sdl2(ctx, text, fname)) return -1;
    
#ifdef EMSCRIPTEN
    emscripten_set_main_loop_arg(run, &ctx, -1, 1);
#else
    while (g_run) { run(&ctx); }
#endif 
    
    teardown(ctx);
    SDL_Quit();
  
    return 0;
}
