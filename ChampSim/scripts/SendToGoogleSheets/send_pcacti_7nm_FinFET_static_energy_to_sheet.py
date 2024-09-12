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
# server, spec17, cloud

results_directory = ["../../IPC_SOTA_results_50M/",
                     "../../SPEC2017_SOTA_results_100M/", "../../CLOUDSUITE_SOTA_results_50M/"]

branch_pred = "-hashed_perceptron"

l1i_pref = ["-fnlmma_25KB-", "-no-", "-no-"]

trace_list_files = ["../selected_ipc_trace_list.txt",
                    "../selected_intensive_trace_list.txt", "../selected_cloudsuite_trace_list.txt"]

# sheet_names = ["SERVER_STATS!", "SPEC2017_STATS!", "SPEC2006_STATS!"]

power_gating_low_apkc = 0.002
power_gating_med_apkc = 0.01
power_gating_high_apkc = 0.045

# sheet_name = "Copy of Test 1!"
sheet_name = "SOTA_Criticality_Energy!"
# "Memory_Hierarchy_Energy_Power!"

after_pref = ["-no-no-no-no-lru-lru-lru-srrip-drrip-lru-lru-lru-1core.txt",
              "-no-no-no-no-lru-lru-lru-srrip-drrip-lru-lru-lru-1core.txt", "-no-no-no-no-lru-lru-lru-srrip-drrip-lru-lru-lru-1core-cloudsuite.txt"]

#"DRAMBanksInterleaved-no",
pref_combination = [["ipcp_isca2020_FDP_PrefetchThrottler-no", "ipcp_isca2020_FDP_PrefetchThrottler-ipcp_isca2020", "no_FDP_PrefetchThrottler-spp", "no_FDP_PrefetchThrottler-ppf", "no_FDP_PrefetchThrottler-bingo_dpc3", "ipcp_isca2020_critical_FDP_PrefetchThrottler-no", "ipcp_isca2020_critical_FDP_PrefetchThrottler-ipcp_isca2020_critical", "no_FDP_PrefetchThrottler-spp_critical", "no_FDP_PrefetchThrottler-ppf_critical", "no_FDP_PrefetchThrottler-bingo_critical"], ["ipcp_isca2020_FDP_PrefetchThrottler-no", "ipcp_isca2020_FDP_PrefetchThrottler-ipcp_isca2020", "no_FDP_PrefetchThrottler-spp", "no_FDP_PrefetchThrottler-ppf", "no_FDP_PrefetchThrottler-bingo_dpc3", "ipcp_isca2020_critical_FDP_PrefetchThrottler-no", "ipcp_isca2020_critical_FDP_PrefetchThrottler-ipcp_isca2020_critical", "no_FDP_PrefetchThrottler-spp_critical", "no_FDP_PrefetchThrottler-ppf_critical", "no_FDP_PrefetchThrottler-bingo_critical"], ["ipcp_isca2020_FDP_PrefetchThrottler-no", "ipcp_isca2020_FDP_PrefetchThrottler-ipcp_isca2020", "no_FDP_PrefetchThrottler-spp", "no_FDP_PrefetchThrottler-ppf", "no_FDP_PrefetchThrottler-bingo_dpc3", "ipcp_isca2020_critical_FDP_PrefetchThrottler-no", "ipcp_isca2020_critical_FDP_PrefetchThrottler-ipcp_isca2020_critical", "no_FDP_PrefetchThrottler-spp_critical", "no_FDP_PrefetchThrottler-ppf_critical", "no_FDP_PrefetchThrottler-bingo_critical"]]

