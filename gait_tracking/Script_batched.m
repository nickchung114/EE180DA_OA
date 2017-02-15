clear;
close all;
clc;
addpath('Quaternions');
addpath('ximu_matlab_library');

% CONSTANTS
DEBUG = 1;
PLOT = 1;
ANIMATE = 1;
PERSISTENT_DATA = 1;
batchSize = 256;
sampRate = 256;
bufSize = 100; % number of files in ring buffer btwn Python and Matlab
initTot = 2*sampRate+1; % 2 seconds
initSteps = 2000; % 2000 steps for initial AHRS convergence
stationaryThresh = .05; % This shouldn't be too low or the algo won't think a step ever happened
%stationaryThresh = 0.005;

% for visualization
runningPos = zeros(0,3);
runningGyr = zeros(0,3);
runningAcc = zeros(0,3);
runningQuat = zeros(0,4);
runningStationary = zeros(0,1);
runningVel = zeros(0,3);
runningFilt = zeros(0,3);

initCount = 0;
initDone = 0;
danglingStart = 0;
currentFile = 0;    
velSinceStart = zeros(0,3);
initTime = zeros(initTot,1);
initGyrX = zeros(initTot,1);
initGyrY = zeros(initTot,1);
initGyrZ = zeros(initTot,1);
initAccX = zeros(initTot,1);
initAccY = zeros(initTot,1);
initAccZ = zeros(initTot,1);

currentPosition = zeros(1,3);
currentVelocity = zeros(1,3);

% -------------------------------------------------------------------------
% Select dataset (comment in/out)

%filePath = 'split_files_256/straightLine';
%filePath = 'split_files_1024/straightLine';
%filePath = 'split_files_4096/straightLine';
%filePath = 'Datasets/straightLine';

filePath = 'data_2017-02-09_17-52-11/data';

%filePath = 'Datasets/stairsAndCorridor';

%filePath = 'Datasets/spiralStairs';

% HPF Stuff
%filtCutOffHigh = 0.001; % in Hz. Just cut out DC component
filtCutOffHigh = 5; % needs to be higher for df1 to work...?
[bh, ah] = butter(1, filtCutOffHigh/(sampRate/2), 'high');
[bh2, ah2] = butter(1, .001/(sampRate/2), 'high');
hdh = dfilt.df1(bh, ah);
hdh.PersistentMemory = true; % remember state between filters
%hdh.states = 0; % initial state set to 0 b/c stationary at first
hdh.states = 1;

% LPF Stuff
filtCutOffLow = 5;
[bl, al] = butter(1, filtCutOffLow/(sampRate/2), 'low');
hdl = dfilt.df1(bl, al);
hdl.PersistentMemory = true;
hdl.states = 0;

AHRSalgorithm = AHRS('SamplePeriod', 1/sampRate, 'Kp', 1, 'KpInit', 1);

%while (~DEBUG || (DEBUG && (currentFile <= maxFile)))
while (1)
    currentFilePath = strcat(filePath, num2str(currentFile,'%.2i'));

    % gather batchSize data points from next file
    fileName = strcat(currentFilePath, '_CalInertialAndMag.csv');
    if (DEBUG)
        if (~(exist(fileName, 'file') == 2))
            break
        end
    else % ~DEBUG
        % infinite loop until next file is available
        while (~(exist(fileName, 'file') == 2)) end
    end
    
    % -------------------------------------------------------------------------
    % Import data

    samplePeriod = 1/sampRate;
    xIMUdata = xIMUdataClass(currentFilePath, 'InertialMagneticSampleRate', sampRate); 
    
    time = xIMUdata.CalInertialAndMagneticData.Time;
    gyrX = xIMUdata.CalInertialAndMagneticData.Gyroscope.X;
    gyrY = xIMUdata.CalInertialAndMagneticData.Gyroscope.Y;
    gyrZ = xIMUdata.CalInertialAndMagneticData.Gyroscope.Z;
    accX = xIMUdata.CalInertialAndMagneticData.Accelerometer.X;
    accY = xIMUdata.CalInertialAndMagneticData.Accelerometer.Y;
    accZ = xIMUdata.CalInertialAndMagneticData.Accelerometer.Z;
    
    clear('xIMUdata');
    
    % *** TODO: IMPLEMENT THIS FILE DELETION ONCE THINGS WORK
    % clear file
    %delete(currentFilePath);
    
    len = length(time);
    % assert len <= batchSize
    if (len > batchSize)
        fprintf('WARNING: the length of this frame is longer that the expected batch size. Consider revising the code that creates the frame files\n');
    end
    
    % Initial convergence
    % This is the calibration step. This should run on for file 00 and 01
    % since each file is a second long.

    % DEPRECATED COMMENT: for now we only do this for 00. if we need 01 also we need do stuff.
    % if init period isn't done in this frame, collect init data
    if (initCount + len < initTot) 
        %if (currentFile == 0) % if file 00
