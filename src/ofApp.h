#pragma once

#include "ofMain.h"

#include "plyLoader.h"
#include "fusion.h"

#include "ofxGui.h"
#include "ofxOsc.h"
#include "ofxSyphon.h"
#include "ofxPostGlitch.h"

#define RES 1

#if   RES == 0          // HD
    #define WIDTH  800
    #define HEIGHT 600
#elif RES == 1          // retina
    #define WIDTH  2880
    #define HEIGHT 1800
#elif RES == 2          // 4K
    #define WIDTH  3840
    #define HEIGHT 2160
#elif RES == 3          // Projector
    #define WIDTH  1280
    #define HEIGHT 720
#elif RES == 4          // VROOM
    #define WIDTH  3200
    #define HEIGHT 900
#elif RES == 5          // GIF
    #define WIDTH  1000
    #define HEIGHT 1000
#endif

#define FRAMERATE 30
#define NUM_PARTICLES 7700000

#define OSC 0
#define PORT 12345

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
        void setupGUI(void);
        void readMessages(void);
		
    private:
        Fusion *fusion;
        ofFbo fbo;
        ofShader converge;
        std::vector<string> paths;
    
        ofEasyCam cam;
        int frame_num;
    
        ofxPanel gui;
        ofParameter<float> frame, opacity, displace;
        ofParameter<float> df, dDist;
        ofParameter<float> scanHeight, scanWidth, scanRate;
        ofParameter<float> dx, dy, dz;
        ofParameter<float> camX, camY, camZ;
        ofParameter<ofPoint> dofCam;
        ofParameter<ofPoint> dofCenter;
        ofParameter<float> bell;
        ofParameter<ofPoint> pos;
        ofParameter<float> offsetX, offsetY;
        ofxToggle run, record;
        ofxToggle runDistort;
        ofxToggle runScan;
        ofxButton reset;
        bool showGUI;
    
        ofxPostGlitch *glitch;

        // audio
        float rms, mod0, mod1;
    
        // save camera perspectives
        void camLoadPos(string);
        void camSavePos(string);
        ofMatrix4x4 defaultPosition;
    
        // Syphon
        ofxSyphonServer mainOutputSyphonServer;
        ofxSyphonClient mClient;
    
        // OSC
        ofxOscReceiver receive;
};
