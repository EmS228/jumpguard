function jumpguardDetection(currentImage, referenceImage)
    %#codegen
    
    %Define Thresholds
    threshold = 60; % Binary threshold
    diffThreshold = 24000; % Difference threshold for detection
    
    % Convert to grayscale manually 
    grayImage = 0.2989 * currentImage(:,:,1) + 0.5870 * currentImage(:,:,2) + 0.1140 * currentImage(:,:,3);
    
    % Convert to binary using threshold
    binaryImage = grayImage > threshold;
    
    % If reference image is empty, set it as the first image
    if isempty(referenceImage)
        referenceImage = binaryImage;
    end
    
    % Call the image subtraction function
    detect = imageSubtraction(binaryImage, referenceImage, diffThreshold);
    
    % If no detection, update reference image
    if detect == 0
        referenceImage = binaryImage;
    end
        
end


%----------------------- Previous complex code -----------------------

% referenceImage = []; % Initialize empty reference image
% threshold = 60; % Binary threshold
% diffThreshold = 24000; % Difference threshold for detection

% This code will be used for the final process as the current image will be
% overwritten each time a new image is captured
% while true
%     % Acquire current image (replace with actual image acquisition)
%     currentImage = imread('current_image.jpg');
% 
%     % Convert to grayscale
%     grayImage = rgb2gray(currentImage);
% 
%     % Convert to binary using threshold
%     binaryImage = grayImage > threshold;
% 
%     % If reference image is empty, set it as the first image
%     if isempty(referenceImage)
%         referenceImage = binaryImage;
%     end
% 
%     % Call the image subtraction function
%     detect = imageSubtraction(binaryImage, referenceImage, diffThreshold);
% 
%     % Display detection result
%     fprintf('Detection status: %d\n', detect);
% 
%     % If no detection, update reference image
%     if detect == 0
%         referenceImage = binaryImage;
%     end
% 
%     pause(1); % Adjust loop timing as needed
% end

% This code will be used to test the accuracy of the code through preloaded
% images instead of real time processing
% Directory containing the image series
% imageFiles = dir(fullfile(imageDir, '*.png')); % Adjusted for PNG files
% imageFiles = dir('frames/set5/*.png'); % Update with the correct folder path
% numImages = length(imageFiles);
% 
% for i = 1:length(imageFiles)
%     % Read the current image
%     currentImage = imread(fullfile('frames/set5', imageFiles(i).name));
% 
%     % Convert to grayscale
%     grayImage = rgb2gray(currentImage);
% 
%     % Convert to binary using threshold
%     binaryImage = grayImage > threshold;
% 
%     % If reference image is empty, set it as the first image
%     if isempty(referenceImage)
%         referenceImage = binaryImage;
%         continue;
%     end
% 
%     % Call the image subtraction function
%     [detect, diffImage, diffValue] = imageSubtraction(binaryImage, referenceImage, diffThreshold);
% 
%     % Display detection result and difference
%     fprintf('Image: %s - Detection status: %d - Difference value: %f\n', imageFiles(i).name, detect, diffValue);
%     imshow(diffImage); title(sprintf('Difference Image: %s', imageFiles(i).name));
% 
%     % If no detection, update reference image
%     if detect == 0
%         referenceImage = binaryImage;
%     end
% 
%     pause(1); % Adjust loop timing as needed
% end