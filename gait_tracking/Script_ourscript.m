clear;
close all;
clc;
addpath('Quaternions');
addpath('ximu_matlab_library');
fileExist = 0;
n = 1024;
currentFile = 0;
maxFile = 2;

currentPosition = zeros(1,3);

lastPos = zeros(1,3);

runningPos = zeros(0,3);
runningQuat = zeros(0,4);
% -------------------------------------------------------------------------
% Select dataset (comment in/out)

%filePath = 'split_files/straightLine';
% startTime = 0;
% stopTime = .99;

filePath = 'split_files_4/data';
%filePath = 'Datasets/data';

%filePath = 'split_files_4/stairsAndCorridor';
% startTime = 5;
% stopTime = 53;

% filePath = 'Datasets/spiralStairs';
% startTime = 4;
% stopTime = 47;

danglingStart = 0;
%sampsSinceStart = 0;
%driftSinceStart = [0 0 0];
velSinceStart = zeros(0,3);

AHRSalgorithm = AHRS('SamplePeriod', 1/256, 'Kp', 1, 'KpInit', 1);

while (currentFile <= maxFile)
    currentFilePath = strcat(filePath, num2str(currentFile,'%.2i'));

    %G gather n data points
%     while (fileExist == 0)
%         if (exist(currentFilePath, 'file') == 2)
%             fileExist = 1; 
%         end
%     end
    

    % -------------------------------------------------------------------------
    % Import data

    samplePeriod = 1/256;
    xIMUdata = xIMUdataClass(currentFilePath, 'InertialMagneticSampleRate', 1/samplePeriod);
    %G clear file
    %delete(currentFilePath); 
    
    time = xIMUdata.CalInertialAndMagneticData.Time;
    gyrX = xIMUdata.CalInertialAndMagneticData.Gyroscope.X;
    gyrY = xIMUdata.CalInertialAndMagneticData.Gyroscope.Y;
    gyrZ = xIMUdata.CalInertialAndMagneticData.Gyroscope.Z;
    accX = xIMUdata.CalInertialAndMagneticData.Accelerometer.X;
    accY = xIMUdata.CalInertialAndMagneticData.Accelerometer.Y;
    accZ = xIMUdata.CalInertialAndMagneticData.Accelerometer.Z;
     
    clear('xIMUdata');

    % -------------------------------------------------------------------------
    % Manually frame data

    % startTime = 0;
    % stopTime = 10;

%     indexSel = find(sign(time-startTime)+1, 1) : find(sign(time-stopTime)+1, 1);
%     time = time(indexSel);
%     gyrX = gyrX(indexSel, :);
%     gyrY = gyrY(indexSel, :);
%     gyrZ = gyrZ(indexSel, :);
%     accX = accX(indexSel, :);
%     accY = accY(indexSel, :);
%     accZ = accZ(indexSel, :);

    % -------------------------------------------------------------------------
    % Detect stationary periods

    % Compute accelerometer magnitude
    acc_mag = sqrt(accX.*accX + accY.*accY + accZ.*accZ);

    % HP filter accelerometer data
    filtCutOff = 0.001;
    [b, a] = butter(1, (2*filtCutOff)/(1/samplePeriod), 'high');
    acc_magFilt = filtfilt(b, a, acc_mag);

    % Compute absolute value
    acc_magFilt = abs(acc_magFilt);

    % LP filter accelerometer data
    filtCutOff = 5;
    [b, a] = butter(1, (2*filtCutOff)/(1/samplePeriod), 'low');
    acc_magFilt = filtfilt(b, a, acc_magFilt);

    % Threshold detection
    stationary = acc_magFilt < 0.05;

    % -------------------------------------------------------------------------
    % Plot data raw sensor data and stationary periods
% 
%     figure('Position', [9 39 900 600], 'Number', 'off', 'Name', 'Sensor Data');
%     ax(1) = subplot(2,1,1);
%         hold on;
%         plot(time, gyrX, 'r');
%         plot(time, gyrY, 'g');
%         plot(time, gyrZ, 'b');
%         title('Gyroscope');
%         xlabel('Time (s)');
%         ylabel('Angular velocity (^\circ/s)');
%         legend('X', 'Y', 'Z');
%         hold off;
%     ax(2) = subplot(2,1,2);
%         hold on;
%         plot(time, accX, 'r');
%         plot(time, accY, 'g');
%         plot(time, accZ, 'b');
%         plot(time, acc_magFilt, ':k');
%         plot(time, stationary, 'k', 'LineWidth', 2);
%         title('Accelerometer');
%         xlabel('Time (s)');
%         ylabel('Acceleration (g)');
%         legend('X', 'Y', 'Z', 'Filtered', 'Stationary');
%         hold off;
%     linkaxes(ax,'x');

    % -------------------------------------------------------------------------
    % Compute orientation

    quat = zeros(length(time), 4);

    % Initial convergence
    % This is the calibration step. This should run on for file 00 and 01
    % since each file is a second long.
    
    % initPeriod = 2;
