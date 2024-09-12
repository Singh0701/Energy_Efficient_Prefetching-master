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
send_data = [1, 1, 1];  #server, spec17, cloudsuite

results_directory = ["../../IPC_SOTA_results_50M/", "../../SPEC2017_SOTA_results_100M/", "../../CLOUDSUITE_SOTA_results_50M/"]; 

branch_pred = "-hashed_perceptron";

l1i_pref = ["-fnlmma_25KB-", "-no-", "-no-"];

trace_list_files = ["../selected_ipc_trace_list.txt", "../selected_intensive_trace_list.txt", "../selected_cloudsuite_trace_list.txt"];

sheet_names = ["ENTER_SHEET_NAME_HERE", "ENTER_SHEET_NAME_HERE", "ENTER_SHEET_NAME_HERE"];

after_pref = ["-no-no-no-no-lru-lru-lru-srrip-drrip-lru-lru-lru-1core.txt",
                      "-no-no-no-no-lru-lru-lru-srrip-drrip-lru-lru-lru-1core.txt", "-no-no-no-no-lru-lru-lru-srrip-drrip-lru-lru-lru-1core-cloudsuite.txt"]

pref_combinations = ["ipcp_isca2020_FDP_PrefetchThrottler-no", "ipcp_isca2020_FDP_PrefetchThrottler-ipcp_isca2020", "no_FDP_PrefetchThrottler-spp", "no_FDP_PrefetchThrottler-ppf", "no_FDP_PrefetchThrottler-bingo_dpc3", "ipcp_isca2020_critical_FDP_PrefetchThrottler-no", "ipcp_isca2020_critical_FDP_PrefetchThrottler-ipcp_isca2020_critical", "no_FDP_PrefetchThrottler-spp_critical", "no_FDP_PrefetchThrottler-ppf_critical", "no_FDP_PrefetchThrottler-bingo_critical"]

row_number = [47, 54, 14];
row_number_addition = [46, 49, 9];

column_names = [["CX","CY", "CZ", "DA", "DB", "DC", "DD", "DE", "DF", "DG", "DH", "DI"], ["ER","ES","ET", "EU", "EV", "EW", "EX", "EY", "EZ", "FA", "FB"], ["CV", "CW","CX","CY", "CZ", "DA", "DB", "DC", "DD", "DE", "DF", "DG", "DH", "DI"]] 


def get_ipc(trace_names, pref_name, index):
	output = []
	for trace in trace_names:
		filename = results_directory[index]+trace+branch_pred+l1i_pref[index]+pref_name+after_pref[index];
		my_file = codecs.open(filename, "r", encoding="utf-8", errors='ignore')
		for line in my_file:
			if re.search("CPU 0 cumulative IPC:", line):
				output.append(line.split(" ")[4])
				break
	return output	

	
def get_coverage(trace_names, pref_name, cache_level, index):
    output = []

    prefs_list = pref_name.split("-");
    
    search_key="";
    
    if cache_level == 1:
        if prefs_list[0] == "no":
            return output;
        search_key = "L1D LOAD      ACCESS:";
    elif cache_level == 2:
        if prefs_list[1] == "no":
            return output;
        search_key = "L2C LOAD      ACCESS:";
    else:
        print("Invalid cache level in get_coverage: I only provide prefetcher coverage for L1D and L2C. If you want more, modify me!");
        exit(0);

    for trace in trace_names:
        filename = results_directory[index]+trace+branch_pred+l1i_pref[index]+pref_name+after_pref[index];
        basefilename = results_directory[index]+trace+branch_pred+l1i_pref[index]+"-no-no"+after_pref[index];
        my_file = codecs.open(filename, "r", encoding="utf-8", errors='ignore')
        pref_misses = 0
        for line in my_file:
                if re.search(search_key, line):
                    line = line.split()
                    cnt = 0
                    for word in line:
                            if word.isdigit():
                                    cnt = cnt + 1
                            if cnt == 3:
                                    pref_misses = float(word)
                                    break
                    break
        my_file = codecs.open(filename, "r", encoding="utf-8", errors='ignore')
        base_misses = 0
        for line in my_file:
                if re.search(search_key, line):
                    line = line.split()
                    cnt = 0
                    for word in line:
                            if word.isdigit():
                                    cnt = cnt + 1
                            if cnt == 3:
                                    base_misses = float(word)
                                    break
                    break
        if(base_misses != 0):
            output.append((1-(pref_misses/base_misses))*100)
        else:
            output.append(0)
        #output.append(pref_misses)
    return output	

