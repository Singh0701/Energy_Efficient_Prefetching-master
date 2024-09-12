
#include "ooo_cpu.h"
#include "set.h"
#include <map>
#include <set>
#include <iterator>
#include <vector>
#include <algorithm>

#include "criticality_detection.h" 

// out-of-order core
O3_CPU ooo_cpu[NUM_CPUS]; 
uint64_t current_core_cycle[NUM_CPUS], stall_cycle[NUM_CPUS];
uint32_t SCHEDULING_LATENCY = 0, EXEC_LATENCY = 0, DECODE_LATENCY = 0;
uint8_t TRACE_ENDS_STOP = 0;
uint8_t UNIQUE_ASID[5];
int asid_index=0;

int reg_instruction_pointer = REG_INSTRUCTION_POINTER, reg_flags = REG_FLAGS, reg_stack_pointer = REG_STACK_POINTER;

//Neelu: Issue rate stats.... 
uint64_t issued_all = 0, issued_lt_all = 0, issued_lt_one = 0, rob_not_full_when_issued_lt_all = 0, rob_not_full_when_issued_lt_one = 0; 

uint64_t critical_ip_candidate_identification_operations = 0; 

#ifdef HPCA_2009_CRITICALITY
std::map<uint64_t, uint64_t> loads_in_rob[NUM_CPUS], reverse_loads_in_rob[NUM_CPUS];	//instr_id, destination
std::map<uint64_t, uint64_t> loads_direct_dependents[NUM_CPUS];	//instr_id, number_of_direct_dependents
extern uint64_t critical_dynamic_instances[NUM_CPUS];                                                                                                                  
extern std::set<uint64_t> unique_critical_ips[NUM_CPUS];
uint64_t multiple_destinations[NUM_CPUS] = {0}, load_to_same_dest[NUM_CPUS] = {0};  
//Average issue rate calculated every 1000 cycles i.e., 250 ns
#define ISSUE_RATE_COMPUTATION 1000
#define ISSUE_RATE_THRESHOLD 4 
uint64_t sum_issued[NUM_CPUS] = {0}, low_issue_rate[NUM_CPUS] = {0}, total_issue_rate_checked_count[NUM_CPUS] = {0};
uint64_t global_issue_rate[NUM_CPUS] = {0}; 
#endif

#ifdef FOCUSED_VALUE_PREDICTION_CRITICALITY_DETECTOR 
extern uint32_t fvp_dynamic_critical_instances[NUM_CPUS];                                                                                                   
extern std::set<uint64_t> fvp_unique_critical_ips[NUM_CPUS]; 
#endif

#define NUM_OF_INSTRUCTIONS_IN_EPOCH 1000000		//500000		//1000000

//Neelu: Thresholds_modification_factor denotes the number to multiply the threshold reductors with ex. 1 decrement = threshold+(-1)*5. 
int num_of_times_thresholds_modified = 0, thresholds_modification_factor = 0, last_dynamic_threshold_test_result = -1, epoch_id = 1, current_epoch_all_ip_prefetch = 1;
//last_dynamic_threshold_test_result = -1 means, test hasn't been conducted. 0 means no modification occured. 1 means thresholds were modified.  
//last_epoch_all_ip_prefetch = 1 means all ip prefetch, 0 means only critical ip prefetch. -1 means no prefetching at all.
float no_prefetch_epoch_ipc = 0, all_ip_prefetch_epoch_ipc = 0, critical_ip_prefetch_epoch_ipc = 0;
uint64_t last_epoch_completion_cycle = 0, last_epoch_completion_instruction = 0;


//RSI = ROB Stalling IP. MaC = Marked as Critical. 

map<uint64_t, uint64_t> RSI_occurence_freq_before_MaC, RSI_occurence_freq_after_MaC, RSI_stall_freq_before_MaC, RSI_stall_freq_after_MaC;  

uint32_t log2_training_table_num_sets = 0;

#if defined ROB_OCCUPANCY_BASED_CRITICALITY || defined FOCUSED_PREFETCHING_COUNTING_CLASSIFIER 
uint64_t max_PIST_entries[NUM_CPUS] = {0}, min_PIST_entries[NUM_CPUS] = {NUM_OF_CRITICAL_IP_TRAINING_TABLE_WAYS+1}, sum_PIST_entries[NUM_CPUS] = {0}, count_PIST_entries[NUM_CPUS] = {0};	//Per IP Stall Table (Training Table)

//Neelu: Critical IP Detection Thresholds.
uint32_t candidacy_threshold = CANDIDACY_THRESHOLD, min_stalls_confidence = 16, static_reliance_threshold = STATIC_RELIANCE_THRESHOLD;
#endif

uint64_t global_stall_metric_count[NUM_CPUS], rob_stall_count[NUM_CPUS], load_rob_stall_count[NUM_CPUS];
uint64_t max_global_stall_metric_count = 0;

//Neelu: Note that these IPs are LOADs.
set<uint64_t> critical_ips[NUM_CPUS], rob_stalling_load_ips[NUM_CPUS], unique_load_ips[NUM_CPUS];
vector<uint64_t> lower_threshold_critical_ips[NUM_CPUS];


map<uint64_t, uint64_t> freq_critical_ips[NUM_CPUS], freq_rob_stalling_load_ips[NUM_CPUS];

map<uint64_t, vector<uint64_t>> load_stalls_rob_occu, load_stalls_cycles;

//Neelu: Access stats for PIST and Critical IP Cache (cic)
uint64_t pist_tag_read_accesses[NUM_CPUS] = {0}, pist_read_accesses[NUM_CPUS] = {0}, pist_tag_write_accesses[NUM_CPUS] = {0}, pist_write_accesses[NUM_CPUS] = {0}; 

#ifdef SET_ASSOCIATIVE_CRITICAL_IP_CACHE
CRITICAL_IP_CACHE critical_ip_cache(CRITICAL_IP_CACHE_SETS, CRITICAL_IP_CACHE_WAYS);
#endif


//map<uint64_t, uint64_t> num_of_stall_cycles_per_ip[NUM_CPUS], num_of_stalls_per_ip[NUM_CPUS], num_of_max_stall_cycles_per_ip[NUM_CPUS], num_of_min_stall_cycles_per_ip[NUM_CPUS]; 
//Neelu: To get an idea of the contribution of loads in ROB head stalls.
uint64_t total_load_rob_stall_cycles[NUM_CPUS] = {0}, total_rob_stall_cycles[NUM_CPUS] = {0}; /*total_load_rob_stalls[NUM_CPUS] = {0}, total_rob_stalls[NUM_CPUS] = {0}, other_instances_of_stalling_ip_in_rob[NUM_CPUS] = {0}, max_instances_of_stalling_ip_in_rob[NUM_CPUS] = {0}, rob_occupancy_at_max_instances[NUM_CPUS] = {0};*/

uint64_t rob_occu_load_stall_begin[NUM_CPUS] = {0}, rob_occu_load_stall_end[NUM_CPUS] = {0}, rob_occu_critical_load_stall_begin[NUM_CPUS] = {0}, rob_occu_critical_load_stall_end[NUM_CPUS] = {0}, critical_load_rob_stall_count[NUM_CPUS] = {0}, min_rob_occu_load_stall_begin[NUM_CPUS] = {400}, max_rob_occu_load_stall_begin[NUM_CPUS] = {0}, min_rob_occu_load_stall_end[NUM_CPUS] = {400}, max_rob_occu_load_stall_end[NUM_CPUS] = {0}, min_rob_occu_critical_load_stall_begin[NUM_CPUS] = {400}, max_rob_occu_critical_load_stall_begin[NUM_CPUS] = {0}, min_rob_occu_critical_load_stall_end[NUM_CPUS] = {400}, max_rob_occu_critical_load_stall_end[NUM_CPUS] = {0};

uint64_t rob_load_stall_begin_bucket_count[NUM_CPUS][7] = {0}, rob_load_stall_end_bucket_count[NUM_CPUS][7] = {0}, rob_critical_load_stall_begin_bucket_count[NUM_CPUS][7] = {0}, rob_critical_load_stall_end_bucket_count[NUM_CPUS][7] = {0};

map<uint64_t, array<uint64_t, 7>> per_ip_rob_load_stall_end_buckets[NUM_CPUS];

map<uint64_t, uint64_t> per_ip_rob_stall_occupancy_sum[NUM_CPUS]; 

#if defined ROB_OCCUPANCY_BASED_CRITICALITY || defined FOCUSED_PREFETCHING_COUNTING_CLASSIFIER
extern map<uint64_t, struct per_ip_stall_table> per_ip_stall_count[NUM_CPUS][NUM_OF_CRITICAL_IP_TRAINING_TABLE_SETS];
#endif

void O3_CPU::initialize_core()
{
	global_stall_metric_count[cpu] = 0;
	total_rob_stall_cycles[cpu] = 0;
	rob_stall_count[cpu] = 0;
	load_rob_stall_count[cpu] = 0;
	total_load_rob_stall_cycles[cpu] = 0;

#if defined ROB_OCCUPANCY_BASED_CRITICALITY || defined FOCUSED_PREFETCHING_COUNTING_CLASSIFIER 

	for(int max_index = NUM_OF_CRITICAL_IP_TRAINING_TABLE_SETS - 1; max_index > 0; max_index >>= 1)
		log2_training_table_num_sets++; 


	cout << "NUM_OF_IPS_IN_CRITICAL_IP_TRAINING: " << NUM_OF_IPS_IN_CRITICAL_IP_TRAINING << endl;
	cout << "NUM_OF_CRITICAL_IP_TRAINING_TABLE_WAYS: " << NUM_OF_CRITICAL_IP_TRAINING_TABLE_WAYS << endl;
	cout << "NUM_OF_CRITICAL_IP_TRAINING_TABLE_SETS: " << NUM_OF_CRITICAL_IP_TRAINING_TABLE_SETS << endl;

	cout << "CRITICAL_IP_CACHE_SETS: " << CRITICAL_IP_CACHE_SETS << endl;
	cout << "CRITICAL_IP_CACHE_WAYS: " << CRITICAL_IP_CACHE_WAYS << endl;

#endif 

}


void O3_CPU::read_from_trace()
{
    // actual processors do not work like this but for easier implementation,
    // we read instruction traces and virtually add them in the ROB
    // note that these traces are not yet translated and fetched 

    uint8_t continue_reading = 1;
    uint32_t num_reads = 0;
    instrs_to_read_this_cycle = FETCH_WIDTH;
	
    // first, read PIN trace
    while (continue_reading) {

        size_t instr_size = knob_cloudsuite ? sizeof(cloudsuite_instr) : sizeof(input_instr);
	
        if (knob_cloudsuite) {
        
            if (!fread(&current_cloudsuite_instr, instr_size, 1, trace_file)) {
                // reached end of file for this trace
                //cout << "*** Reached end of trace for Core: " << cpu << " Repeating trace: " << trace_string << endl; 
		//TRACE_ENDS_STOP = 1; /*@Vasudha - STOP simulation once trace file ends*/
                // close the trace file and re-open it
                pclose(trace_file);
		//Neelu: Commenting return on trace file end, need to keep reading trace for equal number of instructions simulation for all cores in multi-core.
		//return; /*@Vasudha */
                trace_file = popen(gunzip_command, "r");
                if (trace_file == NULL) {
                    cerr << endl << "*** CANNOT REOPEN TRACE FILE: " << trace_string << " ***" << endl;
                    assert(0);
                }
            } else { // successfully read the trace

                // copy the instruction into the performance model's instruction format
                ooo_model_instr arch_instr;
                int num_reg_ops = 0, num_mem_ops = 0;

                arch_instr.instr_id = instr_unique_id;
                arch_instr.ip = current_cloudsuite_instr.ip;
                arch_instr.is_branch = current_cloudsuite_instr.is_branch;
                arch_instr.branch_taken = current_cloudsuite_instr.branch_taken;

                arch_instr.asid[0] = current_cloudsuite_instr.asid[0];
                arch_instr.asid[1] = current_cloudsuite_instr.asid[1];


		/*@Vasudha - find unique asid*/
		int flag=0;
		for (int i=0; i<asid_index; i++)
		{
			if(UNIQUE_ASID[i] == arch_instr.asid[0])
			{
				flag = 1;
				break;
			}
		}
		if(flag==0)
		{
			UNIQUE_ASID[asid_index] = arch_instr.asid[0];
			cout <<"UNIQUE_ASID[" << asid_index << "] = " << UNIQUE_ASID[asid_index] << endl; 
			asid_index++;
		}
                for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
                    arch_instr.destination_registers[i] = current_cloudsuite_instr.destination_registers[i];
                    arch_instr.destination_memory[i] = current_cloudsuite_instr.destination_memory[i];
                    arch_instr.destination_virtual_address[i] = current_cloudsuite_instr.destination_memory[i];

                    if (arch_instr.destination_registers[i])
                        num_reg_ops++;
                    if (arch_instr.destination_memory[i]) {
                        num_mem_ops++;

                        // update STA, this structure is required to execute store instructios properly without deadlock
                        if (num_mem_ops > 0) {
#ifdef SANITY_CHECK
                            if (STA[STA_tail] < UINT64_MAX) {
                                if (STA_head != STA_tail)
                                    assert(0);
                            }
#endif
                            STA[STA_tail] = instr_unique_id;
                            STA_tail++;

                            if (STA_tail == STA_SIZE)
                                STA_tail = 0;
                        }
                    }
                }

                for (int i=0; i<NUM_INSTR_SOURCES; i++) {
                    arch_instr.source_registers[i] = current_cloudsuite_instr.source_registers[i];
                    arch_instr.source_memory[i] = current_cloudsuite_instr.source_memory[i];
                    arch_instr.source_virtual_address[i] = current_cloudsuite_instr.source_memory[i];

                    if (arch_instr.source_registers[i])
                        num_reg_ops++;
                    if (arch_instr.source_memory[i])
                        num_mem_ops++;
                }

                arch_instr.num_reg_ops = num_reg_ops;
                arch_instr.num_mem_ops = num_mem_ops;
                if (num_mem_ops > 0) 
                    arch_instr.is_memory = 1;

//Neelu: Addition begin

		// add this instruction to the IFETCH_BUFFER
		if (IFETCH_BUFFER.occupancy < IFETCH_BUFFER.SIZE) {
			uint32_t ifetch_buffer_index = add_to_ifetch_buffer(&arch_instr);
			num_reads++;
							                      // handle branch prediction

			if (IFETCH_BUFFER.entry[ifetch_buffer_index].is_branch) {
				DP( if (warmup_complete[cpu]) {
						cout << "[BRANCH] instr_id: " << instr_unique_id << " ip: " << hex << arch_instr.ip << dec << " taken: " << +arch_instr.branch_taken << endl; });

			
				num_branch++;	
				// handle branch prediction & branch predictor update

				uint8_t branch_prediction = predict_branch(IFETCH_BUFFER.entry[ifetch_buffer_index].ip);
	    			if(IFETCH_BUFFER.entry[ifetch_buffer_index].branch_taken != branch_prediction)
	   			{
	       				branch_mispredictions++;
					total_rob_occupancy_at_branch_mispredict += ROB.occupancy;
		 			if(warmup_complete[cpu])
      					{
  						fetch_stall = 1;
						instrs_to_read_this_cycle = 0;
						IFETCH_BUFFER.entry[ifetch_buffer_index].branch_mispredicted = 1;
					}
	  			}
	    			else
				{
					//correct prediction
					if(branch_prediction == 1)
					{
						// if correctly predicted taken, then we can't fetch anymore instructions this cycle
		                                instrs_to_read_this_cycle = 0;
					}
				}

			last_branch_result(IFETCH_BUFFER.entry[ifetch_buffer_index].ip, IFETCH_BUFFER.entry[ifetch_buffer_index].branch_taken);
			}


				
//Neelu: Addition end

                    //if ((num_reads == FETCH_WIDTH) || (ROB.occupancy == ROB.SIZE))
                    if ((num_reads >= instrs_to_read_this_cycle) || (IFETCH_BUFFER.occupancy == IFETCH_BUFFER.SIZE))
                        continue_reading = 0;
                }
                instr_unique_id++;
            }
        }
	else
	  {
	   	/* context-switch code*/
		if(ooo_cpu[cpu].context_switch)
		{
			//cout << "context_switch = " << cpu <<" cycle="<<current_core_cycle[cpu]<<"ROB-occupancy = "<< ROB.occupancy <<  endl;
	               
			break;
		} 
		input_instr trace_read_instr;
            if (!fread(&trace_read_instr, instr_size, 1, trace_file)) {
                // reached end of file for this trace
                cout << "*** Reached end of trace for Core: " << cpu << " Repeating trace: " << trace_string << endl; 

                // close the trace file and re-open it
                pclose(trace_file);
                trace_file = popen(gunzip_command, "r");
                if (trace_file == NULL) {
                    cerr << endl << "*** CANNOT REOPEN TRACE FILE: " << trace_string << " ***" << endl;
                    assert(0);
                }
            } else { // successfully read the trace


		    if(instr_unique_id == 0)
		    {
			    current_instr = next_instr = trace_read_instr;
		    }
		    else
		    {
			    current_instr = next_instr;
			    next_instr = trace_read_instr;
		    }



                // copy the instruction into the performance model's instruction format
                ooo_model_instr arch_instr;
                int num_reg_ops = 0, num_mem_ops = 0;

                arch_instr.instr_id = instr_unique_id;
                arch_instr.ip = current_instr.ip;
                arch_instr.is_branch = current_instr.is_branch;
                arch_instr.branch_taken = current_instr.branch_taken;

                arch_instr.asid[0] = cpu;
                arch_instr.asid[1] = cpu;

		bool reads_sp = false;
		bool writes_sp = false;
		bool reads_flags = false;
		bool reads_ip = false;
		bool writes_ip = false;
		bool reads_other = false;

		////cout << arch_instr.instr_id <<" "<<arch_instr.ip<<" "<< arch_instr.is_branch<<" "<<arch_instr.branch_taken<<" "<<cpu<<endl;
                for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
                    arch_instr.destination_registers[i] = current_instr.destination_registers[i];
                    arch_instr.destination_memory[i] = current_instr.destination_memory[i];
                    arch_instr.destination_virtual_address[i] = current_instr.destination_memory[i];

		if(arch_instr.destination_registers[i] == reg_stack_pointer)
                writes_sp = true;
		else if(arch_instr.destination_registers[i] == reg_instruction_pointer)
                writes_ip = true;


                    if (arch_instr.destination_registers[i])
                        num_reg_ops++;
                    if (arch_instr.destination_memory[i]) {
                        num_mem_ops++;
                        // update STA, this structure is required to execute store instructios properly without deadlock
                        if (num_mem_ops > 0) {
#ifdef SANITY_CHECK
                            if (STA[STA_tail] < UINT64_MAX) {
                                if (STA_head != STA_tail)
                                    assert(0);
                            }
#endif
                            STA[STA_tail] = instr_unique_id;
                            STA_tail++;

                            if (STA_tail == STA_SIZE)
                                STA_tail = 0;
                        }
                    }
                }

                for (int i=0; i<NUM_INSTR_SOURCES; i++) {
                    arch_instr.source_registers[i] = current_instr.source_registers[i];
                    arch_instr.source_memory[i] = current_instr.source_memory[i];
                    arch_instr.source_virtual_address[i] = current_instr.source_memory[i];

			if(arch_instr.source_registers[i] == reg_stack_pointer)
				reads_sp = true;
			else if(arch_instr.source_registers[i] == reg_flags)
                reads_flags = true;
			else if(arch_instr.source_registers[i] == reg_instruction_pointer)
                reads_ip = true;
			else if(arch_instr.source_registers[i] != 0)
                reads_other = true;


                    if (arch_instr.source_registers[i])
                        num_reg_ops++;
                    if (arch_instr.source_memory[i])
                        num_mem_ops++;
                }

                arch_instr.num_reg_ops = num_reg_ops;
                arch_instr.num_mem_ops = num_mem_ops;
                if (num_mem_ops > 0) 
                    arch_instr.is_memory = 1;


		// determine what kind of branch this is, if any
		if(!reads_sp && !reads_flags && writes_ip && !reads_other)
		{
			// direct jump
			arch_instr.is_branch = 1; 
			arch_instr.branch_taken = 1;
			arch_instr.branch_type = BRANCH_DIRECT_JUMP;
		}
		else if(!reads_sp && !reads_flags && writes_ip && reads_other)
		{
			// indirect branch
			arch_instr.is_branch = 1; 
			arch_instr.branch_taken = 1;
			arch_instr.branch_type = BRANCH_INDIRECT;
		}
		else if(!reads_sp && reads_ip && !writes_sp && writes_ip && reads_flags && !reads_other)
		{
			// conditional branch
			arch_instr.is_branch = 1;
			arch_instr.branch_taken = arch_instr.branch_taken; // don't change this
			arch_instr.branch_type = BRANCH_CONDITIONAL;
		}
		else if(reads_sp && reads_ip && writes_sp && writes_ip && !reads_flags && !reads_other)
		{
			// direct call
			arch_instr.is_branch = 1;
    			arch_instr.branch_taken = 1;
    			arch_instr.branch_type = BRANCH_DIRECT_CALL;

  		}
                else if(reads_sp && reads_ip && writes_sp && writes_ip && !reads_flags && reads_other)
		{
			// indirect call
			arch_instr.is_branch = 1;
			arch_instr.branch_taken = 1;
			arch_instr.branch_type = BRANCH_INDIRECT_CALL;
		}
		else if(reads_sp && !reads_ip && writes_sp && writes_ip)
		{
			//return
			arch_instr.is_branch = 1;
			arch_instr.branch_taken = 1;
			arch_instr.branch_type = BRANCH_RETURN;
		}
		else if(writes_ip)
		{
			// some other branch type that doesn't fit the above categories
			arch_instr.is_branch = 1;
			arch_instr.branch_taken = arch_instr.branch_taken; // don't change this
			arch_instr.branch_type = BRANCH_OTHER;
		}

		total_branch_types[arch_instr.branch_type]++;

		if((arch_instr.is_branch == 1) && (arch_instr.branch_taken == 1))
		{
			arch_instr.branch_target = next_instr.ip;
		}


		//Neelu: Inserting instruction to IFETCH BUFFER

		// add this instruction to the IFETCH_BUFFER
		if (IFETCH_BUFFER.occupancy < IFETCH_BUFFER.SIZE) {
			uint32_t ifetch_buffer_index = add_to_ifetch_buffer(&arch_instr);
			num_reads++;
	
	 		// handle branch prediction

			if (IFETCH_BUFFER.entry[ifetch_buffer_index].is_branch) {

				DP( if (warmup_complete[cpu]) {
				cout << "[BRANCH] instr_id: " << instr_unique_id << " ip: " << hex << arch_instr.ip << dec << " taken: " << +arch_instr.branch_taken << endl; });
							                        num_branch++;
			// handle branch prediction & branch predictor update
			uint8_t branch_prediction = predict_branch(IFETCH_BUFFER.entry[ifetch_buffer_index].ip);
			uint64_t predicted_branch_target = IFETCH_BUFFER.entry[ifetch_buffer_index].branch_target;
			if(branch_prediction == 0)
			{
				predicted_branch_target = 0;
			}
									                        // call code prefetcher every time the branch predictor is used
			l1i_prefetcher_branch_operate(IFETCH_BUFFER.entry[ifetch_buffer_index].ip, IFETCH_BUFFER.entry[ifetch_buffer_index].branch_type, predicted_branch_target);


#ifndef PERFECT_BTB
			if(arch_instr.branch_taken)
			{
					uint32_t btb_set = BTB.get_set(arch_instr.ip >> 2);
					int btb_way = BTB.get_way(arch_instr.ip, btb_set);
					if(btb_way == BTB_WAY)
					{
						BTB.sim_miss[cpu][ arch_instr.branch_type - 1]++;
						BTB.sim_access[cpu][ arch_instr.branch_type - 1]++;
						if(warmup_complete[cpu])
						{
							fetch_stall = 1;
							instrs_to_read_this_cycle = 0;
							IFETCH_BUFFER.entry[ifetch_buffer_index].btb_miss = 1;
						}
					}
					else
					{
						if(BTB.block[btb_set][btb_way].data == IFETCH_BUFFER.entry[ifetch_buffer_index].branch_target)
						{
								BTB.sim_hit[cpu][ arch_instr. branch_type - 1]++;
								BTB.sim_access[cpu][ arch_instr.branch_type - 1]++;
								(BTB.*BTB.update_replacement_state)(cpu, btb_set, btb_way, arch_instr.ip, arch_instr.ip, 0, 0, 1);
						}
						else
						{
							BTB.sim_miss[cpu][ arch_instr.branch_type - 1]++;
							BTB.sim_access[cpu][ arch_instr.branch_type - 1]++;
							if(warmup_complete[cpu])
								{
									fetch_stall = 1;
									instrs_to_read_this_cycle = 0;
									IFETCH_BUFFER.entry[ifetch_buffer_index].btb_miss = 1;
								}
						}
					}
			}
			else
			{
				uint32_t btb_set = BTB.get_set(arch_instr.ip >> 2);
                int btb_way = BTB.get_way(arch_instr.ip, btb_set);

				if(btb_way != BTB_WAY)
				{
					BTB.sim_hit[cpu][ arch_instr. branch_type - 1]++;
                    BTB.sim_access[cpu][ arch_instr.branch_type - 1]++;
                    (BTB.*BTB.update_replacement_state)(cpu, btb_set, btb_way, arch_instr.ip, arch_instr.ip, 0, 0, 1);
				}
			}

#endif





			if(IFETCH_BUFFER.entry[ifetch_buffer_index].branch_taken != branch_prediction)
			{
				branch_mispredictions++;
				total_rob_occupancy_at_branch_mispredict += ROB.occupancy;
				if(warmup_complete[cpu])
				{
					fetch_stall = 1;
					instrs_to_read_this_cycle = 0;
					IFETCH_BUFFER.entry[ifetch_buffer_index].branch_mispredicted = 1;
				}
			}
			else
			{
				// correct prediction

				if(branch_prediction == 1)
				{
					// if correctly predicted taken, then we can't fetch anymore instructions this cycle
					instrs_to_read_this_cycle = 0;
				}
			}

			last_branch_result(IFETCH_BUFFER.entry[ifetch_buffer_index].ip, IFETCH_BUFFER.entry[ifetch_buffer_index].branch_taken);
			}


                    if ((num_reads >= instrs_to_read_this_cycle) || (IFETCH_BUFFER.occupancy == IFETCH_BUFFER.SIZE))
                        continue_reading = 0;
                }
                instr_unique_id++;
            }
        }
    }
}

