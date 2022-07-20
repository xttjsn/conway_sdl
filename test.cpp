#include <gtest/gtest.h>
#include <limits>
#include "cellmap.h"
static Coord MAX = std::numeric_limits<Coord>::max();
static Coord MIN = std::numeric_limits<Coord>::min();


TEST(CellStateManipulation, Aliveness) {
    CellState state = 0;
    EXPECT_FALSE(GetCellAliveness(state));

    state = 1;
    EXPECT_TRUE(GetCellAliveness(state));

    state = 0b01111;
    EXPECT_TRUE(GetCellAliveness(state));

    state = 0b01110;
    EXPECT_FALSE(GetCellAliveness(state));

    SetCellAliveness(state, true);
    EXPECT_TRUE(GetCellAliveness(state));

    state = 0b01011;
    SetCellAliveness(state, false);
    EXPECT_FALSE(GetCellAliveness(state));

    state = 0b00110;
    UpdateCellAliveness(state);
    EXPECT_TRUE(GetCellAliveness(state));
}

TEST(CellStateManipulation, NeighborCount) {
    CellState state = 0;
    EXPECT_EQ(GetCellNeighborCount(state), 0);

    state = 0b01111;
    EXPECT_EQ(GetCellNeighborCount(state), 7);

    state = 0b01110;
    EXPECT_EQ(GetCellNeighborCount(state), 7);

    state = 0b10000;
    EXPECT_EQ(GetCellNeighborCount(state), 8);

    UpdateCellNeighborCount(state, -1);
    EXPECT_EQ(GetCellNeighborCount(state), 7);
    UpdateCellNeighborCount(state, -1);
    EXPECT_EQ(GetCellNeighborCount(state), 6);
    UpdateCellNeighborCount(state, -1);
    EXPECT_EQ(GetCellNeighborCount(state), 5);
    UpdateCellNeighborCount(state, -1);
    EXPECT_EQ(GetCellNeighborCount(state), 4);
    UpdateCellNeighborCount(state, -1);
    EXPECT_EQ(GetCellNeighborCount(state), 3);
    UpdateCellNeighborCount(state, -1);
    EXPECT_EQ(GetCellNeighborCount(state), 2);
    UpdateCellNeighborCount(state, -1);
    EXPECT_EQ(GetCellNeighborCount(state), 1);
    UpdateCellNeighborCount(state, -1);
    EXPECT_EQ(GetCellNeighborCount(state), 0);
    UpdateCellNeighborCount(state, 1);
    EXPECT_EQ(GetCellNeighborCount(state), 1);
    UpdateCellNeighborCount(state, 1);
    EXPECT_EQ(GetCellNeighborCount(state), 2);
    UpdateCellNeighborCount(state, 1);
    EXPECT_EQ(GetCellNeighborCount(state), 3);
    UpdateCellNeighborCount(state, 1);
    EXPECT_EQ(GetCellNeighborCount(state), 4);
    UpdateCellNeighborCount(state, 1);
    EXPECT_EQ(GetCellNeighborCount(state), 5);
    UpdateCellNeighborCount(state, 1);
    EXPECT_EQ(GetCellNeighborCount(state), 6);
    UpdateCellNeighborCount(state, 1);
    EXPECT_EQ(GetCellNeighborCount(state), 7);
    UpdateCellNeighborCount(state, 1);
    EXPECT_EQ(GetCellNeighborCount(state), 8);

    ClearCellNeighborCount(state);
    EXPECT_EQ(GetCellNeighborCount(state), 0);
}

TEST(AABBQueries, Contain) {
    {
        AABB bb(XY(0,0), -10, 10, -10, 10);
        EXPECT_TRUE(bb.contains(XY(0,0)));
        EXPECT_TRUE(bb.contains(XY(-10, -10)));
        EXPECT_TRUE(bb.contains(XY(10, -10)));
        EXPECT_TRUE(bb.contains(XY(10, 10)));
        EXPECT_TRUE(bb.contains(XY(-10, 10)));
        EXPECT_TRUE(bb.contains(XY(5, 5)));
        EXPECT_FALSE(bb.contains(XY(10, -11)));
        EXPECT_FALSE(bb.contains(XY(11, 0)));
    }
    {
        AABB bb(XY(((Coord)1) << 16, -(((Coord)1) << 32)), -(((Coord)1) << 16), ((Coord)1)<<16, -(((Coord)1) << 63), 0);
        EXPECT_TRUE(bb.contains(XY(0, 0)));
        EXPECT_FALSE(bb.contains(XY(0, 1)));
        EXPECT_TRUE(bb.contains(XY(10000, -100000000)));
    }
}

TEST(AABBQueries, Intersect) {
    {
        AABB aa(XY(0,0), -10, 10, -10, 10);
        AABB bb(XY(10, 10), 10, 100, -100, 10);
        EXPECT_TRUE(bb.intersect(aa));
        EXPECT_TRUE(aa.intersect(bb));
    }
}

