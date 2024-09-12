# Neelu: Taking input parameters for cache_cfg from a file one by one and then writing the required outputs to a csv file.

import csv;
import os;

"""Input file here must contain the following parameters in sequence: Comment (which will be ignored), cache size, block size, associativity, tag size, cache type, input/output bus width, exclusive read ports, exclusive write ports. Figure out search port based on associativity value."""

input_file = open("scripts/cfg_input.txt", "r");

input_file_iterations = 31;
# Assigning statically for now.

curr_iter = 0;

first_row = [];
second_row = [];

# result_row = ["Cache name", "Read ports", "Write ports", "Search ports", "Technology (um)", "Low operating power", "Input/Output bus width (bits)", "Access Time (ns)", "Total leakage power of a bank (mW)", "Data array: Total dynamic read energy/access (nJ)", "Data array: Total dynamic write energy/access (nJ)", "Tag array:  Total dynamic read energy/access (nJ)", "Tag array:  Total dynamic write energy/access (nJ)", "Total dynamic associative search energy per access (nJ)", "Total dynamic read energy per access (nJ)", "Total dynamic write energy per access (nJ)"]

# result_file = open("CACTI_Model_temp.csv", "w");
# result_writer = csv.writer(result_file);

# result_writer.writerow(result_row);


count = 0;
while curr_iter < input_file_iterations:
    # if(temp_iter < 5 or temp_iter > 9):
    #    continue;
    count = count + 1;
    config_file = open("scripts/finfet_setup_cache_config.xml", "r");
    lines = config_file.readlines();
    config_file.close();

    cache_name = input_file.readline().strip("\n");
    print(cache_name);
    first_row.append(cache_name);
    cache_size = input_file.readline().strip("\n");
    print(cache_size);
    block_size = input_file.readline().strip("\n");
    print(block_size);
    associativity = input_file.readline().strip("\n");
    print(associativity);
    tag_size = input_file.readline().strip("\n");
    print(tag_size);
    cache_level = input_file.readline().strip("\n");
    print(cache_level);
    bus_width = input_file.readline().strip("\n");
    print(bus_width);
    exclusive_read_port = input_file.readline().strip("\n");
    print(exclusive_read_port)
    exclusive_write_port = input_file.readline().strip("\n");
    print(exclusive_write_port)

    line_count = 0;

    # curr_result_row = [cache_name]

    # curr_read_ports = 1
    # curr_write_ports = 1

    # technology_node = 0.032

    for line in lines:
        if(line.find("<cache_size>64</cache_size>") != -1):
            lines[line_count] = "<cache_size>" + \
                str(cache_size)+"</cache_size>\n";
        elif(line.find("<block_size>64</block_size>") != -1):
            lines[line_count] = "<block_size>" + \
                str(block_size)+"</block_size>\n";
        elif(line.find("<associativity>0</associativity>") != -1):
            lines[line_count] = "<associativity>" + \
                str(associativity)+"</associativity>\n";
        elif(line.find("<bus_width>0</bus_width>") != -1):
            lines[line_count] = "<bus_width>"+str(bus_width)+"</bus_width>\n";
        elif(line.find("<tag_size>0</tag_size>") != -1):
            lines[line_count] = "<tag_size>"+str(tag_size)+"</tag_size>\n";
        elif(line.find("<cache_level>L4</cache_level>") != -1):
            lines[line_count] = "<cache_level>" + \
                str(cache_level)+"</cache_level>\n";
        elif(line.find("<exclusive_read_port>0</exclusive_read_port>") != -1):
            lines[line_count] = "<exclusive_read_port>" + \
                str(exclusive_read_port)+"</exclusive_read_port>\n";
        elif(line.find("<exclusive_write_port>0</exclusive_write_port>") != -1):
            lines[line_count] = "<exclusive_write_port>" + \
                str(exclusive_write_port)+"</exclusive_write_port>\n";

        """ if(line.find("-size (bytes)") != -1 and line.find("//") == -1):
            # print(line.strip("\n"));
            lines[line_count] = "-size (bytes) "+str(cache_size)+"\n";
        elif(line.find("-block size (bytes)") != -1 and line.find("//") == -1):
            lines[line_count] = "-block size (bytes) "+str(block_size)+"\n";
        elif(line.find("-associativity") != -1 and line.find("//") == -1):
            lines[line_count] = "-associativity "+str(associativity)+"\n";
        elif(line.find("-tag size (b)") != -1 and line.find("//") == -1):
            lines[line_count] = "-tag size (b) "+str(tag_size)+"\n";
        elif(line.find("-Cache level (L2/L3) -") != -1 and line.find("//") == -1):
            lines[line_count] = "-Cache level (L2/L3) - \""+ \
                                               str(cache_type)+"\"\n";
        elif(line.find("-output/input bus width") != -1 and line.find("//") == -1):
            lines[line_count] = "-output/input bus width "+ \
                str(input_output_bus_width)+"\n";
        elif(line.find("-search port 1") != -1 and associativity == "0"):
            lines[line_count] = "-search port 1\n";
        elif(line.find("-exclusive read port 1") != -1):
            if(cache_name.find("L1D") != -1):
                lines[line_count] = "-exclusive read port 4\n"
                curr_read_ports = 4
            elif((cache_name.find("L2C") != -1) or (cache_name.find("LLC") != -1) or (cache_name.find("TLB") != -1)):
                lines[line_count] = "-exclusive read port 2\n"
                curr_read_ports = 2
        elif(line.find("-exclusive write port 1") != -1):
            if(cache_name.find("L1D") != -1):
                lines[line_count] = "-exclusive write port 2\n"
                curr_write_ports = 2
        elif(line.find("-Data array cell type") != -1):
            if(cache_name.find("LLC") != -1):
                lines[line_count] = "-Data array cell type - \"itrs-lop\"\n"
        elif(line.find("-Data array peripheral type") != -1):
            if(cache_name.find("LLC") != -1):
                lines[line_count] = "-Data array peripheral type - \"itrs-lop\"\n"
        elif(line.find("-Tag array cell type") != -1):
            if(cache_name.find("LLC") != -1):
                lines[line_count] = "-Tag array cell type - \"itrs-lop\"\n"
        elif(line.find("-Tag array peripheral type") != -1):
            if(cache_name.find("LLC") != -1):
                lines[line_count] = "-Tag array peripheral type - \"itrs-lop\"\n"
        # elif(line.find("-technology (u) 0.032") != -1):
            # if(cache_name.find("L1I") != -1 or cache_name.find("L1D") != -1 or cache_name.find("L2C") != -1 or cache_name.find("LLC") != -1 or cache_name.find("TLB") != -1):
            # if(associativity != "0"):
                # lines[line_count] = "-technology (u) 0.022\n"
                # technology_node = 0.022 """

        line_count = line_count + 1;

    # R/W ports added or not.
    # curr_result_row.append(curr_read_ports)
    # curr_result_row.append(curr_write_ports)

    # search port.
    # if(associativity == "0"):
    #    curr_result_row.append("1")
    # else:
    #    curr_result_row.append("0")

    # Technology.
    # print("Technology: "+str(technology_node))
    # curr_result_row.append(technology_node)
    # if(cache_name.find("L1I") != -1 or cache_name.find("L1D") != -1 or cache_name.find("L2C") != -1 or cache_name.find("LLC") != -1 or cache_name.find("TLB") != -1):
    #    curr_result_row.append("0.022")
    # else:
    #    curr_result_row.append("0.032")

    # LOP or not.
    # if(cache_name.find("LLC") != -1):
    #    curr_result_row.append("Yes")
    # else:
    #    curr_result_row.append("")

    # I/O bus width.
    # curr_result_row.append(str(input_output_bus_width))

    os.popen('mkdir -p results/'+str(cache_name))

    os.popen("touch results/"+str(cache_name)+"/" +str(cache_name)+"_cache_config.xml")

    config_file = open("results/"+str(cache_name)+"/" +str(cache_name)+"_7nm_finfet_cache_config.xml", "w");

    config_file.writelines(lines);

    config_file.close();

    # Now we have to execute cacti by providing the created config file as input and create a name for the output.

    stream = os.popen("./cacti -infile results/"+str(cache_name)+"/"+str(cache_name)+"_7nm_finfet_cache_config.xml -outfile results/"+str(cache_name)+"/"+str(cache_name)+"_7nm_finfet_cache_config.txt")
    output = stream.read()
    print(output);

    """cacti_output_file = open("scripts/out_file_cacti.txt","r");
    cacti_output_file_lines = cacti_output_file.readlines();
    cacti_output_file.close();
    flag = 0;
    read_flag = 0;
    tag_read_flag = 0;
    write_flag = 0;
    tag_write_flag = 0;

    total_leakage_power_printed = 0;

    for output_line in cacti_output_file_lines:
        # for output_line in output:
        if(output_line.find("Access time (ns):") != -1):
            print(output_line); #.strip("Access time (ns):"));
            second_row.append(output_line.strip("Access time (ns):"));
            curr_result_row.append(output_line.strip("Access time (ns):").strip());
            flag = 1;

        if(output_line.find("Total leakage power of a bank (mW)") != -1):
            print(output_line);
            if(total_leakage_power_printed == 0):
                curr_result_row.append(output_line.strip("Total leakage power of a bank (mW):").strip());
                total_leakage_power_printed = 1

        if(associativity == '0'):
            if(len(curr_result_row) == 10):
                curr_result_row.append("");
                curr_result_row.append("");
                curr_result_row.append("");
                curr_result_row.append("");
                
            if(output_line.find("Total dynamic associative search energy per access (nJ):") != -1):
                print(output_line);
                curr_result_row.append(output_line.strip("Total dynamic associative search energy per access (nJ):").strip())
                tag_read_flag = 1;
                tag_write_flag = 1;

            if(output_line.find("Total dynamic read energy per access (nJ):") != -1):
                print(output_line);
                curr_result_row.append(output_line.strip("Total dynamic read energy per access (nJ):").strip())
                read_flag = 1;

            if(output_line.find("Total dynamic write energy per access (nJ):") != -1):
                print(output_line);
                curr_result_row.append(output_line.strip("Total dynamic write energy per access (nJ):").strip())
                write_flag = 1;

        else:
            if(output_line.find("Data array: Total dynamic read energy/access  (nJ):") != -1):
                curr_result_row.append(output_line.strip("Data array: Total dynamic read energy/access  (nJ):").strip())
                print(output_line);
                read_flag = 1;

            if(output_line.find("Data array: Total dynamic write energy/access  (nJ):") != -1):
                curr_result_row.append(output_line.strip("Data array: Total dynamic write energy/access  (nJ):").strip())
                print(output_line);
                write_flag = 1;

            if(output_line.find("Tag array:  Total dynamic read energy/access (nJ):") != -1):
                curr_result_row.append(output_line.strip("Tag array:  Total dynamic read energy/access (nJ):").strip())
                print(output_line);
                tag_read_flag = 1;

            if(output_line.find("Tag array:  Total dynamic write energy/access (nJ):") != -1):
                curr_result_row.append(output_line.strip("Tag array:  Total dynamic write energy/access (nJ):").strip())
                print(output_line);
                tag_write_flag = 1;

    if(flag == 0 or read_flag == 0 or write_flag == 0 or tag_read_flag == 0 or tag_write_flag == 0):
        print("Cannot read one of the parameters in out_file_cacti.txt");
        exit(0);

    # if(cache_name.find("L1D") != -1):
    #    exit(0);
    result_writer.writerow(curr_result_row) """ 
    
    curr_iter = curr_iter + 1; 

    # if(count == 10):
     #   exit(0);
input_file.close();

# output_file = open("scripts/AccessTimeNumbers.csv","w");
# writer = csv.writer(output_file);
# writer.writerow(first_row);
# writer.writerow(second_row); 
