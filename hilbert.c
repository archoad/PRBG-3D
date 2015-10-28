/*hilbert
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
#include <math.h>
#include <png.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#define WINDOW_TITLE_PREFIX "Hilbert Curve"
#define couleur(param) printf("\033[%sm",param)

static short winSizeW = 920,
	winSizeH = 690,
	frame = 0,
	currentTime = 0,
	timebase = 0,
	fullScreen = 0,
	rotate = 0,
	number = 0,
	dt = 5; // in milliseconds

static int textList = 0,
	objectList = 0,
	hilbertList = 0,
	cpt = 0,
	background = 0;

static float fps = 0.0,
	rotx = 0.0,
	roty = 0.0,
	rotz = 0.0,
	xx = 0.0,
	yy = 0.0,
	zoom = 45.0,
	prevx = 0.0,
	prevy = 0.0,
	sphereRadius = 0.2,
	squareWidth = 0.055;

typedef struct _point {
	GLfloat x, y, z;
	GLfloat r, g, b;
} point;

static point *hilbertPointList = NULL;

static double xMax = 0,
	xMin = 0;

static unsigned long order = 0,
	hilbertSize = 0;




void usage(void) {
	couleur("31");
	printf("Michel Dubois -- hilbert -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: hilbert <number> <background color>\n");
	printf("\t<number> -> number of repetition\n");
	printf("\t<background color> -> 'white' or 'black'\n");
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


void drawPoint(point p) {
	glPointSize(1.0);
	glColor3f(p.r, p.g, p.b);
	glBegin(GL_POINTS);
	glNormal3f(p.x, p.y, p.z);
	glVertex3f(p.x, p.y, p.z);
	glEnd();
}


void drawSphere(point p) {
	glColor3f(p.r, p.g, p.b);
	glTranslatef(p.x, p.y, p.z);
	glutSolidSphere(sphereRadius, 16, 16);
}


void drawSquare(point p) {
	glColor3f(p.r, p.g, p.b);
	glTranslatef(p.x, p.y, p.z);
	glBegin(GL_QUADS);
	glVertex3f(-squareWidth, -squareWidth, 0.0); // Bottom left corner
	glVertex3f(-squareWidth, squareWidth, 0.0); // Top left corner
	glVertex3f(squareWidth, squareWidth, 0.0); // Top right corner
	glVertex3f(squareWidth, -squareWidth, 0.0); // Bottom right corner
	glEnd();
}


void drawLine(point p1, point p2){
	glLineWidth(2.0);
	glBegin(GL_LINES);
	glColor3f(p1.r, p1.g+0.5, p1.b+0.5);
	glNormal3f(p1.x, p1.y, p1.z);
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p2.x, p2.y, p2.z);
	glEnd();
}


void drawString(float x, float y, float z, char *text, float size) {
	unsigned i = 0;
	glPushMatrix();
	glLineWidth(1.0);
	if (background){ // White background
		glColor3f(0.0, 0.0, 0.0);
	} else { // Black background
		glColor3f(1.0, 1.0, 1.0);
	}
	glTranslatef(x+0.15, y+0.15, z);
	glScalef(size, size, size);
	for(i=0; i < strlen(text); i++) {
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, (int)text[i]);
	}
	glPopMatrix();
}


void drawText(void) {
	char text[50];
	sprintf(text, "Michel Dubois, dt: %1.3f, FPS: %4.2f", (dt/1000.0), fps);
	textList = glGenLists(1);
	glNewList(textList, GL_COMPILE);
	drawString(-40.0, -40.0, -100.0, text, 0.01);
	glEndList();
}


void drawAxes(void) {
	/*
	glPushMatrix();
	glColor3f(0.8, 0.8, 0.8);
	glTranslatef(0.0, 0.0, 0.0);
	glutSolidSphere(sphereRadius, 8, 8);
	glPopMatrix();
	*/

	glPushMatrix();
	glLineWidth(1.0);
	glColor3f(0.8, 0.8, 0.8);
	glTranslatef(0.0, 0.0, 0.0);
	glutWireCube(100.0/2.0);
	glPopMatrix();
}


