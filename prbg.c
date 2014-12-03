/*prbg
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

// inspired from http://lcamtuf.coredump.cx/oldtcp/tcpseq.html
// http://www.mpipks-dresden.mpg.de/~tisean/TISEAN_2.1/docs/chaospaper/node6.html

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define couleur(param) printf("\033[%sm",param)


static unsigned long sampleSize = 0;




void usage(void) {
	couleur("31");
	printf("Michel Dubois -- prbg -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: prbg <filename> <num> <algo>\n");
	printf("\t<filename> -> file where the results of the algorithm will be stored\n");
	printf("\t<num> -> sample size\n");
	printf("\t<algo> -> 'alea', 'logistic', 'lin_cong', 'sinus', 'mid_square', 'blumblum'\n");
}


long modulus(long x, long y) {
	return (x - (x / y) * y);
}


char *dec2bin(long n) {
	int c, k = 0;
	char *result;
	result = malloc(33 * sizeof(char));
	result[32] = '\0';
	for (c=0; c<=31; c++) {
		k = n >> c;
		result[31-c] = (k & 1) ? '1' : '0';
	}
	return result;
}


double generateTrueAlea(void) {
	return rand();
}


double generateLogisticMap(void) {
	//static float xn = 0.2837462593; // [0 .. 1]
	static double xn = 0.7364738523; // [0 .. 1]
	static double lambda = 3.8; // [1 .. 4]
	double value = xn;
	value = xn * pow(10,12);
	xn = (lambda * xn) * (1 - xn);
	lambda -= 0.0001;
	return value;
}


double generateLinearCongruence(void) {
	static double xn = 1;
	double value = xn;
	xn = fmod((5 * xn + 3), 4096); //equivalent to (5 * xn + 3) % 4096
	return value;
}


double generateSinusoidalAlea(void) {
	static double xn = 1;
	double value = xn;
	xn = 2 * cos(xn);
	if (xn < 0) xn = xn * - 1;
	return value * 100000000000;
}


double generateMiddleSquare(void) {
	// https://en.wikipedia.org/wiki/Middle-square_method
	static double xn = 16752482;
	double value = xn;
	xn = xn * xn;
	xn = fmod((xn / 10000), 100000000); // equivalent to (xn / 10000) % 100000000
	return value;
}


double generateBlumBlumShub(void) {
	// https://en.wikipedia.org/wiki/Blum_Blum_Shub
	// x0 = (s*s) mod(n) avec n=p.q et p, q nombre de Blum
	// p = 869393, q = 589497, n = p.q = 512504565321
	// s in [1, n-1] -> s= 412504565321
	double n = 512504565321;
	static double xn = 496880664266; // result of fmod(pow(s, 2), n)
	double value = xn;
	xn = fmod(pow(xn, 2), n); // equivalent to (xn * xn) % n
	return value;
}


void createFile(char *argv[]) {
	unsigned long i = 0;
	double alea = 0;
	FILE *fic = fopen(argv[1], "w");
	if (fic != NULL) {
		srand(time(NULL));
		printf("INFO: file create\n");
		if (strcmp(argv[3], "alea") == 0) {
			for (i=0; i<sampleSize; i++) {
				alea = generateTrueAlea();
				fprintf(fic, "%lf\n", alea);
			}
		} else if (strcmp(argv[3], "logistic") == 0) {
			for (i=0; i<sampleSize; i++) {
				alea = generateLogisticMap();
				fprintf(fic, "%lf\n", alea);
			}
		} else if (strcmp(argv[3], "lin_cong") == 0) {
			for (i=0; i<sampleSize; i++) {
				alea = generateLinearCongruence();
				fprintf(fic, "%lf\n", alea);
			}
		} else if (strcmp(argv[3], "sinus") == 0) {
			for (i=0; i<sampleSize; i++) {
				alea = generateSinusoidalAlea();
				fprintf(fic, "%lf\n", alea);
			}
		} else if (strcmp(argv[3], "mid_square") == 0) {
			for (i=0; i<sampleSize; i++) {
				alea = generateMiddleSquare();
				fprintf(fic, "%lf\n", alea);
			}
		} else if (strcmp(argv[3], "blumblum") == 0) {
			for (i=0; i<sampleSize; i++) {
				alea = generateBlumBlumShub();
				fprintf(fic, "%lf\n", alea);
				//printf("%ld -> %s\n", alea, dec2bin(alea));
			}
		} else {
			fclose(fic);
			usage();
			exit(EXIT_FAILURE);
		}
		fclose(fic);
		printf("INFO: file close\n");
	} else {
		printf("INFO: open error\n");
		exit(EXIT_FAILURE);
	}
}


int main(int argc, char *argv[]) {
	switch (argc) {
		case 4:
			sampleSize = atol(argv[2]);
			createFile(argv);
			exit(EXIT_SUCCESS);
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;	
		}
}



