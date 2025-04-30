#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistd.h>
#include <pthread.h>

int main(){

//kill any previous camera processes
system("pkill -9 libcamera-still"); // Ensure no other libcamera processes are running
system("pkill -9 libcamera-vid"); // Ensure no other libcamera-vid processes are running
system("pkill -9 libcamera-jpeg"); // Ensure no other libcamera-jpeg processes are running


    const int num_cycles = 5;
    const int num_images = 10;
    const int delay_seconds = 10;

    // Ensure the output directory exists
    system("mkdir -p imageTest12");

    for (int cycle = 1; cycle<=num_cycles; ++cycle){
        printf("Cycle %d started.\n", cycle);

        for (int i = 1; i<=num_images; ++i){
            char command[200];
            snprintf(command, sizeof(command), "libcamera-still -o imageTest6/imageSet_%02d.png --encoding png --width 4608 --height 2592 -t 0 --nopreview -n --immediate --quality 10 2>&1", (i + (cycle-1)*10));
            printf("Executing command: %s\n", command);
            int ret = system(command);
            if (ret != 0) {
                fprintf(stderr, "Error capturing image %d of cycle %d. Command output:\n%s\n", i, cycle, command);
                return 1;
            }
            sleep(1);
        }
        printf("Images captured successfully. Captureing Next cycle\n");
        sleep(10);

        /*printf("Program will start capturing video... \n");

        // Step 1: Capture a 10-second video using libcamera-vid
        char command[100];
        snprintf(command, sizeof(command), "libcamera-vid -o test3.h264 --timeout 10000 --nopreview");
        printf("Capturing a 10 second video...\n");
        int ret = system(command);

        if (ret != 0){
            fprintf(stderr, "Error capturing video of cycle %d\n", cycle);
            return 1;
        }*/
        
        // printf("Video captured successfully as 'test%d.h264'.\n", cycle);

        // if(cycle < num_cycles){
        //  printf("Waiting for %d seconds before next cycle...\n", delay_seconds);
        //  sleep(delay_seconds);
        // }

        /*printf("Extracting frames from the captured video...\n");
        char command[100];
        snprintf(command, sizeof(command), "ffmpeg -i test11.h264 -vf fps=3 frames/framesTest11/frame_%%02d.png");
        int ret = system(command);
        if (ret != 0) {
            fprintf(stderr, "Error extracting frames from video of cycle %d\n", cycle);
            return 1;
        }
        printf("Frames extracted successfully from 'test11.h264' to 'frames/framesTest11/frame_%%02d.png'.\n");

        sleep(delay_seconds);*/
    }

    // printf("Program will start capturing video... \n");

    // // Step 1: Capture a 10-second video using libcamera-vid
    // char command[100];
    // snprintf(command, sizeof(command), "libcamera-vid -o test%d.h264 --timeout 100000 --nopreview", cycle + 10);
    // printf("Capturing a 100 second video...\n");
    // int ret = system(command);

    return 0;
}