void drawHilbert(void) {
	unsigned long i;
	char str[7];
	hilbertList = glGenLists(1);
	glNewList(hilbertList, GL_COMPILE_AND_EXECUTE);
	for (i=0; i<hilbertSize; i++) {
		sprintf(str, "%lu", i+1);
		glPushMatrix();
		if (number) {
			drawString(hilbertPointList[i].x, hilbertPointList[i].y, hilbertPointList[i].z, str, 0.004);
		}
		if (i<hilbertSize-1) {
			drawLine(hilbertPointList[i], hilbertPointList[i+1]);
		}
		drawSphere(hilbertPointList[i]);
		glPopMatrix();
	}
	glEndList();
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
	char *name = malloc(20 * sizeof(char));
	switch (key) {
		case 27: // Escape
			printf("INFO: exit\n");
			printf("x %d, y %d\n", x, y);
			exit(0);
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
			if (fullScreen) {
				glutFullScreen();
			} else {
				glutReshapeWindow(winSizeW, winSizeH);
				glutPositionWindow(100,100);
				printf("INFO: fullscreen %d\n", fullScreen);
			}
			break;
		case 'r':
			rotate = !rotate;
			printf("INFO: rotate %d\n", rotate);
			break;
		case 'n':
			number = !number;
			printf("INFO: display number %d\n", number);
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
		roty -= 0.2;
	} else {
		roty += 0.0;
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

	glPushMatrix();
	glTranslatef(xx, yy, -zoom);
	glRotatef(rotx, 1.0, 0.0, 0.0);
	glRotatef(roty, 0.0, 1.0, 0.0);
	glRotatef(rotz, 0.0, 0.0, 1.0);
	//drawAxes();
	drawHilbert();
	glCallList(hilbertList);
	glPopMatrix();

	glutSwapBuffers();
	glutPostRedisplay();
}


void init(void) {
	if (background){ // White background
		glClearColor(1.0, 1.0, 1.0, 1.0);
	} else { // Black background
		glClearColor(0.1, 0.1, 0.1, 1.0);
	}

	GLfloat position[] = {0.0, 0.0, 0.0, 1.0};
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	GLfloat modelAmbient[] = {0.5, 0.5, 0.5, 1.0};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, modelAmbient);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);

	GLfloat no_mat[] = {0.0, 0.0, 0.0, 1.0};
	GLfloat mat_diffuse[] = {0.1, 0.5, 0.8, 1.0};
	GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
	GLfloat shininess[] = {128.0};
	glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

	glEnable(GL_NORMALIZE);
	glEnable(GL_AUTO_NORMAL);
	glDepthFunc(GL_LESS);
}


void glmain(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitWindowSize(winSizeW, winSizeH);
	glutInitWindowPosition(120, 10);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow(WINDOW_TITLE_PREFIX);
	init();
	glutDisplayFunc(display);
	glutReshapeFunc(onReshape);
	glutSpecialFunc(onSpecial);
	glutMotionFunc(onMotion);
	glutIdleFunc(onIdle);
	glutMouseFunc(onMouse);
	glutKeyboardFunc(onKeyboard);
	glutTimerFunc(dt, onTimer, 0);
	fprintf(stdout, "INFO: OpenGL Version: %s\n", glGetString(GL_VERSION));
	glutMainLoop();
	glDeleteLists(textList, 1);
	glDeleteLists(objectList, 1);
}


void hilbert(double x, double y, double z, float xi, float xj, float yi, float yj, int n) {
	// src: http://www.fundza.com/algorithmic/space_filling/hilbert/basics/index.html
	// x and y are the coordinates of the bottom left corner
	// (xi, yi) & (xj, yj) are the i & j components of the unit x & y vectors of the frame

	static unsigned long i = 0;
	if (n <= 0) {
		hilbertPointList[i].x = x + (xi + yi)/2;
		hilbertPointList[i].y = y + (xj + yj)/2;
		hilbertPointList[i].z = z;
		hilbertPointList[i].r = 1.0;
		hilbertPointList[i].g = 0.196;
		hilbertPointList[i].b = 0.196;
		if (xMax < fabs(hilbertPointList[i].x)) xMax = fabs(hilbertPointList[i].x);
		i += 1;
	} else {
		hilbert(x,				y,				z, yi/2,	yj/2,	xi/2,	xj/2,	n-1);
		hilbert(x+xi/2,			y+xj/2,			z, xi/2,	xj/2,	yi/2,	yj/2,	n-1);
		hilbert(x+xi/2+yi/2,	y+xj/2+yj/2,	z, xi/2,	xj/2,	yi/2,	yj/2,	n-1);
		hilbert(x+xi/2+yi,		y+xj/2+yj,		z, -yi/2,	-yj/2,	-xi/2,	-xj/2,	n-1);
	}
}


void populateHilbert() {
	double x0 = 0.0, y0 = 0.0, z0 = 0.0;
	unsigned long i;
	hilbertPointList = (point*)calloc(hilbertSize, sizeof(point));
	if (hilbertPointList == NULL) {
		printf("### ERROR hilbertPointList\n");
		exit(EXIT_FAILURE);
	}
	hilbert(x0, y0, z0, order*8.0, 0.0, 0.0, order*8.0, order);
	xMin = hilbertPointList[0].x;
	printf("INFO: hilbert order %lu (%lu points)\n", order, hilbertSize);
	printf("INFO: xMax = %f, xMin = %f\n", xMax, xMin);

	for (i=0; i<hilbertSize; i++) {
		hilbertPointList[i].x = hilbertPointList[i].x - ((xMax + xMin) / 2.0);
		hilbertPointList[i].y = hilbertPointList[i].y - ((xMax + xMin) / 2.0);
		//printf("%lu -> (%f,%f,%f)\n", i, hilbertPointList[i].x, hilbertPointList[i].y, hilbertPointList[i].z);
	}
}


void initiateList(int argc, char *argv[]) {
	hilbertSize = pow(4,order);
	populateHilbert();
	glmain(argc, argv);
}


int main(int argc, char *argv[]) {
	switch (argc) {
		case 3:
			if (!strncmp(argv[2], "white", 5)) {
				background = 1;
			}
			order = atoi(argv[1]);
			initiateList(argc, argv);
			exit(EXIT_SUCCESS);
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;	
	}
}



