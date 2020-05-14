/*
	opensesame is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	opensesame is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with opensesame.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <utils/headers.h>
#include <common/general.h>
#include <common/storage.h>
#include <common/private_key.h>
#include <common/argument.h>
#include <common/daemon.h>
#include <overlay/peer_manager.h>
#include <ledger/ledger_manager.h>
#include <consensus/consensus_manager.h>
#include <glue/glue_manager.h>
#include <api/web_server.h>
#include <api/websocket_server.h>
#include <api/console.h>
#include <contract/contract_manager.h>
#include <monitor/monitor_manager.h>
#include "configure.h"

void SaveWSPort();
void RunLoop();
int main(int argc, char *argv[]){

#ifdef WIN32
	_set_output_format(_TWO_DIGIT_EXPONENT);
#else
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	size_t stacksize = 0;
	int ret = pthread_attr_getstacksize(&attr, &stacksize);
	if (ret != 0) {
		printf("get stacksize error!:%d\n", (int)stacksize);
		return -1;
	}

	if (stacksize <= 2 * 1024 * 1024)
	{
		stacksize = 2 * 1024 * 1024;

		pthread_attr_t object_attr;
		pthread_attr_init(&object_attr);
		ret = pthread_attr_setstacksize(&object_attr, stacksize);
		if (ret != 0) {
			printf("set main stacksize error!:%d\n", (int)stacksize);
			return -1;
		}
	}
#endif

	utils::SetExceptionHandle();
	utils::Thread::SetCurrentThreadName("opensesame-thread");

	utils::Daemon::InitInstance();
	utils::net::Initialize();
	utils::Timer::InitInstance();
	opensesame::Configure::InitInstance();
	opensesame::Storage::InitInstance();
	opensesame::Global::InitInstance();
	opensesame::SlowTimer::InitInstance();
	utils::Logger::InitInstance();
	opensesame::Console::InitInstance();
	opensesame::PeerManager::InitInstance();
	opensesame::LedgerManager::InitInstance();
	opensesame::ConsensusManager::InitInstance();
	opensesame::GlueManager::InitInstance();
	opensesame::WebSocketServer::InitInstance();
	opensesame::WebServer::InitInstance();
	opensesame::MonitorManager::InitInstance();
	opensesame::ContractManager::InitInstance();

	opensesame::Argument arg;
	if (arg.Parse(argc, argv)){
		return 1;
	}

	do {
		utils::ObjectExit object_exit;
		opensesame::InstallSignal();

		if (arg.console_){
			arg.log_dest_ = utils::LOG_DEST_FILE; //Cancel the std output
			opensesame::Console &console = opensesame::Console::Instance();
			console.Initialize();
			object_exit.Push(std::bind(&opensesame::Console::Exit, &console));
		}

		srand((uint32_t)time(NULL));
		opensesame::StatusModule::modules_status_ = new Json::Value;
#ifndef OS_MAC
		utils::Daemon &daemon = utils::Daemon::Instance();
		if (!opensesame::g_enable_ || !daemon.Initialize((int32_t)1234))
		{
			LOG_STD_ERRNO("Failed to initialize daemon", STD_ERR_CODE, STD_ERR_DESC);
			break;
		}
		object_exit.Push(std::bind(&utils::Daemon::Exit, &daemon));
#endif

		opensesame::Configure &config = opensesame::Configure::Instance();
		std::string config_path = opensesame::General::CONFIG_FILE;
		if (!utils::File::IsAbsolute(config_path)){
			config_path = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), config_path.c_str());
		}

		if (!config.Load(config_path)){
			LOG_STD_ERRNO("Failed to load configuration", STD_ERR_CODE, STD_ERR_DESC);
			break;
		}

		std::string log_path = config.logger_configure_.path_;
		if (!utils::File::IsAbsolute(log_path)){
			log_path = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), log_path.c_str());
		}
		const opensesame::LoggerConfigure &logger_config = opensesame::Configure::Instance().logger_configure_;
		utils::Logger &logger = utils::Logger::Instance();
		logger.SetCapacity(logger_config.time_capacity_, logger_config.size_capacity_);
		logger.SetExpireDays(logger_config.expire_days_);
		if (!opensesame::g_enable_ || !logger.Initialize((utils::LogDest)(arg.log_dest_ >= 0 ? arg.log_dest_ : logger_config.dest_),
			(utils::LogLevel)logger_config.level_, log_path, true)){
			LOG_STD_ERR("Failed to initialize logger");
			break;
		}
		object_exit.Push(std::bind(&utils::Logger::Exit, &logger));
		LOG_INFO("Initialized daemon successfully");
		LOG_INFO("Loaded configure successfully");
		LOG_INFO("Initialized logger successfully");

		// end run command
		opensesame::Storage &storage = opensesame::Storage::Instance();
		LOG_INFO("The path of the database is as follows: keyvalue(%s),account(%s),ledger(%s)", 
			config.db_configure_.keyvalue_db_path_.c_str(),
			config.db_configure_.account_db_path_.c_str(),
			config.db_configure_.ledger_db_path_.c_str());

		if (!opensesame::g_enable_ || !storage.Initialize(config.db_configure_, arg.drop_db_)) {
			LOG_ERROR("Failed to initialize database");
			break;
		}
		object_exit.Push(std::bind(&opensesame::Storage::Exit, &storage));
		LOG_INFO("Initialized database successfully");

		if (arg.drop_db_) {
			LOG_INFO("Droped database successfully");
			return 1;
		} 
		
		if ( arg.clear_consensus_status_ ){
			opensesame::Pbft::ClearStatus();
			LOG_INFO("Cleared consensus status successfully");
			return 1;
		}

		if (arg.clear_peer_addresses_) {
			opensesame::KeyValueDb *db = opensesame::Storage::Instance().keyvalue_db();
			db->Put(opensesame::General::PEERS_TABLE, "");
			LOG_INFO("Cleared peer addresss list successfully");
			return 1;
		} 

		if (arg.create_hardfork_) {
			opensesame::LedgerManager &ledgermanger = opensesame::LedgerManager::Instance();
			if (!ledgermanger.Initialize()) {
				LOG_ERROR("Failed to initialize legder manger!");
				return -1;
			}
			opensesame::LedgerManager::CreateHardforkLedger();
			return 1;
		}

		opensesame::Global &global = opensesame::Global::Instance();
		if (!opensesame::g_enable_ || !global.Initialize()){
			LOG_ERROR_ERRNO("Failed to initialize global variable", STD_ERR_CODE, STD_ERR_DESC);
			break;
		}
		object_exit.Push(std::bind(&opensesame::Global::Exit, &global));
		LOG_INFO("Initialized global module successfully");

		//Consensus manager must be initialized before ledger manager and glue manager
		opensesame::ConsensusManager &consensus_manager = opensesame::ConsensusManager::Instance();
		if (!opensesame::g_enable_ || !consensus_manager.Initialize(opensesame::Configure::Instance().ledger_configure_.validation_type_)) {
			LOG_ERROR("Failed to initialize consensus manager");
			break;
		}
		object_exit.Push(std::bind(&opensesame::ConsensusManager::Exit, &consensus_manager));
		LOG_INFO("Initialized consensus manager successfully");

		opensesame::LedgerManager &ledgermanger = opensesame::LedgerManager::Instance();
		if (!opensesame::g_enable_ || !ledgermanger.Initialize()) {
			LOG_ERROR("Failed to initialize ledger manager");
			break;
		}
		object_exit.Push(std::bind(&opensesame::LedgerManager::Exit, &ledgermanger));
		LOG_INFO("Initialized ledger successfully");

		opensesame::GlueManager &glue = opensesame::GlueManager::Instance();
		if (!opensesame::g_enable_ || !glue.Initialize()){
			LOG_ERROR("Failed to initialize glue manager");
			break;
		}
		object_exit.Push(std::bind(&opensesame::GlueManager::Exit, &glue));
		LOG_INFO("Initialized glue manager successfully");

		opensesame::PeerManager &p2p = opensesame::PeerManager::Instance();
		if (!opensesame::g_enable_ || !p2p.Initialize(NULL, false)) {
			LOG_ERROR("Failed to initialize peer network");
			break;
		}
		object_exit.Push(std::bind(&opensesame::PeerManager::Exit, &p2p));
		LOG_INFO("Initialized peer network successfully");

		opensesame::SlowTimer &slow_timer = opensesame::SlowTimer::Instance();
		if (!opensesame::g_enable_ || !slow_timer.Initialize(1)){
			LOG_ERROR_ERRNO("Failed to initialize slow timer", STD_ERR_CODE, STD_ERR_DESC);
			break;
		}
		object_exit.Push(std::bind(&opensesame::SlowTimer::Exit, &slow_timer));
		LOG_INFO("Initialized slow timer with " FMT_SIZE " successfully", utils::System::GetCpuCoreCount());

		opensesame::WebSocketServer &ws_server = opensesame::WebSocketServer::Instance();
		if (!opensesame::g_enable_ || !ws_server.Initialize(opensesame::Configure::Instance().wsserver_configure_)) {
			LOG_ERROR("Failed to initialize web server");
			break;
		}
		object_exit.Push(std::bind(&opensesame::WebSocketServer::Exit, &ws_server));
		LOG_INFO("Initialized web server successfully");

		opensesame::WebServer &web_server = opensesame::WebServer::Instance();
		if (!opensesame::g_enable_ || !web_server.Initialize(opensesame::Configure::Instance().webserver_configure_)) {
			LOG_ERROR("Failed to initialize web server");
			break;
		}
		object_exit.Push(std::bind(&opensesame::WebServer::Exit, &web_server));
		LOG_INFO("Initialized web server successfully");

		SaveWSPort();
		
		opensesame::MonitorManager &monitor_manager = opensesame::MonitorManager::Instance();
		if (!opensesame::g_enable_ || !monitor_manager.Initialize()) {
			LOG_ERROR("Failed to initialize monitor manager");
			break;
		}
		object_exit.Push(std::bind(&opensesame::MonitorManager::Exit, &monitor_manager));
		LOG_INFO("Initialized monitor manager successfully");

		opensesame::ContractManager &contract_manager = opensesame::ContractManager::Instance();
		if (!contract_manager.Initialize(argc, argv)){
			LOG_ERROR("Failed to initialize contract manager");
			break;
		}
		object_exit.Push(std::bind(&opensesame::ContractManager::Exit, &contract_manager));
		LOG_INFO("Initialized contract manager successfully");

		opensesame::g_ready_ = true;

		RunLoop();

		LOG_INFO("Process begins to quit...");
		delete opensesame::StatusModule::modules_status_;

	} while (false);

	opensesame::ContractManager::ExitInstance();
	opensesame::SlowTimer::ExitInstance();
	opensesame::GlueManager::ExitInstance();
	opensesame::LedgerManager::ExitInstance();
	opensesame::PeerManager::ExitInstance();
	opensesame::WebSocketServer::ExitInstance();
	opensesame::WebServer::ExitInstance();
	opensesame::MonitorManager::ExitInstance();
	opensesame::Configure::ExitInstance();
	opensesame::Global::ExitInstance();
	opensesame::Storage::ExitInstance();
	utils::Logger::ExitInstance();
	utils::Daemon::ExitInstance();
	
	if (arg.console_ && !opensesame::g_ready_) {
		printf("Initialized failed, please check log for detail\n");
	}
	printf("process exit\n");
}

void RunLoop(){
	int64_t check_module_interval = 5 * utils::MICRO_UNITS_PER_SEC;
	int64_t last_check_module = 0;
	while (opensesame::g_enable_){
		int64_t current_time = utils::Timestamp::HighResolution();

		for (auto item : opensesame::TimerNotify::notifys_){
			item->TimerWrapper(utils::Timestamp::HighResolution());
			if (item->IsExpire(utils::MICRO_UNITS_PER_SEC)){
				LOG_WARN("The execution time(" FMT_I64 " us) for the timer(%s) is expired after 1s elapses", item->GetLastExecuteTime(), item->GetTimerName().c_str());
			}
		}

		utils::Timer::Instance().OnTimer(current_time);
		utils::Logger::Instance().CheckExpiredLog();

		if (current_time - last_check_module > check_module_interval){
			utils::WriteLockGuard guard(opensesame::StatusModule::status_lock_);
			opensesame::StatusModule::GetModulesStatus(*opensesame::StatusModule::modules_status_);
			last_check_module = current_time;
		}

		utils::Sleep(1);
	}
}

void SaveWSPort(){    
    std::string tmp_file = utils::File::GetTempDirectory() +"/opensesame_listen_port";
	Json::Value json_port = Json::Value(Json::objectValue);
	json_port["webserver_port"] = opensesame::WebServer::Instance().GetListenPort();
	json_port["wsserver_port"] = opensesame::WebSocketServer::Instance().GetListenPort();
	utils::File file;
	if (file.Open(tmp_file, utils::File::FILE_M_WRITE | utils::File::FILE_M_TEXT))
	{
		std::string line = json_port.toFastString();
		file.Write(line.c_str(), 1, line.length());
		file.Close();
	}
}
