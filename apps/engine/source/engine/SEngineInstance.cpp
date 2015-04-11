/**
 *  @file SEngineInstance.cpp
 *  @brief Source code associated with the Kiaro::EngineInstance singleton class.
 *
 *  This software is licensed under the Draconic Free License version 1. Please refer
 *  to LICENSE.txt for more information.
 *
 *  @author Draconic Entertainment
 *  @copyright (c) 2014 Draconic Entertainment
 */

#include <irrlicht.h>
#include <lua.hpp>

#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/Irrlicht/Renderer.h>

#include <engine/config.hpp>
#include <engine/SEngineInstance.hpp>
#include <engine/Logging.hpp>
#include <engine/SInputListener.hpp>

#include <network/SClient.hpp>

#include <network/CClient.hpp>
#include <network/SServer.hpp>

#include <support/FTime.hpp>
#include <engine/SScheduler.hpp>

namespace Kiaro
{
    namespace Engine
    {
        Kiaro::Engine::SEngineInstance *CoreSingleton_Instance = NULL;

        SEngineInstance *SEngineInstance::getPointer(void)
        {
            if (!CoreSingleton_Instance)
                CoreSingleton_Instance = new SEngineInstance;

            return CoreSingleton_Instance;
        }

        void SEngineInstance::destroy(void)
        {
            if (CoreSingleton_Instance)
            {
                CoreSingleton_Instance->kill();
                delete CoreSingleton_Instance;
            }

            CoreSingleton_Instance = NULL;
        }

        void SEngineInstance::setMode(const ENGINE_MODE mode)
        {
            mEngineMode = mode;
        }

        void SEngineInstance::setTargetServer(Kiaro::Common::C8 *address, Kiaro::Common::U16 port)
        {
            mTargetServerAddress = address;
            mTargetServerPort = port;
        }

        void SEngineInstance::setGame(const std::string &gameName)
        {
            mGameName = gameName;
        }

        bool SEngineInstance::isDedicated(void)
        {
            return mEngineMode & Kiaro::ENGINE_DEDICATED;
        }

        irr::IrrlichtDevice *SEngineInstance::getIrrlichtDevice(void)
        {
            return mIrrlichtDevice;
        }

        irr::scene::ISceneManager *SEngineInstance::getSceneManager(void)
        {
            return mSceneManager;
        }

