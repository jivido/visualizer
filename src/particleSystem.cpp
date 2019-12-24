//
//  particleSystem.cpp
//  Visualizer_1_2
//
//  Created by Jildert Viet on 04-12-16.
//
//

#include "particleSystem.hpp"

typedef struct{
    float2 vel;
    float2 spawnPos;
    float mass;
    float alpha;        // need this to make sure the float2 vel is aligned to a 16 byte boundary
//    float fadeInValue;
//    float dummy;
} particleSystemParticle;

msa::OpenCLBufferManagedT<particleSystemParticle> particles; // vector of Particles on host and corresponding clBuffer on device
msa::OpenCLBufferManagedT<float2> particlePos; // vector of particle positions on host and corresponding clBuffer, and vbo on device
msa::OpenCLBufferManagedT<float4> particleCos;

particleSystem::particleSystem(){
    init(numParticles);
}

particleSystem::particleSystem(int numParticles, ofVec2f size, ofFloatColor color, int testIndex){
    this->numParticles = numParticles;
    this->color = color;
    dimensions = size;
    customOneArguments[0] = testIndex;
    init(numParticles);
    clImage.initWithTexture(128, 80, GL_RGBA);
    clImage.getTexture().setTextureMinMagFilter(GL_LINEAR, GL_LINEAR); // Remove this later :
    // init particles
}

void particleSystem::init(int numParticles){
    setType("particleSystem");
    particlesPos = new float2[numParticles];
    
    opencl.setupFromOpenGL(1); // 1 is NVIDIA?

    // create vbo
    glGenBuffersARB(1, &vbo);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float2) * numParticles, 0, GL_DYNAMIC_COPY_ARB);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    
    glGenBuffersARB(1, &cbo);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, cbo);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float4) * numParticles, 0, GL_DYNAMIC_COPY_ARB);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    
    
//    particleCos.initBuffer(numParticles);
    
    // init host and CL buffers
    particles.initBuffer(numParticles);
    particlePos.initFromGLObject(vbo, numParticles);
    particleCos.initFromGLObject(cbo, numParticles);
    
    
//    clImage.initWithTexture(10, 10, GL_RGBA);
//    clImage.getTexture().setTextureMinMagFilter(GL_LINEAR, GL_LINEAR); // Remove this later
    
    for(int i=0; i<numParticles; i++) {
        particleSystemParticle &p = particles[i];
        p.vel.set(ofRandom(10000.), ofRandom(10000.));
        p.mass = ofRandom(0.00001, 1.0) / 10.;
        
        int x,y;
        x = sin(i)*ofGetHeight()*0.4; // For circle resetPos
        y = cos(i)*ofGetHeight()*0.4;
        x += (ofGetWidth()/2.) + ofRandom(-2.5, 2.5); // Bit of noise on the spawncircle
        y += (ofGetHeight()/2.) + ofRandom(-2.5, 2.5);
        p.spawnPos = float2(x,y);
        
        x = ofRandom(dimensions.x);
        y = ofRandom(dimensions.y);
        p.spawnPos = float2(x,y);
        particlePos[i].set(x,y);
        particleCos[i].set(1, 1.0, 1.0,0.4);
        p.alpha = 0.4; // Takes this as a "fade in to" value
    }
    
    particles.writeToDevice();
    particlePos.writeToDevice(); ///< uploads buffer data to shared CL/GL memory, so the vbo and the cl buffer are written in one go, since they occupy the same memory locations.
    particleCos.writeToDevice();
    
    
    opencl.loadProgramFromFile("MSAOpenCL/Particle.cl");
    opencl.loadKernel("updateParticle");
    
    

    opencl.kernel("updateParticle")->setArg(0, particles);
    opencl.kernel("updateParticle")->setArg(1, particlePos);
    opencl.kernel("updateParticle")->setArg(2, dimensions);
//    opencl.kernel("updateParticle")->setArg(3, clImage); // Only set it when the clImage is allocated (by setVecField function)
    opencl.kernel("updateParticle")->setArg(4, particleCos);
    opencl.kernel("updateParticle")->setArg(5, globalForce);
    opencl.kernel("updateParticle")->setArg(6, forceMultiplier);
    opencl.kernel("updateParticle")->setArg(7, traagheid);
    opencl.kernel("updateParticle")->setArg(8, fadeTime);
    
    glPointSize(2);
}

