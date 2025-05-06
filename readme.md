C++

    windows:

        Download equis.cpp from https://github.com/biohack5079/equis/blob/main/equis/equis.cpp, install MinGW, and run the following command in Command Prompt: 

        g++ -o equis equis.cpp -lwinmm -lgdiplus -mwindows ; .\equis

    linux:

        sudo apt update
        sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
        
        pkg-config --cflags --libs sdl2 SDL2_image SDL2_mixer SDL2_ttf
        ↓   
        g++ -g -o equis_linux equis_linux.cpp `pkg-config --cflags --libs sdl2 SDL2_image SDL2_mixer SDL2_ttf`
        ↓
        g++ -g -o equis_linux equis_linux.cpp -I/usr/include/SDL2 -I/usr/include/libpng16 -I/usr/include/x86_64-linux-gnu -I/usr/include/webp -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/opus -I/usr/include/pipewire-0.3 -I/usr/include/spa-0.2 -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -I/usr/include/libinstpatch-2 -pthread -D_REENTRANT -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -D_REENTRANT -I/usr/include/harfbuzz -I/usr/include/freetype2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lSDL2 

        ./equis_linux


Python

    windows:

        py equis.py

    linux:

        python3 equis.py
