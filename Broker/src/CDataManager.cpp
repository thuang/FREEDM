#include "CDataManager.hpp"
#include "CDeviceManager.hpp"
#include "CLogger.hpp"

#include <stdexcept>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/map.hpp>

namespace freedm {
namespace broker {

namespace {
CLocalLogger Logger(__FILE__);
}

CDataManager & CDataManager::Instance()
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    static CDataManager instance;
    return instance;
}

void CDataManager::AddData(std::string key, float value)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;

    device::CDevice::Pointer clock = device::CDeviceManager::Instance().GetClock();

    if(clock)
    {
        float time = clock->GetState("time");
        std::string path = key + "." + boost::lexical_cast<std::string>(time);
        m_data.add(path, value);

        boost::property_tree::ptree & parent = m_data.get_child(key);
        while(parent.size() > MAX_DATA_ENTRIES)
        {
            Logger.Notice << "Deleted historic data for " << key << " at time " << parent.front().first << std::endl;
            parent.pop_front();
        }
    }
    else
    {
        Logger.Warn << "Historic data not saved because no clock was found" << std::endl;
    }
}

float CDataManager::GetData(std::string key, float time)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    std::string path = key + "." + boost::lexical_cast<std::string>(time);
    return m_data.get<float>(path);
}

void CDataManager::AddFIDState(const std::map<std::string, bool> & fidstate)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    
    device::CDevice::Pointer clock = device::CDeviceManager::Instance().GetClock();

    if(clock)
    {
        float time = clock->GetState("time");
        m_fidstate[time] = fidstate;

        while(m_fidstate.size() > MAX_DATA_ENTRIES)
        {
            Logger.Notice << "Deleted historic data for fidstate at time " << m_fidstate.begin()->first << std::endl;
            m_fidstate.erase(m_fidstate.begin());
        }
    }
    else
    {
        // because fidstate does not update each clock tick
        // the initial topology might be lost without this line
        m_fidstate[0] = fidstate;
    }
}

std::map<std::string, bool> CDataManager::GetFIDState(float time)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    
    if(time < m_fidstate.begin()->first)
    {
        Logger.Notice << "No FID state for time " << time << std::endl;
        throw std::runtime_error("Invalid FID State");
    }

    float matched_time = m_fidstate.begin()->first;

    BOOST_FOREACH(float t, m_fidstate | boost::adaptors::map_keys)
    {
        if(time >= t)
        {
            matched_time = t;
        }
    }
    return m_fidstate[matched_time];
}

} // namespace freedm
} // namespace broker
