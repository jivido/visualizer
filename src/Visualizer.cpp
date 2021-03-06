//  Created by Jildert Viet on 24-01-16.

#include "Visualizer.hpp"

Visualizer::Visualizer(glm::vec2 size){
    this->size = size;
    for(unsigned short i=0; i<MAX_EVENTS_PTRS; i++)
        events[i] = nullptr;
    for(int i=0; i<NUMLAYERS; i++)
        layers.push_back(new Event()); // Dummy events
    
#if USE_PP
    post.init(ofGetWidth(), ofGetHeight());
    noisePP = post.createPass<NoiseWarpPass>();
    noisePP->setEnabled(false);
    fxaaPP = post.createPass<FxaaPass>();
    fxaaPP->setEnabled(false);
    rgbPP = post.createPass<RGBShiftPass>();
    rgbPP->setEnabled(false);
    
    zoomPP = post.createPass<ZoomBlurPass>();
    zoomPP->setEnabled(false);
#endif

    alphaScreen = new AlphaBlackScreen(true);
    alphaScreen->size = size;
    addEvent((Event*)alphaScreen, 0);
    setAlpha(255);
    
    for(int i=0; i<9; i++)
        busses.push_back(new Bus());
    
    loadLastMaskFile();

    verdana30.load("verdana.ttf", 50, true, true);
    verdana30.setLineHeight(34.0f);
    verdana30.setLetterSpacing(1.035);
    
    cout << "Visualizer made" << endl;
#ifndef TARGET_RASPBERRY_PI
    ofFbo::Settings fs; // For export quality
    fs.numSamples = 8;
    fs.width = size.x;
    fs.height = size.y;
    fs.internalformat = GL_RGBA16F;
    fs.useStencil = true;
    fbo.allocate(fs);
#else
    fbo.allocate(size.x, size.y, GL_RGBA);
#endif
    fbo.begin();
    ofClear(0, 0);
    fbo.end();
    
    initCam();
//    sharedFbo.allocate(2560, 800, GL_RGBA);
//    sharedFbo2.allocate(2560, 800, GL_RGBA);
}

void Visualizer::initCam(){
    cam.reset();
//    cam.rotate(-180, ofVec3f(0,1,0));
//    cam.setPosition(0, 0, 0);
//    cam.rotate(-180, ofVec3f(1,0,0));
//    cam.setDistance(fbo.getWidth() * 0.25);
//    cam.lookAt(ofVec3f(0,0,0));
    
//    cam.move(0, ofGetHeight() * -0.5, 0);
//    cam.move(fbo.getWidth() * 0.5, fbo.getHeight() * 0.5, 0); // Temp for ADEtje
    
    if(!camController){
        camController = new cameraController(&cam);
        addEvent((Event*)camController, 0);
    }
}

Visualizer::~Visualizer(){
    cout << "Visualizer deleted" << endl;
    // Delete all events?
    for(int i=0; i<layers.size(); i++)
        layers[i][0].deleteNext();
    for(int i=0; i<mappers.size(); i++)
        delete mappers[i];
}

void Visualizer::deconstructor(){
    delete this;
}

void Visualizer::display(){
#if USE_PP
        glPushAttrib(GL_ENABLE_BIT);
        // setup gl state
//        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        post.begin();
#endif

    if(bRotate){
//        ofPushMatrix();
//        ofTranslate(ofGetWidth()/2., 0);
//        ofRotateZ(rotationAngle);
    }
    
    if(bMirror){
        ofTranslate(ofGetWindowWidth(), 0);
        ofRotateY(180);
    }
    
    ofSetColor(255);
    fbo.draw(0,0); // See update();
    
    if(bAddMirror){
        ofPushMatrix();

        ofTranslate(ofGetWindowWidth(), 0);
        ofRotateY(180);
        
        for(uint8 i=2; i<layers.size()-2; i++){ // Everything except alphascreen and non-camera-layer
            layers[i]->displayMain();
        }

        ofPopMatrix();
    }
    
#if USE_PP
        post.end();
        // set gl state back to original
        glPopAttrib();
#endif
}

