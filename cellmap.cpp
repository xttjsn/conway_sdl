#include "cellmap.h"
#include <limits>
#include <iostream>
#include <unordered_set>

static Coord MAX = std::numeric_limits<Coord>::max();
static Coord MIN = std::numeric_limits<Coord>::min();

#define big_int_distance(x, y) \
    ((x > y) ? ((unsigned long)x - (unsigned long)y) : ((unsigned long)y - (unsigned long)x))

#define big_int_average(a, b) \
    ((a / 2) + (b / 2) + (((a % 2) + (b % 2)) / 2))

inline Coord big_int_addition(Coord a, Coord b) {
#ifdef Windows
    if (b > 0 && a > MAX - b) {
        // Will overflow, wrap around
        return (a + MIN) + (b + MIN);
    }
    else if (b < 0 && a < MIN - b) {
        // Will underflow, wrap around
        return (a - MIN) + (b - MIN);
    }
    return a + b;
#else
    int64_t res;
    __builtin_add_overflow(a, b, &res);
    return res;
#endif
}

inline Coord big_int_subtraction(Coord a, Coord b) {
#ifdef Windows
    return big_int_addition(a, -b);
#else
    int64_t res;
    __builtin_sub_overflow(a, b, &res);
    return res;
#endif
}


bool AABB::contains(const XY& xy) const {
#if DEBUG
    std::cout << "left=" << left << ", right=" << right << ", top=" << top << ", bottom=" << bottom << std::endl;
    std::cout << "sizeof(top)=" << sizeof(top) << std::endl;
#endif
    return xy.x <= right
        && xy.x >= left
        && xy.y >= top
        && xy.y <= bottom;
}

bool AABB::intersect(const AABB& other) const {
    return !(
        left > other.right ||
        right < other.left ||
        top > other.bottom ||
        bottom < other.top
        );
}


void clearSurface(void* pixels, int len) {
    Uint8* pixel_ptr = (Uint8*)pixels;
    memset(pixel_ptr, 0, len);
}


CellMap::CellMap(SDL_Surface* surface, int hpixels, int vpixels, int pixelsPerCell, int printAtIteration)
    : m_surface(surface),
      m_celltree(nullptr),
      m_pixelsPerCell(pixelsPerCell),
      m_hpixels(hpixels),
      m_vpixels(vpixels),
      m_hcells(hpixels / pixelsPerCell),
      m_vcells(vpixels / pixelsPerCell),
      m_hoff(-(hpixels / pixelsPerCell / 2)),
      m_voff(-(vpixels / pixelsPerCell / 2)),
      m_queryBox(XY(0, 0), -m_hcells/2, m_hcells/2, -m_vcells/2, m_vcells/2),
      m_printAtIteration(printAtIteration) {
    m_celltree = CellTreeNode::createRoot();
    // m_queryBox = AABB();        //
}

void CellMap::drawCell(XY xy, RGBA color) {
    uint8_t* pixel_ptr = (uint8_t*)m_surface->pixels + (xy.y * m_pixelsPerCell * m_hpixels + xy.x * m_pixelsPerCell) * 4;

	for (unsigned int i = 0; i < m_pixelsPerCell; i++)
	{
		for (unsigned int j = 0; j < m_pixelsPerCell; j++)
		{
			*(pixel_ptr + j * 4) = color.r;
			*(pixel_ptr + j * 4 + 1) = color.g;
			*(pixel_ptr + j * 4 + 2) = color.b;
		}
		pixel_ptr += m_hpixels * 4;
	}
}

void CellMap::clearSurface() {
    uint8_t* pixel_ptr = (uint8_t*)m_surface->pixels;
    memset(pixel_ptr, 0, m_hpixels * m_vpixels * 4);
}

std::pair<XY, bool> CellMap::worldXY2WindowXY(XY worldXY) {
    Coord x = big_int_subtraction(worldXY.x, m_hoff);
    Coord y = big_int_subtraction(worldXY.y, m_voff);
    if (x < m_hcells && x >= 0 && y < m_vcells && y >= 0) {
        return std::make_pair(XY(x, y), true);
    } else {
        return std::make_pair(XY(0, 0), false);
    }
}

void CellMap::move(const XY& xy) {
    m_queryBox.left = big_int_addition(m_queryBox.left, xy.x);
    m_queryBox.right = big_int_addition(m_queryBox.right, xy.x);
    m_queryBox.top = big_int_addition(m_queryBox.top, xy.y);
    m_queryBox.bottom = big_int_addition(m_queryBox.bottom, xy.y);
    m_queryBox.center.x = big_int_average(m_queryBox.left, m_queryBox.right);
    m_queryBox.center.y = big_int_average(m_queryBox.top, m_queryBox.bottom);

    m_hoff = big_int_addition(m_hoff, xy.x);
    m_voff = big_int_addition(m_voff, xy.y);
}

void CellMap::update() {
    // Query
    std::vector<CellRef> cells;
    m_celltree->query(m_queryBox, cells);

    // Clear
    this->clearSurface();

    // Draw
    for (auto& cell : cells) {
        auto result = this->worldXY2WindowXY(cell->xy);
        if (result.second) {
            this->drawCell(result.first, kOnColor);
        }
    }
    if (m_iteration == m_printAtIteration) {
        m_celltree->print(std::cout);
    }

    // Update celltree according to the rules
    m_celltree->update();

    m_iteration++;
}

