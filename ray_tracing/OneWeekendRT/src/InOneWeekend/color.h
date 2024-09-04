#ifndef COLOR_H
#define COLOR_H

#include "interval.h"
#include "vec3.h"

#include <iostream>

using color = vec3;

void write_color(std::ostream& out, const vec3& color){
  auto r = color.x();
  auto g = color.y();
  auto b = color.z();

  // translate from 0...1 to 0...255
  static const interval intensity(0.0,0.999);
  int rbyte = int(256 * intensity.clamp(r));
  int gbyte = int(256 * intensity.clamp(g));
  int bbyte = int(256 * intensity.clamp(b));

  // print out to output ppm file
  out << rbyte << " " << gbyte << " " << bbyte << "\n";
}

#endif