%         for i = 1:length(acc_mag)
%             initCount = initCount + 1;
%             initAccX(initCount) = accX(i);
%             initAccY(initCount) = accY(i);
%             initAccZ(initCount) = accZ(i);
%             if (initCount >= initTot)
%                 break;
%             end
%         end
        if (~initCount)
            initTime(1:len) = time;
        else
            initTime(initCount+1:initCount+len) = initTime(initCount)+time+samplePeriod;
        end
        initGyrX(initCount+1:initCount+len) = gyrX;
        initGyrY(initCount+1:initCount+len) = gyrY;
        initGyrZ(initCount+1:initCount+len) = gyrZ;
        initAccX(initCount+1:initCount+len) = accX;
        initAccY(initCount+1:initCount+len) = accY;
        initAccZ(initCount+1:initCount+len) = accZ;
        initCount = initCount + len;
    elseif (~initDone)
        % if init period is complete this frame, do it
        
        % prepend init arrays to this frame's if not empty
        if (initCount)
            time = [initTime(1:initCount); initTime(initCount)+time+samplePeriod];
            gyrX = [initGyrX(1:initCount); gyrX];
            gyrY = [initGyrY(1:initCount); gyrY];
            gyrZ = [initGyrZ(1:initCount); gyrZ];
            accX = [initAccX(1:initCount); accX];
            accY = [initAccY(1:initCount); accY];
            accZ = [initAccZ(1:initCount); accZ];
        end
        
        for i = 1:initSteps % initial convergence
            AHRSalgorithm.UpdateIMU([0 0 0], [mean(accX(1:initTot)) mean(accY(1:initTot)) mean(accZ(1:initTot))]);
        end
        initDone = 1;
    end
    
    % if init is done, do actual analysis
    if (initDone)
        len = length(time); % update len if we prepended stuff
        
        % -------------------------------------------------------------------------
        % Detect stationary periods

        % Compute accelerometer magnitude
        acc_mag = sqrt(accX.*accX + accY.*accY + accZ.*accZ);

        % HP filter accelerometer data
        %acc_magFilt1 = filtfilt(bh2, ah2, acc_mag);
        acc_magFilt = filter(hdh, acc_mag);

        % Compute absolute value
        %acc_magFilt1 = abs(acc_magFilt1);
        acc_magFilt = abs(acc_magFilt);

        % LP filter accelerometer data
        %acc_magFilt1 = filtfilt(bl, al, acc_magFilt1);
        acc_magFilt = filter(hdl, acc_magFilt);

        % Threshold detection
        %stationary1 = acc_magFilt1 < 0.05;
        stationary = acc_magFilt < stationaryThresh;
        %fprintf('Stationary diff for file %i: %i\n', currentFile, sum(abs(stationary-stationary1)));

        % -------------------------------------------------------------------------
        % Compute orientation

        quat = zeros(len, 4);
    
        % For all data
        for t = 1:len
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
        % *** TODO: should we bring this back...?
        % acc = acc - [zeros(length(time), 2) ones(length(time), 1)];     % unnecessary due to velocity integral drift compensation

        % Convert acceleration measurements to m/s/s
        acc = acc * 9.81;

        % Plot translational accelerations