void Visualizer::update(){
    if(bRotate)
        rotationAngle += rotationAngleIcrement;
    
    for(uint8 i=0; i<layers.size(); i++)
        layers[i]->updateMain();
    
//    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Found this on the OF forum
    // Goal: two grey area's should be brighter when they overlap, currently they merge in the same greyness
    
//    ofEnableBlendMode(OF_BLENDMODE_ADD);
//    ofEnableDepthTest();
    fbo.begin();
//        ofEnableSmoothing();
        alphaScreen->displayMain();
        layers[1]->displayMain(); // Non-cam layer back

        if(bCam){
            ofPushMatrix();
            cam.begin();
            ofScale(1, -1, 1);
            ofTranslate(-ofGetWidth()/2., -ofGetHeight()/2.);
        }
        for(uint8 i=2; i<layers.size()-2; i++) // Don't draw the last one, that's fade, happens at end // But I removed this, so now we have one layer too much
            layers[i]->displayMain();
        if(bCam){
            cam.end();
        ofPopMatrix();
        }

        layers[NUMLAYERS-2]->displayMain(); // Non-cam layer front
//        ofClearAlpha();
    fbo.end();
}

bool Visualizer::checkIfNull(Event* e){
    if(e){
        return false;
    } else{
        return true;
    }
}

bool Visualizer::checkIfNullM(Mapper* m){
    if(m){
        return false;
    } else{
        return true;
    }
}

Event* Visualizer::addEvent(Event* e, int layerIndex, unsigned short index){ // Index 0 means: don't save
    if(index){
        if(index < MAX_EVENTS_PTRS){
            if(events[index]){
                cout << "Position in array is occupied, delete" << endl;
                delete events[index]; // Sets events[index] to nullptr? Doesn't matter...
                events[index] = nullptr;
            }
            events[index] = e; // For accessing events later, with id
            cout << "Added to ptrs[] w/ index: " << index << endl;
        } else{
            cout << "Can't save new event, need a reset? SC should notice this and call killAll() ?" << endl;
            return nullptr;
        }
    }
    
    layers[layerIndex]->addEvent(e);
    last = e;
    e->lastPtr = &last; // For setting to nullptr if event is deleted
    
    numEvents++;
//    cout << "event ID: " << e->id << endl; Used?
    
//    e->SCsender = SCsender;
    e->numEventsPtr = &numEvents;
    
    e->events = events;
    e->id = index;
    e->mappersParent = &mappers;

//    e->receivingPointers = &receivingPointers;
    return e;
}

Event* Visualizer::addEvent(Event* e, VisualizerLayer l, unsigned short index){
    switch(l){
        case NON_CAM_BACK:
            return addEvent(e,1, index);
            break;
        case NON_CAM_FRONT:
            return addEvent(e,NUMLAYERS-2, index);
            break;
        case DEFAULT:
            return addEvent(e, 2, index);
            break;
        default:
            break;
    }
return nullptr;
}

void Visualizer::setAlpha(int alpha, bool bDo){
    if(bDo){
        alphaScreen->setAlpha(alpha);
        alphaScreen->setActiveness(true);
    } else{
        alphaScreen->setAlpha(alpha);
    }
}

void Visualizer::setBrightness(unsigned char b){
    brightness = b;
}

void Visualizer::addMapper(Mapper* m){ // Not using this for >3 years I guess
    mappers.push_back(m);
}

void Visualizer::killAll(){
    alphaScreen->bGradient = false;
#if USE_PP
    bPostProcessing = false;
#endif
    bAddMirror = false;
//    ofSetFrameRate(60);
    
    for(uint8 i=1; i<layers.size()-1; i++){
        if(layers[i]->next)
            layers[i]->next->deleteNext();
    }
    
    for(unsigned short i=0; i<MAX_EVENTS_PTRS; i++)
        events[i] = nullptr;
}

void Visualizer::makeFit(glm::vec2 size){
    if(this->size == size)
        return;
    alphaScreen->size = size;
    if(fbo.getWidth() != size.x || fbo.getHeight() != size.y){
        fbo.allocate(size.x, size.y, fbo.getTexture().getTextureData().glInternalFormat);
    }
    this->size = size;
}

vector<float> Visualizer::vec(float a){
    vector<float> v;
    v.push_back(a);
    return v;
}

