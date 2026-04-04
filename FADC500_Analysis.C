// FADC500_Analysis.C
void FADC500_Draw_Single(Long64_t entry = 0, UInt_t channel = 0, Int_t opt = 0) {
    FADC500_Delete_Canvas();

    FADC500_Canvas = new TCanvas("FADC500_Canvas", "Waveform Viewer", 1000, 600);

    TPad *pad_title = new TPad("pad_title", "Title", 0.00, 0.93, 1.00, 1.00);
    pad_title->Draw();
    TPad *pad_wave = new TPad("pad_wave", "Waveform", 0.00, 0.00, 0.70, 0.93);
    pad_wave->Draw();
    TPad *pad_hist = new TPad("pad_hist", "Amplitude", 0.70, 0.00, 1.00, 0.93);
    pad_hist->Draw();

    FADC500_Load_Entry_And_Channel(entry, channel);

    Short_t vmin = FADC500_Calib_Waveform[0];
    Short_t vmax = FADC500_Calib_Waveform[0];
    for (UInt_t j = 1; j < FADC500_N_Data_Point; j++) {
        if (FADC500_Calib_Waveform[j] < vmin) vmin = FADC500_Calib_Waveform[j];
        if (FADC500_Calib_Waveform[j] > vmax) vmax = FADC500_Calib_Waveform[j];
    }
    double ymin = (double)vmin;
    double ymax = (double)vmax;

    const Float_t TS = 0.04;

    auto ApplyAxisStyle = [](TAxis *ax, Float_t ts) {
        ax->SetLabelSize(ts);
        ax->SetTitleSize(ts);
        ax->SetTitleOffset(1.2);
    };

    pad_title->cd();
    TLatex *title_tex = new TLatex(0.5, 0.5,
        Form("Channel %u - Entry %lld", FADC500_Channel_Id, FADC500_Entry_N));
    title_tex->SetNDC();
    title_tex->SetTextAlign(22);
    title_tex->SetTextSize(0.6);
    title_tex->SetTextFont(62);
    title_tex->Draw();

    pad_wave->cd();
    pad_wave->SetTopMargin(0.06);
    pad_wave->SetBottomMargin(0.12);
    pad_wave->SetLeftMargin(0.12);
    pad_wave->SetRightMargin(0.05);

    gStyle->SetOptStat(1110);
    gStyle->SetStatX(0.40);
    gStyle->SetStatY(0.88);
    gStyle->SetStatFontSize(TS);

    if (opt == 0) {
        if (FADC500_TH1D) { delete FADC500_TH1D; FADC500_TH1D = nullptr; }
        FADC500_TH1D = new TH1D(Form("FADC500_TH1D_%lld", entry),
                                 "Waveform;Time (Sample);Voltage (ADC)",
                                 FADC500_N_Data_Point, 0, FADC500_N_Data_Point);
        for (Int_t j = 0; j < (Int_t)FADC500_N_Data_Point; j++)
            FADC500_TH1D->SetBinContent(j + 1, FADC500_Calib_Waveform[j]);
        FADC500_TH1D->SetDirectory(0);
        FADC500_TH1D->SetLineWidth(1);
        ApplyAxisStyle(FADC500_TH1D->GetXaxis(), TS);
        ApplyAxisStyle(FADC500_TH1D->GetYaxis(), TS);
        FADC500_TH1D->GetYaxis()->SetRangeUser(ymin - 0.5, ymax + 0.5);
        FADC500_TH1D->GetYaxis()->SetTitleOffset(0.8);

        FADC500_TH1D->Draw("HIST");
    } else {
        TGraph *g_wave = new TGraph(FADC500_N_Data_Point);
        g_wave->SetTitle("Waveform;Time (Sample);Voltage (ADC)");
        for (UInt_t j = 0; j < FADC500_N_Data_Point; j++)
            g_wave->SetPoint(j, j, FADC500_Calib_Waveform[j]);
        g_wave->SetMarkerStyle(7);
        g_wave->SetMarkerColor(kBlue+1);
        g_wave->Draw("AP");
        ApplyAxisStyle(g_wave->GetXaxis(), TS);
        ApplyAxisStyle(g_wave->GetYaxis(), TS);
        g_wave->GetXaxis()->SetLimits(0, FADC500_N_Data_Point);
        g_wave->GetYaxis()->SetRangeUser(ymin - 0.5, ymax + 0.5);
        g_wave->GetYaxis()->SetTitleOffset(0.8);
        g_wave->Draw("AP");
    }

    const Float_t TS_HIST = 0.07;
    int bin_min = (int)std::round(ymin);
    int bin_max = (int)std::round(ymax);
    int nbins   = bin_max - bin_min + 1;

    if (FADC500_TH1D_2) { delete FADC500_TH1D_2; FADC500_TH1D_2 = nullptr; }
    FADC500_TH1D_2 = new TH1D("FADC500_AmpHist",
                               "Amplitude Histogram;    ;Counts",
                               nbins, bin_min - 0.5, bin_max + 0.5);

    for (UInt_t j = 0; j < FADC500_N_Data_Point; j++)
        FADC500_TH1D_2->Fill(FADC500_Calib_Waveform[j]);
    FADC500_TH1D_2->SetDirectory(0);

    pad_hist->cd();
    pad_hist->SetTopMargin(0.06);
    pad_hist->SetBottomMargin(0.12);
    pad_hist->SetRightMargin(0.05);
    pad_hist->SetLogx();
    gStyle->SetOptStat(0);
    ApplyAxisStyle(FADC500_TH1D_2->GetXaxis(), TS_HIST);
    ApplyAxisStyle(FADC500_TH1D_2->GetYaxis(), TS_HIST);
    FADC500_TH1D_2->SetLineWidth(1);
    FADC500_TH1D_2->Draw("HBAR");
    FADC500_TH1D_2->GetXaxis()->SetRangeUser(bin_min, bin_max);
    FADC500_TH1D_2->GetYaxis()->SetTitleOffset(0.7);
    FADC500_TH1D_2->GetYaxis()->SetLabelOffset(-0.04);
    pad_hist->Modified();
    pad_hist->Update();

    TPaveText *title = (TPaveText*)pad_hist->GetPrimitive("title");
    title->SetTextSize(0.08);
    pad_hist->Modified();
    pad_hist->Update();

    FADC500_Canvas->Modified();
    FADC500_Canvas->Update();

    pad_wave->AddExec("zoom_update", Form(R"(
        {
            TAxis *ax = nullptr;
            if (FADC500_TH1D) {
                ax = FADC500_TH1D->GetXaxis();
            } else {
                TGraph *g = (TGraph*)pad_wave->GetPrimitive("Graph");
                if (g) ax = g->GetXaxis();
            }
            if (!ax) return;

            Double_t xlo = ax->GetBinLowEdge(ax->GetFirst());
            Double_t xhi = ax->GetBinUpEdge(ax->GetLast());

            if (!FADC500_TH1D_2) return;

            int bin_min = (int)std::round(FADC500_TH1D_2->GetYaxis()->GetXmin() + 0.5);
            int bin_max = (int)std::round(FADC500_TH1D_2->GetYaxis()->GetXmax() - 0.5);
            int nbins   = bin_max - bin_min + 1;

            FADC500_TH1D_2->Reset("ICES");

            int ilo = (int)std::ceil(xlo);
            int ihi = (int)std::floor(xhi);
            if (ilo < 0) ilo = 0;
            if (ihi >= (int)FADC500_N_Data_Point)
                ihi = (int)FADC500_N_Data_Point - 1;

            for (int j = ilo; j <= ihi; j++)
                FADC500_TH1D_2->Fill(FADC500_Calib_Waveform[j]);

            pad_hist->Modified();
            pad_hist->Update();
        }
    )"));
}



void FADC500_Draw_Continuous(Long64_t start_entry = 0, UInt_t channel=0){
    Long64_t entry = start_entry;
    char a;
    while (1==1) {
    
        FADC500_Draw_Single(entry, channel);
        entry++;
        gPad->Update();
        gSystem->ProcessEvents();
    
        cin >> a;

    if (a == 'q') break;
    }

}


void FADC500_Draw_Persistence(Long64_t start_entry = 0, Long64_t n_entry = 1000, UInt_t channel=0,
                               Int_t min_adc = -10, Int_t max_adc = 60) {
    
    if (!FADC500_TChain) {
        std::cout << "Please run FADC500_Initialize(filename) first!" << std::endl;
        return;
    }
    
    FADC500_Delete_Canvas();
    Long64_t end_entry = TMath::Min(start_entry + n_entry, FADC500_N_Entry);
    Int_t nbins_y = (Int_t)(max_adc - min_adc) + 1;
    
    FADC500_TH2D = new TH2D("FADC500_TH2D",
                            Form("Channel %u Persistence (Entry %lld - %lld, %lld entries);Time (Sample);Voltage (ADC)", 
                                 FADC500_Channel_Id, start_entry, end_entry - 1, end_entry - start_entry),
                            FADC500_N_Data_Point, 0, FADC500_N_Data_Point,
                            nbins_y, min_adc, max_adc + 1);
    
    for (Long64_t entry = start_entry; entry < end_entry; entry++) {

        FADC500_Load_Entry_And_Channel(entry, channel);

        for (UInt_t j = 0; j < FADC500_N_Data_Point; j++) {
            UInt_t time_sample = j;
            Short_t adc_value = FADC500_Calib_Waveform[j];
            FADC500_TH2D->Fill(time_sample, adc_value);
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




void FADC500_Calculate_Mean_Std(Long64_t start_entry = 0, Long64_t n_entry = 1, UInt_t channel=0) {
    if (!FADC500_TChain) {
        std::cout << "Please run FADC500_Initialize(filename) first!" << std::endl;
        return;
    }
    double sum  = 0;
    double sum2 = 0;
    Long64_t count = 0;
    Long64_t end_entry = TMath::Min(start_entry + n_entry, FADC500_N_Entry);
    for (Long64_t entry = start_entry; entry < end_entry; entry++) {
        FADC500_Load_Entry_And_Channel(entry, channel);
        for (UInt_t j = 0; j < FADC500_N_Data_Point; j++) {
            double v = FADC500_Calib_Waveform[j];
            sum  += v;
            sum2 += v * v;
            count++;
        }
    }
    FADC500_Channel_Mean = sum / count;
    FADC500_Channel_Std  = sqrt(sum2 / count - FADC500_Channel_Mean * FADC500_Channel_Mean);
}




void FADC500_Draw_Amplitude_Histogram(Long64_t start_entry = 0, Long64_t n_entry = -1, UInt_t channel = 0) {
    if (!FADC500_TChain) {
        std::cout << "Please run FADC500_Initialize(filename) first!" << std::endl;
        return;
    }

    Long64_t max_entry = (n_entry < 0 || n_entry > FADC500_N_Entry) ? FADC500_N_Entry : n_entry;

    FADC500_Delete_Canvas();
    std::vector<int> adc_vals;
    adc_vals.reserve(max_entry * FADC500_N_Data_Point);
    int vmin = 0;
    int vmax = 0;
    for (Long64_t entry = start_entry; entry < max_entry; entry++) {
        FADC500_Load_Entry_And_Channel(entry, channel);
        for (UInt_t j = 0; j < FADC500_N_Data_Point; j++) {
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



void FADC500_Draw_FFT(Long64_t entry = 0, Long64_t n_entry = 1,  UInt_t channel = 0) {
    FADC500_Delete_Canvas();
    FADC500_Load_Entry_And_Channel(entry, channel);
    Int_t    n      = FADC500_N_Data_Point;
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

    Long64_t end_entry = TMath::Min(entry + n_entry, FADC500_N_Entry);
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


