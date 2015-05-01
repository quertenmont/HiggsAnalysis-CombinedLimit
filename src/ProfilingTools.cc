#include "../interface/ProfilingTools.h"

// we try to stick to pure C within the signal handlers
#include <stdio.h>
#include <dlfcn.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <boost/unordered_map.hpp>

void (*igProfRequestDump_)(const char *);
int igProfDumpNumber_ = 0;

void igProfDumpNow(int) {
    char buff[50];
    igProfDumpNumber_++;
    sprintf(buff,"dump.%d.%d.out.gz", getpid(), igProfDumpNumber_);
    igProfRequestDump_(buff);
    fprintf(stderr, "Dumped to %s\n", buff); fflush(stderr);
}

bool setupIgProfDumpHook() {
    if (void *sym = dlsym(0, "igprof_dump_now")) {
        igProfRequestDump_ = __extension__ (void(*)(const char *)) sym;
        fprintf(stderr, "IgProf dump hook enabled. Do kill -USR2 %d to request a dump.\n", int(getpid())); fflush(stderr);
    } else {
        fprintf(stderr, "Not being profiled by IgProf. The command you should use to profile this is:\n"); 
        fprintf(stderr, "  igprof -mp -z -t combine combine datacard [options]\n\n");
        fflush(stderr);
        return false;
    }
    signal(SIGUSR2,igProfDumpNow);
    return true;
}


boost::unordered_map<const char *, PerfCounter> perfCounters_;

PerfCounter & PerfCounter::get(const char *name) 
{
    return perfCounters_[name];
}

void PerfCounter::printAll() 
{
    for (boost::unordered_map<const char *, PerfCounter>::const_iterator it = perfCounters_.begin(), ed = perfCounters_.end(); it != ed; ++it) {
        fprintf(stderr, "%-40s: %g\n", it->first, it->second.get());
    }
}

// we define them by string value, but we lookup them by const char *
namespace runtimedef {
    boost::unordered_map<const char *, std::pair<int,int> > defines_;
    boost::unordered_map<std::string,  int>                 definesByString_;
    int get(const char *name) {
        std::pair<int,int> & ret = defines_[name];
        if (ret.second == 0) {
            ret.first = definesByString_[name];
            ret.second = 1;
        }
        return ret.first;
    }
    int get(const std::string & name) {
        return definesByString_[name];
    }
    void set(const std::string & name, int value) {
        definesByString_[name] = value;
    }
}

#if 1
   #include "FWCore/Utilities/interface/HRRealTime.h"
#else
   namespace edm { inline long long int hrRealTime() { return 0; } }
#endif 

namespace perftimer {
    struct Record { 
        Record() : total(0) {}
        long long int total;
    };
    boost::unordered_map<std::string, Record> records_;
    TimeMe::TimeMe(const std::string & name, const std::string &postfix):
        start_(edm::hrRealTime()),
        record_(records_[(postfix.empty() ? name : name+postfix)])
    {
    }

    TimeMe::~TimeMe() 
    {
        record_.total += (edm::hrRealTime() - start_);
    }

    void printAll() {
        std::vector<std::pair<std::string,double>> recs;
        for (const auto &p : records_) { recs.push_back(std::make_pair(p.first,p.second.total/1.0e6)); }
        std::sort(recs.begin(), recs.end(), [](const std::pair<std::string,double> &p1, const std::pair<std::string,double> &p2)->bool { return p2.second < p1.second; } );
        for (const auto &p : recs) {
            fprintf(stderr, "%-80s: %15.6f\n", p.first.c_str(), p.second);
        }
    }
}
