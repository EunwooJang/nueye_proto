// FADC500_Analysis.C
void FADC500_Draw_Entry_Channel(Long64_t entry = 0, UInt_t channel=0, Bool_t apply_scale = true) {
   
    FADC500_Delete_Canvas();
    FADC500_Canvas = new TCanvas("FADC500_Canvas", "Waveform Viewer", 800, 600);
    
    FADC500_Load_Entry_And_Channel(entry, channel);
        
        if (FADC500_TH1D) {
            delete FADC500_TH1D;
            FADC500_TH1D = nullptr;
        }
        
        if (apply_scale) {
            FADC500_TH1D = new TH1D(Form("FADC500_TH1D_%lld", entry),
                                    Form("Channel %u - Entry %lld;Time (ns);Voltage (mV)", 
                                         FADC500_Channel_Id, FADC500_Entry_N),
                                    FADC500_Channel_N_Data_Point, 
                                    0, 
                                    FADC500_Channel_N_Data_Point * FADC500_Time_Scale_ns);
            
            for (Int_t j = 0; j < FADC500_Channel_N_Data_Point; j++) {
                FADC500_TH1D->SetBinContent(j + 1, FADC500_Calib_Waveform[j] * FADC500_Voltage_Scale_mV);
            }
        } else {
            FADC500_TH1D = new TH1D(Form("FADC500_TH1D_%lld", entry),
                                    Form("Channel %u - Entry %lld;Time (TDC);Voltage (ADC)", 
                                         FADC500_Channel_Id, FADC500_Entry_N),
                                    FADC500_Channel_N_Data_Point, 
                                    0, 
                                    FADC500_Channel_N_Data_Point);
            
            for (Int_t j = 0; j < FADC500_Channel_N_Data_Point; j++) {
                FADC500_TH1D->SetBinContent(j + 1, FADC500_Calib_Waveform[j]);
            }
        }
        
        FADC500_TH1D->SetDirectory(0);
        gStyle->SetOptStat(0);
        FADC500_TH1D->SetLineWidth(1);
        FADC500_TH1D->Draw("HIST");
        
        FADC500_Canvas->Modified();
        FADC500_Canvas->Update();
        
}


void FADC500_Draw_Entries_Channel(Long64_t start_entry = 0, UInt_t channel=0, Bool_t apply_scale = true){

    Long64_t entry = start_entry;

    char a;

    while (1==1) {
    
    FADC500_Draw_Entry_Channel(entry, channel, apply_scale);
    entry++;
    
    gPad->Update();
    gSystem->ProcessEvents();
    
    cin >> a;

    if (a == 'q') break;
    }

}