vector<float> Visualizer::vec(float a, float b){
    vector<float> v;
    v.push_back(a); v.push_back(b);
    return v;
}
vector<float> Visualizer::vec(float a, float b, float c){
    vector<float> v;
    v.push_back(a); v.push_back(b); v.push_back(c);
    return v;
}
vector<float> Visualizer::vec(float a, float b, float c, float d){
    vector<float> v;
    v.push_back(a); v.push_back(b); v.push_back(c); v.push_back(d);
    return v;
}
vector<float> Visualizer::vec(float a, float b, float c, float d, float e){
    vector<float> v;
    v.push_back(a); v.push_back(b); v.push_back(c); v.push_back(d); v.push_back(e);
    return v;
}

void Visualizer::loadLastMaskFile(){
    ofFile f;
    f.open("./maskFile.txt", ofFile::Mode::ReadOnly);
    ofBuffer b;
    b = f.readToBuffer();
    string path = b.getText();
    if(path.length() > 5){
        mask.load(path);
        maskBrightness = 0;
        bMask = true;
    }
}

ofxOscMessage Visualizer::getAllEvents(){
    ofxOscMessage m;
    m.setAddress("/allEvents");
    for(unsigned short i=1; i<NUMLAYERS-1; i++){
        // A, B, C
        if(layers[i]->next){
            Event* lastRead = layers[i]->next; // First: A (skip dummy Event)
            m.addIntArg(lastRead->id);
            m.addStringArg(lastRead->type);
            
            while(lastRead->next){ // Is there a B? Yes.
                lastRead = lastRead->next; // So this is B.
                m.addIntArg(lastRead->id);
                m.addStringArg(lastRead->type);
                // has B a next? Yes. Has C a next? No
            }
        } else{
             // Only the dummy Event
        }
    }
    return m;
}

Event* Visualizer::getEventById(int idToFind){
    if(idToFind<MAX_EVENTS_PTRS){
        if(events[idToFind]){
            return events[idToFind];
        } else{
            return nullptr;
        }
    } else{
        return nullptr;
    }
    
//    if(last){
//        if(last->id == idToFind){
//            return last;
//        }
//    }
//
//    for(int i=0; i<NUMLAYERS; i++){
//        Event* toCheck = layers[i]; // Dummy
//        while(toCheck->next){
//            cout << "test: layer: " << i << " " << toCheck->next->id << endl;
//            if(toCheck->next->id == idToFind){
//                return toCheck->next;
//            }
//            toCheck = toCheck->next;
//        }
//    }
//    return nullptr;
}

void Visualizer::getFreePointers(){
    unsigned short freeEvents[MAX_EVENTS_PTRS];
    unsigned short numFreeEvents = 0;
    for(unsigned short i=1; i<MAX_EVENTS_PTRS; i++){ // Keep 0 free... 
        if(events[i] == nullptr){
            freeEvents[numFreeEvents] = i;
            numFreeEvents++;
        }
    }
    ofxOscMessage m;
    m.setAddress("/freePointers");
    for(unsigned short i=0; i<numFreeEvents; i++){
        m.addInt32Arg(freeEvents[i]);
    }
    SCsender->sendMessage(m);
    
    // [3, 4, 5 .. 512], when only 0, 1, 2 are still occupied / alive. numFreeEvents should 509
    
    // So should have a 'blank list', of all ptrs[x] that are free...
    // @ Reset receive a bunch of numbers that are free, use that for setting new addresses @ SC...
    
}

void Visualizer::initCircularMaskFbo(glm::vec2 size, int num){
    cout << "create circle mask w/ size: " << size << endl;
    // create one circle first
    circularMask.allocate(size.x, size.y, GL_RGBA);
    ofPixels p;
    p.allocate(size.x, size.y, OF_PIXELS_RGBA);
    float yRatio, xRatio;
    for(int i=0; i<p.getWidth(); i++){
        xRatio = (float)i / p.getWidth();
        xRatio = pow((1-abs(sin(xRatio*(num*PI)))), 0.5);
        for(int j=0; j<p.getHeight(); j++){
            yRatio = float(j) / p.getHeight();
            yRatio = pow((1-sin(yRatio*(PI))), 0.5);
            float toWrite = (yRatio + xRatio) * 0.5;
            ofColor c = ofColor(0, 255 * toWrite); // Half phase of sine
            p.setColor(i, j, c);
        }
    }
    circularMask.getTexture().loadData(p);
}

void Visualizer::drawMask(){
    if(bMask){
        ofSetColor(255, maskBrightness);
        mask.draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
    }
    if(bDrawCirclularMask){
        ofSetColor(255);
        circularMask.draw(0, 0);
    }
}