def get_accuracy(trace_names, pref_name, cache_level, index):
    output = []
   
    prefs_list = pref_name.split("-");

    search_key="";
    if cache_level == 1:
        if prefs_list[0] == "no":
            return output;
        search_key = "L1D USEFUL LOAD PREFETCHES:";
    elif cache_level == 2:
        if prefs_list[1] == "no":
            return output;
        search_key = "L2C USEFUL LOAD PREFETCHES:";
    else:
        print("Invalid cache level in get_accuracy: I only provide prefetcher accuracy for L1D and L2C. If you want more, modify me!");
        exit(0); 

    for trace in trace_names:
        filename = results_directory[index]+trace+branch_pred+l1i_pref[index]+pref_name+after_pref[index];
        my_file = codecs.open(filename, "r", encoding="utf-8", errors='ignore')
        for line in my_file:
                if re.search(search_key, line):
                        output.append(line.split(" ")[-1].strip())
                        break
    return output	

def get_timeliness(trace_names, pref_name, cache_level, index):
    output = []

    prefs_list = pref_name.split("-");

    search_key="";

    if cache_level == 1:
        if prefs_list[0] == "no":
            return output;
        search_key = "L1D TIMELY PREFETCHES:";
    elif cache_level == 2:
        if prefs_list[1] == "no":
            return output;
        search_key = "L2C TIMELY PREFETCHES:";
    else:
        print("Invalid cache level in get_timeliness: I only provide prefetcher timeliness for L1D and L2C. If you want more, modify me!");
        exit(0); 

    for trace in trace_names:
        filename = results_directory[index]+trace+branch_pred+l1i_pref[index]+pref_name+after_pref[index];
        my_file = codecs.open(filename, "r", encoding="utf-8", errors='ignore')
        for line in my_file:
                if re.search("L1D TIMELY PREFETCHES:", line):
                        line = line.split(" ")
                        cnt = 1
                        for word in line:
                                if word.isdigit():
                                        if cnt == 1:
                                                timely = float(word)
                                                cnt = 2
                                        else:
                                                late = float(word)
                                                break
                        if late == 0:
                            output.append(0);
                        else:
                            output.append((late/(timely+late))*100)
                        break
    return output	