void particleSystem::setVecField(JVecField* vF){
    vecField = vF;
    
    
    cout << vF->density << endl;
    pixels = new unsigned char[(int)vF->density.x*(int)vF->density.y*4];
    return;
//    clImage.reset();
    cout << "initWithTexture" << endl;
//    clImage.initWithTexture(vF->texture); // GL_RGBA
    clImage.initFromTexture(vF->vecTex);
    cout << "setTextureMinMagFilter" << endl;
    clImage.getTexture().setTextureMinMagFilter(GL_LINEAR, GL_LINEAR); // Remove this later :
};


void particleSystem::specificFunction(){
    if(vecField && vecField->vecTex.getWidth() != 0){ // Update vecField-info
        ofPixels p;
        vecField->vecTex.readToPixels(p);
        int pixelIndex = 0;
        for(int i=0; i<vecField->density.x; i++) {
            for(int j=0; j<vecField->density.y; j++) {
                int indexRGB    = pixelIndex * 4;
                int indexRGBA    = pixelIndex * 4;

                pixels[indexRGBA  ] = p[indexRGB];// .getColor(i, j).r;
                pixels[indexRGBA+1] = p[indexRGB + 1]; // p.getColor(i, j).g;
                pixels[indexRGBA+2] = p[indexRGB + 2]; // p.getColor(i, j).b;
                pixels[indexRGBA+3] = p[indexRGB + 3];
                pixelIndex++;
            }
        }
//        cout << "Before writing to the CL image" << endl;
        clImage.write(pixels, true);
    } else{
        return; // Test: only work with vecField for now
    }
    
    opencl.kernel("updateParticle")->setArg(2, dimensions);
    opencl.kernel("updateParticle")->setArg(3, clImage);
    opencl.kernel("updateParticle")->setArg(5, globalForce);
    opencl.kernel("updateParticle")->setArg(6, forceMultiplier);
    opencl.kernel("updateParticle")->setArg(7, traagheid);
    opencl.kernel("updateParticle")->setArg(8, fadeTime);
    opencl.kernel("updateParticle")->setArg(9, destAlpha);
    
    glFlush();
    
//    cout << "Before run1D" << endl;
    opencl.kernel("updateParticle")->run1D(numParticles);
//    opencl.flush();
}

void particleSystem::display(){
    ofSetColor(255);
    opencl.finish();
    
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
    glVertexPointer(2, GL_FLOAT, 0, 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, cbo);
    glColorPointer(4, GL_FLOAT, 0, 0);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glPointSize(1);
    glDrawArrays(GL_POINTS, 0, numParticles);
    
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    opencl.finish();
//    opencl.finish();
//
//    glColor4f(color.r, color.g, color.b, color.a);
//    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
//
//    opencl.finish();
//
//    glEnableClientState(GL_VERTEX_ARRAY);
//    glVertexPointer(2, GL_FLOAT, 0, 0);
//    glDrawArrays(GL_POINTS, 0, numParticles);
//    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void particleSystem::customOne(){
    globalForce = float2(customOneArguments[0], customOneArguments[1]);
}

void particleSystem::customTwo(){
    forceMultiplier = float2(customOneArguments[2], customOneArguments[3]);
}

void particleSystem::customThree(){ // Set traagheid
    traagheid = customOneArguments[4];
}

void particleSystem::customFour(){ // Set alpha
    float alpha = colors[0].a / 255.;
    cout << "Set alpha: " << alpha << endl;
    for(unsigned int i=0; i<numParticles; i++) {
        particleSystemParticle &p = particles[i];
        particleCos[i].set(colors[0].r / 255., colors[0].g / 255., colors[0].b / 255., alpha);
        p.alpha = alpha; // Takes this as a "fade in to" value
    }
//    particleCos.initFromGLObject(cbo, numParticles);
    opencl.kernel("updateParticle")->setArg(4, particleCos);
    particleCos.writeToDevice(0, numParticles);
    destAlpha = alpha;
}

void particleSystem::customFive(){
    fadeTime = customOneArguments[0];
}

void particleSystem::setColor(ofColor c, int index){
    colors[0] = c;
    customFour();
}

void particleSystem::setSize(ofVec3f size){
    dimensions = size;
    this->size = size;
}