%         figure('Position', [9 39 900 300], 'Number', 'off', 'Name', 'Accelerations');
%         hold on;
%         plot(time, acc(:,1), 'r');
%         plot(time, acc(:,2), 'g');
%         plot(time, acc(:,3), 'b');
%         title('Acceleration');
%         xlabel('Time (s)');
%         ylabel('Acceleration (m/s/s)');
%         legend('X', 'Y', 'Z');
%         hold off;

        % -------------------------------------------------------------------------
        % Compute translational velocities

        % cursory gravity subtraction...?
        % *** TODO: Should we change this?
        acc(:,3) = acc(:,3) - 9.81;

        % Integrate acceleration to yield velocity
        vel = zeros(size(acc));
        vel(1,:) = currentVelocity + acc(1,:) * samplePeriod;
        if (stationary(1) == 1)
            vel(1,:) = [0 0 0];
        end
        for t = 2:length(vel)
            vel(t,:) = vel(t-1,:) + acc(t,:) * samplePeriod;
            if (stationary(t) == 1)
                vel(t,:) = [0 0 0];     % force zero velocity when foot stationary
            end
        end
        currentVelocity = vel(end,:);
    
        % Compute integral drift during non-stationary periods
        velDrift = zeros(size(vel));
        stepStart = find([0; diff(stationary)] == -1);
        stepEnd = find([0; diff(stationary)] == 1);
        if (danglingStart == 1)
            if (stationary(1) == 1) % if starts stationary, assume stepEnd at 1
                stepEnd = [1; stepEnd];
            end
            
            vel = [velSinceStart; vel];
            % resize velDrift, stepStart, and stepEnd
            velDrift = zeros(size(vel));
            stepStart = [1; stepStart + size(velSinceStart,1)];
            stepEnd = stepEnd + size(velSinceStart,1);
            danglingStart = 0;
            velSinceStart = zeros(0,3);
        else
            if (stationary(1) == 0) % if starts moving w/ no danglingStart, assume stepStart at 1
                stepStart = [1; stepStart];
            end
        end  % at this point, vel starts with a stationaryStart
    
        if (size(stepStart,1) > size(stepEnd,1)) % dangling start at end
            danglingStart = 1;
            velSinceStart = vel(stepStart(end):end,:); % vel since last stepStart
            vel = vel(1:(stepStart(end)-1),:); % trim off dangling start
            velDrift = zeros(size(vel));
        end % at this point, vel starts with a stationaryStart and a stationaryEnd/stationary period

        for i = 1:numel(stepEnd) % num of active stepStart's same as stepEnd's
            % assume constant drift accel during step
            driftRate = vel(stepEnd(i)-1, :) / (stepEnd(i) - stepStart(i));
            enum = 1:(stepEnd(i) - stepStart(i));
            drift = [enum'*driftRate(1) enum'*driftRate(2) enum'*driftRate(3)];
            velDrift(stepStart(i):stepEnd(i)-1, :) = drift;
        end

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
        pos = zeros(size(vel));
        if (size(vel,1) > 0)
            pos(1,:) = currentPosition + vel(1,:) * samplePeriod;
            for t = 2:length(pos)
                pos(t,:) = pos(t-1,:) + vel(t,:) * samplePeriod;    % integrate velocity to yield position
            end
            currentPosition = pos(end,:);
        end

        runningVel = [runningVel; vel];
        runningPos = [runningPos; pos];
        runningQuat = [runningQuat; quat];
        runningStationary = [runningStationary; stationary];
        runningGyr = [runningGyr; [gyrX gyrY gyrZ]];
        runningAcc = [runningAcc; [accX accY accZ]];
        runningFilt = [runningFilt; acc_magFilt];
    end
    
    if (~initDone)
        fprintf('(INIT STILL NOT DONE) ');
    end
    
    if (DEBUG)
        fprintf('After File %d: %f %f %f at %i samples\n', currentFile, currentPosition(end,1), currentPosition(end,2), currentPosition(end,3), size(runningPos,1));
    end
    
    currentFile = mod((currentFile + 1), bufSize);
end

if (PLOT)
    if (size(runningPos,1) > 0)
        fprintf('Now plotting...\n');
        
                % -------------------------------------------------------------------------
        % Plot data raw sensor data and stationary periods
    
        figure('Position', [9 39 900 600], 'NumberTitle', 'off', 'Name', 'Sensor Data');
        ax(1) = subplot(2,1,1);
            hold on;
            %plot(time, runningGyr(:,1), 'r');
            %plot(time, runningGyr(:,2), 'g');
            %plot(time, runningGyr(:,3), 'b');
            plot(runningGyr(:,1), 'r');
            plot(runningGyr(:,2), 'g');
            plot(runningGyr(:,3), 'b');
            title('Gyroscope');
            xlabel('Time (s)');
            ylabel('Angular velocity (^\circ/s)');
            legend('X', 'Y', 'Z');
            hold off;
        ax(2) = subplot(2,1,2);
            hold on;
            %plot(time, runningAcc(:,1), 'r');
            %plot(time, runningAcc(:,2), 'g');
            %plot(time, runningAcc(:,3), 'b');
            %plot(time, runningFilt, ':k');
            %plot(time, runningStationary, 'k', 'LineWidth', 2);
            plot(runningAcc(:,1), 'r');
            plot(runningAcc(:,2), 'g');
            plot(runningAcc(:,3), 'b');
            plot(runningFilt, ':k');
            plot(runningStationary, 'k', 'LineWidth', 2);
            title('Accelerometer');
            xlabel('Time (s)');
            ylabel('Acceleration (g)');
            legend('X', 'Y', 'Z', 'Filtered', 'Stationary');
            hold off;
        linkaxes(ax,'x');

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
%         extraTime = 20;
%         onesVector = ones(extraTime*sampRate, 1);
%         posPlot = [posPlot; [posPlot(end, 1)*onesVector, posPlot(end, 2)*onesVector, posPlot(end, 3)*onesVector]];
%         quatPlot = [quatPlot; [quatPlot(end, 1)*onesVector, quatPlot(end, 2)*onesVector, quatPlot(end, 3)*onesVector, quatPlot(end, 4)*onesVector]];

        if (ANIMATE)
            % Create 6 DOF animation
            SamplePlotFreq = 4;
            Spin = 120;
            figure;
            SixDofAnimation(posPlot, quatern2rotMat(quatPlot), ...
                            'SamplePlotFreq', SamplePlotFreq, 'Trail', 'All', ...
                            'Position', [9 39 1280 768], 'View', [(100:(Spin/(length(posPlot)-1)):(100+Spin))', 10*ones(length(posPlot), 1)], ...
                            'AxisLength', 0.1, 'ShowArrowHead', false, ...
                            'Xlabel', 'X (m)', 'Ylabel', 'Y (m)', 'Zlabel', 'Z (m)', 'ShowLegend', false, ...
                            'CreateAVI', false, 'AVIfileNameEnum', false, 'AVIfps', (sampRate / SamplePlotFreq));
        end
    else
        fprintf('Nothing to plot...\n');
    end
end
