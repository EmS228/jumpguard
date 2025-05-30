#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Function declarations
int startup(void);
void capture_image(const char* filename);
int jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile, unsigned char* binaryImage, unsigned char* referenceBinary, unsigned char* diffImage, int diffThreshold, int* outDiffValue);
int imageSubtraction(unsigned char* currentBinary, unsigned char* referenceBinary, int width, int height, int diffThreshold, int* diffValue);
void run_python_script(const char* scriptName, int detect);
unsigned char* convert_to_grayscale(unsigned char* image, int width, int height, int channels);
unsigned char* convert_to_binary(unsigned char* grayImage, int width, int height, int threshold);

unsigned char* binaryImage = NULL;
unsigned char* referenceBinary = NULL;
unsigned char* diffImage = NULL;

void cleanup(int signum) {
    if (binaryImage) free(binaryImage);
    if (referenceBinary) free(referenceBinary);
    if (diffImage) free(diffImage);
    exit(0);
}

int main() {
    const char* currentImageFile = "/home/jumpguard/Desktop/jumpguard/software/current.png";
    const char* referenceImageFile = "/home/jumpguard/Desktop/jumpguard/software/reference.bin";
    const char* referenceImageUpdateFile = "/home/jumpguard/Desktop/jumpguard/software/reference_update.bin";
    const char* pythonScript = "/home/jumpguard/Desktop/jumpguard/software/LoRa_detect.py";

    signal(SIGINT, cleanup);

    system("pkill -9 libcamera-still");
    system("pkill -9 libcamera-vid");
    system("pkill -9 libcamera-jpeg");

    if (access(referenceImageFile, F_OK) == 0) {
        remove(referenceImageFile);
    }

    if (access(currentImageFile, F_OK) == 0) {
        remove(currentImageFile);
    }

    binaryImage = malloc(4608 * 2592);
    referenceBinary = malloc(4608 * 2592);
    diffImage = malloc(4608 * 2592);

    sleep(15);
    int diffThreshold = startup();

    while (1) {
        capture_image(currentImageFile);

        int diffValue = 0;
        int detect = jumpguardDetection(currentImageFile, referenceImageFile, referenceImageUpdateFile, binaryImage, referenceBinary, diffImage, diffThreshold, &diffValue);


        printf("Diff Value: %d\n", diffValue);

        run_python_script(pythonScript, detect);
    }

    return 0;
}

void capture_image(const char* filename) {
    char command[256];
    snprintf(command, sizeof(command), "libcamera-still -o %s --encoding png --width 4608 --height 2592 -t 0 --nopreview -n --immediate --quality 10", filename);
    system(command);
}

int jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile, unsigned char* binaryImage, unsigned char* referenceBinary, unsigned char* diffImage, int diffThreshold, int* outDiffValue) {
    const int threshold = 50;

    int width, height, channels;
    unsigned char* currentImage = stbi_load(currentImageFile, &width, &height, &channels, 0);
    if (!currentImage) {
        fprintf(stderr, "Error loading Image %s\n", currentImageFile);
        return 0;
    }
    unsigned char* grayImage = convert_to_grayscale(currentImage, width, height, channels);
    unsigned char* tempBinary = convert_to_binary(grayImage, width, height, threshold);
    memcpy(binaryImage, tempBinary, width * height);
    free(tempBinary);
    stbi_image_free(currentImage);
    free(grayImage);

    FILE* refFile = fopen(referenceImageFile, "rb");
    if (refFile) {
        fread(referenceBinary, 1, width * height, refFile);
        fclose(refFile);
    } else {
        FILE* refUpdateFile = fopen(referenceImageUpdateFile, "wb");
        fwrite(binaryImage, 1, width * height, refUpdateFile);
        fclose(refUpdateFile);
        rename(referenceImageUpdateFile, referenceImageFile);
        printf("No reference Image found. Set current Image %s as reference image.\n", currentImageFile);
        return 0;
    }

    int detect = imageSubtraction(binaryImage, referenceBinary, width, height, diffThreshold, outDiffValue);
    printf("\nDifference value: %d\n", *outDiffValue);

    FILE* refUpdateFile = fopen(referenceImageUpdateFile, "wb");
    if (refUpdateFile) {
        fwrite(binaryImage, 1, width * height, refUpdateFile);
        fclose(refUpdateFile);
        rename(referenceImageUpdateFile, referenceImageFile);
    } else {
        fprintf(stderr, "Error updating reference image.\n");
    }

    return detect;
}

