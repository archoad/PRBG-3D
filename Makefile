# définition des cibles particulières
.PHONY: clean, mrproper
  
# désactivation des règles implicites
.SUFFIXES:

UNAME_S:=$(shell uname -s)

CC=gcc
CL=clang
CFLAGS= -O3 -Wall -W -Wstrict-prototypes -Werror
ifeq ($(UNAME_S),Linux)
	IFLAGSDIR= -I/usr/include
	LFLAGSDIR= -L/usr/lib
	COMPIL=$(CC)
endif
ifeq ($(UNAME_S),Darwin)
	IFLAGSDIR= -I/opt/local/include
	LFLAGSDIR= -L/opt/local/lib
	COMPIL=$(CL)
endif
GL_FLAGS= -lGL -lGLU -lglut
NET_FLAGS= -lnet -lpcap
MATH_FLAGS= -lm
AES_FLAGS= -lcrypto
PNG_FLAGS= -lpng

all: dest_sys visualize3d specialNumbers network prbg aes

visualize3d: visualize3d.c
	$(COMPIL) $(CFLAGS) $(IFLAGSDIR) $(LFLAGSDIR) $(GL_FLAGS) $(PNG_FLAGS) $< -o $@

network: network.c
	$(COMPIL) $(CFLAGS) $(IFLAGSDIR) $(LFLAGSDIR) $(NET_FLAGS) $< -o $@

specialNumbers: specialNumbers.c
	$(COMPIL) $(CFLAGS) $(MATH_FLAGS) $< -o $@

prbg: prbg.c
	$(COMPIL) $(CFLAGS) $(MATH_FLAGS) $< -o $@

aes: aes.c
	$(COMPIL) $(CFLAGS) $(IFLAGSDIR) $(LFLAGSDIR) $(AES_FLAGS) $< -o $@

dest_sys:
	@echo "Destination system:" $(UNAME_S)

clean:
	@rm -f visualize3d
	@rm -f specialNumbers
	@rm -f network
	@rm -f prbg
	@rm -f aes