#[["ipcp_isca2020_critical_staticThresh_10-no", "ipcp_isca2020_critical_staticThresh_10-ipcp_isca2020_critical", "no_staticThresh_10-spp_critical", "no_staticThresh_10-ppf_critical", "no_staticThresh_10-bingo_critical", "ipcp_isca2020_critical_staticThresh_20-no", "ipcp_isca2020_critical_staticThresh_20-ipcp_isca2020_critical", "no_staticThresh_20-spp_critical", "no_staticThresh_20-ppf_critical", "no_staticThresh_20-bingo_critical", "ipcp_isca2020_critical_staticThresh_30-no", "ipcp_isca2020_critical_staticThresh_30-ipcp_isca2020_critical", "no_staticThresh_30-spp_critical", "no_staticThresh_30-ppf_critical", "no_staticThresh_30-bingo_critical", "ipcp_isca2020_critical_staticThresh_40-no", "ipcp_isca2020_critical_staticThresh_40-ipcp_isca2020_critical", "no_staticThresh_40-spp_critical", "no_staticThresh_40-ppf_critical", "no_staticThresh_40-bingo_critical", "ipcp_isca2020_critical_staticCritThresholds-no", "ipcp_isca2020_critical_staticCritThresholds-ipcp_isca2020_critical", "no_staticCritThresholds-spp_critical", "no_staticCritThresholds-ppf_critical", "no_staticCritThresholds-bingo_critical"], ["ipcp_isca2020_critical_staticThresh_10-no", "ipcp_isca2020_critical_staticThresh_10-ipcp_isca2020_critical", "no_staticThresh_10-spp_critical", "no_staticThresh_10-ppf_critical", "no_staticThresh_10-bingo_critical", "ipcp_isca2020_critical_staticThresh_20-no", "ipcp_isca2020_critical_staticThresh_20-ipcp_isca2020_critical", "no_staticThresh_20-spp_critical", "no_staticThresh_20-ppf_critical", "no_staticThresh_20-bingo_critical", "ipcp_isca2020_critical_staticThresh_30-no", "ipcp_isca2020_critical_staticThresh_30-ipcp_isca2020_critical", "no_staticThresh_30-spp_critical", "no_staticThresh_30-ppf_critical", "no_staticThresh_30-bingo_critical", "ipcp_isca2020_critical_staticThresh_40-no", "ipcp_isca2020_critical_staticThresh_40-ipcp_isca2020_critical", "no_staticThresh_40-spp_critical", "no_staticThresh_40-ppf_critical", "no_staticThresh_40-bingo_critical", "ipcp_isca2020_critical_staticCritThresholds-no", "ipcp_isca2020_critical_staticCritThresholds-ipcp_isca2020_critical", "no_staticCritThresholds-spp_critical", "no_staticCritThresholds-ppf_critical", "no_staticCritThresholds-bingo_critical"],
#                ["ipcp_isca2020_critical_staticThresh_10-no", "ipcp_isca2020_critical_staticThresh_10-ipcp_isca2020_critical", "no_staticThresh_10-spp_critical", "no_staticThresh_10-ppf_critical", "no_staticThresh_10-bingo_critical", "ipcp_isca2020_critical_staticThresh_20-no", "ipcp_isca2020_critical_staticThresh_20-ipcp_isca2020_critical", "no_staticThresh_20-spp_critical", "no_staticThresh_20-ppf_critical", "no_staticThresh_20-bingo_critical", "ipcp_isca2020_critical_staticThresh_30-no", "ipcp_isca2020_critical_staticThresh_30-ipcp_isca2020_critical", "no_staticThresh_30-spp_critical", "no_staticThresh_30-ppf_critical", "no_staticThresh_30-bingo_critical", "ipcp_isca2020_critical_staticThresh_40-no", "ipcp_isca2020_critical_staticThresh_40-ipcp_isca2020_critical", "no_staticThresh_40-spp_critical", "no_staticThresh_40-ppf_critical", "no_staticThresh_40-bingo_critical", "ipcp_isca2020_critical_staticCritThresholds-no", "ipcp_isca2020_critical_staticCritThresholds-ipcp_isca2020_critical", "no_staticCritThresholds-spp_critical", "no_staticCritThresholds-ppf_critical", "no_staticCritThresholds-bingo_critical"]]