CellTreeNode::CellTreeNode(AABB ibbox, bool root)
    : m_bbox(ibbox), m_root(root) {}

CellTreeNodeRef CellTreeNode::createRoot() {
    AABB rootbb = AABB(XY(0,0), MIN, MAX, MIN, MAX);
#if DEBUG
    std::cout << "Root min: " << MIN << ", max:" << MAX << std::endl;
#endif
    return std::make_shared<CellTreeNode>(rootbb, true);
}

bool CellTreeNode::insert(CellRef cell) {
    // Insert a new cell

    if (!m_bbox.contains(cell->xy)) {
        // Not within bounds
        return false;
    }

    // If we cannot subdivide anymore
    bool insert = false;
    if ((big_int_distance(m_bbox.bottom, m_bbox.top)) < 2 || (big_int_distance(m_bbox.right, m_bbox.left)) < 2) {
#if DEBUG
        std::cout << "Cannot subdivide anymore\n";
#endif
        insert = true;
    }
    // If still has room for one more cell and hasn't subdivided yet
    else if (m_cells.size() < kNodeCapacity && m_nw == nullptr) {
#if DEBUG
        std::cout << "Still has room and hasn't subdivided yet \n";
#endif
        insert = true;
    }

    if (insert) {
        auto result = m_cells.insert(cell);
        if (!result.second) {
            throw std::runtime_error("Unable to insert cell to node");
        }
        if (m_root) {
            auto result = m_cells_map.insert(std::make_pair(cell->xy, cell));
            if (!result.second) {
                throw std::runtime_error("Unable to insert cell to root node");
            }
        }
        return true;
    }

    if (m_nw == nullptr)
        subdivide();

    // Before inserting this cell, inserting all existing cells to the new children
    for (auto& old_cell : m_cells) {
        if (!m_nw->insert(old_cell)
            && !m_ne->insert(old_cell)
            && !m_sw->insert(old_cell)
            && !m_se->insert(old_cell)) {
            throw std::runtime_error("Unable to insert old cells to children");
        }
    }
    m_cells.clear();

    // Insert the new cell
    if (m_nw->insert(cell)
        || m_ne->insert(cell)
        || m_sw->insert(cell)
        || m_se->insert(cell)) {
        if (m_root){
            auto result = m_cells_map.insert(std::make_pair(cell->xy, cell));
            if (!result.second) {
                throw std::runtime_error("Unable to insert cell to root node");
            }
        }
        return true;
    } else {
        // Should not reach here
        throw std::runtime_error("Unable to insert a cell");
    }

    return false;
}

void CellTreeNode::subdivide() {
    if (m_nw == nullptr) {
        {
            // North West is favored
            Coord left = m_bbox.left;
            Coord right = m_bbox.center.x;
            Coord top = m_bbox.top;
            Coord bottom = m_bbox.center.y;
            XY center = XY(big_int_average(left, right), big_int_average(top, bottom));
            m_nw = std::make_unique<CellTreeNode>(AABB(center, left, right, top, bottom));
        }
        {
            // North East
            Coord left = big_int_addition(m_bbox.center.x, 1);
            Coord right = m_bbox.right;
            Coord top = m_bbox.top;
            Coord bottom = m_bbox.center.y;
            XY center = XY(big_int_average(left, right), big_int_average(top, bottom));
            m_ne = std::make_unique<CellTreeNode>(AABB(center, left, right, top, bottom));
        }
        {
            // South West
            Coord left = m_bbox.left;
            Coord right = m_bbox.center.x;
            Coord top = big_int_addition(m_bbox.center.y, 1);
            Coord bottom = m_bbox.bottom;
            XY center = XY(big_int_average(left, right), big_int_average(top, bottom));
            m_sw = std::make_unique<CellTreeNode>(AABB(center, left, right, top, bottom));
        }
        {
            // South East
            Coord left = big_int_addition(m_bbox.center.x, 1);
            Coord right = m_bbox.right;
            Coord top = big_int_addition(m_bbox.center.y, 1);
            Coord bottom = m_bbox.bottom;
            XY center = XY(big_int_average(left, right), big_int_average(top, bottom));
            m_se = std::make_unique<CellTreeNode>(AABB(center, left, right, top, bottom));
        }
    }
}

size_t CellTreeNode::cellCount() {
    if (m_nw) {
        return m_nw->cellCount()
            + m_ne->cellCount()
            + m_sw->cellCount()
            + m_se->cellCount();
    } else {
        return m_cells.size();
    }
}

