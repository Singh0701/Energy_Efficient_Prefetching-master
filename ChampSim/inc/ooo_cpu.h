#ifndef OOO_CPU_H
#define OOO_CPU_H

#include "cache.h"
#include "page_table_walker.h"

#ifdef CRC2_COMPILE
#define STAT_PRINTING_PERIOD 1000000
#else
#define STAT_PRINTING_PERIOD 10000000
#endif
#define DEADLOCK_CYCLE 1000000

using namespace std;

// CORE PROCESSOR
#define FETCH_WIDTH 6
#define DECODE_WIDTH 6
#define EXEC_WIDTH 6
#define LQ_WIDTH 2
#define SQ_WIDTH 2
#define RETIRE_WIDTH 4
#define SCHEDULER_SIZE 128
#define BRANCH_MISPREDICT_PENALTY 1 //@Vishal: Updated from 20 to be same as new ChampSim, anyway penalty is simulated because of queues
//#define SCHEDULING_LATENCY 6
//#define EXEC_LATENCY 1

#define STA_SIZE (ROB_SIZE*NUM_INSTR_DESTINATIONS_SPARC)

#define BTB_SET 1024
#define BTB_WAY 4

extern uint32_t SCHEDULING_LATENCY, EXEC_LATENCY, DECODE_LATENCY;
extern uint8_t TRACE_ENDS_STOP;



// cpu
class O3_CPU {
  public:
    uint32_t cpu;

    // trace
    FILE *trace_file;
    char trace_string[1024];
    char gunzip_command[1024];
    int context_switch, operating_index;

    // instruction
    input_instr next_instr;
    input_instr current_instr;
    cloudsuite_instr current_cloudsuite_instr;
    uint64_t instr_unique_id, completed_executions, 
             begin_sim_cycle, begin_sim_instr, 
             last_sim_cycle, last_sim_instr,
             finish_sim_cycle, finish_sim_instr,
             warmup_instructions, simulation_instructions, instrs_to_read_this_cycle, instrs_to_fetch_this_cycle,
             next_print_instruction, num_retired;
    uint32_t inflight_reg_executions, inflight_mem_executions, num_searched;
    uint32_t next_ITLB_fetch;

    // reorder buffer, load/store queue, register file
    CORE_BUFFER IFETCH_BUFFER{"IFETCH_BUFFER", FETCH_WIDTH*2};
    CORE_BUFFER DECODE_BUFFER{"DECODE_BUFFER", DECODE_WIDTH*3};
    CORE_BUFFER ROB{"ROB", ROB_SIZE};
    LOAD_STORE_QUEUE LQ{"LQ", LQ_SIZE}, SQ{"SQ", SQ_SIZE};
    
    // store array, this structure is required to properly handle store instructions
    uint64_t STA[STA_SIZE], STA_head, STA_tail; 

    // Ready-To-Execute
    uint32_t RTE0[ROB_SIZE], RTE0_head, RTE0_tail, 
             RTE1[ROB_SIZE], RTE1_head, RTE1_tail;  

    // Ready-To-Load
    uint32_t RTL0[LQ_SIZE], RTL0_head, RTL0_tail, 
             RTL1[LQ_SIZE], RTL1_head, RTL1_tail;  

    // Ready-To-Store
    uint32_t RTS0[SQ_SIZE], RTS0_head, RTS0_tail,
             RTS1[SQ_SIZE], RTS1_head, RTS1_tail;

    // branch
    int branch_mispredict_stall_fetch; // flag that says that we should stall because a branch prediction was wrong
    int mispredicted_branch_iw_index; // index in the instruction window of the mispredicted branch.  fetch resumes after the instruction at this index executes
    uint8_t  fetch_stall;
    uint64_t fetch_resume_cycle;
    uint64_t num_branch, branch_mispredictions;
    uint64_t total_rob_occupancy_at_branch_mispredict;
	uint64_t total_branch_types[8];

