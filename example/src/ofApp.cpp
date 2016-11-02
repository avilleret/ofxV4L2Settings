#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    gui = new ofxDatGui();
    gui->addHeader("V4L2Settings");
    settings.setup("/dev/video0");
    gui->addButton("save");
    gui->addButton("load");
    gui->addFolder(settings.parameters);

    gui->onButtonEvent(this, &ofApp::onButtonEvent);

    grabber.initGrabber(640,480);
}

//--------------------------------------------------------------
void ofApp::update(){
    grabber.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    grabber.draw(200,0);
}

void ofApp::onButtonEvent(ofxDatGuiButtonEvent e){
    if ( e.target->is("save")){
        ofLogNotice("ofApp") << "save settings";
        settings.save("settings.xml");
    } else if (e.target->is("load")){
        ofLogNotice("ofApp") << "reload settings";
        settings.load();
    }
}
