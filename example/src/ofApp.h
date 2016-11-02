#pragma once

#include "ofMain.h"
#include "ofxV4L2Settings.h"
#include "ofxDatGui.h"

class ofApp : public ofBaseApp{

    public:
        void setup();
        void update();
        void draw();

        ofxV4L2Settings settings;
        ofVideoGrabber grabber;

        ofxDatGui* gui;

    private:
        void onButtonEvent(ofxDatGuiButtonEvent e);

};
