#! /usr/bin/python
from __future__ import print_function
import pickle
import os.path
from googleapiclient.discovery import build
from google_auth_oauthlib.flow import InstalledAppFlow
from google.auth.transport.requests import Request
import re
import sys
import codecs


#Reset the bits below if you wish to send only data for a single workload suite. eg. for sending only IPC traces results: [1, 0, 0]
send_data = [1, 1, 1]
# server/ipc, spec17, cloud

results_directory = ["../../IPC_SOTA_results_50M/",
                     "../../SPEC2017_SOTA_results_100M/", "../../CLOUDSUITE_SOTA_results_50M/"]

branch_pred = "-hashed_perceptron"

l1i_pref = ["-fnlmma_25KB-", "-no-", "-no-"]

trace_list_files = ["../selected_ipc_trace_list.txt",
                    "../selected_intensive_trace_list.txt", "../selected_cloudsuite_trace_list.txt"]

sheet_name = "ENTER_SHEET_NAME_HERE"


before_pref = "-hashed_perceptron-mana_32KB-"
after_pref = ["-no-no-no-no-lru-lru-lru-srrip-drrip-lru-lru-lru-1core.txt",
              "-no-no-no-no-lru-lru-lru-srrip-drrip-lru-lru-lru-1core.txt", "-no-no-no-no-lru-lru-lru-srrip-drrip-lru-lru-lru-1core-cloudsuite.txt"]

pref_combinations = [["ipcp_isca2020_FDP_PrefetchThrottler-no", "ipcp_isca2020_FDP_PrefetchThrottler-ipcp_isca2020", "no_FDP_PrefetchThrottler-spp", "no_FDP_PrefetchThrottler-ppf", "no_FDP_PrefetchThrottler-bingo_dpc3", "ipcp_isca2020_critical_FDP_PrefetchThrottler-no", "ipcp_isca2020_critical_FDP_PrefetchThrottler-ipcp_isca2020_critical", "no_FDP_PrefetchThrottler-spp_critical", "no_FDP_PrefetchThrottler-ppf_critical", "no_FDP_PrefetchThrottler-bingo_critical"], ["ipcp_isca2020_FDP_PrefetchThrottler-no", "ipcp_isca2020_FDP_PrefetchThrottler-ipcp_isca2020", "no_FDP_PrefetchThrottler-spp", "no_FDP_PrefetchThrottler-ppf", "no_FDP_PrefetchThrottler-bingo_dpc3", "ipcp_isca2020_critical_FDP_PrefetchThrottler-no", "ipcp_isca2020_critical_FDP_PrefetchThrottler-ipcp_isca2020_critical", "no_FDP_PrefetchThrottler-spp_critical", "no_FDP_PrefetchThrottler-ppf_critical", "no_FDP_PrefetchThrottler-bingo_critical"], ["ipcp_isca2020_FDP_PrefetchThrottler-no", "ipcp_isca2020_FDP_PrefetchThrottler-ipcp_isca2020", "no_FDP_PrefetchThrottler-spp", "no_FDP_PrefetchThrottler-ppf", "no_FDP_PrefetchThrottler-bingo_dpc3", "ipcp_isca2020_critical_FDP_PrefetchThrottler-no", "ipcp_isca2020_critical_FDP_PrefetchThrottler-ipcp_isca2020_critical", "no_FDP_PrefetchThrottler-spp_critical", "no_FDP_PrefetchThrottler-ppf_critical", "no_FDP_PrefetchThrottler-bingo_critical"]]
        
column_range_end = "T"

a_to_z = ["", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",
          "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"]

column_names = ["", "", "", "", "BF", "BG", "BH", "BI", "BJ", "BK", "BL", "BM", "BN","BO", "BP", "BQ", "BR"]

energy_row_number = ["5", "48", "97"]


def generate_column_names(last_column_range):
    loop_count = a_to_z.index(last_column_range)
    count = 0
    while count < loop_count:
        inner_index = 0
        for x in a_to_z:
            if inner_index > 0:
                column_names.append(a_to_z[count] + a_to_z[inner_index])
            inner_index = inner_index + 1
        count = count + 1


def get_apki(trace_names, pref_name, index, access_list):
    output = []
    access_list_index = 0
    for trace in trace_names:
        filename = results_directory[
            index] + trace + branch_pred + l1i_pref[index] + pref_name + after_pref[index]
        myfile = codecs.open(filename, "r", encoding="utf-8", errors='ignore')
        # open(filename, "r")
        for line in myfile:
            if re.search("CPU 0 cumulative IPC:", line):
                output.append(
                    (float(access_list[access_list_index]) * 1000) / float(line.split(" ")[8]))
                # 6 for APKI and 8 for APKC
                break
        access_list_index = access_list_index + 1

    return output


def get_cic_access_stats(trace_names, pref_name, index, search_key):
    output = []
    # print(search_key)
    for trace in trace_names:
        filename = results_directory[index] + trace + branch_pred + l1i_pref[
            index] + pref_name + after_pref[index]
        result_file = codecs.open(
            filename, "r", encoding="utf-8", errors='ignore')
        # open(filename, "r")

        file_lines = result_file.readlines()

        # Neelu: Need to skip stats for Sturdy cache as they are not needed.

        flag = 0
        for line in file_lines:
            if re.search(search_key, line):
                if flag == 1:
                    # print(trace + line.split(" ")[-1].strip())
                    output.append(line.split(" ")[-1].strip())
                    break
                flag = 1
    # exit(0)
    return output


def get_prefetcher_structure_access_stats(trace_names, pref_name, index, search_key):
    output = []
    for trace in trace_names:
        filename = results_directory[index] + trace + branch_pred + l1i_pref[
            index] + pref_name + after_pref[index]
        result_file = codecs.open(
            filename, "r", encoding="utf-8", errors='ignore')
        # open(filename, "r")

        file_lines = result_file.readlines()

        for line in file_lines:
            if re.search(search_key, line):
                output.append(line.split(" ")[-1].strip())

    return output


