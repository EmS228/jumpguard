#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//Function declarations
int startup(void);
void capture_image(const char* filename);
int jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile, unsigned char* binaryImage, unsigned char* referenceBinary, unsigned char* diffImage, int diffThreshold);
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

    // printf("Memory freed. Exiting program.\n");
    exit(0);
}

int main(){
    const char* currentImageFile = "/home/jumpguard/Desktop/jumpguard/software/current.png";
    const char* referenceImageFile = "/home/jumpguard/Desktop/jumpguard/software/reference.bin";
    const char* referenceImageUpdateFile = "/home/jumpguard/Desktop/jumpguard/software/reference_update.bin";
    const char* pythonScript = "/home/jumpguard/Desktop/jumpguard/software/LoRa_detect.py";
    const char* logFilePath = "/home/jumpguard/Desktop/jumpguard/software/diff_log.txt";

    // Register signal handler
    signal(SIGINT, cleanup);

    // Log reboot timestamp
    FILE* logFile = fopen(logFilePath, "a");
    if (logFile) {
        time_t now = time(NULL);
        char* timestamp = ctime(&now);
        timestamp[strcspn(timestamp, "\n")] = 0; // Remove newline character from timestamp
        fprintf(logFile, "\n===== Program Rebooted at %s =====\n", timestamp);
        fclose(logFile);
    } else {
        fprintf(stderr, "Error opening log file for writing.\n");
    }

    // Kill any previous camera processes
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

    int diffThreshold = startup();  // Takes a couple of images and then sets the threshold value
    // int diffThreshold = 261354772;  // Set the diff threshold; this is an average of 10 tests

    while(1){
        clock_t t;
        t = clock();

        // Capture image
        capture_image(currentImageFile);

        // Run jumpguardDetection 
        int detect = jumpguardDetection(currentImageFile, referenceImageFile, referenceImageUpdateFile, binaryImage, referenceBinary, diffImage, diffThreshold);

        // Print Detect Value
        printf("Detect: %d\n", detect);

        // Run Python Script
        run_python_script(pythonScript, detect);
    }

    return 0;
}


void capture_image(const char* filename){
    char command[256];
    snprintf(command, sizeof(command), "libcamera-still -o %s --encoding png --width 4608 --height 2592 -t 0 --nopreview -n --immediate --quality 10", filename); //--timeout 500
    system(command);
}

int jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile, unsigned char* binaryImage, unsigned char* referenceBinary, unsigned char* diffImage, int diffThreshold){
    // Define Thresholds
    const int threshold = 50; // binary threshold

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
    } else {
        // If there is no reference image, set the current image as the reference image
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

    // Log the diffValue with a timestamp to a file
    FILE* logFile = fopen("/home/jumpguard/Desktop/jumpguard/software/diff_log.txt", "a");
    if (logFile) {
        time_t now = time(NULL);
        char* timestamp = ctime(&now);
        timestamp[strcspn(timestamp, "\n")] = 0; // Remove newline character from timestamp
        fprintf(logFile, "%s - Difference value: %d\n", timestamp, diffValue);
        fclose(logFile);
    } else {
        fprintf(stderr, "Error opening log file for writing.\n");
    }

    // Always update the reference image with the current image
    FILE* refUpdateFile = fopen(referenceImageUpdateFile, "wb");
    if (refUpdateFile) {
        fwrite(binaryImage, 1, width * height, refUpdateFile);
        fclose(refUpdateFile);
        rename(referenceImageUpdateFile, referenceImageFile);
        printf("Reference image updated with the current image.\n");
    } else {
        fprintf(stderr, "Error updating reference image.\n");
    }

    return detect;
}

int imageSubtraction(unsigned char* currentBinary, unsigned char* referenceBinary, int width, int height, int diffThreshold, int* diffValue){
    *diffValue = 0;
    // #pragma omp parallel for reduction(+:*diffValue) // OpenMP parallelization for performance improvement
    for (int i = 0; i < width * height; i++){
        int diff = abs(currentBinary[i] - referenceBinary[i]);
        *diffValue += diff;
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

int startup() {
    printf("Starting up JumpGuard system...\n");

    const char* currentImageFile = "/home/jumpguard/Desktop/jumpguard/software/reference/startup.png";
    const char* referenceImageFile = "/home/jumpguard/Desktop/jumpguard/software/reference/reference.bin";
    const char* pythonScript = "/home/jumpguard/Desktop/jumpguard/software/LoRa_detect.py";

    int width = 4608;
    int height = 2592;
    int diffValues[10] = {0};

    unsigned char* image1 = malloc(width * height);
    unsigned char* image2 = malloc(width * height);
    unsigned char* grayImage1 = NULL;
    unsigned char* grayImage2 = NULL;
    unsigned char* binaryImage1 = NULL;
    unsigned char* binaryImage2 = NULL;

    // Capture the first image
    capture_image(currentImageFile);
    unsigned char* currentImage = stbi_load(currentImageFile, &width, &height, NULL, 0);
    if (!currentImage) {
        fprintf(stderr, "Error loading Image %s\n", currentImageFile);
        free(image1);
        free(image2);
        return -1;
    }
    memcpy(image1, currentImage, width * height);
    // stbi_image_free(currentImage);
    run_python_script(pythonScript, 1); //Notify the other system that the first image is taken

    for (int i = 0; i < 10; i++) {
        capture_image(currentImageFile);
        usleep(300000); // 0.3 sec delay between captures

        currentImage = stbi_load(currentImageFile, &width, &height, NULL, 3);
        if (!currentImage) {
            fprintf(stderr, "Error loading Image %s (iteration %d)\n", currentImageFile, i);
            free(image1);
            free(image2);
            return -1;
        }
        memcpy(image2, currentImage, width * height);
        stbi_image_free(currentImage);

        grayImage1 = convert_to_grayscale(image1, width, height, 3);
        grayImage2 = convert_to_grayscale(image2, width, height, 3);

        binaryImage1 = convert_to_binary(grayImage1, width, height, 128);
        binaryImage2 = convert_to_binary(grayImage2, width, height, 128);

        // Calculate pixel-wise binary difference
        for (int j = 0; j < width * height; j++) {
            diffValues[i] += abs(binaryImage1[j] - binaryImage2[j]);
        }

        // Prep for next round
        memcpy(image1, image2, width * height);

        // Free temporary images
        free(grayImage1);
        free(grayImage2);
        free(binaryImage1);
        free(binaryImage2);
    }

    free(image1);
    free(image2);
    run_python_script(pythonScript, 0); // Notify system: calibration done

    // Print and calculate threshold
    printf("Difference values for the 10 images:\n");
    for (int i = 0; i < 10; i++) {
        printf("  Diff[%d] = %d\n", i, diffValues[i]);
    }

    int sum = 0;
    for (int i = 0; i < 10; i++) sum += diffValues[i];
    int avgDiff = sum / 10;
    int threshold = avgDiff + (avgDiff * 0.05);

    printf("Average difference: %d\n", avgDiff);
    printf("Threshold set to: %d\n", threshold);
    printf("Startup complete.\n");

    return threshold;
}
