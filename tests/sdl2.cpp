#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <emscripten.h>

struct RGBA {
    Uint8 r, g, b, a;
    RGBA() {}
    RGBA(RGBA &c) : r(c.r), g(c.g), b(c.b), a(c.a) {}
    RGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) : r(r), g(g), b(b), a(a) {}
    void set(Uint8 r0, Uint8 g0, Uint8 b0, Uint8 a0) { r=r0; g=g0; b=b0; a=a0; }
};

struct Tile {
    SDL_Renderer *rndr;         // pointer to renderer
    SDL_Texture  *tex  = NULL;  // Texture (stored in hardwared/GPU)
    RGBA         *rgba = NULL;  // set draw color if defined
    SDL_Rect     rect;          // rectangle to be drawn upon
    double       ang = 0.0;
    
    Tile(SDL_Renderer *rndr, int x, int y, int w=0, int h=0) : rndr(rndr) {
        rect.x = x; rect.y = y, rect.w = w; rect.h = h;
    }
    ~Tile() { if (rgba) delete rgba; }
    
    void free() { if (tex) SDL_DestroyTexture(tex); }  // run before destructor is called
    Tile *load(const char *fname, RGBA *key=NULL) {
        SDL_Surface *img = IMG_Load(fname);
        if (!img) return NULL;
        
        rect.w = img->w;                               // adjust image size
        rect.h = img->h;
        
        if (key) {
            RGBA &k = *key;                            // use reference
            SDL_SetColorKey(img, SDL_TRUE,             // key color => transparent 
                SDL_MapRGB(img->format, k.r, k.g, k.b));
        }
        tex = SDL_CreateTextureFromSurface(rndr, img); // convert img to GPU texture
        SDL_FreeSurface(img);
        
        return this;
    }
    Tile *set_color(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
        if (rgba) rgba->set(r,g,b,a);
        else      rgba = new RGBA(r,g,b,a);
        
        return this;
    }
    Tile *render(SDL_Rect *clip=NULL) {
        if (rgba) {
            RGBA &c = *rgba;                                  // use reference
            SDL_SetRenderDrawColor(rndr, c.r, c.g, c.b, c.a); // draw blue rectangle
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

struct Context {
    std::string    title = "SDL2 works";
    int            x=50, y=30, w=640, h=480;
    Uint8          r = 0x80, a = 0x80;
    SDL_Window     *window;
    SDL_Renderer   *rndr;
    Tile           *img, *sq;
};

void callback(void *arg){
    Context   &ctx = *static_cast<Context*>(arg);
    SDL_Event ev;
    SDL_Rect  &img = ctx.img->rect;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:            exit(0);     break;
        case SDL_MOUSEBUTTONDOWN: img.w <<= 1; break;
        case SDL_MOUSEBUTTONUP:   img.w >>= 1; break;
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
        Tile &t = *ctx.sq;                                // display square
        t.set_color(ctx.r, 0xf0, 0xc0, ctx.a);            // with changing color
        t.render();
    }
    SDL_RenderPresent(rn);                                // update screen
}

void setup(Context &ctx) {
    SDL_Init(SDL_INIT_EVERYTHING);
    
    ctx.window = SDL_CreateWindow(
        ctx.title.c_str(),
        ctx.x, ctx.y, ctx.w, ctx.h,
        SDL_WINDOW_SHOWN
        );
    ctx.rndr = SDL_CreateRenderer(ctx.window, -1, 0);
    SDL_SetRenderDrawBlendMode(ctx.rndr, SDL_BLENDMODE_BLEND);  // for alpha blending
}

int play(Context &ctx, const char *fname) {
    RGBA key(0xff, 0xff, 0xff, 0xff);
    
    ctx.sq  = new Tile(ctx.rndr, 400, 100, 200, 200);
    ctx.img = new Tile(ctx.rndr, 160, 160);
    if (!ctx.img->load(fname, &key)) {
        printf("IMG_Load: %s\n", IMG_GetError());
        return 1;
    }
    return 0;
}    

void teardown(Context &ctx) {
    SDL_DestroyRenderer(ctx.rndr);
    ctx.img->free();
    SDL_DestroyWindow(ctx.window);
    SDL_Quit();
}

int main(int argc, char** argv) {
    const char *fname = "tests/assets/owl.png";
    Context ctx;
    
    setup(ctx);
    if (play(ctx, fname)) return -1;
    
    emscripten_set_main_loop_arg(callback, &ctx, -1, 1);
    
    teardown(ctx);
  
    return 0;
}
