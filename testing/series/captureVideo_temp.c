#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

//Function to read and print the CPU temperature
void *print_temperature(void *arg){
	for(int i = 0; i < 10; i++){
		FILE *temp_file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
		if(temp_file == NULL){
			fprintf(stderr, "Error: Unable to read CPU temperature.\n");
			pthread_exit(NULL);
		}

		int temp_millideg;
		fscanf(temp_file, "%d", &temp_millideg);
		fclose(temp_file);

		float temp_celsius = temp_millideg / 1000.0;
		printf("CPU Temperature: %.2f°C\n", temp_celsius);

		sleep(1); //wait for 1 second
	}
	pthread_exit(NULL);
}

int main(){
	/*//Print message to notify the user 
	printf("Program will start capturing video in 10 seconds... \n");

	// Delay for 10 seconds
	sleep(10);

	// Start the temperature monitoring thread
	pthread_t temp_thread;
	if(pthread_create(&temp_thread, NULL, print_temperature, NULL) != 0){
		fprintf(stderr, "Error: Failed to create temperature monitoring thread.\n");
		return 1;
	}

	// Step 1: Capture a 10-second video using libcamera-vid
	printf("Capturing a 10 second video...\n");
	int video_status = system("libcamera-vid -o initialtest10.h264 --timeout 10000");
	if(video_status != 0){
		fprintf(stderr, "Error: Failed to capture video.\n");
		return 1;
	}
	printf("Video captured successfully as 'initialtest.h264'.\n");

	//Wait for the temperature monitoring thread to complete
	pthread_join(temp_thread, NULL);*/

	// Step 2: Extract frames at 2-3 frames per second using ffmpeg
	printf("Extracting frames from the video...\n");
	int frame_status = system("ffmpeg -i initialtest10.h264 -vf fps=3 frames/frame2_%02d.png");
	if(frame_status != 0){
		fprintf(stderr, "Error: Failed to extract frames from video.\n");
		return 1;
	}
	printf("Frames extracted successfully and saved in the 'frames' directory.\n");

	// Notify the user the program has completed
	printf("Process complete. Check the 'frames' directory for the extracted frames.\n");
	
	return 0;
}
