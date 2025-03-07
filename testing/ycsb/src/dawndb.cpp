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
#include "dawndb.h"
#include <common/cpu.h>
#include <common/str_utils.h>
#include <core/dpdk.h>
#include <core/task.h>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>

using namespace Component;
using namespace Common;
using namespace std;
using namespace ycsb;

/*
Component::IKVStore::pool_t pool;
Stopwatch                   timer;
*/

DawnDB::DawnDB(Properties &props, unsigned core) { init(props, core); }
DawnDB::~DawnDB()
{
  clean();
}

void DawnDB::init(Properties &props, unsigned core)
{
  Component::IBase *comp = Component::load_component(
      "libcomanche-dawn-client.so", dawn_client_factory);

  IKVStore_factory *fact =
      (IKVStore_factory *) comp->query_interface(IKVStore_factory::iid());
  string username = "luna";
  string address  = props.getProperty("address");
  string dev      = props.getProperty("dev");
  int    debug    = stoi(props.getProperty("debug_level", "1"));
  client          = fact->create(debug, username, address, dev);
  fact->release_ref();
  pool = client->open_pool("table" + to_string(core), 0);

  if (pool == Component::IKVStore::POOL_ERROR) {
    /* ok, try to create pool instead */
    pool = client->create_pool("table" + to_string(core), GB(1));
  }
}

int DawnDB::get(const string &table,
                const string &key,
                char *        value,
                bool          direct)
{
  int ret = 0;
  /* open or create pool */
  /*
  Component::IKVStore::pool_t pool = client->open_pool(table, 0);

  if (pool == Component::IKVStore::POOL_ERROR) {
    pool = client->create_pool(table, GB(1));
  }
  if (direct) {
    size_t length = strlen(value);
    auto   handle = client->register_direct_memory(value, length);
    ret           = client->get_direct(pool, key, value, length, handle);
    client->unregister_direct_memory(handle);
  }
  else {
  */
  void * b   = nullptr;
  size_t len = 0;
  ret        = client->get(pool, key, b, len);
  memcpy(value, b, len);
  client->free_memory(b);

  // client->close_pool(pool);
  return ret;
}

int DawnDB::put(const string &table,
                const string &key,
                const string &value,
                bool          direct)
{
  /*
  Component::IKVStore::pool_t pool = client->open_pool(table, 0);

  if (pool == Component::IKVStore::POOL_ERROR) {
    pool = client->create_pool(table, GB(1));
  }

  if (direct) {
    auto handle = client->register_direct_memory(value, length);
    ret         = client->put_direct(pool, key, value, length, handle);
    client->unregister_direct_memory(handle);
  }
  else {
  */
  int ret = client->put(pool, key, value.c_str(), value.length());
  // cout << "in db:" << timer.get_lap_time_in_seconds() << endl;
  // client->close_pool(pool);
  return ret;
}

int DawnDB::update(const string &table,
                   const string &key,
                   const string &value,
                   bool          direct)
{
  return put(table, key, value, direct);
}

int DawnDB::erase(const string &table, const string &key)
{
  /*
  Component::IKVStore::pool_t pool = client->open_pool(table, 0);

  if (pool == Component::IKVStore::POOL_ERROR) {
    pool = client->create_pool(table, GB(1));
  }
  */
  int ret = client->erase(pool, key);
  // client->close_pool(pool);
  return ret;
}

int DawnDB::scan(const string &                table,
                 const string &                key,
                 int                           count,
                 vector<pair<string, string>> &results)
{
  return -1;
}

void DawnDB::clean()
{
  client->close_pool(pool);
  client->release_ref();
}
