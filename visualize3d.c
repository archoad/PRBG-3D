/*visualize3d
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
#include <png.h>
#include <fftw3.h>

#include <GL/freeglut.h>

#define WINDOW_TITLE_PREFIX "Visualize PRBG"
#define couleur(param) printf("\033[%sm",param)
#define MAXSAMPLE 5000000

static short winSizeW = 920,
	winSizeH = 690,
	frame = 0,
	currentTime = 0,
	timebase = 0,
	fullScreen = 0,
	displayHilbert = 0,
	displayFFT = 0,
	rotate = 0,
	dt = 5; // in milliseconds

static int textList = 0,
	fftList =0,
	cpt = 0,
	background = 0,
	mono = 0;

static float fps = 0.0,
	rotx = -80.0,
	roty = 0.0,
	rotz = 20.0,
	xx = 0.0,
	yy = 5.0,
	zoom = 100.0,
	prevx = 0.0,
	prevy = 0.0,
	alpha = 0.0,
	pSize = 0.0,
	hilbertWidth = 1.5, hilbertHeight = 1.5;

static double xMax = 0, yMax = 0, zMax = 0,
	minAll = 0, maxAll = 0,
	fftMin = 0, fftMax = 0,
	fftWidthx = 0.0, fftWidthy = 0.0,
	fftDepth = 0.0,
	sum = 0, sumv = 0,
	average = 0, variance = 0, deviation = 0;

static double randList[MAXSAMPLE];
static fftw_complex fftOut[MAXSAMPLE];

typedef struct _point {
	GLdouble x, y, z;
	GLfloat r, g, b, a;
} point;

static point pointsList[MAXSAMPLE];
static point hilbertPointList[MAXSAMPLE];
static point fftPointList[MAXSAMPLE];

static unsigned long sampleSize = 0,
	fftN = 0,
	hilbertSize = 0;




void usage(void) {
	couleur("31");
	printf("Michel Dubois -- visualize3d -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: visualize3d <filename> <background color> <color type>\n");
	printf("\t<filename> -> file where the results of the algorithm will be stored\n");
	printf("\t<background color> -> 'white' or 'black'\n");
	printf("\t<color type> -> 'mono' or 'multi'\n");
}


double distance(point p1, point p2) {
	double dx=0.0, dy=0.0, dz=0.0, dist=0.0;
	dx = p2.x - p1.x;
	dx = dx * dx;
	dy = p2.y - p1.y;
	dy = dy * dy;
	dz = p2.z - p1.z;
	dz = dz * dz;
	dist = sqrt(dx + dy + dz);
	return(dist);
}


void takeScreenshot(char *filename) {
	FILE *fp = fopen(filename, "wb");
	int width = glutGet(GLUT_WINDOW_WIDTH);
	int height = glutGet(GLUT_WINDOW_HEIGHT);
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(png);
	unsigned char *buffer = calloc((width * height * 3), sizeof(unsigned char));
	int i;

	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)buffer);
	png_init_io(png, fp);
	png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png, info);
	for (i=0; i<height; i++) {
		png_write_row(png, &(buffer[3*width*((height-1) - i)]));
	}
	png_write_end(png, NULL);
	png_destroy_write_struct(&png, &info);
	free(buffer);
	fclose(fp);
	printf("INFO: Save screenshot on %s (%d x %d)\n", filename, width, height);
}


void hsv2rgb(double h, double s, double v, GLfloat *r, GLfloat *g, GLfloat *b) {
	double hp = h * 6;
	if ( hp == 6 ) hp = 0;
	int i = floor(hp);
	double v1 = v * (1 - s),
		v2 = v * (1 - s * (hp - i)),
		v3 = v * (1 - s * (1 - (hp - i)));
	if (i == 0) { *r=v; *g=v3; *b=v1; }
	else if (i == 1) { *r=v2; *g=v; *b=v1; }
	else if (i == 2) { *r=v1; *g=v; *b=v3; }
	else if (i == 3) { *r=v1; *g=v2; *b=v; }
	else if (i == 4) { *r=v3; *g=v1; *b=v; }
	else { *r=v; *g=v1; *b=v2; }
}


double calculateMaxTab(void) {
	double result = 0;
	unsigned long i;
	for (i=0; i<sampleSize; i++) {
		if (randList[i] > result) { result = randList[i]; }
	}
	return(result);
}


double calculateMinTab(void) {
	double result = 0;
	unsigned long i;
	result = randList[0];
	for (i=1; i<sampleSize; i++) {
		if (randList[i] < result) { result = randList[i]; }
	}
	return(result);
}


void drawString(float x, float y, float z, char *text) {
	unsigned i = 0;
	glPushMatrix();
	glLineWidth(1.0f);
	if (background){ // White background
		glColor3f(0.0, 0.0, 0.0);
	} else { // Black background
		glColor3f(1.0, 1.0, 1.0);
	}
	glTranslatef(x, y, z);
	glScalef(0.0008, 0.0008, 0.0008);
	for(i=0; i < strlen(text); i++) {
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, (int)text[i]);
	}
	glPopMatrix();
}


void drawText(void) {
	char text[4][50];
	sprintf(text[0], "Standard deviation:%1.2e Nbr elts:%ld", deviation, sampleSize);
	sprintf(text[1], "Average:%1.2e Variance:%1.2e", average, variance);
	sprintf(text[2], "Min:%1.2e Max:%1.2e", minAll, maxAll);
	sprintf(text[3], "dt:%1.3f FPS:%4.2f", (dt/1000.0), fps);
	textList = glGenLists(1);
	glNewList(textList, GL_COMPILE);
	drawString(-4.0, 3.55, -10.0, text[0]);
	drawString(-4.0, 3.70, -10.0, text[1]);
	drawString(-4.0, 3.85, -10.0, text[2]);
	drawString(-4.0, 4.0, -10.0, text[3]);
	glEndList();
}


void drawAxes(void) {
	float rayon = 0.1;
	float length = 100/4.0;

	// cube
	glPushMatrix();
	glLineWidth(1.0);
	glColor3f(0.8, 0.8, 0.8);
	glTranslatef(0.0, 0.0, 0.0);
	glutWireCube(100.0/2.0);
	glPopMatrix();

	// origin
	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	glutSolidSphere(rayon*4, 16, 16);
	glPopMatrix();

	// x axis
	glPushMatrix();
	glColor3f(1.0, 0.0, 0.0);
	glTranslatef(length/2.0, 0.0, 0.0);
	glScalef(length*5.0, 1.0, 1.0);
	glutSolidCube(rayon*2.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(length, 0.0, 0.0);
	glRotated(90, 0, 1, 0);
	glutSolidCone(rayon*2, rayon*4, 8, 8);
	glPopMatrix();
	drawString(length+2.0, 0.0, 0.0, "X");

	// y axis
	glPushMatrix();
	glColor3f(0.0, 1.0, 0.0);
	glTranslatef(0.0, length/2.0, 0.0);
	glScalef(1.0, length*5.0, 1.0);
	glutSolidCube(rayon*2.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0, length, 0.0);
	glRotated(90, -1, 0, 0);
	glutSolidCone(rayon*2, rayon*4, 8, 8);
	glPopMatrix();
	drawString(0.0, length+2.0, 0.0, "Y");

	// z axis
	glPushMatrix();
	glColor3f(0.0, 0.0, 1.0);
	glTranslatef(0.0, 0.0, length/2.0);
	glScalef(1.0, 1.0, length*5.0);
	glutSolidCube(rayon*2.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0, 0.0, length);
	glRotated(90, 0, 0, 1);
	glutSolidCone(rayon*2, rayon*4, 8, 8);
	glPopMatrix();
	drawString(0.0, 0.0, length+2.0, "Z");
}


void drawFFTAxes(void) {
	int i=0, midx=fftWidthx/2, midy=fftWidthy;
	double val=0;
	char text[24] = {'0'};

	fftList = glGenLists(1);
	glNewList(fftList, GL_COMPILE_AND_EXECUTE);
	glPushMatrix();
	glColor4f(0.8, 0.8, 0.8, 1.0);
	glBegin(GL_LINES); // vertical axe
		glVertex3f(-midx-0.10, -midy-0.10, -fftDepth);
		glVertex3f(-midx-0.10,  0.10, -fftDepth);
	glEnd();
	glBegin(GL_LINES); // horizontal axe
		glVertex3f(-midx-0.10, -midy-0.10, -fftDepth);
		glVertex3f( midx+0.10, -midy-0.10, -fftDepth);
	glEnd();
	for (i=0; i<=fftWidthx; i++) {
		val = i * fftN / fftWidthx;
		sprintf(text, "%0.0f", val);
		drawString(i-midx-0.05, -midy-0.30, -fftDepth, text);
		glBegin(GL_LINES);
			glVertex3f(i-midx, -midy-0.15, -fftDepth);
			glVertex3f(i-midx, -midy-0.10, -fftDepth);
		glEnd();
	}
	for (i=0; i<=fftWidthy; i++) {
		val = i * fftMax / fftWidthy;
		sprintf(text, "%1.1e", val);
		drawString(-midx-0.80, i-midy-0.05, -fftDepth, text);
		glBegin(GL_LINES);
			glVertex3f(-midx-0.15, i-midy, -fftDepth);
			glVertex3f(-midx-0.10, i-midy, -fftDepth);
		glEnd();
	}
	glPopMatrix();
	glEndList();
}


void drawHilbert(void) {
	glPushMatrix();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_DOUBLE, sizeof(point), hilbertPointList);
	glColorPointer(3, GL_FLOAT, sizeof(point), &hilbertPointList[0].r);
	glDrawArrays(GL_LINE_STRIP, 0, hilbertSize);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glPopMatrix();
}


void onReshape(int width, int height) {
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, width/height, 1.0, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


void onSpecial(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_UP:
			rotx += 5.0;
			printf("INFO: x = %f\n", rotx);
			break;
		case GLUT_KEY_DOWN:
			rotx -= 5.0;
			printf("INFO: x = %f\n", rotx);
			break;
		case GLUT_KEY_LEFT:
			rotz += 5.0;
			printf("INFO: z = %f\n", rotz);
			break;
		case GLUT_KEY_RIGHT:
			rotz -= 5.0;
			printf("INFO: z = %f\n", rotz);
			break;
		default:
			printf("x %d, y %d\n", x, y);
			break;
	}
	glutPostRedisplay();
}


void onMotion(int x, int y) {
	if (prevx) {
		xx += ((x - prevx)/10.0);
		printf("INFO: x = %f\n", xx);
	}
	if (prevy) {
		yy -= ((y - prevy)/10.0);
		printf("INFO: y = %f\n", yy);
	}
	prevx = x;
	prevy = y;
	glutPostRedisplay();
}


void onIdle(void) {
	frame += 1;
	currentTime = glutGet(GLUT_ELAPSED_TIME);
	if (currentTime - timebase >= 1000.0){
		fps = frame*1000.0 / (currentTime-timebase);
		timebase = currentTime;
		frame = 0;
	}
	glutPostRedisplay();
}


void onMouse(int button, int state, int x, int y) {
	switch (button) {
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN) {
				printf("INFO: left button, x %d, y %d\n", x, y);
			}
			break;
		case GLUT_RIGHT_BUTTON:
			if (state == GLUT_DOWN) {
				printf("INFO: right button, x %d, y %d\n", x, y);
			}
			break;
	}
}


void onKeyboard(unsigned char key, int x, int y) {
	unsigned long i = 0;
	char *name = malloc(20 * sizeof(char));
	switch (key) {
		case 27: // Escape
			printf("x %d, y %d\n", x, y);
			printf("INFO: exit loop\n");
			glutLeaveMainLoop();
			break;
		case 'x':
			xx += 1.0;
			printf("INFO: x = %f\n", xx);
			break;
		case 'X':
			xx -= 1.0;
			printf("INFO: x = %f\n", xx);
			break;
		case 'y':
			yy += 1.0;
			printf("INFO: y = %f\n", yy);
			break;
		case 'Y':
			yy -= 1.0;
			printf("INFO: y = %f\n", yy);
			break;
		case 'f':
			fullScreen = !fullScreen;
			printf("INFO: fullscreen %d\n", fullScreen);
			if (fullScreen) {
				glutFullScreen();
			} else {
				glutPositionWindow(120,10);
				glutReshapeWindow(winSizeW, winSizeH);
			}
			break;
		case 'h':
			displayHilbert = !displayHilbert;
			printf("INFO: display Hilbert graph %d\n", displayHilbert);
			break;
		case 't':
			displayFFT = !displayFFT;
			printf("INFO: display FFT graph %d\n", displayFFT);
			break;
		case 'a':
			alpha -= 0.05;
			if (alpha <= 0) { alpha = 1.0; }
			for (i=0; i<sampleSize; i++) {
				pointsList[i].a = alpha;
			}
			printf("INFO: alpha channel %f\n", alpha);
			break;
		case 's':
			pSize += 1.0;
			if (pSize >= 20) { pSize = 0.5; }
			printf("INFO: point size %f\n", pSize);
			break;
		case 'r':
			rotate = !rotate;
			printf("INFO: rotate %d\n", rotate);
			break;
		case 'z':
			zoom -= 5.0;
			if (zoom < 5.0) {
				zoom = 5.0;
			}
			printf("INFO: zoom = %f\n", zoom);
			break;
		case 'Z':
			zoom += 5.0;
			printf("INFO: zoom = %f\n", zoom);
			break;
		case 'p':
			printf("INFO: take a screenshot\n");
			sprintf(name, "capture_%.3d.png", cpt);
			takeScreenshot(name);
			cpt += 1;
			break;
		default:
			break;
	}
	free(name);
	glutPostRedisplay();
}


void onTimer(int event) {
	switch (event) {
		case 0:
			break;
		default:
			break;
	}
	if (rotate) {
		rotz -= 0.2;
	} else {
		rotz += 0.0;
	}
	if (rotz > 360) rotz = 360;
	glutPostRedisplay();
	glutTimerFunc(dt, onTimer, 1);
}


void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	drawText();
	glCallList(textList);

	if (displayFFT) {
		glPushMatrix();
		glCallList(fftList);
		glPopMatrix();
		glPointSize(pSize);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3, GL_DOUBLE, sizeof(point), fftPointList);
		glColorPointer(4, GL_FLOAT, sizeof(point), &fftPointList[0].r);
		glDrawArrays(GL_LINES, 0, fftN);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	if (displayHilbert) {
		drawHilbert();
	}

	glPushMatrix();
	glTranslatef(xx, yy, -zoom);
	glRotatef(rotx, 1.0, 0.0, 0.0);
	glRotatef(roty, 0.0, 1.0, 0.0);
	glRotatef(rotz, 0.0, 0.0, 1.0);

	GLfloat ambient1[] = {0.15f, 0.15f, 0.15f, 1.0f};
	GLfloat diffuse1[] = {0.8f, 0.8f, 0.8f, 1.0f};
	GLfloat specular1[] = {1.0f, 1.0f, 1.0f, 1.0f};
	GLfloat position1[] = {0.0f, 0.0f, 24.0f, 1.0f};
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, specular1);
	glLightfv(GL_LIGHT1, GL_POSITION, position1);
	glEnable(GL_LIGHT1);

	drawAxes();

	glPointSize(pSize);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_DOUBLE, sizeof(point), pointsList);
	glColorPointer(4, GL_FLOAT, sizeof(point), &pointsList[0].r);
	glDrawArrays(GL_POINTS, 0, sampleSize);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glPopMatrix();

	glutPostRedisplay();
	glutSwapBuffers();
}


void init(void) {
	if (background){ // White background
		glClearColor(1.0, 1.0, 1.0, 1.0);
	} else { // Black background
		glClearColor(0.1, 0.1, 0.1, 1.0);
	}

	glEnable(GL_LIGHTING);

	GLfloat ambient[] = {0.05f, 0.05f, 0.05f, 1.0f};
	GLfloat diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
	GLfloat specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
	GLfloat position[] = {0.0f, 0.0f, 0.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glEnable(GL_LIGHT0);

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	GLfloat matAmbient[] = {0.3f, 0.3f, 0.3f, 1.0f};
	GLfloat matDiffuse[] = {0.6f, 0.6f, 0.6f, 1.0f};
	GLfloat matSpecular[] = {0.8f, 0.8f, 0.8f, 1.0f};
	GLfloat matShininess[] = {128.0f};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);

	GLfloat baseAmbient[] = {0.5f, 0.5f, 0.5f, 0.5f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, baseAmbient);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

	// points smoothing
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

	//needed for transparency
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glShadeModel(GL_SMOOTH); // smooth shading
	glEnable(GL_NORMALIZE); // recalc normals for non-uniform scaling
	glEnable(GL_AUTO_NORMAL);

	glEnable(GL_CULL_FACE); // do not render back-faces, faster

	drawFFTAxes();
}


void glmain(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(winSizeW, winSizeH);
	glutInitWindowPosition(120, 10);
	glutCreateWindow(WINDOW_TITLE_PREFIX);
	glutDisplayFunc(display);
	glutReshapeFunc(onReshape);
	glutSpecialFunc(onSpecial);
	glutMotionFunc(onMotion);
	glutIdleFunc(onIdle);
	glutMouseFunc(onMouse);
	glutKeyboardFunc(onKeyboard);
	glutTimerFunc(dt, onTimer, 0);
	init();
	fprintf(stdout, "INFO: OpenGL Version: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "INFO: FreeGLUT Version: %d\n", glutGet(GLUT_VERSION));
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
	fprintf(stdout, "INFO: Freeing memory\n");
	glDeleteLists(textList, 1);
	glDeleteLists(fftList, 1);
}


void computeHilbert(double x, double y, double z, float xi, float xj, float yi, float yj, int n) {
	// src: http://www.fundza.com/algorithmic/space_filling/hilbert/basics/index.html
	// x and y are the coordinates of the bottom left corner
	// (xi, yi) & (xj, yj) are the i & j components of the unit x & y vectors of the frame

	static unsigned long i = 0;
	if (n <= 0) {
		hilbertPointList[i].x = x + (xi + yi)/2;
		hilbertPointList[i].y = y + (xj + yj)/2;
		hilbertPointList[i].z = z;
		i += 1;
	} else {
		computeHilbert(x,				y,				z, yi/2,	yj/2,	xi/2,	xj/2,	n-1);
		computeHilbert(x+xi/2,			y+xj/2,			z, xi/2,	xj/2,	yi/2,	yj/2,	n-1);
		computeHilbert(x+xi/2+yi/2,	y+xj/2+yj/2,	z, xi/2,	xj/2,	yi/2,	yj/2,	n-1);
		computeHilbert(x+xi/2+yi,		y+xj/2+yj,		z, -yi/2,	-yj/2,	-xi/2,	-xj/2,	n-1);
	}
}


int determineHilbertOrder(void) {
	int order = 0, n = 0;
	do {
		if ((sampleSize <= pow(4,n+1)) & (sampleSize > pow(4,n))) {
			order = n+1;
			break;
		} else {
			n += 1;
		}
	} while(n <= 8);
	return(order);
}


void determineHilbertMaximum(void) {
	unsigned long i;
	xMax=0; yMax=0;
	for (i=0; i<hilbertSize; i++) {
		if (xMax < fabs(hilbertPointList[i].x)) xMax = fabs(hilbertPointList[i].x);
		if (yMax < fabs(hilbertPointList[i].y)) yMax = fabs(hilbertPointList[i].y);
	}
	xMax = round(xMax);
	yMax = round(yMax);
}


void scaleHilbert(void) {
	unsigned long i;
	for (i=0; i<hilbertSize; i++) {
		hilbertPointList[i].x = hilbertPointList[i].x * hilbertWidth / xMax;
		hilbertPointList[i].y = hilbertPointList[i].y * hilbertHeight / yMax;
		hilbertPointList[i].x += 2.5;
		hilbertPointList[i].y += 2.5;
	}
}


void colorizeHilbert(void) {
	unsigned long i;
	double hue=0.0, current=0;
	for (i=0; i<hilbertSize; i++) {
		hilbertPointList[i].a=1.0f;
		if (i<sampleSize) {
			current = randList[i];
			hue = current / maxAll;
			hsv2rgb(hue, 0.8, 1.0, &(hilbertPointList[i].r), &(hilbertPointList[i].g), &(hilbertPointList[i].b));
		} else {
			hsv2rgb(0.0, 0.1, 0.1, &(hilbertPointList[i].r), &(hilbertPointList[i].g), &(hilbertPointList[i].b));
		}
	}
}


void computeFFT(void) {
	/* info: http://paulbourke.net/miscellaneous/dft/
	source http://people.sc.fsu.edu/~jburkardt/c_src/fftw3/fftw3_prb.c
	http://people.sc.fsu.edu/~jburkardt/c_src/fftw3/fftw3_prb_output.txt
	based on test02
	The same with GNU octave:
		x = textread("result.dat", "%s");
		x = hex2dec(x);
		x = x / (2^48);
		t = abs(fft(x));
	*/

	unsigned long i=0, cpt=0;

	//Suppress pic on nul frequency
	for (i=0; i<sampleSize; i++) {
		randList[i] -= average;
	}

	fftw_plan p;
	p = fftw_plan_dft_r2c_1d(sampleSize, randList, fftOut, FFTW_ESTIMATE);
	fftw_execute(p);
	fftw_destroy_plan(p);

	for (i=0; i<fftN; i++) {
		// magnitude spectrum
		fftPointList[cpt].y = sqrt(fftOut[i][0]*fftOut[i][0] + fftOut[i][1]*fftOut[i][1]);
		// phase spectrum
		//fftPointList[i].y = atan2(fftOut[i][1], fftOut[i][0]) * 180 / M_PI;

		if (i==0) { fftMin=fftPointList[i].y; fftMax=0; }
		fftMax = fmax(fftMax, fftPointList[i].y);
		fftMin = fmin(fftMin, fftPointList[i].y);
		cpt+=2;
	}
}