int imageSubtraction(unsigned char* currentBinary, unsigned char* referenceBinary, int width, int height, int diffThreshold, int* diffValue) {
    *diffValue = 0;
    for (int i = 0; i < width * height; i++) {
        int diff = abs(currentBinary[i] - referenceBinary[i]);
        *diffValue += diff;

        if (*diffValue > diffThreshold) {
            break;
        }
    }
    return *diffValue > diffThreshold ? 1 : 0;
}

void run_python_script(const char* scriptName, int detect) {
    char command[256];
    snprintf(command, sizeof(command), "python3 %s %d", scriptName, detect);
    system(command);
}

unsigned char* convert_to_grayscale(unsigned char* image, int width, int height, int channels) {
    unsigned char* grayImage = malloc(width * height);
    for (int i = 0; i < width * height; i++) {
        grayImage[i] = (unsigned char)(0.2989 * image[i * channels] + 0.5870 * image[i * channels + 1] + 0.1140 * image[i * channels + 2]);
    }
    return grayImage;
}

unsigned char* convert_to_binary(unsigned char* grayImage, int width, int height, int threshold) {
    unsigned char* binaryImage = malloc(width * height);
    for (int i = 0; i < width * height; i++) {
        binaryImage[i] = grayImage[i] > threshold ? 255 : 0;
    }
    return binaryImage;
}

int startup() {
    printf("Starting up JumpGuard system...\n");

    const char* currentImageFile = "/home/jumpguard/Desktop/jumpguard/software/reference/startup.png";
    const char* referenceImageFile = "/home/jumpguard/Desktop/jumpguard/software/reference/reference.bin";

    int width = 4608;
    int height = 2592;
    int diffValues[10] = {0};

    unsigned char* image1 = malloc(width * height);
    unsigned char* image2 = malloc(width * height);

    printf("Waiting for camera to initialize...\n");
    sleep(15);

    printf("Warming up the camera...\n");
    for (int i = 0; i < 5; i++) {
        capture_image(currentImageFile);
        usleep(500000);
    }
    printf("Camera warm-up complete.\n");

    capture_image(currentImageFile);
    unsigned char* currentImage = stbi_load(currentImageFile, &width, &height, NULL, 3);
    if (!currentImage) {
        fprintf(stderr, "Error loading Image %s\n", currentImageFile);
        free(image1);
        free(image2);
        return -1;
    }
    memcpy(image1, currentImage, width * height);
    stbi_image_free(currentImage);

    for (int i = 0; i < 10; i++) {
        capture_image(currentImageFile);
        usleep(300000);

        currentImage = stbi_load(currentImageFile, &width, &height, NULL, 3);
        if (!currentImage) {
            fprintf(stderr, "Error loading Image %s (iteration %d)\n", currentImageFile, i);
            free(image1);
            free(image2);
            return -1;
        }
        memcpy(image2, currentImage, width * height);
        stbi_image_free(currentImage);

        unsigned char* grayImage1 = convert_to_grayscale(image1, width, height, 3);
        unsigned char* grayImage2 = convert_to_grayscale(image2, width, height, 3);

        unsigned char* binaryImage1 = convert_to_binary(grayImage1, width, height, 128);
        unsigned char* binaryImage2 = convert_to_binary(grayImage2, width, height, 128);

        for (int j = 0; j < width * height; j++) {
            diffValues[i] += abs(binaryImage1[j] - binaryImage2[j]);
        }

        memcpy(image1, image2, width * height);

        free(grayImage1);
        free(grayImage2);
        free(binaryImage1);
        free(binaryImage2);
    }

    free(image1);
    free(image2);

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
