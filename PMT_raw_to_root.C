//before to run this macro you should delete the headers of the data file (~24-25 lines) 
//In WaveFormFunctions.C you should provide the bin widht (time) and number of bins per signal
//Remeber to provide the baseline and signal region for the pulse

#include <stdio.h>
#include <fcntl.h>
#include <TTree.h>
#include <TFile.h>
#include <TNtuple.h>
#include "Riostream.h"

#include <math.h>
#include "TMath.h"
#include <TRandom.h>
#include "TH1.h"
#include "TH2.h"
#include "TF1.h"

#include <string.h>
#include <TStyle.h>
#include <TCanvas.h>
#include "TGraph.h"
#include "TLine.h"
#include "TMultiGraph.h"
#include "TLegend.h"

#include <iostream>
#include <fstream>
#include <string>

#include <stdlib.h> //Atof funtion

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cassert>
#include <cmath>
#include <stdio.h>
#include <cstdlib>


#include "WaveFormFunctions.C"

using namespace std;

const Int_t nChannels = 1;
int iCh = 0;

class Pulse : public TObject {
public:
  float fBase;
  float fMaxBin;
  float fMax;
  float fInt;
  double fInt_Coulomb;
  float fRCharge;
  float fT0_30;

  Pulse() { };
  ClassDef(Pulse,1);
};

// Eliminamos la función analyzeWaveform que causaba problemas

