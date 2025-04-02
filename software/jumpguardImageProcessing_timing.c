#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//Function declarations
void capture_image(const char* filename);
int jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile, unsigned char* binaryImage, unsigned char* referenceBinary, unsigned char* diffImage);
int imageSubtraction(unsigned char* currentBinary, unsigned char* referenceBinary, int width, int height, int diffThreshold, int* diffValue);
void run_python_script(const char* scriptName, int detect);
unsigned char* convert_to_grayscale(unsigned char* image, int width, int height, int channels);
unsigned char* convert_to_binary(unsigned char* grayImage, int width, int height, int threshold);

unsigned char* binaryImage = NULL;
unsigned char* referenceBinary = NULL;
unsigned char* diffImage = NULL;

void cleanup(int signum) {
    // Free allocated memory
    if (binaryImage) free(binaryImage);
    if (referenceBinary) free(referenceBinary);
    if (diffImage) free(diffImage);

    printf("Memory freed. Exiting program.\n");
    exit(0);
}

int main(){
    const char* currentImageFile = "/home/jumpguard/Desktop/jumpguard/software/current.png";
    const char* referenceImageFile = "/home/jumpguard/Desktop/jumpguard/software/reference.bin";
    const char* referenceImageUpdateFile = "/home/jumpguard/Desktop/jumpguard/software/reference_update.bin";
    const char* pythonScript = "/home/jumpguard/Desktop/jumpguard/software/LoRa_detect.py";

    // Register signal handler
    signal(SIGINT, cleanup);

    //kill any previous camera processes
    system("pkill -9 libcamera-still"); // Ensure no other libcamera processes are running
    system("pkill -9 libcamera-vid"); // Ensure no other libcamera-vid processes are running
    system("pkill -9 libcamera-jpeg"); // Ensure no other libcamera-jpeg processes are running

    // Delete the old reference binary image if it exists
    if (access(referenceImageFile, F_OK) == 0) {
        remove(referenceImageFile);
    }

    // Delete the old current image if it exists
    if (access(currentImageFile, F_OK) == 0) {
        remove(currentImageFile);
    }

    // Allocate memory for the images
    binaryImage = malloc(4608 * 2592);
    referenceBinary = malloc(4608 * 2592);
    diffImage = malloc(4608 * 2592);

    while(1){
        clock_t t;
        t = clock();

        // Capture image
        capture_image(currentImageFile);

        // Run jumpguardDetection 
        int detect = jumpguardDetection(currentImageFile, referenceImageFile, referenceImageUpdateFile, binaryImage, referenceBinary, diffImage);

        // Run Python Script
        run_python_script(pythonScript, detect);

        t = clock() - t; // Calculate the time taken for processing
        double time_taken = ((double)t)/CLOCKS_PER_SEC; // Convert to seconds
        printf("\nTime taken for processing: %f seconds\n", time_taken);
        

        // Wait 1 second before repeating
        //sleep(1);
    }

    return 0;
}


void capture_image(const char* filename){
    char command[256];
    snprintf(command, sizeof(command), "libcamera-still -o %s --encoding png --width 4608 --height 2592 -t 0 --nopreview -n --autofocus-mode manual --lens-position 3.2 --immediate --quality 10", filename); //--timeout 500
    system(command);
}
/*
void capture_image(const char* filename) {
    char command[256];
    snprintf(command, sizeof(command), "libcamera-still -o %s --encoding png --width 4608 --height 2592 --nopreview --immediate", filename);
    system(command);
    
}
    */

int jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile, unsigned char* binaryImage, unsigned char* referenceBinary, unsigned char* diffImage){
    // Define Thresholds
    const int threshold = 60; // binary threshold
    const int diffThreshold = 200000; // difference threshold for detection

    // Load current image and convert to greyscale and binary
    int width, height, channels;
    unsigned char* currentImage = stbi_load(currentImageFile, &width, &height, &channels, 0);
    if(!currentImage){
        fprintf(stderr, "Error loading Image %s\n", currentImageFile);
        return 0;
    }
    unsigned char* grayImage = convert_to_grayscale(currentImage, width, height, channels);
    binaryImage = convert_to_binary(grayImage, width, height, threshold); // Use the parameter
    stbi_image_free(currentImage);
    free(grayImage);

    // Load reference Image
    FILE* refFile = fopen(referenceImageFile, "rb");
    if(refFile){
        fread(referenceBinary, 1, width * height, refFile);
        fclose(refFile);
    } else{
        // If there is no reference image then set the current image as the reference image
        FILE* refUpdateFile = fopen(referenceImageUpdateFile, "wb");
        fwrite(binaryImage, 1, width * height, refUpdateFile);
        fclose(refUpdateFile);
        rename(referenceImageUpdateFile, referenceImageFile);
        printf("No reference Image found. Set current Image %s as reference image.\n", currentImageFile);
        free(binaryImage);
        return 0;
    }

    // Run image subtraction
    int diffValue = 0;
    int detect = imageSubtraction(binaryImage, referenceBinary, width, height, diffThreshold, &diffValue);
    printf("\nDifference value: %d\n", diffValue);

    // Update reference image if no detection
    if (detect == 0){
        FILE* refUpdateFile = fopen(referenceImageUpdateFile, "wb");
        fwrite(binaryImage, 1, width * height, refUpdateFile);
        fclose(refUpdateFile);
        rename(referenceImageUpdateFile, referenceImageFile);
    }

    return detect;
}

int imageSubtraction(unsigned char* currentBinary, unsigned char* referenceBinary, int width, int height, int diffThreshold, int* diffValue){
    *diffValue = 0;
    // #pragma omp parallel for reduction(+:*diffValue) // OpenMP parallelization for performance improvement
    for (int i = 0; i < width * height; i++){
        int diff = abs(currentBinary[i] - referenceBinary[i]);
        *diffValue += diff;

        // Break out of the loop if the threshold is exceeded
        if (*diffValue > diffThreshold) {
            // #pragma omp cancel for
            break;
        }
    }
    if(*diffValue > diffThreshold){
        return 1;
    }
    else{
        return 0;
    }
    //return *diffValue > diffThreshold;
}

void run_python_script(const char* scriptName, int detect){
    char command[256];
    snprintf(command, sizeof(command), "python3 %s %d", scriptName, detect);
    system(command);
    printf("%s", command);
    
}

unsigned char* convert_to_grayscale(unsigned char* image, int width, int height, int channels){
    unsigned char* grayImage = malloc(width * height);
    #pragma omp parallel for // OpenMP parallelization for performance improvement
    for(int i=0; i<width*height;i+=4){
        grayImage[i] = (unsigned char)(0.2989 * image[i * channels] + 0.5870 * image[i * channels + 1] + 0.1140 * image[i * channels + 2]);
        grayImage[i + 1] = (unsigned char)(0.2989 * image[(i + 1) * channels] + 0.5870 * image[(i + 1) * channels + 1] + 0.1140 * image[(i + 1) * channels + 2]);
        grayImage[i + 2] = (unsigned char)(0.2989 * image[(i + 2) * channels] + 0.5870 * image[(i + 2) * channels + 1] + 0.1140 * image[(i + 2) * channels + 2]);
        grayImage[i + 3] = (unsigned char)(0.2989 * image[(i + 3) * channels] + 0.5870 * image[(i + 3) * channels + 1] + 0.1140 * image[(i + 3) * channels + 2]);
    }
    return grayImage;
}

unsigned char* convert_to_binary(unsigned char* grayImage, int width, int height, int threshold){
    unsigned char* binaryImage = malloc(width * height);
    for(int i=0; i < width * height; i+=4){
        binaryImage[i] = grayImage[i] > threshold ? 255 : 0;
        binaryImage[i + 1] = grayImage[i + 1] > threshold ? 255 : 0;
        binaryImage[i + 2] = grayImage[i + 2] > threshold ? 255 : 0;
        binaryImage[i + 3] = grayImage[i + 3] > threshold ? 255 : 0;
    }
    return binaryImage;
}