def get_stat_by_key(trace_names, pref_name, index, search_key, search_index, split_tab):
    output = []
    for trace in trace_names:
        filename = results_directory[index] + trace + branch_pred + l1i_pref[
            index] + pref_name + after_pref[index]
        result_file = codecs.open(
            filename, "r", encoding="utf-8", errors='ignore')
        # open(filename, "r")

        file_lines = result_file.readlines()
        found = 0
        for line in file_lines:
            if re.search(search_key, line):
                line_words = line.split(" ")
                blank_clear = 0
                while(blank_clear == 0):
                    try:
                        line_words.remove('')
                    except ValueError:
                        blank_clear = 1
                        pass
                # print(line_words)
                if(split_tab):
                    words = line_words[search_index].split("\t")
                    output.append(words[0].strip())
                else:
                    output.append(line_words[search_index].strip())
                found = 1

        if found == 0:
            if len(output) >= 0:
                output.append(0)

    if(search_key == "L2C PREFETCH  ACCESS:"):
        print(trace_names)
        print(output)

    return output


def get_load_requests(trace_names, pref_name, index, cache_level):
    output = []

    search_key = "NOT FOUND"
    if cache_level == 1:
        search_key = "L1D LOAD      ACCESS:"
    else:
        search_key = "L2C LOAD      ACCESS:"
    for trace in trace_names:
        filename = results_directory[index] + trace + branch_pred + l1i_pref[
            index] + pref_name + after_pref[index]
        result_file = codecs.open(
            filename, "r", encoding="utf-8", errors='ignore')
        # open(filename, "r")

        file_lines = result_file.readlines()

        for line in file_lines:
            if re.search(search_key, line):
                line_words = line.split(" ")
                blank_clear = 0
                while(blank_clear == 0):
                    try:
                        line_words.remove('')
                    except ValueError:
                        blank_clear = 1
                        pass
                output.append(line_words[3])

    return output


def get_prefetch_requests(trace_names, pref_name, index, cache_level):
    output = []

    search_key = "NOT FOUND"
    if cache_level == 1:
        search_key = "L1D PREFETCH  REQUESTED:"
    else:
        search_key = "L2C PREFETCH  REQUESTED:"
    for trace in trace_names:
        filename = results_directory[index] + trace + branch_pred + l1i_pref[
            index] + pref_name + after_pref[index]
        result_file = codecs.open(
            filename, "r", encoding="utf-8", errors='ignore')
        # open(filename, "r")

        file_lines = result_file.readlines()

        for line in file_lines:
            if re.search(search_key, line):
                line_words = line.split(" ")
                blank_clear = 0
                while(blank_clear == 0):
                    try:
                        line_words.remove('')
                    except ValueError:
                        blank_clear = 1
                        pass
                output.append(line_words[3])

    return output


def get_fill_level_llc_requests(trace_names, pref_name, index, search_key):
    output = []
    for trace in trace_names:
        filename = results_directory[index] + trace + branch_pred + l1i_pref[
            index] + pref_name + after_pref[index]
        result_file = codecs.open(
            filename, "r", encoding="utf-8", errors='ignore')
        # open(filename, "r")

        file_lines = result_file.readlines()

        for line in file_lines:
            if re.search(search_key, line):
                # print(line)
                output.append(line.split(" ")[-1].strip())

    return output


def get_execution_cycles(trace_names, pref_name, index):
    output = []
    for trace in trace_names:
        filename = results_directory[index] + trace + branch_pred + l1i_pref[
            index] + pref_name + after_pref[index]
        result_file = codecs.open(
            filename, "r", encoding="utf-8", errors='ignore')
        # open(filename, "r")

        file_lines = result_file.readlines()

        for line in file_lines:
            if re.search("CPU 0 cumulative IPC:", line):
                output.append(line.split(" ")[8])
                if(filename.find("server_013.champsimtrace.xz-hashed_perceptron-mana_32KB-ipcp_isca2020_UpdatedAccessStats") != -1):
                    print("Execution cycles " + line.split(" ")[8])
    return output


def get_cache_misses(trace_names, pref_name, index, search_key):
    output = []
    for trace in trace_names:
        filename = results_directory[index] + trace + branch_pred + l1i_pref[
            index] + pref_name + after_pref[index]
        result_file = codecs.open(
            filename, "r", encoding="utf-8", errors='ignore')
        # open(filename, "r")
        file_lines = result_file.readlines()
        for line in file_lines:
            if re.search(search_key, line):
                line_words = line.split(" ")
                blank_clear = 0
                while(blank_clear == 0):
                    try:
                        line_words.remove('')
                    except ValueError:
                        blank_clear = 1
                        pass  # do nothing!
                output.append(line_words[7])
                if(filename.find("server_013.champsimtrace.xz-hashed_perceptron-mana_32KB-ipcp_isca2020_UpdatedAccessStats") != -1):
                    print(search_key + " " + line_words[3])
                break

    return output


def get_cache_access_stats(trace_names, pref_name, index, search_key):
    output = []
    for trace in trace_names:
        found = 0
        filename = results_directory[index] + trace + branch_pred + l1i_pref[
            index] + pref_name + after_pref[index]
        result_file = codecs.open(
            filename, "r", encoding="utf-8", errors='ignore')
        # open(filename, "r")
        file_lines = result_file.readlines()
        for line in file_lines:
            if re.search(search_key, line):
                found = 1
                line_words = line.split(" ")
                blank_clear = 0
                while(blank_clear == 0):
                    try:
                        line_words.remove('')
                    except ValueError:
                        blank_clear = 1
                        pass  # do nothing!
                output.append(line_words[3])
                if(filename.find("server_013.champsimtrace.xz-hashed_perceptron-mana_32KB-ipcp_isca2020_UpdatedAccessStats") != -1):
                    print(search_key + " " + line_words[3])
                break
        if(found == 0):
            output.append(0)
    return output