void FADC500_Draw_Entries_Channel_Persistence(Long64_t start_entry = 0, Long64_t entry_n = 1000, UInt_t channel=0,
                               Float_t min_mV = -10., Float_t max_mV = 60., Bool_t apply_scale = true) {
    
    if (!FADC500_TChain) {
        std::cout << "Please run FADC500_Initialize(filename) first!" << std::endl;
        return;
    }
    
    FADC500_Delete_Canvas();
    
    Long64_t end_entry = TMath::Min(start_entry + entry_n, FADC500_N_Entry);
    
    if (apply_scale) {
        Float_t max_adc = std::floor(max_mV / FADC500_Voltage_Scale_mV);
        Float_t min_adc = std::ceil(min_mV / FADC500_Voltage_Scale_mV);
        Int_t nbins_y = (Int_t)(max_adc - min_adc) + 1;
        
        FADC500_TH2D = new TH2D("FADC500_TH2D",
                                Form("Channel %u Persistence (Entry %lld - %lld, %lld entries);Time (ns);Voltage (mV)", 
                                     FADC500_Channel_Id, start_entry, end_entry - 1, end_entry - start_entry),
                                FADC500_Channel_N_Data_Point, 0, FADC500_Channel_N_Data_Point * FADC500_Time_Scale_ns,
                                nbins_y, min_adc * FADC500_Voltage_Scale_mV, (max_adc + 1) * FADC500_Voltage_Scale_mV);
        
        for (Long64_t entry = start_entry; entry < end_entry; entry++) {
            
            FADC500_Load_Entry_And_Channel(entry, channel);

            for (UInt_t j = 0; j < FADC500_Channel_N_Data_Point; j++) {
                UInt_t time_sample = j;
                Short_t adc_value = FADC500_Calib_Waveform[j];
                FADC500_TH2D->Fill(time_sample * FADC500_Time_Scale_ns, adc_value * FADC500_Voltage_Scale_mV);
            }
        }
        
    } else {
        Float_t min_adc = std::ceil(min_mV / FADC500_Voltage_Scale_mV);
        Float_t max_adc = std::floor(max_mV / FADC500_Voltage_Scale_mV);
        Int_t nbins_y = (Int_t)(max_adc - min_adc) + 1;
        
        FADC500_TH2D = new TH2D("FADC500_TH2D",
                                Form("Channel %u Persistence (Entry %lld - %lld, %lld entries);Time (TDC);Voltage (ADC)", 
                                     FADC500_Channel_Id, start_entry, end_entry - 1, end_entry - start_entry),
                                FADC500_Channel_N_Data_Point, 0, FADC500_Channel_N_Data_Point,
                                nbins_y, min_adc, max_adc + 1);
        
        for (Long64_t entry = start_entry; entry < end_entry; entry++) {

            FADC500_Load_Entry_And_Channel(entry, channel);

            for (UInt_t j = 0; j < FADC500_Channel_N_Data_Point; j++) {
                UInt_t time_sample = j;
                Short_t adc_value = FADC500_Calib_Waveform[j];
                FADC500_TH2D->Fill(time_sample, adc_value);
            }
        }
    }
    
    FADC500_TH2D->SetDirectory(0);
    
    FADC500_Canvas = new TCanvas("FADC500_Canvas", 
                                 Form("Channel %u - Persistence", FADC500_Channel_Id), 
                                 800, 600);
    
    gStyle->SetOptStat(0);
    gStyle->SetPalette(kBird);
    
    FADC500_TH2D->Draw("COLZ");
    
    FADC500_Canvas->Modified();
    FADC500_Canvas->Update();
}


void FADC500_Calculate_Mean_Std(Long64_t Start_Entry = 0, Long64_t N_Sample = 10000) {
    if (!FADC500_TChain) {
        std::cout << "Please run FADC500_Initialize(filename) first!" << std::endl;
        return;
    }
    printf("%4s  %8s  %10s  %10s\n", "Ch", "Samples", "Mean", "Std");
    printf("------------------------------------------\n");
    for (UInt_t channel = 0; channel < FADC500_N_Channel; channel++) {
        double sum   = 0;
        double sum2  = 0;
        Long64_t count = 0;
        for (Long64_t entry = Start_Entry; entry < FADC500_N_Entry && count < N_Sample; entry++) {
            FADC500_Load_Entry_And_Channel(entry, channel);
            for (UInt_t j = 0; j < FADC500_Channel_N_Data_Point && count < N_Sample; j++) {
                double v = FADC500_Calib_Waveform[j];
                sum  += v;
                sum2 += v * v;
                count++;
            }
        }
        double mean = sum / count;
        double std  = sqrt(sum2 / count - mean * mean);
        printf("%4u  %8lld  %10.4f  %10.4f\n", channel, count, mean, std);
    }
}


void FADC500_Draw_Amplitude_Histogram(int channel = 0, Long64_t N_Entry = -1) {
    if (!FADC500_TChain) {
        std::cout << "Please run FADC500_Initialize(filename) first!" << std::endl;
        return;
    }

    Long64_t max_entry = (N_Entry < 0 || N_Entry > FADC500_N_Entry) ? FADC500_N_Entry : N_Entry;

    FADC500_Delete_Canvas();
    std::vector<int> adc_vals;
    adc_vals.reserve(max_entry * FADC500_Channel_N_Data_Point);
    int vmin = 0;
    int vmax = 0;
    for (Long64_t entry = 0; entry < max_entry; entry++) {
        FADC500_Load_Entry_And_Channel(entry, channel);
        for (UInt_t j = 0; j < FADC500_Channel_N_Data_Point; j++) {
            int v = (int)FADC500_Calib_Waveform[j];
            adc_vals.push_back(v);
            if (v < vmin) vmin = v;
            if (v > vmax) vmax = v;
        }
    }
    int bin_min = (int)std::floor(vmin - 1.);
    int bin_max = (int)std::ceil(vmax + 1.);
    int nbins   = bin_max - bin_min + 1;
    FADC500_Hist_Name = Form("FADC500_Amplitude_Ch%d", channel);
    FADC500_TH1D = new TH1D(FADC500_Hist_Name,
                             Form("Channel %d - Amplitude Histogram;Voltage (ADC);Counts", FADC500_Channel_Id),
                             nbins,
                             bin_min,
                             bin_max + 1);
    for (const int &v : adc_vals) {
        FADC500_TH1D->Fill(v);
    }
    FADC500_Canvas = new TCanvas("FADC500_Canvas",
                                  Form("Channel %d - Amplitude Histogram", FADC500_Channel_Id),
                                  1200, 600);
    gStyle->SetOptStat(1110);
    gStyle->SetStatX(0.9);
    gStyle->SetStatY(0.9);
    FADC500_TH1D->SetLineWidth(2);
    FADC500_TH1D->Draw();
    FADC500_Canvas->SetLogy();
    FADC500_Canvas->Modified();
    FADC500_Canvas->Update();
}


