////////////////////////////////////////////////////////////////////////////////
// Carve
////////////////////////////////////////////////////////////////////////////////
// A simple seam-carving test with golden outputs!
// Finds and highlights the lowest-energy seam of an image.
////////////////////////////////////////////////////////////////////////////////

#include "rigel.h"
#include "rigel-tasks.h"
#include <spinlock.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>

#if 0
#define BARRIER_INFO  RigelBarrier_Info
#define BARRIER_INIT  RigelBarrier_Init
#define BARRIER_ENTER RigelBarrier_EnterFull
#endif

#if 1 
#define BARRIER_INFO  RigelBarrierMT_Info
#define BARRIER_INIT  RigelBarrierMT_Init
#define BARRIER_ENTER RigelBarrierMT_EnterFull
#endif

#define INT_MAX 0x7FFFFFFF

// Synchronization primitives.
ATOMICFLAG_INIT_CLEAR(Init_flag);
BARRIER_INFO bi;

// Image/algorithm-required globals.
static int width, height;
static int pixels_per_thread;
static int x_per_thread;

// Necessary buffers.
int * rgb_buffer;
double * energy;
double * map;

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
double min2(double a, double b);
double min(double a, double b, double c);
double compute_e(int* buf, int x, int y, int w, int h);

int colorMinPath(double* e_buf, int* rgb_buf);
int writeImageOut(int* rgb_buf, char* filename);
int writeEnergyMapOut(double* energy, char* filename);
int writeSeamMapOut(double* map, char* filename);

////////////////////////////////////////////////////////////////////////////////
// Simple functions for computing energy/seam map values.
////////////////////////////////////////////////////////////////////////////////

void DoEnergy(int i) {
  int y = i / width;
  int x = i % width;

  if(y >= height || x >= width)
  {
    return;
  }

  energy[i] = compute_e(rgb_buffer, x, y, width, height);

  // The first row of the seam map is also the first row of the energy map.  
  if(y == 0)
  {
    map[i] = energy[i];
  }
}

