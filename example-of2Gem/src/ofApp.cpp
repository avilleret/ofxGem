#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

  ofSetLogLevel(OF_LOG_VERBOSE);
  vidGrabber.setVerbose(true);
  vidGrabber.setPixelFormat(OF_PIXELS_RGBA); // Gem works only with RGBA, MONO or YUV color
  vidGrabber.setup(320,240);
  pix_share.setup("test",320,240,4); // "test" is the name of the shared memory, you should use the same on the other side
                                     // 320, 240 is the size image
                                     // 4 is the number of byte per pixel (could be 1, 2 or 4)

}

//--------------------------------------------------------------
void ofApp::update(){

  vidGrabber.update();

  if (vidGrabber.isFrameNew()){
    // load video pixel data into shared memory
    pix_share.setPixels(vidGrabber.getPixels());
  }

}

//--------------------------------------------------------------
void ofApp::draw(){

  // draw what we load into shared memory
  pix_share.draw(0,0,320,240);

}
