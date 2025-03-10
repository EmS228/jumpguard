#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Function Declarations
void jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile, int imageIndex);
int imageSubtraction(unsigned char* currentBinary, unsigned char* referenceBinary, int width, int height, int diffThreshold, int* diffValue, unsigned char* diffImage);
unsigned char* convert_to_grayscale(unsigned char* image, int width, int height, int channels);
unsigned char* convert_to_binary(unsigned char* grayImage, int width, int height, int threshold);

int main(){
    const char* imageBaseName = "./imageTest/imageSet_"; // base names for images
    const char* imageExtension = ".jpg";    // extension of images
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

    //Delete the reference image after processing all images
    if (remove(referenceImageFile) == 0) {
        printf("Reference image %s deleted successfully.\n", referenceImageFile);
    } else {
        printf("Error deleting reference image %s.\n", referenceImageFile);
    }

    // // Test grayscale and binary conversion
    // const char* testImageFile1 = "./testImage1.jpg";
    // const char* testImageFile2 = "./testImage2.jpg";
    // int width1, height1, channels1;
    // int width2, height2, channels2;

    // // Load first test image
    // unsigned char* testImage1 = stbi_load(testImageFile1, &width1, &height1, &channels1, 0);
    // if(!testImage1){
    //     fprintf(stderr, "Error loading Image %s\n", testImageFile1);
    //     return 1;
    // }

    // // Check if the image is a PNG, if not convert it to PNG
    // if (strcmp(testImageFile1 + strlen(testImageFile1) - 4, ".png") != 0) {
    //     stbi_write_png("./testImage1_converted.png", width1, height1, channels1, testImage1, width1 * channels1);
    //     printf("Converted %s to PNG format.\n", testImageFile1);
    // }

    // unsigned char* grayImage1 = convert_to_grayscale(testImage1, width1, height1, channels1);
    // stbi_image_free(testImage1);

    // // Load second test image
    // unsigned char* testImage2 = stbi_load(testImageFile2, &width2, &height2, &channels2, 0);
    // if(!testImage2){
    //     fprintf(stderr, "Error loading Image %s\n", testImageFile2);
    //     free(grayImage1);
    //     return 1;
    // }

    // // Check if the image is a PNG, if not convert it to PNG
    // if (strcmp(testImageFile2 + strlen(testImageFile2) - 4, ".png") != 0) {
    //     stbi_write_png("./testImage2_converted.png", width2, height2, channels2, testImage2, width2 * channels2);
    //     printf("Converted %s to PNG format.\n", testImageFile2);
    // }

    // unsigned char* grayImage2 = convert_to_grayscale(testImage2, width2, height2, channels2);
    // stbi_image_free(testImage2);

    // // Ensure both images have the same dimensions
    // if (width1 != width2 || height1 != height2) {
    //     fprintf(stderr, "Error: Images have different dimensions.\n");
    //     free(grayImage1);
    //     free(grayImage2);
    //     return 1;
    // }

    // // Save grayscale images
    // stbi_write_jpg("./grayImage1.jpg", width1, height1, 1, grayImage1, 100);
    // stbi_write_jpg("./grayImage2.jpg", width2, height2, 1, grayImage2, 100);

    // // Allocate memory for the difference image
    // unsigned char* diffImage = malloc(width1 * height1);

    // // Run image subtraction
    // int diffValue = 0;
    // for (int i = 0; i < width1 * height1; i++) {
    //     diffImage[i] = abs(grayImage1[i] - grayImage2[i]);
    //     diffValue += diffImage[i];
    // }

    // // Save the difference image
    // stbi_write_jpg("./diffImage.jpg", width1, height1, 1, diffImage, 100);

    // // Print the last 100 grayscale pixel values for verification
    // for(int i = width1 * height1 - 100; i < width1 * height1; i++){
    //     printf("Pixel %d: Grayscale value 1 = %d, Grayscale value 2 = %d\n", i, grayImage1[i], grayImage2[i]);
    // }

    // // Print the last 100 difference pixel values for verification
    // for(int i = width1 * height1 - 100; i < width1 * height1; i++){
    //     printf("Pixel %d: Difference value = %d\n", i, diffImage[i]);
    // }

    free(grayImage1);
    free(grayImage2);
    free(diffImage);

    return 0;
}

void jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile, int imageIndex){
    // Define Thresholds
    const int threshold = 40; // binary threshold
    const int diffThreshold = 90000; // difference threshold for detection

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