#[["ipcp_isca2020_critical_HPCA2009_Updated-no", "ipcp_isca2020_critical_HPCA2009_Updated-ipcp_isca2020_critical", "no_HPCA2009_Updated-spp_critical", "no_HPCA2009_Updated-ppf_critical", "no_HPCA2009_Updated-bingo_critical", "ipcp_isca2020_critical_CATCH_All_Loads-no", "ipcp_isca2020_critical_CATCH_All_Loads-ipcp_isca2020_critical", "no_CATCH_All_Loads-spp_critical",  "no_CATCH_All_Loads-ppf_critical", "no_CATCH_All_Loads-bingo_critical"], ["ipcp_isca2020_critical_HPCA2009_Updated-no", "ipcp_isca2020_critical_HPCA2009_Updated-ipcp_isca2020_critical", "no_HPCA2009_Updated-spp_critical", "no_HPCA2009_Updated-ppf_critical", "no_HPCA2009_Updated-bingo_critical", "ipcp_isca2020_critical_CATCH_All_Loads-no", "ipcp_isca2020_critical_CATCH_All_Loads-ipcp_isca2020_critical", "no_CATCH_All_Loads-spp_critical",  "no_CATCH_All_Loads-ppf_critical", "no_CATCH_All_Loads-bingo_critical"], ["ipcp_isca2020_critical_HPCA2009_Updated-no", "ipcp_isca2020_critical_HPCA2009_Updated-ipcp_isca2020_critical", "no_HPCA2009_Updated-spp_critical", "no_HPCA2009_Updated-ppf_critical", "no_HPCA2009_Updated-bingo_critical", "ipcp_isca2020_critical_CATCH_All_Loads-no", "ipcp_isca2020_critical_CATCH_All_Loads-ipcp_isca2020_critical", "no_CATCH_All_Loads-spp_critical",  "no_CATCH_All_Loads-ppf_critical", "no_CATCH_All_Loads-bingo_critical"]]
    
#["no-no","no_updatedCritConditions-spp_critical", "no_updatedCritConditions-ppf_critical", "ipcp_isca2020_critical-64-no", "ipcp_isca2020_critical-64-ipcp_isca2020_critical", "no_updatedCritConditions-bingo_critical", "no_FVP_CD-spp_critical", "no_FVP_CD-ppf_critical", "ipcp_isca2020_critical_FVP_CD-no", "ipcp_isca2020_critical_FVP_CD-ipcp_isca2020_critical", "no_FVP_CD-bingo_critical", "no_HPCA2009-spp_critical", "no_HPCA2009-ppf_critical", "ipcp_isca2020_critical_HPCA2009-no", "ipcp_isca2020_critical_HPCA2009-ipcp_isca2020_critical", "no_HPCA2009-bingo_critical", "no_FocusedPrefetchingUpdated-spp_critical", "no_FocusedPrefetchingUpdated-ppf_critical", "ipcp_isca2020_critical_FocusedPrefetchingUpdated-no", "ipcp_isca2020_critical_FocusedPrefetchingUpdated-ipcp_isca2020_critical", "no_FocusedPrefetchingUpdated-bingo_critical"], ["no-no", "no_updatedCritConditions-spp_critical", "no_updatedCritConditions-ppf_critical", "ipcp_isca2020_critical-64-no", "ipcp_isca2020_critical-64-ipcp_isca2020_critical", "no_updatedCritConditions-bingo_critical", "no_FVP_CD-spp_critical", "no_FVP_CD-ppf_critical", "ipcp_isca2020_critical_FVP_CD-no", "ipcp_isca2020_critical_FVP_CD-ipcp_isca2020_critical", "no_FVP_CD-bingo_critical", "no_HPCA2009-spp_critical",
#                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     "no_HPCA2009-ppf_critical", "ipcp_isca2020_critical_HPCA2009-no", "ipcp_isca2020_critical_HPCA2009-ipcp_isca2020_critical", "no_HPCA2009-bingo_critical", "no_FocusedPrefetchingUpdated-spp_critical", "no_FocusedPrefetchingUpdated-ppf_critical", "ipcp_isca2020_critical_FocusedPrefetchingUpdated-no", "ipcp_isca2020_critical_FocusedPrefetchingUpdated-ipcp_isca2020_critical", "no_FocusedPrefetchingUpdated-bingo_critical"], ["no-no", "no_updatedCritConditions-spp_critical", "no_updatedCritConditions-ppf_critical", "ipcp_isca2020_critical-64-no", "ipcp_isca2020_critical-64-ipcp_isca2020_critical", "no_updatedCritConditions-bingo_critical", "no_FVP_CD-spp_critical", "no_FVP_CD-ppf_critical", "ipcp_isca2020_critical_FVP_CD-no", "ipcp_isca2020_critical_FVP_CD-ipcp_isca2020_critical", "no_FVP_CD-bingo_critical", "no_HPCA2009-spp_critical", "no_HPCA2009-ppf_critical", "ipcp_isca2020_critical_HPCA2009-no", "ipcp_isca2020_critical_HPCA2009-ipcp_isca2020_critical", "no_HPCA2009-bingo_critical", "no_FocusedPrefetchingUpdated-spp_critical", "no_FocusedPrefetchingUpdated-ppf_critical", "ipcp_isca2020_critical_FocusedPrefetchingUpdated-no", "ipcp_isca2020_critical_FocusedPrefetchingUpdated-ipcp_isca2020_critical", "no_FocusedPrefetchingUpdated-bingo_critical"]]