void PMT_raw_to_root3(int dataSampleID, int waveformToAnalyze = 4){

// -----------------------
   //input data sample
   string sample[6]={
       
   /*Vbias 800V*/     
    "40.2V",  //0
    "40.7V",  //1
    "41.2V",  //2
    "Test02", //3
    "Test07", //4
    "Test12", //5
   };
   
   string main_file = Form("RawData/");
   const char * FileID = sample[dataSampleID].c_str();

// ---------- input root file-------------
    std::string const myFile{main_file+FileID+".csv"};
    string fname = myFile.c_str();
// ----------output root file-------------
   const char* outputFile;

   // Corregido: Eliminar main_file de la ruta de salida
   string main_file_output = Form("ROOTtrees/");
   std::string const myString{main_file_output+sample[dataSampleID]+".root"};
   outputFile = myString.c_str();    
// -----------------------
   
   // Crear el directorio ROOTtrees si no existe
   system("mkdir -p ROOTtrees");

    /// tree definitions
    Pulse *B0 = new Pulse();
    Pulse *pPulse[nChannels];
    pPulse[0] = B0;
    TTree tTree("tree","Parameters of pulses in Oscilloscope input channels");
    int run = 1;
    int iEvent;
    tTree.Branch("B0",&B0,10000,1);

    //Reading the input file "fname"
    Int_t events_size,graph_size; //number of events and number of data point
    events_size = 5000;
    vector <vector <double>> Data;
    vector <double> graphM;
    vector <string> row;
    string line, data;
    double step;
    cout <<"open file: "<< fname << endl; // flag to know the code progress
    fstream file (fname, ios::in);
    
    if(!file.is_open()) { // file couldn't be opened
       cout << "Error: Data file "<< fname <<" could not be opened" << endl;
       exit(1);
    }    
    
    // Mejorado: Manejar errores durante la lectura del CSV
    if(file.is_open()){
        bool first_line = true;
        while(getline(file, line)){
            row.clear();
            stringstream str(line);
            while(getline(str, data, ',')){ //"," type of separator of data input
                row.push_back(data);
            }
            
            // Si es la primera línea, inicializar el vector de columnas
            if (first_line) {
                for (size_t i = 0; i < row.size(); i++) {
                    Data.push_back(vector<double>());
                }
                first_line = false;
            }
            
            // Guardar todos los datos
            for (size_t i = 0; i < row.size() && i < Data.size(); i++) {
                try {
                    double val = stod(row[i]);
                    Data[i].push_back(val);
                } catch(const std::exception& e) {
                    cout << "Error converting value in CSV at column " << i << endl;
                    Data[i].push_back(0.0); // Valor por defecto
                }
            }
        }
    }
    
    // Verificar que tenemos suficientes columnas
    if (Data.size() < 2) {
        cout << "Error: File does not have enough columns" << endl;
        return;
    }
    
    step = bin_width;
    graph_size = Data[0].size();
    cout << "number of data points = " << graph_size << endl;
    cout << "number of columns = " << Data.size() << endl;
    
    // Determinar qué columnas tienen datos reales (detectando variación)
    vector<int> validColumns;
    for (size_t col = 1; col < Data.size(); col++) {
        double min_val = 1e9, max_val = -1e9;
        for (size_t i = 0; i < min(size_t(100), Data[col].size()); i++) {
            if (Data[col][i] < min_val) min_val = Data[col][i];
            if (Data[col][i] > max_val) max_val = Data[col][i];
        }
        
        // Si hay variación significativa, considerar válida
        if (max_val - min_val > 0.0001) {
            validColumns.push_back(col);
            cout << "Valid column detected: " << col << endl;
        }
    }
    
    if (validColumns.empty()) {
        cout << "Error: No valid data columns found!" << endl;
        return;
    }
    
    auto *h_WF0 = new TH1F("h_WF0", "waveform", bins_per_record, 0, bin_width*(bins_per_record-1));
    auto *h_WF1 = new TH1F("h_WF1", "waveform", bins_per_record, 0, bin_width*(bins_per_record-1));

    OSC_Record fWaveForm, fWaveFormBL;
    OSC_Record *pfWaveForm[4];
    OSC_Record *pfWaveFormBL[4];
    pfWaveForm[0]    = &fWaveForm;
    pfWaveFormBL[0]  = &fWaveFormBL;

    cout<<"rootification ..."<<endl;

    int iBLfrom[nChannels];     // time window for baseline subrtaction
    int iBLto[nChannels];
    int iPULSEfrom[nChannels];  // time window for pulse
    int iPULSEto[nChannels];

    float fBase[nChannels];     // average baseline determined beforehand

    // Mantener las ventanas de tiempo como en el original
    iBLfrom[0] = 0;  iBLto[0] = 0.20e-6/bin_width;  iPULSEfrom[0] = 0.223e-6/bin_width;  iPULSEto[0] = 0.25e-6/bin_width;   
    
    // Validar que los índices estén dentro de los límites
    if (iBLto[0] >= bins_per_record) iBLto[0] = bins_per_record - 1;
    if (iPULSEfrom[0] >= bins_per_record) iPULSEfrom[0] = bins_per_record - 1;
    if (iPULSEto[0] >= bins_per_record) iPULSEto[0] = bins_per_record - 1;    
    
    
    cout << "Starting analysis for all " << validColumns.size() << "valid waveforms..." << endl;
    for (size_t i = 0; i < validColumns.size(); i++) {
      int columnToAnalyze = validColumns[i];
      // Llenar histograma con la forma de onda seleccionada
      for (int j = 0; j < bins_per_record && j < (int)Data[columnToAnalyze].size(); j++) {
        h_WF0->SetBinContent(j+1, Data[columnToAnalyze][j]);  // Bins empiezan en 1
        if (j < bins_per_record) {
            fWaveForm.data[j] = Data[columnToAnalyze][j];
        }
      }
    
      // Procesar el pulso para la forma de onda seleccionada - USANDO LAS FUNCIONES ORIGINALES
      fBase[iCh] = GetBaseLine(pfWaveForm[iCh], iBLfrom[iCh], iBLto[iCh]);
      pPulse[iCh]->fBase = fBase[iCh];
      SubtractBaseLine(pfWaveForm[iCh], pfWaveFormBL[iCh], fBase[iCh]);
      InvertWaveForm(pfWaveFormBL[iCh], pfWaveFormBL[iCh]);
      pPulse[iCh]->fMaxBin = GetPeakPosition(pfWaveFormBL[iCh], iPULSEfrom[iCh], iPULSEto[iCh]);
      pPulse[iCh]->fMax = GetPeak(pfWaveFormBL[iCh], iPULSEfrom[iCh], iPULSEto[iCh]);
      pPulse[iCh]->fInt = GetIntegral(pfWaveFormBL[iCh], iPULSEfrom[iCh], iPULSEto[iCh], kFALSE);
      pPulse[iCh]->fInt_Coulomb = (GetIntegral(pfWaveFormBL[iCh], iPULSEfrom[iCh], iPULSEto[iCh], kFALSE))*(bin_width/50.);
      pPulse[iCh]->fRCharge = GetRCharge(pfWaveFormBL[iCh], iPULSEfrom[iCh], iPULSEto[iCh]);
      pPulse[iCh]->fT0_30 = GetFrontThresholdPosition(pfWaveFormBL[iCh], iPULSEfrom[iCh], iPULSEto[iCh], 0.3);
    
      tTree.Fill();
    }
    cout << "Analysis finished. Total waveforms processed: " << validColumns.size() << endl;
    
    // Crear el histograma de persistencia mejorado
    TString hName, hTitle;
    hName  = "hPers_"+sample[dataSampleID];
    hTitle = "Persistance "+sample[dataSampleID];
    TH2F *h_WFPer = new TH2F(hName, hTitle,
                            bins_per_record, 0, bin_width*bins_per_record,
                            1000, -1.3, .05);
    
    // Llenar el histograma de persistencia con TODAS las formas de onda válidas
    for (size_t col_idx = 0; col_idx < validColumns.size(); col_idx++) {
        int col = validColumns[col_idx];
        for (int j = 0; j < bins_per_record && j < (int)Data[col].size(); j++) {
            h_WFPer->Fill(j*bin_width, Data[col][j]);
        }
    }


    
    // Debug información
    cout << "Time 0,0=" << Data[0][0] << endl;
    cout << "Time 0,1=" << Data[0][1] << endl;
    cout << "Time 0,2=" << Data[0][2] << endl;
    cout << "Time bin width=" << Data[0][1] - Data[0][0] << endl; 
  
    // Llenar el histograma con la forma de onda procesada
    for (int j = 0; j < bins_per_record; j++) {
        h_WF1->SetBinContent(j+1, fWaveFormBL.data[j]);
    }

    // *** CREAR MULTIGRÁFICO SOLO CON COLUMNAS VÁLIDAS ***
    TMultiGraph *mg = new TMultiGraph();
    mg->SetTitle("Todas las formas de onda;Tiempo (s);Voltaje (V)");
    
    // Limitar a 10 para mantener claridad visual
    int maxToShow = min(10, (int)validColumns.size());
    
    // Crear un gráfico para cada forma de onda válida
    for (int i = 0; i < maxToShow; i++) {
        int col = validColumns[i];
        
        TGraph *gr = new TGraph(Data[0].size());
        
        for (size_t j = 0; j < Data[0].size(); j++) {
            gr->SetPoint(j, Data[0][j], Data[col][j]); // Tiempo real, Voltaje
        }
        
        gr->SetLineColor(i+1); // Diferentes colores (evita el blanco)
        if (i+1 == 10) gr->SetLineColor(kGreen+2);
        
        gr->SetLineWidth(1);
        
        // Nombrar cada gráfico para poder recuperarlo después
        TString graphName = Form("WaveForm_%d", i+1);
        gr->SetName(graphName);
        gr->SetTitle(graphName);
        
        mg->Add(gr, "l"); // "l" para dibujar como línea
    }
    
    // Guardar todos los objetos en el archivo ROOT
    auto f = TFile::Open(outputFile, "RECREATE");
    tTree.Write();
    
    // Guardar el multigráfico
    mg->Write("AllWaveforms");
    
    // También guardar cada gráfico individual del multigráfico
    TList *graphList = mg->GetListOfGraphs();
    if (graphList) {
        TIter next(graphList);
        TGraph *graph;
        while ((graph = (TGraph*)next())) {
            graph->Write();
        }
    }
    
    h_WFPer->Write();
    h_WF0->Write();
    h_WF1->Write();
    
    // Crear un canvas para mostrar todas las formas de onda claramente
    TCanvas *c1 = new TCanvas("c_waveforms", "All Waveforms", 1200, 800);
    mg->Draw("AL");
    gPad->SetGrid();
    
    // Añadir leyenda
    TLegend *leg = new TLegend(0.7, 0.7, 0.9, 0.9);
    if (graphList) {
        TIter next2(graphList);
        TGraph *graph;
        while ((graph = (TGraph*)next2())) {
            leg->AddEntry(graph, graph->GetTitle(), "l");
        }
    }
    leg->Draw();
    
    c1->Write();
    
    // Canvas para h_WF0 y h_WF1 juntos
    TCanvas *c2 = new TCanvas("c_analysis", "Waveform Analysis", 1200, 600);
    c2->Divide(2,1);
    c2->cd(1);
    h_WF0->Draw();
    gPad->SetGrid();
    c2->cd(2);
    h_WF1->Draw();
    gPad->SetGrid();
    c2->Write();
    
    // Canvas para el histograma de persistencia
    TCanvas *c3 = new TCanvas("c_persistence", "Persistence Plot", 1000, 800);
    h_WFPer->Draw("COLZ");
    gPad->SetGrid();
    c3->Write();
    
    f->Close();

    cout << "Processing completed successfully" << endl;
}
