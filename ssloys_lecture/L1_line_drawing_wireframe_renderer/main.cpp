#include "tgaimage.h"
#include "model.h"
#include "math.h"
#include "algorithm"
#include "string.h"
#include "iostream"

using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::swap;
using std::vector;

#define ROUNDNUM(x) ((int)(x >= .5f ? (x + 1.0f) : x))

const int imageh = 800;
const int imagew = 800;
const string filename = "tgatest.tga";

const TGAColor white = {255, 255, 255};
const TGAColor red = {255, 0, 0};
const TGAColor green = {0, 255, 0};
const TGAColor blue = {0, 0, 255};

void draw_line(int x0, int y0, int x1, int y1, TGAImage *image, const TGAColor &color)
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

int main(int argc, char **argv)
{
   // TGAImage *image = new TGAImage(imagew, imageh, 3);
   // TGAColor icolor = {255, 255, 255, 1};
   // // image->set(50, 50, icolor);
   // int x0 = 20;
   // int y0 = 200;
   // int x1 = 180;
   // int y1 = 25;
   // draw_line(x0, y0, x1, y1, image, white);
   // draw_line(x0 + 120, y0, x1, y1, image, green);
   // draw_line(x0, y0, x0 + 200, y0, image, white);
   // draw_line(x0, y0, x0, y0 - 170, image, green);
   // image->write_tga_file(filename, true, true);

   string filename;
   if (argc == 2)
   {
      filename = argv[1];
   }
   else
      filename = "obj/african_head/african_head.obj";

   Model* model = new Model(filename);
   cout << model->nfaces() << endl;

   TGAImage *image = new TGAImage(imagew, imageh, TGAImage::RGB);
   for (int i = 0; i < model->nfaces(); i++)
   {
      for (int j = 0; j < 3; j++)
      {
         vec3 vert1 = model->vert(i, j);
         vec3 vert2 = model->vert(i, (j + 1) % 3);

         int x0 = (vert1.x + 1.) * imagew / 2;
         int y0 = (vert1.y + 1.) * imageh / 2;
         int x1 = (vert2.x + 1.) * imagew / 2;
         int y1 = (vert2.y + 1.) * imageh / 2;

         // cout << x0 << " " << y0 << " " << x1 << " " << y1 << std::endl;
         draw_line(x0, y0, x1, y1, image, white);
      }
   }
   image->write_tga_file("test.tga");

   return 0;
}
