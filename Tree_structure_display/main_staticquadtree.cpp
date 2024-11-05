/* Implementation of static quadtree
 *
 * Based on the display of linear graph of data using SDL2, we use 
 * a quadtree to handle the search.
 */

#include "src/App.h"
#include <chrono>
#include <array>
#include <list>
#include <memory>

#define TEXT_COLOR color::red
#define NUM_ENTITIES 1000000
#define MAX_ENTITY_SIZE 100.0f
#define MAX_DEPTH 8


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


template <class OBJ_T>
class StaticQuadTree
{
    private:
        struct Node
        {
            Rect _area; // the area to be divided
            int _depth; // the depth of the tree, time to devide into quads
            std::array<Rect, 4> _vSubAreas{}; // areas of the quads
            std::array<std::shared_ptr<Node>, 4> _vSubNodes{}; // children of the node
            std::vector<OBJ_T> _vObjects; // the objects belonging to the node

            Node(const Rect& r, int depth) : _area(r), _depth(depth)
            {
                Vec2<float> childSize = _area.size / 2.0f;

                _vSubAreas =
                {
                    Rect(_area.pos, childSize), // top left
                    Rect(_area.pos + Vec2<float>{childSize.x, 0.0f}, childSize), // top right
                    Rect(_area.pos + Vec2<float>{0.0f, childSize.y}, childSize), // bottom left
                    Rect(_area.pos + childSize, childSize) // bottom right
                };
            }
        };

        std::shared_ptr<Node> _root;
        Rect _area = {{0.0f, 0.0f}, {100.0f, 100.0f}};
        
        // recursive insert of an object
        void insert(std::shared_ptr<Node>& node, const Rect& r, const OBJ_T& obj, int depth)
        {
            if (!node) node = std::make_shared<Node>(r, depth);

            for (int i=0; i<4; i++)
            {
                if (node->_vSubAreas[i].contains(obj.GetArea()))
                {
                    if (node->_depth + 1 < MAX_DEPTH)
                    {
                        if (!node->_vSubNodes[i])
                        {
                            node->_vSubNodes[i] = std::make_shared<Node>(node->_vSubAreas[i], node->_depth+1);
                        }
                        insert(node->_vSubNodes[i], node->_vSubAreas[i], obj, node->_depth+1);
                        return;
                    }   
                }
            }
            node->_vObjects.push_back(obj);
        }

        // recursive search of objects in an area
        void search(const std::shared_ptr<Node>& node, const Rect& r, std::list<OBJ_T>& result) const
        {
            if (r.overlaps(node->_area))
            {
                for (const auto& obj : node->_vObjects)
                { 
                    if (r.overlaps(obj.GetArea()))
                        result.push_back(obj);
                }
            
                for (int i=0; i<4; i++)
                {
                    if (node->_vSubNodes[i])
                    {
                        if (r.contains(node->_vSubAreas[i]))
                            items(node->_vSubNodes[i], result);
                        else if (node->_vSubAreas[i].overlaps(r))
                            search(node->_vSubNodes[i], r, result);
                    }
                }
            }
        }

        // recursive print of the tree structure
        void print(const std::shared_ptr<Node>& node) const
        {
            if (!node) return;

            // Print current node with indentation based on depth
            for (int i = 0; i < node->_depth; i++) std::cout << "  ";
            std::cout << "(";
            std::cout << node->_area;
            std::cout << ")" << std::endl;

            for (int i=0; i<4; i++)
            {
                if (node->_vSubNodes[i])
                {
                    print(node->_vSubNodes[i]);
                }
            }
        }

        // recursive output all objects (parents and children) given a parent node
        void items(const std::shared_ptr<Node>& node, std::list<OBJ_T>& result) const
        {
            if (!node) return;

            for (const auto& obj : node->_vObjects)
            { 
                result.push_back(obj);
            }
            for (const auto& child : node->_vSubNodes)
            {
                items(child, result);
            }
        }

