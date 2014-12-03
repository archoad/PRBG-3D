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


typedef struct data_struct {
	long *list;
	long size;
} data;


void usage(void) {
	couleur("31");
	printf("Michel Dubois -- specialNumbers -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: specialNumbers <name> <sample size>\n");
	printf("\t<name> -> name of the algorithm\n");
	printf("\t\tavalaible algorithms are: prime, semiprime, blum, euler\n");
	printf("\t<sample size> -> sample size\n");
}


long calculateGCD(long n1, long n2) {
	long tmp;
	while (n1 != 0) {
		tmp = n1;
		n1 = n2 % n1;
		n2 = tmp;
	}
	return n2;
}


long eulerTotient(long n) {
	// www.answers.com/topic/euler-s-totient-function
	long result = 0, k = 0;
	for (k=0; k<n; k++) {
		if (calculateGCD(k,n) == 1)
			result++;
	}
	return result;
}


void printData(data primes) {
	long i;
	for (i=0; i<primes.size; i++) {
		printf("%ld, ", primes.list[i]);
	}
	printf("\n%ld\n", primes.size);
}


void saveData(data primes) {
	long i;
	FILE *fic = fopen("result.dat", "w");
	if (fic != NULL) {
		printf("INFO: file create\n");
		for (i=0; i<primes.size; i++) {
			fprintf(fic, "%ld\n", primes.list[i]);
		}
		fclose(fic);
		printf("INFO: file close\nINFO: data saved in result.dat\n");
	} else {
		printf("INFO: open error\n");
	}
}


data bubbleSort(data structure) {
	long i, j, t, len=structure.size, *a = NULL;
	data result;
	a = malloc(len * sizeof(long));
	for (i=0; i<len; i++) {
		a[i] = structure.list[i];
	}
	for (i=len-1; i>=0; i--) {
		for (j=0; j<i; j++) {
			if(a[j] > a[j+1]) {
				t = a[j];
				a[j] = a[j+1];
				a[j+1] = t;
			}
		}
	}
	result.list = a;
	result.size = len;
	return result;
}


data cribleEratosthene(int n) {
	// affiche la liste des nombres premiers <n
	// https://en.wikipedia.org/wiki/Sieve_of_Eratosthenes
	// http://mathworld.wolfram.com/SieveofEratosthenes.html
	long tab[n], i, max=round(sqrt(n)), val=0, nbrPrime=0, *result = NULL;
	data temp;
	printf("Prime numbers (https://oeis.org/A000040)\nLess than %d\n", n);
	// initialisation du tableau
	tab[0] = 0;
	tab[1] = 0;
	for (i=2; i<n; i++) {
		tab[i] = 1;
	}
	// recherche des nombres premiers
	for (i=2; i<=max; i++) {
		if (tab[i] == 1) {
			val = i;
			while (i*val<=n) {
				tab[i*val] = 0;
				val++;
			}
		}
	}
	// calcul du nombre de nombre premier trouvé
	for (i=0; i<n; i++) {
		if (tab[i] == 1) {
			nbrPrime++;
		}
	}
	// écriture du tableau de nombres premiers
	result = malloc(nbrPrime * sizeof(long));
	val=0;
	for (i=0; i<n; i++) {
		if (tab[i] == 1) {
			result[val] = i;
			val++;
		}
	}
	temp.list = result;
	temp.size = nbrPrime;
	return temp;
}


data semiPrimeNumber(int n) {
	// affiche la liste des nombres semi premiers <n
	// https://en.wikipedia.org/wiki/Semiprime
	// http://mathworld.wolfram.com/Semiprime.html
	data primes = cribleEratosthene(n);
	long i, j, tmp, len=primes.size, *result = NULL, cpt=0;
	result = malloc(sizeof(long));
	data temp;
	printf("Semiprimes (https://oeis.org/A001358)\nLess than %d\n", n);
	for (i=0; i<len; i++) {
		for (j=i; j<len; j++) {
			tmp = primes.list[i] * primes.list[j];
			if (tmp <= n) {
				result[cpt] = tmp;
				cpt++;
				result = realloc(result, sizeof(long) * (cpt + 1));
			}
		}
	}
	temp.list = result;
	temp.size = cpt;
	return bubbleSort(temp);
}


data blumNumber(int n) {
	// affiche la liste des nombres de Blum <n
	// Blum numbers: of form p*p where p & q are distinct primes congruent to 3 (mod 4)
	// https://en.wikipedia.org/wiki/Blum_integer
	// http://www.gilith.com/research/talks/cambridge1997.pdf
	// http://cseweb.ucsd.edu/~mihir/papers/gb.pdf
	data primes = cribleEratosthene(n);
	long i, j, len=primes.size, *result = NULL, cpt=0, val=0;
	result = malloc(sizeof(long));
	data temp;
	printf("Blum numbers (https://oeis.org/A016105)\nLess than %d\n", n);
	for (i=0; i<len; i++) {
		for (j=i+1; j<len; j++) {
			if ((primes.list[i] % 4 == 3) && (primes.list[j] % 4 == 3)) {
				val = primes.list[i] * primes.list[j];
				if (val <= n) {
					result[cpt] = val;
					cpt++;
					result = realloc(result, sizeof(long) * (cpt + 1));
				}
			}
		}
	}
	temp.list = result;
	temp.size = cpt;
	return bubbleSort(temp);
}


data eulerNumber(int n) {
	// Euler totient function phi(n): count numbers <= n and prime to n. 
	// https://en.wikipedia.org/wiki/Euler%27s_totient_function
	// http://mathworld.wolfram.com/TotientFunction.html
	long i, *result = NULL;
	result = malloc(n * sizeof(long));
	data temp;
	printf("Totient numbers (https://oeis.org/A000010)\nLess than %d\n", n);
	for (i=0; i<n; i++) {
		result[i] = eulerTotient(i+1);
	}
	temp.list = result;
	temp.size = i;
	return temp;
}


void testBlumNumbers(int n) {
	data numbers;
	numbers = blumNumber(n);
	long i, j, len = numbers.size, temp = 0, less = 100000;
	for (i=0; i<len; i++) {
		for (j=i; j<len; j++) {
			temp = calculateGCD( eulerTotient(numbers.list[i]-1), eulerTotient(numbers.list[j]-1) );
			printf("(%ld, %ld) -> %ld\n", numbers.list[i], numbers.list[j], temp);
			if (temp < less)
				less = temp;
		}
	}
	printf("%ld\n", less);
}


int main(int argc, char *argv[]) {
	data numberList;
	switch (argc) {
		case 3:
			if (!strncmp(argv[1], "prime", 5)) {
				numberList = cribleEratosthene(atoi(argv[2]));
			} else if (!strncmp(argv[1], "semiprime", 10)) {
				numberList = semiPrimeNumber(atoi(argv[2]));
			} else if (!strncmp(argv[1], "blum", 4)) {
				numberList = blumNumber(atoi(argv[2]));
			} else if (!strncmp(argv[1], "euler", 5)) {
				numberList = eulerNumber(atoi(argv[2]));
			} else {
				usage();
				exit(EXIT_FAILURE);
				break;	
			}
			printData(numberList);
			saveData(numberList);
			exit(EXIT_SUCCESS);
			break;
		case 2:
			testBlumNumbers(atoi(argv[1]));
 			exit(EXIT_SUCCESS);
 			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;	
		}
	return 0;
}