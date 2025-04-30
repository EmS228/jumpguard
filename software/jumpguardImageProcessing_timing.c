#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/sysinfo.h>
#include <sys/time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//Function declarations
int startup(void);
void log_temperature(const char* log_path, int is_boot_log);
void capture_image(const char* filename);
int jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile, unsigned char* binaryImage, unsigned char* referenceBinary, unsigned char* diffImage, int diffThreshold, int* outDiffValue);
int imageSubtraction(unsigned char* currentBinary, unsigned char* referenceBinary, int width, int height, int diffThreshold, int* diffValue);
void run_python_script(const char* scriptName, int detect);
unsigned char* convert_to_grayscale(unsigned char* image, int width, int height, int channels);
unsigned char* convert_to_binary(unsigned char* grayImage, int width, int height, int threshold);
float get_cpu_usage();
float get_memory_usage();
void get_timestamp(char* buffer, size_t len);

unsigned char* binaryImage = NULL;
unsigned char* referenceBinary = NULL;
unsigned char* diffImage = NULL;

void cleanup(int signum) {
    if (binaryImage) free(binaryImage);
    if (referenceBinary) free(referenceBinary);
    if (diffImage) free(diffImage);
    exit(0);
}

int main(){
    const char* currentImageFile = "/home/jumpguard/Desktop/jumpguard/software/current.png";
    const char* referenceImageFile = "/home/jumpguard/Desktop/jumpguard/software/reference.bin";
    const char* referenceImageUpdateFile = "/home/jumpguard/Desktop/jumpguard/software/reference_update.bin";
    const char* pythonScript = "/home/jumpguard/Desktop/jumpguard/software/LoRa_detect.py";
    const char* metricsLogPath = "/home/jumpguard/Desktop/jumpguard/software/system_metrics.csv";

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

    FILE* metricsFile = fopen(metricsLogPath, "a+");
    fseek(metricsFile, 0, SEEK_END);
    long size = ftell(metricsFile);
    if (size == 0) {
        fprintf(metricsFile, "TIMESTAMP, CPU TEMP (C), CPU USAGE (%%), MEMORY USAGE (%%), DIFF VALUE, CAPTURE TIME (ms), PROCESS TIME (ms)\n");
    }
    char bootTime[100];
    get_timestamp(bootTime, sizeof(bootTime));
    fprintf(metricsFile, "\n# System rebooted at: %s\n", bootTime);
    fclose(metricsFile);

    log_temperature("/home/jumpguard/Desktop/jumpguard/software/temperature_log.txt", 1);
    int imageCount = 0;

    while(1){
        struct timeval start, captureStart, captureEnd, processEnd;
        gettimeofday(&start, NULL);

        gettimeofday(&captureStart, NULL);
        capture_image(currentImageFile);
        gettimeofday(&captureEnd, NULL);

        int diffValue = 0;
        int detect = jumpguardDetection(currentImageFile, referenceImageFile, referenceImageUpdateFile, binaryImage, referenceBinary, diffImage, diffThreshold, &diffValue);
        gettimeofday(&processEnd, NULL);

        long captureTime = (captureEnd.tv_sec - captureStart.tv_sec) * 1000 + (captureEnd.tv_usec - captureStart.tv_usec)/1000;
        long processTime = (processEnd.tv_sec - captureEnd.tv_sec) * 1000 + (processEnd.tv_usec - captureEnd.tv_usec)/1000;

        float cpuTemp = 0.0;
        FILE* tempFile = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
        if (tempFile != NULL) {
            int temp_millideg;
            fscanf(tempFile, "%d", &temp_millideg);
            fclose(tempFile);
            cpuTemp = temp_millideg / 1000.0;
        }
        float cpuUsage = get_cpu_usage();
        float memUsage = get_memory_usage();

        char timestamp[100];
        get_timestamp(timestamp, sizeof(timestamp));

        metricsFile = fopen(metricsLogPath, "a");
        if (metricsFile) {
            fprintf(metricsFile, "%s,%.2f,%.2f,%.2f,%d,%ld,%ld\n", timestamp, cpuTemp, cpuUsage, memUsage, diffValue, captureTime, processTime);
            fclose(metricsFile);
        } else {
            fprintf(stderr, "Failed to open metrics file.\n");
        }

        printf("Timestamp: %s\n", timestamp);
        printf("CPU Temp: %.2f\n", cpuTemp);
        printf("CPU Usage: %.2f%%\n", cpuUsage);
        printf("Memory Usage: %.2f%%\n", memUsage);
        printf("Diff Value: %d\n", diffValue);
        printf("Capture Time: %ld ms\n", captureTime);
        printf("Process Time: %ld ms\n", processTime);

        run_python_script(pythonScript, detect);

        imageCount++;
        if (imageCount % 10 == 0) {
            log_temperature("/home/jumpguard/Desktop/jumpguard/software/temperature_log.txt", 0);
        }
    }
    return 0;
}