TEST(CellTree, BigInts) {
    Coord min = std::numeric_limits<Coord>::min();
    Coord max = std::numeric_limits<Coord>::max();
}

TEST(CellTree, Construction) {
    CellTreeNodeRef root = CellTreeNode::createRoot();
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(((Coord)1) << 40, -(((Coord)1) << 40)), 0)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(((Coord)1) << 32, -(((Coord)1) << 32)), 0)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(-(((Coord)1) << 32), -(((Coord)1) << 32)), 0)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(-(((Coord)1) << 33), -(((Coord)1) << 32)), 0)));

    EXPECT_EQ(root->m_cells.size(), 4);
    EXPECT_EQ(root->m_cells_map.size(), 4);
    EXPECT_EQ(root->m_nw, nullptr);

    CellRef origin = std::make_shared<Cell>(XY(0, 0), 0);
    EXPECT_TRUE(root->insert(origin));
    EXPECT_EQ(root->m_cells.size(), 0);
    EXPECT_EQ(root->m_cells_map.size(), 5);

    AABB query(XY(0, 0), -10000, 10000, -10000, 10000);
    std::vector<CellRef> cells;
    root->query(query, cells);
    EXPECT_EQ(cells.size(), 1);
    EXPECT_EQ(cells[0].get(), origin.get());

    EXPECT_EQ(root->m_nw->cellCount(), 3);
    EXPECT_EQ(root->m_ne->cellCount(), 2);
    EXPECT_EQ(root->m_sw->cellCount(), 0);
    EXPECT_EQ(root->m_se->cellCount(), 0);
    EXPECT_EQ(root->cellCount(), 5);

    EXPECT_THROW({
            try
            {
                CellRef other = std::make_shared<Cell>(XY(1, 1), 0);
                root->remove(other);
            }
            catch( const std::runtime_error& e )
            {
                // and this tests that it has the correct message
                EXPECT_STREQ( "Unable to remove cell from a node, check the algorithm", e.what() );
                throw;
            }
        }, std::runtime_error);

    EXPECT_TRUE(root->remove(origin));

    EXPECT_EQ(root->m_nw, nullptr);
}

TEST(CellTree, Update) {
    CellTreeNodeRef root = CellTreeNode::createRoot();
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(0, 0), 0)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(100, 100), 0)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(200, 200), 0)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(202, 202), 0)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(204, 204), 0)));
    EXPECT_FALSE(root->m_nw == nullptr);
    EXPECT_EQ(root->m_cells.size(), 0);
    EXPECT_EQ(root->m_cells_map.size(), 5);

    root->update();

    EXPECT_EQ(root->cellCount(), 0);
    EXPECT_EQ(root->m_nw, nullptr);
    EXPECT_EQ(root->m_cells.size(), 0);
    EXPECT_EQ(root->m_cells_map.size(), 0);

    root->update();

    EXPECT_EQ(root->cellCount(), 0);
    EXPECT_EQ(root->m_nw, nullptr);
    EXPECT_EQ(root->m_cells.size(), 0);
    EXPECT_EQ(root->m_cells_map.size(), 0);

    Coord MAX_M1 = MAX - 1;
    Coord MIN_P1 = MIN + 1;
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(MAX_M1, MIN_P1), 0)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(MAX, MIN_P1), 0)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(MAX, MIN), 0)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(0, 0), 0)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(1, 0), 0)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(1, 1), 0)));

    root->update();

    EXPECT_EQ(root->cellCount(), 2);
    EXPECT_EQ(root->m_cells_map.size(), 2);
    EXPECT_EQ(root->m_cells.size(), 2);

    // MAX_M1, MIN
    // 0, 1
    EXPECT_TRUE(root->m_cells_map.find(XY(MAX_M1, MIN)) != root->m_cells_map.end());
    EXPECT_TRUE(root->m_cells_map.find(XY(0, 1)) != root->m_cells_map.end());

    root->update();
    root->print(std::cout);

    root->update();
    root->print(std::cout);
    EXPECT_EQ(root->cellCount(), 0);
}

TEST(CellTree, Oscillator) {
    CellTreeNodeRef root = CellTreeNode::createRoot();
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(-1, 0), 1)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(0, 0), 1)));
    EXPECT_TRUE(root->insert(std::make_shared<Cell>(XY(1, 0), 1)));

    root->update();

    EXPECT_TRUE(root->m_cells_map.find(XY(0, 1)) != root->m_cells_map.end());
    EXPECT_TRUE(root->m_cells_map.find(XY(0, 0)) != root->m_cells_map.end());
    EXPECT_TRUE(root->m_cells_map.find(XY(0, -1)) != root->m_cells_map.end());
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
