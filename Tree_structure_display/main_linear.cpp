/* Implementation of linear display
 *
 * This file is used to display a linear graph of data using SDL2.
 * This is served as the base structure to integrate tree's structures 
 * and compare their performances and omptimize for better performance.
 *
 * For more detailed implmentation of SDL2 for this purposes, refer
 * to the file App.h and App.cpp in the directory src.
 */

#include "src/App.h"
#include <chrono>
#include <array>
#include <list>
#include <memory>

#define TEXT_COLOR color::red
#define NUM_ENTITIES 10000
#define MAX_ENTITY_SIZE 100.0f


struct Rect
{
    Rect(){};
    Rect(Vec2<float> _pos, Vec2<float> _size): size{_size}, pos{_pos} {};
    Rect(Vec2<float> _pos, Vec2<float> _size, std::vector<int> _color): 
        size{_size}, pos{_pos}, color{_color} {};
    Rect(SDL_Rect rec) : size{(float)rec.w, (float)rec.h}, pos{(float)rec.x, (float)rec.y} {};

    Vec2<float> size{0.0f, 0.0f};
    Vec2<float> pos{0.0f, 0.0f};
    std::vector<int> color {0, 0, 0};

    constexpr bool contains(const Vec2<float>& point) const
    {
        return !(point.x < pos.x || 
                 point.y < pos.y || 
                 point.x >= (pos.x + size.x) || 
                 point.y >= (pos.y + size.y));
    }

    constexpr bool contains(const Rect& rect) const
    {
        return (rect.pos.x >= pos.x) && (rect.pos.x + rect.size.x < pos.x + size.x) &&
               (rect.pos.y >= pos.y) && (rect.pos.y + rect.size.y < pos.y + size.y);
    }

    constexpr bool overlaps(const Rect& rect) const
    {
        return (pos.x < rect.pos.x + rect.size.x && 
                pos.x + size.x >= rect.pos.x && 
                pos.y < rect.pos.y + rect.size.y && 
                pos.y + size.y >= rect.pos.y);
    }

    friend std::ostream& operator<<(std::ostream& os, const Rect& v)
    {
        os << v.pos.x << " , " << v.pos.y << " , " << v.size.x << " , " << v.size.y;
        return os;
    }
};


class TreeApp: public SDLCommon
{
    public:
        TreeApp() {_appName = "Trees For Display";};
        ~TreeApp() = default;

    protected:
        struct Object // a circle object
        {
            Vec2<float> pos = {0.0f, 0.0f};
            Vec2<float> vel = {0.0f, 0.0f};
            Vec2<float> size = {0.0f, 0.0f};
            float r = 1.0f;
            SDL_Color color = {0, 0, 0, 255};
            Rect GetArea() const {return {pos-r, {r*2.0f, r*2.0f}};};
        };

        float areaLength = MAX_ENTITY_SIZE * 100.0f;
        std::vector<Object> vObjects;

    protected:
        bool onUserInit() override 
        {
            auto randf = [](const float x, const float y){
                return (float)rand() / (float)RAND_MAX * (y - x) + x;
            };

            // create object to diplay
            for (int i = 0; i < NUM_ENTITIES; i++)
            {
                Object obj;
                obj.pos.x = randf(0.0f, areaLength);
                obj.pos.y = randf(0.0f, areaLength);
                obj.r = randf(0.0f, MAX_ENTITY_SIZE);
                obj.size.x = 2.0f * obj.r;
                obj.size.y = 2.0f * obj.r;
                obj.color = {(Uint8)(rand()%256), (Uint8)(rand()%256), (Uint8)(rand()%256)};
                vObjects.push_back(obj);
            }

            // Show some  information
            std::cout << "#objs created: " << vObjects.size() << std::endl;
            
            return true;
        };

        void onUserUpdate(float frameTime) override 
        {
            // handle keyboard inputs
            while (SDL_PollEvent(&_event) != 0)
            {
                if (_event.type == SDL_QUIT)
                {
                    _atomIsRunning = false;
                }
                else if (_event.type == SDL_MOUSEWHEEL)
                {
                    if (_event.wheel.y > 0)
                    {
                        Zoom(1.1);
                    }
                    else if (_event.wheel.y < 0)
                    {
                        Zoom(1.0/1.1);
                    }
                }
                else if (_event.type == SDL_KEYDOWN)
                {
                    switch (_event.key.keysym.sym)
                    {
                        case SDLK_UP: Pan(0, -10); break;
                        case SDLK_DOWN: Pan(0, 10); break;
                        case SDLK_LEFT: Pan(-10, 0); break;
                        case SDLK_RIGHT: Pan(10, 0); break;
                    }
                }
            }  
        }

        void onUserRender() override
        {
            Rect screen = {getCameraViewport()};
            size_t count = 0;

            auto ticStart = std::chrono::system_clock::now();
            for (const auto& obj : vObjects)
            {
                if (screen.overlaps(obj.GetArea()))
                {
                    DrawFilledCircle({(int)obj.pos.x, (int)obj.pos.y}, obj.r, obj.color);
                    count++;
                }
            }
            std::chrono::duration<double> ticDuration = std::chrono::system_clock::now() - ticStart;
            std::string info = "LINEAR: " + 
                                std::to_string(count) + "/" + 
                                std::to_string(vObjects.size()) + " Time: " + 
                                std::to_string(ticDuration.count()) + " s";
            DrawText(info, {10, 10}, TEXT_COLOR);
        }
};

int main()
{
    TreeApp treeapp;
    if (treeapp.init(800, 800, 10000, 10000))
        treeapp.execute();
    return 0;
}