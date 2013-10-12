//
//  Main.cpp
//  MYGUI
//
//  Created by john on 12-12-24.
//
//

#include "main_osx.h"
#include "Application.h"

//MYGUI_APP(tools::Application)
int main(int argc, char **argv) {
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    mAppDelegate = [[AppDelegate alloc] init];
    [[NSApplication sharedApplication] setDelegate:mAppDelegate];
    int retVal = NSApplicationMain(argc, (const char **) argv);
    [pool release];
    return retVal;
}

static tools::Application app;

void _rander_init()
{
    try {
        app.prepare();
        app.create();
    } catch( MyGUI::Exception& e ) {
        std::cerr << "An exception has occured" << " : " << e.getFullDescription().c_str();
    }
}

bool _rander_one_frame()
{
    if( !app.renderOneFrame() ){
  //      app.destroy();
        return false;
    }else{
        return true;
    }
}