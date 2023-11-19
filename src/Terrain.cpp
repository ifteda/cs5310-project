#include "Terrain.hpp"
#include "Image.hpp"

#include <iostream>

// Constructor for our object
// Calls the initialization method
Terrain::Terrain(unsigned int xSegs, unsigned int zSegs, std::string fileName) : 
                m_xSegments(xSegs), m_zSegments(zSegs) {
    std::cout << "(Terrain.cpp) Constructor called \n";

    // Load up some image data
    Image heightMap(fileName);
    heightMap.LoadPPM(true);
    // Set the height data for the image
    float scale = 5.0f; // Note that this scales down the values to make
                        // the image a bit more flat.
    // Create height data
    m_heightData = new int[m_xSegments*m_zSegments];
    // Set the height data equal to the grayscale value of the heightmap
    // Because the R,G,B will all be equal in a grayscale image, then
    // we just grab one of the color components.

    // Populate height data
    for (unsigned int z = 0; z < m_zSegments; ++z) {
        for (unsigned int x = 0; x < m_xSegments; ++x) {
            m_heightData[z * m_xSegments + x] = (float) heightMap.GetPixelR(z,x)/scale;
        }
    }

    Init();
}

// Destructor
Terrain::~Terrain(){
    // Delete our allocatted higheithmap data
    if(m_heightData != nullptr){
        delete m_heightData;
    }
}


// Creates a grid of segments
// This article has a pretty handy illustration here:
// http://www.learnopengles.com/wordpress/wp-content/uploads/2012/05/vbo.png
// of what we are trying to do.
void Terrain::Init(){
    // Build grid of vertices
    for (unsigned int z = 0; z < m_zSegments; ++z) {
        for (unsigned int x = 0; x < m_xSegments; ++x){
            float u = 1.0f - ((float) x / (float) m_xSegments);
            float v = 1.0f - ((float) z / (float) m_zSegments);
            // Calculate the correct position and add the texture coordinates
            m_geometry.AddVertex(x, m_heightData[x+z*m_xSegments], z, u, v);
        }
    }

    // Build triangle strip
    for (unsigned int z = 0; z < m_zSegments-1; ++z) {
        for (unsigned int x = 0; x < m_xSegments-1; ++x) {
            m_geometry.AddIndex(z * m_zSegments + x);
            m_geometry.AddIndex(z * m_zSegments + x + m_xSegments);
            m_geometry.AddIndex((z * m_zSegments + 1) + x);

            m_geometry.AddIndex(z * m_zSegments + x + 1);
            m_geometry.AddIndex(z * m_zSegments + x + m_xSegments);
            m_geometry.AddIndex(z * m_zSegments + x + m_xSegments + 1);
        }
    }

   // Finally generate a simple 'array of bytes' that contains
   // everything for our buffer to work with.
   m_geometry.Gen();  
   // Create a buffer and set the stride of information
   m_vertexBufferLayout.CreateNormalBufferLayout(m_geometry.GetBufferDataSize(),
                                        m_geometry.GetIndicesSize(),
                                        m_geometry.GetBufferDataPtr(),
                                        m_geometry.GetIndicesDataPtr());
}

// Loads an image and uses it to set the heights of the terrain.
void Terrain::LoadHeightMap(Image image) { }