uint32_t O3_CPU::add_to_rob(ooo_model_instr *arch_instr)
{
	
    uint32_t index = ROB.tail;

    // sanity check
    if (ROB.entry[index].instr_id != 0) {
        cerr << "[ROB_ERROR] " << __func__ << " is not empty index: " << index;
        cerr << " instr_id: " << ROB.entry[index].instr_id << endl;
        assert(0);
    }

    ROB.entry[index] = *arch_instr;
    ROB.entry[index].event_cycle = current_core_cycle[cpu];

    //Neelu: 
    ROB.entry[index].add_to_rob_timestamp = current_core_cycle[cpu];


    ROB.occupancy++;
    ROB.tail++;
    if (ROB.tail >= ROB.SIZE)
        ROB.tail = 0;

    //DP ( if (warmup_complete[cpu]) {
    //cout << "[ROB] " <<  __func__ << " instr_id: " << ROB.entry[index].instr_id;
    //cout << " ip: " << hex << ROB.entry[index].ip << dec;
    //cout << " head: " << ROB.head << " tail: " << ROB.tail << " occupancy: " << ROB.occupancy;
    //cout << " event: " << ROB.entry[index].event_cycle << " current: " << current_core_cycle[cpu] << endl; });

#ifdef SANITY_CHECK
    if (ROB.entry[index].ip == 0) {
        cerr << "[ROB_ERROR] " << __func__ << " ip is zero index: " << index;
        cerr << " instr_id: " << ROB.entry[index].instr_id << " ip: " << ROB.entry[index].ip << endl;
        assert(0);
    }
#endif


#ifdef HPCA_2009_CRITICALITY
	//Neelu: if load, add to loads_in_rob
	for (uint32_t i=0; i<NUM_INSTR_SOURCES; i++) 
	{                                                                                                                 
		if(ROB.entry[index].lq_index[i] != UINT32_MAX)                                                                                
		{ 	
			//This is a load, so capturing destination 
			for(uint32_t j = 0; j < NUM_INSTR_DESTINATIONS; j++)
			{
				if(ROB.entry[index].destination_registers[j])
				{
					if(loads_in_rob[cpu].find(ROB.entry[index].instr_id) != loads_in_rob[cpu].end())
						multiple_destinations[cpu]++;

					loads_in_rob[cpu][ROB.entry[index].instr_id] = ROB.entry[index].destination_registers[j];
				
					if(reverse_loads_in_rob[cpu].find(ROB.entry[index].destination_registers[j]) != reverse_loads_in_rob[cpu].end())
						load_to_same_dest[cpu]++;

					reverse_loads_in_rob[cpu][ROB.entry[index].destination_registers[j]] = ROB.entry[index].instr_id; 

				}	
			}	
		}
	}

	//check for dependents. 
	for (uint32_t i=0; i<NUM_INSTR_SOURCES; i++)                                                                                                                                {
		if(ROB.entry[index].source_registers[i])
		{
			if(reverse_loads_in_rob[cpu].find(ROB.entry[index].source_registers[i]) != reverse_loads_in_rob[cpu].end())
			{
				//increase dependent count 
				critical_ip_candidate_identification_operations++;	
				loads_direct_dependents[cpu][reverse_loads_in_rob[cpu][ROB.entry[index].source_registers[i]]]++; 
			}
		}
	}	
#endif 

    return index;
}


uint32_t O3_CPU::add_to_ifetch_buffer(ooo_model_instr *arch_instr)
{
	uint32_t index = IFETCH_BUFFER.tail;

	  if(IFETCH_BUFFER.entry[index].instr_id != 0)
	  {
		  cerr << "[IFETCH_BUFFER_ERROR] " << __func__ << " is not empty index: " << index;
		  cerr << " instr_id: " << IFETCH_BUFFER.entry[index].instr_id << endl;
		  assert(0);
	  }

	  IFETCH_BUFFER.entry[index] = *arch_instr;
	  IFETCH_BUFFER.entry[index].event_cycle = current_core_cycle[cpu];

	//Neelu: Instead of translating instructions magically, translating them as usual to pay ITLB access penalty. 
	IFETCH_BUFFER.entry[index].instruction_pa = 0;
        IFETCH_BUFFER.entry[index].translated = 0;
	IFETCH_BUFFER.entry[index].fetched = 0;

	IFETCH_BUFFER.occupancy++;
	IFETCH_BUFFER.tail++;

	if(IFETCH_BUFFER.tail >= IFETCH_BUFFER.SIZE)
	{
		IFETCH_BUFFER.tail = 0;
	}

      return index;
}

uint32_t O3_CPU::add_to_decode_buffer(ooo_model_instr *arch_instr)
{
	  uint32_t index = DECODE_BUFFER.tail;

	  if(DECODE_BUFFER.entry[index].instr_id != 0)
	  {
		  cerr << "[DECODE_BUFFER_ERROR] " << __func__ << " is not empty index: " << index;
		  cerr << " instr_id: " << IFETCH_BUFFER.entry[index].instr_id << endl;
		  assert(0);
	  }

	  DECODE_BUFFER.entry[index] = *arch_instr;
	  DECODE_BUFFER.entry[index].event_cycle = current_core_cycle[cpu];

	  DECODE_BUFFER.occupancy++;
	  DECODE_BUFFER.tail++;
	  if(DECODE_BUFFER.tail >= DECODE_BUFFER.SIZE)
	  {
		  DECODE_BUFFER.tail = 0;
	  }

	  return index;
}



uint32_t O3_CPU::check_rob(uint64_t instr_id)
{
    if ((ROB.head == ROB.tail) && ROB.occupancy == 0)
        return ROB.SIZE;

    if (ROB.head < ROB.tail) {
        for (uint32_t i=ROB.head; i<ROB.tail; i++) {
            if (ROB.entry[i].instr_id == instr_id) {
                //DP ( if (warmup_complete[ROB.cpu]) {
                //cout << "[ROB] " << __func__ << " same instr_id: " << ROB.entry[i].instr_id;
                //cout << " rob_index: " << i << endl; });
                return i;
            }
        }
    }
    else {
        for (uint32_t i=ROB.head; i<ROB.SIZE; i++) {
            if (ROB.entry[i].instr_id == instr_id) {
                //DP ( if (warmup_complete[cpu]) {
                //cout << "[ROB] " << __func__ << " same instr_id: " << ROB.entry[i].instr_id;
                //cout << " rob_index: " << i << endl; });
                return i;
            }
        }
        for (uint32_t i=0; i<ROB.tail; i++) {
            if (ROB.entry[i].instr_id == instr_id) {
                //DP ( if (warmup_complete[cpu]) {
                //cout << "[ROB] " << __func__ << " same instr_id: " << ROB.entry[i].instr_id;
                //cout << " rob_index: " << i << endl; });
                return i;
            }
        }
    }
    
    cerr << "[ROB_ERROR] " << __func__ << " does not have any matching index! " << " head="<<ROB.head<<" tail="<<ROB.tail<<endl;
    cerr << " instr_id: " << instr_id << endl;
    assert(0);

    return ROB.SIZE;
}

void O3_CPU::fetch_instruction()
{
    // TODO: can we model wrong path execusion?

	
  // if we had a branch mispredict, turn fetching back on after the branch mispredict penalty
  ////////cout <<"prev, fetch_stall="<<fetch_stall<<" "<<current_core_cycle[cpu]<<" "<<fetch_resume_cycle<<endl;
  if((fetch_stall == 1) && (current_core_cycle[cpu] >= fetch_resume_cycle) && (fetch_resume_cycle != 0))
    {
	   ////cout<<"fetch_resume at "<< fetch_resume_cycle<<" " ;
      fetch_stall = 0;
      fetch_resume_cycle = 0;
      ////cout <<"****noww, fetch_stall="<<fetch_stall<<" "<<current_core_cycle[cpu]<<" "<<fetch_resume_cycle<<endl;
    }


	if(IFETCH_BUFFER.occupancy == 0)
	{
		return;
	}

	//Neelu: From IFETCH buffer, requests will be sent to L1-I and it will take care of sending the translation request to ITLB as L1 caches are VIPT in this version. 

	uint32_t index = IFETCH_BUFFER.head;
	for(uint32_t i=0; i<IFETCH_BUFFER.SIZE; i++)
	{
		if(IFETCH_BUFFER.entry[index].ip == 0)
		{
			break;
		}

		if((IFETCH_BUFFER.entry[index].fetched == 0))
		{
			// add it to the L1-I's read queue
			PACKET fetch_packet;
			fetch_packet.instruction = 1;
			fetch_packet.is_data = 0;
			fetch_packet.fill_level = FILL_L1;
			fetch_packet.fill_l1i = 1;
			fetch_packet.cpu = cpu;
			fetch_packet.address = IFETCH_BUFFER.entry[index].ip >> 6;
			fetch_packet.full_addr = IFETCH_BUFFER.entry[index].ip;
			fetch_packet.full_virtual_address = IFETCH_BUFFER.entry[index].ip;
			fetch_packet.instr_id = 0;
			//Neelu: Is assigning rob_index = 0 going to cause any problems? 
			fetch_packet.rob_index = 0;
			fetch_packet.producer = 0;
			fetch_packet.ip = IFETCH_BUFFER.entry[index].ip;
			fetch_packet.type = LOAD;
			fetch_packet.asid[0] = 0;
			fetch_packet.asid[1] = 0;
			fetch_packet.event_cycle = current_core_cycle[cpu];

			int rq_index = L1I.add_rq(&fetch_packet);

			if(rq_index != -2)
			{
				// mark all instructions from this cache line as having been fetched
				for(uint32_t j=0; j<IFETCH_BUFFER.SIZE; j++)
				{
					if(((IFETCH_BUFFER.entry[j].ip)>>6) == ((IFETCH_BUFFER.entry[index].ip)>>6))
					{
						IFETCH_BUFFER.entry[j].translated = COMPLETED;
						IFETCH_BUFFER.entry[j].fetched = INFLIGHT;
					}
				}
			}
		}

		index++;
		if(index >= IFETCH_BUFFER.SIZE)
		{
			index = 0;
		}


		if(index == IFETCH_BUFFER.head)
		{
			break;
		}
	}	//Neelu: For loop ending. 

	//send to decode stage
	
	bool decode_full = false;
	for(uint32_t i=0; i<DECODE_WIDTH; i++)
	{
		if(decode_full)
		{
			break;
		}

		if(IFETCH_BUFFER.entry[IFETCH_BUFFER.head].ip == 0)
		{
			break;
		}

		if((IFETCH_BUFFER.entry[IFETCH_BUFFER.head].translated == COMPLETED) && (IFETCH_BUFFER.entry[IFETCH_BUFFER.head].fetched == COMPLETED))
		{
			if(DECODE_BUFFER.occupancy < DECODE_BUFFER.SIZE)
			{
				uint32_t decode_index = add_to_decode_buffer(&IFETCH_BUFFER.entry[IFETCH_BUFFER.head]);
				DECODE_BUFFER.entry[decode_index].event_cycle = 0;

				ooo_model_instr empty_entry;
				IFETCH_BUFFER.entry[IFETCH_BUFFER.head] = empty_entry;
				IFETCH_BUFFER.head++;
				if(IFETCH_BUFFER.head >= IFETCH_BUFFER.SIZE)
				{
					IFETCH_BUFFER.head = 0;
				}
				IFETCH_BUFFER.occupancy--;
			}
			else
			{
				decode_full = true;
			}

		}

  		index++;
		if(index >= IFETCH_BUFFER.SIZE)
		{
			index = 0;
		}
	}
} //Ending of fetch_instruction()


void O3_CPU::fill_btb(uint64_t trigger, uint64_t target)
{
	uint32_t btb_set = BTB.get_set(trigger >> 2);
	int btb_way = BTB.get_way(trigger, btb_set);
	
	if(btb_way == BTB_WAY)
	{
		btb_way = (BTB.*BTB.find_victim)(cpu, 0, btb_set, BTB.block[btb_set], trigger, trigger, 0);
		(BTB.*BTB.update_replacement_state)(cpu, btb_set, btb_way, trigger, trigger, BTB.block[btb_set][btb_way].address, 0, 0);
		BLOCK &entry = BTB.block[btb_set][btb_way];
		if(entry.valid == 0)
			entry.valid = 1;
		entry.dirty = 0;
		entry.tag = trigger;
		entry.address = trigger;
		entry.full_addr = trigger;
		entry.data = target;
		entry.ip = trigger;
		entry.cpu = cpu;
	}
	else
	{
		BTB.block[btb_set][btb_way].data = target;
	}			
}



void O3_CPU::decode_and_dispatch()
{
	  // dispatch DECODE_WIDTH instructions that have decoded into the ROB
	  uint32_t count_dispatches = 0;
	  //cout << "Called decode_and_dispatch: " << current_core_cycle[cpu] << endl;
	  
	  if(warmup_complete[cpu])
	  	issued_all++;
	  
	  for(uint32_t i=0; i<DECODE_BUFFER.SIZE; i++)
	  {
		  if(DECODE_BUFFER.entry[DECODE_BUFFER.head].ip == 0)
		  {
			  break;
		  }

		  if(((!warmup_complete[cpu]) && (ROB.occupancy < ROB.SIZE)) || ((DECODE_BUFFER.entry[DECODE_BUFFER.head].event_cycle != 0) && (DECODE_BUFFER.entry[DECODE_BUFFER.head].event_cycle < current_core_cycle[cpu]) && (ROB.occupancy < ROB.SIZE)))
		  {
			if(DECODE_BUFFER.entry[DECODE_BUFFER.head].btb_miss == 1 && DECODE_BUFFER.entry[DECODE_BUFFER.head].branch_mispredicted == 0)
		{
			uint8_t branch_type = DECODE_BUFFER.entry[DECODE_BUFFER.head].branch_type;
			if(branch_type == BRANCH_DIRECT_JUMP || branch_type == BRANCH_DIRECT_CALL || (branch_type == BRANCH_CONDITIONAL))
			{
				if(warmup_complete[cpu])
				{
					fetch_resume_cycle = current_core_cycle[cpu] + 1; //Resume fetch from next cycle.
				}
				DECODE_BUFFER.entry[DECODE_BUFFER.head].btb_miss = 0;
				//if(branch_type == BRANCH_CONDITIONAL)
				//assert(DECODE_BUFFER.entry[DECODE_BUFFER.head].ip + 4 != DECODE_BUFFER.entry[DECODE_BUFFER.head].branch_target);		
				fill_btb(DECODE_BUFFER.entry[DECODE_BUFFER.head].ip, DECODE_BUFFER.entry[DECODE_BUFFER.head].branch_target);		
			}
		}


			  // move this instruction to the ROB if there's space
			uint32_t rob_index = add_to_rob(&DECODE_BUFFER.entry[DECODE_BUFFER.head]);
			ROB.entry[rob_index].event_cycle = current_core_cycle[cpu];

			ooo_model_instr empty_entry;
			DECODE_BUFFER.entry[DECODE_BUFFER.head] = empty_entry;

			DECODE_BUFFER.head++;
			if(DECODE_BUFFER.head >= DECODE_BUFFER.SIZE)
			{
				DECODE_BUFFER.head = 0;
			}
			DECODE_BUFFER.occupancy--;

			count_dispatches++;
			if(count_dispatches >= DECODE_WIDTH)
			{
				break;
			}
		  }
		  else
		  {
			//Neelu: setting rob_full_on_add_to_rob_attempt flag
			if(warmup_complete[cpu])
			{
				if((DECODE_BUFFER.entry[DECODE_BUFFER.head].event_cycle != 0) && (DECODE_BUFFER.entry[DECODE_BUFFER.head].event_cycle < current_core_cycle[cpu]) && (ROB.occupancy == ROB.SIZE))
					DECODE_BUFFER.entry[DECODE_BUFFER.head].rob_full_on_add_to_rob_attempt = true; 
			}

			  break;
		  }
	  }

	  if(warmup_complete[cpu])
	  {
#ifdef HPCA_2009_CRITICALITY
		if(current_core_cycle[cpu] % ISSUE_RATE_COMPUTATION == 0)
		{
			critical_ip_candidate_identification_operations++;
			global_issue_rate[cpu] = float(sum_issued[cpu])/ISSUE_RATE_COMPUTATION;
			sum_issued[cpu] = 0;
		}
		sum_issued[cpu] += count_dispatches;
#endif 
		if((count_dispatches < DECODE_WIDTH) && (ROB.occupancy < ROB.SIZE))
			rob_not_full_when_issued_lt_all++;	
		if(count_dispatches < DECODE_WIDTH)
			issued_lt_all++;
		if(count_dispatches < 1)
			issued_lt_one++;
		if((count_dispatches < 1) && (ROB.occupancy < ROB.SIZE))
			rob_not_full_when_issued_lt_one++;
	  }

	  // make new instructions pay decode penalty if they miss in the decoded instruction cache
	uint32_t decode_index = DECODE_BUFFER.head;
      	uint32_t count_decodes = 0;
      	for(uint32_t i=0; i<DECODE_BUFFER.SIZE; i++)
	{
		if(DECODE_BUFFER.entry[DECODE_BUFFER.head].ip == 0)
		{
			break;
		}

  		if(DECODE_BUFFER.entry[decode_index].event_cycle == 0)
  		{
			// apply decode latency
	  		DECODE_BUFFER.entry[decode_index].event_cycle = current_core_cycle[cpu] + DECODE_LATENCY;
			count_decodes++;
		}

		if(decode_index == DECODE_BUFFER.tail)
		{
			break;
		}
		decode_index++;
		if(decode_index >= DECODE_BUFFER.SIZE)
		{
			decode_index = 0;
		}

		if(count_decodes >= DECODE_WIDTH)
		{
			break;
		}
	}
}//Ending of decode_and_dispatch()

