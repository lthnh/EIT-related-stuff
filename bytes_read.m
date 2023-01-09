function bytes_read(src, ~)
    data = read(src, 8, 'string');
    disp(data);
    src.UserData(end+1) = data;
end
