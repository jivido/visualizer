
//
//  JFboWindow.cpp
//  visualizer
//
//  Created by Jildert Viet on 12-12-19.
//

#include "JFboWindow.hpp"

JFboWindow::JFboWindow(){
    
}

void JFboWindow::setup(){
    
}

void JFboWindow::update(){
    
}

void JFboWindow::draw(){
    ofBackground(255, 0, 0);
    fbo->getTexture().drawSubsection(0, 0, ofGetWidth(), ofGetHeight(), 0, 0);
//    fbo->getTexture().draw(0, 0);
}