""" ["no-no", "ipcp_isca2020-64-no", "ipcp_isca2020-64-ipcp_isca2020", "no-bingo_dpc3", "no-spp", "no-ppf", "ipcp_isca2020_critical-64-no",
      "ipcp_isca2020_critical-64-ipcp_isca2020_critical", "no_updatedCritConditions-bingo_critical", "no_updatedCritConditions-spp_critical", "no_updatedCritConditions-ppf_critical"],
                 ["DRAMBanksInterleaved-no", "ipcp_isca2020-64-no", "ipcp_isca2020-64-ipcp_isca2020", "no-bingo_dpc3_124KB", "no-spp_PrefFillLevelStats", "no-ppf_PrefFillLevelStats", "ipcp_isca2020_critical-64-no",
                  "ipcp_isca2020_critical-64-ipcp_isca2020_critical", "no_updatedCritConditions-bingo_critical", "no_updatedCritConditions-spp_critical", "no_updatedCritConditions-ppf_critical"],
                 ["no-no", "ipcp_isca2020-64-no", "ipcp_isca2020-64-ipcp_isca2020", "no-bingo_dpc3", "no-spp", "no-ppf", "ipcp_isca2020_critical-64-no", "ipcp_isca2020_critical-64-ipcp_isca2020_critical", "no_updatedCritConditions-bingo_critical", "no_updatedCritConditions-spp_critical", "no_updatedCritConditions-ppf_critical"]] """
# pref_combination = [["DRAMBanksInterleaved-no",
# "ipcp_isca2020_DRAMBanksInterleaved-no",
# "ipcp_RRFilterStatsUpdated-ipcp_isca2020", "no-bingo_dpc3_124KB",
# "no-spp_PrefFillLevelStats", "no-ppf_PrefFillLevelStats",
# "ipcp_critical_energy-no", "ipcp_critical_energy-ipcp_critical",
# "no-bingo_critical_124KB", "no-spp_critical", "no-ppf_critical"],
# ["DRAMBanksInterleaved-no", "ipcp_isca2020_DRAMBanksInterleaved-no",
# "ipcp_RRFilterStatsUpdated-ipcp_isca2020", "no-bingo_dpc3_124KB",
# "no-spp_PrefFillLevelStats", "no-ppf_PrefFillLevelStats",
# "ipcp_critical_energy-no", "ipcp_critical_energy-ipcp_critical",
# "no-bingo_critical_124KB", "no-spp_critical", "no-ppf_critical"],
# ["no-no", "ipcp_isca2020-no", "ipcp_isca2020-ipcp_isca2020",
# "no-bingo_dpc3", "no-spp", "no-ppf", "ipcp_critical-no",
# "ipcp_critical-ipcp_critical", "no-bingo_critical", "no-spp_critical",
# "no-ppf_critical"]]


#[ "bingo_dpc3_writeCountUpdated-no", "no-bingo_dpc3_writeCountUpdated", "ipcp_RRFilterStatsUpdated-ipcp_isca2020", "bingo_critical-no", "no-bingo_critical"]
         #"DRAMBanksInterleaved-no",
         #"ipcp_critical_energy-no",
         #             "bingo_critical-no", "no-spp_critical", "no-ppf_critical",
         #             "no-bingo_critical", "ipcp_critical_energy-ipcp_critical"]
#["no-no", "ipcp_isca2020_256entr-no", "bingo_dpc3-no","no-spp", "no-ppf"];

column_range_end = "T"

