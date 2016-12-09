/*
 * ofxV4L2Settings.h
 *
 *  Created on: 03/03/2012
 *      Author: arturo
 */

#ifndef OFXV4L2SETTINGS_H_
#define OFXV4L2SETTINGS_H_

#include "ofConstants.h"
#include <map>
#include <linux/videodev2.h>
#include "ofxDatGui.h"
#include "ofxDatGuiSettings.h"

class ofxV4L2Settings {
public:
    ofxV4L2Settings();
    virtual ~ofxV4L2Settings();

    bool setup(string device);
    bool set(string name, int value);
    void save(string filename);
    void save(){save(m_filename);}
    void load(string filename);
    void load(){load(m_filename);}
    string getFilename(){return m_filename;}
    void setFilename(string file){
        m_filename = file;
        return;
    }

/*
    struct Control{
        Control() : parameter(nullptr){
            id = -1;
        }
        Control(int fd, const struct v4l2_queryctrl & ctrl, const struct v4l2_control & c);
        ~Control(){
            if(parameter) delete parameter;
        }

        int id;
        ofxDatGuiComponent *parameter;
        __u32  type;
        int		     step;
        int		     default_value;

        int operator=(const int & value){
            if(id!=-1)
                *parameter = value;
            return *parameter;
        }

        operator int(){
            return *parameter;
        }

    };
*/

    ofxDatGuiComponent* operator[](string name){
        for ( auto p : parameters->children ){
            if ( p->is(name) ) return p;
        }
        ofLogError(LOG_NAME) << "parameter " << name << " doesn't exist returning null pointer";
        return nullptr;
    }

    ofxDatGuiFolder *parameters;
    ofxDatGuiSettings guisettings;

    static string LOG_NAME;

private:
    int fd;
    vector<int> ids;
    void addControl(int fd, const struct v4l2_queryctrl & ctrl, const struct v4l2_control & c);

    void onButtonEvent(ofxDatGuiButtonEvent e);
    void onToggleEvent(ofxDatGuiToggleEvent e);
    void onSliderEvent(ofxDatGuiSliderEvent e);
    void onTextInputEvent(ofxDatGuiTextInputEvent e);
    void onDropdownEvent(ofxDatGuiDropdownEvent e);
    string m_filename;

};

#endif /* OFXV4L2SETTINGS_H_ */
