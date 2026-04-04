// FADC500_Global.C
R__LOAD_LIBRARY(build/libRawObjs.dylib)

TString FADC500_Default_Directory = "dir_root";
TString FADC500_File_Dir_Path = "";
TString FADC500_File_Path = "";

TChain *FADC500_TChain = nullptr;
EventInfo *FADC500_Event_Info = nullptr;
FChannelData *FADC500_Channel_Data = nullptr;
FChannel *FADC500_Channel = nullptr;

TCanvas *FADC500_Canvas = nullptr;
TH1D *FADC500_TH1D = nullptr;
TH1D *FADC500_TH1D_2 = nullptr;
TH2D *FADC500_TH2D = nullptr;

TString FADC500_Hist_Name = "";

// Default Data

// Global
Long64_t FADC500_N_Entry;
UInt_t FADC500_N_Channel;
UInt_t FADC500_N_Data_Point;

// Channel Specific - Entries Independent
UInt_t FADC500_Channel_Id;

// Entries Specific - Channel Independent
Long64_t FADC500_Entry_N;
Long64_t FADC500_Event_N;
Long64_t FADC500_Trigger_N;
Long64_t FADC500_Trigger_Time;
UShort_t FADC500_Trigger_Type;
UShort_t FADC500_N_Hit;

// Channel Specific - Entries Specific
Short_t FADC500_Channel_Pedestal;
UShort_t FADC500_Channel_Bit;


// User defined data

Float_t FADC500_Time_Scale_ns = 2.0;
Float_t FADC500_Voltage_Scale_mV = 2500./4096.;
Float_t FADC500_Resistence_ohm = 50.0;

Short_t *FADC500_Calib_Waveform = nullptr;

Float_t FADC500_Channel_Mean;
Float_t FADC500_Channel_Std;

// Pulse Finding A ( Fixed threshold )
Float_t FADC500_PFA_Peak_Threshold = 8;
UInt_t FADC500_PFA_Pre_Window  = 10;
UInt_t FADC500_PFA_Post_Window = 10;
UInt_t FADC500_PFA_N_Pulse;


// Pulse Finding B ( Calculate Mean & Std by iteration )
UInt_t FADC500_PFB_N_Iter;
Float_t FADC500_PFB_Mean;
Float_t FADC500_PFB_Std;

Float_t FADC500_PFB_Peak_Threshold_Factor = 3;
Float_t FADC500_PFB_Peak_Threshold;

UInt_t FADC500_PFB_Pre_Window  = 10;
UInt_t FADC500_PFB_Post_Window = 10;

UInt_t FADC500_PFB_N_Pulse;

// Pulse Finding C ( Fixed region )
UInt_t FADC500_PFC_Fixed_Start = 0;
UInt_t FADC500_PFC_Fixed_End = 375;

std::vector<Int_t> FADC500_PFC_Fixed_Start_idx(32, FADC500_PFC_Fixed_Start);
std::vector<Int_t> FADC500_PFC_Fixed_End_idx(32, FADC500_PFC_Fixed_End);

struct PulseRegion {
    UInt_t start_idx;
    UInt_t end_idx;
};

std::vector<PulseRegion> FADC500_PFA_Regions;
std::vector<PulseRegion> FADC500_PFB_Regions;
std::vector<PulseRegion> FADC500_PFC_Regions;





void FADC500_Load_Entry(Long64_t entry = 0) {
    if (!FADC500_TChain) {
        std::cout << "Please run FADC500_Initialize(filename) first!" << std::endl;
        return;
    }
    
    if (entry >= FADC500_N_Entry) {
        std::cout << "Entry " << entry << " out of range!" << std::endl;
        return;
    }
    
    FADC500_TChain->GetEntry(entry);
    FADC500_Entry_N = entry;
    FADC500_N_Hit = FADC500_Event_Info->GetNHit();
    FADC500_Trigger_Type = FADC500_Event_Info->GetTriggerType();
    FADC500_Trigger_N = FADC500_Event_Info->GetTriggerNumber();
    FADC500_Event_N = FADC500_Event_Info->GetEventNumber();
    FADC500_Trigger_Time = FADC500_Event_Info->GetTriggerTime();
}



