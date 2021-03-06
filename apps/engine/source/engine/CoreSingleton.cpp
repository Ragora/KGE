/**
 *  @file InstanceSingleton.cpp
 *  @brief Source code associated with the Kiaro::EngineInstance singleton class.
 *
 *  This software is licensed under the GNU Lesser General Public License version 3.
 *  Please refer to gpl.txt and lgpl.txt for more information.
 *
 *  @author Draconic Entertainment
 *  @version 0.0.0.19
 *  @date 3/19/2014
 *  @copyright (c) 2014 Draconic Entertainment
 */

#include <engine/CoreSingleton.hpp>

#include <irrlicht.h>
#include <CEGUI/RendererModules/Irrlicht/Renderer.h>
#include <CEGUI/CEGUI.h>

#include <engine/Config.hpp>

#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <support/SchedulerSingleton.hpp>
#include <support/Time.hpp>

#include <engine/Logging.hpp>

#include <engine/InputListenerSingleton.hpp>

#include <game/IncomingClient.hpp>
#include <game/OutgoingClientSingleton.hpp>
#include <game/ServerSingleton.hpp>


namespace Kiaro
{
    namespace Engine
    {
        Kiaro::Engine::CoreSingleton *CoreSingleton_Instance = NULL;

        CoreSingleton *CoreSingleton::getPointer(void)
        {
            if (!CoreSingleton_Instance)
                CoreSingleton_Instance = new CoreSingleton;

            return CoreSingleton_Instance;
        }

        void CoreSingleton::destroy(void)
        {
            if (CoreSingleton_Instance)
            {
                CoreSingleton_Instance->kill();
                delete CoreSingleton_Instance;
            }

            CoreSingleton_Instance = NULL;
        }

        void CoreSingleton::setMode(const ENGINE_MODE mode)
        {
            mEngineMode = mode;
        }

        void CoreSingleton::setTargetServer(Kiaro::Common::C8 *address, Kiaro::Common::U16 port)
        {
            mTargetServerAddress = address;
            mTargetServerPort = port;
        }

        void CoreSingleton::setGame(const std::string &gameName)
        {
            mGameName = gameName;
        }

        irr::IrrlichtDevice *CoreSingleton::getIrrlichtDevice(void)
        {
            return mIrrlichtDevice;
        }

