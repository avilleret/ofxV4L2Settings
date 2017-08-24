/*
 * ofxV4L2Settings.cpp
 *
 *  Created on: 03/03/2012
 *      Author: arturo
 */

#include "ofxV4L2Settings.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/types.h>
#include <libv4l2.h>
#include "ofLog.h"
string ofxV4L2Settings::LOG_NAME = "ofxV4L2Settings";


void ofxV4L2Settings::addControl(int fd, const struct v4l2_queryctrl & ctrl, const struct v4l2_control & c){
    ids.push_back(c.id);

    switch(ctrl.type){
    case V4L2_CTRL_TYPE_INTEGER:
    {
        ofxDatGuiSlider * slider = parameters->addSlider((char*)ctrl.name,ctrl.minimum,ctrl.maximum,c.value);
        slider->setPrecision(0);
        break;
    }
    case V4L2_CTRL_TYPE_BOOLEAN:
    {
        parameters->addToggle((char*)(ctrl.name),c.value);
        break;
    }
    case V4L2_CTRL_TYPE_MENU:
    {
        struct v4l2_querymenu menu;
        vector<string> menu_options;
        menu.id = c.id;
        for(int j=0;j<=ctrl.maximum;j++){
            menu.index = j;
            if(v4l2_ioctl(fd, VIDIOC_QUERYMENU, &menu)==0){
                ofLogVerbose(LOG_NAME) << "    " << j << ": " << menu.name;
                menu_options.push_back((char*)menu.name);
            }else{
                ofLogError(LOG_NAME) << "error couldn0t get menu option " << j<< strerror(errno);
            }
        }
        parameters->addDropdown((char*)ctrl.name,menu_options);
        break;
    }
    case V4L2_CTRL_TYPE_BUTTON:
    {
      parameters->addButton((char*)ctrl.name);
      break;
    }
    case V4L2_CTRL_TYPE_STRING:
    {
      parameters->addTextInput((char*)ctrl.name, (char*)ctrl.default_value);
      break;
    }
    case V4L2_CTRL_TYPE_INTEGER64:
    case V4L2_CTRL_TYPE_INTEGER_MENU:
    default:
    ;
    }
}

ofxV4L2Settings::ofxV4L2Settings() : parameters(nullptr), m_filename("v4l2settings.xml") {
    fd=0;
}

ofxV4L2Settings::~ofxV4L2Settings() {
    if (parameters) delete parameters;
    if(fd) v4l2_close(fd);
}

void ofxV4L2Settings::save(string filename){
    m_filename = filename;
    guisettings.save(filename, parameters);
}

void ofxV4L2Settings::load(string filename){
    guisettings.load(filename, parameters);
}

void ofxV4L2Settings::onButtonEvent(ofxDatGuiButtonEvent e)
{
    set(e.target->getName(),0);
}

void ofxV4L2Settings::onToggleEvent(ofxDatGuiToggleEvent e)
{
    set(e.target->getName(),e.checked);
}

void ofxV4L2Settings::onSliderEvent(ofxDatGuiSliderEvent e)
{
    set(e.target->getName(),e.value);
}

void ofxV4L2Settings::onTextInputEvent(ofxDatGuiTextInputEvent e)
{
    cout << "onTextInputEvent not implemented" << endl;
}

void ofxV4L2Settings::onDropdownEvent(ofxDatGuiDropdownEvent e)
{
    set(e.target->getName(), e.child);
}

