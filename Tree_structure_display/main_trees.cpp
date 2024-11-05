/* Implementation and comparison of trees
 *
 * Based on the implementation of linear search, we implemented the quadtree, 
 * grid based structure, and the KDTree for the static 2D search.
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

    // https://www.cs.umd.edu/class/fall2019/cmsc420-0201/Lects/lect14-kd-query.pdf
    Rect leftRect(const Vec2<float>& point) const
    { 
        return Rect(pos, Vec2<float>{point.x - pos.x, size.y});
    }

    Rect rightRect(const Vec2<float>& point) const
    {
        return Rect({point.x, pos.y}, Vec2<float>{pos.x + size.x - point.x, size.y});
    }

    Rect upperRect(const Vec2<float>& point) const
    {
        return Rect({pos.x, point.y}, Vec2<float>{size.x, pos.y + size.y - point.y});
    }

    Rect lowerRect(const Vec2<float>& point) const
    {
        return Rect(pos, Vec2<float>{size.x, point.y - pos.y});
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
            // if (!node) return;

            // if (r.contains(node->_area))
            // {
            //     items(node, result);
            //     return;
            // }
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


template <class OBJ_T>
class GridTree
{
    private:
        struct Node
        {
            Rect _area; // the area to be divided
            Vec2<float> _cellSize = {100.0f, 100.0f}; // cell size for x and y axis
            Vec2<size_t> _cellCounts = {0, 0}; // number of cells for x and y axis
            std::vector<Rect> _vCellAreas{}; // areas of children cell
            std::vector<std::shared_ptr<Node>> _vCellNodes{}; // children cell of the node
            std::vector<std::vector<OBJ_T>> _vCellObjects; // the objects belonging to the cell

            Node(Rect& r, Vec2<size_t> cellCounts) : _area(r)
            {
                _cellCounts = cellCounts;
                _cellSize.x = r.size.x / _cellCounts.x;
                _cellSize.y = r.size.y / _cellCounts.y;
                _vCellAreas.resize(_cellCounts.x * _cellCounts.y);
                _vCellObjects.resize(_cellCounts.x * _cellCounts.y);  

                for (size_t y = 0; y < _cellCounts.y; y++)
                {
                    for (size_t x = 0; x < _cellCounts.x; x++)
                    {
                        _vCellAreas[y*_cellCounts.x+x] = Rect(r.pos + Vec2<float>{x * _cellSize.x, y * _cellSize.y}, _cellSize);
                    }
                }
            }
        };

        std::shared_ptr<Node> _root;
        Rect _area = {{0.0f, 0.0f}, {100.0f, 100.0f}};
        Vec2<size_t> _cellCounts = {0, 0};

        void insert(std::shared_ptr<Node>& node, const OBJ_T& obj)
        {
            if (!node) node = std::make_shared<Node>(_area, _cellCounts);

            for (auto it=node->_vCellAreas.begin(); it!=node->_vCellAreas.end(); ++it)
            {
                if (it->contains(obj.GetArea()) || it->overlaps(obj.GetArea()))
                {
                    node->_vCellObjects[it-node->_vCellAreas.begin()].push_back(obj);
                }
            }
            return;
        }

        void search(std::shared_ptr<Node>& node, const Rect& r, std::list<OBJ_T>& result) const
        {
            for (auto it=node->_vCellAreas.begin(); it!=node->_vCellAreas.end(); ++it)
            {
                if (r.contains(*it))
                {
                    for (const auto& obj : node->_vCellObjects[it-node->_vCellAreas.begin()])
                    {
                        result.push_back(obj);
                    }
                }
                if (r.overlaps(*it))
                {
                    for (const auto& obj : node->_vCellObjects[it-node->_vCellAreas.begin()])
                    {
                        if (r.overlaps(obj.GetArea()) || r.contains(obj.GetArea()))
                            result.push_back(obj);
                    }
                }
            }
            return;
        }

        void print(const std::shared_ptr<Node>& node) const
        {
            
        }


    public:

        GridTree(): _root(nullptr) {};

        void SetArea(const Rect r, const Vec2<size_t> cellCounts = {10, 10})
        {
            _area = r;
            _cellCounts = cellCounts;
        }

        void insert(const OBJ_T& obj)
        {
            insert(_root, obj);
        }

        std::list<OBJ_T> search(const Rect& r)
        {
            std::list<OBJ_T> result;
            search(_root, r, result);
            return result;
        }

        size_t size() const 
        { 
            size_t s = 0;
            for (const auto& cell : _root->_vCellObjects)
            {
                s += cell.size();
            }
            return s;
        }
    
};


template <typename OBJ_T>
class KDTree 
{
    protected:
        // Node structure representing each point in the KDTree
        struct Node 
        {
            Rect _area;
            OBJ_T _object;
            std::shared_ptr<Node> _left;          
            std::shared_ptr<Node> _right;
            std::array<Rect, 2> _vRects;
            int _depth;
                    
            // Constructor to initialize a Node
            Node(const OBJ_T& ob, const Rect& r={{0.0f, 0.0f}, {100.0f, 100.0f}}, int d=0) : 
                _object(ob),
                _depth(d),
                _left(nullptr), 
                _right(nullptr),
                _area(r)
            {
                // Calculate the bounding rectangles for the left and right children
                if (_depth % 2 == 0)
                {
                    _vRects[0] = r.leftRect(ob.pos);
                    _vRects[1] = r.rightRect(ob.pos);
                }
                else
                {
                    _vRects[0] = r.lowerRect(ob.pos);
                    _vRects[1] = r.upperRect(ob.pos);
                }
            }
        };

        std::shared_ptr<Node> _root; // Root of the KDTree
        Rect _area = {{0.0f, 0.0f}, {100.0f, 100.0f}};

        // Recursive function to insert a point into the KDTree
        void insert(std::shared_ptr<Node>& node, const Rect& r, const OBJ_T& ob, int depth) 
        {
            
            // Base case: If node is null, create a new node
            if (node == nullptr) 
            {
                node =  std::make_shared<Node>(ob, r, depth);
                return;
            }
            

            // Calculate current dimension (cd)
            int cd = depth % 2;

            // Compare point with current node and decide to go left or right
            if (ob.pos[cd] < node->_object.pos[cd])
                insert(node->_left, node->_vRects[0], ob, depth + 1);
            else
                insert(node->_right, node->_vRects[1], ob, depth + 1);

            return;
        }

        void search(const std::shared_ptr<Node>& node, const Rect& r, std::list<OBJ_T>& results) 
        {
            // Base case: If node is null, the point is not found
            if (node == nullptr) return;

            // Calculate current dimension (cd)
            int cd = node->_depth % 2;

            if (r.overlaps(node->_object.GetArea()))
                results.push_back(node->_object);
                
            // Compare point with current node and decide to go left or right
            if (r.contains(node->_vRects[0]))
            {
                items(node->_left, results);
            }
                
            else if (r.overlaps(node->_vRects[0]))
                search(node->_left, r, results);
            if (r.contains(node->_vRects[1]))
                items(node->_right, results);
            else if (r.overlaps(node->_vRects[1]))
                search(node->_right, r, results);
    
        }

        void items(const std::shared_ptr<Node>& node, std::list<OBJ_T>& results)
        {
            // Base case: If node is null, return
            if (node == nullptr) return;

            // Add current node to the results list
            results.push_back(node->_object);

            // Recursively add items from left and right children
            items(node->_left, results);
            items(node->_right, results);
        }

        std::list<OBJ_T> items(const std::shared_ptr<Node>& node)
        {
            std::list<OBJ_T> results;
            items(node, results);
            return results;
        }

        // Recursive function to print the KDTree
        void print(const std::shared_ptr<Node>& node, const int depth) const {
            // Base case: If node is null, return
            if (node == nullptr) return;

            // Print current node with indentation based on depth
            for (int i = 0; i < depth; i++) std::cout << "  ";
            std::cout << "(";
            for (size_t i = 0; i < 2; i++) {
                std::cout << node->_object.GetArea();
                if (i < 1) std::cout << ", ";
            }
            std::cout << ")" << std::endl;

            // Recursively print left and right children
            print(node->_left, depth + 1);
            print(node->_right, depth + 1);
        }

        size_t size(const std::shared_ptr<Node>& node) const
        {
            size_t s = 0;
            if (node != nullptr)
                s += 1;
            if (node->_left!= nullptr)
                s += size(node->_left);
            if (node->_right!= nullptr)
                s += size(node->_right);
            return s;
        }
        

    public:
        // Constructor to initialize the KDTree with a null root
        KDTree() : _root(nullptr) {}

        void SetArea(const Rect r)
        {
            _area = r;
        }

        // Public function to insert a point into the KDTree
        void insert(const OBJ_T& ob) {
            insert(_root, _area, ob, 0);
        }

        // Public function to search for a point in the KDTree
        bool search(const Vec2<float>& point) const {
            return search(_root, point, 0);
        }

        // Public function to search for a point in the KDTree
        std::list<OBJ_T> search(const Rect& r) 
        {
            std::list<OBJ_T> result;
            search(_root, r, result);
            return result;
        }

        // Public function to print the KDTree
        void print() const {
            print(_root, 0);
        }

        size_t size() const
        {
            return size(_root);
        }
};


enum class UseTree
{
    LINEAR=0,
    QUADTREE,
    GRID,
    KDTREE,
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
        std::vector<CObject> _vObjects;
        StaticQuadTree<CObject> _staticQuadTree;
        GridTree<CObject> _gridTree;
        KDTree<CObject> _kdTree;

        UseTree _useMethod = UseTree::GRID; // option to use QuadTree

        bool onUserInit() override 
        {
            // initialize the tree
            _staticQuadTree.SetArea({{0.0f, 0.0f}, {areaLength, areaLength}});
            _gridTree.SetArea({{0.0f, 0.0f}, {areaLength, areaLength}}, {20, 20});
            _kdTree.SetArea({{0.0f, 0.0f}, {areaLength, areaLength}});
            
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
                _vObjects.push_back(obj);
                _staticQuadTree.insert(obj); // insert objects in quadtree
                _gridTree.insert(obj);
                _kdTree.insert(obj);
            }

            // Show some  information
            std::cout << "objs created: " << _vObjects.size() << std::endl;
            std::cout << "objs in QuadTree: " << _staticQuadTree.size() << std::endl;
            std::cout << "objs in GridTree: " << _gridTree.size() << std::endl;
            std::cout << "objs in KDTree: " << _kdTree.size() << std::endl;
  
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
                        case SDLK_TAB: _useMethod = (UseTree)(((int)_useMethod + 1) % 4); break; // add quadtree option
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

            switch(_useMethod)
            {
                case(UseTree::LINEAR):
                {
                    auto ticStart = std::chrono::system_clock::now();
                    for (const auto& obj : _vObjects)
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
                                    std::to_string(_vObjects.size()) + " Time: " + 
                                    std::to_string(ticDuration.count()) + " s";
                    DrawText(info, {10, 10}, TEXT_COLOR);
                    break;
                }
                case(UseTree::QUADTREE):
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
                                    std::to_string(_vObjects.size()) + " Time: " + 
                                    std::to_string(ticDuration.count()) + " s";
                    DrawText(info, {10, 10}, TEXT_COLOR);
                    break;
                }
                case(UseTree::GRID):
                {
                    auto ticStart = std::chrono::system_clock::now();
                    for (const auto& obj : _gridTree.search(screen))
                    {
                        DrawFilledCircle({(int)obj.pos.x, (int)obj.pos.y}, obj.r, obj.color);
                        count++;
                    }
                    std::chrono::duration<double> ticDuration = std::chrono::system_clock::now() - ticStart;
                    std::string info = "GRID: " + 
                                        std::to_string(count) + "/" + 
                                        std::to_string(_vObjects.size()) + " Time: " + 
                                        std::to_string(ticDuration.count()) + " s";
                    DrawText(info, {10, 10}, TEXT_COLOR);
                    break;
                }
                case(UseTree::KDTREE):
                {
                    auto ticStart = std::chrono::system_clock::now();
                    for (const auto& obj : _kdTree.search(screen))
                    {
                        DrawFilledCircle({(int)obj.pos.x, (int)obj.pos.y}, obj.r, obj.color);
                        count++;
                    }
                    std::chrono::duration<double> ticDuration = std::chrono::system_clock::now() - ticStart;
                    std::string info = "KDTree: " + 
                                        std::to_string(count) + "/" + 
                                        std::to_string(_vObjects.size()) + " Time: " + 
                                        std::to_string(ticDuration.count()) + " s";
                    DrawText(info, {10, 10}, TEXT_COLOR);
                    break;
                }
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