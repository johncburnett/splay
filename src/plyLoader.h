#pragma once
#include <thread>
#include <chrono>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <ofMain.h>

#include "../libs/tinyply/source/tinyply.h"

using namespace tinyply;


typedef std::chrono::time_point<std::chrono::high_resolution_clock> timepoint;
std::chrono::high_resolution_clock c;

inline std::chrono::time_point<std::chrono::high_resolution_clock> now()
{
    return c.now();
}

inline double difference_micros(timepoint start, timepoint end)
{
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}


class plyLoader {
public:
    uint32_t vertexCount, normalCount, colorCount, faceCount, faceTexcoordCount, faceColorCount;
    plyLoader(const std::string & filename);
    ~plyLoader();
    
    std::vector<float>* get_verts(){
        return &verts;
    }
    
    std::vector<float>* get_norms(){
        return &norms;
    }
    
    std::vector<uint8_t>* get_colors(){
        return &colors;
    }
private:
    void write_ply_example(const std::string & filename);
    void read_ply_file(const std::string & filename);
    
    // Define containers to hold the extracted data. The type must match
    // the property type given in the header. Tinyply will interally allocate the
    // the appropriate amount of memory.
    std::vector<float> verts;
    std::vector<float> norms;
    std::vector<uint8_t> colors;
    
    std::vector<uint32_t> faces;
    std::vector<float> uvCoords;
    
};