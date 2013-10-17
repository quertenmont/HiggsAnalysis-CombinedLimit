#include <cmath>
#include <cstdio>
#include "HiggsAnalysis/CombinedLimit/interface/RooMinimizerOpt.h"
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
#include <RooSentinel.h>
#include <Math/MinimizerOptions.h>
#include "Math/Minimizer.h"
#include "Math/Factory.h"

void runMinim(RooAbsReal *nll) {
    RooMinimizer minim(*nll);
    minim.minimize("Minuit2","minimize");
    double y = nll->getVal();
}
void runMinimOpt(RooAbsReal *nll) {
    RooMinimizerOpt minim(*nll);
    minim.minimize("Minuit2","minimize");
    double y = nll->getVal();
}



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
        RooSentinel::activate(); 
        for (int i = 0, n = wi.size(); i < n; ++i) {
            RooAbsReal   *nll   = nlli[i];
            workers.push_back(std::thread([nll]() {
                RooMinimizer minim(*nll);
                double y = nll->getVal();
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
    } else if (test == 6) {
        std::cout << "Parallel minimizer, no lambdas" << std::endl;
        RooSentinel::activate(); 
        for (int i = 0, n = wi.size(); i < n; ++i) {
            workers.push_back(std::thread(runMinim, nlli[i]));
        }
    } else if (test == 7) {
        std::cout << "Parallel minimizerOpt, no lambdas" << std::endl;
        RooSentinel::activate(); 
        for (int i = 0, n = wi.size(); i < n; ++i) {
            workers.push_back(std::thread(runMinimOpt, nlli[i]));
        }
    } else if (test == 8) {
        std::cout << "Pre-instantiation of the minimizerOpt on the heap, running it in parallel (error)" << std::endl;
        minims.resize(wi.size());
        for (int i = 0, n = wi.size(); i < n; ++i) {
            minims[i] = new RooMinimizerOpt(*nlli[i]);
            RooAbsReal *nll = nlli[i];
            RooMinimizer *minim = minims[i];
            double y = nll->getVal(); // trigger one getval
            workers.push_back(std::thread([nll,minim]() {
                minim->minimize("Minuit2","migrad");
                double y2 = nll->getVal(); // trigger one getval
            }));
        }
    }
    for (auto & w : workers) w.join();
    for (auto minim : minims) delete minim;
}

int main(int argc, char **argv) {
    if (argc > 1)     test0(atoi(argv[1]));
}