void scaleFFT(void) {
	unsigned long i=0;

	for (i=0; i<fftN*2; i+=2) {
		fftPointList[i].x = (double)(fftWidthx * i) / (double)fftN;
		fftPointList[i].x -= fftWidthx/2;
		fftPointList[i+1].x = fftPointList[i].x;

		fftPointList[i].y = fftPointList[i].y * fftWidthy / (fftMax - fftMin);
		fftPointList[i].y += -fftWidthy;
		fftPointList[i+1].y = -fftWidthy;

		fftPointList[i].z = -fftDepth;
		fftPointList[i+1].z = -fftDepth;
	}
}


void colorizeFFT(void) {
	unsigned long i=0;
	double hue=0.0, max=0.0;

	for (i=0; i<fftN; i++) {
		max = fmax(max, fftPointList[i].y);
	}
	for (i=0; i<fftN*2; i+=2) {
		//hue = (double)i / (double)fftN;
		hue = fftPointList[i].y / max;
		hsv2rgb(hue, 0.8, 1.0, &(fftPointList[i].r), &(fftPointList[i].g), &(fftPointList[i].b));
		fftPointList[i].a = 1.0;
		hsv2rgb(0.01, 0.8, 1.0, &(fftPointList[i+1].r), &(fftPointList[i+1].g), &(fftPointList[i+1].b));
		fftPointList[i+1].a = 1.0;
	}
}


