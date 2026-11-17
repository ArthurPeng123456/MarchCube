/*
    Tables and conventions from
    http://paulbourke.net/geometry/polygonise/
*/

#include <iostream>
#include <stdexcept>
#include "marching_cubes.hpp"

int MarchingCubes::calculate_cube_index(GridCell &cell, float isovalue)
{
    int cubeIndex = 0;
    for (int i = 0; i < 8; i++)
        if (cell.value[i] < isovalue) cubeIndex |= (1 << i);
    return cubeIndex;
}

std::vector<Point> MarchingCubes::get_intersection_coordinates(GridCell &cell, float isovalue)
{
    std::vector<Point> intersections (12);

    int cubeIndex = calculate_cube_index(cell, isovalue);
    int intersectionsKey = edgeTable[cubeIndex];

    int idx = 0;
    while (intersectionsKey)
    {
        if (intersectionsKey&1)
        {
            int v1 = edgeToVertices[idx].first, v2 = edgeToVertices[idx].second;
            Point intersectionPoint = interpolate(cell.vertex[v1], cell.value[v1],
                                                    cell.vertex[v2], cell.value[v2], isovalue);
            intersections[idx] = intersectionPoint;
        }
        idx++;
        intersectionsKey >>= 1;
    }


    return intersections;
}

Point MarchingCubes::interpolate(Point& v1, float val1, Point& v2, float val2, float isovalue)
{
    Point interpolated;
    float mu = (isovalue - val1) / (val2 - val1);

    interpolated.x = mu*(v2.x - v1.x) + v1.x;
    interpolated.y = mu*(v2.y - v1.y) + v1.y;
    interpolated.z = mu*(v2.z - v1.z) + v1.z;

    return interpolated;
}

std::vector<std::vector<Point>> MarchingCubes::get_triangles(std::vector<Point>& intersections, int cubeIndex)
{
    std::vector<std::vector<Point>> triangles;
    for (int i = 0; triangleTable[cubeIndex][i] != -1; i += 3)
    {
        std::vector<Point> triangle (3);
        for (int j = 0; j < 3; j++)
            triangle[j] = intersections[triangleTable[cubeIndex][i + j]];
        triangles.push_back(triangle);
    }

    return triangles;
}

void MarchingCubes::print_triangles(std::vector<std::vector<Point>> triangles)
{
    for (int i = 0; i < (int)triangles.size(); i++)
    {
        for (int j = 0; j < 3; j++)
            std::cout << triangles[i][j].x << ",\t" << triangles[i][j].y << ",\t" << triangles[i][j].z << "\n";
        std::cout << "\n";
    }
}

std::vector<std::vector<Point>> MarchingCubes::triangulate_cell(GridCell &cell, float isovalue)
{
    int cubeIndex = calculate_cube_index(cell, isovalue);
    std::vector<Point> intersections = get_intersection_coordinates(cell, isovalue);
    std::vector<std::vector<Point>> triangles = get_triangles(intersections, cubeIndex);

    // int flag = 0;
    // for (auto& triangle: triangles)
    //     for (int i = 0; i < 3; i++)
    //         for (int j = i + 1; j < 3; j++)
    //             if (triangle[i].x == triangle[j].x && triangle[i].y == triangle[j].y && triangle[i].z == triangle[j].z)
    //                 flag++;

    // if (flag)
    // {
    //     print_triangles(triangles);
    //     for (int i = 0; i < 8; i++)
    //         std::cout << cell.vertex[i].x << ", " << cell.vertex[i].y << ", " << cell.vertex[i].z << ": " << cell.value[i] << "\n";
    //     std::cout << "\n";
    // }

    return triangles;
}

