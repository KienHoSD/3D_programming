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
const string filename = "tgatest.tga";

const TGAColor white = {255, 255, 255};
const TGAColor red = {255, 0, 0};
const TGAColor green = {0, 255, 0};
const TGAColor blue = {0, 0, 255};
vec3 lightsource_direction = {-1, -1, -1}; // will be reduce to normal vector later
vec3 camera_coord = {1, 1, 3};
vec3 center = {0, 0, 0};
vec3 up = {0, 1, 0};
double lintensity = 1;
double contrast = 1;
double ambientcolor = 0; // TGAColor{5,5,5}
double specularstrength_scale = 1.5;
double glowscale = 10;
double *shadowbuffer = new double[imageh * imagew];
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

struct DepthShader : public IShader
{
   mat<3, 3> texturecoords;
   mat<3, 3> normalvectors;
   mat<3, 3> varying_tri;
   vec3 normalvector;
   vec3 colorcoord;

   virtual vec3 vertex(int iface, int nthvert)
   {
      texturecoords[nthvert] = model->uv(iface, nthvert);
      normalvectors[nthvert] = model->normal(iface, nthvert);
      vec4 gl_vertex = embed<4, 3>(model->vert(iface, nthvert));
      gl_vertex = Viewport * Projection * Modelview * gl_vertex;
      varying_tri[nthvert] = ROUND_VECTOR(proj<3, 4>(gl_vertex / gl_vertex[3]));
      return varying_tri[nthvert];
   }

   virtual bool fragment(vec3 barycentricvec, TGAColor &color)
   {
      vec3 p = varying_tri.transpose() * barycentricvec;
      for (int i = 0; i < 3; i++)
         color[i] = 255. * max<double>(p.z, 0) / imaged;
      return 0;
   }
};

struct Shader : public IShader
{
   mat<3, 3> texturecoords;
   mat<3, 3> normalvectors;
   mat<3, 3> varying_tri;
   mat<4, 4> Mshadow_transform; // transform view screen vertices to light screen vertices.
   vec3 normalvector;
   vec3 colorcoord;

   Shader() {}
   Shader(mat<4, 4> MShadow_transmat) : Mshadow_transform(MShadow_transmat) {}

   virtual vec3 vertex(int iface, int nthvert)
   {
      texturecoords[nthvert] = model->uv(iface, nthvert);
      normalvectors[nthvert] = model->normal(iface, nthvert);
      vec4 gl_vertex = embed<4, 3>(model->vert(iface, nthvert));
      gl_vertex = Viewport * Projection * Modelview * gl_vertex;
      varying_tri[nthvert] = ROUND_VECTOR(proj<3, 4>(gl_vertex / gl_vertex[3]));
      return varying_tri[nthvert];
   }

   virtual bool fragment(vec3 barycentricvec, TGAColor &color)
   {
      vec4 current_pixel_coord_lightview = Mshadow_transform * embed<4, 3>(varying_tri.transpose() * barycentricvec);
      current_pixel_coord_lightview = current_pixel_coord_lightview / current_pixel_coord_lightview[3];
      int idx = int(current_pixel_coord_lightview[0]) + int(current_pixel_coord_lightview[1]) * imagew;
      double shadow_strength = 0.3 + 0.7 * (shadowbuffer[idx] < current_pixel_coord_lightview[2] + 45);

      normalvector = normalvectors.transpose() * barycentricvec;
      normalvector = normalvector * contrast;
      colorcoord = texturecoords.transpose() * barycentricvec;
      TGAColor specularcolor = specularmap.get(colorcoord.x, colorcoord.y);

      mat<3, 3> A = mat<3, 3>{texturecoords[1] - texturecoords[0], texturecoords[2] - texturecoords[0], normalvector}.transpose();
      mat<3, 3> AI = A.invert();
      vec3 i = AI * vec<3>{texturecoords[1] - texturecoords[0]};
      vec3 j = AI * vec<3>{texturecoords[2] - texturecoords[0]};
      mat<3, 3> B = mat<3, 3>{i, j, normalvector}.transpose();
      vec3 n = (B * model->normal(colorcoord)).normalized();
      vec3 l = proj<3, 4>(Projection * Modelview * embed<4, 3>(lightsource_direction)).normalized();
      vec3 r = ((n * -l * 2.) * n + l).normalized();
      double strength = max<double>(0, (n * -l) * lintensity);
      double specularstrength = pow(max<double>(0, r.z), specularcolor[0]);

      if (strength < 0)
         strength = 0;
      else if (strength > 1)
         strength = 1;
      if (specularstrength < 0)
         specularstrength = 0;
      else if (specularstrength > 1)
         specularstrength = 1;

      // find texture.
      color = diffusemap.get(ROUNDNUM(colorcoord.x * diffusemap.width()), ROUNDNUM(colorcoord.y * diffusemap.height()));
      TGAColor glow = glowmap.get(ROUNDNUM(colorcoord.x * glowmap.width()), ROUNDNUM(colorcoord.y * glowmap.height()));

      for (int i = 0; i < 3; i++)
      {
         color[i] = max<double>(0, min<double>(255, glow[i]*glowscale + ambientcolor + color[i] * shadow_strength * (strength + specularstrength_scale * specularstrength)));
      }
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

int main(int argc, char **argv)
{
   string filename;
   if (argc == 2)
      filename = argv[1];
   else
   {
      filename = "obj/african_head/african_head.obj";
   }

   double *zbuffer = new double[imageh * imagew];
   fill(zbuffer, zbuffer + imageh * imagew, -std::numeric_limits<double>::max());
   fill(shadowbuffer, shadowbuffer + imageh * imagew, -std::numeric_limits<double>::max());

   model = new Model(filename);
   diffusemap = model->diffuse();
   specularmap = model->specular();
   glowmap = model->glow();

   TGAImage *image = new TGAImage(imagew, imageh, TGAImage::RGB);
   TGAImage *depth = new TGAImage(imagew, imageh, TGAImage::GRAYSCALE);
   DepthShader depthshader;

   vec3 vertices[3];
   vec3 depthvertices[3];

   viewport(imagew * 1 / 8, imageh * 1 / 8, imagew * 3 / 4, imageh * 3 / 4, imaged);
   projection(0);
   lookat(-lightsource_direction, center, up);
   mat<4, 4> MS = Viewport * Projection * Modelview;

   for (int i = 0; i < model->nfaces(); i++)
   {
      for (int j = 0; j < 3; j++)
      {
         depthvertices[j] = depthshader.vertex(i, j);
      }
      triangle(depthvertices, depthshader, shadowbuffer, depth);
   }

   viewport(imagew * 1 / 8, imageh * 1 / 8, imagew * 3 / 4, imageh * 3 / 4, imaged);
   projection(-1 / camera_coord.z);
   lookat(camera_coord, center, up);
   mat<4, 4> M = MS * (Viewport * Projection * Modelview).invert();
   Shader shader(M);

   for (int i = 0; i < model->nfaces(); i++)
   {
      for (int j = 0; j < 3; j++)
      {
         vertices[j] = shader.vertex(i, j);
      }
      triangle(vertices, shader, zbuffer, image);
   }

   image->write_tga_file("test.tga");
   depth->write_tga_file("depth.tga");
   delete[] zbuffer;
   delete model;
   delete image;
   return 0;
}