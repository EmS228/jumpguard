% Define input and output folders
inputFolder = 'frames/set1';
% outputFolder = 'binImg';
outputFolder = 'greyscaleImg';

% Create the output folder if it doesn't exist
if ~exist(outputFolder, 'dir')
    mkdir(outputFolder);
end

% Get a list of all image files in the input folder
imageFiles = dir(fullfile(inputFolder, '*.*')); % Adjust filter if needed, e.g., '*.jpg' or '*.png'
validExtensions = {'.jpg', '.jpeg', '.png', '.bmp', '.tiff'}; % Valid image extensions

% Loop through each file in the folder
for i = 1:length(imageFiles)
    [~, ~, ext] = fileparts(imageFiles(i).name);
    
    % Check if the file has a valid image extension
    if ismember(lower(ext), validExtensions)
        % Read the image
        inputFilePath = fullfile(inputFolder, imageFiles(i).name);
        img = imread(inputFilePath);
        
        % Convert the image to grayscale
        grayImg = rgb2gray(img);
        
        % Save the grayscale image in the output folder
        [~, name, ~] = fileparts(imageFiles(i).name);
        outputFilePath = fullfile(outputFolder, strcat(name, '_gray2', ext));
        imwrite(grayImg, outputFilePath);

        % % Convert the grayscale image to binary
        % binaryImg = imbinarize(grayImg);
        % 
        % % Save the binary image in the output folder
        % [~, name, ~] = fileparts(imageFiles(i).name);
        % outputFilePath = fullfile(outputFolder, strcat(name, '_binary2', ext));
        % imwrite(binaryImg, outputFilePath);
        
        fprintf('Processed and saved: %s\n', outputFilePath);
    end
end

fprintf('All images have been processed and saved in the output folder.\n');
