#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//Function declarations
void capture_image(const char* filename);
void jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile);
int imageSubtraction(unsigned char* currentBinary, unsigned char* referenceBinary, int width, int height, int diffThreshold);
void run_python_script(const char* scriptName);
unsigned char* convert_to_grayscale(unsigned char* image, int width, int height, int channels);
unsigned char* convert_to_binary(unsigned char* grayImage, int width, int height, int threshold);

int main(){
    const char* currentImageFile = "current.jpg";
    const char* referenceImageFile = "reference.bin";
    const char* referenceImageUpdateFile = "reference_update.bin";
    const char* pythonScript = "LoRa.py";

    while(1){
        //capture image
        capture_image(currentImageFile);

        //Run jumpguardDetection 
        jumpguardDetection(currentImageFile, referenceImageFile, referenceImageUpdateFile);

        //Run Python Script
        run_python_script(pythonScript);

        //Wait 1 second before repeating
        sleep(1);
    }
    return 0;
}

void capture_image(const char* filename){
    char command[256];
    snprintf(command, sizeof(command), "rpicam-still --output %s --nopreview --timeout 100", filename);
    system(command);
}

void jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile){
    //Defing Thresholds
    const int threshold = 60; //binary threshold
    const int diffThreshold = 24000; //difference threshold for detection

    //Load current image and convert to greyscale and binary
    int width, height, channels;
    unsigned char* currentImage = stbi_load(currentImageFile, &width, &height, &channels, 0);
    if(!currentImage){
        fprintf(stderr, "Error loading Image\n");
        return;
    }
    unsigned char* grayImage = convert_to_grayscale(currentImage, width, height, channels);
    unsigned char* binaryImage = convert_to_binary(grayImage, width, height, threshold);
    stbi_image_free(currentImage);
    free(grayImage);

    //Load reference Image
    FILE* refFile = fopen(referenceImageFile, "rb");
    unsigned char* referenceBinary = malloc(width * height);
    if(refFile){
        fread(referenceBinary, 1, width * height, refFile);
        fclose(refFile);
    } else{
        // if there is no reference image then set the current image as the reference image
        FILE* refUpdateFile = fopen(referenceImageUpdateFile, "wb");
        fwrite(binaryImage, 1, width * height, refUpdateFile);
        fclose(refUpdateFile);
        rename(referenceImageUpdateFile, referenceImageFile);
        printf("No reference Image found. Set current Image %s as reference image.\n", currentImageFile);
        free(binaryImage);
        free(referenceBinary);
        return;
    }

    // Run image subtraction
    int detect = imageSubtraction(binaryImage, referenceBinary, width, height, diffThreshold);

    //Update reference image is no detection
    if (detect == 0){
        FILE* refUpdateFile = fopen(referenceImageUpdateFile, "wb");
        fwrite(binaryImage, 1, width * height, refUpdateFile);
        fclose(refUpdateFile);
        rename(referenceImageUpdateFile, referenceImageFile);
    }

    free(binaryImage);
    free(referenceBinary);
}

int imageSubtraction(unsigned char* currentBinary, unsigned char* referenceBinary, int width, int height, int diffThreshold){
    int diffValue = 0;
    for (int i = 0; i < width * height; i++){
        diffValue += abs(currentBinary[i] - referenceBinary[i]);
    }
    return diffValue > diffThreshold;
}

void run_python_script(const char* scriptName){
    char command[256];
    snprintf(command, sizeof(command), "python3 %s", scriptName);
    system(command);
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