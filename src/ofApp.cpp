/*
 * TODO:
 * allocate and clear mesh loading during runtime in thread
 * update to GLSL to 330 
 * integrate transform feedback
 */

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(FRAMERATE);
    ofBackground(0);
    
    frame = 0;
    df = 0;
    opacity = 0.2;
    run = false;
    showGUI = false;
    frame_num = 0;
    dx = dy = dz = 0;
    camX = camY = camZ = 0;
    rms = mod0 = mod1 = 0;
    
    glitch = new ofxPostGlitch();
    
    // OSC
    receive.setup(PORT);

    fusion = new Fusion(NUM_PARTICLES);
    
    // load models to fuse
    ofDirectory dir("./models");
    dir.listDir();
    for(unsigned i = 0; i < dir.size(); i++){
        string path = dir.getFile(i).getAbsolutePath();
        fusion->addMesh(path);
    }
    
    head = 0;
    tail = 1;
    
    // shaders
    converge.load("shaders/convergence");
    
    // GUI
    setupGUI();
   
    // syphon
    mainOutputSyphonServer.setName("Screen Output");
    mClient.setup();
    mClient.set("","Simple Server");

    ofFbo::Settings s;
    s.width             = ofGetWidth();
    s.height			= ofGetHeight();
    s.internalformat    = GL_RGBA32F_ARB;
    s.useDepth			= false;
    s.wrapModeVertical  = GL_MIRRORED_REPEAT_ARB;
    s.wrapModeHorizontal= GL_MIRRORED_REPEAT_ARB;
    fbo.allocate(s);
    glitch->setup(&fbo);
    
    if(OSC == 0) camLoadPos("camPos0");
    cam.setNearClip(0);
}

//--------------------------------------------------------------
void ofApp::update(){
    if(frame >= 1000) {
        swapModels();
    }

    fusion->setInterp(frame * 0.001);
    fusion->setOpacity(opacity);
    fusion->setDisplacement(displace);
    
    fusion->setPos0(dx, dy, dz);
    fusion->setPos1(-1*dx, -1*dy, -1*dz);
    
//    fusion->setScanX(scanWidth, scanRate, runScan);
    fusion->setScanY(scanHeight, scanRate, runScan);
    
    fusion->setDof(dofCam, dofCenter, bell);
    
    fusion->update();
    
#if OSC
    readMessages();
    cam.setPosition(camX, camY, camZ);
#else
    frame += df * run;
    displace += dDist * runDistort;
#endif
    
    ofSetWindowTitle(ofToString(ofGetFrameRate()));
}

//--------------------------------------------------------------
void ofApp::draw(void) {
    fbo.begin();
    ofEnableSmoothing();
    ofClear(0,0,0,255);
    
    ofEnableAlphaBlending();
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    
    // render
    cam.begin();
    fusion->draw();
    cam.end();
    
    ofDisableAlphaBlending();
    
    fbo.end();
    
    //glitch->generateFx();
    converge.begin();
    converge.setUniformTexture("tex0", fbo, 0);
    converge.setUniform3f("pos", pos);
    converge.setUniform1f("offsetX", offsetX);
    converge.setUniform1f("offsetY", offsetY);
    ofClear(0, 0, 0, 1);
    fbo.draw(0, 0);
    converge.end();
    
    if(record) {
        ofPixels pix;
        pix.allocate(ofGetWidth(), ofGetHeight(), 4);
        fbo.readToPixels(pix);
        
        ofImage img;
        img.setFromPixels(pix);
        
        char name[32];
        sprintf(name, "./runs/frame_%05d.tif", frame_num);
        img.save(name);
        frame_num++;
        if ((frame_num % 100) == 0) sleep(10);
    }
    
    // syphon
    mClient.draw(50, 50);
    mainOutputSyphonServer.publishScreen();
    
    if(showGUI) gui.draw();
}

//--------------------------------------------------------------
void ofApp::swapModels() {
    for(unsigned i = 0; i < fusion->getNumModels(); ++i) {
        if(i != head && i != tail) {
            head = tail;
            tail = i;
            fusion->setHead(head);
            fusion->setTail(tail);
            frame = 0;
            break;
        }
    }
}