std::vector<std::vector<Point>> MarchingCubes::triangulate_field(
    const std::vector<std::vector<std::vector<float>>>& scalarFunction,
    float isovalue,
    Point origin,
    Point spacing,
    std::vector<std::vector<std::uint32_t>>* cellTriangleIndices)
{
    const std::size_t sizeX = scalarFunction.size();
    if (sizeX < 2 || scalarFunction[0].size() < 2 || scalarFunction[0][0].size() < 2)
        throw std::invalid_argument("Scalar field must contain at least two samples on each axis");

    const std::size_t sizeY = scalarFunction[0].size();
    const std::size_t sizeZ = scalarFunction[0][0].size();
    for (const auto& plane : scalarFunction) {
        if (plane.size() != sizeY)
            throw std::invalid_argument("Scalar field must have a rectangular layout");
        for (const auto& row : plane) {
            if (row.size() != sizeZ)
                throw std::invalid_argument("Scalar field must have a rectangular layout");
        }
    }

    std::vector<std::vector<Point>> triangles;
    if (cellTriangleIndices != nullptr) {
        cellTriangleIndices->clear();
        cellTriangleIndices->resize((sizeX - 1) * (sizeY - 1) * (sizeZ - 1));
    }

    for (std::size_t i = 0; i + 1 < sizeX; i++)
    {
        for (std::size_t j = 0; j + 1 < sizeY; j++)
        {
            for (std::size_t k = 0; k + 1 < sizeZ; k++)
            {
                const float x = origin.x + static_cast<float>(i) * spacing.x;
                const float y = origin.y + static_cast<float>(j) * spacing.y;
                const float z = origin.z + static_cast<float>(k) * spacing.z;
                const float xNext = x + spacing.x;
                const float yNext = y + spacing.y;
                const float zNext = z + spacing.z;
                // cell ordered according to convention in referenced website
                GridCell cell = 
                {
                    {
                        {x, y, z}, {xNext, y, z},
                        {xNext, y, zNext}, {x, y, zNext},
                        {x, yNext, z}, {xNext, yNext, z},
                        {xNext, yNext, zNext}, {x, yNext, zNext}
                    },
                    {
                        scalarFunction[i][j][k], scalarFunction[i+1][j][k],
                        scalarFunction[i+1][j][k+1], scalarFunction[i][j][k+1],
                        scalarFunction[i][j+1][k], scalarFunction[i+1][j+1][k],
                        scalarFunction[i+1][j+1][k+1], scalarFunction[i][j+1][k+1]
                    }
                };
                std::vector<std::vector<Point>> cellTriangles = triangulate_cell(cell, isovalue);
                const std::size_t cellId = (sizeX - 1) * (sizeY - 1) * k +
                                           (sizeX - 1) * j + i;
                for (std::size_t triangleInCell = 0; triangleInCell < cellTriangles.size(); ++triangleInCell) {
                    if (cellTriangleIndices != nullptr) {
                        (*cellTriangleIndices)[cellId].push_back(
                            static_cast<std::uint32_t>(triangles.size()));
                    }
                    triangles.push_back(cellTriangles[triangleInCell]);
                }
            }
        }
    }
    return triangles;
}

std::vector<std::vector<Point>> MarchingCubes::triangulate_grid(
    std::size_t cellsX,
    std::size_t cellsY,
    std::size_t cellsZ,
    Point origin,
    Point spacing,
    float isovalue,
    const std::function<float(float, float, float)>& sample,
    std::vector<std::vector<std::uint32_t>>* cellTriangleIndices)
{
    if (cellsX == 0 || cellsY == 0 || cellsZ == 0)
        throw std::invalid_argument("Grid must contain at least one cell on each axis");
    if (!sample)
        throw std::invalid_argument("Scalar field sampler must be callable");
    if (spacing.x == 0.0f || spacing.y == 0.0f || spacing.z == 0.0f)
        throw std::invalid_argument("Grid spacing must be non-zero on each axis");

    std::vector<std::vector<std::vector<float>>> field(
        cellsX + 1,
        std::vector<std::vector<float>>(
            cellsY + 1,
            std::vector<float>(cellsZ + 1)));

    for (std::size_t i = 0; i <= cellsX; ++i) {
        for (std::size_t j = 0; j <= cellsY; ++j) {
            for (std::size_t k = 0; k <= cellsZ; ++k) {
                const float x = origin.x + static_cast<float>(i) * spacing.x;
                const float y = origin.y + static_cast<float>(j) * spacing.y;
                const float z = origin.z + static_cast<float>(k) * spacing.z;
                field[i][j][k] = sample(x, y, z);
            }
        }
    }

    return triangulate_field(field, isovalue, origin, spacing, cellTriangleIndices);
}