void populatePoints(void) {
	unsigned long i;
	double x = 0, y = 0, z = 0, hue = 0;
	printf("INFO: Compute point list\n");
	srand(time(NULL));

	xMax=0; yMax=0; zMax=0;
	if (mono) { hue = (double)rand() / (double)(RAND_MAX - 1); }

	for (i=0; i<sampleSize; i++) {
		if (!mono) { hue = (double)i / (double)sampleSize; }
		hsv2rgb(hue, 1.0, 1.0, &(pointsList[i].r), &(pointsList[i].g), &(pointsList[i].b));
		pointsList[i].a = alpha;
		sum += randList[i];
		if (i>=3) {
			x = randList[i-2] - randList[i-3];
			y = randList[i-1] - randList[i-2];
			z = randList[i] - randList[i-1];
			if (xMax < fabs(x)) xMax = fabs(x);
			if (yMax < fabs(y)) yMax = fabs(y);
			if (zMax < fabs(z)) zMax = fabs(z);
			pointsList[i].x = x;
			pointsList[i].y = y;
			pointsList[i].z = z;
		} else {
			pointsList[i].x = 0;
			pointsList[i].y = 0;
			pointsList[i].z = 0;
		}
	}
	average = sum / (double)sampleSize;
	for (i=0; i<sampleSize; i++) {
		sumv += pow((randList[i] - average), 2);
		pointsList[i].x = pointsList[i].x * 25.0 / xMax;
		pointsList[i].y = pointsList[i].y * 25.0 / yMax;
		pointsList[i].z = pointsList[i].z * 25.0 / zMax;
	}
	variance = sumv / (double)sampleSize;
	deviation = sqrt(variance);
}


