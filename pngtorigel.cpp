#include <pngwriter.h>

#include <stdlib.h>
#include <string>

#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf("Usage: pngtorigel [input file] [output file]\n");
		return 0;
	}

	int width;
	int height;
	ofstream outFile;

	char* inFilename = argv[1];
	char* outFilename = argv[2];

	pngwriter image;
	image.readfromfile(inFilename);
	width = image.getwidth();
	height = image.getheight();

	outFile.open(outFilename);
	outFile << width << " " << height << "\n";

	// Copy the PNG data.
	for(int i = 0; i < height; i++)
	{
		for(int j = 0; j < width; j++)
		{
			outFile << image.read(j, i, 1) << " " << image.read(j, i, 2) << " " << image.read(j, i, 3) << " ";	
		}
		outFile << "\n";
	}

	image.close();
	outFile.close();
	return 0;
}

		