void capture_image(const char* filename){
    char command[256];
    snprintf(command, sizeof(command), "libcamera-still -o %s --encoding png --width 4608 --height 2592 -t 0 --nopreview -n --immediate --quality 10", filename); //--timeout 500
    system(command);
}

int jumpguardDetection(const char* currentImageFile, const char* referenceImageFile, const char* referenceImageUpdateFile, unsigned char* binaryImage, unsigned char* referenceBinary, unsigned char* diffImage, int diffThreshold, int* outDiffValue){
    const int threshold = 50;

    int width, height, channels;
    unsigned char* currentImage = stbi_load(currentImageFile, &width, &height, &channels, 0);
    if(!currentImage){
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
    if(refFile){
        fread(referenceBinary, 1, width * height, refFile);
        fclose(refFile);
    } else{
        FILE* refUpdateFile = fopen(referenceImageUpdateFile, "wb");
        fwrite(binaryImage, 1, width * height, refUpdateFile);
        fclose(refUpdateFile);
        rename(referenceImageUpdateFile, referenceImageFile);
        printf("No reference Image found. Set current Image %s as reference image.\n", currentImageFile);
        return 0;
    }

    int detect = imageSubtraction(binaryImage, referenceBinary, width, height, diffThreshold, outDiffValue);
    printf("\nDifference value: %d\n", *outDiffValue);

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

    // 🔧 NEW: Give the system and camera time to fully initialize
    printf("Waiting for camera to initialize...\n");
    sleep(15);  // Allow system + libcamera to finish startup

    // 🔧 NEW: Warm up camera by capturing and discarding a few images
    printf("Warming up the camera...\n");
    for (int i = 0; i < 5; i++) {
        capture_image(currentImageFile);
        usleep(500000);  // 0.5 sec between captures
    }
    printf("Camera warm-up complete.\n");

    // Capture the first image
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
    run_python_script(pythonScript, 1); // Notify system: first image taken

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


void log_temperature(const char* log_path, int is_boot_log) {
    FILE* tempFile = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (tempFile == NULL) {
        perror("Error opening temperature file");
        return;
    }

    int temp_millideg;
    fscanf(tempFile, "%d", &temp_millideg);
    fclose(tempFile);

    float temp_celsius = temp_millideg / 1000.0;

    FILE* logFile = fopen(log_path, "a");
    if (logFile == NULL) {
        perror("Error opening temperature log file");
        return;
    }

    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char time_str[100];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    if (is_boot_log) {
        fprintf(logFile, "\n--- System restarted at %s ---\n", time_str);
    }

    fprintf(logFile, "%s - CPU Temp: %.2f°C\n", time_str, temp_celsius);
    fclose(logFile);
}


void get_timestamp(char* buffer, size_t len){
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buffer, len, "%Y-%m-%d %H:%M:%S", t);
}

float get_cpu_usage(){
    FILE* file = fopen("/proc/stat", "r");
    if (!file) return -1;

    char buffer[256];
    fgets(buffer, sizeof(buffer), file); //first line
    fclose(file);

    unsigned long user, nice, system, idle;
    sscanf(buffer, "cpu %lu %lu %lu %lu", &user, &nice, &system, &idle);

    static unsigned long last_total = 0, last_idle = 0;
    unsigned long total = user + nice + system + idle;
    unsigned long diff_total = total - last_total;
    unsigned long diff_idle = idle - last_idle;

    last_total = total;
    last_idle = idle;

    if(diff_total == 0) return 0.0;
    return (100.0 * (diff_total - diff_idle)) / diff_total;
}

float get_memory_usage(){
    struct sysinfo memInfo;
    sysinfo(&memInfo);

    long long totalPhysMem = memInfo.totalram;
    totalPhysMem *= memInfo.mem_unit;

    long long physMemUsed = memInfo.totalram - memInfo.freeram;
    physMemUsed *= memInfo.mem_unit;

    return (float)physMemUsed / totalPhysMem * 100.0;
}
