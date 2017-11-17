function convert_vcd_to_table
%convert_vcd_to_table Convert a .vcd file exported from a logic analyser to a csv file
%   This was written due to the .csv export from the LWLA1016 logic
%   analyser program not correctly handling sample counts which exceeded
%   32-bits.

    [filename, pathname] = uigetfile ('*.vcd', 'Select analyser VCD export file');
    if isequal (filename,0)
        return
    end
    
    vcd_fid = fopen (fullfile(pathname, filename), 'r');
    if vcd_fid == -1
        error ('Failed to open %s', fullfile(pathname, filename));
    end
    
    timescale = [];
    header_read = false;
    var_ids = {};
    var_names = {};
    table_names = {'time_ns'};
    
    % Read header from .vcd file to determine number of signals
    while ~header_read && ~feof (vcd_fid)
        line = fgetl (vcd_fid);
        tokens = textscan (line, '%s');
        tokens = tokens{1};
        if (length (tokens) == 4) && strcmp (tokens{1}, '$timescale')
            timescale = str2double (tokens{2});
        elseif (length (tokens) == 6) && strcmp (tokens{1}, '$var')
            var_id = tokens{4};
            var_name = tokens{5};
            var_ids{length(var_ids) + 1} = var_id;
            var_names{length(var_names) + 1} = var_name;
            table_names{length(table_names)+1} = var_name;
        elseif (length (tokens) >= 1) && strcmp (tokens{1}, '$enddefinitions')
            header_read = ~isempty(timescale) && ~isempty(var_ids);
        end
    end
    
    if ~header_read
        printf ('Failed to read header from %s\n', fullfile(pathname, filename));
        return
    end

    % Read the text of the .vcd file which contain the times when signals
    % change
    value_change_data = textscan (vcd_fid','%s%s');
    fclose (vcd_fid);
    
    % Count the number of signal changes, and use to size arrays for the
    % complete results.
    num_fields = length(table_names);
    num_signal_changes = length(find (strncmp(value_change_data{1},'#',1)));
    signal_values = zeros ([1 num_fields]);
    all_values = zeros ([num_signal_changes num_fields]);
    
    sample_set = false;
    sample_index = 1;
    for vcd_index = 1:length(value_change_data{1})
        token_a = value_change_data{1}{vcd_index};
        token_b = value_change_data{2}{vcd_index};
        if (token_a(1) == '#')
            if sample_set
                all_values(sample_index,:) = signal_values;
                sample_index = sample_index + 1;
                sample_set = false;
            end
            
            sample_time = sscanf (token_a, '#%lu');
            if ~isempty (sample_time)
                sample_time = sample_time * timescale;
                signal_values(1,1) = sample_time;
                sample_set = true;
            end
        else
            if ~isempty (token_b)
                % Sample number
                id_index = find(strcmp(token_b, var_ids));
                if length(id_index) == 1
                    signal_values(1,id_index+1) = str2double (token_a(2:end));
                end
            else
                % Signal value
                id_index = find(strcmp(token_a(2:end), var_ids));
                if length(id_index) == 1
                    if token_a(1) == '1'
                        signal_value = 1;
                    else
                        signal_value = 0;
                    end
                    signal_values(1,id_index+1) = signal_value;
                end
            end
        end
    end
    
    % Store final signal values
    if (sample_set)
        all_values(sample_index,:) = signal_values;
    end
    
    % Convert to a table
    converted_data = table();
    for col_index = 1:length(table_names)
        converted_data.(table_names{col_index}) = all_values(:,col_index);
    end
    
    % @todo Shift the timestamps by one index, to correct an apparent
    % offset between the timestamps and signal value in the exported VCD
    % file
    converted_data.time_ns(2:end) = converted_data.time_ns(1:end-1);
    converted_data.time_ns(2:end) = converted_data.time_ns(2:end) + timescale;
    
    % Write the table
    [filepath,name,~] = fileparts (fullfile(pathname, filename));
    writetable (converted_data,fullfile (filepath, [name '.csv']));
end

