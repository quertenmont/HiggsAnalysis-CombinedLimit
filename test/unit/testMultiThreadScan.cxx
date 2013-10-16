#include <cmath>
#include <cstdio>
//#include "HiggsAnalysis/CombinedLimit/interface/CascadeMinimzer.h"
#include <thread>
#include <TMath.h>
#include <TFile.h>
#include <TStopwatch.h>
#include <TPluginManager.h>
#include <RooWorkspace.h>
#include <RooRealVar.h>
#include <RooDataSet.h>
#include <RooAbsPdf.h>
#include <RooRandom.h>
#include <RooMinimizer.h>
#include <RooFitResult.h>
#include <RooStats/RooStatsUtils.h>
#include <RooStats/ModelConfig.h>
#include <Math/MinimizerOptions.h>
#include "Math/Minimizer.h"
#include "Math/Factory.h"

void test0(int test) {
    std::vector<RooWorkspace *> wi;
    std::vector<RooAbsData *> di;
    std::vector<RooAbsReal *> nlli;
    std::vector<RooMinimizer *> minims;
    for (int i = 0; i < 5; ++i) {
        wi.push_back(new RooWorkspace("w","w"));
        RooWorkspace *w = wi.back();
        w->factory("x[-5,5]");
        w->factory("Gaussian::pdf(x,mu[0,-1,1], sigma[1])");
        di.push_back(w->pdf("pdf")->generate(RooArgSet(*w->var("x")), 100000));
        nlli.push_back(w->pdf("pdf")->createNLL(*di.back())); 
    }
    std::vector<std::thread> workers;
    if (test == 0) {
        std::cout << "Parallel evaluation of the likelihood: works" << std::endl; 
        for (int i = 0, n = wi.size(); i < n; ++i) {
            RooAbsReal   *nll   = nlli[i];
            workers.push_back(std::thread([nll]() {
                double y = nll->getVal();
            }));
        }
    } else if (test == 1) {
        std::cout << "Sequential minimization of the likelihood (minimizer on the stack): works" << std::endl; 
        for (int i = 0, n = wi.size(); i < n; ++i) {
            RooAbsReal   *nll   = nlli[i];
            //workers.push_back(std::thread([nll]() {
            auto task = [nll]() {
                RooMinimizer minim(*nll);
                minim.minimize("Minuit2","minimize");
           // }));
            }; task();
        }
    } else if (test == 2) {
        std::cout << "Parallel minimization (crash)" << std::endl;
        for (int i = 0, n = wi.size(); i < n; ++i) {
            RooAbsReal   *nll   = nlli[i];
            workers.push_back(std::thread([nll]() {
                RooMinimizer minim(*nll);
                minim.minimize("Minuit2","minimize");
            }));
        }
    } else if (test == 3) {
        std::cout << "Parallel instantiation of the minimizer, on the stack (crash)" << std::endl;
        for (int i = 0, n = wi.size(); i < n; ++i) {
            RooAbsReal   *nll   = nlli[i];
            workers.push_back(std::thread([nll]() {
                RooMinimizer minim(*nll);
            }));
        }
    } else if (test == 4) {
        std::cout << "Parallel instantiation of the minimizer, on the heap, no delete (no crash)" << std::endl;
        for (int i = 0, n = wi.size(); i < n; ++i) {
            RooAbsReal   *nll   = nlli[i];
            workers.push_back(std::thread([nll]() {
                RooMinimizer *minim = new RooMinimizer(*nll);
            }));
        }
    } else if (test == 5) {
        std::cout << "Parallel instantiation of the minimizer, on the heap, running it (error)" << std::endl;
        for (int i = 0, n = wi.size(); i < n; ++i) {
            RooAbsReal   *nll   = nlli[i];
            workers.push_back(std::thread([nll]() {
                RooMinimizer *minim = new RooMinimizer(*nll);
                minim->minimize("Minuit2","migrad");
            }));
        }
    }
    for (auto & w : workers) w.join();
    for (auto minim : minims) delete minim;
}

int main(int argc, char **argv) {
    if (argc > 1)     test0(atoi(argv[1]));
}
