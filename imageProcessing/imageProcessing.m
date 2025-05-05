% Directory containing the image series
imageDir = ['testWindDiff'];
imageFiles = dir(fullfile(imageDir, '*.png'));
numImages = length(imageFiles);

% Binary threshold value
binaryThreshold = 60;

% Output directories
binaryOutDir = 'binaryOutput';
diffOutDir = 'differenceOutput';

if ~exist(binaryOutDir, 'dir')
    mkdir(binaryOutDir);
end
if ~exist(diffOutDir, 'dir')
    mkdir(diffOutDir);
end

if numImages < 2
    error('At least two images are required for comparison.');
end

% Initialize variables
processedImages = cell(1, numImages);
diffValues = zeros(1, numImages - 1);

% Step 1: Convert to binary and save binary images
for i = 1:numImages
    img = imread(fullfile(imageDir, imageFiles(i).name));
    grayImg = rgb2gray(img);
    binaryImg = grayImg > binaryThreshold;
    binaryImgUint8 = uint8(binaryImg) * 255;

    % Save binary image
    binaryOutPath = fullfile(binaryOutDir, imageFiles(i).name);
    imwrite(binaryImgUint8, binaryOutPath);

    % Store the binary image
    processedImages{i} = binaryImgUint8;
end

% Step 2: Compute differences and save difference images
diffImages = cell(1, numImages - 1);
for i = 1:numImages - 1
    diffImg = abs(processedImages{i} - processedImages{i+1});
    diffValues(i) = sum(diffImg(:));
    diffImages{i} = diffImg;

    % Save difference image as diff_XX_YY.png
    diffFileName = sprintf('diff_%02d_%02d.png', i, i+1);
    diffOutPath = fullfile(diffOutDir, diffFileName);
    imwrite(diffImg, diffOutPath);

    % Print out the difference value
    fprintf('Binary difference between image %d and %d: %f\n', i, i+1, diffValues(i));
end

% Step 3: Compute average of first 10 differences and set threshold
% numToAverage = min(10, length(diffValues));
% avgDiff = mean(diffValues(1:numToAverage));
% threshold = avgDiff * 1.10;
% 
% fprintf('\nAverage of first %d binary image differences: %f\n', numToAverage, avgDiff);
% fprintf('Computed threshold (average + 10%%): %f\n', threshold);

% Step 4: Plot the difference values
figure;
plot(diffValues, '-o');
xlabel('Image Pair Index');
ylabel('Binary Difference Value');
title('Binary Differences Between Consecutive Images');
grid on;
