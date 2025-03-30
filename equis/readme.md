C++

    windows:

        Download equis.cpp from https://github.com/biohack5079/equis/blob/main/equis/equis.cpp, install MinGW, and run the following command in Command Prompt: 

        g++ -o equis equis.cpp -lwinmm -lgdiplus -mwindows ; .\equis

    linux:

        sudo apt update
        sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
        
        g++ -g -o equis_linux equis_linux.cpp `pkg-config --cflags --libs sdl2 SDL2_image SDL2_mixer SDL2_ttf`


        ./equis_linux


Python

    windows:

        py equis.py

    linux:

        python3 equis.py
