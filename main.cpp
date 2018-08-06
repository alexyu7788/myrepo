#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include "mainwindow.h"

int main(int argc, char * argv[])
{
    int ch, w=0, h=0;
    bool has_dimension = false;
    std::string yuv_folder;
    std::string window_title = "Crop YUV";
    CMainWindow *mw = NULL;

    while ((ch = getopt(argc, argv, "s:d:o:")) != -1)
    {
        switch (ch)
        {
            case 's':
            yuv_folder = optarg;
            break;
            case 'd':
            sscanf(optarg, "%d:%d", &w, &h);
            has_dimension = true;
            break;
            default:
            break;
        }
    }

    sscanf(yuv_folder.c_str(), "yuv_%d_%d", &w, &h);
    
    if (has_dimension == false && w == 0 && h == 0)
    {
        printf("Need dimension.\n");
        exit(-1);
    }
    // printf("%s=>%d:%d\n", yuv_folder.c_str(), w, h);

    mw = new CMainWindow(window_title, yuv_folder, w, h);

    if (mw && mw->Init() == true)
        mw->ProcessEvent();
    else {
        printf("Can not create Main Window.\n");
        return -1;
    }

    delete mw;
    mw = NULL;

    return 0;
}
