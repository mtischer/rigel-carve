#include <pngwriter.h>

#include <stdlib.h>
#include <string>
#include <stdio.h>

using namespace std;

int main(int argc, char* argv[])
{
        int width;
        int height;
	int r, g, b;
        FILE * outfile;

	char * inFilename;
	char * outFilename;

	if(argc != 3)
	{
		printf("Usage: rigeltopng [input file] [output file]\n");
		return 0;
	}

	inFilename = argv[1];
	outFilename = argv[2];

	outfile = fopen((char*)inFilename, "r");

	if(outfile == NULL)
	{
		return -1;
	}

	fscanf(outfile, "%d %d", &width, &height);
	
	pngwriter image(width, height, 0.0, outFilename);
	
        // Copy the PNG data.
        for(int i = 0; i < height; i++)
        {
                for(int j = 0; j < width; j++)
                {/*
			if(r == 0 || g == 0 || b==0)
			{
				printf("Pixel (%d,%d)has value %d %d %d\n", j, i, r, g, b); 
			}*/
			fscanf(outfile, "%d %d %d ", &r, &g, &b);
			image.plot((j+1), (i+1), r, g, b);
                }
        }

        image.close();
        fclose(outfile);
        return 0;
}


