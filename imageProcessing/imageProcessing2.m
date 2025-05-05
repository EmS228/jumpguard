
% %read image
%     a= imread('testImages/PPDNoPeople-01.jpg');
%     b= imread('testImages/PPDNoPeople-06.jpg');
%     img1 = im2gray(a);
%     img2 = im2gray(b);
%     c=imsubtract(img1,img2);
% 
%     k = sum(sum(c));
% 
%     % differenceImage = abs(double(img1)-double(img2));
%     % imshow(differenceImage, []);
%     % 
%     % absDiffImage=imabsdiff(img1,img2);
% 
%     % subplot(2,2,1),imshow(a),title('Image 1: No Person');
%     % subplot(2,2,2),imshow(b),title('Image 2: Person');
%     % subplot(2,2,3),imshow(c),title('Subtraction');
% 
%     %take the average of all the images with no people 

%set the counters
corrected = 0;
totalDiff = 0;
correct = 0;
incorrect = 0;

% Directory containing the images
imageDir = 'testImages/People'; % Replace with your directory path
% threshold = 146887303; % Define a threshold for "difference"
threshold = 237881131; %threshold for larger file types

% Get a list of all image files in the directory
imageFiles = dir(fullfile(imageDir, '*.jpg')); % Adjust extension as needed
numImages = length(imageFiles);

if numImages < 2
    error('Not enough images in the directory to perform subtraction.');
end

% Read the first image (reference image)
referenceImage = imread('testImages/NoPeople/IMG_1745.JPG');

% Ensure the reference image is grayscale
referenceImage = im2gray(referenceImage);

% Process each subsequent image
for i = 1:numImages
    % Read the current image
    currentImage = imread(fullfile(imageDir, imageFiles(i).name));

    % Ensure the current image is grayscale
    currentImage = im2gray(currentImage);

    % Perform image subtraction and calculate the difference
    diff=imsubtract(referenceImage,currentImage);
    diff = sum(sum(diff));
    totalDiff = totalDiff + diff;
    % diffImage = abs(double(referenceImage) - double(currentImage));
    % diffMetric = mean(diffImage(:)); % Mean pixel difference

    % Determine if the difference exceeds the threshold
    if diff > threshold
        difference = 1;
    else
        difference = 0;
    end

    % Display results
    % fprintf('Comparing %s (reference) and %s:\n', imageFiles(1).name, imageFiles(i).name);
    % fprintf('The difference is: %f\n', diff);
    if difference == 1
        % fprintf('There is something in the image\n');
        correct = correct + 1;
        corrected = corrected + 1;
    elseif difference == 0
        % fprintf('There is not something in the image\n');
        incorrect = incorrect + 1;
    end
    % fprintf('-----------------------------\n');
end
    accuracy = correct/222;
    % fprintf('Accuracy: %.2f\n', accuracy);
    % fprintf('-----------------------------\n');
    avgDiff = totalDiff/222;
    % fprintf('Average Difference: %.2f\n', avgDiff);
    % fprintf('-----------------------------\n');

% ----------------------------- No People Test -----------------------------

%set the counters
NototalDiff = 0;
Nocorrect = 0;
Noincorrect = 0;

% Directory containing the images
imageDirNo = 'testImages/NoPeople'; % Replace with your directory path

% Get a list of all image files in the directory
imageFiles = dir(fullfile(imageDirNo, '*.jpg')); % Adjust extension as needed
numImages = length(imageFiles);

if numImages < 2
    error('Not enough images in the directory to perform subtraction.');
end

% Process each subsequent image
for i = 2:numImages
    % Read the current image
    currentImage = imread(fullfile(imageDirNo, imageFiles(i).name));

    % Ensure the current image is grayscale
    currentImage = im2gray(currentImage);

    % Perform image subtraction and calculate the difference
    diff=imsubtract(referenceImage,currentImage);
    diff = sum(sum(diff));
    NototalDiff = NototalDiff + diff;
    % diffImage = abs(double(referenceImage) - double(currentImage));
    % diffMetric = mean(diffImage(:)); % Mean pixel difference

    % Determine if the difference exceeds the threshold
    if diff > threshold
        Nodifference = 1;
    else
        Nodifference = 0;
    end

    % Display results
    % fprintf('Comparing %s (reference) and %s:\n', imageFiles(1).name, imageFiles(i).name);
    % fprintf('The difference is: %f\n', diff);
    if Nodifference == 1
        % fprintf('There is something in the image\n');
        Noincorrect = Noincorrect + 1;
    elseif Nodifference == 0
        % fprintf('There is not something in the image\n');
        Nocorrect = Nocorrect + 1;
        corrected = corrected + 1;
    end
    % fprintf('-----------------------------\n');
end
    Noaccuracy = Nocorrect/93;
    % fprintf('Accuracy: %.2f\n', Noaccuracy);
    % fprintf('-----------------------------\n');
    NoavgDiff = NototalDiff/93;
    % fprintf('Average Difference: %.2f\n', NoavgDiff);
    % fprintf('-----------------------------\n');

% ----------------------- Compare the Two -----------------------
totalaccuracy = corrected/316;

fprintf('-------------Results----------------\n');
fprintf('No People Files\n');
fprintf('Accuracy: %.2f%%\n', Noaccuracy*100);
fprintf('Average Difference: %.2f\n', NoavgDiff);
fprintf('-----------------------------\n');
fprintf('People Files\n');
fprintf('Accuracy: %.2f%%\n', accuracy*100);
fprintf('Average Difference: %.2f\n', avgDiff);
fprintf('-----------------------------\n');
fprintf('Total Accuracy: %.2f%%\n', totalaccuracy*100);
fprintf('Current Threshold: %.2f\n', threshold);
fprintf('-----------------------------\n');

disp('Processing complete.');