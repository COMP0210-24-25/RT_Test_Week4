#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include "VectorUtils.hpp"
#include "Rendering.hpp"
#include "Sphere.hpp"
#include "Ray.hpp"
#include "Camera.hpp"
#include "Material.hpp"
#include <string>
#include <fstream>
#include <exception>
#include <sstream>
#include <memory>
#include "ImageUtils.hpp"
#include "Object.hpp"
#include "Plane.hpp"
#include <algorithm>

using namespace Catch::Matchers;

std::vector<std::vector<std::array<float, 3>>> loadImage(std::string filename, uint w = 100, uint h = 100)
{
    std::vector<std::vector<std::array<float, 3>>> image;
    std::ifstream image_file;
    image_file.open(filename);
    if (image_file)
    {
        std::string line;

        // ignore header line
        std::getline(image_file, line);

        // get dimensions
        std::getline(image_file, line);
        std::istringstream line_stream(line);
        uint width, height;
        line_stream >> width;
        line_stream >> height;

        if ((width != w) || (height != h))
        {
            throw std::runtime_error("Dimensions of the image are not as expected");
        }

        image.resize(width);
        for (auto &col : image)
        {
            col.resize(height);
        }

        // ignore  pixel limit
        std::getline(image_file, line);

        for (uint y = 0; y < height; y++)
        {
            for (uint x = 0; x < width; x++)
            {
                if (std::getline(image_file, line))
                {
                    std::istringstream pixel_stream(line);
                    pixel_stream >> image[x][y][0];
                    pixel_stream >> image[x][y][1];
                    pixel_stream >> image[x][y][2];
                }
                else
                {
                    throw std::runtime_error("Ran out of pixel data.");
                }
            }
        }
    }
    else
    {
        throw std::runtime_error("File " + filename + " not found.");
    }

    return image;
}

float clamp0to255(float val)
{
    return std::max(std::min(val, float(255.)), float(0.));
}

double diffImage(std::vector<std::vector<std::array<float, 3>>> im1,
                 std::vector<std::vector<std::array<float, 3>>> im2)
{
    size_t width, height;
    width = im1.size();
    height = im1.at(0).size();

    REQUIRE(im2.size() == width);
    REQUIRE(im2.at(0).size() == height);

    double diff = 0;
    for (size_t i = 0; i < width; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
            diff += fabs(im1[i][j][0] - clamp0to255(im2[i][j][0]));
            diff += fabs(im1[i][j][1] - clamp0to255(im2[i][j][1]));
            diff += fabs(im1[i][j][2] - clamp0to255(im2[i][j][2]));
        }
    }
    double average_diff = diff / (width * height);
    return average_diff;
}


TEST_CASE("Test Shaded Sphere", "[Test Sphere]")
{
    using namespace VecUtils;
    Material mat(255, 255, 255);
    
}

TEST_CASE("Test Plane Class", "[Test Plane]")
{
    using namespace VecUtils;
    Material mat(255, 255, 255);
    Plane *p;
    REQUIRE_NOTHROW(p = new Plane({0,0,0}, {0, 1, 0}, mat));
    delete p;
}

TEST_CASE("Test Plane Render", "[Test Plane]")
{
    using namespace VecUtils;
    Vec3 origin{0,-1,0};
    Vec3 up{0, 1, 0};
    Vec3 tilt{0, 1, 1};

    Material mat(255, 255, 0);

    Camera cam(100, 100);
    
    Plane p(origin, up, mat);
    std::vector<Object*> objs{&p};
    std::vector<std::vector<std::array<float, 3>>> image_data = Render::genImage(cam, objs);
    auto expected_image = loadImage("data/floorRender.pbm");
    double average_diff = diffImage(expected_image, image_data);
    REQUIRE(average_diff < 10);

    Plane p_tilted(origin, tilt, mat);
    objs = {&p_tilted};
    image_data = Render::genImage(cam, objs);
    expected_image = loadImage("data/slopeRender.pbm");
    average_diff = diffImage(expected_image, image_data);
    REQUIRE(average_diff < 10);
}

TEST_CASE("Test Virtual Intersect", "[Test Polymorphism]")
{
    using namespace VecUtils;
    Material mat(255, 255, 0);
    std::unique_ptr<Object> p_sphere = std::make_unique<Sphere>(2, Vec3{0, 0, 0}, mat);

    for(double i = -3.5; i <= 3.5; i += 1.0)
    {
        IntersectionData id;
        Ray ray(Vec3{i,0,10},Vec3{0, 0, -1});
        if(i > -2 && i < 2)
        {
            CHECK(p_sphere->Intersect(ray, id));
        }
        else
        {
            CHECK(!p_sphere->Intersect(ray, id));
        }
    }

    std::unique_ptr<Object> p_plane = std::make_unique<Plane>(Vec3{0,0,0}, Vec3{0, 1, 1}, mat);

    for(int i = -5; i <= 5; i++)
    {
        IntersectionData id;
        Ray ray(Vec3{static_cast<double>(i),0,10},Vec3{0, 0, -1});
        CHECK(p_plane->Intersect(ray, id)); 
    }
}

TEST_CASE("Test Polymorphic Object List", "[Test Polymorphism]")
{
    Material mat(255, 255, 255);
    
    Sphere s(2, VecUtils::Vec3{0,0,0}, mat);
    Plane p(VecUtils::Vec3{0,0,0}, VecUtils::Vec3{0, 0, 1}, mat);

    std::vector<Object*> scene_list = {&s, &p};

    for(Object *obj_ptr : scene_list)
    {
        Ray ray(VecUtils::Vec3{0, 0, 10}, VecUtils::Vec3{0, 0, -1});
        IntersectionData id;
        REQUIRE(obj_ptr->Intersect(ray, id));
    } 
}

TEST_CASE("Test Occlusion", "[Test Intersection]")
{
    using namespace VecUtils;
    Material mat(255, 255, 0);
    Sphere s_front(2, Vec3{0,0,0});
    Sphere s_behind(2, Vec3{0,0,-5});

    std::cout << &s_front << " " << &s_behind << std::endl;

    // Front sphere first then behind
    IntersectionData id;
    Ray ray(Vec3{0,0,10}, Vec3{0,0,-1});
    s_front.Intersect(ray, id);
    s_behind.Intersect(ray, id);
    REQUIRE(id.getObject() == &s_front);

    // Back sphere first then front
    IntersectionData id2;
    s_behind.Intersect(ray, id2);
    s_front.Intersect(ray, id2);
    REQUIRE(id.getObject() == &s_front);
}

TEST_CASE("Test Image with Spheres and Planes", "[Test Polymorphism]")
{
    using namespace VecUtils;
    using std::vector;

    Sphere sphere(2, {0,0,3}, Material(255, 255, 0));
    Sphere sphere2(2, {3, 0, 0}, Material(0, 255, 255));
    Plane floor({0, -2, 0}, {0, 1, 0}, Material{255, 0, 0});
    Camera cam(100, 100);
    
    vector<Object*> objs{&sphere, &sphere2, &floor};

    // Attempt to render
    vector<vector<std::array<float, 3>>> image_data = Render::genImage(cam, objs);

    // Load expectation
    auto expected_image = loadImage("data/SphereRender.pbm", 100, 100);

    double average_diff = diffImage(expected_image, image_data);
    REQUIRE(average_diff < 10);
}