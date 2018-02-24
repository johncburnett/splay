#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
    ofGLWindowSettings settings;
    settings.width = WIDTH;
    settings.height = HEIGHT;
    ofCreateWindow(settings);
    ofRunApp(new ofApp());
}