        Kiaro::Common::U32 CoreSingleton::run(Kiaro::Common::S32 argc, Kiaro::Common::C8 *argv[])
        {
            mRunning = true;

            std::cout << "EngineInstance: Running game '" << mGameName << "'" << std::endl;

            // Handle Execution Flag
            irr::video::E_DRIVER_TYPE videoDriver = irr::video::EDT_OPENGL;

            // Init ENet
            enet_initialize();

            // Print the linked E-Net version
            ENetVersion enetVersion = enet_linked_version();
            std::cout << "E-Net Version: " << ENET_VERSION_GET_MAJOR(enetVersion) << "." << ENET_VERSION_GET_MINOR(enetVersion) <<
            "." << ENET_VERSION_GET_PATCH(enetVersion) << std::endl;

            switch (mEngineMode)
            {
                case Kiaro::ENGINE_CLIENT:
                {
                    mClient = Kiaro::Game::OutgoingClientSingleton::getPointer();
                    break;
                }
                case Kiaro::ENGINE_CLIENTCONNECT:
                {
                    mClient = Kiaro::Game::OutgoingClientSingleton::getPointer();
                    mClient->connect(mTargetServerAddress, mTargetServerPort, 5000);

                    if (!mClient->isConnected())
                    {
                        std::cerr << "EngineInstance: Failed to connect to remote host with server flag" << std::endl;

                        return 1;
                    }
                    break;
                }
                case Kiaro::ENGINE_DEDICATED:
                {
                    videoDriver = irr::video::EDT_NULL;
                    mServer = Kiaro::Game::ServerSingleton::getPointer("0.0.0.0", 11595, 32);

                    break;
                }
            }

            // Init the Input listener
            Kiaro::Engine::InputListenerSingleton *inputListener = Kiaro::Engine::InputListenerSingleton::getPointer();

            // Start up Irrlicht
            mIrrlichtDevice = irr::createDevice(videoDriver, irr::core::dimension2d<Kiaro::Common::U32>(640, 480), 32, false, false, false, inputListener);
            mIrrlichtDevice->setWindowCaption(L"Kiaro Game Engine");

            // Start up CEGUI (if we're a client)
            if (mEngineMode == Kiaro::ENGINE_CLIENTCONNECT || mEngineMode == Kiaro::ENGINE_CLIENT)
            {
                // We don't need the OS cursor
                mIrrlichtDevice->getCursorControl()->setVisible(false);

                CEGUI::IrrlichtRenderer::bootstrapSystem(*mIrrlichtDevice);

                // Setup our resource directories
                CEGUI::DefaultResourceProvider* resourceProvider = static_cast<CEGUI::DefaultResourceProvider*>( CEGUI::System::getSingleton().getResourceProvider());

                std::string basePath = "./";
                basePath += mGameName + "/";

                resourceProvider->setResourceGroupDirectory("fonts", basePath + "fonts/");
                resourceProvider->setResourceGroupDirectory("ui", basePath + "ui/");
                resourceProvider->setResourceGroupDirectory("global", basePath);

                CEGUI::SchemeManager::getSingleton().createFromFile("TaharezLook.scheme", "ui");
                CEGUI::FontManager::getSingleton().createFromFile( "DejaVuSans-10.font", "fonts" );

                // Set the defaults
                CEGUI::GUIContext &guiContext = CEGUI::System::getSingleton().getDefaultGUIContext();

                guiContext.setDefaultFont( "DejaVuSans-10" );
                guiContext.getMouseCursor().setDefaultImage( "TaharezLook/MouseArrow" );
                guiContext.getMouseCursor().setImage(guiContext.getMouseCursor().getDefaultImage());
            }

            irr::core::dimension2d<irr::u32> lastDisplaySize = mIrrlichtDevice->getVideoDriver()->getScreenSize();

            // Start the Loop
            Kiaro::Common::F32 deltaTimeSeconds = 0.0f;

            while (mRunning && mIrrlichtDevice->run())
            {
                try
                {
                    // Update all our subsystems
                    Kiaro::Support::Time::timer timerID = Kiaro::Support::Time::startTimer();

                    Kiaro::Support::SchedulerSingleton::getPointer()->update();

                    if (mClient)
                    {
                        irr::core::dimension2d<Kiaro::Common::U32> currentDisplaySize = mIrrlichtDevice->getVideoDriver()->getScreenSize();
                        if (lastDisplaySize != currentDisplaySize)
                        {
                            CEGUI::Sizef newDisplaySize(currentDisplaySize.Width, currentDisplaySize.Height);
                            CEGUI::System::getSingleton().notifyDisplaySizeChanged(newDisplaySize);

                            lastDisplaySize = currentDisplaySize;
                        }

                        CEGUI::System::getSingleton().injectTimePulse(deltaTimeSeconds);
                        mClient->update();

                        // Since we're a client, render the frame right after updating
                        mIrrlichtDevice->getVideoDriver()->beginScene(true, true, mClearColor);
                        mIrrlichtDevice->getSceneManager()->drawAll();

                        CEGUI::System::getSingleton().renderAllGUIContexts();

                        mIrrlichtDevice->getVideoDriver()->endScene();
                    }

                    if (mServer)
                        mServer->update();

                    // Make sure that it takes at least 32ms to complete a single tick to help make sync easier, but only
                    // if we're actually running a sim. If we're not, we shouldn't have to enforce the tickrate
                    if (mServer || mClient)
                        boost::this_thread::sleep(boost::posix_time::milliseconds(32));

                    deltaTimeSeconds = Kiaro::Support::Time::stopTimer(timerID);
                }
                catch(std::exception &e)
                {
                    std::cerr << "An internal exception of type '" << typeid(e).name() << "' has occurred: " << e.what() << std::endl;

                    if (mClient)
                        mClient->disconnect();

                    if (mServer)
                    {
                        Kiaro::Network::IncomingClientBase *lastClient = mServer->getLastPacketSender();
                        if (lastClient)
                            lastClient->disconnect();
                    }
                }
            }

            if (mClient)
                mClient->disconnect();
        }

        void CoreSingleton::kill(void)
        {
            if (!mRunning)
                return;

            std::cout << "EngineInstance: Killed via Kill()" << std::endl;

            mRunning = false;
        }

        CoreSingleton::CoreSingleton(void) : mEngineMode(Kiaro::ENGINE_CLIENT), mIrrlichtDevice(0x00), mTargetServerAddress("127.0.0.1"), mTargetServerPort(11595), mClient(NULL), mServer(NULL),
        mRunning(false), mClearColor(Kiaro::Common::ColorRGBA(0, 0, 0, 0))
        {

        }

        CoreSingleton::~CoreSingleton(void)
        {
            std::cout << "EngineInstance: Deinitializing ..." << std::endl;

            // TODO: Check the destroy order
            if (mClient)
            {
                mClient->disconnect();
                Kiaro::Game::OutgoingClientSingleton::destroy();

                mClient = NULL;
            }

            if (mServer)
            {
                Kiaro::Game::ServerSingleton::destroy();

                mServer = NULL;
            }

            Kiaro::Engine::InputListenerSingleton::destroy();
            Kiaro::Support::SchedulerSingleton::destroy();

            mIrrlichtDevice->drop();

            PHYSFS_deinit();
            enet_deinitialize();
        }
    } // End Namespace Engine
} // End Namespace Kiaro
