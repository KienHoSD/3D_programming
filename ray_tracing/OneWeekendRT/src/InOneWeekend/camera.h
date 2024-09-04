#ifndef CAMERA_H_
#define CAMERA_H_

#include "rtweekend.h"

#include "hittable.h"

class camera
{
public:
  double aspect_ratio = 1.;   // ratio of image width over height
  int image_width = 100;      // pixels in the image width
  int image_height;           // pixles in the image height
  int samples_per_pixel = 10; // Count of random samples for each pixel

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
        color pixel_color = color(0,0,0);
        for(int e = 0; e < samples_per_pixel; e++){
          ray r = get_ray(i,j);
          pixel_color += ray_color(r, world); 
        }
        pixel_color *= pixel_samples_scale; 
        write_color(std::cout, pixel_color);
      }
    }
    std::clog << "\rDone.                   \n";
  }

private:
  double pixel_samples_scale; // Color scale for each pixel
  point3 camera_center;       // Camera center
  point3 pixel00_center;      // Center of the upper left pixel
  vec3 pixel_delta_u;         // Horizontal delta vector from pixel to pixel
  vec3 pixel_delta_v;         // Vertical delta vector from pixel to pixel

  void initialize()
  {
    image_height = int(image_width / aspect_ratio);
    image_height = image_height < 1 ? 1 : image_height;

    pixel_samples_scale = 1.0 / samples_per_pixel;

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

  ray get_ray(int i, int j) const
  {
    // Construct a camera ray originating from the origin and directed at randomly sampled
    // point around the pixel location i, j.

    auto offset = sample_square();
    auto pixel_sample = pixel00_center + ((i + offset.x()) * pixel_delta_u) + ((j + offset.y()) * pixel_delta_v);

    auto ray_origin = camera_center;
    auto ray_direction = pixel_sample - ray_origin;

    return ray(ray_origin, ray_direction);
  }

  vec3 sample_square() const
  {
    // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
    return vec3(random_double() - 0.5, random_double() - 0.5, 0);
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