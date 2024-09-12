#include "champsim.h" 
#include "instruction.h"
#include "criticality_detector_config.h"


// ------------------------------------------ ROB Occupancy/Stall Cycle based Counter Criticality Detection Technique ---------------------- 

extern uint64_t global_stall_metric_count[NUM_CPUS];
extern uint32_t log2_training_table_num_sets;

extern uint64_t pist_tag_read_accesses[NUM_CPUS], pist_read_accesses[NUM_CPUS], pist_tag_write_accesses[NUM_CPUS], pist_write_accesses[NUM_CPUS];

#ifdef SET_ASSOCIATIVE_CRITICAL_IP_CACHE
extern CRITICAL_IP_CACHE critical_ip_cache;
#endif

extern int thresholds_modification_factor, current_epoch_all_ip_prefetch;
extern set<uint64_t> critical_ips[NUM_CPUS], rob_stalling_load_ips[NUM_CPUS];

struct per_ip_stall_table
{
	        uint64_t stall_count;
		        uint32_t lru;
};

#if defined ROB_OCCUPANCY_BASED_CRITICALITY || defined FOCUSED_PREFETCHING_COUNTING_CLASSIFIER
map<uint64_t, struct per_ip_stall_table> per_ip_stall_count[NUM_CPUS][NUM_OF_CRITICAL_IP_TRAINING_TABLE_SETS];
#endif 

//Neelu: Critical IP Detection Thresholds.
extern uint32_t candidacy_threshold, static_reliance_threshold;

void counter_criticality_detector_operate(uint64_t ip, uint64_t stall_metric_count, uint8_t cpu)
{
#if defined ROB_OCCUPANCY_BASED_CRITICALITY || defined FOCUSED_PREFETCHING_COUNTING_CLASSIFIER

        //The following steps are common for confidence and counter based.
        //Step 0: if IP is already critical, return.
        //Step 1: if stall_metric_count > threshold, then add to per_ip_stall_count. Use replacement if required.
        //Step 2: check if ip is ready to be classfied as critical. If critical, then remove from per_ip_stall_count and insert into critical ips.

        //Step 0

#ifdef SET_ASSOCIATIVE_CRITICAL_IP_CACHE

        if(critical_ip_cache.find(ip) || !(warmup_complete[cpu]) || current_epoch_all_ip_prefetch == -1)
                return;

#else

        if(critical_ips[cpu].find(ip) != critical_ips[cpu].end() || !(warmup_complete[cpu]) || current_epoch_all_ip_prefetch == -1)
                return;

#endif

        if(rob_stalling_load_ips[cpu].find(ip) == rob_stalling_load_ips[cpu].end())
                rob_stalling_load_ips[cpu].insert(ip);

        uint64_t victim_region = 0;
        //Step 1
        uint64_t set_index = (ip >> 2) & ((1 << log2_training_table_num_sets)-1);

	if(stall_metric_count > (candidacy_threshold+(thresholds_modification_factor*CANDIDACY_THRESHOLD_SUBTRAHEND)))
        {
                //Neelu: Incrementing tag reads (find) and tag writes (lru update) by associativity, write by one.
                pist_tag_read_accesses[cpu] += NUM_OF_CRITICAL_IP_TRAINING_TABLE_WAYS;
                pist_write_accesses[cpu]++;
                pist_tag_write_accesses[cpu] += NUM_OF_CRITICAL_IP_TRAINING_TABLE_WAYS;

                if(per_ip_stall_count[cpu][set_index].find(ip) != per_ip_stall_count[cpu][set_index].end() && (per_ip_stall_count[cpu][set_index].size() > 0))
                {

                        per_ip_stall_count[cpu][set_index][ip].stall_count += stall_metric_count;
                        map<uint64_t, struct per_ip_stall_table>::iterator iter = per_ip_stall_count[cpu][set_index].begin();
                        while(iter != per_ip_stall_count[cpu][set_index].end())
                        {
                                if(iter->second.lru < per_ip_stall_count[cpu][set_index][ip].lru)
                                        iter->second.lru++;

                                iter++;
                        }
                        per_ip_stall_count[cpu][set_index][ip].lru = 0;

                }
		else
                {
                        map<uint64_t, struct per_ip_stall_table>::iterator iter = per_ip_stall_count[cpu][set_index].begin();
                        if(per_ip_stall_count[cpu][set_index].size() > 0)
                        {
                                while(iter != per_ip_stall_count[cpu][set_index].end())
                                {
                                        if(iter->second.lru == (NUM_OF_CRITICAL_IP_TRAINING_TABLE_WAYS-1))
                                        {
                                                victim_region = iter->first;
                                        }
                                        iter->second.lru++;

                                        iter++;
                                }
                        }
                        per_ip_stall_count[cpu][set_index][ip].stall_count = stall_metric_count;
                        per_ip_stall_count[cpu][set_index][ip].lru = 0;
                }
                global_stall_metric_count[cpu] += stall_metric_count;


        }
        else
                return;

        //Step 2

        pist_read_accesses[cpu]++;

        if(per_ip_stall_count[cpu][set_index][ip].stall_count > (static_reliance_threshold+(thresholds_modification_factor*STATIC_RELIANCE_THRESHOLD_SUBTRAHEND)) && (per_ip_stall_count[cpu][set_index][ip].stall_count > (global_stall_metric_count[cpu]/NUM_OF_IPS_IN_CRITICAL_IP_TRAINING)))
        {
#ifdef SET_ASSOCIATIVE_CRITICAL_IP_CACHE
                uint32_t inserted_way = UINT32_MAX;
                inserted_way = critical_ip_cache.insert(ip);
                critical_ips[cpu].insert(ip);
                assert(inserted_way != UINT32_MAX);

#else
                //Neelu: Inserting in critical_ips only if the size is less than max and if thresholds have not been modified, otherwise inserting in lower_threshold_critical IPs.
                critical_ips[cpu].insert(ip);

                map<uint64_t, struct per_ip_stall_table>::iterator iter = per_ip_stall_count[cpu][set_index].begin();
                while(iter != per_ip_stall_count[cpu][set_index].end())
                {
                        if(iter->second.lru > per_ip_stall_count[cpu][set_index][ip].lru)
                                iter->second.lru--;

                        iter++;
                }
                per_ip_stall_count[cpu][set_index].erase(ip);
#endif
                //Neelu: Check if lower_threshold_critical_IPs max size is reached.        
	} 

	//Evict if number of entry is greater than size.
        if(per_ip_stall_count[cpu][set_index].size() > NUM_OF_CRITICAL_IP_TRAINING_TABLE_WAYS)
        {
                per_ip_stall_count[cpu][set_index].erase(victim_region);
        }

        assert(per_ip_stall_count[cpu][set_index].size() <= NUM_OF_CRITICAL_IP_TRAINING_TABLE_WAYS);
        return;
#endif
}



