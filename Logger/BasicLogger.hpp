#ifndef BASICLOGGER_H__
#define BASICLOGGER_H__
#include "Logger.hpp"
#include <boost/filesystem.hpp>

class BasicLogger : public ILogger
{
protected:
    p_time::ptime _last_creation;
    p_time::time_duration _recreation_period;
    int64_t _last_period = 0;
    std::string _filename, _extension;
    std::ofstream _msgFile, _errFile;
    bool _openedFiles = false;
public:

    BasicLogger()
    {
    }

    virtual bool AssignFiles(const std::string& filenameBase, const std::string& extension = ".log") override
    {
        _extension = extension;

        auto pos = filenameBase.find_last_of('/');
        std::string filePath, fileName;
        if (pos < filenameBase.length())
        {
            filePath = filenameBase.substr(0, pos + 1);
            fileName = filenameBase.substr(pos + 1);
        }
        else
        {
            filePath = "./";
            fileName = filenameBase;
        }

        boost::filesystem::path pt("./" + filePath);
        if (!boost::filesystem::exists(pt) && filePath != "./")
        {
            boost::system::error_code ec;
            boost::filesystem::create_directories(boost::filesystem::current_path()/pt.parent_path(), ec);
            if (ec)
            {
                std::cout << ec.message() << std::endl;
                return false;
            }
        }

        if (_msgFile.is_open())
        {
            std::ofstream tmp;
            if (!TryOpenFile(tmp, filePath + fileName + extension))
            {
                return false;
            }
            else
            {
                _msgFile.close();
                _msgFile = std::move(tmp);
            }
        }
        else
        {
            if (!TryOpenFile(_msgFile, filePath + fileName + extension))
            {
                return false;
            }
        }


        if (_errFile.is_open())
        {
            std::ofstream tmp;
            if (!TryOpenFile(tmp, filePath + fileName + "_Errors" + extension))
            {
                return false;
            }
            else
            {
                _errFile.close();
                _errFile = std::move(tmp);
            }
        }
        else
        {
            if (!TryOpenFile(_errFile, filePath + fileName + "_Errors" + extension))
            {//should not happen unless filenameBase was too long
                _msgFile.close();
                return false;
            }
        }
        _openedFiles = true;
        return true;
    }

    virtual bool AssignFiles(const std::string& filenameBase, const p_time::ptime& time, const std::string& extension = ".log", const p_time::time_duration& recreation_period = p_time::hours(24)) override
    {
        _recreation_period = recreation_period;
        _filename = filenameBase;
        if (recreation_period.total_seconds() > 0)
        {
            _last_creation = time;
        }
        boost::posix_time::time_facet* df = new boost::posix_time::time_facet("%Y.%m.%d_%H%M%S");
        std::stringstream timest;
        timest.imbue(std::locale(timest.getloc(), df));
        timest << time;

        return AssignFiles(filenameBase + timest.str(), extension);
    }

    virtual void ResetFileStreams() override
    {
        if (_openedFiles)
        {
            _msgFile.close();
            _errFile.close();
        }
        _openedFiles = false;
    }

    virtual void LogMessage(const std::string& sender, const  std::string& message, bool alsoLogToConsole = false) override
    {
        auto timeString = p_time::to_simple_string(p_time::second_clock::local_time());
        RecheckFileTime();
        std::string entry = timeString + "," + sender + ",Message: " + message;
        if (_openedFiles)
        {
            _msgFile << entry << std::endl;
        }
        if (alsoLogToConsole)
        {
            std::cout << entry << std::endl;
        }
    }

    virtual void LogError(const std::string& sender, const  std::string& message, bool alsoLogToConsole = true) override
    {
        auto timeString = p_time::to_simple_string(p_time::second_clock::local_time());
        RecheckFileTime();
        std::string entry = timeString + "," + sender + ",Error: " + message;
        if (_openedFiles)
        {
            _msgFile << entry << std::endl;
            _errFile << entry << std::endl;
        }
        if (alsoLogToConsole || !_openedFiles)
        {
            std::cerr << entry << std::endl;
        }
    }

    virtual ~BasicLogger()
    {
        ResetFileStreams();
    }

protected:

    virtual void RecheckFileTime()
    {
        if (_recreation_period.total_seconds() <= 0 || (((p_time::second_clock::local_time() - p_time::ptime(p_time::min_date_time)).total_seconds() % _recreation_period.total_seconds()) >= _last_period))
        {
            if (_recreation_period.total_seconds() > 0)
            {
                _last_period = (p_time::second_clock::local_time() - p_time::ptime(p_time::min_date_time)).total_seconds() % _recreation_period.total_seconds();
            }
            return;
        }
        _last_period = (p_time::second_clock::local_time() - p_time::ptime(p_time::min_date_time)).total_seconds() % _recreation_period.total_seconds();
        AssignFiles(_filename, p_time::second_clock::local_time(), _extension, _recreation_period);
    }

    bool TryOpenFile(std::ofstream& ofs, const std::string& fileName)
    {
        ofs.open(fileName, std::ofstream::out | std::ofstream::app);
        return ofs.is_open();
    }
};

#endif //BASICLOGGER_H__