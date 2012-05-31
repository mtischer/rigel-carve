////////////////////////////////////////////////////////////////////////////////
// SVA
////////////////////////////////////////////////////////////////////////////////
// A simple RTM-based test with golden outputs!
// Adds two dynamically allocated arrays that are initialized in a simple
// fashion
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>

#define INT_MAX 0x7FFFFFFF;

int width, height;
int * rgb_buffer;
double * energy;
double * map;
volatile int VECSIZE;

////
// Functions.
////
double min2(double a, double b);
double min(double a, double b, double c);
double compute_e(int* buf, int x, int y, int w, int h);

int colorMinPath(double* e_buf, int* rgb_buf);
int writeImageOut(int* rgb_buf);

////////////////////////////////////////////////////////////////////////////////
// simple Scaled Vector Addition Testcode
////////////////////////////////////////////////////////////////////////////////

// simple task, add the specified ranges
void DoEnergy(int i) {
	int y = i / width;
	int x = i % width;
	energy[i] = compute_e(rgb_buffer, x, y, width, height);
}


////////////////////////////////////////////////////////////////////////////////
// enqueue and dequeue/distribute/dispatch tasks
////////////////////////////////////////////////////////////////////////////////
void energy_map(int thread, int tasks_per_thread) {
  int i;
  int begin = thread*tasks_per_thread;
  int end = thread*tasks_per_thread+tasks_per_thread;
	for( i = begin; i < end; i++ ){
    DoEnergy(i);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////
////	START MAIN
////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int main() {
  int i=0;
  int j=0;
  int x=0;
  int y=0;
  int core = 0;
  int thread = 0;
  int numthreads = 1;
  int tasks_per_thread;
  if (core == 0 && thread == 0) {
    
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

    // Allocate space for the file stuff.
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

    printf("starting!\n");
    
    // Start full run timer


  }

  // ready, set, GO!
  tasks_per_thread = width*height/numthreads;
  energy_map(thread, tasks_per_thread);

  
  // Actually compute seams and carve.

  if(core == 0 && thread == 0)
  {
    map = (double *)malloc(sizeof(double)*width*height); 	

    // Generate first row.
    for(x = 0; x < width; x++)
    {
      map[x] = energy[x];
    }

    for(y = 1; y < height; y++)
    {
      for(x = 0; x < width; x++)
      {
        if(x == 0)
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
    }
  }
  // Stopfull run timer
  if (core == 0 && thread == 0) {
    colorMinPath(map, rgb_buffer);
    writeImageOut(rgb_buffer);
  }

	return 0;
}

//////
// Helper functions.
/////

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

int colorMinPath(double* e_buf, int* rgb_buf)
{
	int x_min = -1;
	int e_min = INT_MAX;
	int x_curr = 0;
	int y_curr = height - 1;
	// Determine the seam with the lowest energy value.
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

	printf("The minimum energy is %d\n", e_min);
	
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

int writeImageOut(int* rgb_buf)
{
    FILE * outfile;

    outfile = fopen("output.pngdata", "w");

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
