/*aes
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
#include <openssl/aes.h>

int cipherAES(void) {
	unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char text[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	unsigned char cipher[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	AES_KEY enc_key;
	int i = 0;

	AES_set_encrypt_key(key, 128, &enc_key);
	AES_encrypt(text, cipher, &enc_key);

	printf("Clear:\t");
	for (i=0; i<16; i++)
		printf("%.2x", text[i]);
	printf("\n");

	printf("Key:\t");
	for (i=0; i<16; i++)
		printf("%.2x", key[i]);
	printf("\n");

	printf("Cipher:\t");
	for (i=0; i<16; i++)
		printf("%.2x", cipher[i]);
	printf("\n");

	return 0;
}


int main(int argc, char *argv[]) {
	switch (argc) {
		case 1:
		printf("Michel Dubois -- %s\n", argv[0]);
			cipherAES();
			exit(EXIT_SUCCESS);
			break;
		default:
			exit(EXIT_FAILURE);
			break;	
		}
}