bool CellTreeNode::remove(CellRef cell) {
    if (m_nw) {
        // Subdivided, i.e. not a leaf node
        if (!m_nw->remove(cell)
            && !m_ne->remove(cell)
            && !m_sw->remove(cell)
            && !m_se->remove(cell)) {
            return false;
        }

        // Merge
        if ((m_nw->cellCount()
             + m_ne->cellCount()
             + m_sw->cellCount()
             + m_se->cellCount()) <= kNodeCapacity) {
            merge();
        }

        return true;

    } else {
        if (!m_bbox.contains(cell->xy)) {
            return false;
        }
        auto result = m_cells.erase(cell);
        if (result != 1) {
            throw std::runtime_error("Unable to remove cell from a node, check the algorithm");
        }
        return true;
    }

    // Never reaches here
    return false;
}

void CellTreeNode::merge() {
    CellTreeNode* children[4] = {m_nw.get(), m_ne.get(), m_sw.get(), m_se.get()};
    for (auto& child : children) {
        for (auto& cell : child->m_cells) {
            auto result = m_cells.insert(cell);
            if (!result.second) {
                throw std::runtime_error("Unable to transfer cells from child to parent");
            }
        }
    }

    m_nw = nullptr;
    m_se = nullptr;
    m_sw = nullptr;
    m_se = nullptr;
}

void CellTreeNode::update() {
    // Note: you should performs update on root

    // If an "alive" cell had less than 2 or more than 3 alive
    // neighbors (in any of the 8 surrounding cells), it becomes dead.
    // If a "dead" cell had *exactly* 3 alive neighbors, it becomes
    // alive.

    std::unordered_map<XY, CellRef> newmap;
    constexpr CellState initialState = 0;

    // 1. Clear counts for all cells
    for (auto it = m_cells_map.begin(); it != m_cells_map.end(); it++) {
        ClearCellNeighborCount(it->second->state);
    }

    // 2. Calculate contribution
    for (auto it = m_cells_map.begin(); it != m_cells_map.end(); it++) {
        auto xy = it->first;
        auto cell = it->second;

        // Contribute to neighbor XYs
        XY neighbors[8] = {
            XY(big_int_addition(xy.x, 1), xy.y),
            XY(xy.x, big_int_addition(xy.y, 1)),
            XY(big_int_addition(xy.x, -1), xy.y),
            XY(xy.x, big_int_addition(xy.y, -1)),
            XY(big_int_addition(xy.x, 1), big_int_addition(xy.y, 1)),
            XY(big_int_addition(xy.x, 1), big_int_addition(xy.y, -1)),
            XY(big_int_addition(xy.x, -1), big_int_addition(xy.y, -1)),
            XY(big_int_addition(xy.x, -1), big_int_addition(xy.y, 1))
        };

        // For each neighbor, calculate and record its contribution
        for (auto& neighbor : neighbors) {
            auto it = m_cells_map.find(neighbor);
            if (it == m_cells_map.end()) {
                // Try the new map
                it = newmap.find(neighbor);

                if (it == newmap.end()) {
                    // Create a new cell
                    CellRef newcell = std::make_shared<Cell>(neighbor, initialState);

                    // Insert into maps
                    auto result = newmap.insert(std::make_pair(neighbor, newcell));

                    if (!result.second) {
                        // Insertion failed, we should panic
                        std::cerr << "Panic: new Cell insertion into maps failed\n";
                        std::abort();
                    }

                    it = result.first;
                }
            }

            // Inrease neighbor count by 1
            UpdateCellNeighborCount(it->second->state, 1);
        }
    }

    // 3. Prune dead cells
    // 3.1 Remove cell from tree
    std::vector<XY> pendingRemoveXYs;
    for (auto it = m_cells_map.begin(); it != m_cells_map.end(); it++) {
        UpdateCellAliveness(it->second->state);
        if (!GetCellAliveness(it->second->state)) {
            this->remove(it->second);
            pendingRemoveXYs.push_back(it->first);
        }
    }

    // 3.2 Remove cell from map
    for (auto& xy : pendingRemoveXYs) {
        size_t result = m_cells_map.erase(xy);
        if (result != 1) {
            std::cerr << "Panic: Dead cells not removed!\n";
            std::abort();
        }
    }


    // 4. Add new cells
    for (auto it = newmap.begin(); it != newmap.end(); it++) {
        UpdateCellAliveness(it->second->state);
        if (GetCellAliveness(it->second->state)) {
            if (!this->insert(it->second)) {
                std::cerr << "Panic: new cells not inserted in CellTree\n";
                std::abort();
            }
        }
    }
}

void CellTreeNode::query(const AABB& range, std::vector<CellRef>& output) {
    if (!m_bbox.intersect(range)) {
        return;
    }

    if (m_nw) {
        // Has children
        m_nw->query(range, output);
        m_ne->query(range, output);
        m_sw->query(range, output);
        m_se->query(range, output);
    } else {
        // No children, leaf nodes
        for (auto& cell : m_cells) {
            if (range.contains(cell->xy)) {
                output.push_back(cell);
            }
        }
    }
}

void CellTreeNode::print(std::ostream& output) {
    if (m_root) {
        output << "#Life 1.06\n";
    }
    if (m_nw) {
        // Has children
        m_nw->print(output);
        m_ne->print(output);
        m_sw->print(output);
        m_se->print(output);
    } else {
        for (auto& cell : m_cells) {
            output << cell->xy.x << " " << cell->xy.y << std::endl;
        }
    }
}
