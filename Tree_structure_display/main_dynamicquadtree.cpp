
/* Implementation of dynamic quadtree
 *
 * This file use the quadtree to manage the objects in realtime where user can remove
 * objects anytime. So the tree should update the information about the objects
 * constantly.
 * In the previous implementation, the objects were stored in the tree and to find them,
 * we have to start from the root node for each search until we find it. In this way,
 * if the object is located at the bottom right of the display window, which means it is
 * located in the deepest node of the tree, we will traverse the whole tree until we reach
 * the corresponding node, whiwh is slow.
 * To overcome this performance issue, we can store the address of the node for each
 * object. So instead of traversing the tree each time, we have direct access of the
 * object in the tree.
 *
 * So, we modified the static quadtree to dynamic quadtree. You can compare the code to
 * see the difference for implmentation. 
 */

#include "src/App.h"
#include <chrono>
#include <array>
#include <list>
#include <memory>
#include <map>

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
class DynamicQuadTree
{
    private:
        // information about the container address and the iterator corresponding to 
        // the current object.
        template <class OBJ_NODE>
        struct ObjectInfo
        {
            std::list<OBJ_NODE>* _pObjects;
            typename std::list<OBJ_NODE>::iterator _objIt;
        };

        // now instead to hald the objects, the node hold the object address in the list.
        template <class OBJ_NODE>
        struct Node
        {
            Rect _area; // the area to be divided
            int _depth; // the depth of the tree, time to devide into quads
            std::array<Rect, 4> _vSubAreas{}; // areas of the quads
            std::array<std::shared_ptr<Node<OBJ_NODE>>, 4> _vSubNodes{}; // children of the node
            std::list<OBJ_NODE> _vObjects; // the objects belonging to the node


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

        // information about the actual object and its location in the quadtree.
        struct ObjectListItem
        {
            OBJ_T _obj;
            ObjectInfo<typename std::list<ObjectListItem>::iterator> _locationInTree;
        };
        using objType = typename std::list<ObjectListItem>::iterator;
        // all objects will be stored in a container and we pass their location to the tree.
        std::list<ObjectListItem> _vObjects;

        std::shared_ptr<Node<objType>> _root;
        Rect _area = {{0.0f, 0.0f}, {100.0f, 100.0f}};

        
        // recursive insert of an object
        ObjectInfo<objType> insert(std::shared_ptr<Node<objType>>& node, const Rect& r, const objType& obj, int depth)
        {
            if (!node) node = std::make_shared<Node<objType>>(r, depth);

            for (int i=0; i<4; i++)
            {
                if (node->_vSubAreas[i].contains(obj->_obj.GetArea()))
                {
                    if (node->_depth + 1 < MAX_DEPTH)
                    {
                        if (!node->_vSubNodes[i])
                        {
                            node->_vSubNodes[i] = std::make_shared<Node<objType>>(node->_vSubAreas[i], node->_depth+1);
                        }
                        return insert(node->_vSubNodes[i], node->_vSubAreas[i], obj, node->_depth+1);
                    }   
                }
            }
            node->_vObjects.push_back(obj);
            // reture the adress of the object in the tree
            // and this information will be stored with the object in their container.
            return {&(node->_vObjects), std::prev(node->_vObjects.end())};
        }

