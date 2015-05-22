/*picture
Copyright (C) 2013 Michel Dubois

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.*/



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define couleur(param) printf("\033[%sm",param)


/* Sources
http://www.fileformat.info/format/tga/egff.htm
https://fr.wikipedia.org/wiki/Truevision_Targa
*/


typedef struct _targaHeader {
	unsigned char id_length;
	unsigned char cmap_type;
	unsigned char img_type;
	// Color map specification (5 bytes)
	short cmap_start;
	short cmap_length;
	unsigned char cmap_depth;
	// Image specification (10 bytes)
	unsigned char originx;
	unsigned char originy;
	short width;
	short height;
	unsigned char pixel_depth;
	unsigned char img_descriptor;
} targaHeader;


void usage(void) {
	couleur("31");
	printf("Michel Dubois -- picture -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: picture <filename>\n");
	printf("\t<filename> -> TGA file to read\n");
}


char treatBits(char c1, char c2) {
	char tmp;

	if ((c1 == '0') & (c2 == '0')) tmp = '0';
	if ((c1 == '0') & (c2 == '1')) tmp = '0';
	if ((c1 == '1') & (c2 == '0')) tmp = '1';
	if ((c1 == '1') & (c2 == '1')) tmp = '1';
	return(tmp);
}


char* convertByteToBin(unsigned char byte) {
	int n = 0;
	char *result = malloc((8+1) * sizeof(char));

	for (n=0; n<8; n++) {
		if ((byte & 0x80) != 0) {
			result[n] = '1';
		} else {
			result[n] = '0';
		}
		byte = byte << 1;
	}
	result[8] = '\0';
	return(result);
}


unsigned char convertBinToByte(char *binary) {
	unsigned char c;

	c = strtol(binary, 0, 2);
	return(c);
}


void printTargaHeader(targaHeader *header) {
	printf("%d (%02x)\n", header->id_length, header->id_length);
	printf("%d (%02x)\n", header->cmap_type, header->cmap_type);
	printf("%d (%02x) -> img type\n", header->img_type, header->img_type);
	printf("\n");
	printf("%d (%04x)\n", header->cmap_start, header->cmap_start);
	printf("%d (%04x)\n", header->cmap_length, header->cmap_length);
	printf("%d (%02x)\n", header->cmap_depth, header->cmap_depth);
	printf("\n");
	printf("%d (%02x) -> origine x\n", header->originx, header->originx);
	printf("%d (%02x) -> origine y\n", header->originy, header->originy);
	printf("%d (%04x) -> width\n", header->width, header->width);
	printf("%d (%04x) -> height\n", header->height, header->height);
	printf("%d (%02x) -> bits per pixel\n", header->pixel_depth, header->pixel_depth);
	printf("%d (%02x)\n", header->img_descriptor, header->img_descriptor);
}


targaHeader* getTargaHeader(char *filename) {
	targaHeader *header = malloc(sizeof(targaHeader));
	FILE *file = NULL;

	file = fopen(filename, "r");
	fread(header, sizeof(targaHeader), 1, file);
	fclose(file);
	return(header);
}


unsigned char* readTarga(char *filename) {
	targaHeader *header = malloc(sizeof(targaHeader));
	FILE *file = fopen(filename, "r");
	unsigned char *data = NULL;
	int bytesToRead = 0,
		cpt = 0;

	fread(header, sizeof(targaHeader), 1, file);
	bytesToRead = header->pixel_depth / 8;
	data = (unsigned char*)malloc(header->width * header->height * sizeof(unsigned char));
	while (cpt < header->width * header->height) {
		fread(&data[cpt], bytesToRead, 1, file);
		cpt++;
	}
	fclose(file);
	free(header);
	return(data);
}


unsigned char* insertMessage(unsigned char* data, char *msg) {
	int i = 0,
		j = 0,
		cpt = 0,
		msgLength = 0;
	char c;
	char *byte = NULL,
		*curPixel = NULL;

	msgLength = (int)strlen(msg);
	printf("INFO: Insert message '%s' (%d)\n", msg, msgLength);
	for (i=0; i<msgLength; i++) {
		c = msg[i];
		byte = convertByteToBin(c);
		for (j=0; j<8; j++) {
			curPixel = convertByteToBin(data[cpt]);
			curPixel[7] = treatBits(byte[j], curPixel[7]);
			data[cpt] = convertBinToByte(curPixel);
			cpt++;
		}
	}
	free(byte);
	free(curPixel);
	return(data);
}


