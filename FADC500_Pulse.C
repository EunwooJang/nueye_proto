void FADC500_Calculate_Mean_Std_By_Iter(UInt_t channel = 0) {
    FADC500_Load_Channel(channel);
    FADC500_PFB_N_Iter = 0;
    std::vector<Bool_t> valid(FADC500_N_Data_Point, true);
    Float_t mean    = 0.0;
    Float_t std_dev = 0.0;
    for (UShort_t iter = 0; iter < 10; iter++) {
        Float_t sum    = 0.0;
        Float_t sum_sq = 0.0;
        UInt_t  count  = 0;
        FADC500_PFB_N_Iter++;
        for (UInt_t i = 0; i < FADC500_N_Data_Point; i++) {
            if (valid[i]) {
                Float_t val = FADC500_Calib_Waveform[i];
                sum    += val;
                sum_sq += val * val;
                count++;
            }
        }
        if (count == 0) break;
        mean    = sum / count;
        std_dev = std::sqrt(sum_sq / count - mean * mean);
        Float_t lo = mean - 3.0 * std_dev;
        Float_t hi = mean + 3.0 * std_dev;
        Bool_t changed = false;
        for (UInt_t i = 0; i < FADC500_N_Data_Point; i++) {
            if (valid[i] && (FADC500_Calib_Waveform[i] < lo || FADC500_Calib_Waveform[i] > hi)) {
                valid[i] = false;
                if (i > 0)                         valid[i - 1] = false;
                if (i < FADC500_N_Data_Point - 1) valid[i + 1] = false;
                changed = true;
            }
        }
        if (!changed) break;
    }
    FADC500_PFB_Mean = mean;
    FADC500_PFB_Std  = std_dev;
}

static std::vector<PulseRegion> _FindAndMergeRegions(Float_t threshold, UInt_t pre_window, UInt_t post_window) {
    std::vector<PulseRegion> regions;
    UInt_t i = 0;
    while (i < FADC500_N_Data_Point) {
        if (FADC500_Calib_Waveform[i] > threshold) {
            UInt_t seg_start = i;
            while (i < FADC500_N_Data_Point && FADC500_Calib_Waveform[i] > threshold)
                i++;
            UInt_t seg_end = i - 1;

            PulseRegion pulse;
            pulse.start_idx = (seg_start >= pre_window) ? seg_start - pre_window : 0;
            pulse.end_idx   = TMath::Min(FADC500_N_Data_Point - 1, seg_end + post_window);
            regions.push_back(pulse);
        } else {
            i++;
        }
    }
    if (regions.size() > 1) {
        std::vector<PulseRegion> merged;
        merged.reserve(regions.size());
        merged.push_back(regions[0]);
        for (size_t j = 1; j < regions.size(); j++) {
            PulseRegion&       last = merged.back();
            const PulseRegion& curr = regions[j];
            if (curr.start_idx <= last.end_idx)
                last.end_idx = TMath::Max(last.end_idx, curr.end_idx);
            else
                merged.push_back(curr);
        }
        return merged;
    }
    return regions;
}

void FADC500_Find_Pulse_A(UInt_t channel) {
    FADC500_Load_Channel(channel);
    FADC500_PFA_Regions = _FindAndMergeRegions(FADC500_PFA_Peak_Threshold,
                                                FADC500_PFA_Pre_Window,
                                                FADC500_PFA_Post_Window);
    FADC500_PFA_N_Pulse = FADC500_PFA_Regions.size();
}

void FADC500_Find_Pulse_B(UInt_t channel) {
    FADC500_Calculate_Mean_Std_By_Iter(channel);
    FADC500_PFB_Peak_Threshold = FADC500_PFB_Mean
                               + FADC500_PFB_Peak_Threshold_Factor * FADC500_PFB_Std;
    FADC500_PFB_Regions = _FindAndMergeRegions(FADC500_PFB_Peak_Threshold,
                                                FADC500_PFB_Pre_Window,
                                                FADC500_PFB_Post_Window);
    FADC500_PFB_N_Pulse = FADC500_PFB_Regions.size();
}

void FADC500_Find_Pulse_C(UInt_t channel) {
    FADC500_Load_Channel(channel);
    FADC500_PFC_Regions.clear();
    PulseRegion fixed_pulse;
    fixed_pulse.start_idx = FADC500_PFC_Fixed_Start_idx[channel];
    fixed_pulse.end_idx   = FADC500_PFC_Fixed_End_idx[channel];
    FADC500_PFC_Regions.push_back(fixed_pulse);
}