# column_names = [
#    "", "", "", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "AA", "AB", "AC", "AD", "AE", "AF", "AG", "AH", "AI", "AJ", "AK", "AL", "AM", "AN", "AO", "AP", "AQ", "AR", "AS", "AT", "AU", "AV", "AW", "AX", "AY", "AZ", "BA", "BB", "BC", "BD", "BE", "BF", "BG", "BH", "BI", "BJ", "BK", "BL", "BM", "BN", "BO", "BP", "BQ", "BR", "BS", "BT", "BU", "BV", "BW", "BX", "BY", "BZ", "CA", "CB", "CC", "CD", "CE", "CF", "CG", "CH", "CI", "CJ", "CK", "CL", "CM", "CN", "CO", "CP", "CQ", "CR", "CS", "CT", "CU", "CV", "CW", "CX", "CY", "CZ", "DA", "DB", "DC", "DD", "DE", "DF", "DG", "DH", "DI", "DJ", "DK", "DL", "DM", "DN", "DO", "DP", "DQ", "DR", "DS", "DT", "DU", "DV", "DW", "DX", "DY", "DZ", "EA", "EB", "EC", "ED", "EE", "DE", "EG", "EH", "EI", "EJ", "EK", "EL", "EM", "EN", "EO", "EP", "EQ", "ER", "ES", "ET", "EU", "EV", "EW", "EX", "EY", "EZ", "FA", "FB", "FC", "FD", "FE", "FF", "FG", "FH", "FI", "FJ", "FK", "FL", "FM", "FN", "FO", "FP", "FQ", "FR", "FS", "FT", "FU", "FV", "FW", "FX", "FY", "FZ", "GA", "GB", "GC", "GD", "GE", "GF", "GG", "GH", "GI", "GJ", "GK", "GL", "GM", "GN", "GO", "GP", "GQ", "GR", "GS", "GT", "GU", "GV", "GW", "GX", "GY", "GZ", "HA", "HB", "HC", "HD", "HE", "HF", "HG", "HH", "HI", "HJ", "HK", "HL", "HM", "HN", "HO", "HP", "HQ", "HR", "HS", "HT", "HU", "HV", "HW", "HX", "HY", "HZ", ]

a_to_z = ["", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",
          "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"]

column_names = ["", "", "", "", "BF", "BG", "BH", "BI", "BJ", "BK", "BL", "BM", "BN","BO", "BP", "BQ", "BR"]

#["", "", "", "", "AG", "AH", "AI", "AJ", "AK", "AL", "AM", "AN", "AO", "AP", "AQ", "AR", "AS", "AT", "AU", "AV", "AW", "AX", "AY", "AZ", "BA","BB", "BC", "BD", "BE", "BF", "BG", "BH"]

#["", "", "", "W", "X", "Y", "Z", "AA", "AB", "AC", "AD", "AE", "AF", "AG", "AH", "AI", "AJ", "AK", "AL", "AM", "AN", "AO", "AP", "AQ", "AR", "AS", "AT", "AU", "AV", "AW", "AX", "AY", "AZ"] 

energy_row_number = ["240", "283", "332"]

access_row_number = ["106", "149", "198"]

# access_row_number = ["189", "232"]

prefetches_row_number = ["281", "324"]
# server, spec17, spec06


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
        for line in myfile:
            if re.search("CPU 0 cumulative IPC:", line):
                output.append(
                    (float(access_list[access_list_index]) * 1000) / float(line.split(" ")[8]))
                # 6 for APKI and 8 for APKC
                if(trace.find("605.mcf_s-484B") != -1):
                    print(
                        "APKC: " + str((float(access_list[access_list_index]) * 1000) / float(line.split(" ")[8])))
                break
        access_list_index = access_list_index + 1

    return output


def get_execution_cycles(trace_names, pref_name, index):
    output = []
    for trace in trace_names:
        filename = results_directory[index] + trace + branch_pred + l1i_pref[
            index] + pref_name + after_pref[index]
        result_file = codecs.open(
            filename, "r", encoding="utf-8", errors='ignore')

        file_lines = result_file.readlines()

        for line in file_lines:
            if re.search("CPU 0 cumulative IPC:", line):
                output.append(line.split(" ")[8])
    return output



# If modifying these scopes, delete the file token.pickle.
SCOPES = ['https://www.googleapis.com/auth/spreadsheets']
# The ID and range of a sample spreadsheet.
# Enter your sheet ID here
sheet_id_file = open("ENTER_SHEET_ID_HERE", "r")
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
            for pref_combination[iteration_count] in pref_combination[iteration_count]:

                pref_name = pref_combination[iteration_count]
                print(pref_combination[iteration_count])
                prefs_list = pref_combination[iteration_count].split("-")
                print(prefs_list)
                # pref_name = pref_name.strip()
                heading_name = ["MyData"]
                # column_name_index = 8 * inner_iter_count
                column_name_index = column_name_index + 1
                # column_name = column_names[column_name_index];


# -------------Execution Cycles-------------#

                execution_cycles = get_execution_cycles(
                    trace_names, pref_name, iteration_count)

                factor = 1 / 1000 * 4

