#include "App.h"

#include <algorithm>

// constructor
SDLCommon::SDLCommon()
{
    _pTexture = nullptr;
}

// destructor
SDLCommon::~SDLCommon()
{
    SDL_FreeSurface(_pScreenSurface);
    SDL_FreeSurface(_pWindowSurface);
    if (!_pTexture)
        SDL_DestroyTexture(_pTexture);
    if (!_pTextTexture)
        SDL_DestroyTexture(_pTextTexture);
    TTF_CloseFont(_pTextFont);
    _pTextFont = nullptr;

    SDL_DestroyWindow(_pWindow);
    _pWindow = nullptr;
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

// init all sdl stuff and game related resources
bool SDLCommon::init(int sw, int sh, int tw, int th)
{
    _screenHeight = sh;
    _screenWidth = sw;
    _textureHeight = th;
    _textureWidth = tw;

    // initialize SDL
    std::cout << "DEBUG - initialize SDL."  << std::endl;
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        std::cout << "SDL cannot initialize. SDL_Error :" << SDL_GetError() << std::endl;
        return false;
    }

    // create window
    _pWindow = SDL_CreateWindow(_appName.c_str(), 
    							SDL_WINDOWPOS_CENTERED, 
                                SDL_WINDOWPOS_CENTERED,
                                _screenWidth, _screenHeight, 
                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    if (!_pWindow)
    {
        std::cout << "SDL window cannot be created. SDL_Error :" << SDL_GetError() << std::endl;
        return false;
    }

    // initialize SDL_TTF
    if (TTF_Init() < 0)
    {
        std::cout << "Error initializing SDL ttf" << TTF_GetError() << std::endl;
    }

    // load font
    if (_fontFileName != nullptr)
    { 
        std::cout << "DEBUG - Opening :" << _fontFileName << std::endl;
        _fontSizeX = 24;
        _pTextFont = TTF_OpenFont("./assets/VCR_OSD_MONO_1.001.ttf", _fontSizeX);
    }
    else
    {
        std::cout << "WARNING - no font provided." << std::endl;
    }

    if (!_pTextFont)
        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
    else
        _fontSizeY = TTF_FontLineSkip(_pTextFont);

    _frameStart = SDL_GetPerformanceCounter();
    _frameEnd = SDL_GetPerformanceCounter();

    // set camera
    _cameraViewport = {0, 0, _screenWidth, _screenHeight};
    _zoomScale = 1.0f;

    // set surfaces
    _pTextureSurface = SDL_CreateRGBSurface(0, _textureWidth, _textureHeight, 32, 0, 0, 0, 0);
    _pWindowSurface = SDL_GetWindowSurface(_pWindow);

    Uint32 backgroundColor = SDL_MapRGB(_pTextureSurface->format, 0, 0, 0);
    SDL_FillRect(_pTextureSurface, NULL, backgroundColor);

    _texturePixels = (Uint32*)_pTextureSurface->pixels;
    memset(_texturePixels, 0, _textureWidth * _textureHeight * sizeof(Uint32));
    
    std::cout << "DEBUG - finished init." << std::endl;

    return _atomIsRunning = true;
}


// game main loop
void SDLCommon::execute()
{
    if (!onUserInit())
    {
        std::cout << "DEBUG - user init failed." << std::endl;
        _atomIsRunning = false;
    }
    
    while(_atomIsRunning)
    {
        //Handle elapse time
        _frameEnd = SDL_GetPerformanceCounter();
        float frameTime = (_frameEnd - _frameStart) / (float)(SDL_GetPerformanceFrequency());
        _frameStart = _frameEnd;

        // set background color
        SDL_FillRect(_pTextureSurface, NULL, convertColorUint(color::black));
        
        // user defined game loop
        onUserUpdate(frameTime); 
        
        // user defined rendering
        onUserRender();
        
        // Draw the visible portion of the canvas to the screen
        SDL_Rect srcRect = _cameraViewport;
        SDL_Rect dstRect = { 0, 0, _screenWidth, _screenHeight };
        SDL_BlitScaled(_pTextureSurface, &srcRect, _pWindowSurface, &dstRect);
        if (_pTextSurface != nullptr)
            SDL_BlitSurface(_pTextSurface, NULL, _pWindowSurface, &dstRect);

        // Update window surface
        SDL_UpdateWindowSurface(_pWindow);
    }
    onUserStop();
}

void SDLCommon::DrawText(const std::string str, Vec2<int> pos, SDL_Color color)
{
    SDL_Surface *textSurface;

    if(!(textSurface=TTF_RenderText_Blended(_pTextFont, str.c_str(), color))) 
    {
        std::cout << "Unable to create text surface: " << TTF_GetError() << std::endl;
    } 
    else 
    {
        _pTextSurface = SDL_ConvertSurfaceFormat(textSurface, SDL_PIXELFORMAT_ARGB8888, 0);
        _textDstRect = { pos.x, pos.y, textSurface->w, textSurface->h };
        SDL_BlitSurface(_pTextSurface, NULL, _pWindowSurface, NULL);
    }
    
    SDL_FreeSurface(textSurface);
}

void SDLCommon::DrawTextPixels(const std::string str, Vec2<int> pos, SDL_Color color)
{
    SDL_Surface* textSurface = TTF_RenderText_Blended(_pTextFont, str.c_str(), color);
    if (!textSurface) 
    {
        std::cerr << "Unable to render text surface! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return;
    }

    // Ensure the surface is in the correct pixel format
    _pTextSurface = SDL_ConvertSurfaceFormat(textSurface, SDL_PIXELFORMAT_ARGB8888, 0);
    if (!_pTextSurface) 
    {
        std::cerr << "Unable to convert text surface to correct format! SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(textSurface);
        return;
    }

    Uint32* textPixels = static_cast<Uint32*>(_pTextSurface->pixels);
    for (int i = 0; i < _pTextSurface->w; ++i) 
    {
        for (int j = 0; j < _pTextSurface->h; ++j) 
        {
            Uint32 pixel = textPixels[j * _pTextSurface->w + i];
            if ((pixel & 0xFF000000) != 0) // Check if pixel is not transparent
                setPixel(_pTextSurface, pos.x + i, pos.y + j, pixel);
        }
    }
    SDL_FreeSurface(textSurface);
}

// function to draw a line
void SDLCommon::DrawLine(int x1, int y1, int x2, int y2, SDL_Color color)
{
    if (x1 == x2)
        drawVerticalLine(x1, y1, y2, color);
    else if (y1 == y2)
        drawHorizontalLine(y1, x1, x2, color);
    else
    {
        int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
        int dy = abs(y2 - y1), sy = y1 < y2 ? 1 : -1; 
        int err = (dx > dy ? dx : -dy) / 2, e2;

        while (true) {
            setPixel(x1, y1, color);
            if (x1 == x2 && y1 == y2) break;
            e2 = err;
            if (e2 > -dx) { err -= dy; x1 += sx; }
            if (e2 <  dy) { err += dx; y1 += sy; }
        }
    }
}

void SDLCommon::DrawRect(Vec2<int> pos, int w, int h, SDL_Color color)
{
    DrawLine(pos.x, pos.y, pos.x+w, pos.y, color);
    DrawLine(pos.x, pos.y, pos.x, pos.y+h, color);
    DrawLine(pos.x+w, pos.y, pos.x+w, pos.y+h, color);
    DrawLine(pos.x, pos.y+h, pos.x+w, pos.y+h, color);
}

void SDLCommon::DrawFilledRect(Vec2<int> pos, int w, int h, SDL_Color color)
{
    int xStart = std::max(0, pos.x);
    int xEnd = std::min(_textureWidth - 1, pos.x + w - 1);
    int yStart = std::max(0, pos.y);
    int yEnd = std::min(_textureHeight - 1, pos.y + h - 1);

    for (int j = yStart; j <= yEnd; ++j) 
        DrawLine(xStart, j, xEnd, j, color);
}

void SDLCommon::DrawCircle(Vec2<int> pos, int r, SDL_Color color)
{
    int x = r - 1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (r << 1);

    while (x >= y) 
    {
        setPixel(pos.x + x, pos.y + y, color);
        setPixel(pos.x + y, pos.y + x, color);
        setPixel(pos.x - y, pos.y + x, color);
        setPixel(pos.x - x, pos.y + y, color);
        setPixel(pos.x - x, pos.y - y, color);
        setPixel(pos.x - y, pos.y - x, color);
        setPixel(pos.x + y, pos.y - x, color);
        setPixel(pos.x + x, pos.y - y, color);

        if (err <= 0) 
        {
            y++;
            err += dy;
            dy += 2;
        }
        if (err > 0) 
        {
            x--;
            dx += 2;
            err += dx - (r << 1);
        }
    }
}

void SDLCommon::DrawFilledCircle(Vec2<int> pos, int r, SDL_Color color)
{
    int x = r;
    int y = 0;
    int err = 1 - x;

    while (x >= y) 
    {
        // Draw horizontal lines between symmetrical points
        DrawLine(pos.x - x, pos.y + y, pos.x + x, pos.y + y, color);
        DrawLine(pos.x - x, pos.y - y, pos.x + x, pos.y - y, color);
        DrawLine(pos.x - y, pos.y + x, pos.x + y, pos.y + x, color);
        DrawLine(pos.x - y, pos.y - x, pos.x + y, pos.y - x, color);

        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

Vec2<float> SDLCommon::TextureToWindow(Vec2<float>& texturePos) 
{
    Vec2<float> windowPoint;
    windowPoint.x = (texturePos.x - _cameraViewport.x) * _zoomScale;
    windowPoint.y = (texturePos.y - _cameraViewport.y) * _zoomScale;
    return windowPoint;
}

// Convert window position to canvas position
Vec2<float> SDLCommon::WindowToTexture(Vec2<float>& windowPos) 
{
    Vec2<float> texturePoint;
    texturePoint.x = windowPos.x / _zoomScale + _cameraViewport.x;
    texturePoint.y = windowPos.y / _zoomScale + _cameraViewport.y;
    return texturePoint;
}

void SDLCommon::Pan(int dx, int dy)
{
    // update camera
    _cameraViewport.x += dx / _zoomScale;
    _cameraViewport.y += dy / _zoomScale;
    
    //clip if reach the border of the map
    if (_cameraViewport.x < 0)
        _cameraViewport.x = 0;
    if (_cameraViewport.x > _textureWidth - _screenWidth / _zoomScale)
        _cameraViewport.x = _textureWidth- _screenWidth / _zoomScale;
    if (_cameraViewport.y < 0)
        _cameraViewport.y = 0;
    if (_cameraViewport.y > _textureHeight - _screenHeight / _zoomScale)
        _cameraViewport.y =  _textureHeight - _screenHeight / _zoomScale; 
}

void SDLCommon::Zoom(const float scale, float* cursorSize)
{
    float potentialzoom = _zoomScale * scale;
 
    if (_screenWidth / potentialzoom <=  _textureWidth&& 
        _screenHeight / potentialzoom <= _textureHeight&&
        _screenWidth / potentialzoom >= 0.0f && 
        _screenHeight / potentialzoom >= 0.0f) 
    {
        _zoomScale *= scale;
        _cameraViewport.w = static_cast<int>(_screenWidth / _zoomScale);
        _cameraViewport.h = static_cast<int>(_screenHeight / _zoomScale);

        if (cursorSize)
            *cursorSize = *cursorSize / scale;
    }
    else
    {
        if (cursorSize)
            *cursorSize = *cursorSize / (_textureWidth / _cameraViewport.w);
        _cameraViewport.w = static_cast<int>(_textureWidth);
        _cameraViewport.h = static_cast<int>(_textureHeight);
        _zoomScale = _screenWidth / _textureWidth;
    }
}

// helper function to convert color from rgba to uint32
Uint32 SDLCommon::convertColorUint(SDL_Color color)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    Uint32 pixelColor = (color.r << 24) + (color.g << 16) + (color.b << 8) + 255;
#else
    Uint32 pixelColor = (255 << 24) + (color.b << 16) + (color.g << 8) + color.r;
#endif

    return pixelColor;
}


// helper function to convert color from uint32 to rgba
SDL_Color SDLCommon::convertColorRGBA(Uint32 color)
{
    SDL_Color out;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    out.r = (color >> 24) & 0xff;
    out.g = (color >> 16) & 0xff;
    out.b = (color >> 8) & 0xff;
    out.a = color & 0xff;

#else
    out.a = (color >> 24) & 0xff;
    out.b = (color >> 16) & 0xff;
    out.g = (color >> 8) & 0xff;
    out.r = color & 0xff;
#endif

    return out;
}

// function to change indivitual pixel value at a position on texture
void SDLCommon::setPixel(const int x, const int y, SDL_Color color)
{
    if((x >= 0) && (x < _textureWidth) && (y >= 0) && (y < _textureHeight))
        _texturePixels[y * _textureWidth + x] = convertColorUint(color);
}

// overloaded function of setPixel
void SDLCommon::setPixel(const int x, const int y, Uint32 color)
{
    if((x >= 0) && (x < _textureWidth) && (y >= 0) && (y < _textureHeight))
        _texturePixels[y * _textureWidth + x] = color;
}

void SDLCommon::setPixel(SDL_Surface* surface, const int x, const int y, Uint32 color)
{
    if (x >= 0 && x < surface->w && y >= 0 && y < surface->h) 
    {
        Uint32* pixels = (Uint32*)surface->pixels;
        pixels[y * surface->w + x] = color;
    }
}

// function to get the mouse position on the map
Vec2<float> SDLCommon::getMousePosOnRender()
{
    int x, y;
    Vec2<float> pos;
    SDL_GetMouseState(&x, &y); // window position
    pos.x = (float)x / _zoomScale + (float)_cameraViewport.x;
    pos.y = (float)y / _zoomScale + (float)_cameraViewport.y;
    return pos;
}

// function to load an image to a surface
SDL_Surface* SDLCommon::loadImageToSurface(const std::string filename, int& w, int& h)
{
    //The final optimized image
    if (!std::ifstream(filename.c_str()).good())
    {
        std::cout << "file not exist" << std::endl;
        return nullptr;
    }

    //Load image at specified path
    SDL_Surface *s = IMG_Load(filename.c_str());

    if(s == nullptr)
    {
        printf("Unable to load image %s! SDL_image Error: %s\n", filename.c_str(), IMG_GetError());
        return nullptr;
    }

    w = s->w;
    h = s->h;
    return s;
}

SDL_PixelFormat* SDLCommon::_pSpriteFormat = nullptr;

//function to load an image to pixel format
std::vector<std::shared_ptr<Uint32[]>> SDLCommon::loadImageToPixels(const std::string filename, std::vector<SDL_Rect> rects)
{
    std::vector<std::shared_ptr<Uint32[]>> vecPixels;
    Uint32* pixels = nullptr;

    //Load image at specified path
    int imgW, imgH; // dimension of the raw image
    SDL_Surface* loadedSurface = loadImageToSurface(filename, imgW, imgH);

    if (!loadedSurface)
    {
        std::cout << "ERR - surface not valid." << std::endl;
    }
    else
    {
        pixels = static_cast<Uint32*>(loadedSurface->pixels);
    }
    if(pixels == nullptr)
    {
        std::cout << "ERR - pixels poiter is not valid" << std::endl;
    }

    for (int i = 0; i<rects.size(); i++) // we loop through all sprites
    {
        size_t sizePixels = rects[i].w * rects[i].h;
        std::shared_ptr<Uint32[]> subPixels(new Uint32[sizePixels], std::default_delete<Uint32[]>());

        int l = 0;
        for (int k=rects[i].y; k<rects[i].y+rects[i].h; k++)
        {
            SDL_memcpy(&subPixels.get()[l*rects[i].w], &pixels[k*imgW+rects[i].x], sizeof(Uint32)*rects[i].w);
            l++;
        }

        vecPixels.push_back(subPixels);
        
    }

    _pSpriteFormat = loadedSurface->format;
    SDL_FreeSurface(loadedSurface);

    return vecPixels;
}

// helper function to create a transparant surface
SDL_Surface * SDLCommon::createColorSurface(int w, int h)
{
    Uint32 rmask, gmask, bmask, amask;

    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
    #else
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
    #endif

    SDL_Surface *surf = SDL_CreateRGBSurface(0, w, h, 32, rmask, gmask, bmask, amask);

    if (surf)
        return surf;
    else
        return nullptr;
} 

void SDLCommon::drawHorizontalLine(int y, int xStart, int xEnd, SDL_Color color) 
{
    if (y < 0 || y >= _textureWidth) return; // Ensure y is within bounds
    xStart = std::max(0, xStart);
    xEnd = std::min(_textureWidth - 1, xEnd);
    
    for (int x = xStart; x <= xEnd; ++x) {
        _texturePixels[y * _textureWidth + x] = convertColorUint(color);
    }
}

void SDLCommon::drawVerticalLine(int x, int yStart, int yEnd, SDL_Color color) 
{
    // draw a vertical line by setting the pixels
    if (x < 0 || x >= _textureWidth) return; // Ensure x is within bounds
    yStart = std::max(0, yStart);
    yEnd = std::min(_textureHeight - 1, yEnd);

    for (int y = yStart; y <= yEnd; ++y) {
        _texturePixels[y * _textureWidth + x] = convertColorUint(color);
    }
}