// file_watch.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

namespace fs = std::filesystem;

struct destination
{
	std::string address;
	std::string user;
	std::string password;
	std::string path;
};

DWORD open_connection(destination dest);
DWORD close_connection(destination dest);

int main()
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	fs::path from;
	fs::path to;
	std::string name;
	destination dest;

	std::ifstream config;
	config.open("config.ini");
	if (!config.good())
	{
		return 1;
	}
	std::string line;
	while (std::getline(config, line))
	{
		std::cout << line << std::endl;
		if (line.find("FROM=") != std::string::npos)
			from = fs::path(line.substr(line.find_first_of('=') + 1, std::string::npos));
		else if (line.find("TO=") != std::string::npos)
			to = fs::path(line.substr(line.find_first_of('=') + 1, std::string::npos));
		else if (line.find("DEST=") != std::string::npos)
		{
			line = line.substr(line.find_first_of('=') + 1, std::string::npos);
			dest.address = line.substr(0, line.find_first_of(','));
			line = line.substr(line.find_first_of(',') + 1, std::string::npos);
			dest.user = line.substr(0, line.find_first_of(','));
			line = line.substr(line.find_first_of(',') + 1, std::string::npos);
			dest.password = line.substr(0, line.find_first_of(','));
			line = line.substr(line.find_first_of(',') + 1, std::string::npos);
			dest.path = line;
		}
		else if (line.find("NAME=") != std::string::npos)
			name = line.substr(line.find_first_of('=') + 1, std::string::npos);
	}
	std::cout << "Loaded data" << std::endl;
	
	auto code = open_connection(dest);
	if (code != NO_ERROR && code != ERROR_SESSION_CREDENTIAL_CONFLICT)
	{
		LPSTR buf = nullptr;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buf, 0, nullptr);
		std::cout << "Failed to open connection " << dest.address << ". Error code: " << code << " " << buf << std::endl;
		close_connection(dest);

		return 3;
	}

	std::cout << "Connected" << std::endl;

	if (!fs::is_directory(from) || !fs::is_directory(to))
	{
		return 2;
	}
	std::cout << "Directories exist" << std::endl;

	bool exit = false;
	
	while (!exit)
	{
		for (fs::directory_iterator it(from); it != fs::directory_iterator();)
		{
			auto path = *it++;
			if (path.path().filename() == "exit")
				exit = true;
			else if(path.path().filename() == "show")
				ShowWindow(GetConsoleWindow(), SW_SHOW);
			else if(path.path().filename() == "hide")
				ShowWindow(GetConsoleWindow(), SW_HIDE);
			else
			{
				HANDLE fh;
				fh = CreateFile(path.path().c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
				if ((fh != nullptr) && (fh != INVALID_HANDLE_VALUE))
				{
					CloseHandle(fh);
					std::time_t rawtime;
					std::tm timeinfo;

					std::time(&rawtime);
					localtime_s(&timeinfo, &rawtime);
					char buf[80];
					std::strftime(buf, 80, "%Y%m%d %H-%M-%S", &timeinfo);
					auto outpath = to;
					outpath /= name;
					outpath += std::string(buf);
					outpath += path.path().filename();
					if (!fs::exists(outpath))
					{
						try
						{
							fs::copy_file(path, outpath);
							fs::remove(path);
						}
						catch (std::exception& e)
						{
							std::cerr << e.what() << std::endl;
						}
					}
				}
				else
					std::cerr << "File in use " << std::endl;
			}
		}
		Sleep(500);
	}

	return 0;
}

DWORD open_connection(destination dest)
{
	NETRESOURCEA resource;
	resource.dwType = RESOURCETYPE_DISK;
	resource.lpLocalName = 0;
	resource.lpRemoteName = const_cast<LPSTR>(dest.address.c_str());
	resource.lpProvider = 0;
	return WNetAddConnection2A(&resource, const_cast<LPSTR>(dest.password.c_str()), const_cast<LPSTR>(dest.user.c_str()), CONNECT_TEMPORARY);
}

DWORD close_connection(destination dest)
{
	return WNetCancelConnection2A(const_cast<LPSTR>(dest.address.c_str()), 0, 1);
}