char* extractMessage(unsigned char* data, int length) {
	int i = 0,
		j = 0,
		cpt = 0;
	char *tmp,
		*result = malloc((8) * sizeof(char)),
		*msg = malloc((length+1) * sizeof(char));

	for (i=0; i<length; i++) {
		for (j=0; j<8; j++) {
			tmp = convertByteToBin(data[cpt]);
			result[j] = tmp[7];
			cpt++;
		}
		msg[i] = convertBinToByte(result);
	}
	msg[length+1] = '\0';
	free(tmp);
	free(result);
	return(msg);
}


void createStatsFile(char *filename, char *destFile) {
	FILE *file = fopen(destFile, "w");
	targaHeader *header = NULL;
	unsigned char *data = NULL;
	int cpt = 0;

	if (file != NULL) {
		printf("INFO: file %s open\n", destFile);
		header = getTargaHeader(filename);
		data = readTarga(filename);
		while (cpt < header->width * header->height) {
			fprintf(file, "%d\n", data[cpt]);
			cpt++;
		}
		fclose(file);
		free(header);
		free(data);
		printf("INFO: file %s closed\n", destFile);
	} else {
		printf("INFO: open error\n");
		exit(EXIT_FAILURE);
	}
}


void createStegImg(char *source, char *destination, char *message) {
	FILE *file = fopen(destination, "w");
	targaHeader *header = NULL;
	unsigned char *data = NULL;
	unsigned char p = 0;
	int cpt = 0;

	if (file != NULL) {
		printf("INFO: file %s open\n", destination);
		printf("%s\n", message);
		header = getTargaHeader(source);
		data = readTarga(source);
		data = insertMessage(data, message);
		header->cmap_start = (int)strlen(message);
		fwrite(header, sizeof(targaHeader), 1, file);
		while (cpt < header->width * header->height) {
			p = data[cpt];
			fwrite(&p, sizeof(unsigned char), 1, file);
			cpt++;
		}
		free(header);
		free(data);
		fclose(file);
		printf("INFO: file %s closed\n", destination);
	} else {
		printf("INFO: open error\n");
		exit(EXIT_FAILURE);
	}
}


void readStegImg(char *filename) {
	targaHeader *header = NULL;
	unsigned char *data = NULL;
	char *message = NULL;
	int msgLength = 0;

	header = getTargaHeader(filename);
	msgLength = header->cmap_start;
	if (msgLength) {
		printf("INFO: file %s contains a message of %d bytes length\n", filename, msgLength);
		data = readTarga(filename);
		message = extractMessage(data, msgLength);
		printf("%s\n", message);
	} else {
		printf("INFO: file %s doesn't contain message\n", filename);
	}
}


FILE* initiateTarga(char *filename, int width, int height) {
	FILE *file = NULL;
	targaHeader header;

	srand(time(NULL));

	header.id_length = 0;
	header.cmap_type = 0;
	header.img_type = 3;

	header.cmap_start = 0;
	header.cmap_length = 0;
	header.cmap_depth = 0;

	header.originx = 0;
	header.originy = 0;
	header.width = width;
	header.height = height;
	header.pixel_depth = 8;
	header.img_descriptor = 0;

	file = fopen(filename, "w");
	fwrite(&header, sizeof(targaHeader), 1, file);
	fflush(file);
	return(file);

}


void createTarga(char *filename) {
	FILE *file = NULL;
	int cpt = 0,
		width = 1000,
		height = 1000;
	unsigned char p, ps;

	file = initiateTarga(filename, width, height);

	while (cpt < width * height) {
		p = cpt % 256;
		ps = p ^ 0x01;
		//printf("%d 0x%02x 0b%s 0b%s\n", p, p, convertByteToBin(p), convertByteToBin(ps));
		fwrite(&p, sizeof(unsigned char), 1, file);
		cpt++;
	}
	fclose(file);
}


int main(int argc, char *argv[]) {
	switch (argc) {
		case 2:
			createStatsFile(argv[1], "resultOrig.dat");
			createStegImg(argv[1], "picts/stegano.tga", "Labor omnia vincit improbus.");
			createStatsFile("picts/stegano.tga", "resultSteg.dat");
			readStegImg("picts/stegano.tga");
			createTarga("picts/my_targa.tga");
			exit(EXIT_SUCCESS);
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;	
		}
}