        Kiaro::Common::U32 SEngineInstance::run(Kiaro::Common::S32 argc, Kiaro::Common::C8 *argv[])
        {
            mRunning = true;

            std::cout << "SEngineInstance: Running game '" << mGameName << "'" << std::endl;

            // Add the game search path
            PHYSFS_mount(mGameName.c_str(), NULL, 1);

            // Handle Execution Flag
            irr::video::E_DRIVER_TYPE videoDriver = irr::video::EDT_OPENGL;

            // Init ENet
            enet_initialize();

            // Print the linked E-Net version
            ENetVersion enetVersion = enet_linked_version();
            std::cout << "SEngineInstance: E-Net Version is " << ENET_VERSION_GET_MAJOR(enetVersion) << "." << ENET_VERSION_GET_MINOR(enetVersion) <<
            "." << ENET_VERSION_GET_PATCH(enetVersion) << std::endl;

            // Initialize Lua
            // FIXME (Robert MacGregor#9): Init and call our Lua main method within the try, catch
            mLuaState = luaL_newstate();
            luaL_checkversion(mLuaState);
            lua_gc(mLuaState, LUA_GCSTOP, 0);
            luaL_openlibs(mLuaState);
            lua_gc(mLuaState, LUA_GCRESTART, 0);

            // Now create the callback tables
            lua_createtable(mLuaState, 0, 0);
            lua_setglobal(mLuaState, "GameServer");

            lua_createtable(mLuaState, 0, 0);
            lua_setglobal(mLuaState, "GameClient");

            std::cout << "SEngineInstance: Initialized Lua " << std::endl;

            switch (mEngineMode)
            {
                case Kiaro::ENGINE_CLIENT:
                {
                    mClient = Kiaro::Network::SClient::getPointer();
                    break;
                }
                case Kiaro::ENGINE_CLIENTCONNECT:
                {
                    mClient = Kiaro::Network::SClient::getPointer();
                    mClient->connect(mTargetServerAddress, mTargetServerPort, 5000);

                    if (!mClient->getIsConnected())
                    {
                        std::cerr << "SEngineInstance: Failed to connect to remote host with server flag" << std::endl;

                        Kiaro::Network::SClient::destroy();
                    }

                    break;
                }
                case Kiaro::ENGINE_DEDICATED:
                {
                    videoDriver = irr::video::EDT_NULL;
                    mServer = Kiaro::Network::SServer::getPointer("0.0.0.0", 11595, 32);

                    break;
                }
            }

            // Init the Input listener
            Kiaro::Engine::SInputListener *inputListener = Kiaro::Engine::SInputListener::getPointer();

            // Start up Irrlicht
            mIrrlichtDevice = irr::createDevice(videoDriver, irr::core::dimension2d<Kiaro::Common::U32>(640, 480), 32, false, false, false, inputListener);
            mIrrlichtDevice->setWindowCaption(L"Kiaro Game Engine");

            // Grab the scene manager and store it to reduce a function call
            mSceneManager = mIrrlichtDevice->getSceneManager();

            // Start up CEGUI (if we're a client)
            if (mEngineMode == Kiaro::ENGINE_CLIENTCONNECT || mEngineMode == Kiaro::ENGINE_CLIENT)
            {
                // We don't need the OS cursor
                mIrrlichtDevice->getCursorControl()->setVisible(false);

                CEGUI::IrrlichtRenderer::bootstrapSystem(*mIrrlichtDevice);

                // Setup our resource directories
                // TODO (Robert MacGregor#9): Build a resource provider for CEGUI that implements PhysFS
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

            // Load up the main file
            // TODO: Implement PhysFS in Lua
            const char *mainFileBase = PHYSFS_getRealDir("main.lua");
            if (!mainFileBase)
            {
                std::cerr << "CoreSingleton: Failed to locate main.lua file!" << std::endl;
                return 1;
            }

            std::string mainLuaFile = mainFileBase;
            mainLuaFile += "/main.lua";

            if (luaL_dofile(mLuaState, mainLuaFile.c_str()) >= 1)
            {

               std::cout << "Failed to load main.lua!" << std::endl;
            }
            // Call the main(argv) method
            lua_getglobal(mLuaState, "main");
            lua_newtable(mLuaState);

            for (Kiaro::Common::S32 iteration = 0; iteration < argc; iteration++)
            {
                lua_pushinteger(mLuaState, iteration + 1);
                lua_pushstring(mLuaState, argv[iteration]);
                lua_settable(mLuaState, -3);
            }
            lua_call(mLuaState, 1, 0);

            // What is our current display size?
            irr::core::dimension2d<irr::u32> lastDisplaySize = mIrrlichtDevice->getVideoDriver()->getScreenSize();

            // Start the Loop
            Kiaro::Common::F32 deltaTimeSeconds = 0.0f;

            // Setup and start the network time pulses
            EngineTimePulseDelegate *timePulse = new EngineTimePulseDelegate(new EngineTimePulseDelegate::MemberDelegateType<SEngineInstance>(this, &SEngineInstance::networkUpdate));
            Kiaro::Support::SScheduler::getPointer()->schedule(timePulse, ENGINE_TICKRATE, true);

            // The GUI and sound systems run indepedently of our network time pulse
            while (mRunning && mIrrlichtDevice->run())
            {
                try
                {
                    // Update all our subsystems
                    Kiaro::Support::FTime::timer timerID = Kiaro::Support::FTime::startTimer();

                    Kiaro::Support::SScheduler::getPointer()->update();

                    // Send a time pulse to all the client end functionality
                    if (mEngineMode == ENGINE_CLIENT || mEngineMode == ENGINE_CLIENTCONNECT)
                    {
                        irr::core::dimension2d<Kiaro::Common::U32> currentDisplaySize = mIrrlichtDevice->getVideoDriver()->getScreenSize();
                        if (lastDisplaySize != currentDisplaySize)
                        {
                            CEGUI::Sizef newDisplaySize(currentDisplaySize.Width, currentDisplaySize.Height);
                            CEGUI::System::getSingleton().notifyDisplaySizeChanged(newDisplaySize);

                            lastDisplaySize = currentDisplaySize;
                        }

                        CEGUI::System::getSingleton().injectTimePulse(deltaTimeSeconds);

                        // Since we're a client, render the frame right after updating
                        mIrrlichtDevice->getVideoDriver()->beginScene(true, true, mClearColor);
                        mSceneManager->drawAll();

                        CEGUI::System::getSingleton().renderAllGUIContexts();

                        mIrrlichtDevice->getVideoDriver()->endScene();
                    }

                    deltaTimeSeconds = Kiaro::Support::FTime::stopTimer(timerID);
                }
                catch(std::exception &e)
                {
                    std::cerr << "SEngineInstance: An internal exception of type '" << typeid(e).name() << "' has occurred: \"" << e.what() << "\"" << std::endl;

                    // Something is probably up, we should leave.
                    if (mClient)
                    {
                        mClient->disconnect();
                        mClient = NULL;

                        Kiaro::Network::SClient::destroy();
                    }

                    // Servers just drop off the client that it last processed
                    if (mServer)
                    {
                        Kiaro::Network::CClient *lastClient = mServer->getLastPacketSender();

                        if (lastClient)
                            lastClient->disconnect("Internal Exception");
                    }
                }
            }
        }

        void SEngineInstance::kill(void)
        {
            if (!mRunning)
                return;

            //if (mServer)
            //{
            //    mServer->
           // }

            std::cout << "SEngineInstance: Killed via Kill()" << std::endl;

            mRunning = false;
        }

        SEngineInstance::SEngineInstance(void) : mEngineMode(Kiaro::ENGINE_CLIENT), mIrrlichtDevice(NULL), mTargetServerAddress("127.0.0.1"), mTargetServerPort(11595), mClient(NULL), mServer(NULL),
        mRunning(false), mClearColor(Kiaro::Common::ColorRGBA(0, 0, 0, 0)), mLuaState(NULL)
        {

        }

        SEngineInstance::~SEngineInstance(void)
        {
            std::cout << "SEngineInstance: Deinitializing ..." << std::endl;

            // TODO: Check the destroy order
            if (mClient)
            {
                mClient->disconnect();
                Kiaro::Network::SClient::destroy();

                mClient = NULL;
            }

            if (mServer)
            {
                Kiaro::Network::SServer::destroy();

                mServer = NULL;
            }

            Kiaro::Engine::SInputListener::destroy();
            Kiaro::Support::SScheduler::destroy();

            if (mIrrlichtDevice)
            {
                mIrrlichtDevice->drop();
                mIrrlichtDevice = NULL;
            }

            if (mLuaState)
            {
                lua_close(mLuaState);
                mLuaState = NULL;
            }

            PHYSFS_deinit();
            enet_deinitialize();
        }

        void SEngineInstance::networkUpdate(void)
        {
            // FIXME (Robert MacGregor#9): Pass in the time delta
            if (mClient)
                mClient->networkUpdate();

            if (mServer)
                mServer->networkUpdate(0.0f);
        }

        lua_State *SEngineInstance::getLuaState(void)
        {
            return mLuaState;
        }
    } // End Namespace Engine
} // End Namespace Kiaro
