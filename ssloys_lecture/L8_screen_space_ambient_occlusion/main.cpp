#include "tgaimage.h"
#include "model.h"
#include "math.h"
#include "algorithm"
#include "string.h"
#include "iostream"

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::fill;
using std::max;
using std::min;
using std::string;
using std::swap;
using std::vector;

int imageh = 1600;
int imagew = 1600;
int imaged = 1600;
double max_distant = sqrt(imagew*imagew + imageh*imageh); // the max distant of 2 different pixel of the image (pixel unit)
const string filename = "tgatest.tga";
const TGAColor white = {255, 255, 255};
const TGAColor red = {255, 0, 0};
const TGAColor green = {0, 255, 0};
const TGAColor blue = {0, 0, 255};
vec3 lightsource_direction = {-1, -1, 0}; // will be normalized, show light source direction
vec3 camera_coord = {1.2,-.8,3};            // coordinates of the camera
vec3 center = {0, 0, 0};                  // camera look at center (default: origin {0,0,0})
vec3 up = {0, 1, 0};                      // up direction of camera (y axis of camera)

// ^ y        ^ -z (-z not z)
// |        /
// |      /
// |    /
// |  /
// |/
// 0------------> x

double lintensity = 1;               // strength of the light source
double contrast = 1;                 // scaling the normal vector to contrast
double ambientcolor = 5;             // constant ambient color TGA{5,5,5}
double model_scale = 1;              // scale all the vertex to model_scale
double diffstrength_scale = 2;       // can be varies, should be from 1 to 2
double specularstrength_scale = 1.5; // can be varies, should be from 0 to 2
double glow_scale = 10;              // can be varies, should be 1 to 10
double shadowstrength_scale = 0.5;   // from 0 to 1 (lower means darker shadow, 1 means no shadow)

double *zbuffer = new double[imageh * imagew];
mat<4, 4> Projection;
mat<4, 4> Viewport;
mat<4, 4> Modelview;
Model *model;
TGAImage diffusemap;
TGAImage specularmap;
TGAImage glowmap;

#define ROUNDNUM(x) (int)(x >= .5f ? (x + 1.0f) : x)
vec3 ROUND_VECTOR(const vec3 &vec)
{
   vec3 ret;
   for (int i = 0; i < 3; i++)
      ret[i] = ROUNDNUM(vec[i]);
   return ret;
}

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

void viewport(double x, double y, double w, double h, double d)
{
   Viewport = mat<4, 4>::identity();
   Viewport[0][3] = x + w / 2.;
   Viewport[1][3] = y + w / 2.;
   Viewport[2][3] = d / 2;

   Viewport[0][0] = w / 2.;
   Viewport[1][1] = h / 2.;
   Viewport[2][2] = d / 2;
}

void projection(double r)
{
   Projection = mat<4, 4>::identity();
   Projection[3][2] = r;
}

void lookat(vec3 eye, vec3 center, vec3 up)
{
   vec3 z = (eye - center).normalized();
   vec3 x = cross(up, z).normalized();
   vec3 y = cross(z, x).normalized();
   mat<4, 4> model = mat<4, 4>::identity();
   mat<4, 4> view = mat<4, 4>::identity();
   for (int i = 0; i < 3; i++)
   {
      model[0][i] = x[i];
      model[1][i] = y[i];
      model[2][i] = z[i];
      view[i][3] = -center[i];
   }
   Modelview = model * view;
}

struct IShader
{
   virtual vec3 vertex(int iface, int nthvert) = 0;
   virtual bool fragment(vec3 bar, TGAColor &color) = 0;
};

struct ZShader : public IShader{
   mat<3, 3> varying_tri;

   virtual vec3 vertex(int iface, int nthvert)
   {
      vec4 gl_vertex = embed<4, 3>(model->vert(iface, nthvert) * model_scale);
      gl_vertex = Viewport * Projection * Modelview * gl_vertex;
      varying_tri[nthvert] = ROUND_VECTOR(proj<3, 4>(gl_vertex / gl_vertex[3]));
      return varying_tri[nthvert];
   }

   virtual bool fragment(vec3 barycentricvec, TGAColor &color)
   {
      // color = {0,0,0}; // make the image black. No need to, since its already a black image.
      return 0;
   }
};

void triangle(vec3 *vertices, IShader &shader, double *zbuffer, TGAImage *image)
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
         vec3 barycentricvec = barycentric(P, vertices);

         double z = 0;
         for (int i = 0; i < 3; i++)
            z += barycentricvec[i] * vertices[i][2];

         // check zbuffer soon as possible
         int index = x + y * imgw;
         if (index < 0 || index >= imageh * imagew || barycentricvec.x < 0 || barycentricvec.y < 0 || barycentricvec.z < 0 || zbuffer[index] >= z)
            continue;

         zbuffer[index] = z;
         TGAColor color;
         if (shader.fragment(barycentricvec, color))
         {
            cerr << "error";
            abort();
         }
         image->set(x, y, color);
      }
   }
}

double max_elevation_angle(double* zbuffer, vec2 p, vec2 dir){
   double max_angle = 0;
   for(int t=0.;t<max_distant;t+=1.){
      vec2 cur = p + dir*t;
      if(cur.x >= imagew || cur.x < 0 || cur.y >= imageh || cur.y < 0){
         return max_angle;
      } // black pixel appear or out of image then stop
      double elevation = zbuffer[int(cur.x) + int(cur.y)*imagew] - zbuffer[int(p.x) + int(p.y) * imagew];
      double distant = (cur-p).norm();
      if(distant < 1.) continue; // avoid divide by 0
      max_angle = max<double>(max_angle, atanf(elevation/distant));
   }
   return max_angle;
}

int main(int argc, char **argv)
{
   string filename;
   if (argc == 2)
      filename = argv[1];
   else
   {
      filename = "obj/diablo3_pose/diablo3_pose.obj";
      // filename = "obj/african_head/african_head.obj";
      // filename = "obj/skull/skull.obj";
   }

   double *zbuffer = new double[imageh * imagew];
   fill(zbuffer, zbuffer + imageh * imagew, -std::numeric_limits<double>::max());

   model = new Model(filename);

   TGAImage *image = new TGAImage(imagew, imageh, TGAImage::RGB);
   ZShader zshader;

   vec3 zvertices[3];

   viewport(imagew/ 8, imageh/ 8, imagew * 3 / 4, imageh * 3 / 4, imaged);
   projection(-1 / (camera_coord-center).norm());
   lookat(camera_coord, center, up);

   for (int i = 0; i < model->nfaces(); i++)
   {
      for (int j = 0; j < 3; j++)
      {
         zvertices[j] = zshader.vertex(i, j);
      }
      triangle(zvertices, zshader, zbuffer, image);
   }

   for (double x=0; x < imagew; x++){
      for(double y=0; y< imageh; y++){
         double total = 0;
         if(zbuffer[int(x)+int(y)*imagew] < 1e-5) continue; // its already a black pixel, no need to calculate
         for(double i=0; i < 2*M_PI - 1e-4; i+=M_PI/4){ // we only get 8 direction from this for loop
            total += M_PI/2 - max_elevation_angle(zbuffer,vec2{x,y},vec2{cos(i),sin(i)});
         }
         total /= (M_PI/2)*8; // get the average angle of all direction
         image->set(x,y,TGAColor{u_char(total*255),u_char(total*255),u_char(total*255)});
      }
   }

   image->write_tga_file("test.tga");
   delete[] zbuffer;
   delete model;
   delete image;
   return 0;
}