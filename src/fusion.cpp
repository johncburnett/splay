#include "Fusion.h"

Fusion::Fusion(int _n){
    interp = 0;
    opacity = 0.1;
    scale = 200;
    displace = 0;
    x0 = y0 = z0 = 0;
    x1 = y1 = z1 = 0;
    
    scanX = scale;
    scanY = scale;
    scanHeight = 40;
    scanWidth = 0;
    scanRate = 0;
    runScan = 0;

    head = 0;
    tail = 1;

    n = _n;
    w = h = std::sqrt(n/3);

    vbo.clear();
    for (unsigned y = 0; y < h; ++y) {
        for (unsigned x = 0; x < w; ++x) {
            vbo.addVertex(ofVec3f(0,0));
            vbo.addTexCoord(ofVec2f(x, y));
        }
    }
    vbo.setMode(OF_PRIMITIVE_POINTS);

    shader.load("shaders/draw");
}

Fusion::~Fusion() {}

void Fusion::addMesh(std::string path) {
    Mesh *mesh = new Mesh();
    plyLoader ply(path);

    std::vector<float>* pv = ply.get_verts();
    std::vector<float>* nv = ply.get_norms();
    std::vector<float>::iterator it = pv->begin();
    std::vector<float>::iterator it_norm = nv->begin();

    float scale = 1.0;
    it = pv->begin();
    float pxtotal = 0;
    float *positions = new float[w * h * 4];
    float *normals   = new float[w * h * 4];
    
    bool has_norms = (nv->size() != 0);
    
    for (unsigned y = 0; y < h; ++y) {
        for (unsigned x = 0; x < w; ++x) {
            float px, py, pz, nx, ny, nz;
            if (it != pv->end()){
                px = (*it); it++;
                py = (*it); it++;
                pz = (*it); it++;
                
                if (has_norms) {
                    nx = (*it_norm); it_norm++;
                    ny = (*it_norm); it_norm++;
                    nz = (*it_norm); it_norm++;
                }
            } else {
                px = py = pz = 0;
                nx = ny = nz = 0;
            }
            
            unsigned idx = y * w + x;
            positions[idx * 4 + 0] = px*scale; // particle x
            positions[idx * 4 + 1] = py*scale; // particle y
            positions[idx * 4 + 2] = pz*scale; // particle z
            positions[idx * 4 + 3] = 0;
            
            normals[idx * 4 + 0] = nx; // particle x
            normals[idx * 4 + 1] = ny; // particle y
            normals[idx * 4 + 2] = nz; // particle z
            normals[idx * 4 + 3] = 0;
        }
    }
    
    ofFbo::Settings s;
    s.internalformat = GL_RGBA32F_ARB;
    s.textureTarget = GL_TEXTURE_RECTANGLE_ARB;
    s.minFilter = GL_NEAREST;
    s.maxFilter = GL_NEAREST;
    s.wrapModeHorizontal = GL_CLAMP;
    s.wrapModeVertical = GL_CLAMP;
    s.width = w;
    s.height = h;
    
    mesh->pos.clear();
    mesh->pos.allocate(s);
    
    mesh->norm.clear();
    mesh->norm.allocate(s);
    
    mesh->pos.getTexture().bind();
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, w, h, GL_RGBA, GL_FLOAT, positions);
    mesh->pos.getTexture().unbind();
    
    mesh->norm.getTexture().bind();
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, w, h, GL_RGBA, GL_FLOAT, normals);
    mesh->norm.getTexture().unbind();
    
    delete[] positions;
    delete[] normals;

    meshes.push_back(mesh);
}

void Fusion::setHead(int index) { head = index; }
void Fusion::setTail(int index) { tail = index; }

void Fusion::setInterp(float _interp) { interp = _interp; }
void Fusion::setOpacity(float _opacity) { opacity = _opacity; }
void Fusion::setScale(float _scale) { scale = _scale; }
void Fusion::setDisplacement(float _displace) { displace = _displace; }

int Fusion::getNumModels() {
    return meshes.size();
}

void Fusion::setScanX(float width, float rate, int run) {
    runScan = run;
    scanWidth = width;
    scanRate = rate;
}

void Fusion::setScanY(float height, float rate, int run) {
    runScan = run;
    scanHeight = height;
    scanRate = rate;
}

void Fusion::setPos0(float x, float y, float z) { x0 = x; y0 = y, z0 = z; }
void Fusion::setPos1(float x, float y, float z) { x1 = x; y1 = y, z1 = z; }

void Fusion::setDof(ofPoint _cam, ofPoint _dof, float _bell) {
    cam = _cam;
    dofCenter = _dof;
    bell = _bell;
}

void Fusion::update(){
    if (runScan) {
        scanY = (scanY - scanRate);
        if (scanY < 0) scanY = scale;
//        scanX = (scanX - scanRate);
//        if (scanX < 0) scanX = scale;
    }
}

void Fusion::draw(){
    Mesh *mesh0 = meshes[head];
    Mesh *mesh1 = meshes[tail];

    shader.begin();
    shader.setUniformTexture("tex0", mesh0->pos.getTexture(), 0);
    shader.setUniformTexture("tex1", mesh1->pos.getTexture(), 1);
    shader.setUniformTexture("tex2", mesh0->norm.getTexture(), 2);
    shader.setUniformTexture("tex3", mesh1->norm.getTexture(), 3);
    
    shader.setUniform1f("scale", scale);
    shader.setUniform1f("opacity", opacity);
    
    shader.setUniform1f("interp", interp);
    shader.setUniform1f("displace", displace);
    
//    shader.setUniform1f("scanX", scanX);
    shader.setUniform1f("scanY", scanY);
//    shader.setUniform1f("scanWidth", scanWidth);
    shader.setUniform1f("scanHeight", scanHeight);
    shader.setUniform1i("runScan", runScan);
    shader.setUniform1i("wrapScan", 1);
    
    shader.setUniform4f("pos0", ofVec4f(x0, y0, z0, 0));
    shader.setUniform4f("pos1", ofVec4f(x1, y1, z1, 0));
    
    shader.setUniform4f("cam", cam.x, cam.y, cam.z, -cam.dot(dofCenter) * scale);
    shader.setUniform1f("bell", bell);

    vbo.draw();
    shader.end();
}
