function detect = imageSubtraction(currentBinary, referenceBinary, diffThreshold)
    %#codegen

    % Ensure both images are of the same type (double for calculation)
    currentBinary = double(currentBinary);
    referenceBinary = double(referenceBinary);

    % Compute absolute difference
    diffImage = abs(currentBinary - referenceBinary);
    
    % Count number of different pixels
    diffValue = sum(diffImage(:));
    fprintf("Difference: %d", diffValue);
    
    % Compare to threshold
    detect = diffValue > diffThreshold;
end