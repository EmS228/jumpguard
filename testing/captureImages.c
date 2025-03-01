#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistd.h>
#include <pthread.h>

int main(){
	const int num_cycles = 20;
	const int num_images = 4;
	const int delay_seconds = 10;

	for (int cycle = 1; cycle<=num_cycles; ++cycle){
		printf("Cycle %d started.\n", cycle);

		/*for (int i = 1; i<=num_images; ++i){
			char command[100];
			snprintf(command, sizeof(command), "rpicam-still -output imageSet_%d_%d.png", cycle, i);
			printf("Capturing image: %s\n", command);
			int ret = system(command);

			if (ret != 0){
				fprintf(stderr, "Error capturing image %d of cycle %d\n", i, cycle);
				return 1;
			}
			sleep(1);
		}*/

		printf("Program will start capturing video... \n");

		// Step 1: Capture a 10-second video using libcamera-vid
		char command[100];
		snprintf(command, sizeof(command), "libcamera-vid -o test%d.h264 --timeout 10000 --nopreview", cycle + 10);
		printf("Capturing a 10 second video...\n");
		int ret = system(command);

		if (ret != 0){
			fprintf(stderr, "Error capturing video of cycle %d\n", cycle);
			return 1;
		}
		
		// printf("Video captured successfully as 'test%d.h264'.\n", cycle);

		// if(cycle < num_cycles){
		// 	printf("Waiting for %d seconds before next cycle...\n", delay_seconds);
		// 	sleep(delay_seconds);
		// }

			// printf("Extracting frames from the video...\n");
			// char command[100];
			// snprintf(command, sizeof(command), "ffmpeg -i test%d.h264 -vf fps=3 frames/frame%d_%02d.png", cycle, cycle);
			// int ret = system(command);
			// if(ret != 0){
			// 	fprintf(stderr, "Error: Failed to extract frames from video.\n");
			// 	return 1;
			// }
			// printf("Frames extracted successfully and saved in the 'frames' directory.\n");

			// sleep(delay_seconds);
	}

	printf("Image capture complete.\n");
	return 0;
}

