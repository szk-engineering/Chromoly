#pragma once

#include "ofMain.h"
#include "ofxChromaKey.h"
#include "ofxDatGui.h"
#include "ofxXmlSettings.h"

class ofApp : public ofBaseApp {
public:
  void setup();
  void update();
  void draw();
  void keyPressed(int key);
  void keyReleased(int key);
  void mouseDragged(int x, int y, int button);
  void mousePressed(int x, int y, int button);
  void mouseReleased(int x, int y, int button);
  void windowResized(int w, int h);
  void dragEvent(ofDragInfo dragInfo);
  void gotMessage(ofMessage msg);

  void importPhotos();
  void exportPhotos();

  bool isUpdated = true;
  int exportMovieFramerate = 10;
  int exportMovieSeconds = 3;

  ofImage foregroundImages[30];  // TODO: It's makeshift variable.
  ofImage backgroundImages;
  ofImage exportImage;

  ofxChromaKey chromaKey;
  ofxDatGui *gui;
  ofFbo fbo;
};