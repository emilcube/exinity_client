#ifndef LOGGER_H__
#define LOGGER_H__
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <queue>
#include <boost/date_time/posix_time/posix_time.hpp>

//#define default_log std::cout
//#define default_err std::cerr

namespace p_time = boost::posix_time;

class ILogger
{
protected:
    ILogger() {};
public:

    /*sets output to filenameBase+extension (messages and errors) and filenameBase+"_Errors"+extension*/
    virtual bool AssignFiles(const std::string& filenameBase, const std::string& extension = ".log") = 0;
    virtual bool AssignFiles(const std::string& filenameBase, const p_time::ptime& time, const std::string& extension = ".log", const p_time::time_duration& recreation_period = p_time::hours(24)) = 0;

    virtual  void ResetFileStreams() = 0;

    virtual void LogMessage(const std::string& sender, const std::string& message, bool alsoLogToConsole = false) = 0;

    virtual void LogMessage(const std::string& sender, const std::stringstream& message, bool alsoLogToConsole = false)
    {
        LogMessage(sender, message.str(), alsoLogToConsole);
    }

    virtual void LogError(const std::string& sender, const  std::string& message, bool alsoLogToConsole = true) = 0;

    virtual void LogError(const std::string& sender, const  std::stringstream& message, bool alsoLogToConsole = false)
    {
        LogError(sender, message.str(), alsoLogToConsole);
    }

    virtual ~ILogger()
    {

    }
};

/*An effective silencer*/
class EmptyLogger :public ILogger
{
public:
    EmptyLogger() {}

    virtual bool AssignFiles(const std::string& filenameBase, const std::string& extension = ".log") override { return true; }
    virtual bool AssignFiles(const std::string& filenameBase, const p_time::ptime& time, const std::string& extension = ".log", const p_time::time_duration& recreation_time = p_time::seconds(-1)) override { return true; }

    virtual  void ResetFileStreams() override {}

    virtual void LogMessage(const std::string& sender, const std::string& message, bool alsoLogToConsole = false) override {}

    virtual void LogMessage(const std::string& sender, const std::stringstream& message, bool alsoLogToConsole = false) override {}

    virtual void LogError(const std::string& sender, const  std::string& message, bool alsoLogToConsole = true) override {}

    virtual void LogError(const std::string& sender, const  std::stringstream& message, bool alsoLogToConsole = false) override {}

    virtual ~EmptyLogger() {}
};

#endif // !LOGGER_H__