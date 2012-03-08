////////////////////////////////////////////////////////////////////////////////
/// @file           CDeviceFactory.cpp
///
/// @author         Thomas Roth <tprfh7@mst.edu>
///                 Michael Catanzaro <michael.catanzaro@mst.edu>
///
/// @project        FREEDM DGI
///
/// @description    Handles the creation of devices and their structures.
///
/// These source code files were created at the Missouri University of Science
/// and Technology, and are intended for use in teaching or research. They may
/// be freely copied, modified and redistributed as long as modified versions
/// are clearly marked as such and this notice is not removed.
///
/// Neither the authors nor Missouri S&T make any warranty, express or implied,
/// nor assume any legal responsibility for the accuracy, completeness or
/// usefulness of these files or any information distributed with these files.
///
/// Suggested modifications or questions about these files can be directed to
/// Dr. Bruce McMillin, Department of Computer Science, Missouri University of
/// Science and Technology, Rolla, MO 65409 <ff@mst.edu>.
////////////////////////////////////////////////////////////////////////////////

#include "CDeviceFactory.hpp"
#include "config.hpp"

namespace freedm
{
namespace broker
{
namespace device
{

////////////////////////////////////////////////////////////////////////////////
/// CDeviceFactory::CDeviceFactory
///
/// @description Constructs the factory. Should only ever be called from
///  CDeviceFactory::instance.
///
/// @pre None.
/// @post Nothing happens. This function only provides access to the factory.
///
/// @return the factory instance.
///
/// @limitations Be sure CDeviceFactory::init has been called on the factory
///  before doing anything with it.
////////////////////////////////////////////////////////////////////////////////
CDeviceFactory::CDeviceFactory()
: m_lineClient(CLineClient::TPointer()),
m_rtdsClient(CClientRTDS::RTDSPointer()), m_manager(0), m_registry(),
m_initialized(false) { }

////////////////////////////////////////////////////////////////////////////////
/// CDeviceFactory::instance
///
/// @description Retrieves the singleton factory instance.
///
/// @pre None.
/// @post Nothing happens. This function only provides access to the factory.
///
/// @return the factory instance.
///
/// @limitations Be sure CDeviceFactory::init has been called on the factory
///  before doing anything with it.
////////////////////////////////////////////////////////////////////////////////
CDeviceFactory & CDeviceFactory::instance()
{
    // Intentionally breaking the rule against static local variables. Instance
    // will be initialized the first time the function is called. No dynamic
    // allocation needed.
    static CDeviceFactory instance;
    return instance;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
////////////////////////////////////////////////////////////////////////////////
/// CDeviceFactory::init
///
/// @description Initializes the device factory with a device manager and
///  networking information. This function should be called once, before the
///  factory is ever used.  For example:
///  <code>CDeviceFactory::instance().init(params)</code>
///
/// @pre Relevant parameters are set appropriately.
/// @post CDeviceFactory::instance() will retrieve the factory instance.
///
/// @param manager the device manager with which this factory should register
///  newly-created devices.
/// @param ios if PSCAD or RTDS is enabled, the IO service for the line client.
/// @param host if PSCAD or RTDS is enabled, the hostname of the machine that
///  runs the simulation.
/// @param port if PSCAD or RTDS is enabled, the port number this DGI and the
///  simulation communicate with.
/// @param xml if RTDS is enabled, the name of the FPGA configuration file.
///
/// @limitations Must be called before anything else is done with this factory.
////////////////////////////////////////////////////////////////////////////////
void CDeviceFactory::init(CPhysicalDeviceManager & manager,
        boost::asio::io_service & ios, const std::string host,
        const std::string port, const std::string xml)
{
    m_manager = &manager;
#if defined USE_DEVICE_PSCAD
    m_lineClient = CLineClient::Create(ios);
    m_lineClient->Connect(host, port);
#elif defined USE_DEVICE_RTDS
    m_rtdsClient = CClientRTDS::Create(ios, xml);
    m_rtdsClient->Connect(host, port);
    m_rtdsClient->Run();
#endif
    m_initialized = true;
}
#pragma GCC diagnostic warning "-Wunused-parameter"

////////////////////////////////////////////////////////////////////////////////
/// CDeviceFactory::CreateDevice
///
/// @description Translates a string into a class type, then creates a new
///  device of this type with the specified identifier.
///
/// @ErrorHandling Insufficiently throws a string if the device type is not
///  registered with the factory, or if the factory is uninitialized.
///
/// @pre The factory has been configured with CDeviceFactory::init.
/// @post Specified device is created and registered with the factory's device
///  manager.
///
/// @param deviceString a string representing the name of the IDevice subclass
///  be created. Should be exactly the same as the portion of the class name
///  after "CDevice".
/// @param deviceID the unique identifier for the device to be created.
///  No other device on this DGI may have this ID.
///
/// @limitations Device classes must properly register themselves before
///  instances can be constructed.
////////////////////////////////////////////////////////////////////////////////
void CDeviceFactory::CreateDevice(const std::string deviceString,
        const Identifier & deviceID)
{
    if (!m_initialized)
    {
        throw "CDeviceFactory::CreateDevice (public) called before init";
    }
    // Ensure the specified device type exists
    if (m_registry.find(deviceString) == m_registry.end())
    {
        std::stringstream ss;
        ss << "Attempted to create device of unregistered type "
                << deviceString.c_str();
        throw ss.str();
    }

    m_registry[deviceString]( deviceID );
}

////////////////////////////////////////////////////////////////////////////////
/// CDeviceFactory::CreateStructure
///
/// @description Creates the internal structure of a device.  Intended to be
///  immediately passed to a device constructor when the device is created by
///  CreateDevice.
///
/// @ErrorHanding Insufficiently throws a string if the factory has not been
///  configured by CDeviceFactory::init.
///
/// @pre factory must be configured by CDeviceFactory::init.
/// @post desired device structure is created and returned.
///
/// @return an internal device structure for PSCAD, RTDS, or generic devices.
///
/// @limitations only PSCAD, RTDS, and generic devices are supported.
////////////////////////////////////////////////////////////////////////////////
IDeviceStructure::DevicePtr CDeviceFactory::CreateStructure()
{
    if (!m_initialized)
    {
        throw "CDeviceFactory::CreateStructure called before init";
    }
#if defined USE_DEVICE_PSCAD
    return IDeviceStructure::DevicePtr(
            new CDeviceStructurePSCAD(m_lineClient));
#elif defined USE_DEVICE_RTDS
    return IDeviceStructure::DevicePtr(
            new CDeviceStructureRTDS(m_rtdsClient));
#else
    return IDeviceStructure::DevicePtr(new CDeviceStructureGeneric());
#endif
}

} // namespace device
} // namespace freedm
} // namespace broker
