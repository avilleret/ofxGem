#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

  ofSetLogLevel(OF_LOG_VERBOSE);
  pix_share.setup("test",320,240,4); // "test" is the name of the shared memory, you should use the same on the other side
                                     // 320, 240 is the size image
                                     // 4 is the number of byte per pixel (could be 1, 2 or 4)
  pix_share2.setup(12,320,240,4);    // setup a second shared memory segment with id '12'
}

//--------------------------------------------------------------
void ofApp::update(){

  // get the pixels from the shared memory
  // this method return ofPixels
  // and load it into an internal ofImage (used by draw() )
  pix_share.getPixels();
  pix_share2.getPixels();
}

//--------------------------------------------------------------
void ofApp::draw(){

  // draw what we load into shared memory
  pix_share.draw(0,0,320,240);
  pix_share2.draw(0,240,320,240);
}