        // recursive search of objects in an area
        void search(const std::shared_ptr<Node<objType>>& node, const Rect& r, std::list<objType>& result) const
        {
            if (r.overlaps(node->_area))
            {
                for (const auto& obj : node->_vObjects)
                { 
                    if (r.overlaps(obj->_obj.GetArea()))
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

        // this is the remove function when traversing the whole tree
        // not used in this implementation
        bool remove(std::shared_ptr<Node<objType>>& node, const objType& obj)
        {
            auto it = std::find_if(node->_vObjects.begin(), node->_vObjects.end(), 
                            [&obj](const OBJ_T& item)
                            {
                                return item == obj;
                            });

            if (it != node->_vObjects.end())
            {
                node->_vObjects.erase(it);
                return true;
            }
            else
            {
                for (int i=0; i<4; i++)
                {
                    if (node->_vSubNodes[i])
                    {
                        if (remove(node->_vSubNodes[i], obj))
                        return true;
                    }
                }
            }
            return false;
        }

        // recursive print of the tree structure
        void print(const std::shared_ptr<Node<objType>>& node) const
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
        void items(const std::shared_ptr<Node<objType>>& node, std::list<objType>& result) const
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
        size_t size(const std::shared_ptr<Node<objType>>& node) const
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
        DynamicQuadTree(): _root(nullptr) {};

        void SetArea(const Rect r)
        {
            _area = r;
        }

        void insert(const OBJ_T& obj)
        {
            ObjectListItem item;
            item._obj = obj;
            _vObjects.push_back(item);
            _vObjects.back()._locationInTree = insert(_root, _area, std::prev(_vObjects.end()), 0);  
        }

        // old remove function
        bool remove(const OBJ_T& obj)
        {
            return remove(_root, obj);
        }
        
        // This is the remove function when we store the object address in the tree.
        // Since we have direct access to the object in the tree, we can remove it 
        // easily, whithout any recursive loops.
        void remove(objType& obj)
        {
            obj->_locationInTree._pObjects->erase(obj->_locationInTree._objIt);
            _vObjects.erase(obj);
    
        }

        // Now the search returns the adresses of the objects in their holding list.
        std::list<objType> search(const Rect& r)
        {
            std::list<objType> result;
            search(_root, r, result);
            return result;
        }

        // std::list<OBJ_T> search(const Rect& r)
        // {
        //     std::list<OBJ_T> result;
        //     search(_root, r, result);
        //     return result;
        // }

        std::list<objType> items() const
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
            bool operator==(const CObject& other) const {return pos == other.pos && size == other.size;};
        };

        float areaLength = MAX_ENTITY_SIZE * 1000.0f;
        float _cursorSize = 50.0f;
        bool _bErase= false;
        Rect searchRect;
        std::vector<CObject> vObjects;
        DynamicQuadTree<CObject> _dynamicQuadTree;
        bool _bUseQuadTree = true; // option to use QuadTree

        bool onUserInit() override 
        {
            // initialize the tree
            _dynamicQuadTree.SetArea({{0.0f, 0.0f}, {areaLength, areaLength}}); 
            
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
                _dynamicQuadTree.insert(obj); // insert objects in quadtree
            }

            // Show some  information
            std::cout << "objs created: " << vObjects.size() << std::endl;
            std::cout << "objs in QuadTree: " << _dynamicQuadTree.size() << std::endl;

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
                        _cursorSize /= 1.1;
                    }
                    else if (_event.wheel.y < 0)
                    {
                        Zoom(1.0/1.1);
                        _cursorSize *= 1.1;
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
                        case SDLK_q: _cursorSize += 10.0f; break;
                        case SDLK_a: _cursorSize -= 10.0f; break;
                        case SDLK_LSHIFT: _bErase = true; break;
                        default: break;
                    }
                    _cursorSize = std::clamp(_cursorSize, 10.0f, 500.0f);
                }
                if (_event.type == SDL_KEYUP)
                {
                    switch (_event.key.keysym.sym)
                    {
                        case SDLK_LSHIFT: if (_bErase) _bErase = false; break;
                    }
                }
            }
            Vec2<float> posMouse = getMousePosOnRender();
            Vec2<float> searchArea = {_cursorSize, _cursorSize};
            searchRect = {posMouse - searchArea/2.0f, searchArea};

            if (_bErase)
            {
                // std::cout << "erase" << std::endl;
                auto r = _dynamicQuadTree.search(searchRect);
                int n = 0;
                for (auto& i : r)
                {
                    _dynamicQuadTree.remove(i);
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
                for (const auto& item : _dynamicQuadTree.search(r))
                {
                    DrawFilledCircle({(int)item->_obj.pos.x, (int)item->_obj.pos.y}, item->_obj.r, item->_obj.color);
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
            if (_bErase)
                DrawFilledRect({(int)searchRect.pos.x, (int)searchRect.pos.y}, searchRect.size.x, searchRect.size.y, {255, 255, 255, 100});
        }
};


int main()
{
    TreeApp quadtree;
    if (quadtree.init(800, 800, 20000, 20000))
        quadtree.execute();
    return 0;
}