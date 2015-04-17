//
// Created by starksm on 4/4/15.
//

#include <map>
#include <stdio.h>
#include <string>
#include <sys/sysinfo.h>

using namespace std;

int main(int argc, char **argv) {
    struct sysinfo info;
    if(sysinfo(&info)) {
        perror("Failed to read sysinfo");
        return -1;
    }
    int mb = 1024*1024;
    int days = info.uptime / (24*3600);
    int hours = (info.uptime - days * 24*3600) / 3600;
    int minute = (info.uptime - days * 24*3600 - hours*3600) / 60;
    char uptime[128];
    sprintf(uptime, "uptime: %lu, days:%d, hrs: %d, min: %d", info.uptime, days, hours, minute);
    printf("%s\n", uptime);
    printf("load average: %.2f, %.2f, %.2f\n", info.loads[0]/65536.0, info.loads[1]/65536.0, info.loads[2]/65536.0);
    printf("mem_unit=%d\n", info.mem_unit);
    printf("Procs: %d\n", info.procs);
    printf("TotalRam: %luMb\n", info.totalram*info.mem_unit / mb);
    printf("AvailableRam: %luMb\n", info.freeram*info.mem_unit / mb);
    printf("FreeHigh: %luMb\n", info.freehigh*info.mem_unit / mb);
    printf("TotalHigh: %luMb\n", info.totalhigh*info.mem_unit / mb);
    printf("SharedRam: %luMb\n", info.sharedram*info.mem_unit / mb);
    printf("FreeSwap: %luMb\n", info.freeswap*info.mem_unit / mb);
    printf("TotalSwap: %luMb\n", info.totalswap*info.mem_unit / mb);

    printf("--- Unscaled:\n");
    printf("TotalRam: %lu\n", info.totalram*info.mem_unit);
    printf("AvailableRam: %lu\n", info.freeram*info.mem_unit);
    printf("FreeHigh: %lu\n", info.freehigh*info.mem_unit);
    printf("TotalHigh: %lu\n", info.totalhigh*info.mem_unit);
    printf("SharedRam: %lu\n", info.sharedram*info.mem_unit);
    printf("FreeSwap: %lu\n", info.freeswap*info.mem_unit);
    printf("TotalSwap: %lu\n", info.totalswap*info.mem_unit);

    printf("--- to_string:\n");
    printf("TotalRam: %s\n", to_string(info.totalram*info.mem_unit / mb).c_str());
    printf("AvailableRam: %s\n", to_string(info.freeram*info.mem_unit / mb).c_str());

    map<string, string> statusProperties;
    string MemTotal("MemTotal");
    string MemActive("MemActive");
    string MemFree("MemFree");
    statusProperties[MemTotal] = to_string(info.totalram*info.mem_unit / mb);
    statusProperties[MemActive] = to_string((info.totalram - info.freeram)*info.mem_unit / mb);
    statusProperties[MemFree] = to_string(info.freeram*info.mem_unit / mb);
    printf("TotalRam: %s\n", statusProperties[MemTotal].c_str());
    printf("MemActive: %s\n", statusProperties[MemActive].c_str());
    printf("MemFree: %s\n", statusProperties[MemFree].c_str());

    return 0;
}