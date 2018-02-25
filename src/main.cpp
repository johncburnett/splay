#include "ofMain.h"
#include "ofApp.h"

#define GLSL330 0

//========================================================================
int main( ){
    ofGLWindowSettings settings;
#if GLSL330
    settings.setGLVersion(3, 2);
#endif
    settings.width = WIDTH;
    settings.height = HEIGHT;
    ofCreateWindow(settings);
    ofRunApp(new ofApp());
}
