<p align="center">
  <h1 align="center"> Instruction Criticality based Energy-efficient Hardware Data Prefetching </h1>
  <p> This repository encloses critical instruction based data prefetching techniques: our proposal i.e., ROB occupancy-based critical IP detection with threshold tuning along with state-of-the-art critical IP detection proposals: Focused Value Prediction, CATCH, Focused Prefetching, Subramaniam et al.'s technique. We show that our technique is more efficient for reducing energy consumption due to prefetching. We also implement and compare our proposal with prefetcher throttler, i.e., Feedback Directed Prefetching. Please find more information in Instruction_Criticality_based_Energy_Efficient_Hardware_Data_Prefetching.pdf. </p>
</p>

<p align="center">
  <h1 align="center"> ChampSim and System Prerequisites </h1>
  <p> ChampSim is a trace-based simulator for a microarchitecture study. You can sign up to the public mailing list by sending an empty mail to champsim+subscribe@googlegroups.com. You can find more information about ChampSim in the ChampSim github repository: https://github.com/ChampSim/ChampSim. This repository has been tested on a system with Ubuntu 17.04 and gcc version 6.3.0. <p>
  <p> 
</p>

<p align="center">
  <h1 align="center"> Prefetchers </h1>
  We use four state-of-the-art prefetchers, at the cache levels mentioned below along with prefetcher file name in <i>ChampSim/prefetcher/</i> directory: <br>

  | Data Prefetcher | Cache Level | prefetcher file name | 
  | --------------- | ----------- | -------------------- |
  | IPCP at L1D (https://www.cse.iitk.ac.in/users/biswap/IPCP_ISCA20.pdf) | L1D | ipcp_isca2020.l1d_pref |
  | IPCP (https://www.cse.iitk.ac.in/users/biswap/IPCP_ISCA20.pdf) | L1D, L2 | ipcp_isca2020.l1d_pref, ipcp_isca2020.l2c_pref |
  | Bingo (https://ieeexplore.ieee.org/document/8675188) | L2 | bingo_dpc3.l2c_pref |
  | SPP (https://ieeexplore.ieee.org/document/7783763) | L2 | spp.l2c_pref | 
  | PPF (https://ieeexplore.ieee.org/abstract/document/8980306) | L2 | ppf.l2c_pref |

</p>

<p align="center">
  <h1 align="center"> Critical IP based Prefetching </h1>
  This repository contains our critical IP detector (ROB occupancy based) along with four existing critical IP detectors. They can be enabled in the <i>inc/criticality_detector_config.h</i> file in the ChampSim directory. Use the following macros to enable particular critical IP detector: 

  | Technique | Macro | 
  | --------  | ----- |
  | ROB Occupancy with Threshold Relaxation | <i>ROB_OCCUPANCY</i> | 
  | ROB Occupancy without Threshold Relaxation | <i>ROB_OCCUPANCY</i> and <i>THRESHOLD_RELAXATION</i> | 
  | Focused Prefetching (https://jilp.org/vol13/v13paper10.pdf) | <i>FOCUSED_PREF</i> |
  | Focused Value Prediction (https://ieeexplore.ieee.org/document/9138991) | <i>FVP</i> |
  | CATCH (https://www.cse.iitk.ac.in/users/sidrai/papers/rai_isca18.pdf) | <i>CATCH</i> | 
  | Subramaniam et al. (https://ieeexplore.ieee.org/document/4798280) | <i>HPCA2009</i> | 
<br> 

   The critical IP detection code relevant to each of these techniques can be found in the file <i>src/criticality_detection.h</i> in the ChampSim directory. To disable critical IP detection completely, disable the macro <i>DETECT_CRITICAL_IPS</i> in the <i>inc/criticality_detector_config.h</i> file in the ChampSim directory.

<br>

   Please note that to enable critical ip based prefetching, use the following prefetcher files: 
   <br> 

  | Data Prefetcher | prefetcher file name |
  | --------------- | -------------------- |
  | IPCP at L1D | ipcp_isca2020_critical.l1d_pref | 
  | IPCP | ipcp_isca2020_critical.l1d_pref, ipcp_isca2020_critical.l2c_pref |
  | Bingo | bingo_critical.l2c_pref | 
  | SPP | spp_critical.l2c_pref | 
  | PPF | ppf_critical.l2c_pref |

</p>

<p align="center">
  <h1 align="center"> Prefetcher Throttler </h1>
  We also implement FDP prefetcher throttler to compare it with our scheme. To enable it, use the macro <i>FDP_PREFETCH_THROTTLER</i> in the <i>inc/criticality_detector_config.h</i> file in the ChampSim directory. This can be used along with any of the critical IP detection schemes or without them. 

</p>

<p align="center">
  <h1 align="center"> PCACTI </h1>

  We use PCACTI to model on-chip energy consumption. Please refer to the README inside the pcacti_setup directory to know about its usage in detail and find the script to generate energy consumption stats for all on-chip structures.

</p>

<p align="center">
  <h1 align="center"> Traces, Results, and Plots </h1>
  We use three benchmark suites for evaluation: SPEC_CPU_2017, Client/Server traces from IPC1, and Cloudsuite. The traces used for evaluation are listed in the files  <i>scripts/selected_intensive_trace_list.txt</i>, <i>scripts/selected_ipc_trace_list.txt</i>, and <i>scripts/selected_cloudsuite_trace_list.txt</i>. 
  <br> 
  The results for critical IP based prefetching for all prefetchers and critical IP detectors can be found in the following directories in the ChampSim directory: <i>SPEC2017_results_100M</i>, <i>IPC_results_50M</i>, and <i>CLOUDSUITE_results_50M</i>. We do a warmup of 50 million for all simulations before capturing region of interest stats. 
  <br> 
  To build and run the configuration, please edit and use the following scripts: 
  <br>
  $ ./build_all.sh <br>
  $ ./execute_all.sh &
  <br>
  execute_all.sh executes execute_SPEC2017.sh, execute_SERVER.sh, and execute_cloudsuite.sh. 

  To be specified in the build_all.sh, execute_SPEC2017.sh, execute_SERVER.sh, and execute_cloudsuite.sh: <i>l1dpref</i> and corresponding <i>l2cpref</i>. 

  Please note that for IPC traces, we enable L1I prefetcher <i>fnlmma_25KB</i>.

  Ensure to update the following string, "./TRACE_DIRECTORY/", with the path to directory containing required traces in <i>execute_<benchmark_suite>.sh</i> files.

  On running these commands as they are, results for ROB occupancy based criticality with threshold relaxation will be generated (after updating TRACE_DIRECTORY). 

  <br> 
  The python scripts for generating plots in the Instruction_Criticality_based_Energy_Efficient_Hardware_Data_Prefetching.pdf can be found in the directory <i>cal_plots</i> inside the ChampSim directory. 
  <br>  

</p>

<p align="center">
  <h1 align="center"> Send results directly to Google sheets </h1>
  <p>
    Directory: scripts/SendToGoogleSheets/ <br>
    Before using these scripts, ensure the following: <br>
    -> credentials.json and token.pickle files must be present in the directory. For more information on how to set up Google sheets API for python, please visit: https://developers.google.com/sheets/api/quickstart/python <br>
    -> Ensure to replace strings "ENTER_YOUR_SHEET_ID_HERE" and "ENTER_SHEET_NAME_HERE" with sheet ID and sheet name. <br>
    -> Configure column_names to denote which columns should the data go to in the sheet. <br> 
    -> Optional: Configure row_number as per convenience. <br>
    -> Note that each script contains flags to enable or disable sending results for each benchmark suite. For example, if you are working only on SPEC CPU 2017 benchmarks, and not Cloudsuite, the scripts are still useful. <br>
    Find following scripts in this directory: <br>
    (1) send_performance_to_sheet.py: To send IPC, prefetcher coverage, prefetcher accuracy, and prefetcher lateness stats. <br>
    (2, 3) send_pcacti_7nm_FinFET_dynamic/static_energy_to_sheet: static/dynamic energy consumption based on pcacti 7nm FinFET technology. <br> 
  </p>
</p>