//--------------------------------------------------------------
void ofApp::setupGUI(void) {
    gui.setup("fusion");
    gui.add(run.setup("Run", false));
    gui.add(frame.set("time", 0.0, 0.0, 2000));
    gui.add(df.set("rate", 0.0, -3.0, 3.0));
    gui.add(opacity.set("opacity", 0.1, 0.0, 1.0));
    
    gui.add(dx.set("dx", 0.0, -250.0, 250.0));
    gui.add(dy.set("dy", 0.0, -250.0, 250.0));
    gui.add(dz.set("dz", 0.0, -250.0, 250.0));
    
    // deform
    gui.add(runDistort.setup("deform", false));
    gui.add(displace.set("displace", 0.0, -1.0, 1.0));
    gui.add(dDist.set("rate", 0.0, -0.001, 0.001));
    
    gui.add(runScan.setup("scan", false));
    gui.add(scanWidth.set("width", 0.0, 0.0, WIDTH));
    gui.add(scanHeight.set("height", 0.0, 0.0, HEIGHT));
    gui.add(scanRate.set("rate", 0, 0.0, 10.0));
    
    gui.add(dofCam.set("camera_plane",
            ofPoint(0,0,0), ofPoint(-1,-1,-1), ofPoint(1,1,1)));
    gui.add(dofCenter.set("dof_center",
            ofPoint(0,0,0), ofPoint(-1,-1,-1), ofPoint(1,1,1)));
    gui.add(bell.set("bell", 1, 0, 10));
    
    gui.add(offsetX.set("offsetX", 0.0, -1.0, 1.0));
    gui.add(offsetY.set("offsetY", 0.0, -1.0, 1.0));
    gui.add(pos.set("pos",
            ofPoint(0,0,0), ofPoint(-3000,-3000,-3000), ofPoint(3000,3000,3000)));
    
    gui.add(record.setup("record", false));
}

//--------------------------------------------------------------
void ofApp::readMessages(void) {
    while (receive.hasWaitingMessages()){
        ofxOscMessage m;
        receive.getNextMessage(m);
        string address = m.getAddress();
        
        if (address == "/camX"){ camX = m.getArgAsFloat(0); }
        if (address == "/camY"){ camY = m.getArgAsFloat(0); }
        if (address == "/camZ"){ camZ = m.getArgAsFloat(0); }
        if (address == "/time"){ frame = m.getArgAsFloat(0); }
        
        if (address == "/distortOn"){ runDistort = true; }
        if (address == "/distortOff"){ runDistort = false; }
        if (address == "/distort"){ displace = m.getArgAsFloat(0); }
        if (address == "/distortRate"){ dDist = m.getArgAsFloat(0); }
        
        if (address == "/scanOn"){ runScan = true; }
        if (address == "/scanOff"){ runScan = false; }
        if (address == "/scanWidth"){ scanWidth = m.getArgAsFloat(0); }
        if (address == "/scanHeight"){ scanHeight = m.getArgAsFloat(0); }
        if (address == "/scanRate"){ scanRate = m.getArgAsFloat(0); }

        if (address == "/rms"){ rms = m.getArgAsFloat(0); displace = rms*10; }
        if (address == "/mod0"){ mod0 = m.getArgAsFloat(0); frame = mod0; }
        if (address == "/mod1"){ mod1 = m.getArgAsFloat(0); }

        if (address == "/converge"){
            float val = m.getArgAsFloat(0);
            if(val > 0.5) glitch->setFx(OFXPOSTGLITCH_CONVERGENCE, true);
            else glitch->setFx(OFXPOSTGLITCH_CONVERGENCE, false);
        }

        if (address == "/shaker"){
            float val = m.getArgAsFloat(0);
            if(val > 0.5) glitch->setFx(OFXPOSTGLITCH_SHAKER, true);
            else glitch->setFx(OFXPOSTGLITCH_SHAKER, false);
        }

        if (address == "/cutslider"){
            float val = m.getArgAsFloat(0);
            if(val > 0.5) glitch->setFx(OFXPOSTGLITCH_CUTSLIDER, true);
            else glitch->setFx(OFXPOSTGLITCH_CUTSLIDER, false);
        }
    }
}

//--------------------------------------------------------------
void ofApp::camLoadPos(string _filename) {
    ofFile inFile;
    inFile.open(_filename+".mat", ofFile::ReadOnly, true);
    inFile.read((char*) defaultPosition.getPtr(), sizeof(float) * 16);
    inFile.close();
    cam.setTransformMatrix(defaultPosition);
}

