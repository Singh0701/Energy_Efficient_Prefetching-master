//FDP Prefetcher Throttler

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

//FDP Prefetcher Throttler end
