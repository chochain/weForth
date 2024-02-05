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
bool     g_run    = true;
TTF_Font *g_font  = NULL;
///
///> Rectangle textured tile class (for image or solid color)
///
struct Tile {
    SDL_Renderer *rn;            // pointer to renderer
    SDL_Rect     vp;             // rectangle view port to be drawn upon
    SDL_Color    c4;             // set draw color if defined
    bool         c4_set = false; // color set
    double       ang = 0.0;
    
    Tile(SDL_Renderer *r, SDL_Rect v) : rn(r), vp(v) {}
    Tile *set_color(SDL_Color c) {
        c4     = c;
        c4_set = true;
        return this;
    }
    virtual void free() {}
    virtual int  render(SDL_Rect *clip=NULL) {
        if (c4_set) {
            SDL_SetRenderDrawColor(rn, c4.r, c4.g, c4.b, c4.a);
        }
        SDL_RenderFillRect(rn, &vp);    // fill rectangle
        return 0;
    }
};

struct Image : Tile {
    SDL_Texture *tex = NULL;            // texture stored in hardward/GPU
    
    Image(SDL_Renderer *r, SDL_Rect v) : Tile(r, v) {}
    
    int replace_tex_then_free(SDL_Surface *img) {      // boiler plate
        free();
        tex = SDL_CreateTextureFromSurface(rn, img);   // convert img to GPU texture
        SDL_FreeSurface(img);
        CHK(!tex, SDL);
        return 0;
    }
    void free() { if (tex) SDL_DestroyTexture(tex); }
    int  load(const char *fname=NULL, SDL_Color *key=NULL) {
        if (!fname) { printf("IMG_Load: filename not given\n"); return 1; }
        
        SDL_Surface *img = IMG_Load(fname);
        CHK(!img, IMG);
        
        vp.w = img->w;                                 // adjust image size
        vp.h = img->h;
        printf("image %s[%d,%d] loaded ok\n", fname, vp.w, vp.h);
        
        if (key) {
            SDL_Color &k = *key;                       // use reference
            SDL_SetColorKey(img, SDL_TRUE,             // key color => transparent 
                SDL_MapRGB(img->format, k.r, k.g, k.b));
        }
        return replace_tex_then_free(img);
    }
    virtual int render(SDL_Rect *clip=NULL) override {
        if (c4_set) {
            SDL_SetRenderDrawColor(rn, c4.r, c4.g, c4.b, c4.a);
        }
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);    // can use other blending mode
//          SDL_RenderCopy(rndr, tex, NULL, &rect);           // entire texture
//          SDL_RenderCopy(rndr, tex, NULL, NULL);            // stretch to entire viewport
        SDL_RenderCopyEx(rn, tex, NULL, &vp,
                         ang, NULL /* center */, SDL_FLIP_NONE);
        return 0;
    }
};
///
///> Canvas class using locked Surface
///
struct Canvas : Image {
    Canvas(SDL_Renderer *r, SDL_Rect v) : Image(r, v) {}

    int render(SDL_Rect *clip=NULL) override {
        SDL_Surface *rgb =
            SDL_CreateRGBSurface(0, vp.w, vp.h, 32, 0, 0, 0, 0);
        CHK(!rgb, SDL);
        
        if (SDL_MUSTLOCK(rgb)) SDL_LockSurface(rgb);
        Uint32 *px = (Uint32*)rgb->pixels;
        for (int y=0; y < vp.h; y++) {
            for (int x=0; x < vp.w; x++) {
                *px++ = y | (x << 8);
            }
        }
        if (SDL_MUSTLOCK(rgb)) SDL_UnlockSurface(rgb);

        return replace_tex_then_free(rgb) || Image::render();
    }
};
///
///> Text string tile
///
struct Text : Image {
    const char        *header;
    std::stringstream stime;
    
    Text(SDL_Renderer *r, SDL_Rect v) : Image(r, v) {
        SDL_Color black = { 0, 0, 0, 0xff };
        set_color(black);                                    // default to black
    }
    int load(const char *str, SDL_Color *c=NULL) {           // text string with color
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
        
        vp.w = txt->w;
        vp.h = txt->h;

        return replace_tex_then_free(txt) || Image::render();
    }
};
///
/// Canvas class using locked Texture (no texture replacement, parallel to Image)
/// Note:
///   No surface creation or texture replace => but not faster 
///
struct Canvas1 : Tile {
    SDL_Texture *tex = NULL;           // texture pointer
    
    Canvas1(SDL_Renderer *r, SDL_Rect v) : Tile(r, v) {}

    void free() override { if (tex) SDL_DestroyTexture(tex); }
    int  load(const char *dummy=NULL, SDL_Color *key=NULL) {
        free();
        tex = SDL_CreateTexture(rn,
              SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
            vp.w, vp.h);
        CHK(!tex, SDL);
        
        return 0;
    }
    virtual int render(SDL_Rect *clip=NULL) override {
        static Uint8  c = 0, r = 0;
        Uint32 *buf;
        int    sz;
        if (++c==0) r++;
        CHK(SDL_LockTexture(tex, clip, (void**)&buf, &sz), SDL);  // get a pointer to GPU mem
        for (int y=0; y < vp.h; y++) {
            for (int x=0; x < vp.w; x++) {
                *buf++ = (x<<16) | (y<<8) | r;   // AGBR
            }
        }
        SDL_UnlockTexture(tex);                                   // update GPU mem
        SDL_RenderCopyEx(rn, tex, NULL, &vp,
                         ang, NULL /* center */, SDL_FLIP_NONE);
        return 0;
    }
};
///====================================================================================
///
///> Context class
///
struct Context {
    SDL_Window   *win;
    SDL_Renderer *rn;
    SDL_Rect     vp;
    SDL_Color    bg;
    
