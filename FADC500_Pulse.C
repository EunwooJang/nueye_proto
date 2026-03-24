struct PulseRegion {
    UInt_t start_idx;
    UInt_t end_idx;
};

std::vector<PulseRegion> FADC500_Pulse_Finding_Regions;


void FADC500_Calculate_Mean_Std_By_Iter(Bool_t Load_Channel=true, UInt_t channel=0, UInt_t iter_N=10){

    if (Load_Channel){
        FADC500_Load_Channel(channel);
    }

    std::vector<Bool_t> valid(FADC500_Channel_N_Data_Point, true);
    Float_t mean = 0.0;
    Float_t std_dev = 0.0;

    for (UShort_t iter = 0; iter < iter_N; iter++) {
        Float_t sum = 0.0;
        Float_t sum_sq = 0.0;
        UInt_t count = 0;

        for (UInt_t i = 0; i < FADC500_Channel_N_Data_Point; i++) {
            if (valid[i]) {
                Float_t val = FADC500_Calib_Waveform[i];
                sum += val;
                sum_sq += val * val;
                count++;
            }
        }

        if (count == 0) break;

        mean = sum / count;
        std_dev = std::sqrt(sum_sq / count - mean * mean);

        Float_t lo = mean - 3.0 * std_dev;
        Float_t hi = mean + 3.0 * std_dev;

        for (UInt_t i = 0; i < FADC500_Channel_N_Data_Point; i++) {
            if (valid[i] && (FADC500_Calib_Waveform[i] < lo || FADC500_Calib_Waveform[i] > hi)) {
                valid[i] = false;
                if (i > 0) valid[i - 1] = false;
                if (i < FADC500_Channel_N_Data_Point - 1) valid[i + 1] = false;
            }
        }
    }

    FADC500_Pulse_Finding_Mean = mean;
    FADC500_Pulse_Finding_Std = std_dev;


}

// opt = true : Only Pulse / opt = false : Only Fixed Region
void FADC500_Find_Pulse(UInt_t channel, Bool_t opt = true) {
    
    FADC500_Calculate_Mean_Std_By_Iter(true, channel, 10);

    Float_t threshold = FADC500_Pulse_Finding_Mean + FADC500_Pulse_Finding_Peak_Threshold_Factor * FADC500_Pulse_Finding_Std;
    FADC500_Pulse_Finding_Peak_Threshold = threshold;

    FADC500_Pulse_Finding_Regions.clear();

    // Only Pulse
    if (opt) {
        FADC500_Pulse_Finding_Regions.reserve(2*MAX_PULSES);
        
        // Get Pulses
        UInt_t i = 0;
        while (i < FADC500_Channel_N_Data_Point) {
            if (FADC500_Calib_Waveform[i] > threshold) {
                UInt_t search_end = i;
                while (search_end < FADC500_Channel_N_Data_Point && FADC500_Calib_Waveform[search_end] > threshold) search_end++;
                search_end--;

                Float_t peak_val = FADC500_Calib_Waveform[i];
                UInt_t peak_pos = i;
                for (UInt_t j = i + 1; j <= search_end; j++) {
                    if (FADC500_Calib_Waveform[j] > peak_val) {
                        peak_val = FADC500_Calib_Waveform[j];
                        peak_pos = j;
                    }
                }

                PulseRegion pulse;
                pulse.start_idx = TMath::Max((UInt_t)0, peak_pos - 10);
                pulse.end_idx = TMath::Min(FADC500_Channel_N_Data_Point - 1, peak_pos + 50);

                FADC500_Pulse_Finding_Regions.push_back(pulse);
                i = pulse.end_idx + 1;
            } else {
                i++;
            }
        }

        // Merge Pulse if region is overlayed
        if (FADC500_Pulse_Finding_Regions.size() > 1) {
            std::vector<PulseRegion> merged;
            merged.reserve(FADC500_Pulse_Finding_Regions.size());
            merged.push_back(FADC500_Pulse_Finding_Regions[0]);

            for (size_t j = 1; j < FADC500_Pulse_Finding_Regions.size(); j++) {
                PulseRegion& last = merged.back();
                const PulseRegion& curr = FADC500_Pulse_Finding_Regions[j];

                if (curr.start_idx <= last.end_idx) {
                    last.end_idx = TMath::Max(last.end_idx, curr.end_idx);
                } else {
                    merged.push_back(curr);
                }
            }

            FADC500_Pulse_Finding_Regions = std::move(merged);
        }

        FADC500_Pulse_is_Exist = !FADC500_Pulse_Finding_Regions.empty();


    } else {
        PulseRegion fixed_pulse;
        fixed_pulse.start_idx = FADC500_Pulse_Fixed_Start_idx[channel];
        fixed_pulse.end_idx = FADC500_Pulse_Fixed_End_idx[channel];
        FADC500_Pulse_Finding_Regions.push_back(fixed_pulse);

        FADC500_Pulse_is_Exist = false;  // Fixed region은 항상 false
    }
}