# If modifying these scopes, delete the file token.pickle.
SCOPES = ['https://www.googleapis.com/auth/spreadsheets']
# The ID and range of a sample spreadsheet. 
#Neelu: Reading the sheet ID from a file.
sheet_id_file = open("ENTER_YOUR_SHEET_ID_HERE", "r")
sheet_id = sheet_id_file.readlines();
SAMPLE_SPREADSHEET_ID = sheet_id[0].strip() 
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

    #loop for pref and loop for trace_files. So I'm into outer loop for trace_files and inner loop for pref. 
    iteration_count = 0;
    for trace_list_file in trace_list_files:
        if send_data[iteration_count] == 1: 
            trace_names = []
            trace_file = open(trace_list_file,"r")
            for line in trace_file:
                trace_names.append(line.strip())

            inner_iter_count = 0;
            for pref_combination in pref_combinations:

                #pref_name = input("Enter prefetcher names (l1d-l2c): ")
                pref_name = pref_combination;
                prefs_list = pref_combination.split("-");
                #pref_name = pref_name.strip()	
                #heading_name = input("Enter heading name: ")
                #heading_name = heading_name.strip()
                heading_name = ["MyData"];
                #column_name = input("Enter column name: ")
                #column_name = column_name.strip().upper()
                #column_name = column_names[inner_iter_count];
                ipc_values = get_ipc(trace_names, pref_name, iteration_count)
                #ipc_values.insert(0, heading_name)
                
                #btb_mpki_values = get_btb_mpki(trace_names, pref_name)
                #btb_mpki_values.insert(0, heading_name)
                #l1i_mpki_values = get_l1i_mpki(trace_names, pref_name)
                #l1i_mpki_values.insert(0, heading_name)
                #itlb_mpki_values = get_itlb_mpki(trace_names, pref_name)
                #itlb_mpki_values.insert(0, heading_name)
                column_name_index = inner_iter_count;
                l1d_coverage_values = get_coverage(trace_names, pref_name, 1, iteration_count)
                l2c_coverage_values = get_coverage(trace_names, pref_name, 2, iteration_count)
                #coverage_values.insert(0, heading_name)
                l1d_timeliness_values = get_timeliness(trace_names, pref_name, 1, iteration_count)
                l2c_timeliness_values = get_timeliness(trace_names, pref_name, 2, iteration_count)
                #timeliness_values.insert(0, heading_name)
                l1d_accuracy_values = get_accuracy(trace_names, pref_name, 1, iteration_count)
                l2c_accuracy_values = get_accuracy(trace_names, pref_name, 2, iteration_count)

                #accuracy_values.insert(0, heading_name)
                # Call the Sheets API
                sheet = service.spreadsheets()
        
                data = [ #{'range': "IPC_SERVER!"+column_name+"3",'values': ['a'], 'majorDimension': 'COLUMNS'},\
                        {'range': sheet_names[iteration_count]+column_names[iteration_count][column_name_index]+"2",'values': [[prefs_list[0]]], 'majorDimension': 'COLUMNS'},\
                        {'range': sheet_names[iteration_count]+column_names[iteration_count][column_name_index]+"3",'values': [[prefs_list[1]]], 'majorDimension': 'COLUMNS'},\
                        {'range': sheet_names[iteration_count]+column_names[iteration_count][column_name_index]+str(row_number[iteration_count]+0*row_number_addition[iteration_count]),'values': [ipc_values], 'majorDimension': 'COLUMNS'},\
                        {'range': sheet_names[iteration_count]+column_names[iteration_count][column_name_index]+str(row_number[iteration_count]+1*row_number_addition[iteration_count]),'values':[l1d_accuracy_values],'majorDimension':'COLUMNS'},\
                        {'range': sheet_names[iteration_count]+column_names[iteration_count][column_name_index]+str(row_number[iteration_count]+2*row_number_addition[iteration_count]),'values':[l1d_coverage_values],'majorDimension':'COLUMNS'},\
                        {'range': sheet_names[iteration_count]+column_names[iteration_count][column_name_index]+str(row_number[iteration_count]+3*row_number_addition[iteration_count]),'values':[l1d_timeliness_values],'majorDimension':'COLUMNS'},\
                        {'range': sheet_names[iteration_count]+column_names[iteration_count][column_name_index]+str(row_number[iteration_count]+4*row_number_addition[iteration_count]),'values':[l2c_accuracy_values],'majorDimension':'COLUMNS'},\
                        {'range': sheet_names[iteration_count]+column_names[iteration_count][column_name_index]+str(row_number[iteration_count]+5*row_number_addition[iteration_count]),'values':[l2c_coverage_values],'majorDimension':'COLUMNS'},\
                        {'range': sheet_names[iteration_count]+column_names[iteration_count][column_name_index]+str(row_number[iteration_count]+6*row_number_addition[iteration_count]),'values':[l2c_timeliness_values],'majorDimension':'COLUMNS'},\
                    ]
                body = { 'valueInputOption':'USER_ENTERED','data':data}
                result = sheet.values().batchUpdate(spreadsheetId=SAMPLE_SPREADSHEET_ID, body=body).execute()
                print(' {0} cells updated.'.format(result.get('totalUpdatedCells')))   
                inner_iter_count = inner_iter_count + 1;
            
        iteration_count = iteration_count + 1;
if __name__ == '__main__':
    main()

