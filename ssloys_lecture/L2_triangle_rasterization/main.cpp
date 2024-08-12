#include "tgaimage.h"
#include "model.h"
#include "math.h"
#include "algorithm"
#include "string.h"
#include "iostream"

using std::cin;
using std::cout;
using std::endl;
using std::fill;
using std::max;
using std::min;
using std::string;
using std::swap;
using std::vector;

#define ROUNDNUM(x) ((int)(x >= .5f ? (x + 1.0f) : x))

int imageh = 800;
int imagew = 800;
int imaged = 800;
const string filename = "tgatest.tga";

const TGAColor white = {255, 255, 255};
const TGAColor red = {255, 0, 0};
const TGAColor green = {0, 255, 0};
const TGAColor blue = {0, 0, 255};

void draw_line(int x0, int y0, int x1, int y1, TGAImage *image, TGAImage *diffuse, const TGAColor &color)
{
   if (x0 > x1)
   {
      swap(x1, x0);
      swap(y1, y0);
   }
   int dx = abs(x1 - x0);
   int dy = abs(y1 - y0);
   float rateyx = dy * 1. / dx;
   float ratexy = 1 / rateyx;
   bool steep = dy > dx;
   bool upward = y0 < y1;

   // test not steep i++
   if (!steep)
   {
      if (upward)
      {
         for (float i = 0, j = 0; i <= dx; i++)
         {
            j = rateyx * i;
            image->set(ROUNDNUM(x0 + i), ROUNDNUM(y0 + j), color);
         }
      }
      else
      {
         for (float i = 0, j = 0; i <= dx; i++)
         {
            j = -rateyx * i;
            image->set(ROUNDNUM(x0 + i), ROUNDNUM(y0 + j), color);
         }
      }
   }
   // if steep check y0 < y1 mean steep upward +j for y, else steep downward -j for y
   else
   {
      if (upward)
      {
         for (float j = 0, i = 0; j <= dy; j++)
         {
            i = ratexy * j;
            image->set(ROUNDNUM(x0 + i), ROUNDNUM(y0 + j), color);
         }
      }
      else
      {
         for (float j = 0, i = 0; j <= dy; j++)
         {
            i = ratexy * j;
            image->set(ROUNDNUM(x0 + i), ROUNDNUM(y0 - j), color);
         }
      }
   }
}

vec3 barycentric(vec2 P, vec3 *vertices)
{
   vec3 vecx = {vertices[2][0] - vertices[0][0], vertices[1][0] - vertices[0][0], vertices[0][0] - P[0]};
   vec3 vecy = {vertices[2][1] - vertices[0][1], vertices[1][1] - vertices[0][1], vertices[0][1] - P[1]};
   vec3 u = cross(vecx, vecy);
   vec3 barycentricvec = {-1, -1, -1};
   if (abs(u.z) >= 1e-2)
   {
      barycentricvec = {1. - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z};
   }
   return barycentricvec;
}

void triangle(vec3 *vertices, TGAImage *image, float strength)
{
   double imgw = (double)image->width();
   double imgh = (double)image->height();

   vec2 boundmax = {0, 0};
   vec2 boundmin = {imgw, imgh};

   for (int i = 0; i < 3; i++)
   {
      boundmax.x = (double)min(imgw, (double)max(boundmax.x, vertices[i].x));
      boundmax.y = (double)min(imgh, (double)max(boundmax.y, vertices[i].y));

      boundmin.x = (double)max((double)0, min(boundmin.x, vertices[i].x));
      boundmin.y = (double)max((double)0, min(boundmin.y, vertices[i].y));
   }

   for (double x = boundmin.x; x < boundmax.x; x++)
   {
      for (double y = boundmin.y; y < boundmax.y; y++)
      {
         vec2 P = {x, y};
         vec3 barycentricvec = barycentric(P,vertices);

         if (barycentricvec.x < 0 || barycentricvec.y < 0 || barycentricvec.z < 0)
            continue;

         TGAColor color = {128,128,128};
         for (int i = 0; i < 3; i++)
            color[i] *= strength;
         image->set(x, y, color);
      }
   }
   // image->write_tga_file("test.tga");
}

int main(int argc, char **argv)
{
   string filename;
   if (argc == 2)
   {
      filename = argv[1];
   }
   else
      filename = "obj/african_head/african_head.obj";

   Model *model = new Model(filename);
   TGAImage *image = new TGAImage(imagew, imageh, TGAImage::RGB);
   TGAImage diffusemap = model->diffuse();
   vec3 lightsource_direction = {0, 0, -1};

   for (int i = 0; i < model->nfaces(); i++)
   {
      vec3 vertices[3];
      vec3 screencoord[3];
      vec2 texvertices[3];
      vec3 texturecoord[3];
      for (int j = 0; j < 3; j++)
      {
         vertices[j] = model->vert(i, j);
         screencoord[j].x = ROUNDNUM((vertices[j].x + 1.f) * imagew / 2);
         screencoord[j].y = ROUNDNUM((vertices[j].y + 1.f) * imageh / 2);
         screencoord[j].z = vertices[j].z;
         texvertices[j] = model->uv(i, j);
         texturecoord[j].x = texvertices[j].x;
         texturecoord[j].y = texvertices[j].y;
         texturecoord[j].z = 0;
      }

      vec3 vec1 = vertices[1] - vertices[0];
      vec3 vec2 = vertices[2] - vertices[0];
      vec3 crossv1v2 = cross(vec2, vec1);
      vec3 normcrossv1v2 = crossv1v2.normalized();
      float strength = normcrossv1v2 * lightsource_direction;
      if (strength > 0)
      {
         // TGAColor shade = {uint8_t(strength * 128), uint8_t(strength * 128), uint8_t(strength * 128), 255};
         triangle(screencoord, image, strength);
      }
   }
   image->write_tga_file("test.tga");

   return 0;
}
