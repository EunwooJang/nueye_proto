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
TH2D *FADC500_TH2D = nullptr;

TString FADC500_Hist_Name = "";


// Global
Long64_t FADC500_N_Entry;
UInt_t FADC500_N_Channel;
UInt_t FADC500_Channel_N_Data_Point;

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

Short_t *FADC500_Calib_Waveform = nullptr;

Float_t FADC500_Time_Scale_ns = 2.0;
Float_t FADC500_Voltage_Scale_mV = 2500./4096.;
Float_t FADC500_Resistence_ohm = 50.0;  // 50 Ohm termination

UInt_t MAX_PULSES = 5;  // FIXME Currently each pulse have minimum 60 sample width 
Float_t FADC500_Pulse_Finding_Mean;
Float_t FADC500_Pulse_Finding_Std;
Int_t FADC500_Pulse_Finding_Peak_Threshold_Factor = 5;
Float_t FADC500_Pulse_Finding_Peak_Threshold = 0;

std::vector<Int_t> FADC500_Pulse_Fixed_Start_idx(31, 275);  // FIXME Value 275 for nuEYE 1 ton Prototype
std::vector<Int_t> FADC500_Pulse_Fixed_End_idx(31, 375);    // FIXME Value 375 for nuEYE 1 ton Prototype

Bool_t FADC500_Pulse_is_Exist;

UInt_t FADC500_Trigger_Channel_Id = 32; // FIXME It will be 32 for nuEYE 1 ton Prototype

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
    
    FADC500_TChain->GetEntry(0); // Load 1st Entry
    FADC500_N_Channel = FADC500_Channel_Data->GetN();

    FADC500_Channel = FADC500_Channel_Data->Get(0);  // Load 1st Channel
    FADC500_Channel_N_Data_Point = FADC500_Channel->GetNdp();

    FADC500_Calib_Waveform = nullptr;
}





void FADC500_Load_File_And_Initialize(TString Search_Word = "", Int_t Run_Id = 0) {
    
    TObjArray *tokens = Search_Word.Tokenize("_");
    
    void *dir = gSystem->OpenDirectory(FADC500_Default_Directory.Data());
    if (!dir) {
        std::cout << "Error: Cannot open directory " << FADC500_Default_Directory << std::endl;
        return;
    }
    
    const Char_t *entry;
    TString found_folder = "";
    
    while ((entry = gSystem->GetDirEntry(dir))) {
        TString folder_name = entry;
        
        if (folder_name == "." || folder_name == "..") continue;
        
        TString full_path = FADC500_Default_Directory + "/" + folder_name;
        
        Long_t id, flags, modtime;
        Long64_t size;
        if (gSystem->GetPathInfo(full_path.Data(), &id, &size, &flags, &modtime) == 0) {
            if (flags & 2) {
                
                Bool_t all_found = true;
                for (UInt_t i = 0; i < tokens->GetEntries(); i++) {
                    TString token = ((TObjString*)tokens->At(i))->GetString();
                    if (!folder_name.Contains(token)) {
                        all_found = false;
                        break;
                    }
                }
                
                if (all_found) {
                    found_folder = folder_name;
                    break;
                }
            }
        }
    }
    
    gSystem->FreeDirectory(dir);
    delete tokens;
    
    if (found_folder == "") {
        std::cout << "Error: No folder found matching '" << Search_Word << "'" << std::endl;
        return;
    }
   
    FADC500_File_Dir_Path =  Form("%s/%s", 
                              FADC500_Default_Directory.Data(), 
                              found_folder.Data());

    FADC500_File_Path = Form("%s/%s/RUN.root.%05d", 
                              FADC500_Default_Directory.Data(), 
                              found_folder.Data(), 
                              Run_Id);
    
    if (gSystem->AccessPathName(FADC500_File_Path.Data())) {
        std::cout << "Error: File not found: " << FADC500_File_Path << std::endl;
        FADC500_File_Path = "";
        return;
    }
    
    std::cout << "Found file: " << FADC500_File_Path << std::endl;

    FADC500_Initialize();
}





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

    FADC500_Calib_Waveform = new Short_t[FADC500_Channel_N_Data_Point];

    const UShort_t *raw_wave = FADC500_Channel->GetWaveform();
    for (Int_t i = 0; i < FADC500_Channel_N_Data_Point; i++) {
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
    
    FADC500_TChain->GetEntry(entry);
    FADC500_Entry_N = entry;
    FADC500_N_Hit = FADC500_Event_Info->GetNHit();
    FADC500_Trigger_Type = FADC500_Event_Info->GetTriggerType();
    FADC500_Trigger_N = FADC500_Event_Info->GetTriggerNumber();
    FADC500_Event_N = FADC500_Event_Info->GetEventNumber();
    FADC500_Trigger_Time = FADC500_Event_Info->GetTriggerTime();
    

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
    
    FADC500_Calib_Waveform = new Short_t[FADC500_Channel_N_Data_Point];
    
    const UShort_t *raw_wave = FADC500_Channel->GetWaveform();
    for (Int_t i = 0; i < FADC500_Channel_N_Data_Point; i++) {
        FADC500_Calib_Waveform[i] = raw_wave[i] - FADC500_Channel_Pedestal;
    }
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

