#pragma once

#include <iostream>
#include <string>
#include <atomic>
#include <vector>
#include <memory>
#include <math.h>
#include <fstream>
#include <algorithm>

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_image.h"

const float PI = 3.1415926;


template<class T>
struct Vec2
{
    T x = 0;
    T y = 0;

    Vec2<T>(T _x = 0, T _y = 0) : x(_x), y(_y) {};

    Vec2<T> operator/(const T& d) {return {x / d, y / d};};
    Vec2<T> operator+(const Vec2<T>& v) {return {x + v.x, y + v.y};};
    Vec2<T> operator-(const Vec2<T>& v) const { return {x - v.x, y - v.y}; };
    Vec2<T> operator-(const T& d) const {return {x - d, y - d};};
    T operator[](int i) const { return (i == 0) ? x : y; };
    bool operator==(const Vec2<T>& v) const { return (x == v.x && y == v.y); };
};


// color definition
namespace color
{
    constexpr SDL_Color white{255, 255, 255};
    constexpr SDL_Color black{0, 0, 0};
    constexpr SDL_Color red{255, 0, 0};
    constexpr SDL_Color blue{0, 0, 255};
    constexpr SDL_Color green{0, 255, 0};
    constexpr SDL_Color cyan{0, 255, 255};
    constexpr SDL_Color grey{128, 128, 128};
    constexpr SDL_Color fuchsia{255, 0, 255};
    constexpr SDL_Color purple{128, 0, 128};
    constexpr SDL_Color teal{0, 128, 128};
    constexpr SDL_Color yellow{255, 255, 0};
    constexpr SDL_Color orange{255, 165, 0};
    constexpr SDL_Color tomato{255, 99, 71};
    constexpr SDL_Color darkgreen{0, 100, 0};
    constexpr SDL_Color darkblue{0, 0, 139};
    constexpr SDL_Color olive{128, 128, 0};
    constexpr SDL_Color brown{165, 42, 42};
    constexpr SDL_Color lightblue{173, 216, 230};
    constexpr SDL_Color aquamarine{127, 255, 212};
    constexpr SDL_Color silver{192, 192, 192};
}


// base class to handle graphics using sdl
class SDLCommon
{    
    public:
        virtual bool onUserInit() = 0;
        virtual void onUserUpdate(float frameTime) = 0;
        virtual void onUserRender() = 0;
        virtual void onUserStop() {};

        void execute();

        void DrawText(const std::string str, Vec2<int> pos, SDL_Color color={0, 0, 0, 255});
        void DrawTextPixels(const std::string str, Vec2<int> pos, SDL_Color color);
        void DrawLine(int x1, int y1, int x2, int y2, SDL_Color color);
        void DrawRect(Vec2<int> pos, int w, int h, SDL_Color color={0, 0, 0, 255});
        void DrawFilledRect(Vec2<int> pos, int w, int h, SDL_Color color={0, 0, 0, 255});
        void DrawCircle(Vec2<int> pos, int r, SDL_Color color={0, 0, 0, 255});
        void DrawFilledCircle(Vec2<int> pos, int r, SDL_Color color={0, 0, 0, 255});

        void setPixel(const int x, const int y, SDL_Color color);
        void setPixel(const int x, const int y, Uint32 color);
        void setPixel(SDL_Surface* surface, const int x, const int y, Uint32 color);
        
        inline Vec2<int> getScreenSize() const { return {_screenWidth, _screenHeight}; };
        inline Vec2<int> getTexttureSize() const { return {_textureWidth, _textureHeight}; };
        inline SDL_Rect getCameraViewport() const { return _cameraViewport; };

        void Pan(int dx, int dy);
        void Zoom(const float scale, float* cursorSize=nullptr);
        Vec2<float> TextureToWindow(Vec2<float>& texturePos);
        Vec2<float> WindowToTexture(Vec2<float>& windowPos);

        Vec2<float> getMousePosOnRender();

    public:
        SDLCommon();
        ~SDLCommon();

        bool init(int w, int h, int px, int py);

        static SDL_Surface* loadImageToSurface(const std::string filename, int& w, int& h);
        static std::vector<std::shared_ptr<Uint32[]>> loadImageToPixels(const std::string filename, std::vector<SDL_Rect> rects);
        
    protected:

        static Uint32 convertColorUint(SDL_Color color);
        static SDL_Color convertColorRGBA(Uint32 color);
   
        SDL_Window *_pWindow = nullptr;
        TTF_Font *_pTextFont = nullptr;
        SDL_Surface *_pScreenSurface = nullptr;
        Uint32 *_backgroundPixels = nullptr;
        SDL_Texture *_pTextTexture = nullptr;
        SDL_Rect _textDstRect;
        SDL_Surface *_pTextSurface = nullptr;
        SDL_Event _event;

        SDL_Surface* _pTextureSurface = nullptr;
        SDL_Surface* _pWindowSurface = nullptr;

        int _fontSize;
        int _fontSizeX;
        int _fontSizeY;

        Uint64 _frameStart;
        Uint64 _frameEnd;
        SDL_PixelFormat* _format;

        // for text
        std::vector<std::string> _vecTexts;
        std::vector<SDL_Rect> _vecRect;
        int _textSize; // for showing

        int _screenHeight;
        int _screenWidth;
        int _pixelSizeX;
        int _pixelSizeY;
        int _textureHeight;
        int _textureWidth;
        const char* _fontFileName = "playfair.ttf";
        std::string _appName;
        Uint32 *_texturePixels;
        SDL_Texture *_pTexture;
        std::vector<SDL_Color> _textureColors;
        Vec2<int> _cameraPos;
        SDL_Rect _cameraViewport;
        float _zoomScale;
        Vec2<int> _cursorPos;

        static SDL_PixelFormat* _pSpriteFormat;
        inline static std::atomic<bool> _atomIsRunning; // variable to control global game loop


        SDL_Surface * createColorSurface(int w, int h);
        void drawHorizontalLine(int y, int xStart, int xEnd, SDL_Color color);
        void drawVerticalLine(int x, int yStart, int yEnd, SDL_Color color);
};