def get_dram_access_stats(trace_names, pref_name, index, search_key, hits_misses):
    output = []
    for trace in trace_names:
        filename = results_directory[index] + trace + branch_pred + l1i_pref[
            index] + pref_name + after_pref[index]
        result_file = codecs.open(
            filename, "r", encoding="utf-8", errors='ignore')
        # open(filename, "r")
        file_lines = result_file.readlines()
        for line in file_lines:
            if re.search(search_key, line):
                line_words = line.split(" ")
                blank_clear = 0
                while(blank_clear == 0):
                    try:
                        line_words.remove('')
                    except ValueError:
                        blank_clear = 1
                        pass  # do nothing!
                output.append(int(line_words[2]) + int(line_words[4]))
                break
    return output



def get_energy_values(trace_names, pref_name, index, search_key, per_access_energy, dram_or_not, only_hits_misses):
    print("Inside get_energy_values. Exiting.")
    exit(0)

    output = []
    for trace in trace_names:
        filename = results_directory[index] + trace + branch_pred + l1i_pref[
            index] + pref_name + after_pref[index]
        result_file = codecs.open(
            filename, "r", encoding="utf-8", errors='ignore')
        # open(filename, "r")

        file_lines = result_file.readlines()

        # The following value of number of cycles is needed only when DRAM
        # energy calculation is done using MICRON.
        num_of_cycles = 1
        # if dram_or_not:
        for line in file_lines:
            if re.search("CPU 0 cumulative IPC:", line):
                num_of_cycles = line.split(" ")[8]

        if int(num_of_cycles) == 0:
            print(
                "Number of cycles are found to be zero! Please check your code.")
            exit(0)

        # print("Outside for loop")
        for line in file_lines:
            if re.search(search_key, line):
                line_words = line.split(" ")
                blank_clear = 0
                while(blank_clear == 0):
                    try:
                        line_words.remove('')
                    except ValueError:
                        blank_clear = 1
                        pass  # do nothing!
                if dram_or_not:

                    if only_hits_misses == 1:
                        output.append(line_words[2])
                    elif only_hits_misses == 2:
                        output.append(line_words[4])
                    else:
                        # The following one line is the energy calculation using CACTI.
                        # energy =
                        # ((float(line_words[2])+float(line_words[4]))*per_access_energy)

                        # The following is power calculation using Micron.
                        energy = 0
                        num_of_dram_rw_cycles = line_words[7]
                        percentage_of_dram_rw_cycles = (
                            float(num_of_dram_rw_cycles) / float(num_of_cycles)) * 100
                        if percentage_of_dram_rw_cycles > 100:
                            print(
                                "percentage_of_dram_rw_cycles > 100. Please check your code.")
                            exit(0)
                        else:
                            energy = per_access_energy * \
                                percentage_of_dram_rw_cycles
                        output.append(energy)
                else:
                    # The following one line is the energy calculation using CACTI.
                    # output.append(float(line_words[3])*per_access_energy)

                    # Following is the power calculation using CACTI.
                    energy = float(line_words[3]) * per_access_energy
                    power = energy / float(num_of_cycles)

                    output.append(line_words[3])

                    # output.append(energy)

                    # output.append(power)
                    # print("Cache:"+str(power));
                break
    # print("Before Returning")
    return output


# If modifying these scopes, delete the file token.pickle.
SCOPES = ['https://www.googleapis.com/auth/spreadsheets']
# The ID and range of a sample spreadsheet.
# Enter your sheet ID here
sheet_id_file = open("Neelus_MS_Thesis_sheet_id.txt", "r")
sheet_id = sheet_id_file.readlines()
SAMPLE_SPREADSHEET_ID = sheet_id[0].strip()  # Enter your sheet ID here


SAMPLE_RANGE_NAME = 'A2:A13'


def main():
    """Shows basic usage of the Sheets API.
    Prints values from a sample spreadsheet.
    """
    creds = None
    # The file token.pickle stores the user's access and refresh tokens, and is
    # created automatically when the authorization flow completes for the first
    # time.
    if os.path.exists('token.pickle'):
        with open('token.pickle', 'rb') as token:
            creds = pickle.load(token)
    # If there are no (valid) credentials available, let the user log in.
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            flow = InstalledAppFlow.from_client_secrets_file(
                'credentials.json', SCOPES)
            creds = flow.run_local_server(port=0)
        # Save the credentials for the next run
        with open('token.pickle', 'wb') as token:
            pickle.dump(creds, token)
    service = build('sheets', 'v4', credentials=creds)

    # loop for pref and loop for trace_files. So I'm into outer loop for
    # trace_files and inner loop for pref.

    #generate_column_names(column_range_end)

    iteration_count = 0
    for trace_list_file in trace_list_files:
        if send_data[iteration_count] == 1:
            trace_names = []
            trace_file = open(trace_list_file, "r")
            for line in trace_file:
                trace_names.append(line.strip())

            inner_iter_count = 0
            column_name_index = 3
            for pref_combination in pref_combinations[iteration_count]:

                pref_name = pref_combination
                print(pref_combination)
                prefs_list = pref_combination.split("-")
                print(prefs_list)
                # pref_name = pref_name.strip()
                heading_name = ["MyData"]
                # column_name_index = 8 * inner_iter_count
                column_name_index = column_name_index + 1
                # column_name = column_names[column_name_index];


# -------------Execution Cycles-------------#

                execution_cycles = get_execution_cycles(
                    trace_names, pref_name, iteration_count)