// ------------------------------------------ CATCH Criticality Detection Technique ---------------------- 

// Buffering 2x ROB instructions in graph.
// Walking through critical path in buffered graph: record PCs hitting in L2/LLC.
// D: Issue, E: Execution, C: Write-back. 
// Types of edges: D-D, D-E, E-D, E-E, E-C, C-C, C-D
//
// D-D: in-order allocation. on allocation, curr_core_cycle - last_instr_allocated_cycle. (global variable: last_instr_allocated_cycle)
// D-E: renaming latency. starting execution - instr_allocated_cycle. (per instr variables: execution_begin_cycle, instr_allocated_cycle) 
// E-D: branch misprediction, execution start of branch, till correct instr is issued. (execution_begin_cycle, instr_allocated_cycle[branch+1]) 
// E-E: data dependencies. source execution start time - dependent execution start time. (execution_begin_cycle)  
// E-C edge: dispatch to execution units till it does write-back. (weight = execution time/8, stored as a 5 bit saturating counter) 
// C-C: Commit of next instr - commit of last instr, (global: last_instr_commit_cycle)
// C-D: if ROB is full => instr cannot be allocated, (per instr: instr_allocated - instr_ready_for_allocation, rob_full_when_ready_flag) 
// 
// longest path from D of first to C of last: critical path


struct graph_instr
{
	uint32_t instr_id = 0;
	uint64_t cost[3] = {0}; 
	uint64_t prev_node_id[3] = {0};	//0, 1, or 2. (D, E, or C).
	uint64_t prev_instr_id[3] = {0};
        bool load = false; 	
	uint64_t ip = 0;
        uint64_t execution_completed_timestamp = 0; 
	uint64_t execution_inflight_timestamp = 0; 
	uint64_t instr_commit_timestamp = 0; 	
	uint8_t destination_registers[NUM_INSTR_DESTINATIONS_SPARC] = {0};	
};

