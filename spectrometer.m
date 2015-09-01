%clear everything
clear all;
close all;
clc;

%get working directory
[status0, path] = system('pwd');
if(status0>0)
    %deal with pwd errors if any
    disp('couldnt get the working directory')
    break;
end
%add a trailing slash
path = strcat(path, '/');

%search according to naming of device in udev rules
searchstr = '/dev/Spectrometer*';

%get device path
[status1,devices] = system(['ls ', searchstr]);
%process the device list feedback
if(status1)
    %deal with ls errors if any
    disp('couldnt list the spectrometer devices')
    break;
else
    devices = textscan(devices,'%s');
    [Spectnum, ~] = size(devices{1});
    if(Spectnum==0)
        disp('No spectrometers found in the system');
        break;
    elseif(Spectnum==1)
        disp(strcat(['found one spectrometer: ' devices{1}{1}]));
        device = devices{1}{1};
    elseif(Spectnum>1)
        disp('deal with multiple spectrometers here');
        devnum = input('What spectrometer do you want to use?','s');
        if isempty(devnum)
          device = devices{1}{1}
        else
            device = devices{1}{str2num(devnum)}
        end
       break;
    end
end


%change permissions for the device, will ask for root password in command
%window
system(strrep(strcat(['sudo chmod 777 ',device]),sprintf('\n'),''));

%construct a linux command to get data from the spectrometer
command = strcat([path,'spectroread -d ',device,' -V 8 -i 10']);
%remove any new lines from the single command
command = strrep(command,sprintf('\n'),'');

while(1)
%run the command by calling 'system'
[status2,data] = system(command);

check = size(data);
if (check(2)<40685)
    %break
end
%format the data for ploting
%remove text strings
data2 = str2num(data(151:end));
%convert string to numbers
%get only two columns, wavelength and ampitude
[r,c] =size(data2); 
    if(c==4)
        wavelengths = data2(:,2);
        amplitude = data2(:,4);
        %plot the data cropped for the target wavelength
        plot(wavelengths(10:end),smooth(amplitude(10:end)));
        title(strrep(strcat([device,' prevew :)']),sprintf('\n'),''));
        pause(1);
    end
end
