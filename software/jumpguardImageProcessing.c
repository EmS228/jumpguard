#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//Function declarations
void capture_image(const char* filename);
int jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile);
int imageSubtraction(unsigned char* currentBinary, unsigned char* referenceBinary, int width, int height, int diffThreshold, int* diffValue);
void run_python_script(const char* scriptName, int detect);
unsigned char* convert_to_grayscale(unsigned char* image, int width, int height, int channels);
unsigned char* convert_to_binary(unsigned char* grayImage, int width, int height, int threshold);
char* convertImagesToPng(const char* currentImagePath);

int main(){
    const char* currentImageFile = "current.bmp";
    const char* referenceImageFile = "reference.bin";
    const char* referenceImageUpdateFile = "reference_update.bin";
    const char* pythonScript = "LoRa_detect.py";

    // Delete the old reference binary image if it exists
    if (access(referenceImageFile, F_OK) == 0) {
        remove(referenceImageFile);
    }

    // Delete the old current image if it exists
    if (access(currentImageFile, F_OK) == 0) {
        remove(currentImageFile);
    }

    //Delete the old current image png
    if (access("current.png", F_OK) == 0) {
        remove("current.png");
    }

    while(1){

        // Capture image
        capture_image(currentImageFile);

        // Convert image to png
        char* pngImagePath = convertImagesToPng(currentImageFile);
        if(pngImagePath == NULL){
            fprintf(stderr, "Error converting image to PNG format.\n");
            continue;
        }

        // Run jumpguardDetection 
        int detect = jumpguardDetection(pngImagePath, referenceImageFile, referenceImageUpdateFile);

        // Run Python Script
        //run_python_script(pythonScript, detect);

        // Wait 1 second before repeating
        sleep(3);
    }
    return 0;
}

void capture_image(const char* filename){
    char command[256];
    snprintf(command, sizeof(command), "rpicam-still --output %s --nopreview --timeout 500", filename);
    system(command);
}

int jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile){
    // Define Thresholds
    const int threshold = 40; // binary threshold
    const int diffThreshold = 24000; // difference threshold for detection

    // Load current image and convert to greyscale and binary
    int width, height, channels;
    unsigned char* currentImage = stbi_load(currentImageFile, &width, &height, &channels, 0);
    if(!currentImage){
        fprintf(stderr, "Error loading Image %s\n", currentImageFile);
        return 0;
    }
    unsigned char* grayImage = convert_to_grayscale(currentImage, width, height, channels);
    unsigned char* binaryImage = convert_to_binary(grayImage, width, height, threshold);
    stbi_image_free(currentImage);
    free(grayImage);

    // Load reference Image
    FILE* refFile = fopen(referenceImageFile, "rb");
    unsigned char* referenceBinary = malloc(width * height);
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
        free(referenceBinary);
        return 0;
    }

    // Allocate memory for the difference image
    unsigned char* diffImage = malloc(width * height);

    // Run image subtraction
    int diffValue = 0;
    int detect = imageSubtraction(binaryImage, referenceBinary, width, height, diffThreshold, &diffValue);
    printf("Difference value: %d\n", diffValue);

    // Update reference image if no detection
    if (detect == 0){
        FILE* refUpdateFile = fopen(referenceImageUpdateFile, "wb");
        fwrite(binaryImage, 1, width * height, refUpdateFile);
        fclose(refUpdateFile);
        rename(referenceImageUpdateFile, referenceImageFile);
    }

    free(binaryImage);
    free(referenceBinary);
    free(diffImage);

    return detect;
}

int imageSubtraction(unsigned char* currentBinary, unsigned char* referenceBinary, int width, int height, int diffThreshold, int* diffValue){
    *diffValue = 0;
    for (int i = 0; i < width * height; i++){
        int diff = abs(currentBinary[i] - referenceBinary[i]);
        *diffValue += diff;
    }
    return *diffValue > diffThreshold;
}

void run_python_script(const char* scriptName, int detect){
    char command[256];
    snprintf(command, sizeof(command), "python3 %s %d", scriptName, detect);
    system(command);
    printf("%s", command);
}

unsigned char* convert_to_grayscale(unsigned char* image, int width, int height, int channels){
    unsigned char* grayImage = malloc(width * height);
    for(int i=0; i<width*height;i++){
        grayImage[i] = (unsigned char)(0.2989 * image[i * channels] + 0.5870 * image[i * channels + 1] + 0.1140 * image[i * channels + 2]);
    }
    return grayImage;
}

unsigned char* convert_to_binary(unsigned char* grayImage, int width, int height, int threshold){
    unsigned char* binaryImage = malloc(width * height);
    for(int i=0; i < width * height; i++){
        binaryImage[i] = grayImage[i] > threshold ? 255 : 0;
    }
    return binaryImage;
}

char* convertImagesToPng(const char* currentImagePath) {
    char* pngImagePath = malloc(256);
    char command[512];

    printf("Processing image: %s\n", currentImagePath);

    // Check if the file exists
    FILE* file = fopen(currentImagePath, "r");
    if(file){
        fclose(file);

        // Check if the image is a PNG, if not convert it to PNG
        if (strcmp(currentImagePath + strlen(currentImagePath) - 4, ".png") != 0) {
            snprintf(pngImagePath, 256, "current.png");
            snprintf(command, sizeof(command), "ffmpeg -i %s %s", currentImagePath, pngImagePath);
            printf("Executing command: %s\n", command);
            int result = system(command);
            if(result == 0){
                printf("Converted %s to PNG format.\n", currentImagePath);
                remove(currentImagePath); // Delete the original BMP image
            } else {
                fprintf(stderr, "Error converting %s to PNG format.\n", currentImagePath);
                free(pngImagePath);
                return NULL;
            }
        } else {
            snprintf(pngImagePath, 256, "%s", currentImagePath);
        }
    } else {
        fprintf(stderr, "File %s does not exist.\n", currentImagePath);
        free(pngImagePath);
        return NULL;
    }

    return pngImagePath;
}