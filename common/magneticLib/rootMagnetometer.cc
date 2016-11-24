// this macro loops through all the adu5 and magnetometer data of one run and graphs the B fields and attitude for measured and calculated.  so far only heading works well

#include "magneticLib.c"
#include <TGraph.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TFile.h>

#include "Adu5Pat.h"
#include "PrettyAnitaHk.h"

void removeSpikes(TGraph * bGraph, double delta)
{
	double prev = bGraph->GetY()[0];
	for (int i = 1; i < bGraph->GetN(); i++)
	{
		if (bGraph->GetY()[i] < (prev - delta) || bGraph->GetY()[i] > (prev + delta)) 
		{
			bGraph->GetY()[i] = prev;
		}
		prev = bGraph->GetY()[i];
	}
}


void rootMagnetometer()
{

	//gROOT->SetBatch(kTRUE);
	
	TCanvas * c1 = new TCanvas("x", "x", 800,600);
	TCanvas * c2 = new TCanvas("y", "y", 800,600);
	TCanvas * c3 = new TCanvas("z", "z", 800,600);

	TCanvas * c4 = new TCanvas("heading", "heading", 800,600);
	TCanvas * c5 = new TCanvas("pitch", "pitch", 800,600);
	TCanvas * c6 = new TCanvas("roll", "roll", 800,600);

	TFile * hkFile = new TFile("/home/abl/runs/root/Anita3Data/prettyHkFile345.root");
	TFile * adu5File = new TFile("/home/abl/runs/root/Anita3Data/gpsEvent345.root");


	TTree * hkTree = (TTree*)hkFile->Get("prettyHkTree");
	TTree * adu5Tree = (TTree*)adu5File->Get("adu5PatTree");

	PrettyAnitaHk * hk = 0;
	Adu5Pat * adu5 = 0;
	UInt_t pt = 0;

	hkTree->SetBranchAddress("hk", &hk);
	adu5Tree->SetBranchAddress("pat", &adu5);

	magnetic_answer_t answer;

	struct tm t;

	double attitude[3];
	double BfieldErr[3] = {-0.1,0.2,-.204}; //a rough guess at calibration
	double attitudeErr[3];

	int entriesNum = hkTree->GetEntries();

	TGraph * bModX = new TGraph(entriesNum);
	TGraph * bModY = new TGraph(entriesNum);
	TGraph * bModZ = new TGraph(entriesNum);

	TGraph * bMagX = new TGraph(entriesNum);
	TGraph * bMagY = new TGraph(entriesNum);
	TGraph * bMagZ = new TGraph(entriesNum);

	TGraph * headingMeas = new TGraph(entriesNum);
	TGraph * pitchMeas = new TGraph(entriesNum);
	TGraph * rollMeas = new TGraph(entriesNum);

	TGraph * headingCalc = new TGraph(entriesNum);
	TGraph * pitchCalc = new TGraph(entriesNum);
	TGraph * rollCalc = new TGraph(entriesNum);

	double bf[3];

	for (int i = 0; i < entriesNum; i++)
	{
		if (i%10000==0) printf("i've run thru %d entries\n", i);
		hkTree->GetEntry(i);
		adu5Tree->GetEntry(i);

		const time_t t0 = hk->payloadTime;
		gmtime_r(&t0, &t);
		double lon = adu5->longitude;
		double lat = adu5->latitude;
		double alt = adu5->altitude;
	
		double pitch = adu5->pitch;
		double roll = adu5->roll;
		double heading = adu5->heading;
		headingMeas->SetPoint(i, i, heading);
		pitchMeas->SetPoint(  i, i, pitch);
		rollMeas->SetPoint(   i, i, roll);
		//printf("measured heading is %f, pitch is %f, roll is %f\n", heading, pitch, roll);

		Float_t * Bfield = hk->magnetometer;
		//double bf[] = {Bfield[0], Bfield[1], Bfield[2]};
		bf[0] = Bfield[0];
		bf[1] = Bfield[1];
		bf[2] = Bfield[2];
		bMagX->SetPoint(i,i,bf[0]);
		bMagY->SetPoint(i,i,bf[1]);
		bMagZ->SetPoint(i,i,bf[2]);
		//printf("magnet reading is x = %f, y = %f, z = %f\n", bf[0], bf[1], bf[2]);
		
		magneticModel("IGRF12.COF", &answer, t.tm_year, t.tm_mon+1, t.tm_mday+1, lon, lat, alt/1000);
		bModX->SetPoint(i,i,answer.N*1e-6);
		bModY->SetPoint(i,i,answer.E*1e-6);
		bModZ->SetPoint(i,i,answer.Z*1e-6);
	}

	printf("filtering out spikes\n");

	removeSpikes(bMagX, .002);
	removeSpikes(bMagY, .002);
	removeSpikes(bMagZ, .002);
		
	//with the filtered magnetometer data, solve for heading, pitch, roll
	for (int i = 0; i < bMagX->GetN(); i++)
	{
		bf[0] = bMagX->GetY()[i];
		bf[1] = bMagY->GetY()[i];
		bf[2] = bMagZ->GetY()[i];

		hkTree->GetEntry(i);
		adu5Tree->GetEntry(i);

		const time_t t0 = hk->payloadTime;
		gmtime_r(&t0, &t);
		double lon = adu5->longitude;
		double lat = adu5->latitude;
		double alt = adu5->altitude;
	

		solveForAttitude(bf, BfieldErr, attitude, attitudeErr, lon, lat, alt, t0);

		bMagX->SetPoint(i, i, attitudeErr[0]);
		bMagY->SetPoint(i, i, attitudeErr[1]);

		headingCalc->SetPoint(i, i, attitude[0]);
		pitchCalc->SetPoint(  i, i, attitude[1]);
		rollCalc->SetPoint(   i, i, attitude[2]);
	}
	
	//calc x is from .178 to .187 ish
	c1->cd();
	bMagX->GetYaxis()->SetRangeUser(.6, .187);
	bMagX->SetLineColor(kRed);
	bModX->SetLineColor(kBlue);
	bMagX->SetTitle("Magnetometer (red) model (blue) X-component");
	bMagX->Draw("al");
	bModX->Draw("lsame");


	//calc Y is from -.0341 to -.0347 ish
	c2->cd();
	bMagY->GetYaxis()->SetRangeUser(-0.347,-.11);
	bMagY->SetLineColor(kRed);
	bModY->SetLineColor(kBlue);
	bMagY->SetTitle("Magnetometer (red) model (blue) Y-component");
	
	bMagY->Draw("al");
	bModY->Draw("lsame");

	c3->cd();
	//bMagZ->GetYaxis()->SetRangeUser(-.17,-.16);
	bMagZ->SetLineColor(kRed);
	bModZ->SetLineColor(kBlue);
	bMagZ->SetTitle("Magnetometer (red) model (blue) Z-component");
	
	bMagZ->Draw("al");
	bModZ->Draw("lsame");

	c4->cd();
	headingCalc->SetLineColor(kRed);
	headingMeas->SetLineColor(kBlue);
	headingMeas->SetTitle("Magnetometer calculated (red) adu5 (blue) heading");
	
	headingMeas->Draw("al");
	headingCalc->Draw("lsame");

	c5->cd();
	pitchCalc->SetLineColor(kRed);
	pitchMeas->SetLineColor(kBlue);
	pitchCalc->SetTitle("Magnetometer calculated (red) adu5 (blue) pitch");
	
	pitchCalc->Draw("al");
	pitchMeas->Draw("lsame");

	c6->cd();
	rollCalc->SetLineColor(kRed);
	rollMeas->SetLineColor(kBlue);
	rollCalc->SetTitle("Magnetometer calculated (red) adu5 (blue) roll");
	
	rollCalc->Draw("al");
	rollMeas->Draw("lsame");
}