int O3_CPU::prefetch_code_line(uint64_t pf_v_addr)
{
	  if(pf_v_addr == 0)
	  {
		  cerr << "Cannot prefetch code line 0x0 !!!" << endl;
		  assert(0);
	  }

	  L1I.pf_requested++;

	  if (L1I.PQ.occupancy < L1I.PQ.SIZE)
	  {
		  //Neelu: Cannot magically translate prefetches, need to get realistic and access the TLBs. 
		  // magically translate prefetches
		/*  uint64_t pf_pa = (va_to_pa(cpu, 0, pf_v_addr, pf_v_addr>>LOG2_PAGE_SIZE, 1) & (~((1 << LOG2_PAGE_SIZE) - 1))) | (pf_v_addr & ((1 << LOG2_PAGE_SIZE) - 1)); */

		  PACKET pf_packet;
		  pf_packet.instruction = 1; // this is a code prefetch
		  pf_packet.is_data = 0;
		  pf_packet.fill_level = FILL_L1;
		  pf_packet.fill_l1i = 1;
		  pf_packet.pf_origin_level = FILL_L1;
		  pf_packet.cpu = cpu;		 
		  //Neelu: assigning virtual addresses.  
		  //pf_packet.address = pf_pa >> LOG2_BLOCK_SIZE;
		  //pf_packet.full_addr = pf_pa;

		  pf_packet.address = pf_v_addr >> LOG2_BLOCK_SIZE;
		  pf_packet.full_addr = pf_v_addr;

		  //Neelu: Marking translated = 0
		  pf_packet.translated = 0;
		  pf_packet.full_physical_address = 0;

		  pf_packet.ip = pf_v_addr;
		  pf_packet.type = PREFETCH;
		  pf_packet.event_cycle = current_core_cycle[cpu];

		  L1I.add_pq(&pf_packet);
		  L1I.pf_issued++;
		
		  return 1;
	  }

	  return 0;
}



// TODO: When should we update ROB.schedule_event_cycle?
// I. Instruction is fetched
// II. Instruction is completed
// III. Instruction is retired
void O3_CPU::schedule_instruction()
{
    if ((ROB.head == ROB.tail) && ROB.occupancy == 0)
        return;

    // execution is out-of-order but we have an in-order scheduling algorithm to detect all RAW dependencies
    uint32_t limit = ROB.next_fetch[1];
    ////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
      //          //cout << "Entered schedule_instruction() with instr id:"<< ROB.entry[limit].instr_id << endl; });
    num_searched = 0;
    if (ROB.head < limit) {
        for (uint32_t i=ROB.head; i<limit; i++) { 
            if ((ROB.entry[i].fetched != COMPLETED) || (ROB.entry[i].event_cycle > current_core_cycle[cpu]) || (num_searched >= SCHEDULER_SIZE))
                return;

            if (ROB.entry[i].scheduled == 0)
                do_scheduling(i);

            num_searched++;
        }
    }
    else {
        for (uint32_t i=ROB.head; i<ROB.SIZE; i++) {
            if ((ROB.entry[i].fetched != COMPLETED) || (ROB.entry[i].event_cycle > current_core_cycle[cpu]) || (num_searched >= SCHEDULER_SIZE))
                return;

            if (ROB.entry[i].scheduled == 0)
                do_scheduling(i);

            num_searched++;
        }
        for (uint32_t i=0; i<limit; i++) { 
            if ((ROB.entry[i].fetched != COMPLETED) || (ROB.entry[i].event_cycle > current_core_cycle[cpu]) || (num_searched >= SCHEDULER_SIZE))
                return;

            if (ROB.entry[i].scheduled == 0)
                do_scheduling(i);

            num_searched++;
        }
    }
    ////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
      //          //cout << "Exiting schedule_instruction"<< endl; });
}

void O3_CPU::do_scheduling(uint32_t rob_index)
{
    ROB.entry[rob_index].reg_ready = 1; // reg_ready will be reset to 0 if there is RAW dependency 

    reg_dependency(rob_index);
    ROB.next_schedule = (rob_index == (ROB.SIZE - 1)) ? 0 : (rob_index + 1);

    if (ROB.entry[rob_index].is_memory)
        ROB.entry[rob_index].scheduled = INFLIGHT;
    else {
        ROB.entry[rob_index].scheduled = COMPLETED;

        // ADD LATENCY
        if (ROB.entry[rob_index].event_cycle < current_core_cycle[cpu])
            ROB.entry[rob_index].event_cycle = current_core_cycle[cpu] + SCHEDULING_LATENCY;
        else
            ROB.entry[rob_index].event_cycle += SCHEDULING_LATENCY;

        if (ROB.entry[rob_index].reg_ready) {

#ifdef SANITY_CHECK
            if (RTE1[RTE1_tail] < ROB_SIZE)
                assert(0);
#endif
            // remember this rob_index in the Ready-To-Execute array 1
            RTE1[RTE1_tail] = rob_index;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[RTE1] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id << " rob_index: " << rob_index << " is added to RTE1";
            //cout << " head: " << RTE1_head << " tail: " << RTE1_tail << endl; }); 

            RTE1_tail++;
            if (RTE1_tail == ROB_SIZE)
                RTE1_tail = 0;
        }
    }
}

void O3_CPU::reg_dependency(uint32_t rob_index)
{
    // print out source/destination registers
    /*DP (if (warmup_complete[cpu]) {
    for (uint32_t i=0; i<NUM_INSTR_SOURCES; i++) {
        if (ROB.entry[rob_index].source_registers[i]) {
            //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id << " is_memory: " << +ROB.entry[rob_index].is_memory;
            //cout << " load  reg_index: " << +ROB.entry[rob_index].source_registers[i] << endl;
        }
    }
    for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
        if (ROB.entry[rob_index].destination_registers[i]) {
            //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id << " is_memory: " << +ROB.entry[rob_index].is_memory;
            //cout << " store reg_index: " << +ROB.entry[rob_index].destination_registers[i] << endl;
        }
    } }); */

    // check RAW dependency
    int prior = rob_index - 1;
    if (prior < 0)
        prior = ROB.SIZE - 1;

    if (rob_index != ROB.head) {
        if ((int)ROB.head <= prior) {
            for (int i=prior; i>=(int)ROB.head; i--) if (ROB.entry[i].executed != COMPLETED) {
		for (uint32_t j=0; j<NUM_INSTR_SOURCES; j++) {
			if (ROB.entry[rob_index].source_registers[j] && (ROB.entry[rob_index].reg_RAW_checked[j] == 0))
				reg_RAW_dependency(i, rob_index, j);
		}
	    }
        } else {
            for (int i=prior; i>=0; i--) if (ROB.entry[i].executed != COMPLETED) {
		for (uint32_t j=0; j<NUM_INSTR_SOURCES; j++) {
			if (ROB.entry[rob_index].source_registers[j] && (ROB.entry[rob_index].reg_RAW_checked[j] == 0))
				reg_RAW_dependency(i, rob_index, j);
		}
	    }
            for (int i=ROB.SIZE-1; i>=(int)ROB.head; i--) if (ROB.entry[i].executed != COMPLETED) {
		for (uint32_t j=0; j<NUM_INSTR_SOURCES; j++) {
			if (ROB.entry[rob_index].source_registers[j] && (ROB.entry[rob_index].reg_RAW_checked[j] == 0))
				reg_RAW_dependency(i, rob_index, j);
		}
	    }
        }
    }
}

void O3_CPU::reg_RAW_dependency(uint32_t prior, uint32_t current, uint32_t source_index)
{
    for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
        if (ROB.entry[prior].destination_registers[i] == 0)
            continue;

        if (ROB.entry[prior].destination_registers[i] == ROB.entry[current].source_registers[source_index]) {

            // we need to mark this dependency in the ROB since the producer might not be added in the store queue yet
            ROB.entry[prior].registers_instrs_depend_on_me.insert (current);   // this load cannot be executed until the prior store gets executed
            ROB.entry[prior].registers_index_depend_on_me[source_index].insert (current);   // this load cannot be executed until the prior store gets executed
            ROB.entry[prior].reg_RAW_producer = 1;

            ROB.entry[current].reg_ready = 0;
            ROB.entry[current].producer_id = ROB.entry[prior].instr_id; 
            ROB.entry[current].num_reg_dependent++;
            ROB.entry[current].reg_RAW_checked[source_index] = 1;

            //DP (if(warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[current].instr_id << " is_memory: " << +ROB.entry[current].is_memory;
            //cout << " RAW reg_index: " << +ROB.entry[current].source_registers[source_index];
            //cout << " producer_id: " << ROB.entry[prior].instr_id << endl; });

            return;
        }
    }
}

void O3_CPU::execute_instruction()
{
    if ((ROB.head == ROB.tail) && ROB.occupancy == 0)
        return;

    // out-of-order execution for non-memory instructions
    // memory instructions are handled by memory_instruction()
    uint32_t exec_issued = 0, num_iteration = 0;

    while (exec_issued < EXEC_WIDTH) {
        if (RTE0[RTE0_head] < ROB_SIZE) {
            uint32_t exec_index = RTE0[RTE0_head];
            ////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
              //  //cout << "Entered execute_instruction() with  RTE0 with instr_id: "<< ROB.entry[exec_index].instr_id<< endl; });
            if (ROB.entry[exec_index].event_cycle <= current_core_cycle[cpu]) {
                do_execution(exec_index);

                RTE0[RTE0_head] = ROB_SIZE;
                RTE0_head++;
                if (RTE0_head == ROB_SIZE)
                    RTE0_head = 0;
                exec_issued++;
            }
        }
        else {
            ////DP (if (warmup_complete[cpu]) {
            ////cout << "[RTE0] is empty head: " << RTE0_head << " tail: " << RTE0_tail << endl; });
            break;
        }

        num_iteration++;
        if (num_iteration == (ROB_SIZE-1))
            break;
    }

    num_iteration = 0;
    while (exec_issued < EXEC_WIDTH) {
        if (RTE1[RTE1_head] < ROB_SIZE) {
            uint32_t exec_index = RTE1[RTE1_head];
           // //DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
             //   //cout << "Entered execute_instruction() with  RTE1 with instr_id: "<< ROB.entry[exec_index].instr_id<< endl; });
            if (ROB.entry[exec_index].event_cycle <= current_core_cycle[cpu]) {
                do_execution(exec_index);

                RTE1[RTE1_head] = ROB_SIZE;
                RTE1_head++;
                if (RTE1_head == ROB_SIZE)
                    RTE1_head = 0;
                exec_issued++;
            }
        }
        else {
            ////DP (if (warmup_complete[cpu]) {
            ////cout << "[RTE1] is empty head: " << RTE1_head << " tail: " << RTE1_tail << endl; });
            break;
        }

        num_iteration++;
        if (num_iteration == (ROB_SIZE-1))
            break;
    }
    ////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
      //          //cout << "Exit execute_instruction() "<< endl; });
}

void O3_CPU::do_execution(uint32_t rob_index)
{
    //if (ROB.entry[rob_index].reg_ready && (ROB.entry[rob_index].scheduled == COMPLETED) && (ROB.entry[rob_index].event_cycle <= current_core_cycle[cpu])) {

        ROB.entry[rob_index].executed = INFLIGHT;
	
	ROB.entry[rob_index].execution_inflight_timestamp = current_core_cycle[cpu];

        // ADD LATENCY
        if (ROB.entry[rob_index].event_cycle < current_core_cycle[cpu])
            ROB.entry[rob_index].event_cycle = current_core_cycle[cpu] + EXEC_LATENCY;
        else
            ROB.entry[rob_index].event_cycle += EXEC_LATENCY;

        inflight_reg_executions++;

        //DP (if (warmup_complete[cpu]) {
        //cout << "[ROB] " << __func__ << " non-memory instr_id: " << ROB.entry[rob_index].instr_id; 
        //cout << " event_cycle: " << ROB.entry[rob_index].event_cycle << endl;});
    //}
}

void O3_CPU::schedule_memory_instruction()
{
    if ((ROB.head == ROB.tail) && ROB.occupancy == 0)
        return;

    // execution is out-of-order but we have an in-order scheduling algorithm to detect all RAW dependencies
    uint32_t limit = ROB.next_schedule;
   // //DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
     //           //cout << "Entered schedule_memory_instruction() with  next_schedule instr_id: "<< ROB.entry[limit].instr_id<< endl; });
    num_searched = 0;
    if (ROB.head < limit) {
        for (uint32_t i=ROB.head; i<limit; i++) {

            if (ROB.entry[i].is_memory == 0)
                continue;

            if ((ROB.entry[i].fetched != COMPLETED) || (ROB.entry[i].event_cycle > current_core_cycle[cpu]) || (num_searched >= SCHEDULER_SIZE))
                break;

            if (ROB.entry[i].is_memory && ROB.entry[i].reg_ready && (ROB.entry[i].scheduled == INFLIGHT))
                do_memory_scheduling(i);
        }
    }
    else {
        for (uint32_t i=ROB.head; i<ROB.SIZE; i++) {

            if (ROB.entry[i].is_memory == 0)
                continue;

            if ((ROB.entry[i].fetched != COMPLETED) || (ROB.entry[i].event_cycle > current_core_cycle[cpu]) || (num_searched >= SCHEDULER_SIZE))
                break;

            if (ROB.entry[i].is_memory && ROB.entry[i].reg_ready && (ROB.entry[i].scheduled == INFLIGHT))
                do_memory_scheduling(i);
        }
        for (uint32_t i=0; i<limit; i++) {

            if (ROB.entry[i].is_memory == 0)
                continue;

            if ((ROB.entry[i].fetched != COMPLETED) || (ROB.entry[i].event_cycle > current_core_cycle[cpu]) || (num_searched >= SCHEDULER_SIZE))
                break;

            if (ROB.entry[i].is_memory && ROB.entry[i].reg_ready && (ROB.entry[i].scheduled == INFLIGHT))
                do_memory_scheduling(i);
        }
    }
   // //DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
     //           //cout << "Exit execute_instruction() "<< endl; });
}

void O3_CPU::execute_memory_instruction()
{
	////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
          //      //cout << "Entered execute_memory_instruction() "<< endl; });
    operate_lsq();
    operate_cache();
    ////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
      //          //cout << "Exit execute_memory_instruction() "<< endl; });
}

void O3_CPU::do_memory_scheduling(uint32_t rob_index)
{
    uint32_t not_available = check_and_add_lsq(rob_index);
    if (not_available == 0) {
        ROB.entry[rob_index].scheduled = COMPLETED;
        if (ROB.entry[rob_index].executed == 0) // it could be already set to COMPLETED due to store-to-load forwarding
            ROB.entry[rob_index].executed  = INFLIGHT;

        //DP (if (warmup_complete[cpu]) {
        //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id << " rob_index: " << rob_index;
        //cout << " scheduled all num_mem_ops: " << ROB.entry[rob_index].num_mem_ops << endl; });
    }

    num_searched++;
}

uint32_t O3_CPU::check_and_add_lsq(uint32_t rob_index) 
{
    uint32_t num_mem_ops = 0, num_added = 0;

    // load
    for (uint32_t i=0; i<NUM_INSTR_SOURCES; i++) {
        if (ROB.entry[rob_index].source_memory[i]) {
            num_mem_ops++;
            if (ROB.entry[rob_index].source_added[i])
                num_added++;
            else if (LQ.occupancy < LQ.SIZE) {
                add_load_queue(rob_index, i);
                num_added++;
            }
            else {
                //DP(if(warmup_complete[cpu]) {
                //cout << "[LQ] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id;
                //cout << " cannot be added in the load queue occupancy: " << LQ.occupancy << " cycle: " << current_core_cycle[cpu] << endl; });
            }
        }
    }

    // store
    for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
        if (ROB.entry[rob_index].destination_memory[i]) {
            num_mem_ops++;
            if (ROB.entry[rob_index].destination_added[i])
                num_added++;
            else if (SQ.occupancy < SQ.SIZE) {
                if (STA[STA_head] == ROB.entry[rob_index].instr_id) {
                    add_store_queue(rob_index, i);
                    num_added++;
                }
                //add_store_queue(rob_index, i);
                //num_added++;
            }
            else {
                //DP(if(warmup_complete[cpu]) {
                //cout << "[SQ] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id;
                //cout << " cannot be added in the store queue occupancy: " << SQ.occupancy << " cycle: " << current_core_cycle[cpu] << endl; });
            }
        }
    }

    if (num_added == num_mem_ops)
        return 0;

    uint32_t not_available = num_mem_ops - num_added;
    if (not_available > num_mem_ops) {
        cerr << "instr_id: " << ROB.entry[rob_index].instr_id << endl;
        assert(0);
    }

    return not_available;
}

void O3_CPU::add_load_queue(uint32_t rob_index, uint32_t data_index)
{
    // search for an empty slot 
    uint32_t lq_index = LQ.SIZE;
    for (uint32_t i=0; i<LQ.SIZE; i++) {
        if (LQ.entry[i].virtual_address == 0) {
            lq_index = i;
            break;
        }
    }

    // sanity check
    if (lq_index == LQ.SIZE) {
        cerr << "instr_id: " << ROB.entry[rob_index].instr_id << " no empty slot in the load queue!!!" << endl;
        assert(0);
    }

    sim_load_gen++;

    //Neelu: Marking offset as accessed by the IP
    	/*int accessed_offset = (ROB.entry[rob_index].source_memory[data_index] >> 6) & 0x3F;
	if(offsets_accessed_by_ip[cpu].find(ROB.entry[rob_index].ip) == offsets_accessed_by_ip[cpu].end())
	{
		//initialize bit vector
		for(int i = 0; i < 64; i++)
			offsets_accessed_by_ip[cpu][ROB.entry[rob_index].ip][i] = 0;
	}
	offsets_accessed_by_ip[cpu][ROB.entry[rob_index].ip][accessed_offset] = 1;*/

    // add it to the load queue
    ROB.entry[rob_index].lq_index[data_index] = lq_index;
    LQ.entry[lq_index].instr_id = ROB.entry[rob_index].instr_id;
    LQ.entry[lq_index].virtual_address = ROB.entry[rob_index].source_memory[data_index];
    LQ.entry[lq_index].ip = ROB.entry[rob_index].ip;
    LQ.entry[lq_index].data_index = data_index;
    LQ.entry[lq_index].rob_index = rob_index;
    LQ.entry[lq_index].asid[0] = ROB.entry[rob_index].asid[0];
    LQ.entry[lq_index].asid[1] = ROB.entry[rob_index].asid[1];
    LQ.entry[lq_index].event_cycle = current_core_cycle[cpu] + SCHEDULING_LATENCY;
    LQ.occupancy++;

    // check RAW dependency
    int prior = rob_index - 1;
    if (prior < 0)
        prior = ROB.SIZE - 1;

    if (rob_index != ROB.head) {
        if ((int)ROB.head <= prior) {
            for (int i=prior; i>=(int)ROB.head; i--) {
                if (LQ.entry[lq_index].producer_id != UINT64_MAX)
                    break;

                    mem_RAW_dependency(i, rob_index, data_index, lq_index);
            }
        }
        else {
            for (int i=prior; i>=0; i--) {
                if (LQ.entry[lq_index].producer_id != UINT64_MAX)
                    break;

                    mem_RAW_dependency(i, rob_index, data_index, lq_index);
            }
            for (int i=ROB.SIZE-1; i>=(int)ROB.head; i--) { 
                if (LQ.entry[lq_index].producer_id != UINT64_MAX)
                    break;

                    mem_RAW_dependency(i, rob_index, data_index, lq_index);
            }
        }
    }

    // check
    // 1) if store-to-load forwarding is possible
    // 2) if there is WAR that are not correctly executed
    uint32_t forwarding_index = SQ.SIZE;
    for (uint32_t i=0; i<SQ.SIZE; i++) {

        // skip empty slot
        if (SQ.entry[i].virtual_address == 0)
            continue;

        // forwarding should be done by the SQ entry that holds the same producer_id from RAW dependency check
        if (SQ.entry[i].virtual_address == LQ.entry[lq_index].virtual_address) { // store-to-load forwarding check

            // forwarding store is in the SQ
            if ((rob_index != ROB.head) && (LQ.entry[lq_index].producer_id == SQ.entry[i].instr_id)) { // RAW
                forwarding_index = i;
                break; // should be break
            }

            if ((LQ.entry[lq_index].producer_id == UINT64_MAX) && (LQ.entry[lq_index].instr_id <= SQ.entry[i].instr_id)) { // WAR 
                // a load is about to be added in the load queue and we found a store that is 
                // "logically later in the program order but already executed" => this is not correctly executed WAR
                // due to out-of-order execution, this case is possible, for example
                // 1) application is load intensive and load queue is full
                // 2) we have loads that can't be added in the load queue
                // 3) subsequent stores logically behind in the program order are added in the store queue first

                // thanks to the store buffer, data is not written back to the memory system until retirement
                // also due to in-order retirement, this "already executed store" cannot be retired until we finish the prior load instruction 
                // if we detect WAR when a load is added in the load queue, just let the load instruction to access the memory system
                // no need to mark any dependency because this is actually WAR not RAW

                // do not forward data from the store queue since this is WAR
                // just read correct data from data cache

                LQ.entry[lq_index].physical_address = 0;
                LQ.entry[lq_index].translated = 0;
                LQ.entry[lq_index].fetched = 0;
                
                //DP(if(warmup_complete[cpu]) {
                //cout << "[LQ] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << " reset fetched: " << +LQ.entry[lq_index].fetched;
                //cout << " to obey WAR store instr_id: " << SQ.entry[i].instr_id << " cycle: " << current_core_cycle[cpu] << endl; });
            }
        }
    }

    if (forwarding_index != SQ.SIZE) { // we have a store-to-load forwarding

        if ((SQ.entry[forwarding_index].fetched == COMPLETED) && (SQ.entry[forwarding_index].event_cycle <= current_core_cycle[cpu])) {
           
	    //@Vishal: count RAW forwarding
	    sim_RAW_hits++;

            //@Vishal: VIPT, translation is not required, Just mark the entry as fetched
            //LQ.entry[lq_index].physical_address = (SQ.entry[forwarding_index].physical_address & ~(uint64_t) ((1 << LOG2_BLOCK_SIZE) - 1)) | (LQ.entry[lq_index].virtual_address & ((1 << LOG2_BLOCK_SIZE) - 1));
            LQ.entry[lq_index].translated = COMPLETED;
            LQ.entry[lq_index].fetched = COMPLETED;

            uint32_t fwr_rob_index = LQ.entry[lq_index].rob_index;
            ROB.entry[fwr_rob_index].num_mem_ops--;
            ROB.entry[fwr_rob_index].event_cycle = current_core_cycle[cpu];
            if (ROB.entry[fwr_rob_index].num_mem_ops < 0) {
                cerr << "instr_id: " << ROB.entry[fwr_rob_index].instr_id << endl;
                assert(0);
            }
            if (ROB.entry[fwr_rob_index].num_mem_ops == 0)
                inflight_mem_executions++;

            //DP(if(warmup_complete[cpu]) {
            //cout << "[LQ] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << hex;
            //cout << " full_addr: " << LQ.entry[lq_index].physical_address << dec << " is forwarded by store instr_id: ";
            //cout << SQ.entry[forwarding_index].instr_id << " remain_num_ops: " << ROB.entry[fwr_rob_index].num_mem_ops << " cycle: " << current_core_cycle[cpu] << endl; });

            release_load_queue(lq_index);
        }
        else
            ; // store is not executed yet, forwarding will be handled by execute_store()
    }

    // succesfully added to the load queue
    ROB.entry[rob_index].source_added[data_index] = 1;

    if (LQ.entry[lq_index].virtual_address && (LQ.entry[lq_index].producer_id == UINT64_MAX)) { // not released and no forwarding
        RTL0[RTL0_tail] = lq_index;
        RTL0_tail++;
        if (RTL0_tail == LQ_SIZE)
            RTL0_tail = 0;

        //DP (if (warmup_complete[cpu]) {
        //cout << "[RTL0] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << " rob_index: " << LQ.entry[lq_index].rob_index << " is added to RTL0";
        //cout << " head: " << RTL0_head << " tail: " << RTL0_tail << endl; }); 
    }

    //DP(if(warmup_complete[cpu]) {
    //cout << "[LQ] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id;
    //cout << " is added in the LQ address: " << hex << LQ.entry[lq_index].virtual_address << dec << " translated: " << +LQ.entry[lq_index].translated;
    //cout << " fetched: " << +LQ.entry[lq_index].fetched << " index: " << lq_index << " occupancy: " << LQ.occupancy << " cycle: " << current_core_cycle[cpu] << endl; });
}