# -------------- L1I cache ----------------#

                # unit: mW
                l1i_static_power_cacti = 2.93459
                # 15.7766

                l1i_static_energy = []

                loop_count = 0
                for stat in execution_cycles:
                    l1i_static_energy.append(
                        l1i_static_power_cacti * float(stat) * factor)

                l1i_static_energy.insert(0, "L1I Static Energy")


# -------------- ITLB ----------------#

                # unit: mW
                itlb_static_power_cacti = 0.02856

                itlb_static_energy = []

                loop_count = 0
                for stat in execution_cycles:
                    itlb_static_energy.append(
                        itlb_static_power_cacti * float(stat) * factor)

                itlb_static_energy.insert(0, "ITLB Static Energy")


# -------------- DTLB ----------------#

                # unit: mW
                dtlb_static_power_cacti = 0.02856
                # 0.0353538 + 0.0479449

                dtlb_static_energy = []

                loop_count = 0
                for stat in execution_cycles:
                    dtlb_static_energy.append(
                        dtlb_static_power_cacti * float(stat) * factor)

                dtlb_static_energy.insert(0, "DTLB Static Energy")


# -------------- STLB ----------------#

                # unit: mW
                stlb_static_power_cacti = (0.404364 + 0.797819) / 2

                stlb_static_energy = []

                loop_count = 0
                for stat in execution_cycles:
                    stlb_static_energy.append(
                        stlb_static_power_cacti * float(stat) * factor)

                stlb_static_energy.insert(0, "STLB Static Energy")



# -------------- L1D cache ----------------#

                # unit: mW
                l1d_static_power_cacti = (
                    ((23.2409 + 73.4569) / 2) + ((25.1134 + 75.3944) / 2)) / 2

                l1d_static_energy = []

                loop_count = 0
                for stat in execution_cycles:
                    l1d_static_energy.append(
                        l1d_static_power_cacti * float(stat) * factor)

                l1d_static_energy.insert(0, "L1D Static Energy")