#define GRAPH_BUFFER_SIZE ROB_SIZE*2

struct graph_instr buffered_graph[NUM_CPUS][GRAPH_BUFFER_SIZE];
uint64_t instr_in_graph[NUM_CPUS] = {0}, last_instr_allocation_cycle[NUM_CPUS] = {0};
bool last_instr_is_mispredicted_branch[NUM_CPUS] = {false};

// 32 entry, 8 way critical IP table with confidence counter 

extern set<uint64_t> critical_ips[NUM_CPUS];

extern uint64_t critical_ip_candidate_identification_operations;

#ifdef CATCH_CRITICALITY

void catch_criticality_detector_operate(uint64_t cpu)
{
	//cout << "CATCH Criticality Detector Operate " << endl; 
	//This means buffering of 2x ROB_SIZE instructions is complete, so walk the graph and train for critical IPs.

	//First index to traverse is [ROB_SIZE*2-1] <- commit node of last instruction
	
	int64_t prev_instr_index = GRAPH_BUFFER_SIZE-1;
	int64_t prev_node_id = 2;	//starting with C node 

	uint64_t count = 0; 
	while(prev_instr_index >= 0)
	{
		if(buffered_graph[cpu][prev_instr_index].load && prev_node_id == 1)	//only for E node
		{
			critical_ip_candidate_identification_operations++; 	//traversing through loads, as CATCH mentions they optimize to store previous load in prev_node. 
			//cout << "Load IP on critical path." << buffered_graph[cpu][prev_instr_index].ip << endl; 
			//check if IP is already in CIC, update confidence 
			if(critical_ip_cache.find(buffered_graph[cpu][prev_instr_index].ip))
			{
				critical_ip_cache.update_confidence(buffered_graph[cpu][prev_instr_index].ip);
				//cout << "updating" << endl;
			}
			else //insert into CIC and update confidence 
			{
				//cout << "inserting" << endl;
				critical_ip_cache.insert(buffered_graph[cpu][prev_instr_index].ip);
				//cout << "updating conf." << endl; 
				critical_ip_cache.update_confidence(buffered_graph[cpu][prev_instr_index].ip);
			}	
		}

		//cout << "outside" << endl; 

		prev_instr_index = buffered_graph[cpu][prev_instr_index].prev_instr_id[prev_node_id];
		prev_node_id = buffered_graph[cpu][prev_instr_index].prev_node_id[prev_node_id]; 

		count++; 

		//cout << "prev_node_id: " << prev_node_id << " prev_instr_index: " << prev_instr_index << endl; 

		if(count > GRAPH_BUFFER_SIZE)
			break; 
	}

}

bool catch_check_critical(uint64_t ip, uint64_t cpu)
{
	if(critical_ip_cache.find(ip))
	{
		if(critical_ip_cache.get_confidence(ip) == 3)
		{
			critical_ips[cpu].insert(ip);
			return true;
		}
	}	
	return false;
}

