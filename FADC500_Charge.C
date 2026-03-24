// FADC500_Draw_Charge_Histogram.C
void FADC500_Draw_Charge_Histogram(UInt_t channel = 0,
                 Float_t pedestal_t_ns_start = 450.,
                 Float_t pedestal_t_ns_end = 550.,
                 Float_t pedestal_min_threshold_mV = -4000,
                 Float_t pedestal_max_threshold_mV = 4000,
                 Double_t signal_peak_search_start_ns = 550.,
                 Double_t signal_peak_search_end_ns = 750.,
                 Double_t signal_integral_start_ns = 4.,
                 Double_t signal_integral_end_ns = 16.,
                 Double_t x_draw_min = -2,
                 Double_t x_draw_max = 12,
                 Int_t log = 1,
                 Bool_t use_charge_conversion = false)
{

    if (!FADC500_TChain) {
        std::cout << "Error: Please run FADC500_Initialize() first!" << std::endl;
        return;
    }

    FADC500_Delete_Canvas();

    Int_t pedestal_x_min = TMath::CeilNint(pedestal_t_ns_start / FADC500_Time_Scale_ns);
    Int_t pedestal_x_max = TMath::CeilNint(pedestal_t_ns_end / FADC500_Time_Scale_ns);

    Int_t threshold_min = TMath::FloorNint(pedestal_min_threshold_mV / FADC500_Voltage_Scale_mV);
    Int_t threshold_max = TMath::CeilNint(pedestal_max_threshold_mV / FADC500_Voltage_Scale_mV);

    Int_t signal_peak_search_x_min = TMath::CeilNint(signal_peak_search_start_ns / FADC500_Time_Scale_ns);
    Int_t signal_peak_search_x_max = TMath::CeilNint(signal_peak_search_end_ns / FADC500_Time_Scale_ns);

    Int_t signal_integral_x_min = TMath::CeilNint(signal_integral_start_ns / FADC500_Time_Scale_ns);
    Int_t signal_integral_x_max = TMath::CeilNint(signal_integral_end_ns / FADC500_Time_Scale_ns);

    FADC500_TChain->SetBranchStatus("EventInfo", 0);

    TFile *output_file = nullptr;
    TTree *output_tree = nullptr;
    Double_t charge_value = 0;
    Long64_t entry_number = 0;
    
    std::vector<Double_t> charge_list;
    Double_t factor = 1.0;
    
    if (use_charge_conversion) {
        factor = FADC500_Voltage_Scale_mV * FADC500_Time_Scale_ns / FADC500_Resistence_ohm;
    }

    for (Long64_t i = 0; i < FADC500_N_Entry; ++i) {
        FADC500_Load_Entry_And_Channel(i, channel);

        Bool_t avoid = false;

        // Check pedestal region for noise
        for (Int_t j = pedestal_x_min; j < pedestal_x_max && j < FADC500_Channel_N_Data_Point; ++j) {
            Short_t height = FADC500_Calib_Waveform[j];
            if ((height > threshold_max) || (height < threshold_min)) {
                avoid = kTRUE;
                break;
            }
        }

        if (avoid) continue;

        // Calculate pedestal average
        Float_t pedestal_avg_adc = 0;
        Int_t pedestal_count = 0;

        if (pedestal_x_max - pedestal_x_min > 0) {
            for (Int_t j = pedestal_x_min; j < pedestal_x_max && j < FADC500_Channel_N_Data_Point; ++j) {
                pedestal_avg_adc += FADC500_Calib_Waveform[j];
                pedestal_count++;
            }

            if (pedestal_count > 0) {
                pedestal_avg_adc /= pedestal_count;
            }
        }

        // Find peak position
        Int_t peak_pos_x = signal_peak_search_x_min;
        Float_t max_height = -1e9;

        for (Int_t j = signal_peak_search_x_min; j < signal_peak_search_x_max && j < FADC500_Channel_N_Data_Point; ++j) {
            Float_t height = FADC500_Calib_Waveform[j] - pedestal_avg_adc;
            if (height > max_height) {
                max_height = height;
                peak_pos_x = j;
            }
        }

        // Calculate integral region around peak
        Int_t integral_x_min_calc = peak_pos_x - signal_integral_x_min;
        Int_t integral_x_max_calc = peak_pos_x + signal_integral_x_max;

        if (integral_x_min_calc < 0) integral_x_min_calc = 0;
        if (integral_x_max_calc >= FADC500_Channel_N_Data_Point) 
            integral_x_max_calc = FADC500_Channel_N_Data_Point - 1;

        // Calculate charge (or ADC sum)
        Double_t charge = 0;
        for (Int_t j = integral_x_min_calc; j < integral_x_max_calc; ++j) {
            Float_t height = FADC500_Calib_Waveform[j] - pedestal_avg_adc;
            charge += height;
        }

        charge *= factor;
        
        if ((charge < x_draw_min) || (charge > x_draw_max)){
            if (use_charge_conversion) {
                std::cout << "Entry " << i << " out of range: " << charge << " pC" << std::endl;
            } else {
                std::cout << "Entry " << i << " out of range: " << charge << " ADC" << std::endl;
            }
        }
        charge_list.push_back(charge);
        
    }

    // Calculate mean
    Double_t sum = 0;
    for (UInt_t i = 0; i < charge_list.size(); ++i) {
        sum += charge_list[i];
    }
    Double_t mean = sum / static_cast<Double_t>(charge_list.size());

    if (use_charge_conversion) {
        std::cout << "Mean Charge: " << mean << " pC" << std::endl;
    } else {
        std::cout << "Mean ADC Sum: " << mean << " ADC" << std::endl;
    }
    
    std::cout << "Total entries processed: " << FADC500_N_Entry << std::endl;
    std::cout << "Entries passing filter: " << charge_list.size() << std::endl;

    // Setup style
    gStyle->SetOptStat(10);
    gStyle->SetOptFit(1111);
    gStyle->SetStatX(0.9);
    gStyle->SetStatY(0.9);
    gStyle->SetStatW(0.2);
    gStyle->SetStatH(0.4);

    // Create canvas
    TString canvas_title;
    if (use_charge_conversion) {
        canvas_title = Form("Channel %d - Charge Histogram", channel);
    } else {
        canvas_title = Form("Channel %d - ADC Sum Histogram", channel);
    }
    
    FADC500_Canvas = new TCanvas("FADC500_Canvas", canvas_title, 1200, 600);

    // Create histogram
    Int_t min_val = TMath::FloorNint(x_draw_min / factor) - 1;
    Int_t max_val = TMath::CeilNint(x_draw_max / factor) + 1;
    Int_t range = (max_val - min_val);

    TString hist_title;
    if (use_charge_conversion) {
        hist_title = Form("Channel %d - Charge Histogram;Charge (pC);Counts", channel);
    } else {
        hist_title = Form("Channel %d - ADC Sum Histogram;ADC Sum;Counts", channel);
    }

    FADC500_TH1D = new TH1D("FADC500_TH1D", hist_title,
                          range, min_val * factor, max_val * factor);
    
    for (UInt_t i = 0; i < charge_list.size(); ++i) {
        FADC500_TH1D->Fill(charge_list[i]);
    }

    if (log == 1) {
        Double_t maxValue = FADC500_TH1D->GetMaximum();
        Double_t yMax = TMath::Power(10, TMath::Ceil(TMath::Log10(maxValue)));
        FADC500_TH1D->GetYaxis()->SetRangeUser(1e-1, yMax);
        FADC500_Canvas->SetLogy();
    }

    FADC500_TH1D->SetLineWidth(1);
    FADC500_TH1D->SetLineColor(kBlue);
    FADC500_TH1D->Draw("HIST");

    FADC500_Canvas->Update();

    FADC500_TChain->SetBranchStatus("EventInfo", 1);
}
