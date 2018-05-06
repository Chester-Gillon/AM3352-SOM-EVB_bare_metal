function process_serial_data
%UNTITLED Process serial UART data from a LSA capture
%   This decodes serial UART data looking for a framing error. After a
%   framing error has occurred, attempts to see if trying the decode with
%   one more data bit means the framing error isn't dectected.
%   This is used in analysing why sometimes the UART output from the
%   ethernet_passthrough program showed corrupt characters.

    [filename, pathname] = uigetfile ('*.csv', 'Select decoded LSA data');
    if isequal (filename,0)
        return
    end
    [~, basename, ~] = fileparts (filename);
    
    % Set the parameters of the serial port to decode
    uart_state.baud_rate = 115200;
    uart_state.bit_time_ns = 1E9 / uart_state.baud_rate;
    uart_state.half_bit_time_ns = uart_state.bit_time_ns / 2;
    uart_state.num_data_bits = 8;
    uart_state.num_stop_bits = 1;
    uart_state.sample_index = 1;
    uart_state.framing_error = false;
    
    % Read the LSA data produced by convert_vcd_to_table
    lsa_data = readtable (fullfile(pathname, filename));
    if ~ismember('time_ns', lsa_data.Properties.VariableNames)
        fprintf ('Error: %s doesn''t contain a time_ns field\n', filename);
        return
    end
    if ~ismember('RX', lsa_data.Properties.VariableNames)
        fprintf ('Error: %s doesn''t contain a RX field\n', filename);
        return
    end
    
    % Create a file used to report the decoded text of the serial data
    results_pathname = fullfile (pathname, [basename '.txt']);
    results_fid = fopen (results_pathname, 'wt');
    if results_fid == -1
        fprintf ('Error: failed to create %s\n', results_pathname);
        return
    end
    
    % Process all the serial data, building a list of the time gap between
    % each character
    gap_bit_times = [];
    carriage_return = sprintf ('\r');
    previous_uart_char = struct ([]);
    [uart_char, uart_state] = decode_uart_char (lsa_data, uart_state);
    while ~isempty (uart_char)
        if uart_char.data_byte ~= carriage_return
            fprintf ('%c', char(uart_char.data_byte));
            fprintf (results_fid, '%c', char(uart_char.data_byte));
        end
        if ~uart_char.valid_stop_bit
            error_text = sprintf ('<framing_error at start_bit_time=%0.f used_num_data_bits=%u>', ...
                uart_char.start_bit_time, uart_char.used_num_data_bits);
            fprintf ('%s', error_text);
            fprintf (results_fid, '%s', error_text);
        end
        if ~isempty (previous_uart_char)
            gap_bit_time = (uart_char.start_bit_time - previous_uart_char.start_bit_time) / ...
                uart_state.bit_time_ns;
            if gap_bit_time < uart_state.baud_rate
                gap_bit_times(length(gap_bit_times) + 1) = gap_bit_time;
                previous_uart_char = uart_char;
            else
                % Large time gap means start of next status report
                previous_uart_char = struct ([]);
            end
        else
            previous_uart_char = uart_char;
        end
        
        [uart_char, uart_state] = decode_uart_char (lsa_data, uart_state);
    end
    
    fprintf ('\n');
    fclose (results_fid);
    
    % Display histograms of the time gap between characters
    % Uses a seperate histogram for values within and outside the expected
    % number of bits per character to be able to see more detail on the
    % distribution.
    expected_num_bits = 1 + ... % start bit 
        uart_state.num_data_bits + uart_state.num_stop_bits;
    lower_expected_gap = expected_num_bits - 0.5;
    upper_expected_gap = expected_num_bits + 0.5;
    nbins = 100;
    figure
    overall_figure = gcf;
    subplot (2,1,1)
    histogram (gap_bit_times(and(gap_bit_times >= lower_expected_gap, ...
                                 gap_bit_times <= upper_expected_gap)), nbins);
    subplot (2,1,2)
    histogram (gap_bit_times(or(gap_bit_times < lower_expected_gap, ...
                                gap_bit_times > upper_expected_gap)), nbins);
    title(overall_figure.Children(end), ...
        {'Distribution between characters in bit times for', basename}, ...
        'interpreter', 'none');
    savefig(fullfile(pathname,[basename '.fig']));
end

% Attempt to decode one UART character, and if a framing error occurs see
% if re-trying the decode with an extra data bit then allows the correct
% stop bit to be seen.
function [uart_char, uart_state] = decode_uart_char (lsa_data, uart_state)
    uart_char = struct ([]);
    if isempty (uart_state.sample_index)
        return
    end
    
    % Look for leading edge of a start bit
    while (uart_state.sample_index < length(lsa_data.time_ns)) && ...
            lsa_data.RX(uart_state.sample_index) == 1
        uart_state.sample_index = uart_state.sample_index + 1;
    end
    if uart_state.sample_index > length(lsa_data.time_ns)
        return
    end
    
    % Set expected middle time of the first data bit
    start_bit_time = lsa_data.time_ns(uart_state.sample_index);
    bit_middle_time = start_bit_time + uart_state.bit_time_ns + uart_state.half_bit_time_ns;

    % Attempt to get character with the expected parameters
    [data_byte, valid_stop_bit, sample_index] = get_uart_char (lsa_data, uart_state, bit_middle_time);
    used_num_data_bits = uart_state.num_data_bits;
    if isempty (sample_index)
        return
    end
    
    if ~valid_stop_bit
        % If an invalid stop bit try with by incrementing the number of
        % stop bits.
        modified_uart_state = uart_state;
        modified_uart_state.num_data_bits = uart_state.num_data_bits + 1;
        [modified_data_byte, modified_valid_stop_bit, sample_index] = ...
            get_uart_char (lsa_data, modified_uart_state, bit_middle_time);
        if modified_valid_stop_bit
            data_byte = modified_data_byte;
            used_num_data_bits = modified_uart_state.num_data_bits;
        end
    end
    
    % Set results
    uart_state.sample_index = sample_index;
    uart_state.framing_error = ~valid_stop_bit;
    uart_char(1).data_byte = data_byte;
    uart_char(1).used_num_data_bits = used_num_data_bits;
    uart_char(1).start_bit_time = start_bit_time;
    uart_char(1).valid_stop_bit = valid_stop_bit;
end

% Attempt to decode one UART character given the serial parameters in
% uart_state
function [data_byte, valid_stop_bit, sample_index] = get_uart_char (lsa_data, uart_state, bit_middle_time)
    % Read the data bits of the UART char
    data_byte = 0;
    valid_stop_bit = false;
    for bit = 0:uart_state.num_data_bits-1
        sample_index = find(bit_middle_time >= lsa_data.time_ns, 1, 'last');
        if isempty (sample_index) || (bit_middle_time > lsa_data.time_ns(end))
            sample_index = [];
            return
        end
        if lsa_data.RX(sample_index) == 1
            data_byte = data_byte + (2^bit);
        end
        bit_middle_time = bit_middle_time + uart_state.bit_time_ns;
    end
    
    % Check stop bits
    valid_stop_bit = true;
    for bit = 1:uart_state.num_stop_bits
        sample_index = find(bit_middle_time >= lsa_data.time_ns, 1, 'last');
        if isempty (sample_index) || (bit_middle_time > lsa_data.time_ns(end))
            return
        end
        valid_stop_bit = valid_stop_bit && lsa_data.RX(sample_index) == 1;
        bit_middle_time = bit_middle_time + uart_state.bit_time_ns;
    end
end
