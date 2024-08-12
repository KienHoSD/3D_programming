#include <iostream>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const int width = 160, height = 44; // Width and height of the terminal window
float A = 0, B = 0, C = 0;          // number to store current rotation angle value;
float zBuffer[160 * 44];            // Z-buffer for depth calculations
char buffer[160 * 44];              // Buffer for storing ASCII characters
int backgroundASCIICode = ' ';      // ASCII character for the background
int distanceFromCam = 5;            // Distance from the camera to the circle
float horizontalOffset = 0;         // Horizontal offset for projection
float K1 = 100;                     // Projection constant
float increase_dens = 0.05;         // same as above, but less mean more density without increasing the projection
float size1stcircle = 2;            // 2 is size of the 1st circle
float size2ndcircle = 1;            // 1 is size of the 2nd circle
float speedX = 0.02;                // speed rotate around X axis
float speedY = 0.01;                // speed rotate around Y axis
float speedZ = 0.001;               // speed rotate around Z axis

float x, y, z; // 3D coordinates
float ooz;     // One over z (reciprocal of z)
int xp, yp;    // Screen coordinates
int idx;       // Buffer index

// the coordinate vector are
//            ^ z axis
//          /
//        /
//      /
//    /
//  /_ _ _ _ _ _ _ > x axis
// |
// |
// |
// |      screen face
// |
// |
// v y axis

// Calculate X coordinate after projection
float calculateX(float i, float j, float k, float A = A, float B = B, float C = C)
{
  return j * sin(A) * sin(B) * cos(C) -
         k * cos(A) * sin(B) * cos(C) +
         j * cos(A) * sin(C) +
         k * sin(A) * sin(C) +
         i * cos(B) * cos(C);
}

// Calculate Y coordinate after projection
float calculateY(float i, float j, float k, float A = A, float B = B, float C = C)
{
  return j * cos(A) * cos(C) +
         k * sin(A) * cos(C) -
         j * sin(A) * sin(B) * sin(C) +
         k * cos(A) * sin(B) * sin(C) -
         i * cos(B) * sin(C);
}

// Calculate Z coordinate after projection
float calculateZ(float i, float j, float k, float A = A, float B = B, float C = C)
{
  return k * cos(A) * cos(B) -
         j * sin(A) * cos(B) +
         i * sin(B);
}

void drawing2ndCircleAroundCoordinate(float circleX, float circleY, float circleZ, float angle, float size1stcircle, char ch = 0)
{
  float circleX2, circleY2, circleZ2;
  float circle2X, circle2Y, circle2Z;

  for (float i = 0; i < 2 * M_PI; i += increase_dens)
  {
    circle2X = cos(i) * size2ndcircle;
    circle2Y = sin(i) * size2ndcircle;
    circle2Z = 0;

    // rotate circle2 z 90 degree, y -angle degree
    circleX2 = calculateX(circle2X, circle2Y, circle2Z, M_PI / 2, 0, -angle);
    circleY2 = calculateY(circle2X, circle2Y, circle2Z, M_PI / 2, 0, -angle);
    circleZ2 = calculateZ(circle2X, circle2Y, circle2Z, M_PI / 2, 0, -angle);

    circleX2 += circleX;
    circleY2 += circleY;
    circleZ2 += circleZ;

    x = calculateX(circleX2, circleY2, circleZ2);
    y = calculateY(circleX2, circleY2, circleZ2);
    z = calculateZ(circleX2, circleY2, circleZ2);

    // negative means if we want to reflect thing that more close to the light source (-1, -0.5)

    float L = -1 * z + -0.5 * y;
    L += 2;

    z += distanceFromCam * 4;

    if (z == 0)
    {
      z = 1e-6; // Avoid division by zero
    }
    ooz = 1 / z;

    xp = (int)(width / 2 + horizontalOffset + K1 * ooz * x * 2);
    yp = (int)(height / 2 + K1 * ooz * y);

    idx = xp + yp * width;
    if (idx >= 0 && idx < width * height && L > 0)
    {
      if (ooz > zBuffer[idx])
      {
        zBuffer[idx] = ooz;
        // if given a character ch, not apply luminance
        if (ch)
        {
          buffer[idx] = ch;
          continue;
        }
        int luminance = L * 2; // scaling lumincance
        buffer[idx] = ",;:-=*cad$&@"[luminance];
      }
    }
  }
}

int main()
{

  // clear the buffer
  printf("\x1b[2J");

  while (1)
  {
    memset(buffer, backgroundASCIICode, width * height); // Clear the buffer
    memset(zBuffer, 0, width * height * sizeof(float));  // Clear the z-buffer

    // Rotate the circle, fmod to make sure it could rotate forever ;)
    A = A < 100 ? A + speedX : fmod(A,2*M_PI);
    B = B < 100 ? B + speedY : fmod(B,2*M_PI);
    C = C < 100 ? C + speedZ : fmod(C,2*M_PI);

    for (float i = 0; i < 2 * M_PI; i += increase_dens)
      drawing2ndCircleAroundCoordinate(size1stcircle * cos(i), size1stcircle * sin(i), 0, i, size1stcircle);

    // Move the cursor to (0, 0)
    printf("\033[H");

    // Display the rendered frame
    for (int k = 0; k < width * height; k++)
    {
      putchar(buffer[k]);
      if (k % width == width - 1)
      {
        putchar('\n');
      }
    }

    usleep(8000); // Sleep to control frame rate
  }

  return 0;
}