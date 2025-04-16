#include "catch.hpp"
#include "include/VoxelChunk.h"

TEST_CASE("VoxelChunk generates terrain correctly", "[VoxelChunk]") {
    VoxelChunk chunk(16);
    chunk.generateTerrain();

    // Verify that terrain is generated (e.g., non-zero voxels exist)
    bool hasSolidVoxels = false;
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            for (int y = 0; y < 16; ++y) {
                if (chunk.getVoxel(x, y, z) != 0) {
                    hasSolidVoxels = true;
                    break;
                }
            }
        }
    }

    REQUIRE(hasSolidVoxels);
}