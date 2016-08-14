#pragma once

#include "ofMain.h"
#include "ofxGem.h"

class ofApp : public ofBaseApp {

  public:
    void setup();
    void update();
    void draw();

    ofVideoGrabber    vidGrabber;
    ofxGem pix_share;
};
