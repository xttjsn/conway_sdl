#ifndef CellMap_H

#define CellMap_H
#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <cstdint>

typedef int64_t Coord;
typedef uint64_t Len;
typedef uint8_t CellState;
constexpr uint8_t kCellAliveMask = 1;
constexpr uint8_t kCellNeighborCountMask = 0b11110;
// Maximum number of cells in a tree node
constexpr int kNodeCapacity = 4;

class XY;

class RGBA {
public:
    RGBA(unsigned int ir, unsigned int ig, unsigned int ib, unsigned int ia)
        : r(ir), g(ig), b(ib), a(ia) {}

    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int a;
};

const RGBA kOnColor(255, 200, 255, 255);
const RGBA kOffColor(0, 0, 0, 0);

inline bool GetCellAliveness(CellState& state) {
    return (bool) (state & kCellAliveMask);
}

inline uint8_t GetCellNeighborCount(CellState& state) {
    return (state & kCellNeighborCountMask) >> 1;
}

inline void SetCellAliveness(CellState& state, bool alive) {
    state = (state & ~(1)) | (alive);
}

inline void UpdateCellAliveness(CellState& state) {
    SetCellAliveness(state,
                     (GetCellNeighborCount(state) == 3) ||
                     (GetCellNeighborCount(state) == 2 && GetCellAliveness(state)));
}

inline void ClearCellNeighborCount(CellState& state) {
    state = state & 1;
}

inline void UpdateCellNeighborCount(CellState& state, int8_t by) {
    state = (((state >> 1) + by) << 1) | (state & 1);
}

class XY {
public:
    XY(Coord ix, Coord iy)
        : x(ix), y(iy) {}
    Coord x;
    Coord y;

    bool operator==(const XY& other) const {
        return x == other.x && y == other.y;
    }
};


template<>
struct std::hash<XY>
{
    std::size_t operator()(XY const& xy) const noexcept
    {
        std::size_t result = 0;
        // long should be the same as int64_t
        result ^= std::hash<long>()(xy.x) + 0x9e3779b9 + (result << 6) + (result >> 2);
        result ^= std::hash<long>()(xy.y) + 0x9e3779b9 + (result << 6) + (result >> 2);
        return result;
    }
};


class Cell {
public:
    Cell(XY ixy, CellState istate)
        : xy(ixy), state(istate) {}
    XY xy;
    CellState state;
};

// Axis-Aligned Bounding Box
class AABB {
public:
    AABB(XY icenter, Coord ileft, Coord iright, Coord itop, Coord ibottom)
        : center(icenter), left(ileft), right(iright), top(itop), bottom(ibottom) {
        if (left > right || top > bottom) {
            throw std::runtime_error("Invalid AABB");
        }
    }

    XY center;

    // Inclusive boundaries
    Coord left;
    Coord right;
    Coord top;
    Coord bottom;

    bool contains(const XY& xy) const;
    bool intersect(const AABB& other) const;
};

// Quad Tree
typedef std::shared_ptr<Cell> CellRef;

class CellTreeNode;
typedef std::shared_ptr<CellTreeNode> CellTreeNodeRef;
typedef std::unique_ptr<CellTreeNode> CellTreeNodeUniq;
class CellTreeNode {
public:
    CellTreeNode(AABB ibbox, bool root = false);

    static CellTreeNodeRef createRoot();
    bool insert(CellRef cell);
    bool remove(CellRef cell);
    void subdivide();
    void merge();
    void query(const AABB& range, std::vector<CellRef>& output);
    void update();
    void print(std::ostream& output);
    size_t cellCount();

    AABB m_bbox;
    std::unordered_set<CellRef> m_cells;

    // Only root has m_cells_map populated for quick reference
    bool m_root = false;
    std::unordered_map<XY, CellRef> m_cells_map;

    // Children
    CellTreeNodeUniq m_nw = nullptr;
    CellTreeNodeUniq m_ne = nullptr;
    CellTreeNodeUniq m_sw = nullptr;
    CellTreeNodeUniq m_se = nullptr;
};

class CellMap {
public:
    CellMap(SDL_Surface* surface, int hpixels, int vpixels, int cellSize, int printAtIteration);

    void update();
    void drawCurrent();
    void move(const XY& xy);
    inline void addCell(const XY& xy) {
        m_celltree->insert(std::make_shared<Cell>(xy, 1));
    }

private:
    void drawCell(XY xy, RGBA color);
    void clearSurface();

    std::pair<XY, bool> worldXY2WindowXY(XY worldXY);

    SDL_Surface* m_surface;
    CellTreeNodeRef m_celltree;

    int m_pixelsPerCell;

    int m_hpixels;
    int m_vpixels;

    int m_hcells;
    int m_vcells;

    AABB m_queryBox;

    // Horizontal and vertical offset of the window,
    // (i.e. the coordinate of the left top cell)
    Coord m_hoff;
    Coord m_voff;

    int m_printAtIteration;

    int m_iteration;
};



#endif
