#pragma once

#define USE_COLOR 0
#include <ofMain.h>
#include "plyLoader.h"

class Fusion {
public:
    Fusion(int _n);
    ~Fusion();

    struct Mesh {
        ofFbo pos;
        ofFbo norm;
    };
    
    void load(std::string, std::string, int);
    void addMesh(std::string path);

    void update();
    void draw();

    void setMesh0(int index);
    void setMesh1(int index);
    
    void setInterp(float);
    void setOpacity(float);
    void setScale(float);
    void setDisplacement(float);
    
    void setScanX(float, float, int);
    void setScanY(float, float, int);
    
    void setPos0(float, float, float);
    void setPos1(float, float, float);
    
    void setDof(ofPoint, ofPoint, float);
    
private:
    ofVboMesh vbo;
    std::vector<Mesh*> meshes;
    ofShader shader;
    int i0, i1;
    
    int n;
    int w, h;
    float interp, opacity, scale, displace;
    float x0, y0, z0, x1, y1, z1;
    float scanX, scanY, scanWidth, scanHeight, scanRate;
    ofPoint cam, dofCenter;
    float bell;
    int runScan;
};
