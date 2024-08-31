#ifndef CAMERA_H_
#define CAMERA_H_

#include "rtweekend.h"

#include "hittable.h"

class camera
{
public:
  double aspect_ratio = 1.; // ratio of image width over height
  int image_width = 100;    // pixels in the image width
  int image_height;

  void render(const hittable &world)
  {
    initialize();

    std::cout << "P3\n"
              << image_width << ' ' << image_height << "\n255\n";

    // Render
    for (int j = 0; j < image_height; j++)
    {
      std::clog << "\rScanlines remaining: " << (image_height - j) << " " << std::flush;
      for (int i = 0; i < image_width; i++)
      {
        auto pixel_center = pixel00_center + (i * pixel_delta_u) + (j * pixel_delta_v);
        auto ray_direction = pixel_center - camera_center;
        ray r(camera_center, ray_direction);

        color pixel_color = ray_color(r, world);
        write_color(std::cout, pixel_color);
      }
    }
    std::clog << "\rDone.                   \n";
  }

private:
  point3 camera_center;
  point3 pixel00_center;
  vec3 pixel_delta_u;
  vec3 pixel_delta_v;

  void initialize()
  {
    image_height = int(image_width / aspect_ratio);
    image_height = image_height < 1 ? 1 : image_height;

    camera_center = point3(0, 0, 0);

    // determine viewport dimensions.
    auto focal_length = 1.;
    auto viewport_height = 2.;
    auto viewport_width = viewport_height * (double(image_width) / image_height);

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    auto viewport_u = vec3(viewport_width, 0, 0);
    auto viewport_v = vec3(0, -viewport_height, 0);

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    pixel_delta_u = viewport_u / image_width;
    pixel_delta_v = viewport_v / image_height;

    // Calculate the location of the upper left pixel.
    auto viewport_upper_left = camera_center - vec3(0, 0, focal_length) - viewport_u / 2 - viewport_v / 2;
    pixel00_center = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
  }

  color ray_color(const ray &r, const hittable &world) const
  {
    hit_record rec;
    if (world.hit(r, interval(0, infinity), rec))
    {
      return (rec.normal + color(1, 1, 1)) / 2;
    }

    // if not hit, will render background from cyan to white base on ray.y (height)
    vec3 unit_direction = unit_vector(r.direction());
    auto a = (unit_direction.y() + 1.0) / 2;
    return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
  }
};

#endif