/*
   Copyright [2017-2019] [IBM Corporation]
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "ycsb_perf.h"
#include <stdlib.h>
#include <iostream>
#include "../../kvstore/get_cpu_mask_from_string.h"
#include "../../kvstore/stopwatch.h"
#include "db_fact.h"
#include "workload.h"

using namespace std;

int main(int argc, char * argv[])
{
    if(argc<4)
        show_program_options();

    string operation = argv[1];
    string filename=argv[3];

    ifstream input(filename);

    try {
      props.load(input);
    }
    catch (const string &msg) {
      cerr << msg << endl;
      input.close();
      return -1;
    }

    input.close();

    cpu_mask_t cpus;

    try {
      cpus =
          get_cpu_mask_from_string(props.getProperty("cores", "0"));
    }
    catch (...) {
      PERR("%s", "couldn't create CPU mask. Exiting.");
      return 1;
    }

    if (operation == "run") props.setProperty("run", "1");

    Core::Per_core_tasking<ycsb::Workload, Properties &> exp(cpus, props);
    exp.wait_for_all();
    auto first_exp = exp.tasklet(cpus.first_core());
    first_exp->summarize();

    return 0;
}


void show_program_options()
{
    cout<<"./ycsb_perf load/run -P <workload file>"<<endl;
    exit(-1);
}