    //@Vishal
    uint64_t sim_RAW_hits, roi_RAW_hits, //Loads hitting in Store queue
	     sim_load_gen, roi_load_gen, //Loads generated by cpu
	     sim_load_sent, roi_load_sent, //Loads sent to L1D cache
    	     sim_store_gen, roi_store_gen, //Stores generated by cpu
             sim_store_sent, roi_store_sent; //Stores sent to L1D cache

    // TLBs and caches
    CACHE ITLB{"ITLB", ITLB_SET, ITLB_WAY, ITLB_SET*ITLB_WAY, ITLB_WQ_SIZE, ITLB_RQ_SIZE, ITLB_PQ_SIZE, ITLB_MSHR_SIZE},
          DTLB{"DTLB", DTLB_SET, DTLB_WAY, DTLB_SET*DTLB_WAY, DTLB_WQ_SIZE, DTLB_RQ_SIZE, DTLB_PQ_SIZE, DTLB_MSHR_SIZE},
          STLB{"STLB", STLB_SET, STLB_WAY, STLB_SET*STLB_WAY, STLB_WQ_SIZE, STLB_RQ_SIZE, STLB_PQ_SIZE, STLB_MSHR_SIZE},
          L1I{"L1I", L1I_SET, L1I_WAY, L1I_SET*L1I_WAY, L1I_WQ_SIZE, L1I_RQ_SIZE, L1I_PQ_SIZE, L1I_MSHR_SIZE},
          L1D{"L1D", L1D_SET, L1D_WAY, L1D_SET*L1D_WAY, L1D_WQ_SIZE, L1D_RQ_SIZE, L1D_PQ_SIZE, L1D_MSHR_SIZE},
          L2C{"L2C", L2C_SET, L2C_WAY, L2C_SET*L2C_WAY, L2C_WQ_SIZE, L2C_RQ_SIZE, L2C_PQ_SIZE, L2C_MSHR_SIZE},
		  BTB{"BTB", BTB_SET, BTB_WAY, BTB_SET*BTB_WAY, 0, 0, 0, 0};

    #ifdef PUSH_DTLB_PB
    CACHE DTLB_PB{"DTLB_PB", DTLB_PB_SET, DTLB_PB_WAY, DTLB_PB_SET*DTLB_PB_WAY, DTLB_PB_WQ_SIZE, DTLB_PB_RQ_SIZE, DTLB_PB_PQ_SIZE, DTLB_PB_MSHR_SIZE};
    #endif

    PAGE_TABLE_WALKER PTW{"PTW"}; 

    // constructor
    O3_CPU() {
        cpu = 0;

        // trace
        trace_file = NULL;
	context_switch = 0;
	operating_index = -1;
	

        // instruction
        instr_unique_id = 0;
        completed_executions = 0;
        begin_sim_cycle = 0;
        begin_sim_instr = 0;
        last_sim_cycle = 0;
        last_sim_instr = 0;
        finish_sim_cycle = 0;
        finish_sim_instr = 0;
        warmup_instructions = 0;
        simulation_instructions = 0;
        instrs_to_read_this_cycle = 0;
        instrs_to_fetch_this_cycle = 0;

        next_print_instruction = STAT_PRINTING_PERIOD;
        num_retired = 0;

        inflight_reg_executions = 0;
        inflight_mem_executions = 0;
        num_searched = 0;

        next_ITLB_fetch = 0;

        // branch
        branch_mispredict_stall_fetch = 0;
        mispredicted_branch_iw_index = 0;
        fetch_stall = 0;
	fetch_resume_cycle = 0;
        num_branch = 0;
        branch_mispredictions = 0;

       	for(uint32_t i=0; i<8; i++)
		total_branch_types[i] = 0;

        for (uint32_t i=0; i<STA_SIZE; i++)
            STA[i] = UINT64_MAX;
        STA_head = 0;
        STA_tail = 0;

        for (uint32_t i=0; i<ROB_SIZE; i++) {
            RTE0[i] = ROB_SIZE;
            RTE1[i] = ROB_SIZE;
        }
        RTE0_head = 0;
        RTE1_head = 0;
        RTE0_tail = 0;
        RTE1_tail = 0;

        for (uint32_t i=0; i<LQ_SIZE; i++) {
            RTL0[i] = LQ_SIZE;
            RTL1[i] = LQ_SIZE;
        }
        RTL0_head = 0;
        RTL1_head = 0;
        RTL0_tail = 0;
        RTL1_tail = 0;

        for (uint32_t i=0; i<SQ_SIZE; i++) {
            RTS0[i] = SQ_SIZE;
            RTS1[i] = SQ_SIZE;
        }
        RTS0_head = 0;
        RTS1_head = 0;
        RTS0_tail = 0;
        RTS1_tail = 0;
    }

