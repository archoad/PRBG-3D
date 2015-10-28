/*pi
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
#include <time.h>
#include <string.h>
#include <gmp.h>

#define couleur(param) printf("\033[%sm",param)

static unsigned long sampleSize = 0;

mpz_t tmp1,
	tmp2,
	t5,
	t239,
	pows;


void usage(void) {
	couleur("31");
	printf("Michel Dubois -- pi -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: pi <num>\n");
	printf("\t<num> -> sample size\n");
}


void actan(mpz_t res, unsigned long base, mpz_t pows) {
	int i, neg = 1;
	mpz_tdiv_q_ui(res, pows, base);
	mpz_set(tmp1, res);
	for (i = 3; ; i += 2) {
		mpz_tdiv_q_ui(tmp1, tmp1, base * base);
		mpz_tdiv_q_ui(tmp2, tmp1, i);
		if (mpz_cmp_ui(tmp2, 0) == 0) {
			break;
		}
		if (neg) {
			mpz_sub(res, res, tmp2);
		} else {
			mpz_add(res, res, tmp2);
		}
		neg = !neg;
	}
}


char* get_digits(int n) {
	mpz_ui_pow_ui(pows, 10, n + 20);

	actan(t5, 5, pows);
	mpz_mul_ui(t5, t5, 16);

	actan(t239, 239, pows);
	mpz_mul_ui(t239, t239, 4);

	mpz_sub(t5, t5, t239);
	mpz_ui_pow_ui(pows, 10, 20);
	mpz_tdiv_q(t5, t5, pows);

	return mpz_get_str(0, 0, t5);
}


void calculatePi(void) {
	unsigned long i = 0;
	char *s;
	FILE *fic = NULL;
	clock_t launch, done;

	mpz_init(tmp1);
	mpz_init(tmp2);
	mpz_init(t5);
	mpz_init(t239);
	mpz_init(pows);

	launch = clock();
	s = get_digits(sampleSize);
	done = clock();
	printf("%s (%lu)\n", s, strlen(s));
	printf("INFO: Time duration: %f\n", (double)(done-launch)/CLOCKS_PER_SEC);
	fic = fopen("result.dat", "w");
	if (fic != NULL) {
		printf("INFO: file create\n");
		for (i=1; i<sampleSize+1; i++) {
			fprintf(fic, "%c\n", s[i]);
		}
		fclose(fic);
		printf("INFO: file close\nINFO: data saved in result.dat\n");
	}
}


int main(int argc, char *argv[]) {
	switch (argc) {
		case 2:
			sampleSize = atol(argv[1]);
			calculatePi();
			exit(EXIT_SUCCESS);
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;	
		}
}



