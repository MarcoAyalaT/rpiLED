// Notes for the correct implementation of this funtions:
// 1. Always check if the variables bins_per_record and bin_width are in accordance with your data
// 2. For the funtion GetBaseLine look at the region and choose the correct values for bl_from and bl_to

#include "TH1.h"
#include "TF1.h"

int const bins_per_record = 6392; //4395 // number of points per waveform, the value of the Record Length:3125
float const bin_width = 7.82e-11; // here is the Sample Interval, better if you use nanoseconds (ns) 3.20000000e-10

float const bl_from = -0.04; // choose the MIN value for the baseline region
float const bl_to = 0.04; // choose the MAX value for the baseline region

typedef struct {
  std::array<float,bins_per_record> data;
} OSC_Record;

int CopyWaveForm(OSC_Record* pWFin, OSC_Record* pWFout) {
  for (int i=0; i<bins_per_record; i++) pWFout->data[i] = pWFin->data[i];
  return 0;
}

void InvertWaveForm(OSC_Record* pWFin, OSC_Record* pWFout) {
  for (int i=0; i<bins_per_record; i++) pWFout->data[i] = -pWFin->data[i];
}

int SumUpWaveForms(OSC_Record* pWFin_1, OSC_Record* pWFin_2, OSC_Record* pWFout) {
  for (int i=0; i<bins_per_record; i++) pWFout->data[i] = pWFin_1->data[i] + pWFin_2->data[i];
  return 0;
}


int CheckChannelRange(int base_from, int base_to) {
  if(base_from < 0)    { cout << "unexpected first bin for base " << base_from << endl; return -1; }
  if(base_from > bins_per_record) { cout << "unexpected first bin for base " << base_from << endl; return -1; }
  if(base_to > bins_per_record)   { cout << "unexpected last bin for base " << base_to << endl; return -1; }
  if(base_to < 0)      { cout << "unexpected last bin for base " << base_to << endl; return -1; }
  if(base_from > base_to) { 
    cout << "unexpected interval for base: " << base_from << " .. " << base_to << endl; 
    return -1;
  } 
  return 0;
}

float GetBaseLine(OSC_Record* pWFin, int base_from, int base_to) {

  if(CheckChannelRange(base_from, base_to) != 0) return -100000.;

  int iBaseFrom = base_from;
  int iBaseTo = base_to;

  TH1F  *hTmp = new TH1F("hTmp","Pedestal",500,bl_from,bl_to);
  int i;
  float base = 0.;
  for (i=iBaseFrom; i<=iBaseTo; i++) { hTmp->Fill(pWFin->data[i]); }

  Double_t iBin = hTmp->GetMean();
  hTmp->Delete();

  return iBin;

}

void SubtractBaseLine(OSC_Record* pWFin, OSC_Record* pWFout, float base) {
  for (int i=0; i<bins_per_record; i++) pWFout->data[i] = pWFin->data[i] - base;
  return;
}

float SubtractBaseLine(OSC_Record* pWFin, OSC_Record* pWFout, int base_from, int base_to) {

  float base = GetBaseLine(pWFin, base_from, base_to);
  for (int i=0; i<bins_per_record; i++) pWFout->data[i] = pWFin->data[i] - base;
  return base;
}


float GetIntegral(OSC_Record* rec, int from, int to, bool only_positive = kFALSE) {

  // if only_positive is true, negative part of the pulse is not added to the sum
  // in this case one has to expect slight additional baseline shift

  if(CheckChannelRange(from, to) != 0) return -1000000.;

  float sum = 0.;
  for (int i=from; i<=to && i<bins_per_record; i++) { 
    sum = sum + rec->data[i];
    if(only_positive && (rec->data[i] < 0)) sum = sum - rec->data[i];
  }
  return sum;

}

float GetRCharge(OSC_Record* rec, int from, int to) {
//If the integral is correctly computed this value will be zero

  if(CheckChannelRange(from, to) != 0) return -1000000.;

  float sum = 0.;
  for (int i=from; i<=to; i++) { 
    sum = sum + rec->data[i];
    sum = sum - rec->data[i];
  }
  return sum;

}


int GetPeakPosition(OSC_Record* rec, int from, int to, int sign = 1) {

  int iFrom = from;
  int iTo = to;

  int iMinPosition = 0;
  int iMaxPosition = 0;

  float max = -5000.;
  float min =  5000.;

  for (int i=iFrom; i<=iTo; i++) { 
    if (max<rec->data[i]) { max = rec->data[i]; iMaxPosition = i; }
    if (min>rec->data[i]) { min = rec->data[i]; iMinPosition = i; }
  }

  if(sign < 0) return iMinPosition;
  return iMaxPosition; 

}


float GetPeak(OSC_Record* rec, int from, int to, int sign = 1) {

  int iPeak = GetPeakPosition(rec, from, to, sign);

  return rec->data[iPeak];

}


float GetFrontThresholdPosition(OSC_Record *rec, int from, int to, float threshold) {

  // WaveForm is expected to have a positive pulse and a substracted baseline
  // before calling use SubstractBaseline() and InvertWaveForm() if needed

  // threshold is a fraction from the peak value, should be from 0.05 to 0.95

  if((threshold < 0.05) || (threshold > 0.95)) {
    cout << "GetFrontThresholdPosition() error. Unexpected threshold value " << threshold << endl;
    return -1.;
  }

  int   iPeak = GetPeakPosition(rec, from, to);
  float fThreshold = rec->data[iPeak]*threshold;

  float front;
  int   i, iAbove, iBelow;

  iBelow = from;
  for (i=iPeak; i>=from; i--) {
    if ((rec->data[i] < fThreshold) && (rec->data[i+1] >= fThreshold)) {
      iBelow = i;
      break;
    }
  }
  iAbove = iPeak;
  for (i=iPeak; i>=from; i--) {
    if ((rec->data[i-1] <= fThreshold) && (rec->data[i] > fThreshold)) {
      iAbove = i;
      break;
    }
  }
  front = iBelow + (iAbove-iBelow)*(fThreshold - rec->data[iBelow])/(rec->data[iAbove]-rec->data[iBelow]);

  return front;
}