void ofApp::camSavePos(string _filename) {
    defaultPosition = cam.getGlobalTransformMatrix();
    ofFile outFile;
    outFile.open(_filename+".mat", ofFile::WriteOnly, true);
    outFile.write((char*) defaultPosition.getPtr(), sizeof(float) * 16);
    outFile.close();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    if(key == 'f') { ofToggleFullscreen(); camLoadPos("camPos0"); }
    else if(key == 'a') { df = 0; }
    else if(key == 's') { df = 2; }
    else if(key == 'z') { frame = 0; }
    else if(key == 'x') { frame = 1000; }
    else if(key == 't') { run = !run; }
    else if(key == 'g') { showGUI = !showGUI; }
    else if(key == OF_KEY_UP) { df += 0.1; }
    else if(key == OF_KEY_DOWN) { df -= 0.1; }
    else if(key == OF_KEY_LEFT) { frame -= 2; }
    else if(key == OF_KEY_RIGHT) { frame += 2; }
    
    
    else if(key == 'o') { displace -= 0.01; }
    else if(key == 'p') { displace += 0.01; }
    
    else if(key == 'u') { cam.move(0, 0, -30); }
    else if(key == 'n') { cam.move(0, 0, 30); }
    else if(key == 'l') { cam.move(-30, 0, 0); }
    else if(key == 'h') { cam.move(30, 0, 0); }
    else if(key == 'k') { cam.move(0, -30, 0); }
    else if(key == 'j') { cam.move(0, 30, 0); }
    
    else if(key == '<') { camSavePos("camPos0"); }
    else if(key == ',') { camLoadPos("camPos0"); }
    
    else if(key == 'r') {
        frame = df = displace = dDist = 0;
        opacity = 0.2;
        dx = dy = dz = 0;
        run = runDistort = 0;
        camLoadPos("camPos0");
    }
    
    else if(key == 'b') {
        ofPixels pix;
        pix.allocate(ofGetWidth(), ofGetHeight(), 4);
        fbo.readToPixels(pix);
        
        ofImage img;
        img.setFromPixels(pix);
        
        char name[32];
        sprintf(name, "./runs/frame_%05d.tif", frame_num);
        img.save(name);
        frame_num++;
        if ((frame_num % 100) == 0) sleep(10);
    }
    
    if (key == '1') glitch->setFx(OFXPOSTGLITCH_CONVERGENCE,    true);
    if (key == '2') glitch->setFx(OFXPOSTGLITCH_SHAKER,         true);
    if (key == '3') glitch->setFx(OFXPOSTGLITCH_CUTSLIDER,      true);
    if (key == '4') glitch->setFx(OFXPOSTGLITCH_NOISE,          true);
    
    if (key == '8') glitch->setFx(OFXPOSTGLITCH_CR_BLUERAISE,   true);
    if (key == '9') glitch->setFx(OFXPOSTGLITCH_CR_REDRAISE,    true);
    if (key == '0') glitch->setFx(OFXPOSTGLITCH_INVERT,         true);
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    if (key == '1') glitch->setFx(OFXPOSTGLITCH_CONVERGENCE,    false);
    if (key == '2') glitch->setFx(OFXPOSTGLITCH_SHAKER,         false);
    if (key == '3') glitch->setFx(OFXPOSTGLITCH_CUTSLIDER,      false);
    if (key == '4') glitch->setFx(OFXPOSTGLITCH_NOISE,          false);
    
    if (key == '8') glitch->setFx(OFXPOSTGLITCH_CR_BLUERAISE,   false);
    if (key == '9') glitch->setFx(OFXPOSTGLITCH_CR_REDRAISE,    false);
    if (key == '0') glitch->setFx(OFXPOSTGLITCH_INVERT,         false);
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){ }
void ofApp::mouseDragged(int x, int y, int button){ }
void ofApp::mousePressed(int x, int y, int button){ }
void ofApp::mouseReleased(int x, int y, int button){ }
void ofApp::mouseEntered(int x, int y){ }
void ofApp::mouseExited(int x, int y){ }
void ofApp::windowResized(int w, int h){ }
void ofApp::gotMessage(ofMessage msg){ }
void ofApp::dragEvent(ofDragInfo dragInfo){ }
