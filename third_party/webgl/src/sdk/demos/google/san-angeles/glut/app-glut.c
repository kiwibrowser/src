
#include <assert.h>
#include <sys/time.h>
#include <GL/glut.h>
#include "app.h"
#include "importgl.h"

int gAppAlive = 1;

static int sWidth = 640;
static int sHeight = 480;

void display() {
  struct timeval time_now;

  gettimeofday(&time_now, NULL);
#if 1
  appRender(time_now.tv_sec * 1000 + time_now.tv_usec / 1000,
            sWidth, sHeight);
#else
  appRender(0, sWidth, sHeight);
#endif

  glutSwapBuffers();
  glutPostRedisplay();
}

void reshape(int width, int height) {
  sWidth = width;
  sHeight = height;
}

int main(int argc, char *argv[]) {
  int result;

  glutInit(&argc, argv);

  glutInitWindowSize(sWidth, sHeight);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("San Angeles Observation");

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);

  result = importGLInit();
  assert(result);

  appInit();
  glutMainLoop();

  return 0;
}