# --------------- L2C cache --------------#

                # unit: mW
                l2c_static_power_cacti = 17.812

                l2c_static_energy = []

                loop_count = 0
                for stat in l2c_rfo_miss:
                    l2c_energy_stats[loop_count] = l2c_energy_stats[
                        loop_count] + (float(stat) * l2c_tag_read_energy_per_access)
                    loop_count = loop_count + 1 """

                print("L2C APKC")
                l2c_apki = get_apki(
                    trace_names, pref_name, iteration_count, l2c_total_access_stats)

                # *4 because 4 GHz processor, *1000 to convert it into mW.

                loop_index = 0
                l2c_power_gating = []
                for stat in execution_cycles:
                    if(float(l2c_apki[loop_index] > 85.81)):
                        l2c_static_energy.append(
                            l2c_static_power_cacti * float(stat) * factor * power_gating_high_apkc)
                        l2c_power_gating.append(power_gating_high_apkc)
                    elif(float(l2c_apki[loop_index] > 45.96)):
                        l2c_static_energy.append(
                            l2c_static_power_cacti * float(stat) * factor * power_gating_med_apkc)
                        l2c_power_gating.append(power_gating_med_apkc)
                    else:
                        l2c_static_energy.append(
                            l2c_static_power_cacti * float(stat) * factor * power_gating_low_apkc)
                        l2c_power_gating.append(power_gating_low_apkc)
                    loop_index = loop_index + 1

                l2c_static_energy.insert(0, "L2C Static Energy")


# --------------- LLC cache -------------#

                # unit: mW
                llc_static_power_cacti = 64.5777

                llc_static_energy = []

                # 6 for APKI, 8 for APKC in get_apki function
                print("LLC APKC")
                llc_apki = get_apki(
                    trace_names, pref_name, iteration_count, llc_total_access_stats)


                # *4 because 4 GHz processor, *1000 to convert it into mW.


                loop_index = 0
                for stat in execution_cycles:
                    power_gating_factor = (float(llc_apki[loop_index]) / float(
                        l2c_apki[loop_index])) * l2c_power_gating[loop_index]
                    if(power_gating_factor <= 0):
                        print("LLC power gating factor <= 0. Exiting.")
                        exit(0)
                    llc_static_energy.append(
                        llc_static_power_cacti * float(stat) * factor * power_gating_factor)

                    loop_index = loop_index + 1

                llc_static_energy.insert(0, "LLC Static Energy")


# ------------------ DRAM -------------#

                # unit: mW
                # Static energy from Leveraging Power-Performance Relationship
                # of Energy-Efficient Modern DRAM Devices
                dram_static_power = 320

                dram_static_energy = []

                dram_read_apki = get_apki(
                    trace_names, pref_name, iteration_count, dram_read_access_stats)

                dram_write_apki = get_apki(
                    trace_names, pref_name, iteration_count, dram_write_access_stats)

                dram_apki = []
                loop_index = 0
                for stat in dram_read_apki:
                    dram_apki.append(
                        float(stat) + float(dram_write_apki[loop_index]))
                    loop_index = loop_index + 1

                loop_index = 0
                for stat in execution_cycles:
                    if(trace_names[loop_index].find("server_027") != -1):
                        print("Pref: " + pref_name)
                        print("DRAM APKC: " + str(dram_apki[loop_index]))

                    if(float(dram_apki[loop_index] > 11.352)):
                        dram_static_energy.append(
                            dram_static_power * float(stat) * factor * power_gating_high_apkc)
                    elif(float(dram_apki[loop_index] > 2.894)):
                        dram_static_energy.append(
                            dram_static_power * float(stat) * factor * power_gating_med_apkc)
                    else:
                        dram_static_energy.append(
                            dram_static_power * float(stat) * factor * power_gating_low_apkc)
                    loop_index = loop_index + 1

                dram_static_energy.insert(0, "DRAM Static Energy")



# -------------------------------- CRITICALITY DETECTION -------------------------------------- #

                critical_static_power_cacti = 0.0394763 + 0.0203936
                critical_static_energy = ["Critical Static Energy"]
                if pref_name.find("critical") != -1:
                    loop_count = 0
                    for stat in execution_cycles:
                        critical_static_energy.append(
                            critical_static_power_cacti * float(stat) * factor)
                else:
                    for stat in execution_cycles:
                        critical_static_energy.append(0)

                execution_cycles_to_sheet = execution_cycles
                execution_cycles_to_sheet.insert(0, "Execution Cycles")

                # Call the Sheets API
                sheet = service.spreadsheets()

                total_energy_values = ["Total_energy"]

                count = 0
                for stat in l1i_static_energy:
                    if count > 0:
                        total_energy_values.append(0)
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in l1d_static_energy:
                    if count > 0:
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in l2c_static_energy:
                    if count > 0:
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in llc_static_energy:
                    if count > 0:
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in dram_static_energy:
                    if count > 0:
                        total_energy_values[
                            count] = total_energy_values[count] + float(stat)
                    count = count + 1

                count = 0
                for stat in itlb_energy_stats:
                    if count > 0:
                        total_energy_values[
                            count] = total_energy_values[count] + (float(stat) / 4)
                    count = count + 1

                count = 0
                for stat in dtlb_energy_stats:
                    if count > 0:
                        total_energy_values[
                            count] = total_energy_values[count] + (float(stat) / 4)
                    count = count + 1

                count = 0
                for stat in stlb_energy_stats:
                    if count > 0:
                        total_energy_values[
                            count] = total_energy_values[count] + (float(stat) / 4)
                    count = count + 1

                data = [  # {'range': "IPC_SERVER!"+column_name+"3",'values': ['a'], 'majorDimension': 'COLUMNS'},\
                    {'range': sheet_name + column_names[column_name_index] + "237", 'values': [[prefs_list[0]]], 'majorDimension': 'COLUMNS'},\
                    {'range': sheet_name + column_names[column_name_index] + "238", 'values': [[prefs_list[1]]], 'majorDimension': 'COLUMNS'},\
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
                prefetcher_static_power_cacti = 0

                if pref_flag == 1:
                    if prefs_list[len(prefs_list)-1].find("ipcp") != -1:
                        searchkeys = [
                            "L1 IP Table Write Accesses:", "L1 IP Table Tag Write Accesses:", "L1 IP Table Read Accesses:", "L1 IP Table Tag Read Accesses:", "L1 RST Write Accesses:", "L1 RST Read Accesses:", "L1 CSPT Write Accesses:",
                            "L1 CSPT Read Accesses:", "L1 RR Filter Tag Write Accesses:", "L1 RR Filter Read Accesses:", "L2 IP Table Read Accesses:", "L2 IP Table Write Accesses:", "L2 IP Table Tag Read Accesses:", "L2 IP Table Tag Write Accesses:"]
                        cacti_energy_values = [
                            0.000659022, 0.000352568, 0.00084576, 0.000423324,  0.00245553, 0.00236372,
                            0.000294389, 0.000275815, 0.00184435, 0.00188962, 0.000389184, 0.000433576, 0.000423324, 0.000352568]
                        prefetcher_static_power_cacti = (
                            0.0115763 + 0.000576971 + 0.0124575 + 0.00118746 + 0.00796589)
                    else:
                        searchkeys = [
                            "L1 IP Table Write Accesses:", "L1 IP Table Tag Write Accesses:", "L1 IP Table Read Accesses:", "L1 IP Table Tag Read Accesses:",
                            "L1 RST Write Accesses:", "L1 RST Read Accesses:", "L1 CSPT Write Accesses:", "L1 CSPT Read Accesses:", "L1 RR Filter Tag Write Accesses:", "L1 RR Filter Read Accesses:"]
                        cacti_energy_values = [
                            0.000659022, 0.000352568, 0.00084576, 0.000423324,
                            0.00245553, 0.00236372, 0.000294389, 0.000275815, 0.00184435, 0.00188962]
                        prefetcher_static_power_cacti = (
                            0.0115763 + 0.000576971 + 0.0124575 + 0.00118746)

                elif pref_flag == 2:
                    searchkeys = [
                        "PHT read accesses:", "PHT tag read accesses:", "PHT write accesses:", "PHT tag write accesses:", "AT read accesses:", "AT tag read accesses:", "AT write accesses:", "AT tag write accesses:",
                        "FT read accesses:", "FT tag read accesses:", "FT write accesses:", "FT tag write accesses:", "PS read accesses:", "PS tag read accesses:", "PS write accesses:", "PS tag write accesses:"]
                    cacti_energy_values = [
                        0.0109411, 0.0170055, 0.00404168, 0.0488819, 0.00137556, 0.00301434, 0.00076232,
                        0.00540516, 0.00112223, 0.00309947, 0.000673832, 0.00554463, 0.00147997, 0.00082552, 0.00082552, 0.00743868]
                    prefetcher_static_power_cacti = 3.55293 + \
                        ((0.147961) / 2) + (
                            (0.120516) / 4) + ((0.170218) / 2)

                elif pref_flag == 3:
                    searchkeys = [
                        "ST read accesses:", "ST write accesses:", "PT read accesses:",
                        "PT write accesses:", "FILTER read accesses:", "FILTER write accesses:"]
                    cacti_energy_values = [
                        0.01553197, 0.01591901, 0.0019875, 0.00100337, 0.00084576, 0.000659022]
                    prefetcher_static_power_cacti = 0.0283017 + \
                        0.201848 + 0.0610473

                elif pref_flag == 4:
                    searchkeys = [
                        "ST read accesses:", "ST write accesses:", "PT read accesses:", "PT write accesses:",
                        "FILTER read accesses:", "FILTER write accesses:", "PERC read accesses:", "PERC write accesses:"]
                    cacti_energy_values = [
                        0.01553197, 0.01591901, 0.0019875, 0.00100337, 0.00084576, 0.000659022, 0.00164583, 0.000964569]
                    prefetcher_static_power_cacti = 0.0283017 + 0.201848 + 0.0610473 + \
                        (4 * (0.259676)) + (2 * 0.123671) + (
                            2 * 0.0610473) + (1 * 0.00884707)

                execution_cycles.pop(0)

                if pref_flag != 0:
                    if(prefetcher_static_power_cacti == 0):
                        print("Prefetcher static power is zero. check again.")
                        exit(0)
                    prefetcher_static_energy = []

                    loop_count = 0

                    loop_index = 0
                    for stat in execution_cycles:
                        if pref_name.find("ipcp") == -1:
                            prefetcher_static_energy.append(
                                prefetcher_static_power_cacti * float(stat) * factor * l2c_power_gating[loop_index])
                        else:
                            prefetcher_static_energy.append(
                                prefetcher_static_power_cacti * float(stat) * factor)
                        loop_index = loop_index + 1

                    prefetcher_static_energy.insert(
                        0, "Prefetcher Static Energy")

                    count = 0
                    for stat in prefetcher_static_energy:
                        if count > 0:
                            total_energy_values[
                                count] = total_energy_values[count] + float(stat)
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
