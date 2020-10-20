//
//  JPhysarum.cpp
//  visualizer
//
//  Created by Jildert Viet on 01-07-20.
//

#include "JPhysarum.hpp"

JPhysarum::JPhysarum(glm::vec2 loc, glm::vec2 size){
    setType("JPhysarum");
    this->size = size;
    this->loc = loc;
    
    numParticles = (size.x*size.y)*0.2;
    
    string shadersFolder;
    if(ofIsGLProgrammableRenderer()){
        shadersFolder="shaders_gl3";
    }else{
        shadersFolder="shaders";
    }
    
    // Loading the Shaders
    if(ofIsGLProgrammableRenderer()){
        updatePos.load(shadersFolder+"/passthru.vert", shadersFolder+"/posUpdate.frag");
        updateVel.load(shadersFolder+"/passthru.vert", shadersFolder+"/velUpdate.frag");
    } else{
        updatePos.load("",shadersFolder+"/posUpdate.frag");
        updateVel.load("",shadersFolder+"/velUpdate.frag");
        alphaDecay.load("", shadersFolder+"/alphaDecay.frag");
    }
    
    updateRender.load(shadersFolder+"/render");
    
    textureRes = (int)sqrt((float)numParticles);
    numParticles = textureRes * textureRes;
    
    ofFloatPixels p;
    p.allocate(textureRes, textureRes, 3);
    for (int x = 0; x < textureRes; x++){
        for (int y = 0; y < textureRes; y++){
            int i = textureRes * y + x;
            p.setColor(x, y, ofFloatColor((float)x / (float)textureRes,
                                          (float)y / (float)textureRes,
                                          1.0-pow(ofRandom(0.0001, 0.9), 10.0))
                       );
        }
    }
    
    ofImage img;
    img.setFromPixels(p);
    
    posPingPong.allocate(textureRes, textureRes, GL_RGBA32F);
    posPingPong.src->begin();
    ofClear(255);
    img.draw(0,0);
    posPingPong.src->end();
    
    velPingPong.allocate(textureRes, textureRes, GL_RGBA32F);
    velPingPong.src->begin();
    ofClear(255, 0, 0); // All in one direction
    velPingPong.src->end();
    
    renderPingPong.allocate(size.x, size.y, GL_RGBA32F);
    renderPingPong.src->begin();
    ofClear(0,0,0,0);
    ofSetColor(0, 255, 0);
    for(int i=0; i<1; i++){
        ofDrawRectangle(renderPingPong.src->getWidth()*0.5, 0, 30, renderPingPong.src->getHeight());
    }
    renderPingPong.src->end();
    
    mesh.setMode(OF_PRIMITIVE_POINTS);
    for(int x = 0; x < textureRes; x++){
        for(int y = 0; y < textureRes; y++){
            mesh.addVertex({x,y,0});
            mesh.addTexCoord({x, y});
        }
    }
    
//    gui.setup();
//    gui.add(sensorAngle.setup("sensorAngle", 45, 1, 360));
//    gui.add(sensorDistance.setup("sensorDistance", 50, 1, 500));
//    gui.add(turnAngle.setup("turnAngle", 45, 0, 360));
//    gui.add(speedMul.setup("speedMul", 2.5, 0.2, 30.0));
//    gui.add(decay.setup("decay", 0.5, 0, 1));
//    gui.add(depositAmount.setup("depositAmount", 40, 1., 255.));
//    gui.add(balance.setup("balance", 0.,0., 1.0));
//    gui.add(blurMix.setup("blurMix", 0.,0., 1.0));
}

void JPhysarum::display(){
    ofSetColor(colors[0]);
    renderPingPong.dst->draw(loc.x, loc.y);
    
//    velPingPong.dst->draw(0, 0);
//    velPingPong.dst->draw(velPingPong.dst->getWidth(), 0);
//    posPingPong.dst->draw(0, velPingPong.dst->getHeight());
//    posPingPong.dst->draw(velPingPong.dst->getWidth(), velPingPong.dst->getHeight());
    
    if(bDrawGui){
//        posPingPong.src->draw(0, 0);
//        velPingPong.src->draw(0, posPingPong.src->getHeight());
//        ofPushMatrix();
//        ofScale(2);
//        gui.draw();
//        ofPopMatrix();
    }
}

void JPhysarum::specificFunction(){
    velPingPong.dst->begin();
        ofClear(0);
        updateVel.begin();
            updateVel.setUniformTexture("velocityTex", velPingPong.src->getTexture(), 0);
            updateVel.setUniformTexture("positionTex", posPingPong.src->getTexture(), 1);
            updateVel.setUniformTexture("trailTex", renderPingPong.dst->getTexture(), 2);

            updateVel.setUniform1f("sensorAngle", sensorAngle);
            updateVel.setUniform1f("time", ofGetElapsedTimeMillis());
            updateVel.setUniform1f("sensorDistance", sensorDistance);
            updateVel.setUniform1f("turnAngle", turnAngle);

            updateVel.setUniform2f("resolution", glm::vec2(size.x, size.y));
            updateVel.setUniform1f("speedMul", speed);
            updateVel.setUniform1f("balance", balance);
            updateVel.setUniform1f("depositAmount", (float)depositAmount);
            updateVel.setUniform1f("blurMix", (float)blurMix);

            velPingPong.src->draw(0, 0);
        updateVel.end();
        velPingPong.dst->end();
    velPingPong.swap();

    posPingPong.dst->begin();
        ofClear(0);
        updatePos.begin();
            updatePos.setUniformTexture("velocityTex", velPingPong.src->getTexture(), 1);
            updatePos.setUniformTexture("positionTex", posPingPong.src->getTexture(), 0);
            updatePos.setUniformTexture("trailTex", renderPingPong.dst->getTexture(), 2);

            updatePos.setUniform1f("speedMul", speed);

            posPingPong.src->draw(0, 0);
        updatePos.end();
    posPingPong.dst->end();

    posPingPong.swap();
    
    renderPingPong.swap();
    renderPingPong.dst->begin();
    
//        if(ofGetFrameNum() > 1)
//            ofClear(0, decay * 255);
        alphaDecay.begin();
            alphaDecay.setUniformTexture("src", renderPingPong.src->getTexture(), 0);
            alphaDecay.setUniform1f("decay", decay);
            renderPingPong.src->draw(0, 0);
        alphaDecay.end();
    renderPingPong.dst->end();
    
    renderPingPong.dst->begin();
//        ofDrawBitmapString("test", 20, 20);
//        ofSetColor(0, 0, 0, decay); // Decay introduces a black rectangle now... Should use a shader and ping pong texture?
//        ofDrawRectangle(0, 0, renderFbo.getWidth(), renderFbo.getHeight());
        updateRender.begin();
            updateRender.setUniformTexture("posTex", posPingPong.src->getTexture(), 0);
            updateRender.setUniformTexture("alpha", velPingPong.src->getTexture(), 1);
            updateRender.setUniform2f("screen", (float)size.x, (float)size.y);

            ofEnableBlendMode(OF_BLENDMODE_ADD);
            ofSetColor(255,255);

            mesh.draw();

            ofDisableBlendMode();
            glEnd();
        updateRender.end();
//    ofClearAlpha();
    renderPingPong.dst->end();
}
