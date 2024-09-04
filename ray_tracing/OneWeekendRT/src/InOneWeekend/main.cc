#include "rtweekend.h"

#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"

int main()
{
    // World
    hittable_list world;
    world.add(make_shared<sphere>(sphere(point3(0.5, 0, -1), 0.5)));
    world.add(make_shared<sphere>(sphere(point3(-0.5, 0, -1), 0.2)));
    world.add(make_shared<sphere>(sphere(point3(0, -100.5, 0), 100)));

    // Camera
    camera cam;
    cam.aspect_ratio = 16 / 9.;
    cam.image_width = 400;

    // Render
    cam.render(world);
}