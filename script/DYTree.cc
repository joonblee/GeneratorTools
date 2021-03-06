#include "TLorentzVector.h"
#include "TFile.h"
#include "TTree.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/FWLite/interface/Handle.h"
#include "DataFormats/FWLite/interface/Event.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/LHEEventProduct.h"

TLorentzVector MakeTLorentzVector(const reco::GenParticle& gen){
  TLorentzVector vec(gen.px(),gen.py(),gen.pz(),gen.energy());
  return vec;
}
void loop(TString infile,TString outfile){
  cout<<"[DYTree] Start post-process"<<endl;
  TFile f(infile);
  fwlite::Event ev(&f);

  TFile fout(outfile,"recreate");
  int l0_pid,l1_pid;
  float l0_px,l0_py,l0_pz,l0_e,l1_px,l1_py,l1_pz,l1_e,weight;
  vector<float> weights;
  TTree* dytree=new TTree("dytree","dytree");
  dytree->Branch("l0_pid",&l0_pid,"l0_pid/I");
  dytree->Branch("l0_px",&l0_px,"l0_px/F");
  dytree->Branch("l0_py",&l0_py,"l0_py/F");
  dytree->Branch("l0_pz",&l0_pz,"l0_pz/F");
  dytree->Branch("l0_e",&l0_e,"l0_e/F");
  dytree->Branch("l1_pid",&l1_pid,"l1_pid/I");
  dytree->Branch("l1_px",&l1_px,"l1_px/F");
  dytree->Branch("l1_py",&l1_py,"l1_py/F");
  dytree->Branch("l1_pz",&l1_pz,"l1_pz/F");
  dytree->Branch("l1_e",&l1_e,"l1_e/F");
  dytree->Branch("weight",&weight,"weight/F");
  dytree->Branch("weights",&weights);

  int ievent=0;
  for(ev.toBegin();!ev.atEnd();++ev){
    if(ievent%1000==0) 
      cout<<ievent<<endl;

    fwlite::Handle<std::vector<reco::GenParticle>> gens_;
    gens_.getByLabel(ev,"genParticles");
    const vector<reco::GenParticle>& gens=*gens_.ptr();
    
    fwlite::Handle<GenEventInfoProduct> geninfo;
    geninfo.getByLabel(ev,"generator");
    const vector<double>& gen_weights=geninfo.ptr()->weights();
    
    fwlite::Handle<LHEEventProduct> lheevent;
    lheevent.getByLabel(ev,"externalLHEProducer");
    const vector<gen::WeightsInfo>& lhe_weights=lheevent.ptr()->weights();
    weights.clear();
    int nweight=lhe_weights.size();
    for(int i=0;i<nweight;i++){
      weights.push_back(lhe_weights[i].wgt);
    }
    
    bool flag_tau=false;
    vector<reco::GenParticle> photons;
    vector<reco::GenParticle> leptons;
    for(int i=0;i<(int)gens.size();i++){
      if(gens[i].isHardProcess()&&abs(gens[i].pdgId())==15){
	flag_tau=true;
	break;
      }
      if(!gens[i].isPromptFinalState()) continue;
      if(gens[i].pdgId()==22) photons.push_back(gens[i]);
      if(abs(gens[i].pdgId())==13||abs(gens[i].pdgId())==11) leptons.push_back(gens[i]);
    }
    if(flag_tau) continue;
    
    if(leptons.size()!=2){
      int l0=0,l1=0;
      for(int i=0;i<(int)leptons.size();i++){
	for(int j=i+1;j<(int)leptons.size();j++){
	  if(leptons[i].pdgId()+leptons[j].pdgId()!=0) continue;
	  if((MakeTLorentzVector(leptons[i])+MakeTLorentzVector(leptons[j])).M()>(MakeTLorentzVector(leptons[l0])+MakeTLorentzVector(leptons[l1])).M()){
	    l0=i;l1=j;
	  }
	}
      }
      for(int i=0;i<(int)leptons.size();i++){
	if(i==l0||i==l1) continue;
	for(int j=i+1;j<(int)leptons.size();j++){
	  if(j==l0||j==l1) continue;
	  if(leptons[i].pdgId()+leptons[j].pdgId()!=0) continue;
	  if(leptons[i].mother()==leptons[j].mother()&&leptons[i].mother()->pdgId()==22){
	    photons.push_back(*((reco::GenParticle*)leptons[i].mother()));
	  }
	}
      }
      leptons={leptons[l0],leptons[l1]};
    }

    vector<TLorentzVector> leptons_vec;
    if(leptons[0].pdgId()>0&&leptons[1].pdgId()<0) leptons_vec={MakeTLorentzVector(leptons[0]),MakeTLorentzVector(leptons[1])};
    else if(leptons[0].pdgId()<0&&leptons[1].pdgId()>0) leptons_vec={MakeTLorentzVector(leptons[1]),MakeTLorentzVector(leptons[0])};
    else {
      cout<<"same sign!!!"<<endl;
      for(int i=0;i<(int)gens.size();i++){
	//if(gens[i].isHardProcess()||gens[i].isPromptFinalState()||gens[i].isPromptDecayed()){
	cout<<i<<" "<<&gens[i]<<" "<<gens[i].mother()<<" "<<gens[i].pdgId()<<"\t"<<gens[i].status()<<"\t"<<gens[i].px()<<"\t"<<gens[i].py()<<"\t"<<gens[i].pz()<<"\t"<<gens[i].energy()<<"\t"<<gens[i].isHardProcess()<<gens[i].isLastCopy()<<gens[i].isLastCopyBeforeFSR()<<gens[i].isPromptDecayed()<<gens[i].isPromptFinalState()<<endl;
	  //}
      }
      cout<<"leptons"<<endl;
      for(int i=0;i<(int)leptons.size();i++){
	cout<<i<<" "<<&leptons[i]<<" "<<leptons[i].mother()<<" "<<leptons[i].pdgId()<<"\t"<<leptons[i].status()<<"\t"<<leptons[i].px()<<"\t"<<leptons[i].py()<<"\t"<<leptons[i].pz()<<"\t"<<leptons[i].energy()<<"\t"<<leptons[i].isHardProcess()<<leptons[i].isLastCopy()<<leptons[i].isLastCopyBeforeFSR()<<leptons[i].isPromptDecayed()<<leptons[i].isPromptFinalState()<<endl;
      }	
      cout<<"photons"<<endl;
      for(int i=0;i<(int)photons.size();i++){
	cout<<i<<" "<<&photons[i]<<" "<<photons[i].mother()<<" "<<photons[i].pdgId()<<"\t"<<photons[i].status()<<"\t"<<photons[i].px()<<"\t"<<photons[i].py()<<"\t"<<photons[i].pz()<<"\t"<<photons[i].energy()<<"\t"<<photons[i].isHardProcess()<<photons[i].isLastCopy()<<photons[i].isLastCopyBeforeFSR()<<photons[i].isPromptDecayed()<<photons[i].isPromptFinalState()<<endl;
      }	
      break;
    }
    for(int i=0;i<(int)photons.size();i++){
      TLorentzVector photon=MakeTLorentzVector(photons[i]);
      if(leptons_vec[0].DeltaR(photon)>0.4&&leptons_vec[1].DeltaR(photon)>0.4) continue;
      if(leptons_vec[0].DeltaR(photon)<leptons_vec[1].DeltaR(photon)) leptons_vec[0]+=photon;
      else leptons_vec[1]+=photon;
    }

    TLorentzVector dilepton=leptons_vec[0]+leptons_vec[1];
    l0_pid=leptons[0].pdgId();
    l0_px=leptons_vec[0].Px();
    l0_py=leptons_vec[0].Py();
    l0_pz=leptons_vec[0].Pz();
    l0_e=leptons_vec[0].E();
    l1_pid=leptons[1].pdgId();
    l1_px=leptons_vec[1].Px();
    l1_py=leptons_vec[1].Py();
    l1_pz=leptons_vec[1].Pz();
    l1_e=leptons_vec[1].E();
    weight=gen_weights.at(0);

    dytree->Fill();

    ievent++;
  }
  f.Close();

  fout.cd();
  dytree->Write();
  fout.Close();
  cout<<"[DYTree] end post-process"<<endl;
}