# -------------- L1I cache ----------------#

                l1i_data_tag_read_energy_per_access = 6.39614e-05 + 0.033625
                l1i_tag_read_energy_per_access = 6.39614e-05

                # Data + Tag
                l1i_rq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L1I RQ\tACCESS:", 5, 0)
                l1i_rq_merged = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L1I RQ\tACCESS:", 4, 1)
                l1i_prefetch_miss = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L1I PREFETCH  ACCESS:", 7, 0)

                # Tag
                l1i_load_miss = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L1I LOAD      ACCESS:", 7, 0)
                l1i_pq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L1I PQ\tACCESS:", 5, 0)

                l1i_energy_stats = []
                l1i_power_stats = []

                for stat in l1i_rq_to_cache:
                    l1i_energy_stats.append(
                        float(stat) * l1i_data_tag_read_energy_per_access)

                loop_count = 0
                for stat in l1i_rq_merged:
                    l1i_energy_stats[loop_count] = l1i_energy_stats[loop_count] + (
                        float(stat) * l1i_data_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in l1i_prefetch_miss:
                    l1i_energy_stats[loop_count] = l1i_energy_stats[loop_count] + (
                        float(stat) * l1i_data_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in l1i_load_miss:
                    l1i_energy_stats[loop_count] = l1i_energy_stats[
                        loop_count] + (float(stat) * l1i_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in l1i_pq_to_cache:
                    l1i_energy_stats[loop_count] = l1i_energy_stats[
                        loop_count] + (float(stat) * l1i_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_index = 0
                for stat in l1i_energy_stats:
                    l1i_power_stats.append(
                        (float(stat) * 4 * 1000) / float(execution_cycles[loop_index]))
                    loop_index = loop_index + 1

                # *4 because 4 GHz processor, *1000 to convert it into mW.

                l1i_energy_stats.insert(0, "L1I Energy")
                l1i_power_stats.insert(0, "L1I Power")


# -------------- ITLB ----------------#

                itlb_data_tag_read_energy_per_access = 2.34969e-05 + \
                    8.23319e-05
                itlb_tag_read_energy_per_access = 2.34969e-05

                # Data + Tag
                itlb_rq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "ITLB RQ\tACCESS:", 5, 0)
                itlb_rq_merged = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "ITLB RQ\tACCESS:", 4, 1)

                # Tag
                itlb_total_miss = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "ITLB TOTAL     ACCESS:", 7, 0)

                itlb_energy_stats = []
                itlb_power_stats = []

                for stat in itlb_rq_to_cache:
                    itlb_energy_stats.append(
                        float(stat) * itlb_data_tag_read_energy_per_access)

                loop_count = 0
                for stat in itlb_rq_merged:
                    itlb_energy_stats[loop_count] = itlb_energy_stats[loop_count] + (
                        float(stat) * itlb_data_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in itlb_total_miss:
                    itlb_energy_stats[loop_count] = itlb_energy_stats[
                        loop_count] + (float(stat) * itlb_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_index = 0
                for stat in itlb_energy_stats:
                    itlb_power_stats.append(
                        (float(stat) * 4 * 1000) / float(execution_cycles[loop_index]))
                    loop_index = loop_index + 1

                # *4 because 4 GHz processor, *1000 to convert it into mW.

                itlb_energy_stats.insert(0, "ITLB Energy")
                itlb_power_stats.insert(0, "ITLB Power")


# -------------- DTLB ----------------#

                dtlb_data_tag_read_energy_per_access = 2.34969e-05 + \
                    8.23319e-05
                dtlb_tag_read_energy_per_access = 2.34969e-05

                # Data + Tag
                dtlb_rq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "DTLB RQ\tACCESS:", 5, 0)
                dtlb_rq_merged = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "DTLB RQ\tACCESS:", 4, 1)

                # Tag
                dtlb_load_miss = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "DTLB LOAD      ACCESS:", 7, 0)

                dtlb_energy_stats = []
                dtlb_power_stats = []

                for stat in dtlb_rq_to_cache:
                    dtlb_energy_stats.append(
                        float(stat) * dtlb_data_tag_read_energy_per_access)

                loop_count = 0
                for stat in dtlb_rq_merged:
                    dtlb_energy_stats[loop_count] = dtlb_energy_stats[loop_count] + (
                        float(stat) * dtlb_data_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in dtlb_load_miss:
                    dtlb_energy_stats[loop_count] = dtlb_energy_stats[
                        loop_count] + (float(stat) * dtlb_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_index = 0
                for stat in dtlb_energy_stats:
                    dtlb_power_stats.append(
                        (float(stat) * 4 * 1000) / float(execution_cycles[loop_index]))
                    loop_index = loop_index + 1

                # *4 because 4 GHz processor, *1000 to convert it into mW.

                dtlb_energy_stats.insert(0, "DTLB Energy")
                dtlb_power_stats.insert(0, "DTLB Power")


# -------------- STLB ----------------#

                stlb_data_tag_read_energy_per_access = 4.83723e-05 + \
                    0.00021464 + 7.09336e-05 + 0.000737758
                stlb_tag_read_energy_per_access = 4.83723e-05 + 7.09336e-05

                # Data + Tag
                stlb_rq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "STLB RQ\tACCESS:", 5, 0)
                stlb_rq_merged = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "STLB RQ\tACCESS:", 4, 1)

                # Tag
                stlb_total_miss = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "STLB TOTAL     ACCESS:", 7, 0)

                stlb_energy_stats = []
                stlb_power_stats = []

                for stat in stlb_rq_to_cache:
                    stlb_energy_stats.append(
                        float(stat) * stlb_data_tag_read_energy_per_access)

                loop_count = 0
                for stat in stlb_rq_merged:
                    stlb_energy_stats[loop_count] = stlb_energy_stats[loop_count] + (
                        float(stat) * stlb_data_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in stlb_total_miss:
                    stlb_energy_stats[loop_count] = stlb_energy_stats[
                        loop_count] + (float(stat) * stlb_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_index = 0
                for stat in stlb_energy_stats:
                    stlb_power_stats.append(
                        (float(stat) * 4 * 1000) / float(execution_cycles[loop_index]))
                    loop_index = loop_index + 1

                # *4 because 4 GHz processor, *1000 to convert it into mW.

                stlb_energy_stats.insert(0, "STLB Energy")
                stlb_power_stats.insert(0, "STLB Power")


# -------------- L1D cache ----------------#

                # Average of (4 ways + 8 ways) of 32 KB and 64 KB energy from
                # pcacti to get it for 12 ways i.e. 48 KB
                l1d_data_tag_read_energy_per_access = (
                    6.64038e-05 + 0.212011 + 7.49382e-05 + 0.875077 + 0.000108651 + 0.222263 + 0.000122313 + 0.88919) / 2
                l1d_tag_read_energy_per_access = (
                    6.64038e-05 + 7.49382e-05 + 0.000108651 + 0.000122313) / 2
                l1d_data_tag_write_energy_per_access = (
                    8.71032e-05 + 0.218411 + 0.000146534 + 0.888183 + 0.000101881 + 0.235236 + 0.00016644 + 0.915748) / 2

                # Data + Tag
                l1d_rq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L1D RQ\tACCESS:", 5, 0)
                l1d_rq_merged = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L1D RQ\tACCESS:", 4, 1)
                l1d_prefetch_miss = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L1D PREFETCH  ACCESS:", 7, 0)
                l1d_wq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L1D WQ\tACCESS:", 5, 0)

                # Tag
                l1d_load_miss = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L1D LOAD      ACCESS:", 7, 0)
                l1d_pq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L1D PQ\tACCESS:", 5, 0)
                l1d_rfo_miss = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L1D RFO       ACCESS:", 7, 0)

                l1d_energy_stats = []
                l1d_power_stats = []

                for stat in l1d_rq_to_cache:
                    l1d_energy_stats.append(
                        float(stat) * l1d_data_tag_read_energy_per_access)

                loop_count = 0
                for stat in l1d_rq_merged:
                    l1d_energy_stats[loop_count] = l1d_energy_stats[loop_count] + (
                        float(stat) * l1d_data_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in l1d_prefetch_miss:
                    l1d_energy_stats[loop_count] = l1d_energy_stats[loop_count] + (
                        float(stat) * l1d_data_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in l1d_wq_to_cache:
                    l1d_energy_stats[loop_count] = l1d_energy_stats[loop_count] + (
                        float(stat) * l1d_data_tag_write_energy_per_access)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in l1d_load_miss:
                    l1d_energy_stats[loop_count] = l1d_energy_stats[
                        loop_count] + (float(stat) * l1d_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in l1d_pq_to_cache:
                    l1d_energy_stats[loop_count] = l1d_energy_stats[
                        loop_count] + (float(stat) * l1d_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in l1d_rfo_miss:
                    l1d_energy_stats[loop_count] = l1d_energy_stats[
                        loop_count] + (float(stat) * l1d_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_index = 0
                for stat in l1d_energy_stats:
                    l1d_power_stats.append(
                        (float(stat) * 4 * 1000) / float(execution_cycles[loop_index]))
                    loop_index = loop_index + 1

                # *4 because 4 GHz processor, *1000 to convert it into mW.

                l1d_energy_stats.insert(0, "L1D Energy")
                l1d_power_stats.insert(0, "L1D Power")

# --------------- L2C cache --------------#

                l2c_data_tag_read_energy_per_access = 0.000289392 + 0.045232
                l2c_tag_read_energy_per_access = 0.000289392
                l2c_data_read_energy_per_access = 0.045232
                l2c_data_tag_write_energy_per_access = 0.000313777 + 0.0562052

                # Data + Tag
                l2c_rq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L2C RQ\tACCESS:", 5, 0)
                l2c_prefetch_miss = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L2C PREFETCH  ACCESS:", 7, 0)
                l2c_wq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L2C WQ\tACCESS:", 5, 0)

                # Tag
                # Need to subtract prefetch_misses by total misses for getting
                # tag accesses.
                l2c_total_miss = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L2C TOTAL     ACCESS:", 7, 0)
                l2c_pq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "L2C PQ\tACCESS:", 5, 0)

                l2c_energy_stats = []
                l2c_power_stats = []

                for stat in l2c_rq_to_cache:
                    l2c_energy_stats.append(
                        float(stat) * l2c_data_tag_read_energy_per_access)

                loop_count = 0
                for stat in l2c_prefetch_miss:
                    l2c_energy_stats[loop_count] = l2c_energy_stats[loop_count] + (
                        float(stat) * l2c_data_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in l2c_wq_to_cache:
                    l2c_energy_stats[loop_count] = l2c_energy_stats[loop_count] + (
                        float(stat) * l2c_data_tag_write_energy_per_access)
                    loop_count = loop_count + 1

                print(str(len(l2c_total_miss)) +
                      " " + str(len(l2c_prefetch_miss)))

                loop_count = 0
                for stat in l2c_total_miss:
                    if len(l2c_prefetch_miss) != 0:
                        l2c_energy_stats[loop_count] = l2c_energy_stats[
                            loop_count] + ((float(stat) - float(l2c_prefetch_miss[loop_count])) * l2c_tag_read_energy_per_access)
                    else:
                        l2c_energy_stats[loop_count] = l2c_energy_stats[
                            loop_count] + (float(stat) * l2c_tag_read_energy_per_access)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in l2c_pq_to_cache:
                    l2c_energy_stats[loop_count] = l2c_energy_stats[
                        loop_count] + (float(stat) * l2c_data_tag_read_energy_per_access)
                    loop_count = loop_count + 1


                loop_index = 0
                for stat in l2c_energy_stats:
                    l2c_power_stats.append(
                        (float(stat) * 4 * 1000) / float(execution_cycles[loop_index]))
                    loop_index = loop_index + 1

                # *4 because 4 GHz processor, *1000 to convert it into mW.

                l2c_energy_stats.insert(0, "L2C Energy")
                l2c_power_stats.insert(0, "L2C Power")

# --------------- LLC cache -------------#

                llc_data_tag_read_energy_per_access = 0.000692485 + 0.100439
                llc_tag_read_energy_per_access = 0.000692485
                llc_data_tag_write_energy_per_access = 0.000785771 + 0.109842

                # Data + Tag
                llc_rq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "LLC RQ\tACCESS:", 5, 0)
                llc_prefetch_miss = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "LLC PREFETCH  ACCESS:", 7, 0)
                llc_wq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "LLC WQ\tACCESS:", 5, 0)

                # Tag
                # Need to subtract prefetch_misses by total misses for getting
                # tag accesses.
                llc_total_miss = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "LLC TOTAL     ACCESS:", 7, 0)
                llc_pq_to_cache = get_stat_by_key(
                    trace_names, pref_name, iteration_count, "LLC PQ\tACCESS:", 5, 0)

                llc_energy_stats = []
                llc_power_stats = []
                llc_total_access_stats = []

                for stat in llc_rq_to_cache:
                    llc_energy_stats.append(
                        float(stat) * llc_data_tag_read_energy_per_access)
                    llc_total_access_stats.append(int(stat))

                loop_count = 0
                for stat in llc_prefetch_miss:
                    llc_energy_stats[loop_count] = llc_energy_stats[loop_count] + (
                        float(stat) * llc_data_tag_read_energy_per_access)
                    llc_total_access_stats[loop_count] += int(stat)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in llc_wq_to_cache:
                    llc_energy_stats[loop_count] = llc_energy_stats[loop_count] + (
                        float(stat) * llc_data_tag_write_energy_per_access)
                    llc_total_access_stats[loop_count] += int(stat)
                    loop_count = loop_count + 1

                loop_count = 0
                for stat in llc_total_miss:
                    if len(llc_prefetch_miss) != 0:
                        llc_energy_stats[loop_count] = llc_energy_stats[
                            loop_count] + ((float(stat) - float(llc_prefetch_miss[loop_count])) * llc_tag_read_energy_per_access)
                    else:
                        llc_energy_stats[loop_count] = llc_energy_stats[
                            loop_count] + (float(stat) * llc_tag_read_energy_per_access)
                    llc_total_access_stats[loop_count] += int(stat)
                    loop_count = loop_count + 1


                loop_count = 0
                for stat in llc_pq_to_cache:
                    llc_energy_stats[loop_count] = llc_energy_stats[
                        loop_count] + (float(stat) * llc_data_tag_read_energy_per_access)
                    llc_total_access_stats[loop_count] += int(stat)
                    loop_count = loop_count + 1




                loop_index = 0
                for stat in llc_energy_stats:
                    llc_power_stats.append(
                        (float(stat) * 4 * 1000) / float(execution_cycles[loop_index]))
                    loop_index = loop_index + 1

                # *4 because 4 GHz processor, *1000 to convert it into mW.

                llc_energy_stats.insert(0, "LLC Energy")
                llc_power_stats.insert(0, "LLC Power")

# ----------------- On-chip Interconnect ----------------- #


                interconnect_energy_ten_percent_of_llc = []
                dummy_zeroes = []
                interconnect_power_ten_percent_of_llc = []
                loop_index = 1
                for stat in llc_energy_stats:
                    if(loop_index < len(llc_energy_stats)):
                        interconnect_energy_ten_percent_of_llc.append(
                            float(llc_energy_stats[loop_index]) * 0.1)
                        loop_index = loop_index + 1
                        dummy_zeroes.append(0)

                loop_index = 0
                for stat in interconnect_energy_ten_percent_of_llc:
                    interconnect_power_ten_percent_of_llc.append(
                        (float(stat) * 4 * 1000) / float(execution_cycles[loop_index]))
                    loop_index = loop_index + 1

                interconnect_energy_ten_percent_of_llc.insert(
                    0, "Interconnect Total Energy")
                interconnect_power_ten_percent_of_llc.insert(
                    0, "Interconnect Total Power")
                dummy_zeroes.insert(0, "")


# ------------------ DRAM -------------#

                dram_read_access_stats = get_dram_access_stats(
                    trace_names, pref_name, iteration_count, "RQ ROW_BUFFER_HIT:", 1)

                dram_read_apki = get_apki(
                    trace_names, pref_name, iteration_count, dram_read_access_stats)
                dram_read_apki.insert(0, "DRAM Read APKI")

                dram_read_energy_stats = []
                for stat in dram_read_access_stats:
                    dram_read_energy_stats.append(float(stat) * 10.24)
                    # Per Access Energy from Figure 2 (Leveraging
                    # Power-Performance Relationship of Energy-Efficient Modern
                    # DRAM Devices)

                dram_read_power_stats = []
                loop_index = 0
                for stat in dram_read_energy_stats:
                    dram_read_power_stats.append(
                        (float(stat) * 4 * 1000) / float(execution_cycles[loop_index]))
                    loop_index = loop_index + 1

                dram_write_access_stats = get_dram_access_stats(
                    trace_names, pref_name, iteration_count, "WQ ROW_BUFFER_HIT:", 1)

                dram_write_apki = get_apki(
                    trace_names, pref_name, iteration_count, dram_write_access_stats)
                dram_write_apki.insert(0, "DRAM Write APKI")

                dram_write_energy_stats = []
                for stat in dram_write_access_stats:
                    dram_write_energy_stats.append(float(stat) * 10.24)
                    # Per Access Energy from Figure 2 (Leveraging
                    # Power-Performance Relationship of Energy-Efficient Modern
                    # DRAM Devices)

                dram_write_power_stats = []
                loop_index = 0
                for stat in dram_write_energy_stats:
                    dram_write_power_stats.append(
                        (float(stat) * 4 * 1000) / float(execution_cycles[loop_index]))
                    loop_index = loop_index + 1

                dram_read_access_stats.insert(0, "DRAM Reads")
                dram_write_access_stats.insert(0, "DRAM Writes")
                dram_read_energy_stats.insert(0, "DRAM Read Energy")
                dram_write_energy_stats.insert(0, "DRAM Write Energy")
                dram_read_power_stats.insert(0, "DRAM Read Power")
                dram_write_power_stats.insert(0, "DRAM Write Power")


# -------------------------------- CRITICALITY DETECTION -------------------------------------- #
                if pref_name.find("critical") != -1:
                    cic_tag_read_accesses = get_cic_access_stats(
                        trace_names, pref_name, iteration_count, "CIC Tag Read Accesses")

                    print(str(len(cic_tag_read_accesses)) + "LEN")

                    cic_tag_read_energy_stats = []
                    cic_tag_read_power_stats = []
                    loop_index = 0
                    for stat in cic_tag_read_accesses:
                        cic_tag_read_energy_stats.append(
                            float(stat) * 6.09103e-05)
                        loop_index = loop_index + 1


                    cic_tag_write_accesses = get_cic_access_stats(
                        trace_names, pref_name, iteration_count, "CIC Tag Write Accesses")

                    loop_index = 0
                    cic_tag_write_energy_stats = []
                    cic_tag_write_power_stats = []
                    for stat in cic_tag_write_accesses:
                        cic_tag_write_energy_stats.append(
                            float(stat) * 7.27458e-05)
                        loop_index = loop_index + 1

                    pist_tag_read_accesses = get_prefetcher_structure_access_stats(
                        trace_names, pref_name, iteration_count, "PIST Tag Read Accesses")

                    loop_index = 0
                    pist_tag_read_energy_stats = []
                    pist_tag_read_power_stats = []
                    for stat in pist_tag_read_accesses:
                        pist_tag_read_energy_stats.append(
                            float(stat) * 3.01112e-05)
                        loop_index = loop_index + 1

                    pist_read_accesses = get_prefetcher_structure_access_stats(
                        trace_names, pref_name, iteration_count, "PIST Read Accesses")

                    loop_index = 0
                    pist_read_energy_stats = []
                    pist_read_power_stats = []
                    for stat in pist_read_accesses:
                        pist_read_energy_stats.append(
                            float(stat) * 2.28775e-05)
                        loop_index = loop_index + 1

                    pist_tag_write_accesses = get_prefetcher_structure_access_stats(
                        trace_names, pref_name, iteration_count, "PIST Tag Write Accesses")

                    loop_index = 0
                    pist_tag_write_energy_stats = []
                    pist_tag_write_power_stats = []
                    for stat in pist_tag_write_accesses:
                        pist_tag_write_energy_stats.append(
                            float(stat) * 3.63366e-05)
                        loop_index = loop_index + 1

                    pist_write_accesses = get_prefetcher_structure_access_stats(
                        trace_names, pref_name, iteration_count, "PIST Write Accesses")

                    loop_index = 0
                    pist_write_energy_stats = []
                    pist_write_power_stats = []
                    for stat in pist_write_accesses:
                        pist_write_energy_stats.append(
                            float(stat) * 1.71252e-05)
                        loop_index = loop_index + 1


                execution_cycles_to_sheet = execution_cycles
                execution_cycles_to_sheet.insert(0, "Execution Cycles")

                # Call the Sheets API
                sheet = service.spreadsheets()

                total_energy_values = ["Total_energy"]

                count = 0
                for stat in l1i_energy_stats:
                    if count > 0:
                        total_energy_values.append(0)
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in l1d_energy_stats:
                    if count > 0: 
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in l2c_energy_stats:
                    if count > 0: 
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in llc_energy_stats:
                    if count > 0: 
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in interconnect_energy_ten_percent_of_llc:
                    if count > 0: 
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in dram_read_energy_stats:
                    if count > 0: 
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in dram_write_energy_stats:
                    if count > 0:     
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in itlb_energy_stats:
                    if count > 0: 
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in dtlb_energy_stats:
                    if count > 0:     
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in stlb_energy_stats:
                    if count > 0: 
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                data = [  # {'range': "IPC_SERVER!"+column_name+"3",'values': ['a'], 'majorDimension': 'COLUMNS'},\
                    {'range': sheet_name + column_names[column_name_index] + "2", 'values': [[prefs_list[0]]], 'majorDimension': 'COLUMNS'},\
                    {'range': sheet_name + column_names[column_name_index] + "3", 'values': [[prefs_list[1]]], 'majorDimension': 'COLUMNS'},\

                ]

                pref_flag = 0

                if pref_name.find("ipcp") != -1:
                    pref_flag = 1
                elif pref_name.find("bingo") != -1:
                    pref_flag = 2
                elif pref_name.find("spp") != -1:
                    pref_flag = 3
                elif pref_name.find("ppf") != -1:
                    pref_flag = 4

                searchkeys = []
                cacti_energy_values = []

                if pref_flag == 1:
                    if prefs_list[len(prefs_list)-1].find("ipcp") != -1:
                        searchkeys = [
                            "L1 IP Table Write Accesses:", "L1 IP Table Tag Write Accesses:", "L1 IP Table Read Accesses:", "L1 IP Table Tag Read Accesses:", "L1 RST Write Accesses:", "L1 RST Read Accesses:", "L1 CSPT Write Accesses:",
                            "L1 CSPT Read Accesses:", "L1 RR Filter Tag Write Accesses:", "L1 RR Filter Read Accesses:", "L2 IP Table Read Accesses:", "L2 IP Table Write Accesses:", "L2 IP Table Tag Read Accesses:", "L2 IP Table Tag Write Accesses:"]
                        cacti_energy_values = [
                            1.8601e-05, 9.36467e-06, 2.2638e-05, 1.00253e-05, 2.26722e-05, 2.21818e-05,
                            1.8601e-05, 2.2638e-05, 6.36771e-05, 6.07522e-05, 1.47529e-05, 1.26506e-05, 1.00253e-05, 9.36467e-06]
                    else:
                        searchkeys = [
                            "L1 IP Table Write Accesses:", "L1 IP Table Tag Write Accesses:", "L1 IP Table Read Accesses:", "L1 IP Table Tag Read Accesses:",
                            "L1 RST Write Accesses:", "L1 RST Read Accesses:", "L1 CSPT Write Accesses:", "L1 CSPT Read Accesses:", "L1 RR Filter Tag Write Accesses:", "L1 RR Filter Read Accesses:"]
                        cacti_energy_values = [
                            1.8601e-05, 9.36467e-06, 2.2638e-05, 1.00253e-05,
                            2.26722e-05, 2.21818e-05, 1.8601e-05, 2.2638e-05, 6.36771e-05, 6.07522e-05]

                elif pref_flag == 2:
                    # Note: AT, FT, PS: Sets x2/4 in pcacti. So dividing only
                    # data array energy by 2/4, not tag array energy.
                    searchkeys = [
                        "PHT read accesses:", "PHT tag read accesses:", "PHT write accesses:", "PHT tag write accesses:", "AT read accesses:", "AT tag read accesses:", "AT write accesses:", "AT tag write accesses:",
                        "FT read accesses:", "FT tag read accesses:", "FT write accesses:", "FT tag write accesses:", "PS read accesses:", "PS tag read accesses:", "PS write accesses:", "PS tag write accesses:"]
                    cacti_energy_values = [
                        0.000653085, 0.000668904, 0.000468651, 0.00136726, 7.52551e-05 /
                            2, 0.000156216, 4.66294e-05 / 2,
                        0.000151888, 4.087e-05 / 4, 0.000156216, 2.54729e-05 / 4, 0.000151888, 7.88264e-05 / 2, 0.000207258, 4.8172e-05 / 2, 0.000202359]

                elif pref_flag == 3:
                    searchkeys = [
                        "ST read accesses:", "ST write accesses:", "PT read accesses:",
                        "PT write accesses:", "FILTER read accesses:", "FILTER write accesses:"]
                    cacti_energy_values = [
                        0.000603718, 0.000657546, 9.05961e-05, 5.92259e-05, 4.25511e-05, 3.09755e-05]

                elif pref_flag == 4:
                    searchkeys = [
                        "ST read accesses:", "ST write accesses:", "PT read accesses:", "PT write accesses:",
                        "FILTER read accesses:", "FILTER write accesses:", "PERC read accesses:", "PERC write accesses:"]
                    cacti_energy_values = [
                        0.000603718, 0.000657546, 9.05961e-05, 5.92259e-05, 4.25511e-05, 3.09755e-05, 9.077e-05, 5.93997e-05]
                if pref_flag != 0:
                    outer_loop_index = 0
                    # get prefetch and load accesses
                    pref_cache_level = 0
                    if prefs_list[0].find("no") != -1:
                        pref_cache_level = 2
                    elif prefs_list[1].find("no") != -1:
                        pref_cache_level = 1
                    prefetch_accesses = get_prefetch_requests(
                        trace_names, pref_name,  iteration_count, pref_cache_level)
                    prefetch_accesses.insert(0, "Prefetch Accesses")
                    load_accesses = get_load_requests(
                        trace_names, pref_name,  iteration_count, pref_cache_level)
                    load_accesses.insert(0, "Load Accesses")

                    for key in searchkeys:
                        if key.find("L1 RR Filter Read Accesses") != -1 and pref_name.find("no") != -1 and pref_name.find("critical") == -1 and iteration_count == 1:
                            accesses_values = get_prefetcher_structure_access_stats(
                                trace_names, "ipcp_isca2020_256entr-no",  iteration_count, key)
                            execution_cycles = get_execution_cycles(
                                trace_names, "ipcp_isca2020_256entr-no", iteration_count)
                        else:
                            accesses_values = get_prefetcher_structure_access_stats(
                                trace_names, pref_name, iteration_count, key)
                            execution_cycles = get_execution_cycles(
                                trace_names, pref_name, iteration_count)

                        energy_values = []
                        power_values = []

                        inner_loop_index = 0
                        for accesses in accesses_values:
                            energy = float(accesses) * cacti_energy_values[
                                outer_loop_index]
                            power = (float(energy) * 4 * 1000) / float(
                                execution_cycles[inner_loop_index])
                            energy_values.append(energy)
                            power_values.append(power)
                            inner_loop_index = inner_loop_index + 1

                        accesses_values.insert(0, key + " Accesses")

                        energy_values.insert(0, key + " Energy")
                        power_values.insert(0, key + " Power")
                        outer_loop_index = outer_loop_index + 1


                        count = 0
                        print(energy_values[0])
                        for stat in energy_values:
                            if count > 0: 
                                total_energy_values[
                                    count] = total_energy_values[count] + float(stat)
                            if count == 19:
                                print(stat)
                            count = count + 1

                data.append({'range': sheet_name + column_names[column_name_index] + energy_row_number[
                            iteration_count], 'values': [total_energy_values], 'majorDimension': 'COLUMNS'})
        

                body = {'valueInputOption': 'USER_ENTERED', 'data': data}
                result = sheet.values().batchUpdate(
                    spreadsheetId=SAMPLE_SPREADSHEET_ID, body=body).execute()
                print(' {0} cells updated.'.format(
                    result.get('totalUpdatedCells')))
                inner_iter_count = inner_iter_count + 1

        iteration_count = iteration_count + 1


if __name__ == '__main__':
    main()
