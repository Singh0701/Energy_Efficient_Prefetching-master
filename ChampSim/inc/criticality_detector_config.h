//Neelu: Uncomment the particular config to enable corresponding critical IP detector. 

//Neelu: Adding detection of critical IPs.
#define DETECT_CRITICAL_IPS

#define ROB_OCCUPANCY 
#define THRESHOLD_RELAXATION
//#define FOCUSED_PREF 
//#define FVP 
//#define CATCH
//#define HPCA2009

//#define FDP_PREFETCH_THROTTLER 

// (1) ROB Occupancy based Counter Criticality Detection Technique ----------------------
#ifdef ROB_OCCUPANCY 

#define ROB_OCCUPANCY_BASED_CRITICALITY

#ifdef THRESHOLD_RELAXATION
#define CRITICALITY_THRESHOLD_RELAXATION
#endif

#define CANDIDACY_THRESHOLD 50
#define STATIC_RELIANCE_THRESHOLD 20000
//Neelu: Reducing by 20% of original threholds. Why? Because even with the thresholds becoming zero, we will get a set of critical ips equal to all the IPs that are stalling the ROB. More prefetches will be made, however, they will still be less than the usual case which is prefetching for critical + non-critical IPs.
#define CANDIDACY_THRESHOLD_SUBTRAHEND 10
#define STATIC_RELIANCE_THRESHOLD_SUBTRAHEND 4000
//Neelu: Stop decrementing thresholds once max thresholds modification factor is reached, and there is still a reduction request, then all threholds will be set to zero.
#define MAX_THRESHOLDS_MODIFICATION_FACTOR CANDIDACY_THRESHOLD/CANDIDACY_THRESHOLD_SUBTRAHEND
#define NUM_OF_IPS_IN_CRITICAL_IP_TRAINING 64
#define NUM_OF_CRITICAL_IP_TRAINING_TABLE_WAYS 2
#define NUM_OF_CRITICAL_IP_TRAINING_TABLE_SETS NUM_OF_IPS_IN_CRITICAL_IP_TRAINING/NUM_OF_CRITICAL_IP_TRAINING_TABLE_WAYS
#define SET_ASSOCIATIVE_CRITICAL_IP_CACHE
#define CRITICAL_IP_CACHE_SETS 32
#define CRITICAL_IP_CACHE_WAYS 4 

#endif 
//End (1) ROB occupancy based 

// (2) Stall Cycle based (Focused Prefetching) Counter Criticality Detection Technique ----------------------
#ifdef FOCUSED_PREF

#define FOCUSED_PREFETCHING_COUNTING_CLASSIFIER
#define CANDIDACY_THRESHOLD 16
#define STATIC_RELIANCE_THRESHOLD 10000 
#define NUM_OF_IPS_IN_CRITICAL_IP_TRAINING 32
#define NUM_OF_CRITICAL_IP_TRAINING_TABLE_WAYS 8
#define NUM_OF_CRITICAL_IP_TRAINING_TABLE_SETS NUM_OF_IPS_IN_CRITICAL_IP_TRAINING/NUM_OF_CRITICAL_IP_TRAINING_TABLE_WAYS
#define SET_ASSOCIATIVE_CRITICAL_IP_CACHE
#define CRITICAL_IP_CACHE_SETS 4
#define CRITICAL_IP_CACHE_WAYS 8
#define CANDIDACY_THRESHOLD_SUBTRAHEND 0
#define STATIC_RELIANCE_THRESHOLD_SUBTRAHEND 0

#endif 

//End (2) Stall Cycle based (Focused Prefetching)


// (3) CATCH Criticality Detection Technique ----------------------

#ifdef CATCH

#define CATCH_CRITICALITY
#define SET_ASSOCIATIVE_CRITICAL_IP_CACHE
#define CRITICAL_IP_CACHE_SETS 4
#define CRITICAL_IP_CACHE_WAYS 8

#endif

//End (3) CATCH

// (4) HPCA 2009 Criticality Detection Technique by Subramaniam et al. --------------------

#ifdef HPCA2009

#define HPCA_2009_CRITICALITY

#endif 

//End (4) Submraniam et al. 

// (5) Focused Value Prediction Criticality Detection Technique ----------------------

#ifdef FVP 

#define FOCUSED_VALUE_PREDICTION_CRITICALITY_DETECTOR

#endif 

//End (5) FVP CD 

