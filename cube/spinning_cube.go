package main
import "fmt"
import "math"
import "time"

var A, B, C float64 = 0.0, 0.0, 0.0;    // Rotation angles for the cube
const width, height int32 = 160, 44; // Width and height of the terminal window
var zBuffer = [160 * 44]float64{};            // Z-buffer for depth calculations
var buffer = [160 * 44]byte{};                // Buffer for storing ASCII characters
var backgroundASCIICode byte = ' ';      // ASCII character for the background
var distanceFromCam int32 = 150;          // Distance from the camera to the cube
var horizontalOffset float64 = 0;         // Horizontal offset for projection
var K1 float64 = 70                      // Projection constant
var incrementSpeed float64 = 0.02        // Rotation speed (adjust as needed)
var density float64 = 20                 // density of calculating (preventing of not filling the whole plane with ch, also increase projection)
var increase_dens float64 = 0.5            // same as above, but less mean more density without increamath.Sing the projection

var x, y, z float64 // 3D coordinates
var ooz float64     // One over z (reciprocal of z)
var xp, yp int32    // Screen coordinates
var idx int32       // Buffer index

func calculateX(i float64, j float64, k float64) float64 {
  return j * math.Sin(A) * math.Sin(B) * math.Cos(C) - 
         k * math.Cos(A) * math.Sin(B) * math.Cos(C) + 
         j * math.Cos(A) * math.Sin(C) +
         k * math.Sin(A) * math.Sin(C) +
         i * math.Cos(B) * math.Cos(C);
}

func calculateY(i float64, j float64, k float64) float64 {
  return j * math.Cos(A) * math.Cos(C) +
         k * math.Sin(A) * math.Cos(C) -
         j * math.Sin(A) * math.Sin(B) * math.Sin(C) +
         k * math.Cos(A) * math.Sin(B) * math.Sin(C) -
         i * math.Cos(B) * math.Sin(C);
}

func calculateZ(i float64, j float64, k float64) float64 {
  return k * math.Cos(A) * math.Cos(B) -
         j * math.Sin(A) * math.Cos(B) +
         i * math.Sin(B);
}

func calculateForSurface(cubeX float64, cubeY float64, cubeZ float64, ch byte){
  x = calculateX(cubeX,cubeY,cubeZ);
  y = calculateY(cubeX,cubeY,cubeZ);
  z = calculateZ(cubeX,cubeY,cubeZ);

  x = x * (1-z/60);
  y = y * (1-z/60);
  z += float64(distanceFromCam);

  if (z == 0){
    z = 1e-6;
  }

  ooz = 1 / z;

  xp = int32(width / 2 + int32(horizontalOffset + K1 * ooz * x * 2));
  yp = int32(height / 2 + int32(K1 * ooz * y));
  idx = yp*width + xp;
  if (idx >= 0 && idx < width * height){
    if(ooz > zBuffer[idx]){
      zBuffer[idx] = ooz;
      buffer[idx] = ch;
    }
  }
}

func SetAll[T any](s []T, v T){
  for i := range s {
    s[i] = v;
  }
}

func main() {
  fmt.Printf("\x1b[2J")
  for true {
    SetAll(buffer[:], backgroundASCIICode);
    SetAll(zBuffer[:], 0);

    // Rotate the cube
    A += incrementSpeed;
    B += incrementSpeed;
    C += 0;

    // Rende the cubes
    for cubeX := -density; cubeX < density; cubeX += increase_dens {
      for cubeY := -density; cubeY < density; cubeY += increase_dens {
        calculateForSurface(cubeX, cubeY, -density, '@');
        calculateForSurface(density, cubeY, cubeX, '$');
        calculateForSurface(-density, cubeY, -cubeX, '~');
        calculateForSurface(-cubeX, cubeY, density, '#');
        calculateForSurface(cubeX, -density, -cubeY, ';');
        calculateForSurface(cubeX, density, cubeY, '+');
      }
    }

    fmt.Printf("\x1b[H");

    for k := int32(0); k < width * height; k++ {
      fmt.Printf("%c",buffer[k]);
      if (k % width == width - 1){
        fmt.Printf("\n");
      }
    }
    
    time.Sleep(7000 * time.Microsecond);

  }
}