void DoMapping(int x, int y){
  if(x >= width)
  {
    return;
  }
  if(x==0)
  {
    map[y * width + x] = energy[y * width + x] + min2(map[(y - 1) * width + x], map[(y - 1) * width + x + 1]);
  }
  else if(x == width - 1)
  { 
    map[y * width + x] = energy[y * width + x] + min2(map[(y - 1) * width + x - 1], map[(y - 1) * width + x]);
  }
  else
  {
    map[y * width + x] = energy[y * width + x] + min(map[(y - 1) * width + x - 1], map[(y - 1) * width + x], map[(y - 1) * width + x + 1]);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Enqueue and dequeue/distribute/dispatch tasks.
////////////////////////////////////////////////////////////////////////////////
void energy_map(int thread, int tasks_per_thread) {
  int i;
  int begin = thread*tasks_per_thread;
  int end = thread*tasks_per_thread+tasks_per_thread;
  for( i = begin; i < end; i++ ){
    DoEnergy(i);
  }
}

void seam_map(int thread, int x_per_thread){
  int x, y;
  int begin = thread * x_per_thread;
  int end = thread * x_per_thread + x_per_thread;

  if(end > width)
  {
    end = width;
  }

  for(y = 1; y < height; y++)
  {
    for(x = begin; x < end; x++)
    {
        DoMapping(x, y);
    }
    BARRIER_ENTER(&bi);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////
////  START MAIN
////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]){
  int i=0;
  int j=0;
  int x=0;
  int y=0;
  int core = RigelGetCoreNum();
  int thread = RigelGetThreadNum();
  int numthreads = RigelGetNumThreads();

  // Flags used to check whether we should print out input to some sort of file.
  int energyMapPrintFlag = 0;
  int seamMapPrintFlag = 0;

  if (core == 0 && thread == 0) {

    // Set flags based off of our arguments. 
    if(argc >= 1)
    {
       energyMapPrintFlag = 1;
    }
    if(argc >= 2)
    {
       seamMapPrintFlag = 1;
    }

    BARRIER_INIT(&bi);  
    fprintf(stderr, "Beginning...\n");

    // Read input.
    int r, g, b;
    int* data;
    FILE * infile;

    infile = fopen("rigel.pngdata", "r");

    if(infile == NULL)
    {
      return -1;
    }

    fscanf(infile, "%d %d", &width, &height);

    // Allocate space for the pixel data.
    rgb_buffer = malloc((3*width*height+1)*sizeof(int));

    // Copy the PNG data.
    for(i = 0; i < width; i++)
    {
      for(j = 0; j < height; j++)
      {
        fscanf(infile, "%d %d %d ", &r, &g, &b);
        rgb_buffer[3*(i * height + j)] = r;
        rgb_buffer[3*(i * height + j) + 1] = g;
        rgb_buffer[3*(i * height + j) + 2] = b;
      }
    }

    fclose(infile);
  
    // Allocate memory for the energy map.
    energy = (double *)malloc(sizeof(double)*width*height);
    map = (double *)malloc(sizeof(double)*width*height);   

    printf("starting!\n");
    
    ClearTimer(0);
    // Start full run timer
    StartTimer(0);

    SIM_SLEEP_OFF();

    atomic_flag_set(&Init_flag);
  }

  // ready, set, GO!
  atomic_flag_spin_until_set(&Init_flag);

  // Set constants (for all cores).
  pixels_per_thread = width*height/numthreads + 1;
  if(width > numthreads)
  {
    x_per_thread = (width / numthreads) + 1;
  }
  else
  {
    x_per_thread = 1;
  }

  // Create the energy map.
  energy_map(thread, pixels_per_thread);
  BARRIER_ENTER(&bi);

  // Create the seam map.
  seam_map(thread, x_per_thread);
  BARRIER_ENTER(&bi);

  // Stopfull run timer
  if (core == 0 && thread == 0) {
    StopTimer(0);

    if(energyMapPrintFlag)
    {
       writeEnergyMapOut(energy, argv[1]);
    }
    if(seamMapPrintFlag)
    {
       writeSeamMapOut(map, argv[2]);
    } 

    colorMinPath(map, rgb_buffer);
    writeImageOut(rgb_buffer, (char*)"output.pngdata");
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Helper functions.
////////////////////////////////////////////////////////////////////////////////

double fabs(double a)
{
  return a>0?a:-1*a;
}

double min2(double a, double b){
  return a<b?a:b;
}

double min(double a, double b, double c){
  return min2(a, min2(b,c));
}

double compute_e(int* buf, int x, int y, int w, int h)
{
  double gx, gy;
  if(y==0)
  {
        gy = fabs(buf[((y+1)*w+x)*3] - buf[(y*w+x)*3]) +
             fabs(buf[((y+1)*w+x)*3+1] - buf[(y*w+x)*3+1]) +
             fabs(buf[((y+1)*w+x)*3+2] - buf[(y*w+x)*3+2]);
  }
  else if (y < h - 1)
  {
        gy = (fabs(buf[((y+1)*w+x)*3] - buf[((y-1)*w+x)*3]) +
             fabs(buf[((y+1)*w+x)*3+1] - buf[((y-1)*w+x)*3+1]) +
             fabs(buf[((y+1)*w+x)*3+2] - buf[((y-1)*w+x)*3+2]))/2;
  }
  else
  {
        gy = fabs(buf[(y*w+x)*3] - buf[((y-1)*w+x)*3]) +
             fabs(buf[(y*w+x)*3+1] - buf[((y-1)*w+x)*3+1]) +
             fabs(buf[(y*w+x)*3+2] - buf[((y-1)*w+x)*3+2]);
  }
  if(x==0)
  {
        gx = fabs(buf[(y*w+x+1)*3] - buf[(y*w+x)*3]) +
             fabs(buf[(y*w+x+1)*3+1] - buf[(y*w+x)*3+1]) +
             fabs(buf[(y*w+x+1)*3+2] - buf[(y*w+x)*3+2]);
  }
  else if (x < w-1)
  {
        gx = (fabs(buf[(y*w+x+1)*3] - buf[(y*w+x-1)*3]) +
             fabs(buf[(y*w+x+1)*3+1] - buf[(y*w+x-1)*3+1]) +
             fabs(buf[(y*w+x+1)*3+2] - buf[(y*w+x-1)*3+2]))/2;
  }
  else
  {
        gx = fabs(buf[(y*w+x)*3] - buf[(y*w+x-1)*3]) +
             fabs(buf[(y*w+x)*3+1] - buf[(y*w+x-1)*3+1]) +
             fabs(buf[(y*w+x)*3+2] - buf[(y*w+x-1)*3+2]);
  }
  return (gx + gy) / 2;
}

// Colors the seam computed previously by modifying the given buffer of pixel
// data.
int colorMinPath(double* e_buf, int* rgb_buf)
{
  int x_min = -1;
  int e_min = INT_MAX;
  int x_curr = 0;
  int y_curr = height - 1;

  // Determine the x value at the bottom of the image with the lowest energy.
  while(x_curr < width)
  {
    if(e_buf[y_curr * width + x_curr] < e_min)
    {
      e_min = e_buf[y_curr * width + x_curr];
      x_min = x_curr;
    }
    x_curr++;
  }

  // Set the current x position to the minimum one encountered.
  x_curr = x_min;

  // Mark that first pixel.
  rgb_buf[3*(y_curr * width + x_curr) + 0] = 0;
  rgb_buf[3*(y_curr * width + x_curr) + 1] = 0;
  rgb_buf[3*(y_curr * width + x_curr) + 2] = 65535;

  y_curr--;
  
  // Iteratively move up the tree and mark pixels.
  while(y_curr >= 0)
  {
    if(x_curr == 0)
    {
      if(e_buf[y_curr * width + x_curr] > e_buf[y_curr * width + x_curr + 1])
      {
        x_curr++;
      }
    }
    else if (x_curr == width - 1)
    {
      if(e_buf[y_curr * width + x_curr] > e_buf[y_curr * width + x_curr - 1])
      {
        x_curr--;
      }
    }
    else
    {
      if(e_buf[y_curr * width + x_curr] > e_buf[y_curr * width + x_curr + 1] &&
         e_buf[y_curr * width + x_curr - 1] > e_buf[y_curr * width + x_curr + 1])
      {
        x_curr++;
      }
       if(e_buf[y_curr * width + x_curr] > e_buf[y_curr * width + x_curr - 1] &&
          e_buf[y_curr * width + x_curr + 1] > e_buf[y_curr * width + x_curr - 1])
      {
        x_curr--;
      }
    }
    rgb_buf[3*(y_curr * width + x_curr) + 0] = 65535;
    rgb_buf[3*(y_curr * width + x_curr) + 1] = 0;
    rgb_buf[3*(y_curr * width + x_curr) + 2] = 0;
    
    y_curr--;  
  }
  return 0;
}

int writeImageOut(int* rgb_buf, char* filename)
{
    FILE * outfile;

    outfile = fopen(filename, "w");

    if(outfile == NULL)
    {
      return -1;
    }

  fprintf(outfile, "%d %d\n", width, height);

  int i, j;

  for(i = 0; i < height; i++)
  {
    for(j = 0; j < width; j++)
    {
      fprintf(outfile, "%d %d %d ", rgb_buf[3*(i * width + j)], rgb_buf[3*(i * width + j) + 1], rgb_buf[3*(i*width + j) + 2]);
    }
    fprintf(outfile, "\n");
  }

  fclose(outfile);
  return 0;
}

   
int writeEnergyMapOut(double* energy, char* filename)
{
  FILE * outfile;

  outfile = fopen(filename, "w");

  if(outfile == NULL)
  {
    return -1;
  }

  int i, j;

  for(i = 0; i < height; i++)
  {
    for(j = 0; j < width; j++)
    {
      fprintf(outfile, "%f ", energy[i * width + j]);
    }
    fprintf(outfile, "\n");
  }

  fclose(outfile);
  return 0;
}

int writeSeamMapOut(double* map, char* filename)
{
  FILE * outfile;

  outfile = fopen(filename, "w");

  if(outfile == NULL)
  {
    return -1;
  }

  int i, j;

  for(i = 0; i < height; i++)
  {
    for(j = 0; j < width; j++)
    {
      fprintf(outfile, "%f ", map[i * width + j]);
    }
    fprintf(outfile, "\n");
  }

  fclose(outfile);
  return 0;
}