%     indexSel = 1 : find(sign(time-(time(1)+initPeriod))+1, 1);
%     for i = 1:2000
%         AHRSalgorithm.UpdateIMU([0 0 0], [mean(accX(indexSel)) mean(accY(indexSel)) mean(accZ(indexSel))]);
%     end

    % for now we only do this for 00. if we need 01 also we need do stuff.
    if (currentFile == 0) % if file 00
        for i = 1:2000 % initial convergence
            AHRSalgorithm.UpdateIMU([0 0 0], [mean(accX) mean(accY) mean(accZ)]);
        end
    end
    
    % For all data
    for t = 1:length(time)
        if(stationary(t))
            AHRSalgorithm.Kp = 0.5;
        else
            AHRSalgorithm.Kp = 0;
        end
        AHRSalgorithm.UpdateIMU(deg2rad([gyrX(t) gyrY(t) gyrZ(t)]), [accX(t) accY(t) accZ(t)]);
        quat(t,:) = AHRSalgorithm.Quaternion;
    end

    % -------------------------------------------------------------------------
    % Compute translational accelerations

    % Rotate body accelerations to Earth frame
    acc = quaternRotate([accX accY accZ], quaternConj(quat));

    % % Remove gravity from measurements
    % acc = acc - [zeros(length(time), 2) ones(length(time), 1)];     % unnecessary due to velocity integral drift compensation

    % Convert acceleration measurements to m/s/s
    acc = acc * 9.81;

    % Plot translational accelerations