bool ofxV4L2Settings::setup(string device){
    struct v4l2_queryctrl ctrl;
    struct v4l2_control c;

    parameters = new ofxDatGuiFolder("v4l2 parameters of " + device);

    parameters->onButtonEvent(this, &ofxV4L2Settings::onButtonEvent);
    parameters->onToggleEvent(this, &ofxV4L2Settings::onToggleEvent);
    parameters->onSliderEvent(this, &ofxV4L2Settings::onSliderEvent);
    parameters->onTextInputEvent(this, &ofxV4L2Settings::onTextInputEvent);
    parameters->onDropdownEvent(this, &ofxV4L2Settings::onDropdownEvent);

    fd = v4l2_open(device.c_str(), O_RDWR, 0);
    if(fd < 0) {
        ofLogError(LOG_NAME) <<  "Unable to open " << device.c_str() << " " << strerror(errno);
        return false;
    }

    std::vector<std::string> inputs;
    v4l2_input input;
    input.index = 0;
    while ( v4l2_ioctl (fd, VIDIOC_ENUMINPUT, &input) == 0)
    {
      std::stringstream ss;
      if (input.name[0] != '\0')
        ss << input.name;
      else
        ss << "untitled input " << input.index;

      inputs.push_back(ss.str());
      input.index++;
    }
    if (!inputs.empty())
    {
      ofxDatGuiFolder* input_folder = parameters->addFolder("input");
      input_folder->addDropdown("inputs", inputs);
    }

#ifdef V4L2_CTRL_FLAG_NEXT_CTRL
    /* Try the extended control API first */
    ctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    if(v4l2_ioctl (fd, VIDIOC_QUERYCTRL, &ctrl)==0) {
        do {
            c.id = ctrl.id;
            ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
            if(ctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
                continue;
            }
            if(ctrl.type != V4L2_CTRL_TYPE_INTEGER &&
               ctrl.type != V4L2_CTRL_TYPE_BOOLEAN &&
               ctrl.type != V4L2_CTRL_TYPE_BUTTON  &&
               ctrl.type != V4L2_CTRL_TYPE_STRING  &&
               ctrl.type != V4L2_CTRL_TYPE_MENU) {
                continue;
            }
            if(v4l2_ioctl(fd, VIDIOC_G_CTRL, &c) == 0) {
                addControl(fd,ctrl,c);
                // control.parameter->addListener(this,&ofxV4L2Settings::parameterChanged);
            }
        } while(v4l2_ioctl (fd, VIDIOC_QUERYCTRL, &ctrl)== 0);
    } else
#endif
    {
        /* Check all the standard controls */
        for(int i=V4L2_CID_BASE; i<V4L2_CID_LASTP1; i++) {
            ctrl.id = i;
            if(v4l2_ioctl(fd, VIDIOC_QUERYCTRL, &ctrl) == 0) {
                if(ctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
                    continue;
                }
                if(ctrl.type != V4L2_CTRL_TYPE_INTEGER &&
                   ctrl.type != V4L2_CTRL_TYPE_BOOLEAN &&
                   ctrl.type != V4L2_CTRL_TYPE_BUTTON  &&
                   ctrl.type != V4L2_CTRL_TYPE_STRING  &&
                   ctrl.type != V4L2_CTRL_TYPE_MENU) {
                    continue;
                }

                c.id = i;
                if(v4l2_ioctl(fd, VIDIOC_G_CTRL, &c) == 0) {
                    addControl(fd,ctrl,c);
                    // control.parameter.addListener(this,&ofxV4L2Settings::parameterChanged);
                }
            }
        }

        /* Check any custom controls */
        for(int i=V4L2_CID_PRIVATE_BASE; ; i++) {
            ctrl.id = i;
            if(v4l2_ioctl(fd, VIDIOC_QUERYCTRL, &ctrl) == 0) {
                if(ctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
                    continue;
                }
                if(ctrl.type != V4L2_CTRL_TYPE_INTEGER &&
                   ctrl.type != V4L2_CTRL_TYPE_BOOLEAN &&
                   ctrl.type != V4L2_CTRL_TYPE_BUTTON  &&
                   ctrl.type != V4L2_CTRL_TYPE_STRING  &&
                   ctrl.type != V4L2_CTRL_TYPE_MENU) {
                    continue;
                }

                c.id = i;
                if(v4l2_ioctl(fd, VIDIOC_G_CTRL, &c) == 0) {
                    addControl(fd,ctrl,c);
                    // control.parameter.addListener(this,&ofxV4L2Settings::parameterChanged);
                }
            } else {
                break;
            }
        }
    }

    parameters->expand();

    return true;

}

bool ofxV4L2Settings::set(string name, int value){
    struct v4l2_queryctrl ctrl;
    struct v4l2_control c;

    int i = 0;
    for ( ; i<parameters->children.size(); i++){
        if (parameters->children[i]->is(name)){
            ctrl.id = ids[i];
            break;
        }
    }
    if (i==parameters->children.size()) {
        ofLogError(LOG_NAME) << name << " not found";
        return false;
    }

    if(v4l2_ioctl(fd, VIDIOC_QUERYCTRL, &ctrl) == 0) {
        if(strcmp((char *)ctrl.name, name.c_str())) {
            ofLogError() << "Control name mismatch " << name <<"!="<< ctrl.name;
            return false;
        }

        if(ctrl.flags & (V4L2_CTRL_FLAG_READ_ONLY |
                         V4L2_CTRL_FLAG_DISABLED |
                         V4L2_CTRL_FLAG_GRABBED)) {
            ofLogError(LOG_NAME) << name << " not writable";
            return false;
        }
        if(ctrl.type != V4L2_CTRL_TYPE_INTEGER &&
           ctrl.type != V4L2_CTRL_TYPE_BOOLEAN &&
           ctrl.type != V4L2_CTRL_TYPE_BUTTON  &&
           ctrl.type != V4L2_CTRL_TYPE_STRING  &&
           ctrl.type != V4L2_CTRL_TYPE_MENU) {
            ofLogError(LOG_NAME) << name << " type not supported";
            return false;
        }
        c.id = ctrl.id;
        c.value = value;
        if(v4l2_ioctl(fd, VIDIOC_S_CTRL, &c) != 0) {
            ofLogError(LOG_NAME) << "Failed to set control " << name << " error: " << strerror(errno);
            return false;
        }
    } else {

        ofLogError(LOG_NAME) << "Error querying control " << name << strerror(errno);
        return false;
    }

    return true;
}