void O3_CPU::mem_RAW_dependency(uint32_t prior, uint32_t current, uint32_t data_index, uint32_t lq_index)
{
    for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
        if (ROB.entry[prior].destination_memory[i] == 0)
            continue;

        if (ROB.entry[prior].destination_memory[i] == ROB.entry[current].source_memory[data_index]) { //  store-to-load forwarding check

            // we need to mark this dependency in the ROB since the producer might not be added in the store queue yet
            ROB.entry[prior].memory_instrs_depend_on_me.insert (current);   // this load cannot be executed until the prior store gets executed
            ROB.entry[prior].is_producer = 1;
            LQ.entry[lq_index].producer_id = ROB.entry[prior].instr_id; 
            LQ.entry[lq_index].translated = INFLIGHT;

            //DP (if(warmup_complete[cpu]) {
            //cout << "[LQ] " << __func__ << " RAW producer instr_id: " << ROB.entry[prior].instr_id << " consumer_id: " << ROB.entry[current].instr_id << " lq_index: " << lq_index;
            //cout << hex << " address: " << ROB.entry[prior].destination_memory[i] << dec << endl; });

            return;
        }
    }
}

void O3_CPU::add_store_queue(uint32_t rob_index, uint32_t data_index)
{
    uint32_t sq_index = SQ.tail;
#ifdef SANITY_CHECK
    if (SQ.entry[sq_index].virtual_address)
        assert(0);
#endif

    /*
    // search for an empty slot 
    uint32_t sq_index = SQ.SIZE;
    for (uint32_t i=0; i<SQ.SIZE; i++) {
        if (SQ.entry[i].virtual_address == 0) {
            sq_index = i;
            break;
        }
    }

    // sanity check
    if (sq_index == SQ.SIZE) {
        cerr << "instr_id: " << ROB.entry[rob_index].instr_id << " no empty slot in the store queue!!!" << endl;
        assert(0);
    }
    */

    sim_store_gen++;

    // add it to the store queue
    ROB.entry[rob_index].sq_index[data_index] = sq_index;
    SQ.entry[sq_index].instr_id = ROB.entry[rob_index].instr_id;
    SQ.entry[sq_index].virtual_address = ROB.entry[rob_index].destination_memory[data_index];
    SQ.entry[sq_index].ip = ROB.entry[rob_index].ip;
    SQ.entry[sq_index].data_index = data_index;
    SQ.entry[sq_index].rob_index = rob_index;
    SQ.entry[sq_index].asid[0] = ROB.entry[rob_index].asid[0];
    SQ.entry[sq_index].asid[1] = ROB.entry[rob_index].asid[1];
    SQ.entry[sq_index].event_cycle = current_core_cycle[cpu] + SCHEDULING_LATENCY;

    SQ.occupancy++;
    SQ.tail++;
    if (SQ.tail == SQ.SIZE)
        SQ.tail = 0;

    // succesfully added to the store queue
    ROB.entry[rob_index].destination_added[data_index] = 1;
    
    STA[STA_head] = UINT64_MAX;
    STA_head++;
    if (STA_head == STA_SIZE)
        STA_head = 0;

    RTS0[RTS0_tail] = sq_index;
    RTS0_tail++;
    if (RTS0_tail == SQ_SIZE)
        RTS0_tail = 0;

    //DP(if(warmup_complete[cpu]) {
    //cout << "[SQ] " << __func__ << " instr_id: " << SQ.entry[sq_index].instr_id;
    //cout << " is added in the SQ translated: " << +SQ.entry[sq_index].translated << " fetched: " << +SQ.entry[sq_index].fetched << " is_producer: " << +ROB.entry[rob_index].is_producer;
    //cout << " cycle: " << current_core_cycle[cpu] << endl; });
}

void O3_CPU::operate_lsq()
{
    // handle store
    uint32_t store_issued = 0, num_iteration = 0;


    //@Vishal: VIPT Execute store without sending translation request to DTLB.
    while (store_issued < SQ_WIDTH) {
        if (RTS0[RTS0_head] < SQ_SIZE) {
            uint32_t sq_index = RTS0[RTS0_head];
            if (SQ.entry[sq_index].event_cycle <= current_core_cycle[cpu]) {
            	execute_store(SQ.entry[sq_index].rob_index, sq_index, SQ.entry[sq_index].data_index);

            	RTS0[RTS0_head] = SQ_SIZE;
                RTS0_head++;
                if (RTS0_head == SQ_SIZE)
                    RTS0_head = 0;

                store_issued++;
            }
        }
        else {
            ////DP (if (warmup_complete[cpu]) {
            ////cout << "[RTS0] is empty head: " << RTS0_head << " tail: " << RTS0_tail << endl; });
            break;
        }

        num_iteration++;
        if (num_iteration == (SQ_SIZE-1))
            break;
    }

    /*while (store_issued < SQ_WIDTH) {
        if (RTS0[RTS0_head] < SQ_SIZE) {
            uint32_t sq_index = RTS0[RTS0_head];
            if (SQ.entry[sq_index].event_cycle <= current_core_cycle[cpu]) {

                // add it to DTLB
                PACKET data_packet;

                data_packet.tlb_access = 1;
                data_packet.fill_level = FILL_L1;
                data_packet.cpu = cpu;
                data_packet.data_index = SQ.entry[sq_index].data_index;
                data_packet.sq_index = sq_index;
                if (knob_cloudsuite)
                    data_packet.address = ((SQ.entry[sq_index].virtual_address >> LOG2_PAGE_SIZE) << 9) | SQ.entry[sq_index].asid[1];
                else
                    data_packet.address = SQ.entry[sq_index].virtual_address >> LOG2_PAGE_SIZE;
                data_packet.full_addr = SQ.entry[sq_index].virtual_address;
                data_packet.instr_id = SQ.entry[sq_index].instr_id;
                data_packet.rob_index = SQ.entry[sq_index].rob_index;
                data_packet.ip = SQ.entry[sq_index].ip;
                data_packet.type = RFO;
                data_packet.asid[0] = SQ.entry[sq_index].asid[0];
                data_packet.asid[1] = SQ.entry[sq_index].asid[1];
                data_packet.event_cycle = SQ.entry[sq_index].event_cycle;

                //DP (if (warmup_complete[cpu]) {
                //cout << "[RTS0] " << __func__ << " instr_id: " << SQ.entry[sq_index].instr_id << " rob_index: " << SQ.entry[sq_index].rob_index << " is popped from to RTS0";
                //cout << " head: " << RTS0_head << " tail: " << RTS0_tail << endl; }); 

                int rq_index = DTLB.add_rq(&data_packet);

                if (rq_index == -2)
                    break; 
                else 
                    SQ.entry[sq_index].translated = INFLIGHT;

                RTS0[RTS0_head] = SQ_SIZE;
                RTS0_head++;
                if (RTS0_head == SQ_SIZE)
                    RTS0_head = 0;

                store_issued++;
            }
        }
        else {
            ////DP (if (warmup_complete[cpu]) {
            ////cout << "[RTS0] is empty head: " << RTS0_head << " tail: " << RTS0_tail << endl; });
            break;
        }

        num_iteration++;
        if (num_iteration == (SQ_SIZE-1))
            break;
    }

    num_iteration = 0;
    while (store_issued < SQ_WIDTH) {
        if (RTS1[RTS1_head] < SQ_SIZE) {
            uint32_t sq_index = RTS1[RTS1_head];
            if (SQ.entry[sq_index].event_cycle <= current_core_cycle[cpu]) {
                execute_store(SQ.entry[sq_index].rob_index, sq_index, SQ.entry[sq_index].data_index);

                RTS1[RTS1_head] = SQ_SIZE;
                RTS1_head++;
                if (RTS1_head == SQ_SIZE)
                    RTS1_head = 0;

                store_issued++;
            }
        }
        else {
            ////DP (if (warmup_complete[cpu]) {
            ////cout << "[RTS1] is empty head: " << RTS1_head << " tail: " << RTS1_tail << endl; });
            break;
        }

        num_iteration++;
        if (num_iteration == (SQ_SIZE-1))
            break;
    }*/

    unsigned load_issued = 0;
    num_iteration = 0;

    //@Vishal: VIPT. Send request to L1D.

    while (load_issued < LQ_WIDTH) {
        if (RTL0[RTL0_head] < LQ_SIZE) {
            uint32_t lq_index = RTL0[RTL0_head];
            if (LQ.entry[lq_index].event_cycle <= current_core_cycle[cpu]) {

            	int rq_index = execute_load(LQ.entry[lq_index].rob_index, lq_index, LQ.entry[lq_index].data_index);

            	if (rq_index != -2) {
                    RTL0[RTL0_head] = LQ_SIZE;
	                RTL0_head++;
	                if (RTL0_head == LQ_SIZE)
	                    RTL0_head = 0;

                    load_issued++;
                }
            }
        }
        else {
            ////DP (if (warmup_complete[cpu]) {
            ////cout << "[RTL1] is empty head: " << RTL1_head << " tail: " << RTL1_tail << endl; });
            break;
        }

        num_iteration++;
        if (num_iteration == (LQ_SIZE-1))
            break;
    }

    /*while (load_issued < LQ_WIDTH) {
        if (RTL0[RTL0_head] < LQ_SIZE) {
            uint32_t lq_index = RTL0[RTL0_head];
            if (LQ.entry[lq_index].event_cycle <= current_core_cycle[cpu]) {

                // add it to DTLB
                PACKET data_packet;
                data_packet.fill_level = FILL_L1;
                data_packet.cpu = cpu;
                data_packet.data_index = LQ.entry[lq_index].data_index;
                data_packet.lq_index = lq_index;
                if (knob_cloudsuite)
                    data_packet.address = ((LQ.entry[lq_index].virtual_address >> LOG2_PAGE_SIZE) << 9) | LQ.entry[lq_index].asid[1];
                else
                    data_packet.address = LQ.entry[lq_index].virtual_address >> LOG2_PAGE_SIZE;
                data_packet.full_addr = LQ.entry[lq_index].virtual_address;
                data_packet.instr_id = LQ.entry[lq_index].instr_id;
                data_packet.rob_index = LQ.entry[lq_index].rob_index;
                data_packet.ip = LQ.entry[lq_index].ip;
                data_packet.type = LOAD;
                data_packet.asid[0] = LQ.entry[lq_index].asid[0];
                data_packet.asid[1] = LQ.entry[lq_index].asid[1];
                data_packet.event_cycle = LQ.entry[lq_index].event_cycle;

                //DP (if (warmup_complete[cpu]) {
                //cout << "[RTL0] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << " rob_index: " << LQ.entry[lq_index].rob_index << " is popped to RTL0";
                //cout << " head: " << RTL0_head << " tail: " << RTL0_tail << endl; }); 

                int rq_index = DTLB.add_rq(&data_packet);

                if (rq_index == -2)
                    break; // break here
                else  
                    LQ.entry[lq_index].translated = INFLIGHT;

                RTL0[RTL0_head] = LQ_SIZE;
                RTL0_head++;
                if (RTL0_head == LQ_SIZE)
                    RTL0_head = 0;

                load_issued++;
            }
        }
        else {
            ////DP (if (warmup_complete[cpu]) {
            ////cout << "[RTL0] is empty head: " << RTL0_head << " tail: " << RTL0_tail << endl; });
            break;
        }

        num_iteration++;
        if (num_iteration == (LQ_SIZE-1))
            break;
    }

    num_iteration = 0;
    while (load_issued < LQ_WIDTH) {
        if (RTL1[RTL1_head] < LQ_SIZE) {
            uint32_t lq_index = RTL1[RTL1_head];
            if (LQ.entry[lq_index].event_cycle <= current_core_cycle[cpu]) {
                int rq_index = execute_load(LQ.entry[lq_index].rob_index, lq_index, LQ.entry[lq_index].data_index);

                if (rq_index != -2) {
                    RTL1[RTL1_head] = LQ_SIZE;
                    RTL1_head++;
                    if (RTL1_head == LQ_SIZE)
                        RTL1_head = 0;

                    load_issued++;
                }
            }
        }
        else {
            ////DP (if (warmup_complete[cpu]) {
            ////cout << "[RTL1] is empty head: " << RTL1_head << " tail: " << RTL1_tail << endl; });
            break;
        }

        num_iteration++;
        if (num_iteration == (LQ_SIZE-1))
            break;
    }*/
}

void O3_CPU::execute_store(uint32_t rob_index, uint32_t sq_index, uint32_t data_index)
{
    SQ.entry[sq_index].fetched = COMPLETED;
    SQ.entry[sq_index].event_cycle = current_core_cycle[cpu];
   
    ROB.entry[rob_index].num_mem_ops--;
    ROB.entry[rob_index].event_cycle = current_core_cycle[cpu];
    if (ROB.entry[rob_index].num_mem_ops < 0) {
        cerr << "instr_id: " << ROB.entry[rob_index].instr_id << endl;
        assert(0);
    }

    if (ROB.entry[rob_index].num_mem_ops == 0)	
    	inflight_mem_executions++;
    
       //DP (if (warmup_complete[cpu]) {
    //cout << "[SQ1] " << __func__ << " instr_id: " << SQ.entry[sq_index].instr_id << hex;
    //cout << " full_address: " << SQ.entry[sq_index].physical_address << dec << " remain_mem_ops: " << ROB.entry[rob_index].num_mem_ops;
    //cout << " event_cycle: " << SQ.entry[sq_index].event_cycle << endl; });

    // resolve RAW dependency after DTLB access
    // check if this store has dependent loads
    if (ROB.entry[rob_index].is_producer) {
	ITERATE_SET(dependent,ROB.entry[rob_index].memory_instrs_depend_on_me, ROB_SIZE) {
            // check if dependent loads are already added in the load queue
            for (uint32_t j=0; j<NUM_INSTR_SOURCES; j++) { // which one is dependent?
                if (ROB.entry[dependent].source_memory[j] && ROB.entry[dependent].source_added[j]) {
                    if (ROB.entry[dependent].source_memory[j] == SQ.entry[sq_index].virtual_address) { // this is required since a single instruction can issue multiple loads
			
                         //@Vishal: count RAW forwarding
			 sim_RAW_hits++;

                        // now we can resolve RAW dependency
                        uint32_t lq_index = ROB.entry[dependent].lq_index[j];
#ifdef SANITY_CHECK
                        if (lq_index >= LQ.SIZE)
                            assert(0);
                        if (LQ.entry[lq_index].producer_id != SQ.entry[sq_index].instr_id) {
                            cerr << "[SQ2] " << __func__ << " lq_index: " << lq_index << " producer_id: " << LQ.entry[lq_index].producer_id;
                            cerr << " does not match to the store instr_id: " << SQ.entry[sq_index].instr_id << endl;
                            assert(0);
                        }
#endif
                        // update correspodning LQ entry
                        // @Vishal: Dependent load can now get the data, translation is not required
                        //LQ.entry[lq_index].physical_address = (SQ.entry[sq_index].physical_address & ~(uint64_t) ((1 << LOG2_BLOCK_SIZE) - 1)) | (LQ.entry[lq_index].virtual_address & ((1 << LOG2_BLOCK_SIZE) - 1));
                        LQ.entry[lq_index].translated = COMPLETED;
                        LQ.entry[lq_index].fetched = COMPLETED;
                        LQ.entry[lq_index].event_cycle = current_core_cycle[cpu];

                        uint32_t fwr_rob_index = LQ.entry[lq_index].rob_index;
                        ROB.entry[fwr_rob_index].num_mem_ops--;
                        ROB.entry[fwr_rob_index].event_cycle = current_core_cycle[cpu];
#ifdef SANITY_CHECK
                        if (ROB.entry[fwr_rob_index].num_mem_ops < 0) {
                            cerr << "instr_id: " << ROB.entry[fwr_rob_index].instr_id << endl;
                            assert(0);
                        }
#endif
                        if (ROB.entry[fwr_rob_index].num_mem_ops == 0)
                            inflight_mem_executions++;

                        //DP(if(warmup_complete[cpu]) {
                        //cout << "[LQ3] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << hex;
                        //cout << " full_addr: " << LQ.entry[lq_index].physical_address << dec << " is forwarded by store instr_id: ";
                        //cout << SQ.entry[sq_index].instr_id << " remain_num_ops: " << ROB.entry[fwr_rob_index].num_mem_ops << " cycle: " << current_core_cycle[cpu] << endl; });

                        release_load_queue(lq_index);

                        // clear dependency bit
                        if (j == (NUM_INSTR_SOURCES-1))
                            ROB.entry[rob_index].memory_instrs_depend_on_me.insert (dependent);
                    }
                }
            }
        }
    }
}