void catch_record_instruction(ooo_model_instr retiring_instruction, uint8_t cpu)
{
	//For node D: D, E, C are possible incoming edges.
	uint64_t dd_cost = 0, ed_cost = 0, cd_cost = 0; 

	if(instr_in_graph[cpu] > 0)
	{
		critical_ip_candidate_identification_operations++;
		dd_cost = buffered_graph[cpu][instr_in_graph[cpu]-1].cost[0] + (retiring_instruction.add_to_rob_timestamp - last_instr_allocation_cycle[cpu]);
		if(last_instr_is_mispredicted_branch[cpu])
		{
			//This means comparison will be needed
			critical_ip_candidate_identification_operations+=2;
			ed_cost = buffered_graph[cpu][instr_in_graph[cpu]-1].cost[1] + (buffered_graph[cpu][instr_in_graph[cpu]-1].execution_completed_timestamp - buffered_graph[cpu][instr_in_graph[cpu]-1].execution_inflight_timestamp); 
		}
		if(retiring_instruction.rob_full_on_add_to_rob_attempt)
		{
			//This means comparison will be needed
			critical_ip_candidate_identification_operations+=2;
			cd_cost = buffered_graph[cpu][instr_in_graph[cpu]-ROB_SIZE].cost[2] + (buffered_graph[cpu][instr_in_graph[cpu]-ROB_SIZE].instr_commit_timestamp - retiring_instruction.add_to_rob_timestamp);
		}
	}

	if(dd_cost >= ed_cost)
	{
		if(dd_cost >= cd_cost)
		{
			buffered_graph[cpu][instr_in_graph[cpu]].cost[0] = dd_cost;
			buffered_graph[cpu][instr_in_graph[cpu]].prev_node_id[0] = 0;
			buffered_graph[cpu][instr_in_graph[cpu]].prev_instr_id[0] = instr_in_graph[cpu]-1;
		}
		else
		{
			buffered_graph[cpu][instr_in_graph[cpu]].cost[0] = cd_cost;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_node_id[0] = 2;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_instr_id[0] = instr_in_graph[cpu]-ROB_SIZE;
		}
	}
	else
	{
		if(ed_cost >= cd_cost)
                {
                        buffered_graph[cpu][instr_in_graph[cpu]].cost[0] = ed_cost;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_node_id[0] = 1;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_instr_id[0] = instr_in_graph[cpu]-1;
                }
                else
                {
                        buffered_graph[cpu][instr_in_graph[cpu]].cost[0] = cd_cost;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_node_id[0] = 2;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_instr_id[0] = instr_in_graph[cpu]-ROB_SIZE;
                }
	}


	//For node E: D, E are possible incoming edges.
	
	uint64_t de_cost = 0, ee_cost = 0;

        if(instr_in_graph[cpu] > 0)
        {
		critical_ip_candidate_identification_operations++;
                de_cost = retiring_instruction.execution_inflight_timestamp - retiring_instruction.add_to_rob_timestamp;
               
		int src_instr_index = -1;

		for (uint32_t i=0; i<NUM_INSTR_SOURCES; i++)                                                                                                                                {
			if(retiring_instruction.source_registers[i])
			{
				int64_t temp_instr_in_graph = instr_in_graph[cpu]-1;
				while(temp_instr_in_graph >= 0)
				{
					for(int j = 0; j < NUM_INSTR_DESTINATIONS_SPARC; j++)
					{
						if(buffered_graph[cpu][temp_instr_in_graph].destination_registers[j] == retiring_instruction.source_registers[i])
						{
							src_instr_index = temp_instr_in_graph;
							break;
						}
					}
					if(src_instr_index >= 0)
						break;

					temp_instr_in_graph--; 
				}
			}
			if(src_instr_index >= 0)
				break;
		}

		if(src_instr_index >= 0)
		{
			critical_ip_candidate_identification_operations+=2;
			ee_cost = buffered_graph[cpu][src_instr_index].execution_completed_timestamp - buffered_graph[cpu][src_instr_index].execution_inflight_timestamp;
		}

                if(de_cost >= ee_cost)
                {
                        buffered_graph[cpu][instr_in_graph[cpu]].cost[1] = de_cost;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_node_id[1] = 0;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_instr_id[1] = instr_in_graph[cpu];
                }
                else
                {
                        buffered_graph[cpu][instr_in_graph[cpu]].cost[1] = ee_cost;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_node_id[1] = 1;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_instr_id[1] = src_instr_index;
                }

        }
	
	//For node C: E, C are possible incoming edges.
	uint64_t ec_cost = 0, cc_cost = 0;

        if(instr_in_graph[cpu] > 0)
        {	
		critical_ip_candidate_identification_operations+=3;	//one comparison, two counts(subtract)
		cc_cost = retiring_instruction.instr_commit_timestamp - buffered_graph[cpu][instr_in_graph[cpu]-1].instr_commit_timestamp;
		ec_cost = retiring_instruction.instr_commit_timestamp - retiring_instruction.execution_inflight_timestamp;

		if(ec_cost >= cc_cost)
                {
                        buffered_graph[cpu][instr_in_graph[cpu]].cost[2] = ec_cost;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_node_id[2] = 1;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_instr_id[2] = instr_in_graph[cpu];
                }
                else
                {
                        buffered_graph[cpu][instr_in_graph[cpu]].cost[2] = cc_cost;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_node_id[2] = 2;
                        buffered_graph[cpu][instr_in_graph[cpu]].prev_instr_id[2] = instr_in_graph[cpu]-1;
                }		

	}

	//Updating variables
	last_instr_allocation_cycle[cpu] = retiring_instruction.add_to_rob_timestamp;
	last_instr_is_mispredicted_branch[cpu] = retiring_instruction.is_branch && retiring_instruction.branch_mispredicted;
	
	buffered_graph[cpu][instr_in_graph[cpu]].execution_inflight_timestamp = retiring_instruction.execution_inflight_timestamp;
	buffered_graph[cpu][instr_in_graph[cpu]].execution_completed_timestamp = retiring_instruction.execution_completed_timestamp;
	buffered_graph[cpu][instr_in_graph[cpu]].instr_commit_timestamp = retiring_instruction.instr_commit_timestamp;

	buffered_graph[cpu][instr_in_graph[cpu]].ip = retiring_instruction.ip;

	for (uint32_t i=0; i<NUM_INSTR_SOURCES; i++) 
	{
		if(retiring_instruction.lq_index[i] != UINT32_MAX)
		{
			buffered_graph[cpu][instr_in_graph[cpu]].load = true;
			break;
		}
	} 

	//buffered_graph[cpu][instr_in_graph[cpu]].load = retiring_instruction.l2_or_llc_hit; 

	for(int i = 0; i < NUM_INSTR_DESTINATIONS_SPARC; i++)
		buffered_graph[cpu][instr_in_graph[cpu]].destination_registers[i] = retiring_instruction.destination_registers[i];

	instr_in_graph[cpu]++;

	if(instr_in_graph[cpu] == GRAPH_BUFFER_SIZE)
	{
		catch_criticality_detector_operate(cpu);
		instr_in_graph[cpu] = 0; 
	}
}

