#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"

#include <iostream>

using color = vec3;

void write_color(std::ostream& out, const vec3& color){
  auto r = color.x();
  auto g = color.y();
  auto b = color.z();

  // translate from 0...1 to 0...255
  int rbyte = int(255.999 * r);
  int gbyte = int(255.999 * g);
  int bbyte = int(255.999 * b);

  // print out to output ppm file
  out << rbyte << " " << gbyte << " " << bbyte << "\n";
}

#endif