    // functions
    void read_from_trace(),
	    //handle_branch(), Neelu: Now it is read_from_trace.
         fetch_instruction(),
	 decode_and_dispatch(),
         schedule_instruction(),
         execute_instruction(),
         schedule_memory_instruction(),
         execute_memory_instruction(),
         do_scheduling(uint32_t rob_index),  
         reg_dependency(uint32_t rob_index),
         do_execution(uint32_t rob_index),
         do_memory_scheduling(uint32_t rob_index),
         operate_lsq(),
         complete_execution(uint32_t rob_index),
         reg_RAW_dependency(uint32_t prior, uint32_t current, uint32_t source_index),
         reg_RAW_release(uint32_t rob_index),
         mem_RAW_dependency(uint32_t prior, uint32_t current, uint32_t data_index, uint32_t lq_index),
         //handle_o3_fetch(PACKET *current_packet, uint32_t cache_type), //@Vishal: This function is not used anywhere
         handle_merged_translation(PACKET *provider), 
         handle_merged_load(PACKET *provider),
         release_load_queue(uint32_t lq_index),
         complete_instr_fetch(PACKET_QUEUE *queue, uint8_t is_it_tlb),
         complete_data_fetch(PACKET_QUEUE *queue, uint8_t is_it_tlb);

    void initialize_core();
    void core_final_stats();
    void add_load_queue(uint32_t rob_index, uint32_t data_index),
         add_store_queue(uint32_t rob_index, uint32_t data_index),
         execute_store(uint32_t rob_index, uint32_t sq_index, uint32_t data_index);
    int  execute_load(uint32_t rob_index, uint32_t sq_index, uint32_t data_index);
    void check_dependency(int prior, int current);
    void operate_cache();
    void update_rob();
    void retire_rob();

    uint32_t  add_to_rob(ooo_model_instr *arch_instr),
              check_rob(uint64_t instr_id);

	uint32_t add_to_ifetch_buffer(ooo_model_instr *arch_instr);
	uint32_t add_to_decode_buffer(ooo_model_instr *arch_instr);

    uint32_t check_and_add_lsq(uint32_t rob_index);

    // branch predictor
    uint8_t predict_branch(uint64_t ip);
    void    initialize_branch_predictor(),
            last_branch_result(uint64_t ip, uint8_t taken); 

     void l1i_prefetcher_initialize();
     void l1i_prefetcher_branch_operate(uint64_t ip, uint8_t branch_type, uint64_t branch_target);
     void l1i_prefetcher_cache_operate(uint64_t v_addr, uint8_t cache_hit, uint8_t prefetch_hit);
     void l1i_prefetcher_cycle_operate();
     void l1i_prefetcher_cache_fill(uint64_t v_addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_v_addr);
     void l1i_prefetcher_final_stats();
     int prefetch_code_line(uint64_t pf_v_addr);

void fill_btb(uint64_t trigger, uint64_t target);

};

extern O3_CPU ooo_cpu[NUM_CPUS];

void hpca_2009_criticality_detector_operate(uint64_t ip, uint64_t consumer_count, uint8_t cpu);
bool hpca_2009_check_critical(uint64_t ip); 


//Neelu: Adding a simple class for critical ips. 
class CRITICAL_IP_CACHE
{
	public:
	uint32_t NUM_SETS, NUM_WAYS, LOG2_NUM_SETS;
	uint64_t **tag_array;
	uint8_t **valid_bit_array;
	uint8_t **lru_array; 	
	uint8_t **confidence;	//only used in CATCH 

	uint64_t read_accesses, tag_read_accesses, write_accesses, tag_write_accesses; 