#endif

// ------------------------------------------ CATCH Criticality Detection Technique END ----------------------



// ------------------------------------------ Focused Value Prediction Criticality Detection Technique ---------------------- 

#define NUM_CIT_ENTRIES 32 
#define LOG2_NUM_CIT_ENTRIES 5
#define NUM_IP_TAG_BITS 11

//CIT:
 
uint32_t CIT_IP_tag[NUM_CPUS][NUM_CIT_ENTRIES] = {0};
uint32_t CIT_Confidence[NUM_CPUS][NUM_CIT_ENTRIES] = {0}; 
uint32_t CIT_Saturation[NUM_CPUS][NUM_CIT_ENTRIES] = {0};
uint32_t fvp_dynamic_critical_instances[NUM_CPUS] = {0};
std::set<uint64_t> fvp_unique_critical_ips[NUM_CPUS];

void focused_value_prediction_criticality_detector_operate(uint64_t ip, uint64_t cpu)
{
	//CIT:
	//Step 1: Call criticality detector if the load is executing when it is within the retire width.
	
	uint64_t cit_index = (ip >> 2) & ((1 << LOG2_NUM_CIT_ENTRIES) - 1);
	uint64_t cit_ip_tag = ((ip >> 2 ) >> LOG2_NUM_CIT_ENTRIES) & NUM_IP_TAG_BITS; 

	//Check if tag is matching, if yes, increase confidence and saturation, else, if saturation is 0, replace, else reduce saturation. 
	
	if(CIT_IP_tag[cpu][cit_index] == cit_ip_tag)
	{
		if(CIT_Confidence[cpu][cit_index] < 3)
			CIT_Confidence[cpu][cit_index]++;

		if(CIT_Saturation[cpu][cit_index] < 3)
			CIT_Saturation[cpu][cit_index]++;
	}
	else if(CIT_Saturation[cpu][cit_index] == 0)
	{
		CIT_IP_tag[cpu][cit_index] = cit_ip_tag;
		CIT_Saturation[cpu][cit_index] = 1;
		CIT_Confidence[cpu][cit_index] = 1;
	}
	else
	{
		assert(CIT_Saturation[cpu][cit_index]>0);
		CIT_Saturation[cpu][cit_index]--;
	}

}	

