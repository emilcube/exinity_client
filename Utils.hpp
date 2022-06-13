#pragma once 
#include <iostream>
#include <vector>
#include <sstream>

// thread-safe generator
int intRand(const int& min, const int& max) {
	static thread_local std::mt19937* generator = nullptr;
	if (!generator) generator = new std::mt19937(clock() + std::hash<std::thread::id>()(std::this_thread::get_id()));
	//std::hash<std::thread::id>()(std::this_thread::get_id())
	//this_thread::get_id().hash()
	std::uniform_int_distribution<int> distribution(min, max);
	return distribution(*generator);
}

inline bool write_to_file(const std::string& file_data, const std::string& file_name,
	const std::ios_base::openmode& mode)
{
	std::ofstream file;
	file.open(file_name, mode);

	if (file.is_open())
	{
		file << file_data;
		file.close();
		return true;
	}
	else
	{
		//std::cout << "Filed to open/create file: " << file_name << std::endl;
		return false;
	}
}

inline std::string encodeStr(const std::vector<std::string>& vData, const std::string& separator)
{
	std::stringstream s;
	bool first = true;
	for (const auto& p : vData)
	{
		if (first) first = false;
		else s << separator;
		s << p;
	}
	return s.str();
}

class Logger
{
public:

	static void log(const std::string& msg);
	static void log(const std::vector<std::string>& vMsg);
	static void initializeLog();
private:
	static void writeLog(const std::string& filename, const std::string& msg);
};

std::string clientLog = "Client_requests_responses.csv";

inline void Logger::initializeLog()
{
	if (!std::filesystem::exists(clientLog)) {
		const std::string first_mes_event = "#time,request,response\n";
		write_to_file(first_mes_event, clientLog, std::ios_base::app);
	}
	return;
}

inline void Logger::log(const std::string& msg)
{
	Logger::writeLog(clientLog, msg);
}

inline void Logger::log(const std::vector<std::string>& vMsg)
{
	const auto str = encodeStr(vMsg, ",");
	Logger::log(str);
}

inline void Logger::writeLog(const std::string& filename, const std::string& msg)
{
	std::stringstream message;
	message << boost::posix_time::to_simple_string(boost::posix_time::second_clock::universal_time())
		<< "," << msg << std::endl;

	write_to_file(message.str(), filename, std::ios_base::app);
}

