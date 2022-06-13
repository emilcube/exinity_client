#include <iostream>
#include "WebSocketFactory.hpp"
#include "Logger/BasicLogger.hpp"

// thread-safe generator
int intRand(const int& min, const int& max) {
	static thread_local std::mt19937* generator = nullptr;
	if (!generator) generator = new std::mt19937(clock() + std::hash<std::thread::id>()(std::this_thread::get_id()));
	//std::hash<std::thread::id>()(std::this_thread::get_id())
	//this_thread::get_id().hash()
	std::uniform_int_distribution<int> distribution(min, max);
	return distribution(*generator);
}

void for_tests() {

	int n = 1; // amount of clients
	int message_every_n_sec = 10; // each clients sends message every n seconds

	for (int i = 0; i < n; ++i)
	{

		std::thread th([&message_every_n_sec,&i]() {

		std::shared_ptr<BasicLogger> logger = std::make_shared<BasicLogger>();
		logger->AssignFiles("sock_client_log_"+std::to_string(i) + ".log");
		WebSocketFactory::SetDefaultLogger(logger);

		net::io_context ioc;

		auto host = "ws://127.0.0.1";
		auto port = "8083";
		auto sock = WebSocketFactory::GenerateDefault(ioc, host, port);

		sock->Connect(host, port);

		for (;;)
		{
			int val = intRand(0, 1023);
			sock->Write(std::to_string(val));
			//sock->Write("abc|test");
			if (!sock->IsOpen()) break;
			std::string resp;
			auto bstatus = sock->Read(resp);
			std::cout << resp << std::endl;

			std::this_thread::sleep_for(std::chrono::seconds(message_every_n_sec));
		}

		});
	th.detach();
	}

	while (true)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void production() {
	std::shared_ptr<BasicLogger> logger = std::make_shared<BasicLogger>();
	logger->AssignFiles("sock_client_log.log");
	WebSocketFactory::SetDefaultLogger(logger);

	auto host = "ws://127.0.0.1";
	auto port = "8083";

	bool closed = false;
	int attempt = 0;
	while (!closed)
	{
		std::string picked_exchange;
		net::io_context ioc;
		auto sock = WebSocketFactory::GenerateDefault(ioc, host, port);
		sock->Connect(host, port);

		if (!sock->IsOpen())
		{
			std::this_thread::sleep_for(std::chrono::seconds(10));
			std::cout << "\r                                                     ";
			std::cout << "\rReconnection ";
			sock->Close();
			int p = attempt % 5 + 1;
			for (auto k = 0; k < p; ++k) std::cout << ".";
			++attempt;
			continue;
		}

		for (;;)
		{
			std::string getres;
			getline(std::cin, getres);

			sock->Write(getres);

			if (getres == std::string("close")) {
				closed = true;
				break;
			}
			if (!sock->IsOpen()) break;
			std::string resp;
			auto status = sock->Read(resp);
			if (!status) break;
			std::cout << "response: " << resp << std::endl;
		}
	}
}

void main(int argc, char* argv[])
{
#define testing false

#if typeTest == true
	for_tests();
#else
	production();
#endif

	return;
}