	CRITICAL_IP_CACHE(int NUM_SETS, int NUM_WAYS)
	{
		read_accesses = 0;
		tag_read_accesses = 0;
		write_accesses = 0;
		tag_write_accesses = 0;

		this->NUM_SETS = NUM_SETS;
		this->NUM_WAYS = NUM_WAYS;
		tag_array = new uint64_t* [NUM_SETS];
		valid_bit_array = new uint8_t* [NUM_SETS];
		lru_array = new uint8_t* [NUM_SETS];
		confidence = new uint8_t* [NUM_SETS];
		for (uint32_t i = 0; i < NUM_SETS; i++) 
		{
		    	valid_bit_array[i] = new uint8_t[NUM_WAYS];
			tag_array[i] = new uint64_t[NUM_WAYS];
			lru_array[i] = new uint8_t[NUM_WAYS];
			confidence[i] = new uint8_t[NUM_WAYS];
		   	for (uint32_t j = 0; j < NUM_WAYS; j++) 
			{
				valid_bit_array[i][j] = 0;
				tag_array[i][j] = 0;
				lru_array[i][j] = j;
				confidence[i][j] = 0; 
			}
		}

		for(int max_index = NUM_SETS - 1; max_index > 0; max_index >>= 1)
			this->LOG2_NUM_SETS += 1;

		//Neelu: Adding a sanity check for number of sets to be a power of 2.
		if(NUM_SETS > 0)
		{
			int temp_num_sets = 1;
			for(int i = 1; i <= this->LOG2_NUM_SETS; i++)
				temp_num_sets *= 2;

			if(temp_num_sets != NUM_SETS)
			{
				cout << "ERROR: --> num_sets: " << this->NUM_SETS << ", index_len: " << this->LOG2_NUM_SETS << " num_ways: " << this->NUM_WAYS << endl;
				cout << "ERROR: --> Oops: Num of sets are not 2^index_len. Kindly input cache sets and ways accordingly. <--" << endl;
				assert(0);
			}
		}
	 
	}

	bool check_vacancy(uint64_t ip);
	/*{
		uint32_t index = ip & ((1 << LOG2_NUM_SETS) - 1);
		for(uint32_t way = 0; way < NUM_WAYS; way++)
		{
			cout << "Valid bit: " << unsigned(valid_bit_array[index][way]) << endl;
			if(valid_bit_array[index][way] == 0)
				return true;
		}
		cout << "Returngin false " << endl;
		return false;
	}*/

	uint32_t insert(uint64_t ip);
	/*{
		uint32_t index = ip & ((1 << LOG2_NUM_SETS) - 1), way;
	        uint64_t tag = ip >> LOG2_NUM_SETS;
		for(way = 0; way < NUM_WAYS; way++)
		{
			if(valid_bit_array[index][way] == 0)
			{
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
				cout << "Printing LRU: " << unsigned(lru_array[index][way]) << endl;
				if(lru_array[index][way] == NUM_WAYS-1)
					break;
			}
			if(way >= NUM_WAYS)
			{
				for(way = 0; way < NUM_WAYS; way++)
					cout << "LRU: " << unsigned(lru_array[index][way]) << endl;
				assert(0);
			}
			valid_bit_array[index][way] = 1;
			tag_array[index][way] = tag;
			update_lru(index, way);
		}
		return way;
	}*/

	bool find(uint64_t ip);
	/*{
		uint32_t index = ip & ((1 << LOG2_NUM_SETS) - 1);
		uint64_t tag = ip >> LOG2_NUM_SETS;
		for(uint32_t way = 0; way < NUM_WAYS; way++)
		{
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
	}*/

	void update_lru(uint32_t index, uint32_t way);
	/*{
		for(uint32_t temp_way = 0; temp_way < NUM_WAYS; temp_way++)
		{
			if(lru_array[index][temp_way] < lru_array[index][way])
				lru_array[index][temp_way]++;
		}
		lru_array[index][way] = 0;
	}*/

	void print_stats();
	/*{
	}*/

	//Used only in CATCH
	uint8_t get_confidence(uint64_t ip);
	void update_confidence(uint64_t ip);
};

#endif