    Uint8        r = 0x80, a = 0x80;         // default colors
    std::vector<Tile*> que;                  // polymorphic pointers
    Tile         *img, *sq;                  // specialized

    void init(SDL_Window *w, SDL_Renderer *r, SDL_Color *c=NULL) {
        SDL_Color white = { 0xff, 0xff, 0xff, 0xff };
        win = w; rn = r; bg = c ? *c : white;
        
        SDL_SetRenderDrawBlendMode(rn, SDL_BLENDMODE_BLEND);  // for alpha blending
    }
    Tile *add(Tile *t) { que.push_back(t); return t; }
    void render() {
        SDL_SetRenderDrawColor(rn, bg.r, bg.g, bg.b, bg.a);   // shade the background
        SDL_RenderClear(rn);
        for (int i=0; i < que.size(); i++) {                  // render all tiles
            que[i]->render();
        }
        SDL_RenderPresent(rn);                                // update screen
    }
    void free() {
        SDL_DestroyRenderer(rn);
        for (int i=0; i < que.size(); i++) {
            que[i]->free();
        }
        que.clear();
        SDL_DestroyWindow(win);
    }
};
///
///> global main loop callback handler
///
void run(void *arg) {
    Context   &ctx = *static_cast<Context*>(arg);
    SDL_Event ev;
    SDL_Rect  &img = ctx.img->vp;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT: exit(0); /* force exit */   break;
        case SDL_MOUSEBUTTONDOWN: img.w <<= 1;     break;
        case SDL_MOUSEBUTTONUP:   img.w >>= 1;     break;
        case SDL_KEYDOWN: {
            SDL_Rect &sq = ctx.sq->vp;
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
    ///
    /// display all tiles in vector
    ///
    ctx.render();
}
///
///> SDL setup
///
int setup(Context *ctx, const char *title, SDL_Rect *vp, SDL_Color *bg) {
    CHK(IMG_Init(IMG_INIT_PNG)==-1, IMG);
    CHK(TTF_Init()==-1, TTF);
    
    g_font = TTF_OpenFont("tests/assets/FreeSans.ttf", 48);   // global
    CHK(!g_font, TTF);

    SDL_Window *win = SDL_CreateWindow(
        title, vp->x, vp->y, vp->w, vp->h,
        SDL_WINDOW_SHOWN
        );
    SDL_Renderer *rndr = SDL_CreateRenderer(win, -1, 0);
    
    ctx->init(win, rndr, bg);

    return 0;
}
///
///> SDL core
///
int test_sdl2(Context *ctx, const char *text, const char *fname) {
    SDL_Color key   = { 0xff, 0xff, 0xff, 0xff };       // key on white (as transparent)
    SDL_Color red   = { 0xff, 0x0,  0x0,  0xff };
    SDL_Rect  sq_v  = { 400, 80,  200, 200 };
    SDL_Rect  img_v = { 160, 160, 0,   0   };
    SDL_Rect  txt_v = { 60,  120, 0,   0   };
    SDL_Rect  cnv_v = { 240, 100, 256, 256 };

    SDL_Renderer *rn  = ctx->rn;                  // short hand

    Tile  *sq  = new Tile(rn, sq_v);              // initialize square
    Image *img = new Image(rn, img_v);            // initialize image
    if (img->load(fname, &key)) return 1;

    Text *txt = new Text(rn, txt_v);              // initialize text
    if (txt->load(text, &red)) return 1;          // text default background transparent
    
    Canvas1 *cnv = new Canvas1(rn, cnv_v);
    if (cnv->load()) return 1;

    ctx->sq  = sq;                                // keep individual pointers
    ctx->img = img;
    ctx->add(cnv);                                // keep Tiles in a vector
    ctx->add(img);
    ctx->add(txt);
    ctx->add(sq);

    printf("ctx.que with %ld tiles\n", ctx->que.size());

    return 0;
}    
///
/// SDL teardown (not called by WASM)
///
void teardown(Context *ctx) {
#ifndef EMSCRIPTEN
    printf("SDL shutting down...\n");
    ctx->free();
    printf("%s done.\n", __FILE__);
#endif
}

int main(int argc, char** argv) {
    const char *title = "SDL2 works";
    const char *text  = "Hello Owl!";
    const char *fname = "tests/assets/owl.png";
    SDL_Rect    wsize = { 50, 30, 640, 480 };
    SDL_Color   bg    = { 0xf0, 0xff, 0xe0, 0x80 };
    Context     ctx;

    SDL_Init(SDL_INIT_VIDEO);

    if (setup(&ctx, title, &wsize, &bg)) return -1;
    if (test_sdl2(&ctx, text, fname)) return -1;
    
#ifdef EMSCRIPTEN
    emscripten_set_main_loop_arg(run, &ctx, -1, 1);  // -1:fps=RAF, 1=infinite loop
    printf("emscripten force exit...\n");            // should neve come here
    emscripten_force_exit(-1);
#else
    while (g_run) { run(&ctx); }
#endif 
    
    teardown(&ctx);
    SDL_Quit();
  
    return 0;
}