void FADC500_Draw_Entry_FFT(Long64_t entry = 0, UInt_t channel = 0, Long64_t N_Event = 1) {
    FADC500_Delete_Canvas();
    FADC500_Load_Entry_And_Channel(entry, channel);
    Int_t    n      = FADC500_Channel_N_Data_Point;
    Double_t dt_s   = FADC500_Time_Scale_ns * 1e-9;
    Double_t fs     = 1.0 / dt_s;
    Double_t df     = fs / n;
    Int_t    n_bins = n / 2 + 1;

    // Hann window
    Double_t *win     = new Double_t[n];
    Double_t  win_sum = 0.0;
    for (Int_t j = 0; j < n; j++) {
        win[j]    = 0.5 - 0.5 * TMath::Cos(2.0 * TMath::Pi() * j / n);
        win_sum  += win[j];
    }

    // Apply FFT & add
    Double_t    *amp_sum = new Double_t[n_bins]();
    TVirtualFFT *fft     = TVirtualFFT::FFT(1, &n, "R2C M");
    Double_t    *in      = new Double_t[n];

    Long64_t end_entry = TMath::Min(entry + N_Event, FADC500_N_Entry);
    Long64_t n_done    = 0;

    for (Long64_t ev = entry; ev < end_entry; ev++) {
        FADC500_Load_Entry_And_Channel(ev, channel);
        for (Int_t j = 0; j < n; j++)
            in[j] = (Double_t)FADC500_Calib_Waveform[j] * win[j];
        fft->SetPoints(in);
        fft->Transform();
        for (Int_t j = 0; j < n_bins; j++) {
            Double_t re, im;
            fft->GetPointComplex(j, re, im);
            amp_sum[j] += TMath::Sqrt(re*re + im*im);
        }
        n_done++;
    }
    delete[] in;
    delete[] win;
    delete fft;

    if (FADC500_TH1D) { delete FADC500_TH1D; FADC500_TH1D = nullptr; }
    FADC500_TH1D = new TH1D(
        Form("FADC500_FFT_%lld", entry),
        Form("Channel %u  Entry %lld~%lld  Amplitude (avg %lld events);"
             "Frequency [Hz];Amplitude [ADC]",
             FADC500_Channel_Id, entry, end_entry - 1, n_done),
        n_bins, 0.0, fs / 2.0
    );

    for (Int_t j = 0; j < n_bins; j++) {
        Double_t amp = (amp_sum[j] / n_done) / win_sum;
        if (j > 0 && j < n_bins - 1) amp *= 2.0;
        FADC500_TH1D->SetBinContent(j + 1, amp);
    }
    delete[] amp_sum;

    FADC500_Canvas = new TCanvas("FADC500_Canvas",
                                 Form("Channel %u Amplitude", FADC500_Channel_Id),
                                 1200, 600);
    gStyle->SetOptStat(0);
    gStyle->SetCanvasPreferGL(true);
    FADC500_TH1D->SetLineWidth(1);
    FADC500_TH1D->SetLineColor(kBlue);
    FADC500_TH1D->Draw("HIST");
    FADC500_Canvas->SetLogy();
    FADC500_Canvas->Modified();
    FADC500_Canvas->Update();
}