%     figure('Position', [9 39 900 300], 'Number', 'off', 'Name', 'Accelerations');
%     hold on;
%     plot(time, acc(:,1), 'r');
%     plot(time, acc(:,2), 'g');
%     plot(time, acc(:,3), 'b');
%     title('Acceleration');
%     xlabel('Time (s)');
%     ylabel('Acceleration (m/s/s)');
%     legend('X', 'Y', 'Z');
%     hold off;

    % -------------------------------------------------------------------------
    % Compute translational velocities

    acc(:,3) = acc(:,3) - 9.81;
    
    % Integrate acceleration to yield velocity
    vel = zeros(size(acc));
    for t = 2:length(vel)
        vel(t,:) = vel(t-1,:) + acc(t,:) * samplePeriod;
        if(stationary(t) == 1)
            vel(t,:) = [0 0 0];     % force zero velocity when foot stationary
        end
    end
    
    % Compute integral drift during non-stationary periods
    velDrift = zeros(size(vel));
    stepStart = find([0; diff(stationary)] == -1);
    stepEnd = find([0; diff(stationary)] == 1);
    if (danglingStart == 1)
        if (stationary(1) == 1)
            stepEnd = [1; stepEnd];
        end
        %if (size(stationaryEnd,1) ~= 0) % account for previous step's drift
            
            %movingSamps = stationaryEnd(1)-1 + size(velSinceStart, 1);
            %driftRate = (vel(stationaryEnd(1)-1, :) + velSinceStart(end, :)) / movingSamps;
            %enum = 1:movingSamps;
            %drift = [enum'*driftRate(1) enum'*driftRate(2) enum'*driftRate(3)];
            %velDrift(1:prevSampsSinceStart, :) = drift;
            %stationaryEnd = stationaryEnd(2:end);
            vel = [velSinceStart; vel];
            velDrift = zeros(size(vel));%size(velSinceStart,1)+size(vel,1), 3);
            stepStart = [1; stepStart + size(velSinceStart,1)];
            stepEnd = stepEnd + size(velSinceStart,1);
            danglingStart = 0;
            velSinceStart = zeros(0,3);
        %else % non-stationary the whole time
            %velSinceStart = [velSinceStart; vel];
        %end
    else
        if (stationary(1) == 0)
            stepStart = [1; stepStart];
        end
    end % at this point, vel starts with a stationaryStart
    if (size(stepStart,1) > size(stepEnd,1))
        danglingStart = 1;
        velSinceStart = vel(stepStart(end):end,:); %vel since started moving in this window];
        vel = vel(1:stepStart(end)-1, :);
        velDrift = zeros(size(vel));
    end % at this point, vel starts with a stationaryStart and a stationaryEnd
    %if (size(stationaryStart,1) ~= 0)
        for i = 1:numel(stepEnd)
            driftRate = vel(stepEnd(i)-1, :) / (stepEnd(i) - stepStart(i));
            enum = 1:(stepEnd(i) - stepStart(i));
            drift = [enum'*driftRate(1) enum'*driftRate(2) enum'*driftRate(3)];
            velDrift(stepStart(i):stepEnd(i)-1, :) = drift;
        end
    %end
    % Remove integral drift
    vel = vel - velDrift;

    % Plot translational velocity
%     figure('Position', [9 39 900 300], 'Number', 'off', 'Name', 'Velocity');
%     hold on;
%     plot(time, vel(:,1), 'r');
%     plot(time, vel(:,2), 'g');
%     plot(time, vel(:,3), 'b');
%     title('Velocity');
%     xlabel('Time (s)');
%     ylabel('Velocity (m/s)');
%     legend('X', 'Y', 'Z');
%     hold off;

    % -------------------------------------------------------------------------
    % Compute translational position

    % Integrate velocity to yield position
    
    %if (size(vel,1) == 0)
    %    continue;
    %end
    pos = zeros(size(vel));
    if (size(vel,1) > 0)
        pos(1,:) = lastPos;
        for t = 2:length(pos)
            pos(t,:) = pos(t-1,:) + vel(t,:) * samplePeriod;    % integrate velocity to yield position
        end
        lastPos = pos(end,:);
        currentPosition = pos(end,:);
    end
    
    runningPos = [runningPos; pos];
    runningQuat = [runningQuat; quat];
    
    %if (size(pos,1) > 0)
    %    currentPosition = currentPosition + pos(end,:);
    %end
    
    fprintf('After File %d: %f %f %f\n', currentFile, currentPosition(end,1), currentPosition(end,2), currentPosition(end,3));
    
    currentFile = currentFile + 1;
end

fprintf('Now plotting...\n');

% % Plot translational position
% figure('Position', [9 39 900 600], 'NumberTitle', 'off', 'Name', 'Position');
% hold on;
% plot(time, runningPos(:,1), 'r');
% plot(time, runningPos(:,2), 'g');
% plot(time, runningPos(:,3), 'b');
% title('Position');
% xlabel('Time (s)');
% ylabel('Position (m)');
% legend('X', 'Y', 'Z');
% hold off;

% -------------------------------------------------------------------------
% Plot 3D foot trajectory

% % Remove stationary periods from data to plot
% posPlot = pos(find(~stationary), :);
% quatPlot = quat(find(~stationary), :);
posPlot = runningPos;
quatPlot = runningQuat;

% Extend final sample to delay end of animation
extraTime = 20;
onesVector = ones(extraTime*(1/samplePeriod), 1);
posPlot = [posPlot; [posPlot(end, 1)*onesVector, posPlot(end, 2)*onesVector, posPlot(end, 3)*onesVector]];
quatPlot = [quatPlot; [quatPlot(end, 1)*onesVector, quatPlot(end, 2)*onesVector, quatPlot(end, 3)*onesVector, quatPlot(end, 4)*onesVector]];

% Create 6 DOF animation
SamplePlotFreq = 8;
Spin = 120;
SixDofAnimation(posPlot, quatern2rotMat(quatPlot), ...
                'SamplePlotFreq', SamplePlotFreq, 'Trail', 'All', ...
                'Position', [9 39 1280 768], 'View', [(100:(Spin/(length(posPlot)-1)):(100+Spin))', 10*ones(length(posPlot), 1)], ...
                'AxisLength', 0.1, 'ShowArrowHead', false, ...
                'Xlabel', 'X (m)', 'Ylabel', 'Y (m)', 'Zlabel', 'Z (m)', 'ShowLegend', false, ...
                'CreateAVI', false, 'AVIfileNameEnum', false, 'AVIfps', ((1/samplePeriod) / SamplePlotFreq));