void populateHilbert(void) {
	double x0 = 0.0, y0 = 0.0, z0 = -10.0;
	int order = 0;
	order = determineHilbertOrder();
	if (order) {
		hilbertSize = pow(4,order);
		printf("INFO: Compute Hilbert graph\n");
		printf("INFO: hilbert order %d (%lu points)\n", order, hilbertSize);
		computeHilbert(x0, y0, z0, order*2.0, 0.0, 0.0, order*2.0, order);
		determineHilbertMaximum();
		scaleHilbert();
		colorizeHilbert();
		determineHilbertMaximum();
	}
}


void populateFFT(void) {
	printf("INFO: Compute FFT\n");
	fftN = (sampleSize/2);
	fftWidthx = 8.0;
	fftWidthy = 4.0;
	fftDepth = 12.0;
	computeFFT();
	colorizeFFT();
	scaleFFT();
}


long countFileLines(char *name) {
	long count = 0;
	char ch='\0';
	FILE *fic = fopen(name, "r");
	if (fic != NULL) {
		while (ch != EOF) {
			ch = fgetc(fic);
			if (ch == '\n')  count++;
		}
		fclose(fic);
	} else {
		printf("### ERROR open file error\n");
		exit(EXIT_FAILURE);
	}
	return count;
}


void playFile(int argc, char *argv[]) {
	unsigned long i=0;
	double alea;

	FILE *fic = fopen(argv[1], "r");
	if (fic != NULL) {
		printf("INFO: file open\n");
		while (!feof(fic)) {
			fscanf(fic, "%lf\n", &alea);
			randList[i] = alea;
			i++;
		}
		fclose(fic);
		printf("INFO: file close\n");
		maxAll = calculateMaxTab();
		minAll = calculateMinTab();
		populatePoints();
		populateHilbert();
		populateFFT();
		glmain(argc, argv);
	} else {
		printf("### ERROR open file error\n");
		exit(EXIT_FAILURE);
	}
}


int main(int argc, char *argv[]) {
	switch (argc) {
		case 4:
			if (!strncmp(argv[2], "white", 5)) { background = 1; }
			if (!strncmp(argv[3], "mono", 4)) { mono = 1; }
			alpha = 1.0f;
			pSize = 1.0f;
			sampleSize = countFileLines(argv[1]);
			playFile(argc, argv);
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;
	}
	exit(EXIT_SUCCESS);
}
