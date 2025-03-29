#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Function Declarations
void jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile, int imageIndex);
int imageSubtraction(unsigned char* currentBinary, unsigned char* referenceBinary, int width, int height, int diffThreshold, int* diffValue, unsigned char* diffImage);
unsigned char* convert_to_grayscale(unsigned char* image, int width, int height, int channels);
unsigned char* convert_to_binary(unsigned char* grayImage, int width, int height, int threshold);

int main(){
    const char* imageBaseName = "./frames/frameSet2/frame"; // base names for images
    const char* imageExtension = ".png";    // extension of images
    const char* referenceImageFile = "reference.bin";
    const char* referenceImageUpdateFile = "reference_update.bin";

    int imageIndex = 1;
    char currentImagePath[256];

    while(1){
        snprintf(currentImagePath, sizeof(currentImagePath), "%s%02d%s", imageBaseName, imageIndex, imageExtension);

        // Check if the file exists
        FILE* file = fopen(currentImagePath, "r");
        if(file){
            fclose(file);

            // Run jumpguardDetection
            jumpguardDetection(currentImagePath, referenceImageFile, referenceImageUpdateFile, imageIndex);

            imageIndex++;
        }else{
            // No more images to process
            break;
        }
    }

    // Delete the reference image after processing all images
    if (remove(referenceImageFile) == 0) {
        printf("Reference image %s deleted successfully.\n", referenceImageFile);
    } else {
        printf("Error deleting reference image %s.\n", referenceImageFile);
    }

    return 0;
}

void jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile, int imageIndex){
    // Define Thresholds
    const int threshold = 40; // binary threshold
    const int diffThreshold = 6000; // difference threshold for detection

    // Load current image and convert to greyscale and binary
    int width, height, channels;
    unsigned char* currentImage = stbi_load(currentImageFile, &width, &height, &channels, 0);
    if(!currentImage){
        fprintf(stderr, "Error loading Image %s\n", currentImageFile);
        return;
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
        return;
    }

    // Allocate memory for the difference image
    unsigned char* diffImage = malloc(width * height);

    // Run image subtraction
    int diffValue = 0;
    int detect = imageSubtraction(binaryImage, referenceBinary, width, height, diffThreshold, &diffValue, diffImage);
    printf("Subtracting from reference image %s\n", referenceImageFile);
    printf("Detection value for image %s: %d\n", currentImageFile, detect);
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
}

int imageSubtraction(unsigned char* currentBinary, unsigned char* referenceBinary, int width, int height, int diffThreshold, int* diffValue, unsigned char* diffImage){
    *diffValue = 0;
    for (int i = 0; i < width * height; i++){
        int diff = abs(currentBinary[i] - referenceBinary[i]);
        *diffValue += diff;
        diffImage[i] = (unsigned char)diff;
    }
    return *diffValue > diffThreshold;
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
        binaryImage[i] = grayImage[i] > threshold ? 1 : 0;
    }
    return binaryImage;
}
