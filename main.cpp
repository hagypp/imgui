#include "main_window.h"
#include <iostream>


//https://www.omdbapi.com/


int main(int, char**) {
    MainWindow window;  // imgui frame

    if (!window.init())     //start the frame
        return 1;
    bool done = false;

    while (!done) 
    {
        if (window.processMessages(done)) 
        {
            window.render();    //function that render the screen
        }
    }

    return 0;
}