        // recursive count the number of objects (parents and children) given a parent node
        size_t size(const std::shared_ptr<Node>& node) const
        {
            size_t s = node->_vObjects.size();
            for (int i=0; i<4; i++)
            {
                if (node->_vSubNodes[i])
                    s += size(node->_vSubNodes[i]);
            }
            return s;
        }

    public:
        StaticQuadTree(): _root(nullptr) {};

        void SetArea(const Rect r)
        {
            _area = r;
        }

        void insert(const OBJ_T& obj)
        {
            insert(_root, _area, obj, 0);
        }

        std::list<OBJ_T> search(const Rect& r)
        {
            std::list<OBJ_T> result;
            search(_root, r, result);
            return result;
        }

        std::list<OBJ_T> items() const
        {
            std::list<OBJ_T> results;
            items(_root, results);
            return results;
        }

        size_t size() const
        {
            return size(_root);
        }

        void print() const
        {
            print(_root);
        }
};


class TreeApp: public SDLCommon
{
    public:
        TreeApp() {_appName = "Trees For Display";};
        ~TreeApp() = default;

    protected:
        struct CObject
        {
            Vec2<float> pos = {0.0f, 0.0f};
            Vec2<float> vel = {0.0f, 0.0f};
            Vec2<float> size = {0.0f, 0.0f};
            float r = 1.0f;
            SDL_Color color = {0, 0, 0, 255};
            Rect GetArea() const {return {pos-r, {r * 2.0f, r * 2.0f}};};
        };

        float areaLength = MAX_ENTITY_SIZE * 1000.0f;
        std::vector<CObject> vObjects;
        StaticQuadTree<CObject> _staticQuadTree;
        bool _bUseQuadTree = true; // option to use QuadTree

        bool onUserInit() override 
        {
            // initialize the tree
            _staticQuadTree.SetArea({{0.0f, 0.0f}, {areaLength, areaLength}}); 
            
            auto randf = [](const float x, const float y){
                return (float)rand() / (float)RAND_MAX * (y - x) + x;
            };

            for (int i = 0; i < NUM_ENTITIES; i++)
            {
                CObject obj;
                obj.pos.x = randf(0.0f, areaLength);
                obj.pos.y = randf(0.0f, areaLength);
                obj.r = randf(0.0f, MAX_ENTITY_SIZE);
                obj.size.x = 2.0f * obj.r;
                obj.size.y = 2.0f * obj.r;
                obj.color = {(Uint8)(rand()%256), (Uint8)(rand()%256), (Uint8)(rand()%256)};
                vObjects.push_back(obj);
                _staticQuadTree.insert(obj); // insert objects in quadtree
            }

            // Show some  information
            std::cout << "objs created: " << vObjects.size() << std::endl;
            std::cout << "objs in QuadTree: " << _staticQuadTree.size() << std::endl;

            // // uncomment this section to show the tree structure
            // std::cout << "objs tree: " << std::endl;
            // _Tree.print();

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
                        case SDLK_TAB: _bUseQuadTree = !_bUseQuadTree; break; // add quadtree option
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

            if (_bUseQuadTree)
            {
                auto ticStart = std::chrono::system_clock::now();
                Rect r = Rect(_cameraViewport);
                for (const auto& item : _staticQuadTree.search(r))
                {
                    DrawFilledCircle({(int)item.pos.x, (int)item.pos.y}, item.r, item.color);
                    count++;
                }
                std::chrono::duration<double> ticDuration = std::chrono::system_clock::now() - ticStart;
                std::string info = "QUADTREE: "  + 
                                std::to_string(count) + "/" + 
                                std::to_string(vObjects.size()) + " Time: " + 
                                std::to_string(ticDuration.count()) + " s";
                DrawText(info, {10, 10}, TEXT_COLOR);
            }
            else
            {
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
                std::string info = "LINEAR: "  + 
                                std::to_string(count) + "/" + 
                                std::to_string(vObjects.size()) + " Time: " + 
                                std::to_string(ticDuration.count()) + " s";
                DrawText(info, {10, 10}, TEXT_COLOR);
            }  
        }
};


int main()
{
    TreeApp quadtree;
    if (quadtree.init(800, 800, 20000, 20000))
        quadtree.execute();
    return 0;
}