void FADC500_Load_Channel(UInt_t channel) {
    
    if (!FADC500_Channel_Data) {
        std::cout << "Please run FADC500_Initialize(filename) first!" << std::endl;
        return;
    }

    if (channel >= FADC500_N_Channel) {
        std::cout << "Channel " << channel << " does not exist!" << std::endl;
        return;
    }

    FADC500_Channel = FADC500_Channel_Data->Get(channel);
    if (!FADC500_Channel) {
        std::cout << "Channel " << channel << " is null!" << std::endl;
        return;
    }

    FADC500_Channel_Id = FADC500_Channel->GetID();
    FADC500_Channel_Bit = FADC500_Channel->GetBit();
    FADC500_Channel_Pedestal = FADC500_Channel->GetPedestal();

    if (FADC500_Calib_Waveform) delete[] FADC500_Calib_Waveform;
    FADC500_Calib_Waveform = new Short_t[FADC500_N_Data_Point];
    const UShort_t *raw_wave = FADC500_Channel->GetWaveform();
    for (Int_t i = 0; i < FADC500_N_Data_Point; i++) {
        FADC500_Calib_Waveform[i] = raw_wave[i] - FADC500_Channel_Pedestal;
    }
}





void FADC500_Load_Entry_And_Channel(Long64_t entry = 0, UInt_t channel = 0) {
    if (!FADC500_TChain) {
        std::cout << "Please run FADC500_Initialize(filename) first!" << std::endl;
        return;
    }
    
    if (entry >= FADC500_N_Entry) {
        std::cout << "Entry " << entry << " out of range!" << std::endl;
        return;
    }
    
    FADC500_Load_Entry(entry);
    FADC500_Load_Channel(channel);

}





void FADC500_Initialize() {
    if (FADC500_TChain) delete FADC500_TChain;
    if (FADC500_Event_Info) delete FADC500_Event_Info;
    if (FADC500_Channel_Data) delete FADC500_Channel_Data;
    if (FADC500_Channel) delete FADC500_Channel;
    if (FADC500_Calib_Waveform) delete[] FADC500_Calib_Waveform;
    
    FADC500_TChain = new TChain("AbsEvent");
    FADC500_TChain->Add(FADC500_File_Path);
    
    FADC500_Event_Info = new EventInfo();
    FADC500_Channel_Data = new FChannelData();

    FADC500_TChain->SetBranchAddress("EventInfo", &FADC500_Event_Info);
    FADC500_TChain->SetBranchAddress("FChannelData", &FADC500_Channel_Data);

    FADC500_N_Entry = FADC500_TChain->GetEntries();
    FADC500_TChain->GetEntry(0);

    FADC500_N_Channel = FADC500_Channel_Data->GetN();

    FADC500_Entry_N = 0;
    FADC500_N_Hit = FADC500_Event_Info->GetNHit();
    FADC500_Trigger_Type = FADC500_Event_Info->GetTriggerType();
    FADC500_Trigger_N = FADC500_Event_Info->GetTriggerNumber();
    FADC500_Event_N = FADC500_Event_Info->GetEventNumber();
    FADC500_Trigger_Time = FADC500_Event_Info->GetTriggerTime();

    FADC500_Channel = FADC500_Channel_Data->Get(0);

    FADC500_Channel_Id = FADC500_Channel->GetID();
    FADC500_Channel_Bit = FADC500_Channel->GetBit();
    FADC500_Channel_Pedestal = FADC500_Channel->GetPedestal();
    FADC500_N_Data_Point = FADC500_Channel->GetNdp();
        
    if (FADC500_Calib_Waveform) delete[] FADC500_Calib_Waveform;
    FADC500_Calib_Waveform = new Short_t[FADC500_N_Data_Point];
    const UShort_t *raw_wave = FADC500_Channel->GetWaveform();
    for (Int_t i = 0; i < FADC500_N_Data_Point; i++) {
        FADC500_Calib_Waveform[i] = raw_wave[i] - FADC500_Channel_Pedestal;
    }

}