int O3_CPU::execute_load(uint32_t rob_index, uint32_t lq_index, uint32_t data_index)
{
    // add it to L1D
    PACKET data_packet;
    data_packet.fill_level = FILL_L1;
    data_packet.fill_l1d = 1;
    data_packet.cpu = cpu;
    data_packet.data_index = LQ.entry[lq_index].data_index;
    data_packet.lq_index = lq_index;

    //@Vishal: VIPT send virtual address instead of physical address
    //data_packet.address = LQ.entry[lq_index].physical_address >> LOG2_BLOCK_SIZE;
    //data_packet.full_addr = LQ.entry[lq_index].physical_address;
    data_packet.address = LQ.entry[lq_index].virtual_address >> LOG2_BLOCK_SIZE;
    data_packet.full_addr = LQ.entry[lq_index].virtual_address;
    data_packet.full_virtual_address = LQ.entry[lq_index].virtual_address;

    data_packet.instr_id = LQ.entry[lq_index].instr_id;
    data_packet.rob_index = LQ.entry[lq_index].rob_index;
    data_packet.ip = LQ.entry[lq_index].ip;

    //Neelu: Setting critical ip flag in packet if ip is identified as critical
#ifdef DETECT_CRITICAL_IPS
if(warmup_complete[cpu])
{
#ifdef HPCA_2009_CRITICALITY
    	//if issue rate is low.
	//total_issue_rate_checked_count[cpu]++;
    	//if(global_issue_rate[cpu] < ISSUE_RATE_THRESHOLD)
	//{
	//	low_issue_rate[cpu]++;
		data_packet.critical_ip_flag = hpca_2009_check_critical(data_packet.ip, cpu); 
	//}
//else: Not HPCA_2009
#elif defined CATCH_CRITICALITY 
	data_packet.critical_ip_flag = catch_check_critical(data_packet.ip, cpu);
#elif defined FOCUSED_VALUE_PREDICTION_CRITICALITY_DETECTOR
	data_packet.critical_ip_flag = focused_value_prediction_check_critical(data_packet.ip, cpu);  
#else

    	#ifdef CRITICALITY_THRESHOLD_RELAXATION
 	if(current_epoch_all_ip_prefetch == -1)
	{
		//We don't want to prefetch at all in this epoch.
		data_packet.critical_ip_flag = 0;
	}
	else if(current_epoch_all_ip_prefetch == 1)
	{
		//We want to prefetch for all stalling ips in this epoch.
		data_packet.critical_ip_flag = 1;
		/*if(rob_stalling_load_ips[cpu].find(LQ.entry[lq_index].ip) != rob_stalling_load_ips[cpu].end())
			data_packet.critical_ip_flag = 1;
		else
			data_packet.critical_ip_flag = 0;*/
	}
	else if(current_epoch_all_ip_prefetch == 0 && warmup_complete[cpu])
	{
#ifdef SET_ASSOCIATIVE_CRITICAL_IP_CACHE
		if(critical_ip_cache.find(LQ.entry[lq_index].ip))
			data_packet.critical_ip_flag = 1;
		else
			data_packet.critical_ip_flag = 0;
#else		
		//Based on critical ips.
		uint8_t present_in_lower_threshold_critical_ips = 0;

		for(int i = 0; i < lower_threshold_critical_ips[cpu].size(); i++)
		{
			if(lower_threshold_critical_ips[cpu][i] == LQ.entry[lq_index].ip)
			{
				// Prefetch address is present in RR filter
				present_in_lower_threshold_critical_ips = 1;
			}
		}
	
		if(critical_ips[cpu].find(LQ.entry[lq_index].ip) != critical_ips[cpu].end() || present_in_lower_threshold_critical_ips)
			data_packet.critical_ip_flag = 1;
		else
			data_packet.critical_ip_flag = 0;
#endif //SET_ASSOC
	}
	else
		assert(!warmup_complete[cpu]);
	#else
#ifdef SET_ASSOCIATIVE_CRITICAL_IP_CACHE
                if(critical_ip_cache.find(LQ.entry[lq_index].ip))
                        data_packet.critical_ip_flag = 1;
                else
                        data_packet.critical_ip_flag = 0;
#else 
		if(critical_ips[cpu].find(LQ.entry[lq_index].ip) != critical_ips[cpu].end())
			data_packet.critical_ip_flag = 1;
		else
			data_packet.critical_ip_flag = 0;
#endif  //SET_ASSOC
#endif //DYNAMIC
#endif  //HPCA_2009
}
#endif //DETECT_CRITICAL

    data_packet.type = LOAD;
    data_packet.asid[0] = LQ.entry[lq_index].asid[0];
    data_packet.asid[1] = LQ.entry[lq_index].asid[1];
    data_packet.event_cycle = LQ.entry[lq_index].event_cycle;
    
    int rq_index = L1D.add_rq(&data_packet);

    if (rq_index == -2)
        return rq_index;
    else 
    {
        LQ.entry[lq_index].fetched = INFLIGHT;
	
	sim_load_sent++;
    }

    return rq_index;
}

void O3_CPU::complete_execution(uint32_t rob_index)
{
	////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
          //      //cout << "Entered complete_execution() "<< endl; });
    if (ROB.entry[rob_index].is_memory == 0) {
        if ((ROB.entry[rob_index].executed == INFLIGHT) && (ROB.entry[rob_index].event_cycle <= current_core_cycle[cpu])) {

            ROB.entry[rob_index].executed = COMPLETED;
	    ROB.entry[rob_index].execution_completed_timestamp = current_core_cycle[cpu]; 
            inflight_reg_executions--;
            completed_executions++;

            if (ROB.entry[rob_index].reg_RAW_producer)
                reg_RAW_release(rob_index);

            if (ROB.entry[rob_index].branch_mispredicted)
	      	{
				if(warmup_complete[cpu])
				{
					fetch_resume_cycle = current_core_cycle[cpu] + 1;
				}
				if(ROB.entry[rob_index].branch_taken)
				{
					fill_btb(ROB.entry[rob_index].ip, ROB.entry[rob_index].branch_target);
				}
	     	}

			if(ROB.entry[rob_index].btb_miss && ROB.entry[rob_index].branch_mispredicted == 0)
        	{
            	uint8_t branch_type = ROB.entry[rob_index].branch_type;
            	assert(branch_type != BRANCH_DIRECT_JUMP && branch_type != BRANCH_DIRECT_CALL && branch_type != BRANCH_CONDITIONAL);
				if(warmup_complete[cpu])
				{ 
                	fetch_resume_cycle = current_core_cycle[cpu] + 1; //Resume fetch from next cycle.
				}
				fill_btb(ROB.entry[rob_index].ip, ROB.entry[rob_index].branch_target);
        	}


            //DP(if(warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id;
            //cout << " branch_mispredicted: " << +ROB.entry[rob_index].branch_mispredicted << " fetch_stall: " << +fetch_stall;
            //cout << " event: " << ROB.entry[rob_index].event_cycle << endl; });
        }
    }
    else {
        if (ROB.entry[rob_index].num_mem_ops == 0) {
            if ((ROB.entry[rob_index].executed == INFLIGHT) && (ROB.entry[rob_index].event_cycle <= current_core_cycle[cpu])) {
                ROB.entry[rob_index].executed = COMPLETED;
                inflight_mem_executions--;
                completed_executions++;
                
                if (ROB.entry[rob_index].reg_RAW_producer)
                    reg_RAW_release(rob_index);

			if (ROB.entry[rob_index].branch_mispredicted)
	      	{
				if(warmup_complete[cpu])
				{
					fetch_resume_cycle = current_core_cycle[cpu] + 1;
				}
				if(ROB.entry[rob_index].branch_taken)
				{
					fill_btb(ROB.entry[rob_index].ip, ROB.entry[rob_index].branch_target);
				}
	     	}

			if(ROB.entry[rob_index].btb_miss && ROB.entry[rob_index].branch_mispredicted == 0)
        	{
            	uint8_t branch_type = ROB.entry[rob_index].branch_type;
            	assert(branch_type != BRANCH_DIRECT_JUMP && branch_type != BRANCH_DIRECT_CALL && branch_type != BRANCH_CONDITIONAL);
				if(warmup_complete[cpu])
				{ 
                	fetch_resume_cycle = current_core_cycle[cpu] + 1; //Resume fetch from next cycle.
				}
				fill_btb(ROB.entry[rob_index].ip, ROB.entry[rob_index].branch_target);
        	}

                //DP(if(warmup_complete[cpu]) {
                //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id;
                //cout << " is_memory: " << +ROB.entry[rob_index].is_memory << " branch_mispredicted: " << +ROB.entry[rob_index].branch_mispredicted;
                //cout << " fetch_stall: " << +fetch_stall << " event: " << ROB.entry[rob_index].event_cycle << " current: " << current_core_cycle[cpu] << endl; });
            }
        }
    }
    ////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
      //          //cout << "Exit complete_execution() "<< endl; });
}

void O3_CPU::reg_RAW_release(uint32_t rob_index)
{
    // if (!ROB.entry[rob_index].registers_instrs_depend_on_me.empty()) 

    ITERATE_SET(i,ROB.entry[rob_index].registers_instrs_depend_on_me, ROB_SIZE) {
        for (uint32_t j=0; j<NUM_INSTR_SOURCES; j++) {
            if (ROB.entry[rob_index].registers_index_depend_on_me[j].search (i)) {
                ROB.entry[i].num_reg_dependent--;

                if (ROB.entry[i].num_reg_dependent == 0) {
                    ROB.entry[i].reg_ready = 1;
                    if (ROB.entry[i].is_memory)
                        ROB.entry[i].scheduled = INFLIGHT;
                    else {
                        ROB.entry[i].scheduled = COMPLETED;

#ifdef SANITY_CHECK
                        if (RTE0[RTE0_tail] < ROB_SIZE)
                            assert(0);
#endif
                        // remember this rob_index in the Ready-To-Execute array 0
                        RTE0[RTE0_tail] = i;

                        //DP (if (warmup_complete[cpu]) {
                        //cout << "[RTE0] " << __func__ << " instr_id: " << ROB.entry[i].instr_id << " rob_index: " << i << " is added to RTE0";
                        //cout << " head: " << RTE0_head << " tail: " << RTE0_tail << endl; }); 

                        RTE0_tail++;
                        if (RTE0_tail == ROB_SIZE)
                            RTE0_tail = 0;

                    }
                }

                //DP (if (warmup_complete[cpu]) {
                //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id << " releases instr_id: ";
                //cout << ROB.entry[i].instr_id << " reg_index: " << +ROB.entry[i].source_registers[j] << " num_reg_dependent: " << ROB.entry[i].num_reg_dependent << " cycle: " << current_core_cycle[cpu] << endl; });
            }
        }
    }
}

void O3_CPU::operate_cache()
{
    ITLB.operate();
    DTLB.operate();
    STLB.operate();
#ifdef INS_PAGE_TABLE_WALKER
    PTW.operate();
#endif
    L1I.operate();
    L1D.operate();
    L2C.operate();

    // also handle per-cycle prefetcher operation
    l1i_prefetcher_cycle_operate();
}

void O3_CPU::update_rob()
{
    //@Vishal: VIPT ITLB processed entries will be handled by L1I cache.
    //if (ITLB.PROCESSED.occupancy && (ITLB.PROCESSED.entry[ITLB.PROCESSED.head].event_cycle <= current_core_cycle[cpu]))
    //    complete_instr_fetch(&ITLB.PROCESSED, 1);

    if (L1I.PROCESSED.occupancy && (L1I.PROCESSED.entry[L1I.PROCESSED.head].event_cycle <= current_core_cycle[cpu]))
        complete_instr_fetch(&L1I.PROCESSED, 0);

    //@Vishal: VIPT DTLB processed entries will be handled by L1D cache
    //if (DTLB.PROCESSED.occupancy && (DTLB.PROCESSED.entry[DTLB.PROCESSED.head].event_cycle <= current_core_cycle[cpu]))
    //    complete_data_fetch(&DTLB.PROCESSED, 1);

    if (L1D.PROCESSED.occupancy && (L1D.PROCESSED.entry[L1D.PROCESSED.head].event_cycle <= current_core_cycle[cpu]))
        complete_data_fetch(&L1D.PROCESSED, 0);

    // update ROB entries with completed executions
    if ((inflight_reg_executions > 0) || (inflight_mem_executions > 0)) {
        if (ROB.head < ROB.tail) {
            for (uint32_t i=ROB.head; i<ROB.tail; i++) 
                complete_execution(i);
        }
        else {
            for (uint32_t i=ROB.head; i<ROB.SIZE; i++)
                complete_execution(i);
            for (uint32_t i=0; i<ROB.tail; i++)
                complete_execution(i);
        }
    }
}

