/*
    This file is part of move-chain.

    move-chain is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    move-chain is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with move-chain.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 *  @author jimmyshi
 *  @modify first draft
 *  @date 2018-11-30
 */


#include "GlobalConfigureInitializer.h"
#include "include/BuildInfo.h"
#include "libsecurity/KeyCenter.h"
#include <libethcore/EVMSchedule.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace std;
using namespace dev;
using namespace dev::initializer;

DEV_SIMPLE_EXCEPTION(UnknowSupportVersion);

uint32_t dev::initializer::getVersionNumber(const string& _version)
{
    // 0XMNNPPTS, M=MAJOR N=MINOR P=PATCH T=TWEAK S=STATUS
    vector<string> versions;
    uint32_t versionNumber = 0;
    boost::split(versions, _version, boost::is_any_of("."));
    if (versions.size() != 3)
    {
        BOOST_THROW_EXCEPTION(UnknowSupportVersion() << errinfo_comment(_version));
    }
    try
    {
        for (size_t i = 0; i < versions.size(); ++i)
        {
            versionNumber += boost::lexical_cast<uint32_t>(versions[i]);
            versionNumber <<= 8;
        }
    }
    catch (const boost::bad_lexical_cast& e)
    {
        INITIALIZER_LOG(ERROR) << LOG_KV("what", boost::diagnostic_information(e));
        BOOST_THROW_EXCEPTION(UnknowSupportVersion() << errinfo_comment(_version));
    }
    return versionNumber;
}

void dev::initializer::initGlobalConfig(const boost::property_tree::ptree& _pt)
{
    /// default version is RC1
    string version = _pt.get<string>("compatibility.supported_version", "2.0.0-rc1");
    uint32_t versionNumber = 0;
    if (dev::stringCmpIgnoreCase(version, "2.0.0-rc1") == 0)
    {
        g_BCOSConfig.setSupportedVersion(version, RC1_VERSION);
    }
    else if (dev::stringCmpIgnoreCase(version, "2.0.0-rc2") == 0)
    {
        g_BCOSConfig.setSupportedVersion(version, RC2_VERSION);
    }
    else if (dev::stringCmpIgnoreCase(version, "2.0.0-rc3") == 0)
    {
        g_BCOSConfig.setSupportedVersion(version, RC3_VERSION);
    }
    else
    {
        versionNumber = getVersionNumber(version);
        g_BCOSConfig.setSupportedVersion(version, static_cast<VERSION>(versionNumber));
    }

    // set evmSchedule
    if (g_BCOSConfig.version() <= getVersionNumber("2.0.0"))
    {
        g_BCOSConfig.setEVMSchedule(dev::eth::FiscoBcosSchedule);
    }
    else
    {
        g_BCOSConfig.setEVMSchedule(dev::eth::FiscoBcosScheduleV2);
    }

    g_BCOSConfig.binaryInfo.version = MOVE_CHAIN_PROJECT_VERSION;
    g_BCOSConfig.binaryInfo.buildTime = MOVE_CHAIN_BUILD_TIME;
    g_BCOSConfig.binaryInfo.buildInfo =
        string(MOVE_CHAIN_BUILD_PLATFORM) + "/" + string(MOVE_CHAIN_BUILD_TYPE);
    g_BCOSConfig.binaryInfo.gitBranch = MOVE_CHAIN_BUILD_BRANCH;
    g_BCOSConfig.binaryInfo.gitCommitHash = MOVE_CHAIN_COMMIT_HASH;

    string sectionName = "data_secure";
    if (_pt.get_child_optional("storage_security"))
    {
        sectionName = "storage_security";
    }

    g_BCOSConfig.diskEncryption.enable = _pt.get<bool>(sectionName + ".enable", false);
    g_BCOSConfig.diskEncryption.keyCenterIP = _pt.get<string>(sectionName + ".key_manager_ip", "");
    g_BCOSConfig.diskEncryption.keyCenterPort =
        _pt.get<int>(sectionName + ".key_manager_port", 20000);
    if (!isValidPort(g_BCOSConfig.diskEncryption.keyCenterPort))
    {
        BOOST_THROW_EXCEPTION(
            InvalidPort() << errinfo_comment("P2PInitializer:  initConfig for storage_security "
                                             "failed! Invalid key_manange_port!"));
    }


    /// compress related option, default enable
    bool enableCompress = _pt.get<bool>("p2p.enable_compress", true);
    g_BCOSConfig.setCompress(enableCompress);

    /// init version
    int64_t chainId = _pt.get<int64_t>("chain.id", 1);
    if (chainId < 0)
    {
        BOOST_THROW_EXCEPTION(
            ForbidNegativeValue() << errinfo_comment("Please set chain.id to positive!"));
    }
    g_BCOSConfig.setChainId(chainId);

    if (g_BCOSConfig.diskEncryption.enable)
    {
        auto cipherDataKey = _pt.get<string>(sectionName + ".cipher_data_key", "");
        if (cipherDataKey.empty())
        {
            BOOST_THROW_EXCEPTION(
                MissingField() << errinfo_comment("Please provide cipher_data_key!"));
        }
        KeyCenter keyClient;
        keyClient.setIpPort(
            g_BCOSConfig.diskEncryption.keyCenterIP, g_BCOSConfig.diskEncryption.keyCenterPort);
        g_BCOSConfig.diskEncryption.cipherDataKey = cipherDataKey;
        g_BCOSConfig.diskEncryption.dataKey = asString(keyClient.getDataKey(cipherDataKey));
        INITIALIZER_LOG(INFO) << LOG_BADGE("initKeyManager")
                              << LOG_KV("url.IP", g_BCOSConfig.diskEncryption.keyCenterIP)
                              << LOG_KV("url.port",
                                     to_string(g_BCOSConfig.diskEncryption.keyCenterPort));
    }

    INITIALIZER_LOG(INFO) << LOG_BADGE("initGlobalConfig")
                          << LOG_KV("enableCompress", g_BCOSConfig.compressEnabled())
                          << LOG_KV("compatibilityVersion", version)
                          << LOG_KV("versionNumber", g_BCOSConfig.version())
                          << LOG_KV("chainId", g_BCOSConfig.chainId());
}

void dev::version()
{
    std::cout << "move-chain Version : " << MOVE_CHAIN_PROJECT_VERSION << std::endl;
    std::cout << "Build Time         : " << MOVE_CHAIN_BUILD_TIME << std::endl;
    std::cout << "Build Type         : " << MOVE_CHAIN_BUILD_PLATFORM << "/"
              << MOVE_CHAIN_BUILD_TYPE << std::endl;
    std::cout << "Git Branch         : " << MOVE_CHAIN_BUILD_BRANCH << std::endl;
    std::cout << "Git Commit Hash    : " << MOVE_CHAIN_COMMIT_HASH << std::endl;
}
