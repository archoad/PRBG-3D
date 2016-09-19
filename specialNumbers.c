/*specialNumbers
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define couleur(param) printf("\033[%sm",param)
#define MAXSAMPLE 30000000

static unsigned long long tempTab[MAXSAMPLE];
static unsigned long long listNumbers[MAXSAMPLE];
static int sampleSize=0, size=0;




void usage(void) {
	couleur("31");
	printf("Michel Dubois -- specialNumbers -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: specialNumbers <name> <sample size>\n");
	printf("\t<name> -> name of the algorithm\n");
	printf("\t\tavalaible algorithms are: prime, semiprime, blum, euler, fibonacci, fredkin\n");
	printf("\t<sample size> -> sample size\n");
}


unsigned long long factorial(unsigned long long n) {
	unsigned long long i=0, result=1;
	for (i=1; i<=n; i++) {
		result *= i;
	}
	return(result);
}


unsigned long long gcd(unsigned long long n1, unsigned long long n2) {
	unsigned long long tmp;
	while (n1 != 0) {
		tmp = n1;
		n1 = n2 % n1;
		n2 = tmp;
	}
	return n2;
}


unsigned long long eulerTotient(unsigned long long n) {
	// www.answers.com/topic/euler-s-totient-function
	unsigned long long result = 0, k = 0;
	for (k=0; k<n; k++) {
		if (gcd(k,n) == 1)
			result++;
	}
	return result;
}


void printData(void) {
	int i;
	for (i=0; i<size; i++) {
		printf("%lli, ", listNumbers[i]);
	}
	printf("\n%d\n", size);
}


void saveData(void) {
	int i;
	FILE *fd = fopen("result.dat", "w");
	if (fd != NULL) {
		printf("INFO: file create\n");
		for (i=0; i<size; i++) {
			fprintf(fd, "%lli\n", listNumbers[i]);
		}
		fclose(fd);
		printf("INFO: file close\nINFO: data saved in result.dat\n");
	} else {
		printf("INFO: open error\n");
	}
}


void bubbleSort(void) {
	int i, j, t;

	for (i=0; i<size; i++) {
		tempTab[i] = listNumbers[i];
	}
	for (i=size-1; i>=0; i--) {
		for (j=0; j<i; j++) {
			if(tempTab[j] > tempTab[j+1]) {
				t = tempTab[j];
				tempTab[j] = tempTab[j+1];
				tempTab[j+1] = t;
			}
		}
	}
	for (i=0; i<size; i++) {
		listNumbers[i] = tempTab[i];
	}
}



void cribleEratosthene(int n) {
	int i, max=round(sqrt(n)), cpt=0, nbrPrime=0;

	// initialisation du tableau
	tempTab[0] = 0;
	tempTab[1] = 0;
	for (i=2; i<n; i++) {
		tempTab[i] = 1;
	}
	// recherche des nombres premiers
	for (i=2; i<=max; i++) {
		if (tempTab[i] == 1) {
			cpt = i;
			while (i*cpt<=n) {
				tempTab[i*cpt] = 0;
				cpt++;
			}
		}
	}
	// calcul du nombre de nombre premier trouvé
	for (i=0; i<n; i++) {
		if (tempTab[i] == 1) {
			nbrPrime++;
		}
	}
	// écriture du tableau de nombres premiers
	cpt=0;
	for (i=0; i<n; i++) {
		if (tempTab[i] == 1) {
			listNumbers[cpt] = i;
			cpt++;
		}
	}
	size = nbrPrime;
}


void primeNumber(int n) {
	// affiche la liste des nombres premiers <n
	// https://en.wikipedia.org/wiki/Sieve_of_Eratosthenes
	// http://mathworld.wolfram.com/SieveofEratosthenes.html

	printf("Prime numbers (https://oeis.org/A000040) less than %d\n", n);
	printf("A number n is prime if it is greater than 1 and has no positive divisors except 1 and n.\n");
	cribleEratosthene(n);
}


void semiPrimeNumber(int n) {
	// affiche la liste des nombres semi premiers <n
	// https://en.wikipedia.org/wiki/Semiprime
	// http://mathworld.wolfram.com/Semiprime.html

	int i, j, tmp, cpt=0;

	printf("Semiprimes (https://oeis.org/A001358) less than %d\n", n);
	printf("Numbers of the form p*q where p and q are primes, not necessarily distinct.\n");
	cribleEratosthene(n);
	for (i=0; i<size; i++) {
		for (j=i; j<size; j++) {
			tmp = listNumbers[i] * listNumbers[j];
			if (tmp <= n) {
				tempTab[cpt] = tmp;
				cpt++;
			}
		}
	}

	for (i=0; i<cpt; i++) {
		listNumbers[i] = tempTab[i];
	}
	size = cpt;
	bubbleSort();
}


void blumNumber(int n) {
	// affiche la liste des nombres de Blum <n
	// Blum numbers: of form p*p where p & q are distinct primes congruent to 3 (mod 4)
	// https://en.wikipedia.org/wiki/Blum_integer
	// http://www.gilith.com/research/talks/cambridge1997.pdf
	// http://cseweb.ucsd.edu/~mihir/papers/gb.pdf

	int i, j, cpt=0, val=0;

	printf("Blum numbers (https://oeis.org/A016105) less than %d\n", n);
	printf("nNumbers of the form p * q where p and q are distinct primes congruent to 3 (mod 4).\n");
	cribleEratosthene(n);
	for (i=0; i<size; i++) {
		for (j=i+1; j<size; j++) {
			if ((listNumbers[i] % 4 == 3) && (listNumbers[j] % 4 == 3)) {
				val = listNumbers[i] * listNumbers[j];
				if (val <= n) {
					tempTab[cpt] = val;
					cpt++;
				}
			}
		}
	}

	for (i=0; i<cpt; i++) {
		listNumbers[i] = tempTab[i];
	}
	size = cpt;
	bubbleSort();
}


void eulerNumber(int n) {
	// Euler totient function phi(n): count numbers <= n and prime to n.
	// https://en.wikipedia.org/wiki/Euler%27s_totient_function
	// http://mathworld.wolfram.com/TotientFunction.html
	int i;

	printf("Totient numbers (https://oeis.org/A000010) less than %d\n", n);
	for (i=0; i<n; i++) {
		listNumbers[i] = eulerTotient(i+1);
	}
	size = i;
}


void fibonacciNumber(int n) {
	int i;

	printf("Fibonacci numbers (https://oeis.org/A000045)\n");
	for (i=0; i<n; i++) {
		if (i==0) { listNumbers[i] = 0; }
		else if (i==1) { listNumbers[i] = 1; }
		else { listNumbers[i] = listNumbers[i-1] + listNumbers[i-2]; }
	}
	size = i;
}


void fredkinNumber(int n) {
	int i;

	printf("Fredkin replicator (https://oeis.org/A160239)\n");
	listNumbers[0] = 1;
	for (i=0; i<n; i++) {
		listNumbers[2*i] = listNumbers[i];
		listNumbers[4*i+1] = 8 * listNumbers[i];
		listNumbers[4*i+3] = 2 * listNumbers[2*i+1] + 8 * listNumbers[i];
	}
	size = i;
}


int main(int argc, char *argv[]) {
	switch (argc) {
		case 3:
			sampleSize = atoi(argv[2]);
			if (sampleSize<=MAXSAMPLE) {
				if (!strncmp(argv[1], "prime", 5)) {
					primeNumber(sampleSize);
				} else if (!strncmp(argv[1], "semiprime", 10)) {
					semiPrimeNumber(sampleSize);
				} else if (!strncmp(argv[1], "blum", 4)) {
					blumNumber(sampleSize);
				} else if (!strncmp(argv[1], "euler", 5)) {
					eulerNumber(sampleSize);
				} else if (!strncmp(argv[1], "fibonacci", 9)) {
					fibonacciNumber(sampleSize);
				} else if (!strncmp(argv[1], "fredkin", 7)) {
					fredkinNumber(sampleSize);
				} else {
					usage();
					exit(EXIT_FAILURE);
					break;
				}
				printData();
				saveData();
				exit(EXIT_SUCCESS);
			} else {
				printf("Sample size exceeded (authorized max sample: %d)\n", MAXSAMPLE);
				usage();
				exit(EXIT_FAILURE);
			}
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;
		}
	return 0;
}
