#include "ofMain.h"
#include "ofApp.h"
#include "ofAppGLFWWindow.h"
#include "JFboWindow.hpp"

// ./visualizer width height framerate numWindows fullscreen order
// ./visualizer 3840 1080 30 2 f 0 1

int main(int argc, char *argv[]){
    shared_ptr<ofApp> app(new ofApp());
    
    app->arguments = vector<string>(argv, argv + argc);
    for(int i=0; i<app->arguments.size(); i++)
        cout << "arg[" << i << "]: " << app->arguments[i] << endl;
    
    if(app->arguments[1] == "-h"){ // Help info
        cout << "Run like: " << endl;
        cout << "\tvisualizer width height framerate numOFWindows numScreens" << endl;
        cout << "\tvisualizer 3840 1080 30 2 1 <-- Creates two OF windows" << endl;
        cout << "\tvisualizer 3840 1080 30 1 2 <-- Creates one OF window, and has two circles for shading" << endl;
        exit(0);
    }
    
    if(app->arguments.size() >= 6){
        if(app->arguments[2] != "YES"){ // arguments[2] is YES when ran from XCode
            app->size = glm::vec2(ofToInt(app->arguments[1]), ofToInt(app->arguments[2]));
            app->frameRate = ofToFloat(app->arguments[3]);
        } // else: default size is set in ofApp.h
    }
    
    ofGLFWWindowSettings mainSettings;
#ifndef TARGET_RASPBERRY_PI
    mainSettings.setGLVersion(2, 1); // (2, 1) for ofxMSAOpenCL
#endif
    
    mainSettings.windowMode = OF_WINDOW;
//    mainSettings.multiMonitorFullScreen = true;
    
    bool bDualWindow = false;
    if(app->arguments.size() >= 5){
        if(ofToInt(app->arguments[4]) == 2)
            bDualWindow = true;
        app->numCircles = ofToInt(app->arguments[5]);
    }
    
    if(bDualWindow){
        cout << "Dual window" << endl;
        shared_ptr<JFboWindow> fboWindow(new JFboWindow());
        ofGLFWWindowSettings secondWindowSettings;
#ifndef TARGET_RASPBERRY_PI
        secondWindowSettings.setGLVersion(2, 1);
#endif
        secondWindowSettings.monitor = 1;

        app->size.x *= 0.5;
        app->bUseFbo = true;
        
        mainSettings.setSize(app->size.x, app->size.y);
        mainSettings.monitor = 0;
//        mainSettings.multiMonitorFullScreen = true;

        shared_ptr<ofAppBaseWindow> mainWindow = ofCreateWindow(mainSettings);
        ofRunApp(mainWindow, app);
        
        secondWindowSettings.setSize(app->size.x, app->size.y);
        secondWindowSettings.resizable = true;
        secondWindowSettings.shareContextWith = mainWindow;
        shared_ptr<ofAppBaseWindow> secondWindow = ofCreateWindow(secondWindowSettings);
        fboWindow->fbo = &(app->f);
        fboWindow->frameRate = ofToFloat(app->arguments[3]);
        fboWindow->bFullScreen = ofToBool(app->arguments[5]);
        ofRunApp(secondWindow, fboWindow);
    } else{
        app->size = glm::vec2(1920, 1080);
//        app->size = glm::vec2(1280, 800);
        cout << "Single window [" << app->size.x << ", " << app->size.y << "]\n" << endl;
        mainSettings.setSize(app->size.x, app->size.y);
        shared_ptr<ofAppBaseWindow> mainWindow = ofCreateWindow(mainSettings);
        ofRunApp(mainWindow, app);
    }
    ofRunMainLoop();
}