void FADC500_Load_File_And_Initialize(Int_t run_N = 0, Int_t run_id = 0) {
    
    FADC500_File_Dir_Path = Form("%s/%05d", 
                                  FADC500_Default_Directory.Data(), 
                                  run_N);

    FADC500_File_Path = Form("%s/%05d/Run.root.%05d", 
                              FADC500_Default_Directory.Data(), 
                              run_N,
                              run_id);
    
    if (gSystem->AccessPathName(FADC500_File_Path.Data())) {
        std::cout << "Error: File not found: " << FADC500_File_Path << std::endl;
        FADC500_File_Path = "";
        return;
    }
    
    std::cout << "Found file: " << FADC500_File_Path << std::endl;
    FADC500_Initialize();
}



void FADC500_Delete_Canvas() {
    if (FADC500_Canvas) {
        delete FADC500_Canvas;
        FADC500_Canvas = nullptr;
    }
    if (FADC500_TH1D) {
        delete FADC500_TH1D;
        FADC500_TH1D = nullptr;
    }

     if (FADC500_TH1D_2) {
        delete FADC500_TH1D_2;
        FADC500_TH1D_2 = nullptr;
    }

    if (FADC500_TH2D) {
        delete FADC500_TH2D;
        FADC500_TH2D = nullptr;
    }
}



void FADC500_Cleanup() {
    if (FADC500_TChain) delete FADC500_TChain;
    if (FADC500_Event_Info) delete FADC500_Event_Info;
    if (FADC500_Channel_Data) delete FADC500_Channel_Data;
    if (FADC500_Channel) delete FADC500_Channel;
    if (FADC500_Calib_Waveform) delete[] FADC500_Calib_Waveform;
    
    FADC500_Delete_Canvas();

    FADC500_TChain = nullptr;
    FADC500_Event_Info = nullptr;
    FADC500_Channel_Data = nullptr;
    FADC500_Channel = nullptr;
    FADC500_Calib_Waveform = nullptr;
}





void FADC500_SaveHist() {
    if (!FADC500_TH1D && !FADC500_TH2D) {
        std::cout << "Error: No histogram to save!" << std::endl;
        return;
    }
    
    TString hist_name;
    if (FADC500_Hist_Name == "") {
        if (FADC500_TH1D) {
            hist_name = Form("hist_ch%u_entry%lld", FADC500_Channel_Id, FADC500_Entry_N);
        } else if (FADC500_TH2D) {
            hist_name = Form("hist_persistence_ch%u", FADC500_Channel_Id);
        }
    } else {
        hist_name = FADC500_Hist_Name;
    }
    
    TString save_path = FADC500_File_Dir_Path + "/" + hist_name + ".root";
    
    TFile *f = new TFile(save_path, "RECREATE");
    if (f->IsZombie()) {
        std::cout << "Error: Cannot create file " << save_path << std::endl;
        delete f;
        return;
    }
    
    if (FADC500_TH1D) {
        FADC500_TH1D->Write();
    }
    if (FADC500_TH2D) {
        FADC500_TH2D->Write();
    }
    
    f->Close();
    delete f;
    
    std::cout << "Histogram saved to: " << save_path << std::endl;
}




void FADC500_SaveImage() {
    if (!FADC500_Canvas) {
        std::cout << "Error: No canvas to save!" << std::endl;
        return;
    }
    
    TString image_name;
    if (FADC500_Hist_Name == "") {
        if (FADC500_TH1D) {
            image_name = Form("image_ch%u_entry%lld", FADC500_Channel_Id, FADC500_Entry_N);
        } else if (FADC500_TH2D) {
            image_name = Form("image_persistence_ch%u", FADC500_Channel_Id);
        }
    } else {
        image_name = FADC500_Hist_Name;
    }
    
    TString save_path_png = FADC500_File_Dir_Path + "/" + image_name + ".png";
    TString save_path_pdf = FADC500_File_Dir_Path + "/" + image_name + ".pdf";
    
    FADC500_Canvas->SaveAs(save_path_png);
    FADC500_Canvas->SaveAs(save_path_pdf);
    
    std::cout << "Image saved to: " << save_path_png << std::endl;
    std::cout << "Image saved to: " << save_path_pdf << std::endl;
}

