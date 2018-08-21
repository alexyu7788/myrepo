#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <vector>

#include "mainwindow.h"
#include "common.h"

using namespace std;

int main(int argc, char *argv[])
{
    char ch;
    string input_folder;
    CMainWindow *mainwindow = NULL; 

    while ((ch = getopt(argc, argv, "s:")) != -1)
    {
        switch (ch) {
            case 's':
                input_folder = optarg;
                break;
            default:
                break;
        }
    }

    mainwindow = new CMainWindow("basewindow", input_folder, 360, 240);

    if (mainwindow) {
        mainwindow->Init();
        mainwindow->ProcessEvent();
    }

    if (mainwindow) {
        delete mainwindow;
        mainwindow = NULL;
    }





    return 0;
}