bool focused_value_prediction_check_critical(uint64_t ip, uint64_t cpu)
{
	uint64_t cit_index = (ip >> 2) & ((1 << LOG2_NUM_CIT_ENTRIES) - 1);                                                                                                         uint64_t cit_ip_tag = ((ip >> 2 ) >> LOG2_NUM_CIT_ENTRIES) & NUM_IP_TAG_BITS; 

	if(CIT_Confidence[cpu][cit_index] == 3 && CIT_IP_tag[cpu][cit_index] == cit_ip_tag)
	{
		fvp_dynamic_critical_instances[cpu]++;
		fvp_unique_critical_ips[cpu].insert(ip); 
		return true;
	}
	return false;
}

// ----------------------------- FVP CD End -----------------------------------------------



// ------------------------------------------ HPCA 2009 Criticality Detection Technique by Subramaniam et al. -------------------- 

//HPCA_2009_CRITICALITY Begin 

#define NUM_CLPT_ENTRIES 1024
#define LOG2_NUM_CLPT_ENTRIES 10

#define CRITICALITY_THRESHOLD 5

uint64_t critical_dynamic_instances[NUM_CPUS] = {0};
std::set<uint64_t> unique_critical_ips[NUM_CPUS]; 

//CLPT: 
uint32_t CLPT_Prev_Consumer_Count[NUM_CPUS][NUM_CLPT_ENTRIES] = {0};
uint32_t CLPT_Confidence[NUM_CPUS][NUM_CLPT_ENTRIES] = {0};

void hpca_2009_criticality_detector_operate(uint64_t ip, uint64_t consumer_count, uint8_t cpu)
{
	//CCL:
	//Step 1: Capture the destination of all the loads in the ROB.
	//Step 2: For all instructions after the load, check if the source is same as the load's destination, the load will have these many direct dependents.  
	//Step 3: When the load retires, if the issue rate is low, call the detector, which will clsssify loads as critical based on thresholds.

	//CLPT: 
	uint32_t clpt_index = (ip >> 2) & ((1 << LOG2_NUM_CLPT_ENTRIES) - 1);

	//Update Confidence, and Prev_Consumer_Count
	if(abs(CLPT_Prev_Consumer_Count[cpu][clpt_index]-consumer_count) < 2)
		CLPT_Confidence[cpu][clpt_index]++;

	if(CLPT_Confidence[cpu][clpt_index] > 3)
		CLPT_Confidence[cpu][clpt_index] = 3;

	CLPT_Prev_Consumer_Count[cpu][clpt_index] = consumer_count;	
	
}

//Only called when issue rate is low.
bool hpca_2009_check_critical(uint64_t ip, uint8_t cpu)
{
	uint32_t clpt_index = (ip >> 2) & ((1 << LOG2_NUM_CLPT_ENTRIES) - 1);

	if(CLPT_Confidence[cpu][clpt_index] <= 1 || CLPT_Prev_Consumer_Count[cpu][clpt_index] > CRITICALITY_THRESHOLD)
	{
		critical_dynamic_instances[cpu]++;
		unique_critical_ips[cpu].insert(ip);
		return true;
	}
	
	return false;
}

//HPCA_2009_CRITICALITY End


//FDP Prefetcher Throttler 
/*
uint64_t fdp_prefetch_throttler_operate(uint64_t accuracy_counter, uint64_t lateness_counter, uint64_t pollution_counter, uint64_t dynamic_config_counter)
{
	if(accuracy_counter >= 0.75)
	{
		//high accuracy
		if(lateness_counter >= 0.01)
		{
			return (dynamic_config_counter+1 < 5)?dynamic_config_counter+1 : 5; 
		}
		else if(pollution_counter >= 0.005)
		{
			return (dynamic_config_counter-1 > 1)?dynamic_config_counter-1 : 1;
		}

	}
	else if(accuracy_counter >= 0.4)
	{
		//medium accuracy
		if(pollution_counter >= 0.005)
                {
                        return (dynamic_config_counter-1 > 1)?dynamic_config_counter-1 : 1;
                }
		else if(lateness_counter >= 0.01)
                {
                        return (dynamic_config_counter+1 < 5)?dynamic_config_counter+1 : 5;
                }
	}
	else
	{
		//low accuracy
		if(lateness_counter >= 0.01 || pollution_counter >= 0.005)
		{
			return (dynamic_config_counter-1 > 1)?dynamic_config_counter-1 : 1; 
		}
	}

}
*/
//FDP Prefetcher Throttler end