void O3_CPU::complete_instr_fetch(PACKET_QUEUE *queue, uint8_t is_it_tlb)
{
	//@Vishal: VIPT, TLB request should not be handled here
	assert(is_it_tlb == 0);


//Neelu: Todo: IFETCH_BUFFER entries, mark translated and fetched. Since TLB requests will not be handled here due to VIPT, I am not
// inserting code for is_it_tlb condition.

    uint32_t index = queue->head,
             rob_index = queue->entry[index].rob_index,
	     num_fetched = 0;

    uint64_t complete_ip = queue->entry[index].ip;

//Neelu: Marking IFETCH_BUFFER entries translated and fetched and then returning. 

     for(uint32_t j=0; j<IFETCH_BUFFER.SIZE; j++)
     {
	     if(((IFETCH_BUFFER.entry[j].ip)>>6) == ((complete_ip)>>6))
	     {
		     IFETCH_BUFFER.entry[j].translated = COMPLETED;
		     IFETCH_BUFFER.entry[j].fetched = COMPLETED;
	     }
     }

     // remove this entry
     queue->remove_queue(&queue->entry[index]);

     return;

//old function

#ifdef SANITY_CHECK
	//DP (if (warmup_complete[cpu]) {
    //cout << "**(complete_instr_fetch)queue->entry[index].full_addr = "<< hex << queue->entry[index].full_addr << "  instr_id = "<< queue->entry[index].instr_id << " index="<<index<< " rob_index="<<queue->entry[index].rob_index<< endl; });
    int a;
    if (rob_index != (a = check_rob(queue->entry[index].instr_id)))
    {
	    //cout << "complete_instr_fetch, rob_index ="<<rob_index<< " a = " << a << endl; 
	    assert(0);
    }
#endif

    // update ROB entry
    if (is_it_tlb) {
        ROB.entry[rob_index].translated = COMPLETED;
        ROB.entry[rob_index].instruction_pa = (queue->entry[index].instruction_pa << LOG2_PAGE_SIZE) | (ROB.entry[rob_index].ip & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
    }
    else
        ROB.entry[rob_index].fetched = COMPLETED;
    ROB.entry[rob_index].event_cycle = current_core_cycle[cpu];
    num_fetched++;

    //DP ( if (warmup_complete[cpu]) {
    //cout << "[" << queue->NAME << "] " << __func__ << " cpu: " << cpu <<  " instr_id: " << ROB.entry[rob_index].instr_id;
    //cout << " ip: " << hex << ROB.entry[rob_index].ip << " address: " << ROB.entry[rob_index].instruction_pa << dec;
    //cout << " translated: " << +ROB.entry[rob_index].translated << " fetched: " << +ROB.entry[rob_index].fetched;
    //cout << " event_cycle: " << ROB.entry[rob_index].event_cycle << endl; });

    // check if other instructions were merged
    if (queue->entry[index].instr_merged) {
	ITERATE_SET(i,queue->entry[index].rob_index_depend_on_me, ROB_SIZE) {
            // update ROB entry
            if (is_it_tlb) {
                ROB.entry[i].translated = COMPLETED;
                ROB.entry[i].instruction_pa = (queue->entry[index].instruction_pa << LOG2_PAGE_SIZE) | (ROB.entry[i].ip & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            }
            else
                ROB.entry[i].fetched = COMPLETED;
            ROB.entry[i].event_cycle = current_core_cycle[cpu] + (num_fetched / FETCH_WIDTH);
            num_fetched++;

            //DP ( if (warmup_complete[cpu]) {
            //cout << "[" << queue->NAME << "] " << __func__ << " cpu: " << cpu <<  " instr_id: " << ROB.entry[i].instr_id;
            //cout << " ip: " << hex << ROB.entry[i].ip << " address: " << ROB.entry[i].instruction_pa << dec;
            //cout << " translated: " << +ROB.entry[i].translated << " fetched: " << +ROB.entry[i].fetched << " provider: " << ROB.entry[rob_index].instr_id;
            //cout << " event_cycle: " << ROB.entry[i].event_cycle << endl; });
        }
    }

    // remove this entry
    queue->remove_queue(&queue->entry[index]);
    
}

void O3_CPU::complete_data_fetch(PACKET_QUEUE *queue, uint8_t is_it_tlb)
{

	//@Vishal: VIPT, TLB request should not be handled here
	assert(is_it_tlb == 0);


    uint32_t index = queue->head,
             rob_index = queue->entry[index].rob_index,
             sq_index = queue->entry[index].sq_index,
             lq_index = queue->entry[index].lq_index;

#ifdef SANITY_CHECK
    if (queue->entry[index].type != RFO) {
    	//DP (if (warmup_complete[cpu]) {
            //cout << "queue->entry[index].full_addr = "<< queue->entry[index].full_addr << endl; });
        if (rob_index != check_rob(queue->entry[index].instr_id))
        {
            assert(0);
        }
    }
#endif

    // update ROB entry
    if (is_it_tlb) { // DTLB

        if (queue->entry[index].type == RFO) {
            SQ.entry[sq_index].physical_address = (queue->entry[index].data_pa << LOG2_PAGE_SIZE) | (SQ.entry[sq_index].virtual_address & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            SQ.entry[sq_index].translated = COMPLETED;
            SQ.entry[sq_index].event_cycle = current_core_cycle[cpu];

            RTS1[RTS1_tail] = sq_index;
            RTS1_tail++;
            if (RTS1_tail == SQ_SIZE)
                RTS1_tail = 0;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " RFO instr_id: " << SQ.entry[sq_index].instr_id;
            //cout << " DTLB_FETCH_DONE translation: " << +SQ.entry[sq_index].translated << hex << " page: " << (SQ.entry[sq_index].physical_address>>LOG2_PAGE_SIZE);
            //cout << " full_addr: " << SQ.entry[sq_index].physical_address << dec << " store_merged: " << +queue->entry[index].store_merged;
            //cout << " load_merged: " << +queue->entry[index].load_merged << endl; }); 

            handle_merged_translation(&queue->entry[index]);
        }
        else { 
            LQ.entry[lq_index].physical_address = (queue->entry[index].data_pa << LOG2_PAGE_SIZE) | (LQ.entry[lq_index].virtual_address & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            LQ.entry[lq_index].translated = COMPLETED;
            LQ.entry[lq_index].event_cycle = current_core_cycle[cpu];

            RTL1[RTL1_tail] = lq_index;
            RTL1_tail++;
            if (RTL1_tail == LQ_SIZE)
                RTL1_tail = 0;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[RTL1] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << " rob_index: " << LQ.entry[lq_index].rob_index << " is added to RTL1";
            //cout << " head: " << RTL1_head << " tail: " << RTL1_tail << endl; }); 

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " load instr_id: " << LQ.entry[lq_index].instr_id;
            //cout << " DTLB_FETCH_DONE translation: " << +LQ.entry[lq_index].translated << hex << " page: " << (LQ.entry[lq_index].physical_address>>LOG2_PAGE_SIZE);
            //cout << " full_addr: " << LQ.entry[lq_index].physical_address << dec << " store_merged: " << +queue->entry[index].store_merged;
            //cout << " load_merged: " << +queue->entry[index].load_merged << endl; }); 

            handle_merged_translation(&queue->entry[index]);
        }

        ROB.entry[rob_index].event_cycle = queue->entry[index].event_cycle;
    }
    else { // L1D

        if (queue->entry[index].type == RFO)
            handle_merged_load(&queue->entry[index]);
        else { 
#ifdef SANITY_CHECK
            if (queue->entry[index].store_merged)
                assert(0);
#endif
            LQ.entry[lq_index].fetched = COMPLETED;
            LQ.entry[lq_index].event_cycle = current_core_cycle[cpu];
            ROB.entry[rob_index].num_mem_ops--;
            ROB.entry[rob_index].event_cycle = queue->entry[index].event_cycle;

#ifdef SANITY_CHECK
            if (ROB.entry[rob_index].num_mem_ops < 0) {
                cerr << "instr_id: " << ROB.entry[rob_index].instr_id << endl;
                assert(0);
            }
#endif
            if (ROB.entry[rob_index].num_mem_ops == 0)
                inflight_mem_executions++;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " load instr_id: " << LQ.entry[lq_index].instr_id;
            //cout << " L1D_FETCH_DONE fetched: " << +LQ.entry[lq_index].fetched << hex << " address: " << (LQ.entry[lq_index].physical_address>>LOG2_BLOCK_SIZE);
            //cout << " full_addr: " << LQ.entry[lq_index].physical_address << dec << " remain_mem_ops: " << ROB.entry[rob_index].num_mem_ops;
            //cout << " load_merged: " << +queue->entry[index].load_merged << " inflight_mem: " << inflight_mem_executions << endl; }); 

            release_load_queue(lq_index);
            handle_merged_load(&queue->entry[index]);
        }
    }

    // remove this entry
    queue->remove_queue(&queue->entry[index]);
}

//@Vishal: This function is not used anywhere
/*
void O3_CPU::handle_o3_fetch(PACKET *current_packet, uint32_t cache_type)
{
    uint32_t rob_index = current_packet->rob_index,
             sq_index  = current_packet->sq_index,
             lq_index  = current_packet->lq_index;

    // update ROB entry
    if (cache_type == 0) { // DTLB

#ifdef SANITY_CHECK
        if (rob_index != check_rob(current_packet->instr_id))
            assert(0);
#endif
        if (current_packet->type == RFO) {
            SQ.entry[sq_index].physical_address = (current_packet->data_pa << LOG2_PAGE_SIZE) | (SQ.entry[sq_index].virtual_address & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            SQ.entry[sq_index].translated = COMPLETED;

            RTS1[RTS1_tail] = sq_index;
            RTS1_tail++;
            if (RTS1_tail == SQ_SIZE)
                RTS1_tail = 0;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " RFO instr_id: " << SQ.entry[sq_index].instr_id;
            //cout << " DTLB_FETCH_DONE translation: " << +SQ.entry[sq_index].translated << hex << " page: " << (SQ.entry[sq_index].physical_address>>LOG2_PAGE_SIZE);
            //cout << " full_addr: " << SQ.entry[sq_index].physical_address << dec << " store_merged: " << +current_packet->store_merged;
            //cout << " load_merged: " << +current_packet->load_merged << endl; }); 

            handle_merged_translation(current_packet);
        }
        else { 
            LQ.entry[lq_index].physical_address = (current_packet->data_pa << LOG2_PAGE_SIZE) | (LQ.entry[lq_index].virtual_address & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            LQ.entry[lq_index].translated = COMPLETED;

            RTL1[RTL1_tail] = lq_index;
            RTL1_tail++;
            if (RTL1_tail == LQ_SIZE)
                RTL1_tail = 0;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[RTL1] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << " rob_index: " << LQ.entry[lq_index].rob_index << " is added to RTL1";
            //cout << " head: " << RTL1_head << " tail: " << RTL1_tail << endl; }); 

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " load instr_id: " << LQ.entry[lq_index].instr_id;
            //cout << " DTLB_FETCH_DONE translation: " << +LQ.entry[lq_index].translated << hex << " page: " << (LQ.entry[lq_index].physical_address>>LOG2_PAGE_SIZE);
            //cout << " full_addr: " << LQ.entry[lq_index].physical_address << dec << " store_merged: " << +current_packet->store_merged;
            //cout << " load_merged: " << +current_packet->load_merged << endl; }); 

            handle_merged_translation(current_packet);
        }

        ROB.entry[rob_index].event_cycle = current_packet->event_cycle;
    }
    else { // L1D

        if (current_packet->type == RFO)
            handle_merged_load(current_packet);
        else { // do traditional things
#ifdef SANITY_CHECK
            if (rob_index != check_rob(current_packet->instr_id))
                assert(0);

            if (current_packet->store_merged)
                assert(0);
#endif
            LQ.entry[lq_index].fetched = COMPLETED;
            ROB.entry[rob_index].num_mem_ops--;

#ifdef SANITY_CHECK
            if (ROB.entry[rob_index].num_mem_ops < 0) {
                cerr << "instr_id: " << ROB.entry[rob_index].instr_id << endl;
                assert(0);
            }
#endif
            if (ROB.entry[rob_index].num_mem_ops == 0)
                inflight_mem_executions++;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " load instr_id: " << LQ.entry[lq_index].instr_id;
            //cout << " L1D_FETCH_DONE fetched: " << +LQ.entry[lq_index].fetched << hex << " address: " << (LQ.entry[lq_index].physical_address>>LOG2_BLOCK_SIZE);
            //cout << " full_addr: " << LQ.entry[lq_index].physical_address << dec << " remain_mem_ops: " << ROB.entry[rob_index].num_mem_ops;
            //cout << " load_merged: " << +current_packet->load_merged << " inflight_mem: " << inflight_mem_executions << endl; }); 

            release_load_queue(lq_index);

            handle_merged_load(current_packet);

            ROB.entry[rob_index].event_cycle = current_packet->event_cycle;
        }
    }
}*/

void O3_CPU::handle_merged_translation(PACKET *provider)
{

	//@Vishal: VIPT, Translation are not sent from processor, so this code should not be executed.
	assert(0);


    if (provider->store_merged) {
	ITERATE_SET(merged, provider->sq_index_depend_on_me, SQ.SIZE) {
            SQ.entry[merged].translated = COMPLETED;
            SQ.entry[merged].physical_address = (provider->data_pa << LOG2_PAGE_SIZE) | (SQ.entry[merged].virtual_address & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            SQ.entry[merged].event_cycle = current_core_cycle[cpu];

            RTS1[RTS1_tail] = merged;
            RTS1_tail++;
            if (RTS1_tail == SQ_SIZE)
                RTS1_tail = 0;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " store instr_id: " << SQ.entry[merged].instr_id;
            //cout << " DTLB_FETCH_DONE translation: " << +SQ.entry[merged].translated << hex << " page: " << (SQ.entry[merged].physical_address>>LOG2_PAGE_SIZE);
            //cout << " full_addr: " << SQ.entry[merged].physical_address << dec << " by instr_id: " << +provider->instr_id << endl; });
        }
    }
    if (provider->load_merged) {
	ITERATE_SET(merged, provider->lq_index_depend_on_me, LQ.SIZE) {
            LQ.entry[merged].translated = COMPLETED;
            LQ.entry[merged].physical_address = (provider->data_pa << LOG2_PAGE_SIZE) | (LQ.entry[merged].virtual_address & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            LQ.entry[merged].event_cycle = current_core_cycle[cpu];

            RTL1[RTL1_tail] = merged;
            RTL1_tail++;
            if (RTL1_tail == LQ_SIZE)
                RTL1_tail = 0;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[RTL1] " << __func__ << " instr_id: " << LQ.entry[merged].instr_id << " rob_index: " << LQ.entry[merged].rob_index << " is added to RTL1";
            //cout << " head: " << RTL1_head << " tail: " << RTL1_tail << endl; }); 

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " load instr_id: " << LQ.entry[merged].instr_id;
            //cout << " DTLB_FETCH_DONE translation: " << +LQ.entry[merged].translated << hex << " page: " << (LQ.entry[merged].physical_address>>LOG2_PAGE_SIZE);
            //cout << " full_addr: " << LQ.entry[merged].physical_address << dec << " by instr_id: " << +provider->instr_id << endl; });
        }
    }
}

void O3_CPU::handle_merged_load(PACKET *provider)
{
    ITERATE_SET(merged, provider->lq_index_depend_on_me, LQ.SIZE) {
        uint32_t merged_rob_index = LQ.entry[merged].rob_index;

        LQ.entry[merged].fetched = COMPLETED;
        LQ.entry[merged].event_cycle = current_core_cycle[cpu];
     
	ROB.entry[merged_rob_index].num_mem_ops--;
        ROB.entry[merged_rob_index].event_cycle = current_core_cycle[cpu];

#ifdef SANITY_CHECK
        if (ROB.entry[merged_rob_index].num_mem_ops < 0) {
            cerr << "instr_id: " << ROB.entry[merged_rob_index].instr_id << " rob_index: " << merged_rob_index << endl;
            assert(0);
        }
#endif
        if (ROB.entry[merged_rob_index].num_mem_ops == 0)
            inflight_mem_executions++;
	
               //DP (if (warmup_complete[cpu]) {
        //cout << "[ROB] " << __func__ << " load instr_id: " << LQ.entry[merged].instr_id;
        //cout << " L1D_FETCH_DONE translation: " << +LQ.entry[merged].translated << hex << " address: " << (LQ.entry[merged].physical_address>>LOG2_BLOCK_SIZE);
        //cout << " full_addr: " << LQ.entry[merged].physical_address << dec << " by instr_id: " << +provider->instr_id;
        //cout << " remain_mem_ops: " << ROB.entry[merged_rob_index].num_mem_ops << endl; });

        release_load_queue(merged);
    }
}

void O3_CPU::release_load_queue(uint32_t lq_index)
{
    // release LQ entries
    //DP ( if (warmup_complete[cpu]) {
    //cout << "[LQ] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << " releases lq_index: " << lq_index;
    //cout << hex << " full_addr: " << LQ.entry[lq_index].physical_address << dec << endl; });

    LSQ_ENTRY empty_entry;
    LQ.entry[lq_index] = empty_entry;
    //Neelu: Why isn't ROB's LQ index reset when the LQ entry is removed?  
    LQ.occupancy--;
}

void O3_CPU::retire_rob()
{
    for (uint32_t n=0; n<RETIRE_WIDTH; n++) {
        if (ROB.entry[ROB.head].ip == 0)
            return;


	if(warmup_complete[cpu])
	{

		//Neelu: Capturing stats 
                //If it is a load IP, then capturing its occurence before and after it is marked as critical. 
                for (uint32_t i=0; i<NUM_INSTR_SOURCES; i++) {
                        if(ROB.entry[ROB.head].lq_index[i] != UINT32_MAX)
                        {
                                if(critical_ips[cpu].find(ROB.entry[ROB.head].ip) == critical_ips[cpu].end())
                                        RSI_occurence_freq_before_MaC[ROB.entry[ROB.head].ip]++;
                                else
                                        RSI_occurence_freq_after_MaC[ROB.entry[ROB.head].ip]++; 

				unique_load_ips[cpu].insert(ROB.entry[ROB.head].ip);

				break;
                        }
                }


	}


        // retire is in-order
        if (ROB.entry[ROB.head].executed != COMPLETED) { 

//Neelu: Setting stall flag and stall begin cycle if loads are stuck at the head of ROB
#ifdef DETECT_CRITICAL_IPS
	if(warmup_complete[cpu] && (n != RETIRE_WIDTH-1))
	{


		if(ROB.entry[ROB.head].stall_flag == 0)
		{
			//total_rob_stalls[cpu]++;
			rob_stall_count[cpu]++;
			ROB.entry[ROB.head].stall_flag = 1;
			ROB.entry[ROB.head].stall_begin_cycle = current_core_cycle[cpu];
			ROB.entry[ROB.head].stall_begin_rob_occupancy = ROB.occupancy; 

		}

		if(ROB.entry[ROB.head].stall_flag == 1 && ROB.entry[ROB.head].load_stall_flag != 1)
		{
			bool load_or_not = false, lq_entry_or_not = false;
			for (uint32_t i=0; i<NUM_INSTR_SOURCES; i++) {
				if(ROB.entry[ROB.head].lq_index[i] != UINT32_MAX)
				{
					//if(LQ.entry[ROB.entry[ROB.head].lq_index[i]].translated == COMPLETED)
					//{
						load_or_not = true;
						break;
					//}
					//ROB.entry[ROB.head].load_stall_flag = 1;
					//total_load_rob_stalls[cpu]++;
					//break;
					
					//Neelu: Marking the offsets accessed by the current instance of IP as critical.
					//Neelu: Step 1: Check if instr_id is the same in the load queue.
				/*	if(ROB.entry[ROB.head].instr_id == LQ.entry[ROB.entry[ROB.head].lq_index[i]].instr_id) 
					{						
						//Neelu: Step 2: Ensure that the IPs are consistent.
						assert(ROB.entry[ROB.head].ip == LQ.entry[ROB.entry[ROB.head].lq_index[i]].ip);
						int critical_offset = (LQ.entry[ROB.entry[ROB.head].lq_index[i]].virtual_address >> 6) & 0x3F; 
						//Neelu: Step 3: Mark critical_offset in the map
						if(offsets_accessed_by_ip[cpu].find(ROB.entry[ROB.head].ip) == offsets_accessed_by_ip[cpu].end())
						{
							//initialize bit vector
					                for(int i = 0; i < 64; i++)
								offsets_accessed_by_ip[cpu][ROB.entry[ROB.head].ip][i] = 0;
						}
						critical_offsets_of_ip[cpu][ROB.entry[ROB.head].ip][critical_offset] = 1;
						//Neelu: Step 4: Ensure that IP has accessed the critical offset, sanity check.
						assert(offsets_accessed_by_ip[cpu][ROB.entry[ROB.head].ip][critical_offset] == 1);
						
						lq_entry_or_not = true;
					}*/

					//Neelu: Also checking if the same IP is present in the ROB or not
					/*int iteration_index = ROB.head + 1;
					uint8_t same_ip_instance_found = 0;
					uint32_t same_ip_instance_count = 0;
					for(int i = 0; i < ROB.occupancy-1; i++)
					{
						if(iteration_index == ROB.SIZE)
							iteration_index = 0;

						if(ROB.entry[ROB.head].ip == ROB.entry[iteration_index].ip)
						{
							same_ip_instance_found = 1;
							same_ip_instance_count++;
						}
						iteration_index++;	
					}
					if(same_ip_instance_found)
					{
						other_instances_of_stalling_ip_in_rob[cpu]++;
						if(max_instances_of_stalling_ip_in_rob[cpu] < same_ip_instance_count)
						{
							max_instances_of_stalling_ip_in_rob[cpu] = same_ip_instance_count;
							rob_occupancy_at_max_instances[cpu] = ROB.occupancy;
						}
					}*/
					//break;
				}
			}
			if(load_or_not)
			{
				ROB.entry[ROB.head].load_stall_flag = 1;
#ifdef FOCUSED_VALUE_PREDICTION_CRITICALITY_DETECTOR
				critical_ip_candidate_identification_operations++; 
				focused_value_prediction_criticality_detector_operate(ROB.entry[ROB.head].ip, cpu);
#endif 
				//Neelu: Capture stall count before and after it is marked as critical. 
				if(critical_ips[cpu].find(ROB.entry[ROB.head].ip) == critical_ips[cpu].end())
					RSI_stall_freq_before_MaC[ROB.entry[ROB.head].ip]++;
				else
					RSI_stall_freq_after_MaC[ROB.entry[ROB.head].ip]++;	
					
				
				//Check if there is an incomplete instruction in the retire width rn. 
				for (uint32_t y=n; y<RETIRE_WIDTH; y++) {
					if((ROB.entry[ROB.head+y].ip != 0) && (ROB.entry[ROB.head+y].executed != COMPLETED))
						ROB.entry[ROB.head].retire_window_fellow_is_trouble = 1;
				
				}		


				ROB.entry[ROB.head].stall_begin_cycle = current_core_cycle[cpu];
				load_rob_stall_count[cpu]++;
				rob_occu_load_stall_begin[cpu] += ROB.occupancy;

				if(ROB.occupancy < min_rob_occu_load_stall_begin[cpu])
					min_rob_occu_load_stall_begin[cpu] = ROB.occupancy;

				if(ROB.occupancy > max_rob_occu_load_stall_begin[cpu])
					max_rob_occu_load_stall_begin[cpu] = ROB.occupancy;

				if(ROB.occupancy >= 0 && ROB.occupancy < 50)
					rob_load_stall_begin_bucket_count[cpu][0]++;
				else if(ROB.occupancy >= 50 && ROB.occupancy < 100)
					rob_load_stall_begin_bucket_count[cpu][1]++;
				else if(ROB.occupancy >= 100 && ROB.occupancy < 150)
					rob_load_stall_begin_bucket_count[cpu][2]++;
				else if(ROB.occupancy >= 150 && ROB.occupancy < 200)
					rob_load_stall_begin_bucket_count[cpu][3]++;
				else if(ROB.occupancy >= 200 && ROB.occupancy < 250)
					rob_load_stall_begin_bucket_count[cpu][4]++;
				else if(ROB.occupancy >= 250 && ROB.occupancy < 300)
					rob_load_stall_begin_bucket_count[cpu][5]++;
				else if(ROB.occupancy >= 300)
					rob_load_stall_begin_bucket_count[cpu][6]++;
				else 
					assert(0);

				if(critical_ips[cpu].find(ROB.entry[ROB.head].ip) != critical_ips[cpu].end())
				{
					critical_load_rob_stall_count[cpu]++;
					rob_occu_critical_load_stall_begin[cpu] += ROB.occupancy;
					if(ROB.occupancy < min_rob_occu_critical_load_stall_begin[cpu])
						min_rob_occu_critical_load_stall_begin[cpu] = ROB.occupancy;

					if(ROB.occupancy > max_rob_occu_critical_load_stall_begin[cpu])
						max_rob_occu_critical_load_stall_begin[cpu] = ROB.occupancy;

					if(ROB.occupancy >= 0 && ROB.occupancy < 50)
                                        	rob_critical_load_stall_begin_bucket_count[cpu][0]++;
					else if(ROB.occupancy >= 50 && ROB.occupancy < 100)
						rob_critical_load_stall_begin_bucket_count[cpu][1]++;
					else if(ROB.occupancy >= 100 && ROB.occupancy < 150)                                                 
						rob_critical_load_stall_begin_bucket_count[cpu][2]++;
					else if(ROB.occupancy >= 150 && ROB.occupancy < 200)
						rob_critical_load_stall_begin_bucket_count[cpu][3]++;
					else if(ROB.occupancy >= 200 && ROB.occupancy < 250)
						rob_critical_load_stall_begin_bucket_count[cpu][4]++;
					else if(ROB.occupancy >= 250 && ROB.occupancy < 300)
						rob_critical_load_stall_begin_bucket_count[cpu][5]++;
					else if(ROB.occupancy >= 300)
						rob_critical_load_stall_begin_bucket_count[cpu][6]++;
					else
						assert(0);
	

				}

				//total_load_rob_stalls[cpu]++;
				//if(!lq_entry_or_not)
				//	no_lq_entry_on_load_stall[cpu]++;
			}	
		}
	}
#endif
            //DP ( if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[ROB.head].instr_id << " head: " << ROB.head << " is not executed yet" << endl; });
            return;
        }

        // check store instruction
        uint32_t num_store = 0;
        for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
            if (ROB.entry[ROB.head].destination_memory[i])
                num_store++;
        }

        if (num_store) {
            if ((L1D.WQ.occupancy + num_store) <= L1D.WQ.SIZE) {
                for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
                    if (ROB.entry[ROB.head].destination_memory[i]) {

                        PACKET data_packet;
                        uint32_t sq_index = ROB.entry[ROB.head].sq_index[i];

                        // sq_index and rob_index are no longer available after retirement
                        // but we pass this information to avoid segmentation fault
                        data_packet.fill_level = FILL_L1;
						data_packet.fill_l1d = 1;
                        data_packet.cpu = cpu;
                        data_packet.data_index = SQ.entry[sq_index].data_index;
                        data_packet.sq_index = sq_index;

                        //@Vishal: VIPT, send virtual address
                        //data_packet.address = SQ.entry[sq_index].physical_address >> LOG2_BLOCK_SIZE;
                        //data_packet.full_addr = SQ.entry[sq_index].physical_address;

                        data_packet.address = SQ.entry[sq_index].virtual_address >> LOG2_BLOCK_SIZE;
                        data_packet.full_addr = SQ.entry[sq_index].virtual_address;
			data_packet.full_virtual_address = SQ.entry[sq_index].virtual_address;

                        data_packet.instr_id = SQ.entry[sq_index].instr_id;
                        data_packet.rob_index = SQ.entry[sq_index].rob_index;
                        data_packet.ip = SQ.entry[sq_index].ip;
                        data_packet.type = RFO;
                        data_packet.asid[0] = SQ.entry[sq_index].asid[0];
                        data_packet.asid[1] = SQ.entry[sq_index].asid[1];
                        data_packet.event_cycle = current_core_cycle[cpu];

			//Neelu: Setting critical ip flag in packet if ip is identified as critical
			#ifdef DETECT_CRITICAL_IPS
			if(warmup_complete[cpu])
			{
			#ifdef HPCA_2009_CRITICALITY                                                                                                           
			//if issue rate is low. 
			//total_issue_rate_checked_count[cpu]++;
			//if(global_issue_rate[cpu] < ISSUE_RATE_THRESHOLD)                                                                                                
			//{                                     
			//	low_issue_rate[cpu]++;				
				data_packet.critical_ip_flag = hpca_2009_check_critical(data_packet.ip, cpu);                                               
			//} 
			//else: Not HPCA_2009
			#elif defined CATCH_CRITICALITY
			data_packet.critical_ip_flag = catch_check_critical(data_packet.ip, cpu);
			#elif defined FOCUSED_VALUE_PREDICTION_CRITICALITY_DETECTOR                                                                            
			data_packet.critical_ip_flag = focused_value_prediction_check_critical(data_packet.ip, cpu);  
			#else      
			
			#ifdef CRITICALITY_THRESHOLD_RELAXATION
			if(current_epoch_all_ip_prefetch == -1)
			{
				//We don't want to prefetch at all in this epoch.
				data_packet.critical_ip_flag = 0;
			}
			else if(current_epoch_all_ip_prefetch == 1)
			{
				//We want to prefetch for all stalling ips in this epoch.
				data_packet.critical_ip_flag = 1;
				/*if(rob_stalling_load_ips[cpu].find(SQ.entry[sq_index].ip) != rob_stalling_load_ips[cpu].end())
					data_packet.critical_ip_flag = 1;
				else
					data_packet.critical_ip_flag = 0;*/
			}
			else if(current_epoch_all_ip_prefetch == 0 && warmup_complete[cpu])
			{
				//Based on critical ips.

#ifdef SET_ASSOCIATIVE_CRITICAL_IP_CACHE
				if(critical_ip_cache.find(SQ.entry[sq_index].ip))
					data_packet.critical_ip_flag = 1;                               
				else                                    
					data_packet.critical_ip_flag = 0;
#else

				uint8_t present_in_lower_threshold_critical_ips = 0;

				for(int i = 0; i < lower_threshold_critical_ips[cpu].size(); i++)
				{
					if(lower_threshold_critical_ips[cpu][i] == SQ.entry[sq_index].ip)
					{
						// Prefetch address is present in RR filter
						present_in_lower_threshold_critical_ips = 1;
					}
				}

				if(critical_ips[cpu].find(SQ.entry[sq_index].ip) != critical_ips[cpu].end() || present_in_lower_threshold_critical_ips)
					data_packet.critical_ip_flag = 1;
				else
					data_packet.critical_ip_flag = 0;
#endif
			}
			else
				assert(!warmup_complete[cpu]);
			#else
			#ifdef SET_ASSOCIATIVE_CRITICAL_IP_CACHE
	                if(critical_ip_cache.find(SQ.entry[sq_index].ip))
	                        data_packet.critical_ip_flag = 1;
	                else
        	                data_packet.critical_ip_flag = 0;
			#else

			if(critical_ips[cpu].find(SQ.entry[sq_index].ip) != critical_ips[cpu].end())
				data_packet.critical_ip_flag = 1;
			else                                                            
				data_packet.critical_ip_flag = 0;
			#endif
			#endif
			#endif
			}
			#endif
			

                        L1D.add_wq(&data_packet);

			sim_store_sent++;
                    }
                }
            }
            else {
                //DP ( if (warmup_complete[cpu]) {
                //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[ROB.head].instr_id << " L1D WQ is full" << endl; });

                L1D.WQ.FULL++;
                L1D.STALL[RFO]++;

                return;
            }
        }

        // release SQ entries
        for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
            if (ROB.entry[ROB.head].sq_index[i] != UINT32_MAX) {
                uint32_t sq_index = ROB.entry[ROB.head].sq_index[i];

                //DP ( if (warmup_complete[cpu]) {
                //cout << "[SQ] " << __func__ << " instr_id: " << ROB.entry[ROB.head].instr_id << " releases sq_index: " << sq_index;
                //cout << hex << " address: " << (SQ.entry[sq_index].physical_address>>LOG2_BLOCK_SIZE);
                //cout << " full_addr: " << SQ.entry[sq_index].physical_address << dec << endl; });

                LSQ_ENTRY empty_entry;
                SQ.entry[sq_index] = empty_entry;
                
                SQ.occupancy--;
                SQ.head++;
                if (SQ.head == SQ.SIZE)
                    SQ.head = 0;
            }
        }

        // release ROB entry
        //DP ( if (warmup_complete[cpu]) {
        //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[ROB.head].instr_id << " is retired" << endl; });

	//Neelu: Invoking criticality training
#ifdef DETECT_CRITICAL_IPS

	#ifdef CATCH_CRITICALITY
        if(warmup_complete[cpu])
	{	
		ROB.entry[ROB.head].instr_commit_timestamp = current_core_cycle[cpu];
		for (uint32_t i=0; i<NUM_INSTR_SOURCES; i++)
		{
			if(ROB.entry[ROB.head].lq_index[i] != UINT32_MAX)
			{
				if(LQ.entry[ROB.entry[ROB.head].lq_index[i]].l2_or_llc_hit)
					ROB.entry[ROB.head].l2_or_llc_hit = true;
			}
		}
		catch_record_instruction(ROB.entry[ROB.head], cpu);
        }
	#endif

	if(ROB.entry[ROB.head].stall_flag)
	{
		//Mind that the flag should be set only if warmup is complete
		assert(warmup_complete[cpu]);
		total_rob_stall_cycles[cpu] += (current_core_cycle[cpu] - ROB.entry[ROB.head].stall_begin_cycle); 

		if(ROB.entry[ROB.head].load_stall_flag)
		{

			//if(ROB.entry[ROB.head].ip == 10823277 || ROB.entry[ROB.head].ip == 4252061)	//gcc-2226
			//if(ROB.entry[ROB.head].ip == 4546126 || ROB.entry[ROB.head].ip == 4546292) //fotonik-10881
			//{
			//	load_stalls_rob_occu[ROB.entry[ROB.head].ip].push_back(ROB.occupancy);
			//	load_stalls_cycles[ROB.entry[ROB.head].ip].push_back(current_core_cycle[cpu]);

			//}


			if(per_ip_rob_load_stall_end_buckets[cpu].find(ROB.entry[ROB.head].ip) == per_ip_rob_load_stall_end_buckets[cpu].end())
			{
				per_ip_rob_load_stall_end_buckets[cpu][ROB.entry[ROB.head].ip] = {0, 0, 0, 0, 0, 0, 0};
			}
			
			if(freq_rob_stalling_load_ips[cpu].find(ROB.entry[ROB.head].ip) == freq_rob_stalling_load_ips[cpu].end())
				freq_rob_stalling_load_ips[cpu][ROB.entry[ROB.head].ip] = 1;
			else
				freq_rob_stalling_load_ips[cpu][ROB.entry[ROB.head].ip]++;

			if(per_ip_rob_stall_occupancy_sum[cpu].find(ROB.entry[ROB.head].ip) == per_ip_rob_stall_occupancy_sum[cpu].end())
				per_ip_rob_stall_occupancy_sum[cpu][ROB.entry[ROB.head].ip] = 0;
			per_ip_rob_stall_occupancy_sum[cpu][ROB.entry[ROB.head].ip]+= ROB.occupancy;


			rob_occu_load_stall_end[cpu] += ROB.occupancy;
			if(ROB.occupancy > max_rob_occu_load_stall_end[cpu])
				max_rob_occu_load_stall_end[cpu] = ROB.occupancy;
			if(ROB.occupancy < min_rob_occu_load_stall_end[cpu])
				min_rob_occu_load_stall_end[cpu] = ROB.occupancy;

			if(ROB.occupancy >= 0 && ROB.occupancy < 50)
			{
				rob_load_stall_end_bucket_count[cpu][0]++;
				per_ip_rob_load_stall_end_buckets[cpu][ROB.entry[ROB.head].ip][0]++;
			}
			else if(ROB.occupancy >= 50 && ROB.occupancy < 100)
			{
				rob_load_stall_end_bucket_count[cpu][1]++;
				per_ip_rob_load_stall_end_buckets[cpu][ROB.entry[ROB.head].ip][1]++;
			}
			else if(ROB.occupancy >= 100 && ROB.occupancy < 150)
			{
				rob_load_stall_end_bucket_count[cpu][2]++;
				per_ip_rob_load_stall_end_buckets[cpu][ROB.entry[ROB.head].ip][2]++;
			}
			else if(ROB.occupancy >= 150 && ROB.occupancy < 200)
			{
				rob_load_stall_end_bucket_count[cpu][3]++;
				per_ip_rob_load_stall_end_buckets[cpu][ROB.entry[ROB.head].ip][3]++;
			}
			else if(ROB.occupancy >= 200 && ROB.occupancy < 250)
			{
				rob_load_stall_end_bucket_count[cpu][4]++;
				per_ip_rob_load_stall_end_buckets[cpu][ROB.entry[ROB.head].ip][4]++;
			}
			else if(ROB.occupancy >= 250 && ROB.occupancy < 300)
			{
				rob_load_stall_end_bucket_count[cpu][5]++;
				per_ip_rob_load_stall_end_buckets[cpu][ROB.entry[ROB.head].ip][5]++;
			}
			else if(ROB.occupancy >= 300)
			{
				rob_load_stall_end_bucket_count[cpu][6]++;
				per_ip_rob_load_stall_end_buckets[cpu][ROB.entry[ROB.head].ip][6]++;
			}
			else
				assert(0);				

			if(critical_ips[cpu].find(ROB.entry[ROB.head].ip) != critical_ips[cpu].end())
			{
				rob_occu_critical_load_stall_end[cpu] += ROB.occupancy;
				if(ROB.occupancy > max_rob_occu_critical_load_stall_end[cpu])
					max_rob_occu_critical_load_stall_end[cpu] = ROB.occupancy;
				if(ROB.occupancy < min_rob_occu_critical_load_stall_end[cpu])
					min_rob_occu_critical_load_stall_end[cpu] = ROB.occupancy;

				if(ROB.occupancy >= 0 && ROB.occupancy < 50)
					rob_critical_load_stall_end_bucket_count[cpu][0]++;
				else if(ROB.occupancy >= 50 && ROB.occupancy < 100)
					rob_critical_load_stall_end_bucket_count[cpu][1]++;
				else if(ROB.occupancy >= 100 && ROB.occupancy < 150)
					rob_critical_load_stall_end_bucket_count[cpu][2]++;
				else if(ROB.occupancy >= 150 && ROB.occupancy < 200)
					rob_critical_load_stall_end_bucket_count[cpu][3]++;
				else if(ROB.occupancy >= 200 && ROB.occupancy < 250)
					rob_critical_load_stall_end_bucket_count[cpu][4]++;
				else if(ROB.occupancy >= 250 && ROB.occupancy < 300)
					rob_critical_load_stall_end_bucket_count[cpu][5]++;
				else if(ROB.occupancy >= 300) 
					rob_critical_load_stall_end_bucket_count[cpu][6]++;
				else
					assert(0);  
			}

			total_load_rob_stall_cycles[cpu] += (current_core_cycle[cpu] - ROB.entry[ROB.head].load_stall_begin_cycle);
#ifdef FOCUSED_PREFETCHING_COUNTING_CLASSIFIER
			critical_ip_candidate_identification_operations+=2;
		        if(current_core_cycle[cpu] - ROB.entry[ROB.head].load_stall_begin_cycle > CANDIDACY_THRESHOLD)
			{	
				counter_criticality_detector_operate(ROB.entry[ROB.head].ip, current_core_cycle[cpu] - ROB.entry[ROB.head].load_stall_begin_cycle, cpu);
			}
#elif defined ROB_OCCUPANCY_BASED_CRITICALITY 
			//Neelu: This condition ensures that the stall did occur for some cycles as ROB occupancy has increased. 

			if(current_core_cycle[cpu] - ROB.entry[ROB.head].load_stall_begin_cycle > 0)
			{
				critical_ip_candidate_identification_operations+=2; 
				if(ROB.occupancy > CANDIDACY_THRESHOLD)
				{
					counter_criticality_detector_operate(ROB.entry[ROB.head].ip, ROB.occupancy, cpu);
				}
			}

#elif defined HPCA_2009_CRITICALITY
			critical_ip_candidate_identification_operations++;
			total_issue_rate_checked_count[cpu]++;                                                                                                                                      if(global_issue_rate[cpu] < ISSUE_RATE_THRESHOLD)                                                                                                                           {                                                                                                                                                                                   low_issue_rate[cpu]++; 
				hpca_2009_criticality_detector_operate(ROB.entry[ROB.head].ip, loads_direct_dependents[cpu][ROB.entry[ROB.head].instr_id], cpu);
			}
			//load is being removed from rob
			reverse_loads_in_rob[cpu].erase(loads_in_rob[cpu][ROB.entry[ROB.head].instr_id]);
			loads_in_rob[cpu].erase(ROB.entry[ROB.head].instr_id);
			loads_direct_dependents[cpu].erase(ROB.entry[ROB.head].instr_id);

#endif
		}

	}

#endif


        ooo_model_instr empty_entry;
        ROB.entry[ROB.head] = empty_entry;

        ROB.head++;
        if (ROB.head == ROB.SIZE)
            ROB.head = 0;
        ROB.occupancy--;
        completed_executions--;
        num_retired++;

#ifdef CRITICALITY_THRESHOLD_RELAXATION
	if(warmup_complete[cpu] && num_retired > warmup_instructions)
	{
		if((num_retired - warmup_instructions) % NUM_OF_INSTRUCTIONS_IN_EPOCH == 0) 
		{
			//set ipc based on epoch. 

			//Assuming that epoch id was 0 throughout the first epoch, we will set baseline ipc when epoch id is 0.
			if(epoch_id == 0)
			{
				no_prefetch_epoch_ipc = float(float(num_retired - begin_sim_instr)/float(current_core_cycle[cpu] - begin_sim_cycle));
			}
			else
			{
				assert(current_epoch_all_ip_prefetch != -1);
				//Now we need to check whether the epoch was a critical ip prefetch epoch or all ip prefetch epoch.
				if(current_epoch_all_ip_prefetch == 1)
				{
					all_ip_prefetch_epoch_ipc = float(float(num_retired - last_epoch_completion_instruction)/float(current_core_cycle[cpu] - last_epoch_completion_cycle));

				}
				else if(current_epoch_all_ip_prefetch == 0)
				{
					critical_ip_prefetch_epoch_ipc = float(float(num_retired - last_epoch_completion_instruction)/float(current_core_cycle[cpu] - last_epoch_completion_cycle));
				}
			}

			//Update current_epoch_all_ip_prefetch.
			//Compare IPC and update criticality thresholds.
			//Note: If the thresholds were updated, we need two more epochs to again compare a new all_ip_prefetch_ipc and critical_ip_prefetch_ipc. This will ensure that we are not comparing the critical ip prefetch performance with all ip prefetch performance of another phase of the program.
			
		       	if((last_dynamic_threshold_test_result != 0) && (current_epoch_all_ip_prefetch == 0))
			{
				//This means that either last_dynamic_threshold_test_result == -1 which implies this is the first test, or that thresholds were modified in the last test (result = 1) and we are testing again. Either way, the test procedure will be the same. 
				float percent_increase_in_all_ip_prefetch = (all_ip_prefetch_epoch_ipc - critical_ip_prefetch_epoch_ipc)*100/critical_ip_prefetch_epoch_ipc;
				if(percent_increase_in_all_ip_prefetch > 4)
				{
					//This means we need to change thresholds and test again again again. 
					num_of_times_thresholds_modified++;
				        thresholds_modification_factor--;	
					last_dynamic_threshold_test_result = 1;
					if((thresholds_modification_factor*-1) >= MAX_THRESHOLDS_MODIFICATION_FACTOR)
					{
						thresholds_modification_factor = 0;
						candidacy_threshold = 0;
						static_reliance_threshold = 0;
						//Now we want to continue critical ip prefetching without further modifications
						last_dynamic_threshold_test_result = 0;
					}
				}
				else
					last_dynamic_threshold_test_result = 0;
			}	

			//updating current_epoch_all_ip_prefetch, if it is -1, then update it to 1
			if(current_epoch_all_ip_prefetch == -1)
				current_epoch_all_ip_prefetch = 1;
			else if(current_epoch_all_ip_prefetch == 1)
				current_epoch_all_ip_prefetch = 0;
			else if(current_epoch_all_ip_prefetch == 0 && last_dynamic_threshold_test_result == 1)
				current_epoch_all_ip_prefetch = 1;

			last_epoch_completion_cycle = current_core_cycle[cpu];
                        last_epoch_completion_instruction = num_retired;
			epoch_id++;


		}
	}
#endif

//Neelu: resetting global stall count and per ip stall count for counter based mechanism.
#if defined ROB_OCCUPANCY_BASED_CRITICALITY || defined FOCUSED_PREFETCHING_COUNTING_CLASSIFIER  
		#ifdef ROB_OCCUPANCY_BASED_CRITICALITY
		if(num_retired % 1000000 == 0)	//We use instructions because the data collected can be for different number of instructions on using cycles. Even in this case, each chunk of 1M instructions can have different number of loads that stall the ROB, but this is relatively more uniform. 
		#endif
		#ifdef FOCUSED_PREFETCHING_COUNTING_CLASSIFIER
		if(current_core_cycle[cpu] % 1000000 == 0)
		#endif
		{
			uint64_t temp_PIST_entries = 0;
			
			if(max_global_stall_metric_count < global_stall_metric_count[cpu])
				max_global_stall_metric_count = global_stall_metric_count[cpu];
			
			global_stall_metric_count[cpu] = 0;
			for(int i = 0; i < NUM_OF_CRITICAL_IP_TRAINING_TABLE_SETS; i++)
			{
				temp_PIST_entries += per_ip_stall_count[cpu][i].size();
				per_ip_stall_count[cpu][i].clear();	
			}
			if(max_PIST_entries[cpu] < temp_PIST_entries)
				max_PIST_entries[cpu] = temp_PIST_entries;
			if(min_PIST_entries[cpu] > temp_PIST_entries)
				min_PIST_entries[cpu] = temp_PIST_entries;
			sum_PIST_entries[cpu] += temp_PIST_entries;
			count_PIST_entries[cpu]++;
		}
#endif
    }
}

void O3_CPU::core_final_stats()
{
	cout << "Issued all: " << issued_all << endl; 
	cout << "Issued LT all: " << issued_lt_all << endl; 
	cout << "rob_not_full_when_issued_lt_all: " << rob_not_full_when_issued_lt_all << endl;
	cout << "Issued LT one: " << issued_lt_one << endl;
	cout << "rob_not_full_when_issued_lt_one: " << rob_not_full_when_issued_lt_one << endl;

#ifdef FOCUSED_PREFETCHING_COUNTING_CLASSIFIER
	//one addition per cycle
	critical_ip_candidate_identification_operations+= current_core_cycle[cpu] - ooo_cpu[cpu].begin_sim_cycle;
#endif

	cout << "critical_ip_candidate_identification_operations: " << critical_ip_candidate_identification_operations << endl; 
	cout << "critical_ip_candidate_identification_operation_per_cycle: " << float(critical_ip_candidate_identification_operations)/(current_core_cycle[cpu] - ooo_cpu[cpu].begin_sim_cycle) << endl;

#ifdef HPCA_2009_CRITICALITY
	cout << "HPCA_2009 critical_dynamic_instances_" << cpu << ": " << critical_dynamic_instances[cpu] << endl;
	cout << "HPCA_2009 unique_critical_ips_" << cpu << ": " << unique_critical_ips[cpu].size() << endl;
        cout << "low_issue_rate_" << cpu << ": " << low_issue_rate[cpu] << endl;	
	cout << "total_issue_rate_checked_count_" << cpu << ": " << total_issue_rate_checked_count[cpu] << endl; 
#endif
#ifdef FOCUSED_VALUE_PREDICTION_CRITICALITY_DETECTOR 
	cout << "FVP critical_dynamic_instances_" << cpu << ": " << fvp_dynamic_critical_instances[cpu] << endl;                            	
	cout << "FVP unique_critical_ips_" << cpu << ": " << fvp_unique_critical_ips[cpu].size() << endl; 
#endif
	cout << "Unique Load IPs: " << unique_load_ips[cpu].size() << endl;
	cout << "CPU " << cpu << " NUM of IPS identified critical: " << critical_ips[cpu].size() << endl;	
	cout << "CPU " << cpu << " Num of Load IPs stalling ROB: " << rob_stalling_load_ips[cpu].size() << endl;
	cout << "Threshold modification factor: " << thresholds_modification_factor << endl;
	uint64_t lt_50_percent_before_Mac = 0, gt_50_percent_before_Mac = 0, gt_75_percent_before_Mac = 0, gt_90_percent_before_Mac = 0; 
	uint64_t stall_proportion_bin_same_before_after = 0, stall_proportion_bin_lower_before_after = 0, stall_proportion_bin_upper_before_after = 0, count = 0, stall_proportion_zero_count = 0;
	float average_stall_proportion_before = 0, average_stall_proportion_after = 0;
	std::map<uint64_t, uint64_t>::iterator map_it = RSI_stall_freq_after_MaC.begin();
	while(map_it != RSI_stall_freq_after_MaC.end())
	{
		float stall_percent_before_MaC = float(RSI_stall_freq_before_MaC[map_it->first])/(RSI_stall_freq_before_MaC[map_it->first] + map_it->second); 


		if(stall_percent_before_MaC < 0.5)
			lt_50_percent_before_Mac++;
		else if(stall_percent_before_MaC > 0.9)
			gt_90_percent_before_Mac++;
		else if(stall_percent_before_MaC > 0.75)
			gt_75_percent_before_Mac++;
		else if(stall_percent_before_MaC > 0.5)
			gt_50_percent_before_Mac++;
	
		float stall_proportion_before = float(RSI_stall_freq_before_MaC[map_it->first])/RSI_occurence_freq_before_MaC[map_it->first]; 
		float stall_proportion_after = float(RSI_stall_freq_after_MaC[map_it->first])/RSI_occurence_freq_after_MaC[map_it->first];

		average_stall_proportion_before += stall_proportion_before;
		average_stall_proportion_after += stall_proportion_after;
		count++;

		map_it++;
	}

	uint64_t total_freq = 0, total_freq_all_stalling_ips = 0;

	std::set<uint64_t>::iterator it = critical_ips[cpu].begin();
#ifndef CATCH_CRITICALITY
	while(it != critical_ips[cpu].end())
	{
		assert(freq_rob_stalling_load_ips[cpu].find(*it) != freq_rob_stalling_load_ips[cpu].end());
	
		total_freq += freq_rob_stalling_load_ips[cpu][*it]; 

		it++;
	}

	cout << "Critical IP stalls, total frequency: " << total_freq << endl;

	total_freq = 0;

	it = rob_stalling_load_ips[cpu].begin();
        while(it != rob_stalling_load_ips[cpu].end())
        {
		if(critical_ips[cpu].find(*it) == critical_ips[cpu].end())
		{
			assert(freq_rob_stalling_load_ips[cpu].find(*it) != freq_rob_stalling_load_ips[cpu].end());
			total_freq += freq_rob_stalling_load_ips[cpu][*it];
		}
		total_freq_all_stalling_ips += freq_rob_stalling_load_ips[cpu][*it]; 
                it++;
        }

	cout << "Non-critical IP stalls, total frequency: " << total_freq << endl;

	cout << "Total Freq all stalling IPs: " << total_freq_all_stalling_ips << endl;
#endif 
	set<uint64_t> ips_causing_90_percent_rob_stalls, ips_causing_85_percent_rob_stalls, ips_causing_80_percent_rob_stalls, ips_causing_75_percent_rob_stalls, ips_causing_70_percent_rob_stalls, ips_causing_65_percent_rob_stalls, ips_causing_60_percent_rob_stalls, ips_causing_55_percent_rob_stalls, ips_causing_50_percent_rob_stalls; 

	bool check_flag = true;
	uint64_t stat_90_percent_critical_ip_stalls = 0, stat_90_percent_non_critical_ip_stalls = 0, ROB_occu_sum_90_percent_non_critical_ip_stalls = 0, ROB_occu_sum_90_percent_critical_ip_stalls = 0;
	float percent = 0;
	float percent50 = 0, percent55 = 0, percent60 = 0, percent65 = 0, percent70 = 0, percent75 = 0, percent80 = 0, percent85 = 0, percent90 = 0;
	while(check_flag)
	{
		map<uint64_t,uint64_t>::iterator most_frequent = max_element(freq_rob_stalling_load_ips[cpu].begin(),freq_rob_stalling_load_ips[cpu].end(),[] (const std::pair<uint64_t, uint64_t>& a, const std::pair<uint64_t, uint64_t>& b)->bool{ return a.second < b.second; } );
		float curr_percent = float(most_frequent->second)*100/total_freq_all_stalling_ips;
		
		if(percent50 < 50)
                {
                        ips_causing_50_percent_rob_stalls.insert(most_frequent->first);
                        percent50 += curr_percent;
                }

		if(percent55 < 55)
                {
                        ips_causing_55_percent_rob_stalls.insert(most_frequent->first);
                        percent55 += curr_percent;
                }

		if(percent60 < 60)
                {
                        ips_causing_60_percent_rob_stalls.insert(most_frequent->first);
                        percent60 += curr_percent;
                }

		if(percent65 < 65)
                {
                        ips_causing_65_percent_rob_stalls.insert(most_frequent->first);
                        percent65 += curr_percent;
                }

		if(percent70 < 70)
                {
                        ips_causing_70_percent_rob_stalls.insert(most_frequent->first);
                        percent70 += curr_percent;
                }


		if(percent75 < 75)
		{
			ips_causing_75_percent_rob_stalls.insert(most_frequent->first);
			percent75 += curr_percent;
		}
		if(percent80 < 80)
                {
                        ips_causing_80_percent_rob_stalls.insert(most_frequent->first);
                        percent80 += curr_percent;
                }
		if(percent85 < 85)
                {
                        ips_causing_85_percent_rob_stalls.insert(most_frequent->first);
                        percent85 += curr_percent;
                }
		if(percent90 < 90)
                {
                        ips_causing_90_percent_rob_stalls.insert(most_frequent->first);
                        percent90 += curr_percent;

			stat_90_percent_critical_ip_stalls += freq_rob_stalling_load_ips[cpu][most_frequent->first];

			ROB_occu_sum_90_percent_critical_ip_stalls += per_ip_rob_stall_occupancy_sum[cpu][most_frequent->first];

	
                }

		freq_rob_stalling_load_ips[cpu].erase(most_frequent->first);

		if(percent75 >= 75 && percent80 >= 80 && percent85 >= 85 && percent90 >= 90)
			check_flag = false;

	}

	//Coverage and accuracy of critical IP detector.
	uint64_t critical_ip_found[3] = {0}; 
	//For IPs causing 70%/80%/90% ROB stalls. 
	std::set<uint64_t>::iterator critical_ip_set_iterator;
#if defined ROB_OCCUPANCY_BASED_CRITICALITY || defined FOCUSED_PREFETCHING_COUNTING_CLASSIFIER || defined CATCH_CRITICALITY 
	critical_ip_set_iterator = critical_ips[cpu].begin();
	while(critical_ip_set_iterator != critical_ips[cpu].end())
	{
		if(ips_causing_70_percent_rob_stalls.find(*critical_ip_set_iterator) != ips_causing_70_percent_rob_stalls.end())
		{
			critical_ip_found[0]++;
		}
		if(ips_causing_80_percent_rob_stalls.find(*critical_ip_set_iterator) != ips_causing_80_percent_rob_stalls.end()) 
		{                                                                                                                                              
			critical_ip_found[1]++;
		}
		if(ips_causing_90_percent_rob_stalls.find(*critical_ip_set_iterator) != ips_causing_90_percent_rob_stalls.end())
		{
			critical_ip_found[2]++;
		}
		critical_ip_set_iterator++;
	}
	//cout << "Critical_IP_Detector_Accuracy_70: " << float(critical_ip_found[0])/critical_ips[cpu].size() << endl;                                           
    	//cout << "Critical_IP_Detector_Accuracy_80: " << float(critical_ip_found[1])/critical_ips[cpu].size() << endl;                                     
       	cout << "Critical_IP_Detector_Accuracy_90: " << float(critical_ip_found[2])/critical_ips[cpu].size() << endl;   
#elif defined HPCA_2009_CRITICALITY 
	critical_ip_set_iterator = unique_critical_ips[cpu].begin();
        while(critical_ip_set_iterator != unique_critical_ips[cpu].end())                                                                                          
	{  
		if(ips_causing_70_percent_rob_stalls.find(*critical_ip_set_iterator) != ips_causing_70_percent_rob_stalls.end())                                          
		{     
			critical_ip_found[0]++;       
		}   
		if(ips_causing_80_percent_rob_stalls.find(*critical_ip_set_iterator) != ips_causing_80_percent_rob_stalls.end())                   
		{     
			critical_ip_found[1]++;               
		}                
		if(ips_causing_90_percent_rob_stalls.find(*critical_ip_set_iterator) != ips_causing_90_percent_rob_stalls.end())                   
		{                     
			critical_ip_found[2]++;                 
		}                         
		critical_ip_set_iterator++;
	}   	
	//cout << "Critical_IP_Detector_Accuracy_70: " << float(critical_ip_found[0])/unique_critical_ips[cpu].size() << endl;                                    
	//cout << "Critical_IP_Detector_Accuracy_80: " << float(critical_ip_found[1])/unique_critical_ips[cpu].size() << endl;                                             
	cout << "Critical_IP_Detector_Accuracy_90: " << float(critical_ip_found[2])/unique_critical_ips[cpu].size() << endl;   
#elif defined FOCUSED_VALUE_PREDICTION_CRITICALITY_DETECTOR 
	critical_ip_set_iterator = fvp_unique_critical_ips[cpu].begin(); 
	while(critical_ip_set_iterator != fvp_unique_critical_ips[cpu].end())            
	{                
		if(ips_causing_70_percent_rob_stalls.find(*critical_ip_set_iterator) != ips_causing_70_percent_rob_stalls.end())           
		{        
			critical_ip_found[0]++;  
		}               
		if(ips_causing_80_percent_rob_stalls.find(*critical_ip_set_iterator) != ips_causing_80_percent_rob_stalls.end())             
		{                               
			critical_ip_found[1]++;                
		}             
		if(ips_causing_90_percent_rob_stalls.find(*critical_ip_set_iterator) != ips_causing_90_percent_rob_stalls.end())             
		{                   
			critical_ip_found[2]++;         
		}                    
		critical_ip_set_iterator++; 
	}   
	//cout << "Critical_IP_Detector_Accuracy_70: " << float(critical_ip_found[0])/fvp_unique_critical_ips[cpu].size() << endl; 
	//cout << "Critical_IP_Detector_Accuracy_80: " << float(critical_ip_found[1])/fvp_unique_critical_ips[cpu].size() << endl;
	cout << "Critical_IP_Detector_Accuracy_90: " << float(critical_ip_found[2])/fvp_unique_critical_ips[cpu].size() << endl;
#endif	

	//cout << "Critical_IP_Detector_Coverage_70: " << float(critical_ip_found[0])/ips_causing_70_percent_rob_stalls.size() << endl;
	//cout << "Critical_IP_Detector_Coverage_80: " << float(critical_ip_found[1])/ips_causing_80_percent_rob_stalls.size() << endl; 
	cout << "Critical_IP_Detector_Coverage_90: " << float(critical_ip_found[2])/ips_causing_90_percent_rob_stalls.size() << endl; 

	/* std::set<uint64_t>::iterator set_iterator = ips_causing_90_percent_rob_stalls.begin();
	uint64_t stat_90_percent_critical_ip_stalls = 0, stat_90_percent_non_critical_ip_stalls = 0;
	while(set_iterator != ips_causing_90_percent_rob_stalls.end())
	{
		
		if(critical_ips[cpu].find(*set_iterator) != critical_ips[cpu].end())
		{
			stat_90_percent_critical_ip_stalls += freq_rob_stalling_load_ips[cpu][*set_iterator];
		}
		else
		{
			stat_90_percent_non_critical_ip_stalls += freq_rob_stalling_load_ips[cpu][*set_iterator];
		}

		set_iterator++; 
	}*/

	cout << "stat_90_percent_critical_ip_stalls_freq: " << stat_90_percent_critical_ip_stalls << endl;
	cout << "stat_90_percent_non_critical_ip_stalls_freq: " << (total_freq_all_stalling_ips - stat_90_percent_critical_ip_stalls) << endl;

	std::map<uint64_t, uint64_t>::iterator map_iterator = per_ip_rob_stall_occupancy_sum[cpu].begin();
	while(map_iterator != per_ip_rob_stall_occupancy_sum[cpu].end())
	{
		if(ips_causing_90_percent_rob_stalls.find(map_iterator->first) == ips_causing_90_percent_rob_stalls.end())
			ROB_occu_sum_90_percent_non_critical_ip_stalls += per_ip_rob_stall_occupancy_sum[cpu][map_iterator->first];
		map_iterator++;
	}

	cout << "AVERAGE_ROB_Occupancy_90_Percent_Critical_IP_Stalls: " << float(ROB_occu_sum_90_percent_critical_ip_stalls)/stat_90_percent_critical_ip_stalls << endl;

	cout << "AVERAGE_ROB_Occupancy_90_Percent_Non_Critical_IP_Stalls: " << float(ROB_occu_sum_90_percent_non_critical_ip_stalls)/(total_freq_all_stalling_ips - stat_90_percent_critical_ip_stalls) << endl;



	uint64_t critical_bucket[7] = {0}, non_critical_bucket[7] = {0};
	uint64_t percent_critical_bucket[9][7] = {0}, percent_non_critical_bucket[9][7] = {0}; 

	map<uint64_t, array<uint64_t, 7>>::iterator iter = per_ip_rob_load_stall_end_buckets[cpu].begin();

	while(iter != per_ip_rob_load_stall_end_buckets[cpu].end())
	{

		if(critical_ips[cpu].find(iter->first) != critical_ips[cpu].end())
		{
			critical_bucket[0] += iter->second[0];
			critical_bucket[1] += iter->second[1];
			critical_bucket[2] += iter->second[2];
			critical_bucket[3] += iter->second[3];
			critical_bucket[4] += iter->second[4];
			critical_bucket[5] += iter->second[5];
			critical_bucket[6] += iter->second[6];
		}
		else
		{
			non_critical_bucket[0] += iter->second[0];
			non_critical_bucket[1] += iter->second[1];
			non_critical_bucket[2] += iter->second[2];
			non_critical_bucket[3] += iter->second[3];
			non_critical_bucket[4] += iter->second[4];
			non_critical_bucket[5] += iter->second[5];
			non_critical_bucket[6] += iter->second[6];
		}
	

		bool critical_flag[9] = {false};

		if(ips_causing_50_percent_rob_stalls.find(iter->first) != ips_causing_50_percent_rob_stalls.end())
                        critical_flag[0] = true;
		if(ips_causing_55_percent_rob_stalls.find(iter->first) != ips_causing_55_percent_rob_stalls.end())
                        critical_flag[1] = true;
		if(ips_causing_60_percent_rob_stalls.find(iter->first) != ips_causing_60_percent_rob_stalls.end())
                        critical_flag[2] = true;
		if(ips_causing_65_percent_rob_stalls.find(iter->first) != ips_causing_65_percent_rob_stalls.end())
                        critical_flag[3] = true;
		if(ips_causing_70_percent_rob_stalls.find(iter->first) != ips_causing_70_percent_rob_stalls.end())
                        critical_flag[4] = true;
		if(ips_causing_75_percent_rob_stalls.find(iter->first) != ips_causing_75_percent_rob_stalls.end())
			critical_flag[5] = true;
		if(ips_causing_80_percent_rob_stalls.find(iter->first) != ips_causing_80_percent_rob_stalls.end())
			critical_flag[6] = true;
		if(ips_causing_85_percent_rob_stalls.find(iter->first) != ips_causing_85_percent_rob_stalls.end())
			critical_flag[7] = true;
		if(ips_causing_90_percent_rob_stalls.find(iter->first) != ips_causing_90_percent_rob_stalls.end())
			critical_flag[8] = true;

		for(int i = 0; i < 9; i++)
		{
			if(critical_flag[i])
			{
				percent_critical_bucket[i][0] += iter->second[0];
				percent_critical_bucket[i][1] += iter->second[1];
				percent_critical_bucket[i][2] += iter->second[2];
				percent_critical_bucket[i][3] += iter->second[3];
				percent_critical_bucket[i][4] += iter->second[4];
				percent_critical_bucket[i][5] += iter->second[5];
				percent_critical_bucket[i][6] += iter->second[6];	
			}
			else
			{
				percent_non_critical_bucket[i][0] += iter->second[0];
				percent_non_critical_bucket[i][1] += iter->second[1];
				percent_non_critical_bucket[i][2] += iter->second[2];
				percent_non_critical_bucket[i][3] += iter->second[3];
				percent_non_critical_bucket[i][4] += iter->second[4];
				percent_non_critical_bucket[i][5] += iter->second[5];
				percent_non_critical_bucket[i][6] += iter->second[6];
			}
		}

		iter++;

	}	


	for(int i = 0; i <7; i++)
	{
		cout << "Critical_IP_ROB_Stalls_Bucket_" << i <<": " << critical_bucket[i] << endl;
		cout << "Non-critical_IP_ROB_Stalls_Bucket_" << i <<": " << non_critical_bucket[i] << endl;
	}


	cout << "0 => IPs accounting for >90 percent of stalls:" << endl;
	
	/*cout << "percent50: " << percent50 << " num IPs:  " <<ips_causing_50_percent_rob_stalls.size() << endl;
	cout << "percent55: " << percent55 << " num IPs: " <<ips_causing_55_percent_rob_stalls.size() << endl;
	cout << "percent60: " << percent60 << " num IPs: " <<ips_causing_60_percent_rob_stalls.size() << endl;
	cout << "percent65: " << percent65 << " num IPs: " <<ips_causing_65_percent_rob_stalls.size() << endl;
	cout << "percent70: " << percent70 << " num IPs: " <<ips_causing_70_percent_rob_stalls.size() << endl;
	cout << "percent75: " << percent75 << " num IPs: " <<ips_causing_75_percent_rob_stalls.size() << endl;
	cout << "percent80: " << percent80 << " num IPs: " <<ips_causing_80_percent_rob_stalls.size() << endl;
	cout << "percent85: " << percent85 << " num IPs: " <<ips_causing_85_percent_rob_stalls.size() << endl; */
	cout << "percent90: " << percent90 << " num IPs: " <<ips_causing_90_percent_rob_stalls.size() << endl;

	for(int i = 0; i <9; i++)
	{
		for(int j = 0; j < 7; j++)
		{
			cout << "Critical_ROB_Stalls_Percent_" <<i<<"_Bucket_"<<j<<": " << percent_critical_bucket[i][j] << endl;
			cout << "Non-critical_ROB_Stalls_Percent_" <<i<<"_Bucket_"<<j<<": " << percent_non_critical_bucket[i][j] << endl;
		}

	}


	cout << "Average ROB occupancy Load Stall Begin: " << double(double(rob_occu_load_stall_begin[cpu])/double(load_rob_stall_count[cpu])) << endl;
	cout << "Average ROB occupancy Load Stall End: " << double(double(rob_occu_load_stall_end[cpu])/double(load_rob_stall_count[cpu])) << endl;
	cout << "Min ROB occupancy Load Stall Begin: " << min_rob_occu_load_stall_begin[cpu] << endl;
        cout << "Max ROB occupancy Load Stall Begin: " << max_rob_occu_load_stall_begin[cpu] << endl;	
	cout << "Min ROB occupancy Load Stall End: " << min_rob_occu_load_stall_end[cpu] << endl;
        cout << "Max ROB occupancy Load Stall End: " << max_rob_occu_load_stall_end[cpu] << endl;

	cout << "Average ROB occupancy Critical Load Stall Begin: " << double(double(rob_occu_critical_load_stall_begin[cpu])/double(critical_load_rob_stall_count[cpu])) << endl;
	cout << "Average ROB occupancy Critical Load Stall End: " << double(double(rob_occu_critical_load_stall_end[cpu])/double(critical_load_rob_stall_count[cpu])) << endl;

	cout << "Min ROB occupancy Critical Load Stall Begin: " << min_rob_occu_critical_load_stall_begin[cpu] << endl;
	cout << "Max ROB occupancy Critical Load Stall Begin: " << max_rob_occu_critical_load_stall_begin[cpu] << endl;
	cout << "Min ROB occupancy Critical Load Stall End: " << min_rob_occu_critical_load_stall_end[cpu] << endl;
	cout << "Max ROB occupancy Critical Load Stall End: " << max_rob_occu_critical_load_stall_end[cpu] << endl;

	for(int i = 0; i < 7; i++)
	{
		cout << " ROB Load Stall Begin Bucket Count " << i << ": " << rob_load_stall_begin_bucket_count[cpu][i] << endl;
		cout << " ROB Load Stall End Bucket Count " << i << ": " << rob_load_stall_end_bucket_count[cpu][i] << endl;
		cout << " ROB Critical Load Stall Begin Bucket Count " << i << ": " << rob_critical_load_stall_begin_bucket_count[cpu][i] << endl;
		cout << " ROB Critical Load Stall End Bucket Count " << i << ": " << rob_critical_load_stall_end_bucket_count[cpu][i] << endl;
	}

	cout << dec << "PIST Tag Read Accesses: " << pist_tag_read_accesses[cpu] << endl;
	cout << dec << "PIST Read Accesses: " << pist_read_accesses[cpu] << endl;
	cout << dec << "PIST Tag Write Accesses: " << pist_tag_write_accesses[cpu] << endl;
	cout << dec << "PIST Write Accesses: " << pist_write_accesses[cpu] << endl;


	cout << "Max global_stall_metric_count: " << max_global_stall_metric_count << endl;



	cout << "Threshold Modified: " << num_of_times_thresholds_modified <<" times, factor: " << thresholds_modification_factor << endl;
#ifdef SET_ASSOCIATIVE_CRITICAL_IP_CACHE 
	cout << "Critical IP cache stats: " << endl;
	critical_ip_cache.print_stats();
#endif
}


bool CRITICAL_IP_CACHE::check_vacancy(uint64_t ip)
{
	uint32_t index = (ip >> 2) & ((1 << LOG2_NUM_SETS) - 1);
	for(uint32_t way = 0; way < NUM_WAYS; way++)
	{
		//cout << "Valid bit: " << unsigned(valid_bit_array[index][way]) << endl;
		if(valid_bit_array[index][way] == 0)
			return true;
	}
	//cout << "Returngin false " << endl;
	return false;
}

uint32_t CRITICAL_IP_CACHE::insert(uint64_t ip)
{
	uint32_t index = (ip >> 2) & ((1 << LOG2_NUM_SETS) - 1);
	uint64_t tag = (ip >> 2) >> LOG2_NUM_SETS;
	uint32_t way = 0;
	for(way = 0; way < NUM_WAYS; way++)
	{
		tag_read_accesses++;
		if(valid_bit_array[index][way] == 0)
		{
			tag_write_accesses++;
			valid_bit_array[index][way] = 1;
			tag_array[index][way] = tag;
			update_lru(index, way);
			return way;
		}
	}
	if(way == NUM_WAYS)
	{
		for(way = 0; way < NUM_WAYS; way++)
		{
			//cout << "Printing LRU: " << unsigned(lru_array[index][way]) << endl;
			if(lru_array[index][way] == NUM_WAYS-1)
				break;
		}
		if(way >= NUM_WAYS)
		{
			//for(way = 0; way < NUM_WAYS; way++)
			//	cout << "LRU: " << unsigned(lru_array[index][way]) << endl;
			assert(0);
		}
		tag_write_accesses++;
		valid_bit_array[index][way] = 1;
		tag_array[index][way] = tag;
		update_lru(index, way);
	}
	return way;
}

bool CRITICAL_IP_CACHE::find(uint64_t ip)
{
	uint32_t index = (ip >> 2) & ((1 << LOG2_NUM_SETS) - 1);
	uint64_t tag = (ip >> 2) >> LOG2_NUM_SETS;
	for(uint32_t way = 0; way < NUM_WAYS; way++)
	{
		tag_read_accesses++;
		if(valid_bit_array[index][way])
		{
			if(tag_array[index][way] == tag)
			{
				update_lru(index, way);
				return true;
			}
		}
	}
	return false;
}

void CRITICAL_IP_CACHE::update_lru(uint32_t index, uint32_t way)
{
	tag_read_accesses += NUM_WAYS; 
	for(uint32_t temp_way = 0; temp_way < NUM_WAYS; temp_way++)
	{
		if(lru_array[index][temp_way] < lru_array[index][way])
		{
			lru_array[index][temp_way]++;
			tag_write_accesses++;
		}
	}
	lru_array[index][way] = 0;
}

void CRITICAL_IP_CACHE::print_stats()
{
	uint64_t ununsed_entries = 0;
	for(int i = 0; i < NUM_SETS; i++)
		for(int j = 0; j < NUM_WAYS; j++)
			if(!valid_bit_array[i][j])
				ununsed_entries++;

	cout << " Unused entries: " << ununsed_entries << endl;

	cout << "CIC Tag Read Accesses: " << tag_read_accesses << endl;
	cout << "CIC Read Accesses: " << read_accesses << endl;
	cout << "CIC Tag Write Accesses: " << tag_write_accesses << endl;
	cout << "CIC Write Accesses: " << write_accesses << endl;

}

uint8_t CRITICAL_IP_CACHE::get_confidence(uint64_t ip)
{
	uint64_t index = (ip >> 2) & ((1 << LOG2_NUM_SETS) - 1);
        uint64_t tag = (ip >> 2) >> LOG2_NUM_SETS;
        for(uint32_t way = 0; way < NUM_WAYS; way++)
        {
                if(valid_bit_array[index][way])
                {
                        if(tag_array[index][way] == tag)
			{
				return confidence[index][way];
			}
		}
	}
	return 0;   
}

void CRITICAL_IP_CACHE::update_confidence(uint64_t ip)
{
	uint64_t index = (ip >> 2) & ((1 << LOG2_NUM_SETS) - 1);
        uint64_t tag = (ip >> 2) >> LOG2_NUM_SETS;
        for(uint32_t way = 0; way < NUM_WAYS; way++)
        {
                if(valid_bit_array[index][way])
                {
                        if(tag_array[index][way] == tag)
                        {
                                if(confidence[index][way] < 3)
					confidence[index][way] = confidence[index][way] + 1; 
                        }
                }
        }
}

