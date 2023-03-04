[file,path] = uigetfile('*.*');
data = transpose(readmatrix(strcat(path,file)));
data(1,:) = [];
data(:,1) = [];
x = data(:,1);
[~,num_of_meas] = size(data);
for i = 2:num_of_meas
    y = data(:,i);
    imdl = mk_common_model('j2c',16);
%     figure; show_fem(imdl.fwd_model);
    options = {'no_meas_current','no_meas_current_next1','rotate_meas'};
    [stim,meas_select] = mk_stim_patterns(16,1,'{ad}','{ad}',options,4.5);
    imdl.fwd_model.stimulation = stim;
    imdl.fwd_model.meas_select = meas_select;
% x = zeros([256 1]); y = zeros([256 1]);
% j = 1;----------------------------------------
% for i = 1:256
%     if meas_select(i)
%         x(i) = data_homg(j);
%         y(i) = data_objs(j);
%         j = j + 1;
%     end
% end
% clear j;
    img = inv_solve(imdl, x, y);
%     img.calc_colours.cb_shrink_move = [0.5,0.8,0.02];
%     img.calc_colours.ref_level = 0;
    
% clf; show_fem(img,[1,1]); axis off; axis image
    date_current_as_string = string(datetime('now','Format','dd-MM-yyyy'));
    mkdir(date_current_as_string); % Create a directory with current date as its name
    cd(date_current_as_string);
    img_name = strcat(string(i-1),'.png');
    figure; show_slices(img);
    print_convert(img_name);
    close;
    cd ..
end